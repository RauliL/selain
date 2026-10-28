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
  struct GeminiResponse
  {
    /** Final URI after following redirects. */
    std::string uri;
    /** Two-digit Gemini status code, or 0 on transport failure. */
    int status = 0;
    /** Status meta line (MIME type, redirect target, prompt, or error text). */
    std::string meta;
    /** Response body for success (2x) responses. */
    std::vector<std::uint8_t> body;
    /** Human-readable transport or protocol error, empty on success. */
    std::string error;
  };

  using GeminiFetchCallback = std::function<void(GeminiResponse)>;

  /**
   * Performs an asynchronous Gemini request for @p uri.
   *
   * Self-signed certificates are accepted for now (TOFU can be added later).
   * Redirects (3x) are followed up to a small limit.
   */
  void gemini_fetch(const std::string& uri, GeminiFetchCallback callback);
}
