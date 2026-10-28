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
#include <selain/gemini-client.hpp>

#include <gio/gio.h>

#include <cctype>
#include <cstring>
#include <memory>
#include <utility>

namespace selain
{
  namespace
  {
    constexpr int default_port = 1965;
    constexpr int max_redirects = 5;
    constexpr gsize max_header_bytes = 1024 + 2;
    constexpr gsize read_chunk_size = 16 * 1024;

    struct FetchState
    {
      std::string uri;
      GeminiFetchCallback callback;
      int redirects_left = max_redirects;
      ::GSocketClient* client = nullptr;
      ::GSocketConnection* connection = nullptr;
      ::GInputStream* input = nullptr;
      ::GOutputStream* output = nullptr;
      std::string request_bytes;
      gsize request_offset = 0;
      ::GByteArray* buffer = nullptr;
      bool header_done = false;
      int status = 0;
      std::string meta;
    };

    using FetchStatePtr = std::unique_ptr<FetchState>;

    void start_fetch(FetchStatePtr state);
    void read_more(FetchStatePtr state);
    void write_more(FetchStatePtr state);

    void
    complete(FetchStatePtr state, GeminiResponse response)
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
      GeminiResponse response;

      response.uri = state->uri;
      response.error = message;
      complete(std::move(state), std::move(response));
    }

    void
    release_connection(FetchState& state)
    {
      if (state.connection)
      {
        g_io_stream_close(G_IO_STREAM(state.connection), nullptr, nullptr);
      }
      if (state.buffer)
      {
        g_byte_array_unref(state.buffer);
        state.buffer = nullptr;
      }
      g_clear_object(&state.connection);
      g_clear_object(&state.client);
      state.input = nullptr;
      state.output = nullptr;
      state.request_bytes.clear();
      state.request_offset = 0;
      state.header_done = false;
      state.status = 0;
      state.meta.clear();
    }

    std::string
    normalize_request_uri(const std::string& uri_string)
    {
      ::GError* error = nullptr;
      ::GUri* uri = ::g_uri_parse(uri_string.c_str(), G_URI_FLAGS_NONE, &error);

      if (!uri)
      {
        if (error)
        {
          g_error_free(error);
        }

        return uri_string;
      }

      const char* scheme = ::g_uri_get_scheme(uri);
      const char* host = ::g_uri_get_host(uri);
      const char* path = ::g_uri_get_path(uri);
      const char* query = ::g_uri_get_query(uri);
      const int port = ::g_uri_get_port(uri);

      if (!scheme || !host || std::strcmp(scheme, "gemini") != 0)
      {
        g_uri_unref(uri);

        return uri_string;
      }

      std::string normalized = "gemini://";

      normalized += host;
      if (port > 0 && port != default_port)
      {
        normalized += ':';
        normalized += std::to_string(port);
      }

      if (!path || path[0] == '\0')
      {
        normalized += '/';
      } else {
        normalized += path;
      }

      if (query)
      {
        normalized += '?';
        normalized += query;
      }

      g_uri_unref(uri);

      return normalized;
    }

    std::string
    resolve_redirect(const std::string& base_uri, const std::string& target)
    {
      ::GError* error = nullptr;
      char* resolved = ::g_uri_resolve_relative(
        base_uri.c_str(),
        target.c_str(),
        G_URI_FLAGS_ENCODED,
        &error
      );

      if (!resolved)
      {
        if (error)
        {
          g_error_free(error);
        }

        return target;
      }

      std::string result(resolved);

      g_free(resolved);

      return result;
    }

    bool
    parse_header_line(const std::string& line, int& status, std::string& meta)
    {
      if (line.size() < 2
          || !std::isdigit(static_cast<unsigned char>(line[0]))
          || !std::isdigit(static_cast<unsigned char>(line[1])))
      {
        return false;
      }

      status = (line[0] - '0') * 10 + (line[1] - '0');
      if (status < 10 || status > 69)
      {
        return false;
      }

      if (line.size() == 2)
      {
        meta.clear();

        return true;
      }

      if (line[2] != ' ')
      {
        return false;
      }

      meta = line.substr(3);

      return true;
    }

