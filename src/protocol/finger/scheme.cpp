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
#include <cstdint>
#include <format>
#include <string_view>

#include <selain/protocol/finger/client.hpp>
#include <selain/protocol/finger/scheme.hpp>
#include <selain/theme.hpp>
#include <selain/utils.hpp>

namespace selain::protocol::finger
{
  static std::string
  make_error_page(const std::string& title, const std::string& message)
  {
    return theme::render_document(
      title,
      std::format(
        "<h1>{}</h1><p>{}</p>",
        utils::html_escape(title),
        utils::html_escape(message)
      )
    );
  }

  static void
  finish_with_bytes(
    WebKitURISchemeRequest* request,
    const std::uint8_t* data,
    gssize length,
    const char* mime_type
  )
  {
    auto* stream = g_memory_input_stream_new_from_data(
      g_memdup2(data, static_cast<gsize>(length)),
      length,
      g_free
    );

    webkit_uri_scheme_request_finish(request, stream, length, mime_type);
    g_object_unref(stream);
  }

  static void
  finish_with_html(
    WebKitURISchemeRequest* request,
    const std::string& html
  )
  {
    finish_with_bytes(
      request,
      reinterpret_cast<const std::uint8_t*>(html.data()),
      static_cast<gssize>(html.size()),
      "text/html; charset=utf-8"
    );
  }

  static void
  finish_with_error(
    WebKitURISchemeRequest* request,
    const std::string& message
  )
  {
    GError* error = g_error_new_literal(
      G_IO_ERROR,
      G_IO_ERROR_FAILED,
      message.c_str()
    );

    webkit_uri_scheme_request_finish_error(request, error);
    g_error_free(error);
  }

  static std::string
  body_as_text(const Response& response)
  {
    return std::string(
      reinterpret_cast<const char*>(response.body.data()),
      response.body.size()
    );
  }

  static std::string
  text_to_html(std::string_view text)
  {
    std::string cleaned;
    std::size_t start = 0;

    cleaned.reserve(text.size());

    while (start <= text.size())
    {
      const auto end = text.find('\n', start);
      auto line = text.substr(
        start,
        end == std::string_view::npos ? text.size() - start : end - start
      );

      if (!line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }

      if (!cleaned.empty())
      {
        cleaned.push_back('\n');
      }
      cleaned.append(line);

      if (end == std::string_view::npos)
      {
        break;
      }
      start = end + 1;
    }

    return theme::render_document(
      "Finger",
      std::string("<pre>") + utils::html_escape(cleaned) + "</pre>\n"
    );
  }

  static void
  on_finger_response(
    WebKitURISchemeRequest* request,
    Response response
  )
  {
    if (!response.error.empty())
    {
      finish_with_html(
        request,
        make_error_page("Finger request failed", response.error)
      );
      g_object_unref(request);

      return;
    }

    finish_with_html(request, text_to_html(body_as_text(response)));
    g_object_unref(request);
  }

  static void
  finger_uri_scheme_request_cb(
    WebKitURISchemeRequest* request,
    gpointer
  )
  {
    const char* uri = webkit_uri_scheme_request_get_uri(request);

    if (!uri)
    {
      finish_with_error(request, "Missing Finger URI");

      return;
    }

    Request parsed;

    if (!parse_uri(uri, parsed))
    {
      finish_with_html(
        request,
        make_error_page(
          "Invalid Finger URI",
          "Expected a finger:// URL with a host."
        )
      );

      return;
    }

    g_object_ref(request);

    fetch(
      uri,
      [request](Response response)
      {
        on_finger_response(request, std::move(response));
      }
    );
  }

  void
  register_schemes(::WebKitWebContext* context)
  {
    webkit_web_context_register_uri_scheme(
      context,
      "finger",
      finger_uri_scheme_request_cb,
      nullptr,
      nullptr
    );
  }
}
