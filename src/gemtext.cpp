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
#include <selain/gemtext.hpp>

#include <glib.h>

#include <sstream>

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
    resolve_link(std::string_view base_uri, std::string_view link)
    {
      if (link.empty())
      {
        return {};
      }

      ::GError* error = nullptr;
      const auto base = std::string(base_uri);
      const auto relative = std::string(link);
      char* resolved = ::g_uri_resolve_relative(
        base.c_str(),
        relative.c_str(),
        G_URI_FLAGS_ENCODED,
        &error
      );

      if (!resolved)
      {
        if (error)
        {
          g_error_free(error);
        }

        return relative;
      }

      std::string result(resolved);

      g_free(resolved);

      return result;
    }

    void
    append_preformatted(std::ostringstream& html, std::string_view text)
    {
      html << "<pre>" << html_escape(text) << "</pre>\n";
    }

    bool
    starts_with(std::string_view text, std::string_view prefix)
    {
      return text.size() >= prefix.size()
        && text.compare(0, prefix.size(), prefix) == 0;
    }

    std::string_view
    trim_leading(std::string_view text)
    {
      while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
      {
        text.remove_prefix(1);
      }

      return text;
    }

    std::string
    extract_title(std::string_view gemtext)
    {
      std::size_t start = 0;

      while (start < gemtext.size())
      {
        const auto end = gemtext.find('\n', start);
        auto line = gemtext.substr(
          start,
          end == std::string_view::npos ? gemtext.size() - start : end - start
        );

        if (!line.empty() && line.back() == '\r')
        {
          line.remove_suffix(1);
        }

        if (starts_with(line, "# ") && line.size() > 2)
        {
          return std::string(line.substr(2));
        }
        if (starts_with(line, "#") && !starts_with(line, "##") && line.size() > 1)
        {
          return std::string(trim_leading(line.substr(1)));
        }

        if (end == std::string_view::npos)
        {
          break;
        }
        start = end + 1;
      }

      return "Gemini";
    }
  }

  std::string
  gemtext_to_html(std::string_view gemtext, std::string_view base_uri)
  {
    std::ostringstream body;
    std::string preformatted;
    bool in_pre = false;
    bool in_list = false;
    std::size_t start = 0;

    const auto close_list = [&]()
    {
      if (in_list)
      {
        body << "</ul>\n";
        in_list = false;
      }
    };

    while (start <= gemtext.size())
    {
      const auto end = gemtext.find('\n', start);
      auto line = gemtext.substr(
        start,
        end == std::string_view::npos ? gemtext.size() - start : end - start
      );

      if (!line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }

      if (starts_with(line, "```"))
      {
        close_list();
        if (in_pre)
        {
          append_preformatted(body, preformatted);
          preformatted.clear();
          in_pre = false;
        } else {
          in_pre = true;
        }
      }
      else if (in_pre)
      {
        if (!preformatted.empty())
        {
          preformatted.push_back('\n');
        }
        preformatted.append(line);
      }
      else if (starts_with(line, "=>"))
      {
        close_list();

        auto rest = trim_leading(line.substr(2));
        std::string_view url;
        std::string_view label;
        const auto sep = rest.find_first_of(" \t");

        if (sep == std::string_view::npos)
        {
          url = rest;
          label = rest;
        } else {
          url = rest.substr(0, sep);
          label = trim_leading(rest.substr(sep));
          if (label.empty())
          {
            label = url;
          }
        }

        const auto href = resolve_link(base_uri, url);

        body
          << "<p class=\"link\"><a href=\""
          << html_escape(href)
          << "\">"
          << html_escape(label)
          << "</a></p>\n";
      }
      else if (starts_with(line, "* ") || starts_with(line, "*\t"))
      {
        if (!in_list)
        {
          body << "<ul>\n";
          in_list = true;
        }
        body
          << "<li>"
          << html_escape(trim_leading(line.substr(1)))
          << "</li>\n";
      }
      else if (starts_with(line, "###"))
      {
        close_list();
        body
          << "<h3>"
          << html_escape(trim_leading(line.substr(3)))
          << "</h3>\n";
      }
      else if (starts_with(line, "##"))
      {
        close_list();
        body
          << "<h2>"
          << html_escape(trim_leading(line.substr(2)))
          << "</h2>\n";
      }
      else if (starts_with(line, "#"))
      {
        close_list();
        body
          << "<h1>"
          << html_escape(trim_leading(line.substr(1)))
          << "</h1>\n";
      }
      else if (starts_with(line, ">"))
      {
        close_list();
        body
          << "<blockquote>"
          << html_escape(trim_leading(line.substr(1)))
          << "</blockquote>\n";
      }
      else if (line.empty())
      {
        close_list();
        body << "<br/>\n";
      } else {
        close_list();
        body << "<p>" << html_escape(line) << "</p>\n";
      }

      if (end == std::string_view::npos)
      {
        break;
      }
      start = end + 1;
    }

    if (in_pre)
    {
      append_preformatted(body, preformatted);
    }
    close_list();

    const auto title = html_escape(extract_title(gemtext));
    std::ostringstream html;

    html
      << "<!DOCTYPE html>\n"
      << "<html>\n"
      << "<head>\n"
      << "<meta charset=\"utf-8\"/>\n"
      << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>\n"
      << "<title>" << title << "</title>\n"
      << "<style>\n"
      << "  :root {\n"
      << "    color-scheme: dark;\n"
      << "    --bg: #262626;\n"
      << "    --fg: #b3deef;\n"
      << "    --muted: #7aa6c2;\n"
      << "    --link: #a7d6f8;\n"
      << "    --quote: #d7d75f;\n"
      << "    --pre-bg: #1c1c1c;\n"
      << "  }\n"
      << "  html, body {\n"
      << "    margin: 0;\n"
      << "    padding: 0;\n"
      << "    background: var(--bg);\n"
      << "    color: var(--fg);\n"
      << "    font: 16px/1.55 system-ui, sans-serif;\n"
      << "  }\n"
      << "  main {\n"
      << "    max-width: 46rem;\n"
      << "    margin: 0 auto;\n"
      << "    padding: 1.5rem 1.25rem 3rem;\n"
      << "  }\n"
      << "  h1, h2, h3 { line-height: 1.25; }\n"
      << "  h1 { font-size: 1.8rem; }\n"
      << "  h2 { font-size: 1.4rem; }\n"
      << "  h3 { font-size: 1.15rem; }\n"
      << "  a { color: var(--link); }\n"
      << "  p.link { margin: 0.35rem 0; }\n"
      << "  blockquote {\n"
      << "    margin: 0.75rem 0;\n"
      << "    padding-left: 0.9rem;\n"
      << "    border-left: 3px solid var(--quote);\n"
      << "    color: var(--muted);\n"
      << "  }\n"
      << "  pre {\n"
      << "    overflow-x: auto;\n"
      << "    padding: 0.85rem 1rem;\n"
      << "    background: var(--pre-bg);\n"
      << "    border-radius: 4px;\n"
      << "    font: 14px/1.45 ui-monospace, monospace;\n"
      << "  }\n"
      << "  ul { padding-left: 1.25rem; }\n"
      << "</style>\n"
      << "</head>\n"
      << "<body><main>\n"
      << body.str()
      << "</main></body>\n"
      << "</html>\n";

    return html.str();
  }
}
