/*
 * Copyright (c) 2026, Rauli Laine
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cstring>
#include <memory>
#include <utility>

#include <gio/gio.h>

#include <selain/protocol/finger/client.hpp>

namespace selain::protocol::finger
{
  static constexpr int default_port = 79;
  static constexpr gsize read_chunk_size = 16 * 1024;

  namespace
  {
    struct FetchState
    {
      Request request;
      FetchCallback callback;
      GSocketClient* client = nullptr;
      GSocketConnection* connection = nullptr;
      GInputStream* input = nullptr;
      GOutputStream* output = nullptr;
      std::string request_bytes;
      gsize request_offset = 0;
      GByteArray* buffer = nullptr;
    };

    using FetchStatePtr = std::unique_ptr<FetchState>;

    void start_fetch(FetchStatePtr state);
    void read_more(FetchStatePtr state);
    void write_more(FetchStatePtr state);

    void
    complete(FetchStatePtr state, Response response)
    {
      if (state->connection)
      {
        g_io_stream_close(G_IO_STREAM(state->connection), nullptr, nullptr);
      }

      auto callback = std::move(state->callback);

      if (state->buffer)
      {
        g_byte_array_unref(state->buffer);
        state->buffer = nullptr;
      }
      g_clear_object(&state->connection);
      g_clear_object(&state->client);

      if (callback)
      {
        callback(std::move(response));
      }
    }

    void
    fail(FetchStatePtr state, const std::string& message)
    {
      Response response;

      response.uri = state->request.uri;
      response.error = message;
      complete(std::move(state), std::move(response));
    }

    void
    finish_success(FetchStatePtr state)
    {
      Response response;

      response.uri = state->request.uri;

      if (state->buffer && state->buffer->len > 0)
      {
        response.body.assign(
          state->buffer->data,
          state->buffer->data + state->buffer->len
        );
      }

      complete(std::move(state), std::move(response));
    }

    void
    on_body_read(GObject* source, GAsyncResult* result, gpointer data)
    {
      FetchStatePtr state(static_cast<FetchState*>(data));
      GError* error = nullptr;
      GBytes* bytes = g_input_stream_read_bytes_finish(
        G_INPUT_STREAM(source),
        result,
        &error
      );

      if (!bytes)
      {
        const std::string message = error && error->message
          ? error->message
          : "Failed to read Finger response";

        if (error)
        {
          g_error_free(error);
        }
        fail(std::move(state), message);

        return;
      }

      gsize size = 0;
      const auto* chunk = static_cast<const std::uint8_t*>(
        g_bytes_get_data(bytes, &size)
      );

      if (size == 0)
      {
        g_bytes_unref(bytes);
        finish_success(std::move(state));

        return;
      }

      g_byte_array_append(state->buffer, chunk, static_cast<guint>(size));
      g_bytes_unref(bytes);
      read_more(std::move(state));
    }

    void
    read_more(FetchStatePtr state)
    {
      auto* input = state->input;
      auto* raw = state.release();

      g_input_stream_read_bytes_async(
        input,
        read_chunk_size,
        G_PRIORITY_DEFAULT,
        nullptr,
        on_body_read,
        raw
      );
    }

    void
    on_request_written(GObject* source, GAsyncResult* result, gpointer data)
    {
      FetchStatePtr state(static_cast<FetchState*>(data));
      GError* error = nullptr;
      const gssize written = ::g_output_stream_write_finish(
        G_OUTPUT_STREAM(source),
        result,
        &error
      );

      if (written < 0)
      {
        const std::string message = error && error->message
          ? error->message
          : "Failed to send Finger request";

        if (error)
        {
          g_error_free(error);
        }
        fail(std::move(state), message);

        return;
      }

      state->request_offset += static_cast<gsize>(written);
      if (state->request_offset < state->request_bytes.size())
      {
        write_more(std::move(state));

        return;
      }

      state->buffer = g_byte_array_new();
      read_more(std::move(state));
    }

    void
    write_more(FetchStatePtr state)
    {
      auto* output = state->output;
      const auto* data = state->request_bytes.data() + state->request_offset;
      const auto remaining = state->request_bytes.size() - state->request_offset;
      auto* raw = state.release();

      g_output_stream_write_async(
        output,
        data,
        remaining,
        G_PRIORITY_DEFAULT,
        nullptr,
        on_request_written,
        raw
      );
    }

    void
    on_connected(GObject* source, GAsyncResult* result, gpointer data)
    {
      FetchStatePtr state(static_cast<FetchState*>(data));
      GError* error = nullptr;
      auto* connection = ::g_socket_client_connect_to_host_finish(
        G_SOCKET_CLIENT(source),
        result,
        &error
      );

      if (!connection)
      {
        const std::string message = error && error->message
          ? error->message
          : "Failed to connect";

        if (error)
        {
          g_error_free(error);
        }
        fail(std::move(state), message);

        return;
      }

      state->connection = connection;
      state->input = g_io_stream_get_input_stream(G_IO_STREAM(connection));
      state->output = g_io_stream_get_output_stream(G_IO_STREAM(connection));
      write_more(std::move(state));
    }

    std::string
    build_wire_request(const Request& request)
    {
      return request.query + "\r\n";
    }

    void
    start_fetch(FetchStatePtr state)
    {
      if (state->request.host.empty())
      {
        fail(std::move(state), "URI must be a finger:// URL with a host");

        return;
      }

      state->request_bytes = build_wire_request(state->request);
      state->client = g_socket_client_new();
      g_socket_client_set_tls(state->client, FALSE);
      g_socket_client_set_timeout(state->client, 30);

      auto* client = state->client;
      const std::string host = state->request.host;
      const auto port = static_cast<::guint16>(state->request.port);
      auto* raw = state.release();

      g_socket_client_connect_to_host_async(
        client,
        host.c_str(),
        port,
        nullptr,
        on_connected,
        raw
      );
    }

    std::string
    decode_uri_component(const char* value)
    {
      if (!value || !*value)
      {
        return {};
      }

      char* decoded = ::g_uri_unescape_string(value, nullptr);

      if (!decoded)
      {
        return value;
      }

      std::string result = decoded;
      g_free(decoded);

      return result;
    }
  }

  bool
  parse_uri(const std::string& uri_string, Request& out)
  {
    GError* error = nullptr;
    GUri* uri = g_uri_parse(uri_string.c_str(), G_URI_FLAGS_NONE, &error);

    if (!uri)
    {
      if (error)
      {
        g_error_free(error);
      }

      return false;
    }

    const char* scheme = g_uri_get_scheme(uri);
    const char* host = g_uri_get_host(uri);
    const char* path = g_uri_get_path(uri);
    const char* userinfo = g_uri_get_userinfo(uri);
    int port = g_uri_get_port(uri);

    if (!scheme || !host || std::strcmp(scheme, "finger") != 0)
    {
      g_uri_unref(uri);

      return false;
    }

    out = Request{};
    out.uri = uri_string;
    out.host = host;
    out.port = port < 0 ? default_port : port;

    std::string path_part = path ? path : "";

    if (!path_part.empty() && path_part.front() == '/')
    {
      path_part.erase(path_part.begin());
    }

    if (!path_part.empty())
    {
      out.query = decode_uri_component(path_part.c_str());
    } else if (userinfo && *userinfo) {
      // Historical form: finger://user@host/
      out.query = decode_uri_component(userinfo);
    }

    g_uri_unref(uri);

    return true;
  }

  std::string
  build_uri(
    const std::string& host,
    int port,
    const std::string& query
  )
  {
    std::string uri = "finger://";
    uri += host;

    if (port > 0 && port != default_port)
    {
      uri += ':';
      uri += std::to_string(port);
    }

    uri += '/';

    if (!query.empty())
    {
      char* escaped = g_uri_escape_string(
        query.c_str(),
        "/:@-._~!$&'()*+,;=",
        TRUE
      );

      if (escaped)
      {
        uri += escaped;
        g_free(escaped);
      } else {
        uri += query;
      }
    }

    return uri;
  }

  void
  fetch(const std::string& uri, FetchCallback callback)
  {
    auto state = std::make_unique<FetchState>();

    if (!parse_uri(uri, state->request))
    {
      Response response;

      response.uri = uri;
      response.error = "URI must be a finger:// URL with a host";
      if (callback)
      {
        callback(std::move(response));
      }

      return;
    }

    state->request.uri = build_uri(
      state->request.host,
      state->request.port,
      state->request.query
    );
    state->callback = std::move(callback);
    start_fetch(std::move(state));
  }
}