    void
    finish_success(FetchStatePtr state)
    {
      GeminiResponse response;

      response.uri = state->uri;
      response.status = state->status;
      response.meta = state->meta;

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
    follow_redirect(FetchStatePtr state)
    {
      if (state->redirects_left <= 0)
      {
        fail(std::move(state), "Too many redirects");

        return;
      }
      if (state->meta.empty())
      {
        fail(std::move(state), "Redirect response missing target URI");

        return;
      }

      auto next_uri = resolve_redirect(state->uri, state->meta);
      auto callback = std::move(state->callback);
      const int redirects_left = state->redirects_left - 1;

      release_connection(*state);

      auto next = std::make_unique<FetchState>();

      next->uri = std::move(next_uri);
      next->callback = std::move(callback);
      next->redirects_left = redirects_left;
      start_fetch(std::move(next));
    }

    void
    process_completed_header(FetchStatePtr state, gsize header_end)
    {
      std::string header_line(
        reinterpret_cast<char*>(state->buffer->data),
        header_end
      );

      if (!header_line.empty() && header_line.back() == '\r')
      {
        header_line.pop_back();
      }

      if (!parse_header_line(header_line, state->status, state->meta))
      {
        fail(std::move(state), "Invalid Gemini response header");

        return;
      }

      const gsize remaining = state->buffer->len - (header_end + 1);

      if (remaining > 0)
      {
        std::memmove(
          state->buffer->data,
          state->buffer->data + header_end + 1,
          remaining
        );
      }
      g_byte_array_set_size(state->buffer, static_cast<guint>(remaining));
      state->header_done = true;

      const int class_digit = state->status / 10;

      if (class_digit == 3)
      {
        follow_redirect(std::move(state));

        return;
      }

      if (class_digit != 2)
      {
        GeminiResponse response;

        response.uri = state->uri;
        response.status = state->status;
        response.meta = state->meta;
        complete(std::move(state), std::move(response));

        return;
      }

      read_more(std::move(state));
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
          : "Failed to read Gemini response";

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

      if (!state->header_done)
      {
        const auto* begin = state->buffer->data;
        const auto* newline = static_cast<const std::uint8_t*>(
          std::memchr(begin, '\n', state->buffer->len)
        );

        if (!newline)
        {
          if (state->buffer->len > max_header_bytes)
          {
            fail(std::move(state), "Gemini response header too large");

            return;
          }
          read_more(std::move(state));

          return;
        }

        process_completed_header(
          std::move(state),
          static_cast<gsize>(newline - begin)
        );

        return;
      }

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
          : "Failed to send Gemini request";

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

    void
    start_fetch(FetchStatePtr state)
    {
      ::GError* error = nullptr;
      ::GUri* uri = ::g_uri_parse(state->uri.c_str(), G_URI_FLAGS_NONE, &error);

      if (!uri)
      {
        const std::string message = error && error->message
          ? error->message
          : "Invalid URI";

        if (error)
        {
          g_error_free(error);
        }
        fail(std::move(state), message);

        return;
      }

      const char* scheme = ::g_uri_get_scheme(uri);
      const char* host = ::g_uri_get_host(uri);
      int port = ::g_uri_get_port(uri);

      if (!scheme || std::strcmp(scheme, "gemini") != 0 || !host)
      {
        g_uri_unref(uri);
        fail(std::move(state), "URI must be a gemini:// URL with a host");

        return;
      }

      if (port < 0)
      {
        port = default_port;
      }

      state->uri = normalize_request_uri(state->uri);
      state->request_bytes = state->uri + "\r\n";

      if (state->request_bytes.size() > 1024 + 2)
      {
        g_uri_unref(uri);
        fail(std::move(state), "Gemini request URI exceeds 1024 bytes");

        return;
      }

      const std::string host_copy(host);

      g_uri_unref(uri);

      state->client = g_socket_client_new();
      g_socket_client_set_tls(state->client, TRUE);
      g_socket_client_set_timeout(state->client, 30);
      g_signal_connect(
        state->client,
        "event",
        G_CALLBACK(on_client_event),
        nullptr
      );

      auto* client = state->client;
      auto* raw = state.release();

      g_socket_client_connect_to_host_async(
        client,
        host_copy.c_str(),
        static_cast<::guint16>(port),
        nullptr,
        on_connected,
        raw
      );
    }
  }

  void
  gemini_fetch(const std::string& uri, GeminiFetchCallback callback)
  {
    auto state = std::make_unique<FetchState>();

    state->uri = uri;
    state->callback = std::move(callback);
    start_fetch(std::move(state));
  }
}
