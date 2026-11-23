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
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <selain/utils/string.hpp>
#include <selain/utils/url.hpp>

using selain::utils::string::fuzzy_match_score;
using selain::utils::string::html_escape;
using selain::utils::string::is_blank;
using selain::utils::string::js_string_escape;
using selain::utils::string::starts_with;
using selain::utils::string::trim;
using selain::utils::string::trim_start;
using selain::utils::url::get_host;
using selain::utils::url::get_parent;
using selain::utils::url::get_root;
using selain::utils::url::resolve_navigation_uri;

TEST_CASE("is_blank", "[utils::string]")
{
  REQUIRE(is_blank(std::string()));
  REQUIRE(is_blank(std::string(" \t\n")));
  REQUIRE_FALSE(is_blank(std::string("a")));
  REQUIRE_FALSE(is_blank(std::string(" a ")));
}

TEST_CASE("starts_with", "[utils::string]")
{
  REQUIRE(starts_with(std::string("hello"), std::string("he")));
  REQUIRE(starts_with(std::string("hello"), "he"));
  REQUIRE(starts_with(std::string("hello"), 'h'));
  REQUIRE_FALSE(starts_with(std::string("hello"), std::string("ello")));
  REQUIRE_FALSE(starts_with(std::string(""), 'x'));
  REQUIRE(starts_with(std::string("hello"), std::string("")));
}

TEST_CASE("trim and trim_start", "[utils::string]")
{
  REQUIRE(trim(std::string("  hello  ")) == "hello");
  REQUIRE(trim(std::string("hello")) == "hello");
  REQUIRE(trim(std::string("  hello")) == "hello");
  REQUIRE(trim(std::string("hello  ")) == "hello");
  REQUIRE(trim(std::string("   ")) == "");
  REQUIRE(trim(std::string("")) == "");

  REQUIRE(trim_start(std::string("  hello  ")) == "hello  ");
  REQUIRE(trim_start(std::string("hello")) == "hello");
  REQUIRE(trim_start(std::string("   ")) == "");
}

TEST_CASE("html_escape", "[utils::string]")
{
  REQUIRE(html_escape("plain") == "plain");
  REQUIRE(html_escape("a & b < c > \"d\"") == "a &amp; b &lt; c &gt; &quot;d&quot;");
  REQUIRE(html_escape("") == "");
}

TEST_CASE("js_string_escape", "[utils::string]")
{
  REQUIRE(js_string_escape("plain") == "plain");
  REQUIRE(js_string_escape("a\\b'c\"d") == "a\\\\b\\'c\\\"d");
  REQUIRE(js_string_escape("line\nbreak\r") == "line\\nbreak\\r");
  REQUIRE(js_string_escape("<script>") == "\\u003cscript>");
}

TEST_CASE("resolve_navigation_uri", "[utils::url]")
{
  const Glib::ustring engine = "https://example.com/search?q=%s";

  SECTION("empty and blank input")
  {
    REQUIRE(!resolve_navigation_uri("", engine));
    REQUIRE(!resolve_navigation_uri("   ", engine));
  }

  SECTION("keeps absolute URIs")
  {
    REQUIRE(
      resolve_navigation_uri("https://example.com/path", engine)
      == "https://example.com/path"
    );
    REQUIRE(
      resolve_navigation_uri("gemini://host/path", engine)
      == "gemini://host/path"
    );
  }

  SECTION("domain-like hosts get https")
  {
    REQUIRE(
      resolve_navigation_uri("example.com", engine)
      == "https://example.com"
    );
    REQUIRE(
      resolve_navigation_uri("example.com/path?q=1", engine)
      == "https://example.com/path?q=1"
    );
    REQUIRE(
      resolve_navigation_uri("localhost:8080", engine)
      == "https://localhost:8080"
    );
    REQUIRE(
      resolve_navigation_uri("127.0.0.1", engine)
      == "https://127.0.0.1"
    );
  }

  SECTION("bare words become search queries")
  {
    REQUIRE(
      resolve_navigation_uri("hello", engine)
      == "https://example.com/search?q=hello"
    );
    REQUIRE(
      resolve_navigation_uri("hello world", engine)
      == "https://example.com/search?q=hello%20world"
    );
  }

  SECTION("missing %s appends the encoded query")
  {
    REQUIRE(
      resolve_navigation_uri("hello", "https://example.com/search?q=")
      == "https://example.com/search?q=hello"
    );
  }
}

TEST_CASE("get_host", "[utils::url]")
{
  REQUIRE(!get_host(""));
  REQUIRE(!get_host("not a uri"));
  REQUIRE(!get_host("mailto:user@example.com"));
  REQUIRE(get_host("https://example.com/") == "example.com");
  REQUIRE(get_host("https://login.github.com/path?x=1") == "login.github.com");
  REQUIRE(get_host("http://127.0.0.1:8080/") == "127.0.0.1");
}

TEST_CASE("get_parent", "[utils::url]")
{
  REQUIRE(!get_parent(""));
  REQUIRE(!get_parent("not a uri"));
  REQUIRE(!get_parent("https://example.com/"));
  REQUIRE(
    get_parent("https://example.com/a/b/c")
    == "https://example.com/a/b"
  );
  REQUIRE(
    get_parent("https://example.com/a/b/")
    == "https://example.com/a"
  );
  REQUIRE(
    get_parent("https://example.com/a?x=1#frag")
    == "https://example.com/"
  );
}

TEST_CASE("get_root", "[utils::url]")
{
  REQUIRE(!get_root(""));
  REQUIRE(!get_root("https://example.com/"));
  REQUIRE(
    get_root("https://example.com/a/b?x=1#frag")
    == "https://example.com/"
  );
  REQUIRE(
    get_root("https://example.com/?x=1")
    == "https://example.com/"
  );
}

TEST_CASE("fuzzy_match_score", "[utils::string]")
{
  REQUIRE(fuzzy_match_score("anything", "") == 0);
  REQUIRE_FALSE(fuzzy_match_score("abc", "x").has_value());
  REQUIRE(fuzzy_match_score("Hello World", "hw").has_value());
  REQUIRE(fuzzy_match_score("Hello World", "HW").has_value());

  const auto consecutive = fuzzy_match_score("abcdef", "abc");
  const auto scattered = fuzzy_match_score("abcdef", "ace");

  REQUIRE(consecutive.has_value());
  REQUIRE(scattered.has_value());
  REQUIRE(*consecutive > *scattered);
}
