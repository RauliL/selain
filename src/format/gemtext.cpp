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
#include <selain/core/theme.hpp>
#include <selain/utils.hpp>

namespace selain::format::gemtext
{
  static std::string
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
        ::g_error_free(error);
      }

      return relative;
    }

    std::string result(resolved);

    g_free(resolved);

    return result;
  }

  static void
  append_preformatted(std::ostringstream& html, std::string_view text)
  {
    html << "<pre>" << utils::html_escape(text) << "</pre>\n";
  }

  /**
   * Converts Gemini markup (text/gemini) into a self-contained HTML document.
   *
   * @param gemtext Raw gemtext body.
   * @param base_uri Absolute URI of the document, used to resolve relative links.
   */
  std::string
  render(std::string_view source, std::string_view base_uri)
  {
    std::optional<std::string> title;
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

    while (start <= source.size())
    {
      const auto end = source.find('\n', start);
      auto line = source.substr(
        start,
        end == std::string_view::npos ? source.size() - start : end - start
      );

      if (!line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }

      if (utils::starts_with(line, "```"))
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
      else if (utils::starts_with(line, "=>"))
      {
        close_list();

        auto rest = utils::trim_start(line.substr(2));
        std::string_view url;
        std::string_view label;
        const auto sep = rest.find_first_of(" \t");

        if (sep == std::string_view::npos)
        {
          url = rest;
          label = rest;
        } else {
          url = rest.substr(0, sep);
          label = utils::trim_start(rest.substr(sep));
          if (label.empty())
          {
            label = url;
          }
        }

        const auto href = resolve_link(base_uri, url);

        body
          << "<p class=\"link\"><a href=\""
          << utils::html_escape(href)
          << "\">"
          << utils::html_escape(label)
          << "</a></p>\n";
      }
      else if (
        utils::starts_with(line, "* ") ||
        utils::starts_with(line, "*\t")
      )
      {
        if (!in_list)
        {
          body << "<ul>\n";
          in_list = true;
        }
        body
          << "<li>"
          << utils::html_escape(utils::trim_start(line.substr(1)))
          << "</li>\n";
      }
      else if (utils::starts_with(line, "###"))
      {
        close_list();
        body
          << "<h3>"
          << utils::html_escape(utils::trim_start(line.substr(3)))
          << "</h3>\n";
      }
      else if (utils::starts_with(line, "##"))
      {
        close_list();
        body
          << "<h2>"
          << utils::html_escape(utils::trim_start(line.substr(2)))
          << "</h2>\n";
      }
      else if (utils::starts_with(line, "#"))
      {
        const auto heading = utils::trim_start(line.substr(1));

        close_list();
        body << "<h1>" << utils::html_escape(heading) << "</h1>\n";
        if (!title)
        {
          title = heading;
        }
      }
      else if (utils::starts_with(line, ">"))
      {
        close_list();
        body
          << "<blockquote>"
          << utils::html_escape(utils::trim_start(line.substr(1)))
          << "</blockquote>\n";
      }
      else if (line.empty())
      {
        close_list();
        body << "<br/>\n";
      } else {
        close_list();
        body << "<p>" << utils::html_escape(line) << "</p>\n";
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

    return theme::render_document(title.value_or("Gemini"), body.str());
  }
}
