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
#include <selain/adblock/easylist.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <vector>

namespace selain::adblock
{
  namespace
  {
    constexpr std::string_view separator_class = "[^%.0-9a-z_-]";
    constexpr std::string_view hostname_anchor = "^[a-z-]+://([^/?#]+\\.)?";
    constexpr std::string_view hostname_anchor_dot = "^[a-z-]+://([^/?#]+)?";

    enum class ResourceType : std::uint16_t
    {
      document       = 1u << 0,
      image          = 1u << 1,
      style_sheet    = 1u << 2,
      script         = 1u << 3,
      font           = 1u << 4,
      raw            = 1u << 5,
      svg_document   = 1u << 6,
      media          = 1u << 7,
      popup          = 1u << 8,
      ping           = 1u << 9,
      fetch          = 1u << 10,
      websocket      = 1u << 11,
      other          = 1u << 12,
      child_document = 1u << 13,
    };

    constexpr std::uint16_t all_resource_types =
      static_cast<std::uint16_t>(ResourceType::document) |
      static_cast<std::uint16_t>(ResourceType::image) |
      static_cast<std::uint16_t>(ResourceType::style_sheet) |
      static_cast<std::uint16_t>(ResourceType::script) |
      static_cast<std::uint16_t>(ResourceType::font) |
      static_cast<std::uint16_t>(ResourceType::raw) |
      static_cast<std::uint16_t>(ResourceType::svg_document) |
      static_cast<std::uint16_t>(ResourceType::media) |
      static_cast<std::uint16_t>(ResourceType::popup) |
      static_cast<std::uint16_t>(ResourceType::ping) |
      static_cast<std::uint16_t>(ResourceType::fetch) |
      static_cast<std::uint16_t>(ResourceType::websocket) |
      static_cast<std::uint16_t>(ResourceType::other) |
      static_cast<std::uint16_t>(ResourceType::child_document);

    constexpr std::array<std::pair<ResourceType, std::string_view>, 14>
    resource_type_names{{
      {ResourceType::document, "document"},
      {ResourceType::image, "image"},
      {ResourceType::style_sheet, "style-sheet"},
      {ResourceType::script, "script"},
      {ResourceType::font, "font"},
      {ResourceType::raw, "raw"},
      {ResourceType::svg_document, "svg-document"},
      {ResourceType::media, "media"},
      {ResourceType::popup, "popup"},
      {ResourceType::ping, "ping"},
      {ResourceType::fetch, "fetch"},
      {ResourceType::websocket, "websocket"},
      {ResourceType::other, "other"},
      {ResourceType::child_document, "child-document"},
    }};

    enum class LoadType
    {
      first_party,
      third_party,
    };

    enum class ActionType
    {
      block,
      css_display_none,
      ignore_previous_rules,
    };

    struct Domains
    {
      std::vector<std::string> include;
      std::vector<std::string> exclude;
    };

    struct Rule
    {
      std::string url_filter;
      std::string selector;
      Domains domains;
      std::uint16_t resource_types{0};
      std::optional<LoadType> load_type;
      ActionType action{ActionType::block};
      bool case_sensitive{false};
    };

    [[nodiscard]] std::string_view
    trim(std::string_view text)
    {
      while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
      {
        text.remove_prefix(1);
      }
      while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
      {
        text.remove_suffix(1);
      }
      return text;
    }

    [[nodiscard]] bool
    starts_with(std::string_view text, std::string_view prefix)
    {
      return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
    }

