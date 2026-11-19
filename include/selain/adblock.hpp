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
#include <string>
#include <string_view>

#include <webkit/webkit.h>

namespace selain::adblock
{
  /**
   * Directory under the user config dir that holds blocklist files
   * (`~/.config/selain/blocklists` by default).
   */
  std::filesystem::path blocklists_directory();

  /**
   * Directory used by WebKitUserContentFilterStore for compiled filters.
   */
  std::filesystem::path filter_store_directory();

  /**
   * Loads every regular file from the blocklists directory, compiles each
   * into a WebKit content filter, and adds it to @p manager when ad blocking
   * is enabled (the default).
   *
   * Files may be Safari/WebKit content-blocker JSON, or Adblock Plus /
   * EasyList text (converted automatically). Loading is asynchronous; filters
   * are attached to @p manager as each compilation finishes.
   *
   * Designed so a later automatic EasyList updater can drop files into the
   * blocklists directory (or call load_file) without changing this path.
   */
  void initialize(::WebKitUserContentManager* manager);

  /**
   * Releases filters and store state. Safe to call if initialize was never
   * called.
   */
  void shutdown();

  /**
   * Loads a single blocklist file and adds the compiled filter to @p manager
   * when ad blocking is enabled.
   * @p identifier is the WebKit filter id (typically the file basename).
   */
  void load_file(
    ::WebKitUserContentManager* manager,
    const std::filesystem::path& path,
    std::string_view identifier
  );

  /**
   * Whether content filters are applied. Defaults to true.
   */
  bool is_enabled();

  /**
   * Enables or disables content filters on the manager passed to initialize.
   * Compiled filters are kept so toggling back on does not recompile.
   */
  void set_enabled(bool enabled);
}
