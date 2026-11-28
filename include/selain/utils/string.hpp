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
#pragma once

#include <cctype>
#include <concepts>
#include <cstring>

#include <glibmm.h>
#include <webkit/webkit.h>

namespace selain::utils::string
{
  template<typename T>
  concept TrimableString = requires(const T& s, std::size_t i, std::size_t n)
  {
    { s.length() } -> std::convertible_to<std::size_t>;
    { s[i] };
    { s.substr(i, n) } -> std::convertible_to<T>;
  };

  template<typename T>
  bool starts_with(const T& input, const T& prefix)
  {
    return (
      input.length() >= prefix.length() &&
      !input.compare(0, prefix.length(), prefix)
    );
  }

  template<typename T>
  inline bool starts_with(const T& input, typename T::const_pointer prefix)
  {
    const auto prefix_length = std::strlen(prefix);

    return (
      input.length() >= prefix_length &&
      !input.compare(0, prefix_length, prefix)
    );
  }

  template<typename T>
  bool starts_with(const T& input, typename T::value_type c)
  {
    return input.length() > 0 && input[0] == c;
  }

  template<TrimableString T>
  std::size_t find_first_non_whitespace(const T& input)
  {
    const auto length = input.length();
    std::size_t i;

    for (i = 0; i < length; ++i)
    {
      if (!std::isspace(input[i]))
      {
        break;
      }
    }

    return i;
  }

  template<TrimableString T>
  std::size_t find_last_non_whitespace(const T& input)
  {
    const auto length = input.length();
    std::size_t i;

    for (i = length; i != 0; --i)
    {
      if (!std::isspace(input[i - 1]))
      {
        break;
      }
    }

    return i;
  }

  /**
    * Strips whitespace from beginning and of end of given string and returns
    * result.
    */
  template<TrimableString T>
  T trim(const T& input)
  {
    const auto i = find_first_non_whitespace(input);
    const auto j = find_last_non_whitespace(input);

    if (i == 0 && j == input.length())
    {
      return input;
    }

    return input.substr(i, j - i);
  }

  /**
    * Strips whitespace from beginning of given string and returns result.
    */
  template<TrimableString T>
  T trim_start(const T& input)
  {
    const auto i = find_first_non_whitespace(input);

    if (i != 0)
    {
      return input.substr(i);
    }

    return input;
  }

  /**
   * Tests whether given string is empty or contains only whitespace
   * characters.
   */
  template<TrimableString T>
  bool is_blank(const T& input)
  {
    const auto length = input.length();

    if (length == 0)
    {
      return true;
    }
    for (std::size_t i = 0; i < length; ++i)
    {
      if (!std::isspace(input[i]))
      {
        return false;
      }
    }

    return true;
  }

  template<TrimableString T>
  bool has_whitespace(const T& input)
  {
    const auto length = input.length();

    for (std::size_t i = 0; i < length; ++i)
    {
      if (std::isspace(input[i]))
      {
        return true;
      }
    }

    return false;
  }

  /**
    * Truncates @p input to at most @p max_length characters, appending an
    * ellipsis (…) when truncation occurs. @p max_length includes the ellipsis
    * character. Returns @p input unchanged when it already fits, or an empty
    * string when @p max_length is 0.
    */
  Glib::ustring ellipsize(const Glib::ustring& input, std::size_t max_length);

  /**
    * Escapes HTML entities from given string.
    */
  std::string html_escape(std::string_view text);

  /**
    * Escapes JavaScript string literal escape sequences from given string.
    */
  std::string js_string_escape(std::string_view text);

  /**
    * Returns textual representation of given JavaScript value.
    */
  Glib::ustring js_value_to_text(JSCValue* value);

  /**
    * Fuzzy-matches @p needle against @p haystack (case-insensitive subsequence).
    * Returns a score (higher is better) when all needle characters appear in
    * order, or std::nullopt when there is no match. An empty needle matches
    * with score 0.
    */
  std::optional<int> fuzzy_match_score(
    const Glib::ustring& haystack,
    const Glib::ustring& needle
  );
}