    [[nodiscard]] bool
    ends_with(std::string_view text, std::string_view suffix)
    {
      return text.size() >= suffix.size() &&
        text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    void
    append_json_escaped(std::string& out, std::string_view text)
    {
      out.push_back('"');
      for (const unsigned char c : text)
      {
        switch (c)
        {
          case '"':
            out += "\\\"";
            break;
          case '\\':
            out += "\\\\";
            break;
          case '\b':
            out += "\\b";
            break;
          case '\f':
            out += "\\f";
            break;
          case '\n':
            out += "\\n";
            break;
          case '\r':
            out += "\\r";
            break;
          case '\t':
            out += "\\t";
            break;
          default:
            if (c < 0x20)
            {
              constexpr char hex[] = "0123456789abcdef";
              out += "\\u00";
              out.push_back(hex[c >> 4]);
              out.push_back(hex[c & 0xf]);
            }
            else
            {
              out.push_back(static_cast<char>(c));
            }
            break;
        }
      }
      out.push_back('"');
    }

    [[nodiscard]] bool
    is_comment(std::string_view line)
    {
      return line.empty() || line.front() == '!' || line.front() == '[';
    }

    [[nodiscard]] bool
    is_entity_domain(std::string_view domain)
    {
      return ends_with(domain, ".*");
    }

    [[nodiscard]] bool
    is_valid_domain(std::string_view domain)
    {
      if (domain.empty() || is_entity_domain(domain))
      {
        return false;
      }
      for (const unsigned char c : domain)
      {
        if (!(std::isalnum(c) || c == '.' || c == '-' || c == '_'))
        {
          return false;
        }
      }
      return true;
    }

    void
    split_domains(
      std::string_view text,
      char separator,
      Domains& domains
    )
    {
      std::size_t start = 0;
      while (start <= text.size())
      {
        const auto end = text.find(separator, start);
        const auto part = trim(
          text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start)
        );
        if (!part.empty())
        {
          if (part.front() == '~')
          {
            const auto domain = part.substr(1);
            if (is_valid_domain(domain))
            {
              domains.exclude.emplace_back(domain);
            }
          }
          else if (is_valid_domain(part))
          {
            domains.include.emplace_back(part);
          }
        }
        if (end == std::string_view::npos)
        {
          break;
        }
        start = end + 1;
      }
    }

    [[nodiscard]] std::string
    webkit_domain(std::string_view domain)
    {
      std::string out = "*";
      out += domain;
      return out;
    }

    [[nodiscard]] bool
    is_procedural_selector(std::string_view selector)
    {
      static constexpr std::array needles{
        std::string_view(":has("),
        std::string_view(":has-text("),
        std::string_view(":xpath("),
        std::string_view(":matches-css("),
        std::string_view(":matches-attr("),
        std::string_view(":matches-path("),
        std::string_view(":min-text-length("),
        std::string_view(":upward("),
        std::string_view(":remove("),
        std::string_view(":style("),
        std::string_view(":watch-attr("),
        std::string_view(":matches-property("),
        std::string_view(":-abp-has("),
        std::string_view(":-abp-contains("),
        std::string_view(":-abp-properties("),
      };
      return std::any_of(
        needles.begin(),
        needles.end(),
        [selector](std::string_view needle) {
          return selector.find(needle) != std::string_view::npos;
        }
      );
    }

    [[nodiscard]] std::optional<std::size_t>
    find_cosmetic_separator(std::string_view line)
    {
      for (std::size_t i = 0; i < line.size(); ++i)
      {
        if (line[i] != '#')
        {
          continue;
        }
        const auto rest = line.substr(i);
        if (
          starts_with(rest, "##") ||
          starts_with(rest, "#?#") ||
          starts_with(rest, "#@#") ||
          starts_with(rest, "#@?#") ||
          starts_with(rest, "#$#") ||
          starts_with(rest, "#@$#") ||
          starts_with(rest, "#%#") ||
          starts_with(rest, "#@%#")
        )
        {
          return i;
        }
      }
      return std::nullopt;
    }

    [[nodiscard]] std::optional<std::size_t>
    options_start(std::string_view pattern)
    {
      const auto at = pattern.rfind('$');
      if (at == std::string_view::npos)
      {
        return std::nullopt;
      }
      if (at > 0 && pattern[at - 1] == '\\')
      {
        return std::nullopt;
      }
      // Regular expression filters look like /...$/, keep the trailing $.
      if (at + 1 < pattern.size() && pattern[at + 1] == '/')
      {
        return std::nullopt;
      }
      if (
        pattern.size() >= 2 &&
        pattern.front() == '/' &&
        pattern.back() == '/' &&
        at == pattern.size() - 1
      )
      {
        return std::nullopt;
      }
      return at;
    }

