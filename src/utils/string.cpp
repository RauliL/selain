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
#include <selain/utils/string.hpp>

namespace selain::utils::string
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
  js_value_to_text(::JSCValue* value)
  {
    char* text;

    if (jsc_value_is_undefined(value))
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

  std::optional<int>
  fuzzy_match_score(const Glib::ustring& haystack, const Glib::ustring& needle)
  {
    if (needle.empty())
    {
      return 0;
    }

    const auto hay = haystack.casefold();
    const auto nee = needle.casefold();
    int score = 0;
    int consecutive = 0;
    Glib::ustring::size_type hi = 0;

    for (Glib::ustring::size_type ni = 0; ni < nee.size(); ++ni)
    {
      bool found = false;

      for (; hi < hay.size(); ++hi)
      {
        if (hay[hi] != nee[ni])
        {
          consecutive = 0;
          continue;
        }

        if (hi == 0 || !g_unichar_isalnum(hay[hi - 1]))
        {
          score += 10;
        }
        ++consecutive;
        score += 1 + consecutive * 5;
        ++hi;
        found = true;
        break;
      }

      if (!found)
      {
        return std::nullopt;
      }
    }

    score -= static_cast<int>(hay.size());

    return score;
  }
}
