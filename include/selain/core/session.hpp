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

#include <filesystem>
#include <optional>
#include <vector>

#include <selain/ui/tab.hpp>

namespace selain
{
  /**
   * Snapshot of all open tabs in the main window for session restore.
   */
  struct Session
  {
    /** Zero-based notebook page among the saved (non-empty) tabs. */
    int active_tab = 0;
    std::vector<ui::LayoutSnapshot> tabs;
  };

  /**
   * Load and save session JSON under the user data directory.
   */
  class SessionStore
  {
  public:
    /**
     * Default path: `~/.local/share/selain/session.json`.
     */
    static std::filesystem::path default_path();

    /**
     * Writes @p session atomically (temp file + rename). When @p session has
     * no tabs, deletes the file instead. Returns false on I/O failure.
     */
    static bool save(
      const Session& session,
      const std::filesystem::path& path = {}
    );

    /**
     * Loads a session from disk. Returns nullopt when the file is missing,
     * empty, or corrupt.
     */
    static std::optional<Session> load(
      const std::filesystem::path& path = {}
    );

    /**
     * Deletes the session file if present. Returns false on I/O failure.
     */
    static bool clear(const std::filesystem::path& path = {});
  };

  /**
   * Persistent LIFO stack of closed tab layouts for `X` / `:open-closed-tab`.
   */
  class ClosedTabStore
  {
  public:
    /**
     * Default path: `~/.local/share/selain/closed-tabs.json`.
     */
    static std::filesystem::path default_path();

    /**
     * Writes @p tabs atomically. When empty, deletes the file instead.
     */
    static bool save(
      const std::vector<ui::LayoutSnapshot>& tabs,
      const std::filesystem::path& path = {}
    );

    /**
     * Loads closed-tab snapshots (oldest → newest). Returns an empty vector
     * when the file is missing, empty, or corrupt.
     */
    static std::vector<ui::LayoutSnapshot> load(
      const std::filesystem::path& path = {}
    );

    /**
     * Deletes the closed-tabs file if present.
     */
    static bool clear(const std::filesystem::path& path = {});
  };

  namespace session
  {
    bool is_restore_enabled();
    void set_restore_enabled(bool enabled);

    bool is_save_enabled();
    void set_save_enabled(bool enabled);
  }
}
