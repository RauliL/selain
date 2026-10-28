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
#include <selain/view.hpp>

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
    make_input_waiting_page(const std::string& prompt, bool sensitive)
    {
      return make_document(
        "Input required",
        std::format(
          "<h1>Input required</h1>"
          "<p>{}</p>"
          "<p>Type your {} in the command bar, then press "
          "<kbd>Enter</kbd> to submit or <kbd>Esc</kbd> to cancel.</p>",
          html_escape(prompt.empty() ? "This capsule requests input." : prompt),
          sensitive ? "sensitive input" : "reply"
        )
      );
    }

    std::string
    js_string_escape(std::string_view text)
    {
      std::string out;

      out.reserve(text.size());
      for (const char c : text)
      {
        switch (c)
        {
          case '\\':
          case '\'':
          case '"':
            out += '\\';
            out += c;
            break;

          case '\n':
            out += "\\n";
            break;

          case '\r':
            out += "\\r";
            break;

          case '<':
            out += "\\u003c";
            break;

          default:
            out += c;
            break;
        }
      }

      return out;
    }

    std::string
    make_redirect_page(const std::string& target_uri)
    {
      const auto escaped_html = html_escape(target_uri);
      const auto escaped_js = js_string_escape(target_uri);

      return std::format(
        "<!DOCTYPE html>\n"
        "<html><head><meta charset=\"utf-8\"/>"
        "<meta http-equiv=\"refresh\" content=\"0;url={}\"/>"
        "<title>Redirecting</title>"
        "<script>location.replace(\"{}\");</script>"
        "</head><body><main>"
        "<p>Redirecting to <a href=\"{}\">{}</a>…</p>"
        "<p>If nothing happens, activate the link or press "
        "<kbd>Enter</kbd> on it.</p>"
        "</main></body></html>\n",
        escaped_html,
        escaped_js,
        escaped_html,
        escaped_html
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

    struct GeminiInputPrompt
    {
      View* view = nullptr;
      Glib::ustring uri;
      Glib::ustring prompt;
      bool sensitive = false;
    };

    gboolean
    prompt_gemini_input_idle(gpointer user_data)
    {
      auto* data = static_cast<GeminiInputPrompt*>(user_data);

      if (data->view)
      {
        data->view->request_gemini_input(
          data->uri,
          data->prompt,
          data->sensitive
        );
      }
      delete data;

      return G_SOURCE_REMOVE;
    }

    void
    schedule_gemini_input_prompt(
      ::WebKitURISchemeRequest* request,
      const GeminiResponse& response
    )
    {
      auto* web_view = webkit_uri_scheme_request_get_web_view(request);
      auto* view = View::from_web_view(web_view);

      finish_with_html(
        request,
        make_input_waiting_page(response.meta, response.status == 11)
      );

      if (!view)
      {
        return;
      }

      auto* data = new GeminiInputPrompt{
        .view = view,
        .uri = response.uri,
        .prompt = response.meta,
        .sensitive = response.status == 11
      };

      g_idle_add(prompt_gemini_input_idle, data);
    }

    void
    handle_redirect(
      ::WebKitURISchemeRequest* request,
      const GeminiResponse& response
    )
    {
      auto* web_view = webkit_uri_scheme_request_get_web_view(request);
      auto* view = View::from_web_view(web_view);

      if (response.meta.empty())
      {
        finish_with_html(
          request,
          make_error_page(
            "Redirect failed",
            "Redirect response was missing a target URI."
          )
        );

        return;
      }

      if (view && !view->consume_gemini_redirect_hop())
      {
        finish_with_html(
          request,
          make_error_page("Too many redirects", "Redirect limit exceeded.")
        );

        return;
      }

      const auto target = gemini_resolve_uri(response.uri, response.meta);

      if (view)
      {
        view->set_status(std::format("Redirecting to {}…", target));
      }

      finish_with_html(request, make_redirect_page(target));
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
        if (auto* view = View::from_web_view(
              webkit_uri_scheme_request_get_web_view(request)
            ))
        {
          view->reset_gemini_redirect_hops();
        }
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
          if (auto* view = View::from_web_view(
                webkit_uri_scheme_request_get_web_view(request)
              ))
          {
            view->reset_gemini_redirect_hops();
          }
          schedule_gemini_input_prompt(request, response);
          break;

        case 2:
          if (auto* view = View::from_web_view(
                webkit_uri_scheme_request_get_web_view(request)
              ))
          {
            view->reset_gemini_redirect_hops();
          }
          handle_success(request, response);
          break;

        case 3:
          handle_redirect(request, response);
          break;

        case 4:
        case 5:
        case 6:
          if (auto* view = View::from_web_view(
                webkit_uri_scheme_request_get_web_view(request)
              ))
          {
            view->reset_gemini_redirect_hops();
          }
          finish_with_html(
            request,
            make_error_page(
              class_digit == 4
                ? std::format("Temporary failure ({})", response.status)
                : class_digit == 5
                  ? std::format("Permanent failure ({})", response.status)
                  : std::format("Certificate required ({})", response.status),
              response.meta.empty()
                ? (class_digit == 4
                     ? "The capsule reported a temporary failure."
                     : class_digit == 5
                       ? "The capsule reported a permanent failure."
                       : "This capsule requires a client certificate.")
                : response.meta
            )
          );
          break;

        default:
          if (auto* view = View::from_web_view(
                webkit_uri_scheme_request_get_web_view(request)
              ))
          {
            view->reset_gemini_redirect_hops();
          }
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
