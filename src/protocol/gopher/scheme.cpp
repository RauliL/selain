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
#include <cctype>
#include <cstdint>
#include <format>

#include <selain/core/theme.hpp>
#include <selain/protocol/gopher/client.hpp>
#include <selain/protocol/gopher/menu.hpp>
#include <selain/protocol/gopher/scheme.hpp>
#include <selain/utils.hpp>

namespace selain::protocol::gopher
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

  static std::string
  make_search_page(const std::string& uri)
  {
    const auto escaped_js = utils::js_string_escape(uri);

    return theme::render_document(
      "Gopher search",
      std::format(
        "<h1>Gopher search</h1>"
        "<p>Enter search text and submit.</p>"
        "<form id=\"gopher-search\">"
        "<p><input id=\"q\" type=\"search\" autofocus "
        "placeholder=\"Search terms\"/></p>"
        "<p><button type=\"submit\">Search</button></p>"
        "</form>"
        "<script>"
        "document.getElementById('gopher-search')"
        ".addEventListener('submit', function (event) {{"
        "  event.preventDefault();"
        "  var q = document.getElementById('q').value || '';"
        "  location.href = '{}' + '%09' + encodeURIComponent(q);"
        "}});"
        "</script>",
        escaped_js
      )
    );
  }

  static const char*
  mime_for_type(char type)
  {
    switch (type)
    {
      case '0':
        return "text/plain; charset=utf-8";
      case '1':
        return "text/plain";
      case '4':
        return "application/mac-binhex40";
      case '5':
        return "application/octet-stream";
      case '6':
        return "text/x-uuencode";
      case '9':
        return "application/octet-stream";
      case 'g':
        return "image/gif";
      case 'h':
        return "text/html; charset=utf-8";
      case 'I':
        return "image/jpeg";
      case 'p':
        return "image/png";
      case 'd':
        return "application/pdf";
      case 's':
        return "audio/basic";
      default:
        return "application/octet-stream";
    }
  }

  static inline bool
  is_text_type(char type)
  {
    return type == '0' || type == '1' || type == '6' || type == 'h';
  }

  static void
  finish_with_bytes(
    ::WebKitURISchemeRequest* request,
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
    ::WebKitURISchemeRequest* request,
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
    ::WebKitURISchemeRequest* request,
    const std::string& message
  )
  {
    ::GError* error = g_error_new_literal(
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

  static void
  handle_success(
    ::WebKitURISchemeRequest* request,
    const Response& response
  )
  {
    switch (response.type)
    {
      case '1':
        finish_with_html(
          request,
          menu_to_html(body_as_text(response), response.uri)
        );
        return;

      case '0':
        finish_with_html(
          request,
          text_to_html(body_as_text(response), response.uri)
        );
        return;

      case 'h':
        finish_with_bytes(
          request,
          response.body.data(),
          static_cast<gssize>(response.body.size()),
          "text/html; charset=utf-8"
        );
        return;

      case '7':
        // Search with terms already applied returns a menu.
        finish_with_html(
          request,
          menu_to_html(body_as_text(response), response.uri)
        );
        return;

      default:
        break;
    }

    if (is_text_type(response.type))
    {
      const auto html = theme::render_document(
        "Gopher text",
        std::format("<pre>{}</pre>", utils::html_escape(body_as_text(response)))
      );

      finish_with_html(request, html);

      return;
    }

    if (response.body.empty())
    {
      finish_with_html(
        request,
        make_error_page("Empty response", "The Gopher server returned no body.")
      );

      return;
    }

    finish_with_bytes(
      request,
      response.body.data(),
      static_cast<gssize>(response.body.size()),
      mime_for_type(response.type)
    );
  }

  static void
  on_gopher_response(
    ::WebKitURISchemeRequest* request,
    Response response
  )
  {
    if (!response.error.empty())
    {
      finish_with_html(
        request,
        make_error_page("Gopher request failed", response.error)
      );
      g_object_unref(request);

      return;
    }

    handle_success(request, response);
    g_object_unref(request);
  }

  static void
  gopher_uri_scheme_request_cb(
    ::WebKitURISchemeRequest* request,
    gpointer
  )
  {
    const char* uri = webkit_uri_scheme_request_get_uri(request);

    if (!uri)
    {
      finish_with_error(request, "Missing Gopher URI");

      return;
    }

    Request parsed;

    if (!parse_uri(uri, parsed))
    {
      finish_with_html(
        request,
        make_error_page(
          "Invalid Gopher URI",
          "Expected a gopher:// or gophers:// URL with a host."
        )
      );

      return;
    }

    // Type 7 without search terms: prompt locally instead of hitting the server.
    if (parsed.type == '7' && parsed.search.empty())
    {
      finish_with_html(
        request,
        make_search_page(
          build_uri(
            parsed.scheme,
            parsed.host,
            parsed.port,
            parsed.type,
            parsed.selector
          )
        )
      );

      return;
    }

    g_object_ref(request);

    fetch(
      uri,
      [request](Response response)
      {
        on_gopher_response(request, std::move(response));
      }
    );
  }

  void
  register_schemes(::WebKitWebContext* context)
  {
    auto* security = webkit_web_context_get_security_manager(context);

    webkit_security_manager_register_uri_scheme_as_secure(security, "gophers");
    webkit_web_context_register_uri_scheme(
      context,
      "gopher",
      gopher_uri_scheme_request_cb,
      nullptr,
      nullptr
    );
    webkit_web_context_register_uri_scheme(
      context,
      "gophers",
      gopher_uri_scheme_request_cb,
      nullptr,
      nullptr
    );
  }
}
