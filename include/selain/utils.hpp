/*
 * Copyright (c) 2019, Rauli Laine
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

#include <algorithm>
#include <cctype>
#include <concepts>

#include <glibmm.h>
#include <webkit/webkit.h>

namespace selain::utils
{
  template<typename T>
  concept TrimableString = requires(const T& s, std::size_t i, std::size_t n)
  {
    { s.length() } -> std::convertible_to<std::size_t>;
    { s[i] };
    { s.substr(i, n) } -> std::convertible_to<T>;
  };

  /**
   * Tests whether given string is empty or contains only whitespace
   * characters.
   */
  template<typename T>
  bool is_blank(const T& input)
  {
    return input.empty() || std::all_of(
      std::begin(input),
      std::end(input),
      [](T::const_reference c) -> bool
      {
        return std::isspace(static_cast<int>(c));
      }
    );
  }

  template<typename T>
  inline bool starts_with(const T& input, typename T::value_type c)
  {
    return !input.empty() && input[0] == c;
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

    if (i != 0 && j != input.length())
    {
      return input.substr(i, j - i);
    }

    return input;
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
   * Returns textual representation of given JavaScript value.
   */
  Glib::ustring js_value_to_text(::JSCValue* value);
}