    [[nodiscard]] std::optional<ResourceType>
    parse_resource_type(std::string_view name)
    {
      if (name == "script")
      {
        return ResourceType::script;
      }
      if (name == "image" || name == "img")
      {
        return ResourceType::image;
      }
      if (name == "stylesheet" || name == "css")
      {
        return ResourceType::style_sheet;
      }
      if (name == "font")
      {
        return ResourceType::font;
      }
      if (name == "media")
      {
        return ResourceType::media;
      }
      if (name == "xmlhttprequest" || name == "xhr")
      {
        return ResourceType::fetch;
      }
      if (name == "subdocument" || name == "frame")
      {
        return ResourceType::child_document;
      }
      if (name == "ping")
      {
        return ResourceType::ping;
      }
      if (name == "popup")
      {
        return ResourceType::popup;
      }
      if (name == "other" || name == "object" || name == "object-subrequest")
      {
        return ResourceType::other;
      }
      if (name == "websocket")
      {
        return ResourceType::websocket;
      }
      if (name == "document" || name == "doc")
      {
        return ResourceType::document;
      }
      return std::nullopt;
    }

    [[nodiscard]] bool
    parse_options(std::string_view text, Rule& rule)
    {
      std::uint16_t include_types = 0;
      std::uint16_t exclude_types = 0;
      std::size_t start = 0;

      while (start <= text.size())
      {
        const auto end = text.find(',', start);
        const auto part = trim(
          text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start)
        );
        if (!part.empty())
        {
          const auto eq = part.find('=');
          const auto name = part.substr(0, eq);

          if (eq != std::string_view::npos)
          {
            const auto value = part.substr(eq + 1);
            if (name != "domain" && name != "from")
            {
              return false;
            }
            if (starts_with(value, "/"))
            {
              return false;
            }
            split_domains(value, '|', rule.domains);
          }
          else if (name == "all")
          {
            // No extra trigger constraints.
          }
          else if (name == "third-party" || name == "3p")
          {
            rule.load_type = LoadType::third_party;
          }
          else if (
            name == "~third-party" ||
            name == "~3p" ||
            name == "first-party" ||
            name == "1p"
          )
          {
            rule.load_type = LoadType::first_party;
          }
          else if (name == "match-case")
          {
            rule.case_sensitive = true;
          }
          else if (name == "important")
          {
            // WebKit has no priority bit; keep the rule.
          }
          else if (starts_with(name, "~"))
          {
            const auto type = parse_resource_type(name.substr(1));
            if (!type)
            {
              return false;
            }
            exclude_types |= static_cast<std::uint16_t>(*type);
          }
          else
          {
            const auto type = parse_resource_type(name);
            if (!type)
            {
              return false;
            }
            include_types |= static_cast<std::uint16_t>(*type);
          }
        }

        if (end == std::string_view::npos)
        {
          break;
        }
        start = end + 1;
      }

      if (include_types != 0 && exclude_types != 0)
      {
        return false;
      }
      if (include_types != 0)
      {
        rule.resource_types = include_types;
      }
      else if (exclude_types != 0)
      {
        rule.resource_types = static_cast<std::uint16_t>(all_resource_types & ~exclude_types);
      }
      return true;
    }

    [[nodiscard]] bool
    matches_everything(std::string_view pattern)
    {
      std::size_t i = 0;
      std::size_t j = pattern.size();
      while (i < j && (pattern[i] == '|' || pattern[i] == '*'))
      {
        ++i;
      }
      while (j > i && (pattern[j - 1] == '|' || pattern[j - 1] == '*'))
      {
        --j;
      }
      return i == j;
    }

    [[nodiscard]] std::optional<std::size_t>
    class_end(std::string_view pattern, std::size_t open)
    {
      auto index = open + 1;
      if (index < pattern.size() && pattern[index] == '^')
      {
        ++index;
      }
      const auto first = index;
      while (index < pattern.size())
      {
        const auto ch = static_cast<unsigned char>(pattern[index]);
        if (ch >= 0x80)
        {
          return std::nullopt;
        }
        if (ch == '\\')
        {
          if (index + 1 >= pattern.size())
          {
            return std::nullopt;
          }
          const auto next = static_cast<unsigned char>(pattern[index + 1]);
          if (std::isalnum(next) || next >= 0x80)
          {
            return std::nullopt;
          }
          index += 2;
          continue;
        }
        if (ch == ']')
        {
          return index == first ? std::nullopt : std::optional{index};
        }
        ++index;
      }
      return std::nullopt;
    }

