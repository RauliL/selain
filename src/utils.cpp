/*
 * Copyright (c) 2019-2026, Rauli Laine
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
#include <string_view>

#include <selain/utils.hpp>

namespace selain::utils
{
  namespace
  {
    bool
    has_whitespace(std::string_view text)
    {
      for (const unsigned char c : text)
      {
        if (std::isspace(c))
        {
          return true;
        }
      }

      return false;
    }

    bool
    is_host_char(unsigned char c)
    {
      return (
        std::isalnum(c) ||
        c == '-' ||
        c == '.' ||
        c == ':' ||
        c == '[' ||
        c == ']'
      );
    }

    bool
    looks_like_localhost(std::string_view host)
    {
      constexpr std::string_view localhost = "localhost";

      if (host.size() < localhost.size())
      {
        return false;
      }
      if (host.compare(0, localhost.size(), localhost) != 0)
      {
        return false;
      }

      return (
        host.size() == localhost.size() ||
        host[localhost.size()] == ':'
      );
    }

    bool
    looks_like_domain_or_host(std::string_view text)
    {
      if (text.empty() || has_whitespace(text))
      {
        return false;
      }

      const auto host_end = text.find_first_of("/?#");
      const auto host = text.substr(0, host_end);

      if (host.empty())
      {
        return false;
      }
      for (const unsigned char c : host)
      {
        if (!is_host_char(c))
        {
          return false;
        }
      }
      if (looks_like_localhost(host))
      {
        return true;
      }

      // Require at least one dot so bare words like "hello" become searches,
      // while hosts such as example.com or 127.0.0.1 are treated as URLs.
      return host.find('.') != std::string_view::npos;
    }

    Glib::ustring
    build_search_uri(
      const Glib::ustring& search_engine_template,
      const Glib::ustring& query
    )
    {
      char* escaped = ::g_uri_escape_string(query.c_str(), nullptr, TRUE);
      const Glib::ustring encoded = escaped ? escaped : Glib::ustring();

      g_free(escaped);

      const auto placeholder = search_engine_template.find("%s");

      if (placeholder == Glib::ustring::npos)
      {
        return search_engine_template + encoded;
      }

      Glib::ustring result = search_engine_template;

      result.replace(placeholder, 2, encoded);

      return result;
    }
  }

  Glib::ustring
  js_value_to_text(::JSCValue* value)
  {
    char* text;

    if (::jsc_value_is_undefined(value))
    {
      return "undefined";
    }

    if (
      (text = jsc_value_to_json(value, 0)) ||
      (text = jsc_value_to_string(value))
    )
    {
      Glib::ustring result(text);

      g_free(text);

      return result;
    }

    return Glib::ustring();
  }

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

  Glib::ustring
  resolve_navigation_uri(
    const Glib::ustring& input,
    const Glib::ustring& search_engine_template
  )
  {
    const auto trimmed = trim(input);

    if (trimmed.empty())
    {
      return Glib::ustring();
    }
    if (auto scheme = ::g_uri_parse_scheme(trimmed.c_str()))
    {
      g_free(scheme);

      return trimmed;
    }
    if (looks_like_domain_or_host(trimmed.raw()))
    {
      return std::format("https://{}", trimmed.raw());
    }

    return build_search_uri(search_engine_template, trimmed);
  }
}
