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
#include <format>
#include <sstream>

#include <selain/core/theme.hpp>
#include <selain/protocol/gopher/client.hpp>
#include <selain/protocol/gopher/menu.hpp>
#include <selain/utils/string.hpp>

namespace selain::protocol::gopher
{
  static std::string
  document_scheme(std::string_view base_uri)
  {
    if (base_uri.starts_with("gophers://"))
    {
      return "gophers";
    }

    return "gopher";
  }

  static bool
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

  static std::string
  external_url_from_selector(std::string_view selector)
  {
    if (starts_with_ci(selector, "URL:"))
    {
      return std::string(selector.substr(4));
    }

    return {};
  }

  static const char*
  type_label(char type)
  {
    switch (type)
    {
      case '0':
        return "text";
      case '1':
        return "menu";
      case '2':
        return "cso";
      case '3':
        return "error";
      case '4':
        return "binhex";
      case '5':
        return "dos";
      case '6':
        return "uuencode";
      case '7':
        return "search";
      case '8':
        return "telnet";
      case '9':
        return "binary";
      case 'g':
        return "gif";
      case 'h':
        return "html";
      case 'i':
        return "info";
      case 'I':
        return "image";
      case 'd':
        return "document";
      case 's':
        return "sound";
      case 'p':
        return "png";
      case 'T':
        return "tn3270";
      default:
        return "item";
    }
  }

  static bool
  is_linkable_type(char type)
  {
    switch (type)
    {
      case 'i':
      case '3':
      case '2':
      case '8':
      case 'T':
        return false;
      default:
        return true;
    }
  }

  static std::vector<std::string_view>
  split_tabs(std::string_view line)
  {
    std::vector<std::string_view> parts;
    std::size_t start = 0;

    while (start <= line.size())
    {
      const auto end = line.find('\t', start);

      if (end == std::string_view::npos)
      {
        parts.push_back(line.substr(start));
        break;
      }
      parts.push_back(line.substr(start, end - start));
      start = end + 1;
    }

    return parts;
  }

  std::string
  menu_to_html(std::string_view menu, std::string_view base_uri)
  {
    using selain::utils::string::html_escape;

    const auto scheme = document_scheme(base_uri);
    std::ostringstream body;

    body << "<h1>Gopher menu</h1>\n<menu>\n";

    std::size_t start = 0;

    while (start <= menu.size())
    {
      const auto end = menu.find('\n', start);
      auto line = menu.substr(
        start,
        end == std::string_view::npos ? menu.size() - start : end - start
      );

      if (!line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }

      if (line == ".")
      {
        break;
      }

      if (line.empty())
      {
        if (end == std::string_view::npos)
        {
          break;
        }
        start = end + 1;
        continue;
      }

      const char type = line.front();
      const auto fields = split_tabs(line.substr(1));
      const std::string_view label = fields.empty() ? std::string_view{} : fields[0];
      const std::string_view selector = fields.size() > 1 ? fields[1] : std::string_view{};
      const std::string_view host = fields.size() > 2 ? fields[2] : std::string_view{};
      int port = 70;

      if (fields.size() > 3 && !fields[3].empty())
      {
        try
        {
          port = std::stoi(std::string(fields[3]));
        }
        catch (...)
        {
          port = 70;
        }
      }

      body << "<li data-type=\"" << html_escape(type_label(type)) << "\">";

      if (type == 'i' || type == '3' || !is_linkable_type(type) || host.empty())
      {
        body << html_escape(label);
      } else {
        std::string href;
        const auto url_selector = external_url_from_selector(selector);

        if (type == 'h' && !url_selector.empty())
        {
          href = url_selector;
        } else {
          href = build_uri(
            scheme,
            std::string(host),
            port,
            type,
            std::string(selector)
          );
        }

        body
          << "<a href=\""
          << html_escape(href)
          << "\">"
          << html_escape(label)
          << "</a>";
      }

      body << "</li>\n";

      if (end == std::string_view::npos)
      {
        break;
      }
      start = end + 1;
    }

    body << "</menu>\n";

    return theme::render_document("Gopher menu", body.str());
  }

  std::string
  text_to_html(std::string_view text, std::string_view)
  {
    std::string cleaned;
    cleaned.reserve(text.size());

    std::size_t start = 0;

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

      if (line == "." && end == std::string_view::npos)
      {
        break;
      }
      if (line == "." && end != std::string_view::npos)
      {
        // Trailing Gopher terminator; omit it from display.
        break;
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
      "Gopher text",
      std::format("<pre>{}</pre>\n", utils::string::html_escape(cleaned))
    );
  }
}
