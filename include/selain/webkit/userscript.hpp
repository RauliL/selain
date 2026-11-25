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

#include <webkit/webkit.h>

namespace selain::webkit::userscript
{
  /**
   * Directory under the user config dir that holds userscripts
   * (`~/.config/selain/scripts` by default).
   */
  std::filesystem::path scripts_directory();

  /**
   * Loads installable GreaseMonkey-style scripts from the scripts directory
   * and adds them to @p manager when userscripts are enabled (the default).
   */
  void initialize(::WebKitUserContentManager* manager);

  /**
   * Removes previously installed userscripts and reloads from disk.
   */
  void reload(::WebKitUserContentManager* manager);

  /**
   * Releases installed userscripts. Safe if initialize was never called.
   */
  void shutdown();

  /**
   * Whether userscripts are applied. Defaults to true.
   */
  bool is_enabled();

  /**
   * Enables or disables userscript injection on the manager passed to
   * initialize. Parsed scripts are kept so toggling back on does not re-read
   * files from disk.
   */
  void set_enabled(bool enabled);
}