    [[nodiscard]] bool
    is_valid_webkit_regex(std::string_view pattern)
    {
      if (pattern.empty())
      {
        return false;
      }

      constexpr std::size_t max_depth = 32;
      bool filled[max_depth] = {};
      std::size_t depth = 0;
      std::size_t index = 0;
      bool atom = false;

      while (index < pattern.size())
      {
        const auto ch = static_cast<unsigned char>(pattern[index]);
        if (ch >= 0x80)
        {
          return false;
        }

        switch (ch)
        {
          case '\\':
          {
            if (index + 1 >= pattern.size())
            {
              return false;
            }
            const auto next = static_cast<unsigned char>(pattern[index + 1]);
            if (std::isalnum(next) || next >= 0x80)
            {
              return false;
            }
            index += 2;
            atom = true;
            break;
          }
          case '[':
          {
            const auto end = class_end(pattern, index);
            if (!end)
            {
              return false;
            }
            index = *end + 1;
            atom = true;
            break;
          }
          case ']':
            return false;
          case '(':
            if (index + 1 < pattern.size() && pattern[index + 1] == '?')
            {
              return false;
            }
            if (depth == max_depth)
            {
              return false;
            }
            filled[depth] = false;
            ++depth;
            ++index;
            atom = false;
            continue;
          case ')':
            if (depth == 0 || !filled[depth - 1])
            {
              return false;
            }
            --depth;
            ++index;
            atom = true;
            break;
          case '*':
          case '+':
          case '?':
            if (!atom)
            {
              return false;
            }
            ++index;
            atom = false;
            continue;
          case '^':
            if (index != 0)
            {
              return false;
            }
            ++index;
            atom = false;
            continue;
          case '$':
            if (index + 1 != pattern.size())
            {
              return false;
            }
            ++index;
            atom = false;
            continue;
          case '|':
          case '{':
          case '}':
            return false;
          default:
            ++index;
            atom = true;
            break;
        }

        if (depth > 0)
        {
          filled[depth - 1] = true;
        }
      }

      return depth == 0;
    }

    void
    escape_pattern_body(std::string& stage, std::string_view body)
    {
      stage.clear();
      stage.reserve(body.size() * 2);
      for (const char ch : body)
      {
        switch (ch)
        {
          case '.':
          case '+':
          case '?':
          case '$':
          case '{':
          case '}':
          case '(':
          case ')':
          case '|':
          case '[':
          case ']':
          case '\\':
            stage.push_back('\\');
            stage.push_back(ch);
            break;
          case '^':
            stage += separator_class;
            break;
          default:
            stage.push_back(ch);
            break;
        }
      }
    }

    void
    expand_regex_classes(std::string& out, std::string_view pattern)
    {
      bool in_class = false;
      for (std::size_t index = 0; index < pattern.size();)
      {
        const char ch = pattern[index];
        if (ch == '\\' && index + 1 < pattern.size())
        {
          const char next = pattern[index + 1];
          std::string_view expansion;
          switch (next)
          {
            case 'w':
              expansion = in_class ? "a-zA-Z0-9_" : "[a-zA-Z0-9_]";
              break;
            case 'W':
              expansion = in_class ? std::string_view{} : "[^a-zA-Z0-9_]";
              break;
            case 'd':
              expansion = in_class ? "0-9" : "[0-9]";
              break;
            case 'D':
              expansion = in_class ? std::string_view{} : "[^0-9]";
              break;
            default:
              break;
          }
          if (!expansion.empty())
          {
            out += expansion;
            index += 2;
            continue;
          }
          out.push_back('\\');
          out.push_back(next);
          index += 2;
          continue;
        }

        if (ch == '[')
        {
          in_class = true;
        }
        else if (ch == ']')
        {
          in_class = false;
        }

        if (ch == '{' && !in_class)
        {
          // Approximate `{n,}` as `+` when digits precede the comma.
          std::size_t i = index + 1;
          while (i < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[i])))
          {
            ++i;
          }
          if (i > index + 1 && i + 1 < pattern.size() && pattern[i] == ',' && pattern[i + 1] == '}')
          {
            out.push_back('+');
            index = i + 2;
            continue;
          }
        }

