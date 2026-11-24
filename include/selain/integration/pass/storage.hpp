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

#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

#include <glibmm.h>

namespace selain::integration::pass
{
  struct Secret
  {
    Glib::ustring password;
    Glib::ustring username;
  };

  using show_callback = std::function<void(
    std::optional<Secret> secret,
    Glib::ustring error
  )>;

  /**
   * Resolves the password store directory from `$PASSWORD_STORE_DIR`, or
   * `~/.password-store` when unset.
   */
  std::filesystem::path store_directory();

  /**
   * Lists store-relative entry paths (without `.gpg`) that match @p host,
   * trying the full host then progressively stripped subdomains. Matching is
   * case-insensitive and does not decrypt any secrets.
   */
  std::vector<Glib::ustring> find_entries(const Glib::ustring& host);

  /**
   * Username derived from the last path segment of a store entry.
   */
  Glib::ustring username_from_entry(const Glib::ustring& entry);

  /**
   * Runs `pass show <entry>` asynchronously and invokes @p callback with the
   * parsed secret (password = first line; username from the entry path).
   */
  void show_async(const Glib::ustring& entry, show_callback callback);
}
