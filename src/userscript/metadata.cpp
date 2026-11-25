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
#include <algorithm>
#include <cctype>

#include <selain/userscript/metadata.hpp>

namespace selain::userscript
{
  namespace
  {
    constexpr std::string_view meta_start = "// ==UserScript==";
    constexpr std::string_view meta_end = "// ==/UserScript==";

    std::string_view
    trim_view(std::string_view value)
    {
      while (!value.empty() &&
             std::isspace(static_cast<unsigned char>(value.front())))
      {
        value.remove_prefix(1);
      }
      while (!value.empty() &&
             std::isspace(static_cast<unsigned char>(value.back())))
      {
        value.remove_suffix(1);
      }

      return value;
    }

    bool
    equals_ignore_case(std::string_view left, std::string_view right)
    {
      if (left.size() != right.size())
      {
        return false;
      }

      return std::equal(
        left.begin(),
        left.end(),
        right.begin(),
        [](char a, char b) {
          return std::tolower(static_cast<unsigned char>(a)) ==
                 std::tolower(static_cast<unsigned char>(b));
        }
      );
    }

    bool
    is_regex_include(std::string_view pattern)
    {
      return pattern.size() >= 2 &&
             pattern.front() == '/' &&
             pattern.back() == '/';
    }

    bool
    looks_like_url_pattern(std::string_view pattern)
    {
      return pattern.find("://") != std::string_view::npos;
    }

    void
    append_unique(std::vector<std::string>& list, std::string pattern)
    {
      if (std::find(list.begin(), list.end(), pattern) == list.end())
      {
        list.push_back(std::move(pattern));
      }
    }

    std::string
    strip_metadata_block(
      std::string_view source,
      std::size_t block_begin,
      std::size_t block_end
    )
    {
      std::string body;

      body.reserve(source.size() - (block_end - block_begin));
      body.append(source.substr(0, block_begin));
      body.append(source.substr(block_end));

      return body;
    }
  }

  std::optional<std::string>
  convert_match_pattern(std::string_view pattern)
  {
    pattern = trim_view(pattern);

    if (pattern.empty())
    {
      return std::nullopt;
    }
    if (equals_ignore_case(pattern, "<all_urls>") || pattern == "*")
    {
      return std::string("*://*/*");
    }
    if (!looks_like_url_pattern(pattern))
    {
      return std::nullopt;
    }

    return std::string(pattern);
  }

  std::optional<std::string>
  convert_include_pattern(std::string_view pattern)
  {
    pattern = trim_view(pattern);

    if (pattern.empty())
    {
      return std::nullopt;
    }
    if (is_regex_include(pattern))
    {
      return std::nullopt;
    }
    if (equals_ignore_case(pattern, "<all_urls>") || pattern == "*")
    {
      return std::string("*://*/*");
    }
    if (!looks_like_url_pattern(pattern))
    {
      return std::nullopt;
    }

    return std::string(pattern);
  }

  std::optional<ParsedScript>
  parse(std::string_view source)
  {
    const auto start = source.find(meta_start);

    if (start == std::string_view::npos)
    {
      return std::nullopt;
    }

    const auto end = source.find(meta_end, start + meta_start.size());

    if (end == std::string_view::npos)
    {
      return std::nullopt;
    }

    const auto block = source.substr(
      start + meta_start.size(),
      end - (start + meta_start.size())
    );
    const auto block_end = end + meta_end.size();
    // Consume a single trailing newline after the closing marker.
    auto body_end = block_end;

    if (body_end < source.size() &&
        (source[body_end] == '\n' || source[body_end] == '\r'))
    {
      if (source[body_end] == '\r' &&
          body_end + 1 < source.size() &&
          source[body_end + 1] == '\n')
      {
        body_end += 2;
      } else {
        ++body_end;
      }
    }

    ParsedScript script;

    script.source = strip_metadata_block(source, start, body_end);

    std::size_t line_begin = 0;

    while (line_begin <= block.size())
    {
      const auto line_end = block.find('\n', line_begin);
      auto line = block.substr(
        line_begin,
        line_end == std::string_view::npos
          ? std::string_view::npos
          : line_end - line_begin
      );

      if (!line.empty() && line.back() == '\r')
      {
        line.remove_suffix(1);
      }

      line = trim_view(line);
      line_begin = line_end == std::string_view::npos
        ? block.size() + 1
        : line_end + 1;

      if (line.empty())
      {
        continue;
      }
      if (line.starts_with("//"))
      {
        line.remove_prefix(2);
        line = trim_view(line);
      }
      if (line.empty() || line.front() != '@')
      {
        continue;
      }

      line.remove_prefix(1);
      const auto key_end = line.find_first_of(" \t");
      const auto key = trim_view(line.substr(0, key_end));
      const auto value = key_end == std::string_view::npos
        ? std::string_view{}
        : trim_view(line.substr(key_end));

      if (key.empty())
      {
        continue;
      }

      if (key == "name")
      {
        if (!value.empty())
        {
          script.name = std::string(value);
        }
      }
      else if (key == "match")
      {
        if (auto converted = convert_match_pattern(value))
        {
          append_unique(script.allow_list, std::move(*converted));
        } else if (!value.empty()) {
          script.warnings.push_back(
            "Unsupported @match pattern: " + std::string(value)
          );
        }
      }
      else if (key == "include")
      {
        if (is_regex_include(trim_view(value)))
        {
          script.warnings.push_back(
            "Unsupported @include regex pattern: " + std::string(value)
          );
        }
        else if (auto converted = convert_include_pattern(value))
        {
          append_unique(script.allow_list, std::move(*converted));
        } else if (!value.empty()) {
          script.warnings.push_back(
            "Unsupported @include pattern: " + std::string(value)
          );
        }
      }
      else if (key == "exclude")
      {
        if (is_regex_include(trim_view(value)))
        {
          script.warnings.push_back(
            "Unsupported @exclude regex pattern: " + std::string(value)
          );
        }
        else if (auto converted = convert_include_pattern(value))
        {
          append_unique(script.block_list, std::move(*converted));
        } else if (!value.empty()) {
          script.warnings.push_back(
            "Unsupported @exclude pattern: " + std::string(value)
          );
        }
      }
      else if (key == "run-at")
      {
        if (equals_ignore_case(value, "document-start"))
        {
          script.run_at = RunAt::DocumentStart;
        }
        else if (
          equals_ignore_case(value, "document-end") ||
          equals_ignore_case(value, "document-idle")
        )
        {
          script.run_at = RunAt::DocumentEnd;
        } else if (!value.empty()) {
          script.warnings.push_back(
            "Unsupported @run-at value: " + std::string(value)
          );
        }
      }
      else if (key == "noframes")
      {
        script.noframes = true;
      }
    }

    if (script.name.empty())
    {
      script.name = "unnamed";
    }
    if (script.allow_list.empty())
    {
      script.warnings.push_back(
        "No usable @match/@include patterns; script will not be installed."
      );
    }

    return script;
  }
}
