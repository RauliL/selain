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

#include <glibmm.h>

namespace selain::utils::url
{
  /**
   * Returns the URI one path level above @p uri, with query and fragment
   * removed. Returns `std::nullopt` when the URI is empty, invalid, or already
   * at the hierarchy root.
   */
  std::optional<Glib::ustring> get_parent(const Glib::ustring& uri);

  /**
   * Returns the root of the URL hierarchy for @p uri (scheme, authority, and
   * `/`), with query and fragment removed. Returns `std::nullopt` when the URI
   * is empty, invalid, or already at the root.
   */
  std::optional<Glib::ustring> get_root(const Glib::ustring& uri);

  /**
   * Resolves user-entered navigation text to a loadable URI.
   *
   * - Absolute URIs (with a scheme) are returned unchanged.
   * - Domain-like hosts (e.g. `example.com`, `localhost`) get an `https://`
   *   prefix.
   * - Anything else becomes a search URL: `%s` in
   *   @p search_engine_template is replaced with the URL-encoded query.
   */
  std::optional<Glib::ustring> resolve_navigation_uri(
    const Glib::ustring& input,
    const Glib::ustring& search_engine_template
  );
}
