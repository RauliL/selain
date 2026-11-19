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

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace selain::protocol::finger
{
  /**
   * Parsed components of a finger:// URI.
   *
   * The wire query is taken from the path (without the leading slash), or from
   * the URI userinfo when the path is empty — matching historical finger URL
   * forms such as finger://user@host/ and finger://host/user.
   */
  struct Request
  {
    /** Absolute request URI. */
    std::string uri;
    std::string host;
    int port = 79;
    /** Query string sent to the finger server (may be empty). */
    std::string query;
  };

  struct Response
  {
    /** Echo of the request URI. */
    std::string uri;
    /** Response body bytes. */
    std::vector<std::uint8_t> body;
    /** Human-readable transport or protocol error, empty on success. */
    std::string error;
  };

  using FetchCallback = std::function<void(Response)>;

  /**
   * Parses @p uri into Finger request components.
   *
   * Returns false when the URI is not a usable finger:// URL with a host.
   */
  bool parse_uri(const std::string& uri, Request& out);

  /**
   * Builds a finger:// URI from the given components.
   */
  std::string build_uri(
    const std::string& host,
    int port,
    const std::string& query = {}
  );

  /**
   * Performs an asynchronous Finger request for @p uri (RFC 1288).
   */
  void fetch(const std::string& uri, FetchCallback callback);
}
