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
#include <cstring>
#include <format>

#include <selain/utils/string.hpp>
#include <selain/utils/url.hpp>

namespace selain::utils::url
{
  static GUri*
  parse_uri(const Glib::ustring& uri)
  {
    GError* error = nullptr;
    auto* parsed = g_uri_parse(uri.c_str(), G_URI_FLAGS_NONE, &error);

    if (error)
    {
      g_error_free(error);
    }

    return parsed;
  }

  static std::optional<Glib::ustring>
  uri_to_string(GUri* uri)
  {
    auto* text = g_uri_to_string(uri);
    std::optional<Glib::ustring> result;

    if (text)
    {
      result = text;
    }
    g_free(text);

    return result;
  }

  static inline bool
  is_hierarchy_root(const char* path, const char* query, const char* fragment)
  {
    return (
      (!path || !*path || !std::strcmp(path, "/")) &&
      !query &&
      !fragment
    );
  }

  static std::optional<Glib::ustring>
  rebuild_uri_with_path(GUri* uri, const char* path)
  {
    auto* rebuilt = g_uri_build(
      G_URI_FLAGS_NONE,
      g_uri_get_scheme(uri),
      g_uri_get_userinfo(uri),
      g_uri_get_host(uri),
      g_uri_get_port(uri),
      path,
      nullptr,
      nullptr
    );

    if (!rebuilt)
    {
      return std::nullopt;
    }

    const auto result = uri_to_string(rebuilt);

    g_uri_unref(rebuilt);

    return result;
  }

  static std::optional<std::string>
  parent_path(std::string path)
  {
    // Do not attempt to parse empty path.
    if (path.empty() || path == "/")
    {
      return std::nullopt;
    }

    // Strip trailing slash, if there is one.
    if (path.size() > 1 && path.back() == '/')
    {
      path.pop_back();
    }

    const auto slash = path.rfind('/');

    if (slash == std::string::npos)
    {
      return std::nullopt;
    }
    else if (slash == 0)
    {
      return "/";
    }

    return path.substr(0, slash);
  }

  bool
  looks_like_ip(const Glib::ustring& input)
  {
    // Empty string is not an IP address.
    if (input.empty())
    {
      return false;
    }

    // IPv6 addresses inside URI/URL are enclosed with brackets.
    if (input[0] == '[')
    {
      return true;
    }

    // Otherwise check if the input looks like IPv4 address.
    for (const auto c : input)
    {
      if (!std::isdigit(c) && c != '.')
      {
        return false;
      }
    }

    return input.find('.') != Glib::ustring::npos;
  }

  std::optional<Glib::ustring>
  get_host(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return std::nullopt;
    }

    auto* parsed = parse_uri(uri);

    if (!parsed)
    {
      return std::nullopt;
    }

    const char* host = g_uri_get_host(parsed);
    std::optional<Glib::ustring> result;

    if (host && *host)
    {
      result = host;
    }
    g_uri_unref(parsed);

    return result;
  }

  std::optional<Glib::ustring>
  get_parent(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return std::nullopt;
    }

    auto* parsed = parse_uri(uri);

    if (!parsed)
    {
      return std::nullopt;
    }

    const char* path = g_uri_get_path(parsed);
    const auto parent = parent_path(path ? path : "");

    if (!parent || parent->empty())
    {
      g_uri_unref(parsed);

      return std::nullopt;
    }

    const auto result = rebuild_uri_with_path(parsed, parent->c_str());

    g_uri_unref(parsed);

    return result;
  }

  std::optional<Glib::ustring>
  get_root(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return std::nullopt;
    }

    auto* parsed = parse_uri(uri);

    if (!parsed)
    {
      return std::nullopt;
    }

    if (is_hierarchy_root(
      g_uri_get_path(parsed),
      g_uri_get_query(parsed),
      g_uri_get_fragment(parsed)
    ))
    {
      g_uri_unref(parsed);

      return std::nullopt;
    }

    const auto result = rebuild_uri_with_path(parsed, "/");

    g_uri_unref(parsed);

    return result;
  }

  static inline bool
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

  static bool
  looks_like_localhost(const Glib::ustring& host)
  {
    static const Glib::ustring localhost = "localhost";

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

  static bool
  looks_like_domain_or_host(const Glib::ustring& text)
  {
    if (text.empty() || utils::string::has_whitespace(text))
    {
      return false;
    }

    const auto host_end = text.find_first_of("/?#");
    const auto host = text.substr(0, host_end);

    if (host.empty())
    {
      return false;
    }
    for (const auto c : host)
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
    return host.find('.') != Glib::ustring::npos;
  }

  static Glib::ustring
  build_search_uri(
    const Glib::ustring& search_engine_template,
    const Glib::ustring& query
  )
  {
    auto* escaped = g_uri_escape_string(query.c_str(), nullptr, TRUE);
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

  std::optional<Glib::ustring>
  resolve_navigation_uri(
    const Glib::ustring& input,
    const Glib::ustring& search_engine_template
  )
  {
    const auto trimmed = utils::string::trim(input);

    if (trimmed.empty())
    {
      return std::nullopt;
    }
    if (auto scheme = g_uri_parse_scheme(trimmed.c_str()))
    {
      g_free(scheme);

      // g_uri_parse_scheme treats host:port (e.g. localhost:8080) as a
      // scheme. Only keep the input unchanged when it is not domain-like.
      if (!looks_like_domain_or_host(trimmed))
      {
        return trimmed;
      }
    }
    if (looks_like_domain_or_host(trimmed))
    {
      return std::format("https://{}", trimmed.raw());
    }

    return build_search_uri(search_engine_template, trimmed);
  }
}
