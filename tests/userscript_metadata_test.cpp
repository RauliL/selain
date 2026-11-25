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

#include <catch2/catch_test_macros.hpp>

#include <selain/userscript/metadata.hpp>

using selain::userscript::RunAt;
using selain::userscript::convert_include_pattern;
using selain::userscript::convert_match_pattern;
using selain::userscript::parse;

TEST_CASE("convert_match_pattern", "[userscript::metadata]")
{
  REQUIRE(convert_match_pattern("https://example.com/*") == "https://example.com/*");
  REQUIRE(convert_match_pattern("*://*.example.com/*") == "*://*.example.com/*");
  REQUIRE(convert_match_pattern("<all_urls>") == "*://*/*");
  REQUIRE(convert_match_pattern("*") == "*://*/*");
  REQUIRE_FALSE(convert_match_pattern("example.com/*").has_value());
  REQUIRE_FALSE(convert_match_pattern("").has_value());
}

TEST_CASE("convert_include_pattern", "[userscript::metadata]")
{
  REQUIRE(convert_include_pattern("https://example.com/*") == "https://example.com/*");
  REQUIRE(convert_include_pattern("*://*.example.com/*") == "*://*.example.com/*");
  REQUIRE(convert_include_pattern("*") == "*://*/*");
  REQUIRE_FALSE(convert_include_pattern("/^https:\\/\\/example\\.com\\//").has_value());
  REQUIRE_FALSE(convert_include_pattern("*.example.com/*").has_value());
}

TEST_CASE("parse full userscript metadata", "[userscript::metadata]")
{
  constexpr auto source = R"(// ==UserScript==
// @name Example Script
// @match https://example.com/*
// @include *://*.example.org/*
// @exclude https://example.com/admin/*
// @run-at document-start
// @noframes
// ==/UserScript==
console.log('hi');
)";

  const auto parsed = parse(source);

  REQUIRE(parsed.has_value());
  REQUIRE(parsed->name == "Example Script");
  REQUIRE(parsed->allow_list.size() == 2);
  REQUIRE(parsed->allow_list[0] == "https://example.com/*");
  REQUIRE(parsed->allow_list[1] == "*://*.example.org/*");
  REQUIRE(parsed->block_list == std::vector<std::string>{"https://example.com/admin/*"});
  REQUIRE(parsed->run_at == RunAt::DocumentStart);
  REQUIRE(parsed->noframes);
  REQUIRE(parsed->is_installable());
  REQUIRE(parsed->source.find("==UserScript==") == std::string::npos);
  REQUIRE(parsed->source.find("console.log('hi');") != std::string::npos);
}

TEST_CASE("parse document-idle as document-end", "[userscript::metadata]")
{
  constexpr auto source = R"(// ==UserScript==
// @name Idle
// @match https://a.test/*
// @run-at document-idle
// ==/UserScript==
)";

  const auto parsed = parse(source);

  REQUIRE(parsed.has_value());
  REQUIRE(parsed->run_at == RunAt::DocumentEnd);
}

TEST_CASE("parse rejects missing include patterns", "[userscript::metadata]")
{
  constexpr auto source = R"(// ==UserScript==
// @name No Patterns
// ==/UserScript==
1;
)";

  const auto parsed = parse(source);

  REQUIRE(parsed.has_value());
  REQUIRE_FALSE(parsed->is_installable());
  REQUIRE_FALSE(parsed->warnings.empty());
}

TEST_CASE("parse warns on regex include", "[userscript::metadata]")
{
  constexpr auto source = R"(// ==UserScript==
// @name Regex
// @include /^https:\/\/example\.com\//
// @match https://ok.test/*
// ==/UserScript==
)";

  const auto parsed = parse(source);

  REQUIRE(parsed.has_value());
  REQUIRE(parsed->is_installable());
  REQUIRE(parsed->allow_list == std::vector<std::string>{"https://ok.test/*"});
  REQUIRE(
    std::any_of(
      parsed->warnings.begin(),
      parsed->warnings.end(),
      [](const std::string& warning) {
        return warning.find("regex") != std::string::npos;
      }
    )
  );
}

TEST_CASE("parse returns nullopt without metadata block", "[userscript::metadata]")
{
  REQUIRE_FALSE(parse("console.log(1);").has_value());
}
