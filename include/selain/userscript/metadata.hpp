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
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace selain::userscript
{
  enum class RunAt
  {
    DocumentStart,
    DocumentEnd,
  };

  /**
   * Parsed GreaseMonkey-style userscript after metadata processing.
   * Patterns in allow_list / block_list are WebKit URL patterns.
   */
  struct ParsedScript
  {
    std::string name;
    std::vector<std::string> allow_list;
    std::vector<std::string> block_list;
    RunAt run_at = RunAt::DocumentEnd;
    bool noframes = false;
    /** Script source with the ==UserScript== block removed. */
    std::string source;
    std::vector<std::string> warnings;

    /**
     * True when the script has at least one usable @match/@include pattern.
     */
    bool is_installable() const
    {
      return !allow_list.empty();
    }
  };

  /**
   * Converts a GreaseMonkey @match pattern to a WebKit allow/block pattern.
   * Returns nullopt for unsupported patterns.
   */
  std::optional<std::string> convert_match_pattern(std::string_view pattern);

  /**
   * Converts a GreaseMonkey @include/@exclude glob to a WebKit pattern.
   * Returns nullopt for regex includes and other unsupported forms.
   */
  std::optional<std::string> convert_include_pattern(std::string_view pattern);

  /**
   * Parses GreaseMonkey metadata from @p source. Returns nullopt when no
   * ==UserScript== block is present.
   */
  std::optional<ParsedScript> parse(std::string_view source);
}