        out.push_back(ch);
        ++index;
      }
    }

    [[nodiscard]] std::optional<std::string>
    pattern_to_regex(std::string_view pattern, bool end_anchored)
    {
      if (matches_everything(pattern))
      {
        return end_anchored ? std::string(".*$") : std::string(".*");
      }

      auto body = pattern;
      bool hostname = false;
      bool left = false;
      bool right = end_anchored;

      if (starts_with(body, "||"))
      {
        hostname = true;
        body.remove_prefix(2);
      }
      else if (starts_with(body, "|"))
      {
        left = true;
        body.remove_prefix(1);
      }
      if (ends_with(body, "|"))
      {
        right = true;
        body.remove_suffix(1);
      }

      std::string out;
      if (
        !end_anchored &&
        body.size() > 2 &&
        body.front() == '/' &&
        body.back() == '/'
      )
      {
        expand_regex_classes(out, body.substr(1, body.size() - 2));
        if (!is_valid_webkit_regex(out))
        {
          return std::nullopt;
        }
        return out;
      }

      std::string escaped;
      escape_pattern_body(escaped, body);

      std::string_view trimmed = escaped;
      if (!hostname && !left)
      {
        while (!trimmed.empty() && trimmed.front() == '*')
        {
          trimmed.remove_prefix(1);
        }
      }
      if (!right)
      {
        while (!trimmed.empty() && trimmed.back() == '*')
        {
          trimmed.remove_suffix(1);
        }
      }

      if (hostname)
      {
        out += starts_with(trimmed, "\\.") ? hostname_anchor_dot : hostname_anchor;
      }
      else if (left)
      {
        out.push_back('^');
      }

      for (std::size_t i = 0; i < trimmed.size();)
      {
        if (trimmed[i] != '*')
        {
          out.push_back(trimmed[i]);
          ++i;
          continue;
        }
        out += ".*";
        while (i < trimmed.size() && trimmed[i] == '*')
        {
          ++i;
        }
      }

      if (right)
      {
        out.push_back('$');
      }
      if (!is_valid_webkit_regex(out))
      {
        return std::nullopt;
      }
      return out;
    }

    [[nodiscard]] bool
    ends_with_separator(std::string_view pattern)
    {
      auto trimmed = pattern;
      if (ends_with(trimmed, "|"))
      {
        trimmed.remove_suffix(1);
      }
      return ends_with(trimmed, "^");
    }

    void
    append_string_array(
      std::string& out,
      const std::vector<std::string>& values,
      bool webkit_domains
    )
    {
      out.push_back('[');
      for (std::size_t i = 0; i < values.size(); ++i)
      {
        if (i != 0)
        {
          out.push_back(',');
        }
        if (webkit_domains)
        {
          append_json_escaped(out, webkit_domain(values[i]));
        }
        else
        {
          append_json_escaped(out, values[i]);
        }
      }
      out.push_back(']');
    }

    void
    append_resource_types(std::string& out, std::uint16_t types)
    {
      out.push_back('[');
      bool first = true;
      for (const auto& [type, name] : resource_type_names)
      {
        if ((types & static_cast<std::uint16_t>(type)) == 0)
        {
          continue;
        }
        if (!first)
        {
          out.push_back(',');
        }
        first = false;
        append_json_escaped(out, name);
      }
      out.push_back(']');
    }

    void
    append_rule_json(std::string& out, const Rule& rule)
    {
      out += "{\"trigger\":{\"url-filter\":";
      append_json_escaped(out, rule.url_filter);
      if (rule.case_sensitive)
      {
        out += ",\"url-filter-is-case-sensitive\":true";
      }
      if (rule.resource_types != 0)
      {
        out += ",\"resource-type\":";
        append_resource_types(out, rule.resource_types);
      }
      if (rule.load_type)
      {
        out += ",\"load-type\":[";
        append_json_escaped(
          out,
          *rule.load_type == LoadType::third_party ? "third-party" : "first-party"
        );
        out.push_back(']');
      }
      if (!rule.domains.include.empty())
      {
        out += ",\"if-domain\":";
        append_string_array(out, rule.domains.include, true);
      }
      else if (!rule.domains.exclude.empty())
      {
        out += ",\"unless-domain\":";
        append_string_array(out, rule.domains.exclude, true);
      }
      out += "},\"action\":{\"type\":";
      switch (rule.action)
      {
        case ActionType::block:
          append_json_escaped(out, "block");
          break;
        case ActionType::css_display_none:
          append_json_escaped(out, "css-display-none");
          out += ",\"selector\":";
          append_json_escaped(out, rule.selector);
          break;
        case ActionType::ignore_previous_rules:
          append_json_escaped(out, "ignore-previous-rules");
          break;
      }
      out += "}}";
    }

    void
    emit_network_rules(
      std::vector<Rule>& rules,
      std::string_view pattern,
      Domains domains,
      std::uint16_t resource_types,
      std::optional<LoadType> load_type,
      bool case_sensitive,
      bool exception
    )
    {
      // Safari forbids if-domain and unless-domain on the same rule.
      if (!domains.include.empty() && !domains.exclude.empty())
      {
        return;
      }

      const auto action = exception
        ? ActionType::ignore_previous_rules
        : ActionType::block;

      auto emit = [&](std::string url_filter) {
        Rule rule;
        rule.url_filter = std::move(url_filter);
        rule.domains = domains;
        rule.resource_types = resource_types;
        rule.load_type = load_type;
        rule.case_sensitive = case_sensitive;
        rule.action = action;
        rules.push_back(std::move(rule));
      };

      if (const auto regex = pattern_to_regex(pattern, false))
      {
        emit(*regex);
      }
      if (ends_with_separator(pattern))
      {
        if (const auto regex = pattern_to_regex(pattern, true))
        {
          emit(*regex);
        }
      }
    }

    void
    parse_line(std::string_view line, std::vector<Rule>& rules)
    {
      line = trim(line);
      if (is_comment(line))
      {
        return;
      }

      if (const auto sep = find_cosmetic_separator(line))
      {
        const auto rest = line.substr(*sep);
        std::string_view domains_text = line.substr(0, *sep);
        std::string_view selector;
        bool exception = false;
        bool scriptlet = false;

        if (starts_with(rest, "##"))
        {
          selector = rest.substr(2);
        }
        else if (starts_with(rest, "#?#"))
        {
          selector = rest.substr(3);
        }
        else if (starts_with(rest, "#@#") || starts_with(rest, "#@?#"))
        {
          exception = true;
        }
        else if (
          starts_with(rest, "#$#") ||
          starts_with(rest, "#@$#") ||
          starts_with(rest, "#%#") ||
          starts_with(rest, "#@%#")
        )
        {
          scriptlet = true;
        }

        if (exception || scriptlet)
        {
          return;
        }
        if (selector.empty() || starts_with(selector, "+js(") || starts_with(selector, "^"))
        {
          return;
        }
        if (is_procedural_selector(selector))
        {
          return;
        }

        Rule rule;
        rule.url_filter = ".*";
        rule.selector = std::string(selector);
        rule.action = ActionType::css_display_none;
        split_domains(domains_text, ',', rule.domains);
        if (!rule.domains.include.empty() && !rule.domains.exclude.empty())
        {
          return;
        }
        rules.push_back(std::move(rule));
        return;
      }

      bool exception = false;
      if (starts_with(line, "@@"))
      {
        exception = true;
        line.remove_prefix(2);
      }

      Rule draft;
      auto pattern = line;
      if (const auto at = options_start(line))
      {
        pattern = line.substr(0, *at);
        if (!parse_options(line.substr(*at + 1), draft))
        {
          return;
        }
      }

      emit_network_rules(
        rules,
        pattern,
        std::move(draft.domains),
        draft.resource_types,
        draft.load_type,
        draft.case_sensitive,
        exception
      );
    }
  }

  bool
  looks_like_content_blocker_json(std::string_view text)
  {
    auto trimmed = trim(text);
    if (trimmed.empty() || trimmed.front() != '[')
    {
      return false;
    }
    trimmed.remove_prefix(1);
    trimmed = trim(trimmed);
    // EasyList headers also start with '[', e.g. "[Adblock Plus 2.0]".
    // Real content-blocker JSON is an array of objects, or empty "[]".
    return trimmed.empty() || trimmed.front() == '{' || trimmed.front() == ']';
  }

  std::string
  easylist_to_content_blocker_json(std::string_view text)
  {
    std::vector<Rule> rules;
    rules.reserve(4096);

    std::size_t start = 0;
    while (start <= text.size())
    {
      const auto end = text.find('\n', start);
      auto line = text.substr(
        start,
        end == std::string_view::npos ? std::string_view::npos : end - start
      );
      if (!line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }
      parse_line(line, rules);
      if (end == std::string_view::npos)
      {
        break;
      }
      start = end + 1;
    }

    std::string out;
    out.reserve(rules.size() * 96);
    out.push_back('[');
    for (std::size_t i = 0; i < rules.size(); ++i)
    {
      if (i != 0)
      {
        out.push_back(',');
      }
      append_rule_json(out, rules[i]);
    }
    out.push_back(']');
    return out;
  }
}
