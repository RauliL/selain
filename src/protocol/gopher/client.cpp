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

#include <selain/protocol/gopher/client.hpp>

namespace selain::protocol::gopher
{
  static constexpr int default_port = 70;
  static constexpr gsize read_chunk_size = 16 * 1024;

  namespace
  {
    struct FetchState
    {
      Request request;
      FetchCallback callback;
      ::GSocketClient* client = nullptr;
      ::GSocketConnection* connection = nullptr;
      ::GInputStream* input = nullptr;
      ::GOutputStream* output = nullptr;
      std::string request_bytes;
      gsize request_offset = 0;
      ::GByteArray* buffer = nullptr;
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
      response.type = state->request.type;
      response.error = message;
      complete(std::move(state), std::move(response));
    }

    void
    finish_success(FetchStatePtr state)
    {
      Response response;

      response.uri = state->request.uri;
      response.type = state->request.type;

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
    on_body_read(::GObject* source, ::GAsyncResult* result, gpointer data)
    {
      FetchStatePtr state(static_cast<FetchState*>(data));
      ::GError* error = nullptr;
      ::GBytes* bytes = ::g_input_stream_read_bytes_finish(
        G_INPUT_STREAM(source),
        result,
        &error
      );

      if (!bytes)
      {
        const std::string message = error && error->message
          ? error->message
          : "Failed to read Gopher response";

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
    on_request_written(::GObject* source, ::GAsyncResult* result, gpointer data)
    {
      FetchStatePtr state(static_cast<FetchState*>(data));
      ::GError* error = nullptr;
      const gssize written = ::g_output_stream_write_finish(
        G_OUTPUT_STREAM(source),
        result,
        &error
      );

      if (written < 0)
      {
        const std::string message = error && error->message
          ? error->message
          : "Failed to send Gopher request";

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

    gboolean
    on_accept_certificate(
      ::GTlsConnection*,
      ::GTlsCertificate*,
      ::GTlsCertificateFlags,
      gpointer
    )
    {
      return TRUE;
    }

    void
    on_client_event(
      ::GSocketClient*,
      ::GSocketClientEvent event,
      ::GSocketConnectable*,
      ::GIOStream* connection,
      gpointer
    )
    {
      if (event == G_SOCKET_CLIENT_TLS_HANDSHAKING && connection)
      {
        g_signal_connect(
          connection,
          "accept-certificate",
          G_CALLBACK(on_accept_certificate),
          nullptr
        );
      }
    }

    void
    on_connected(::GObject* source, ::GAsyncResult* result, gpointer data)
    {
      FetchStatePtr state(static_cast<FetchState*>(data));
      ::GError* error = nullptr;
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
      std::string wire = request.selector;

      if (!request.search.empty())
      {
        wire += '\t';
        wire += request.search;
      }
      wire += "\r\n";

      return wire;
    }

    void
    start_fetch(FetchStatePtr state)
    {
      if (state->request.host.empty())
      {
        fail(std::move(state), "URI must be a gopher(s):// URL with a host");

        return;
      }

      state->request_bytes = build_wire_request(state->request);
      state->client = g_socket_client_new();
      g_socket_client_set_tls(state->client, state->request.use_tls);
      g_socket_client_set_timeout(state->client, 30);

      if (state->request.use_tls)
      {
        g_signal_connect(
          state->client,
          "event",
          G_CALLBACK(on_client_event),
          nullptr
        );
      }

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
  }

  bool
  parse_uri(const std::string& uri_string, Request& out)
  {
    ::GError* error = nullptr;
    ::GUri* uri = ::g_uri_parse(uri_string.c_str(), G_URI_FLAGS_NONE, &error);

    if (!uri)
    {
      if (error)
      {
        g_error_free(error);
      }

      return false;
    }

    const char* scheme = ::g_uri_get_scheme(uri);
    const char* host = ::g_uri_get_host(uri);
    const char* path = ::g_uri_get_path(uri);
    int port = ::g_uri_get_port(uri);

    if (!scheme
        || !host
        || (std::strcmp(scheme, "gopher") != 0
            && std::strcmp(scheme, "gophers") != 0))
    {
      g_uri_unref(uri);

      return false;
    }

    out = Request{};
    out.uri = uri_string;
    out.scheme = scheme;
    out.host = host;
    out.use_tls = std::strcmp(scheme, "gophers") == 0;
    out.port = port < 0 ? default_port : port;
    out.type = '1';

    std::string path_part = path ? path : "";

    if (!path_part.empty() && path_part.front() == '/')
    {
      path_part.erase(path_part.begin());
    }

    if (!path_part.empty())
    {
      out.type = path_part.front();
      out.selector = path_part.substr(1);
    }

    // RFC 4266: optional search after %09 (tab) in the path.
    if (const auto tab = out.selector.find('\t'); tab != std::string::npos)
    {
      out.search = out.selector.substr(tab + 1);
      out.selector.resize(tab);

      // Gopher+ may append another tab; keep only the search field.
      if (const auto plus = out.search.find('\t'); plus != std::string::npos)
      {
        out.search.resize(plus);
      }
    }

    g_uri_unref(uri);

    return true;
  }

  std::string
  build_uri(
    const std::string& scheme,
    const std::string& host,
    int port,
    char type,
    const std::string& selector,
    const std::string& search
  )
  {
    std::string uri = scheme;
    uri += "://";
    uri += host;

    if (port > 0 && port != default_port)
    {
      uri += ':';
      uri += std::to_string(port);
    }

    uri += '/';
    uri += type;

    // Escape selector but keep path separators and common Gopher characters.
    char* escaped_selector = ::g_uri_escape_string(
      selector.c_str(),
      "/:@-._~!$&'()*+,;=",
      TRUE
    );

    if (escaped_selector)
    {
      uri += escaped_selector;
      g_free(escaped_selector);
    } else {
      uri += selector;
    }

    if (!search.empty())
    {
      char* escaped_search = ::g_uri_escape_string(search.c_str(), nullptr, TRUE);

      uri += "%09";
      if (escaped_search)
      {
        uri += escaped_search;
        g_free(escaped_search);
      } else {
        uri += search;
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
      response.error = "URI must be a gopher:// or gophers:// URL with a host";
      if (callback)
      {
        callback(std::move(response));
      }

      return;
    }

    // Prefer a normalized URI in responses / address-bar updates.
    state->request.uri = build_uri(
      state->request.scheme,
      state->request.host,
      state->request.port,
      state->request.type,
      state->request.selector,
      state->request.search
    );
    state->callback = std::move(callback);
    start_fetch(std::move(state));
  }
}
