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
#include <selain/gemini-scheme.hpp>

#include <selain/gemini-client.hpp>
#include <selain/gemtext.hpp>

#include <cctype>
#include <cstring>
#include <format>
#include <string>
#include <string_view>

namespace selain
{
  namespace
  {
    std::string
    html_escape(std::string_view text)
    {
      std::string out;

      out.reserve(text.size());
      for (const char c : text)
      {
        switch (c)
        {
          case '&':
            out += "&amp;";
            break;

          case '<':
            out += "&lt;";
            break;

          case '>':
            out += "&gt;";
            break;

          case '"':
            out += "&quot;";
            break;

          default:
            out += c;
            break;
        }
      }

      return out;
    }

    std::string
    mime_type_without_parameters(std::string_view meta)
    {
      const auto end = meta.find(';');
      auto type = end == std::string_view::npos ? meta : meta.substr(0, end);

      while (!type.empty()
             && (type.back() == ' ' || type.back() == '\t'))
      {
        type.remove_suffix(1);
      }

      return std::string(type);
    }

    bool
    starts_with_ci(std::string_view text, std::string_view prefix)
    {
      if (text.size() < prefix.size())
      {
        return false;
      }

      for (std::size_t i = 0; i < prefix.size(); ++i)
      {
        const auto a = static_cast<unsigned char>(text[i]);
        const auto b = static_cast<unsigned char>(prefix[i]);

        if (std::tolower(a) != std::tolower(b))
        {
          return false;
        }
      }

      return true;
    }

    std::string
    uri_without_query(const std::string& uri)
    {
      const auto pos = uri.find('?');

      if (pos == std::string::npos)
      {
        return uri;
      }

      return uri.substr(0, pos);
    }

    std::string
    make_document(const std::string& title, const std::string& body_html)
    {
      return std::format(
        "<!DOCTYPE html>\n"
        "<html><head><meta charset=\"utf-8\"/>"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>"
        "<title>{}</title>"
        "<style>"
        "html,body{{margin:0;background:#262626;color:#b3deef;"
        "font:16px/1.55 system-ui,sans-serif}}"
        "main{{max-width:46rem;margin:0 auto;padding:1.5rem 1.25rem 3rem}}"
        "a{{color:#a7d6f8}} input,button{{font:inherit;margin-top:.75rem}}"
        "input{{width:min(100%,24rem);padding:.4rem .55rem}}"
        "button{{padding:.4rem .8rem}}"
        "</style></head><body><main>{}</main></body></html>\n",
        html_escape(title),
        body_html
      );
    }

    std::string
    make_error_page(const std::string& title, const std::string& message)
    {
      return make_document(
        title,
        std::format(
          "<h1>{}</h1><p>{}</p>",
          html_escape(title),
          html_escape(message)
        )
      );
    }

    std::string
    make_input_page(const std::string& uri, const std::string& prompt, bool sensitive)
    {
      const auto action_base = html_escape(uri_without_query(uri));
      const char* input_type = sensitive ? "password" : "text";

      return make_document(
        "Input required",
        std::format(
          "<h1>Input required</h1>"
          "<p>{}</p>"
          "<form id=\"gemini-input\">"
          "<input id=\"value\" type=\"{}\" autocomplete=\"off\" autofocus/>"
          "<div><button type=\"submit\">Submit</button></div>"
          "</form>"
          "<script>"
          "document.getElementById('gemini-input').addEventListener('submit', function (event) {{"
          "  event.preventDefault();"
          "  var value = document.getElementById('value').value;"
          "  location.href = '{}' + '?' + encodeURIComponent(value);"
          "}});"
          "</script>",
          html_escape(prompt.empty() ? "This capsule requests input." : prompt),
          input_type,
          action_base
        )
      );
    }

    void
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

    void
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

    void
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

    std::string
    body_as_text(const GeminiResponse& response)
    {
      return std::string(
        reinterpret_cast<const char*>(response.body.data()),
        response.body.size()
      );
    }

    void
    handle_success(
      ::WebKitURISchemeRequest* request,
      const GeminiResponse& response
    )
    {
      auto mime = mime_type_without_parameters(response.meta);

      if (mime.empty())
      {
        mime = "text/gemini";
      }

      if (mime == "text/gemini")
      {
        const auto html = gemtext_to_html(body_as_text(response), response.uri);

        finish_with_html(request, html);

        return;
      }

      if (starts_with_ci(mime, "text/"))
      {
        const auto html = make_document(
          mime,
          std::format("<pre>{}</pre>", html_escape(body_as_text(response)))
        );

        finish_with_html(request, html);

        return;
      }

      if (response.body.empty())
      {
        finish_with_html(
          request,
          make_error_page("Empty response", "The capsule returned no body.")
        );

        return;
      }

      finish_with_bytes(
        request,
        response.body.data(),
        static_cast<gssize>(response.body.size()),
        mime.c_str()
      );
    }

    void
    on_gemini_response(
      ::WebKitURISchemeRequest* request,
      GeminiResponse response
    )
    {
      if (!response.error.empty())
      {
        finish_with_html(
          request,
          make_error_page("Gemini request failed", response.error)
        );
        g_object_unref(request);

        return;
      }

      const int class_digit = response.status / 10;

      switch (class_digit)
      {
        case 1:
          finish_with_html(
            request,
            make_input_page(
              response.uri,
              response.meta,
              response.status == 11
            )
          );
          break;

        case 2:
          handle_success(request, response);
          break;

        case 4:
          finish_with_html(
            request,
            make_error_page(
              std::format("Temporary failure ({})", response.status),
              response.meta.empty()
                ? "The capsule reported a temporary failure."
                : response.meta
            )
          );
          break;

        case 5:
          finish_with_html(
            request,
            make_error_page(
              std::format("Permanent failure ({})", response.status),
              response.meta.empty()
                ? "The capsule reported a permanent failure."
                : response.meta
            )
          );
          break;

        case 6:
          finish_with_html(
            request,
            make_error_page(
              std::format("Certificate required ({})", response.status),
              response.meta.empty()
                ? "This capsule requires a client certificate."
                : response.meta
            )
          );
          break;

        default:
          finish_with_html(
            request,
            make_error_page(
              "Unexpected Gemini response",
              std::format("Unsupported status code {}.", response.status)
            )
          );
          break;
      }

      g_object_unref(request);
    }

    void
    gemini_uri_scheme_request_cb(
      ::WebKitURISchemeRequest* request,
      gpointer
    )
    {
      const char* uri = webkit_uri_scheme_request_get_uri(request);

      if (!uri)
      {
        finish_with_error(request, "Missing Gemini URI");

        return;
      }

      g_object_ref(request);

      gemini_fetch(
        uri,
        [request](GeminiResponse response)
        {
          on_gemini_response(request, std::move(response));
        }
      );
    }
  }

  void
  register_gemini_scheme(::WebKitWebContext* context)
  {
    auto* security = webkit_web_context_get_security_manager(context);

    webkit_security_manager_register_uri_scheme_as_secure(security, "gemini");
    webkit_web_context_register_uri_scheme(
      context,
      "gemini",
      gemini_uri_scheme_request_cb,
      nullptr,
      nullptr
    );
  }
}
