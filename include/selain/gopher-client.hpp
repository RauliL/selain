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

namespace selain
{
  /**
   * Parsed components of a gopher:// or gophers:// URI (RFC 4266).
   */
  struct GopherRequest
  {
    /** Absolute request URI. */
    std::string uri;
    /** "gopher" or "gophers". */
    std::string scheme;
    std::string host;
    int port = 70;
    /** Single-character Gopher item type (defaults to '1'). */
    char type = '1';
    /** Opaque selector string sent to the server. */
    std::string selector;
    /** Optional search text for type '7' (after %09 in the URI path). */
    std::string search;
    /** True when the URI scheme is gophers://. */
    bool use_tls = false;
  };

  struct GopherResponse
  {
    /** Echo of the request URI. */
    std::string uri;
    /** Item type from the request URI. */
    char type = '1';
    /** Response body bytes. */
    std::vector<std::uint8_t> body;
    /** Human-readable transport or protocol error, empty on success. */
    std::string error;
  };

  using GopherFetchCallback = std::function<void(GopherResponse)>;

  /**
   * Parses @p uri into Gopher request components.
   *
   * Returns false when the URI is not a usable gopher(s) URL with a host.
   */
  bool gopher_parse_uri(const std::string& uri, GopherRequest& out);

  /**
   * Builds a gopher(s) URI from the given components.
   */
  std::string gopher_build_uri(
    const std::string& scheme,
    const std::string& host,
    int port,
    char type,
    const std::string& selector,
    const std::string& search = {}
  );

  /**
   * Performs an asynchronous Gopher request for @p uri.
   *
   * For gophers://, TLS is used and self-signed certificates are accepted for
   * now (same policy as Gemini).
   */
  void gopher_fetch(const std::string& uri, GopherFetchCallback callback);
}
