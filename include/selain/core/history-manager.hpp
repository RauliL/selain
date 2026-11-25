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
#include <functional>
#include <optional>
#include <vector>

#include <glibmm.h>

struct sqlite3;

namespace selain
{
  /**
   * Persistent visit history (distinct from WebKit per-view back/forward).
   */
  class HistoryManager : public Glib::Object
  {
  public:
    struct Entry
    {
      Glib::ustring uri;
      Glib::ustring title;
      gint64 visit_time = 0;
      int visit_count = 0;
    };

    using list_callback_type = std::function<void(
      std::vector<Entry> entries,
      Glib::ustring error
    )>;
    using clear_callback_type = std::function<void(
      bool ok,
      const std::optional<Glib::ustring>& error
    )>;

    static constexpr int default_limit = 500;

    /**
     * Opens the history database. When @p path is empty, uses
     * `~/.local/share/selain/history.sqlite`.
     */
    static Glib::RefPtr<HistoryManager> create(
      const std::filesystem::path& path = {}
    );

    ~HistoryManager() override;

    bool is_enabled() const;
    void set_enabled(bool enabled);

    /**
     * Returns true when @p uri should be stored in visit history.
     */
    static bool should_record_uri(const Glib::ustring& uri);

    /**
     * Upserts a visit for @p uri. No-op when recording is disabled or the URI
     * is not recordable. Empty @p title leaves any existing title unchanged.
     */
    void record(const Glib::ustring& uri, const Glib::ustring& title);

    /**
     * Updates the stored title for @p uri without bumping visit count.
     */
    void update_title(const Glib::ustring& uri, const Glib::ustring& title);

    /**
     * Lists the most recent visits, newest first.
     */
    void list_recent(
      int limit,
      const list_callback_type& callback
    ) const;

    /**
     * Searches visits by URI and title (case-insensitive substring match).
     */
    void search(
      const Glib::ustring& query,
      int limit,
      const list_callback_type& callback
    ) const;

    /**
     * Synchronous recent URIs for tab completion (newest first).
     */
    std::vector<Glib::ustring> list_uris(int limit = default_limit) const;

    /**
     * Deletes every visit.
     */
    void clear(const clear_callback_type& callback);

  private:
    explicit HistoryManager(const std::filesystem::path& path);

    bool open_database(const std::filesystem::path& path);
    void close_database();
    std::vector<Entry> query_recent(int limit) const;
    std::vector<Entry> query_search(const Glib::ustring& query, int limit) const;

  private:
    sqlite3* m_db = nullptr;
    bool m_enabled = true;
  };
}
