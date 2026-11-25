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
#include <algorithm>
#include <chrono>

#include <glibmm.h>
#include <sqlite3.h>

#include <selain/core/history-manager.hpp>

namespace selain
{
  namespace
  {
    std::filesystem::path
    default_database_path()
    {
      return std::filesystem::path(
        Glib::build_filename(
          Glib::get_user_data_dir(),
          "selain",
          "history.sqlite"
        )
      );
    }

    gint64
    now_ms()
    {
      using clock = std::chrono::system_clock;

      return std::chrono::duration_cast<std::chrono::milliseconds>(
        clock::now().time_since_epoch()
      ).count();
    }

    Glib::ustring
    escape_like(const Glib::ustring& value)
    {
      Glib::ustring escaped;

      escaped.reserve(value.size());
      for (const auto ch : value)
      {
        if (ch == '%' || ch == '_' || ch == '\\')
        {
          escaped.push_back('\\');
        }
        escaped.push_back(ch);
      }

      return escaped;
    }

    bool
    exec_sql(sqlite3* db, const char* sql, Glib::ustring* error = nullptr)
    {
      char* errmsg = nullptr;
      const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);

      if (rc != SQLITE_OK)
      {
        if (error)
        {
          *error = errmsg ? errmsg : "SQLite error.";
        }
        sqlite3_free(errmsg);

        return false;
      }

      return true;
    }

    std::vector<HistoryManager::Entry>
    read_entries(sqlite3_stmt* stmt)
    {
      std::vector<HistoryManager::Entry> entries;

      while (sqlite3_step(stmt) == SQLITE_ROW)
      {
        HistoryManager::Entry entry;
        const auto* uri = reinterpret_cast<const char*>(
          sqlite3_column_text(stmt, 0)
        );
        const auto* title = reinterpret_cast<const char*>(
          sqlite3_column_text(stmt, 1)
        );

        entry.uri = uri ? uri : "";
        entry.title = title ? title : "";
        entry.visit_time = sqlite3_column_int64(stmt, 2);
        entry.visit_count = sqlite3_column_int(stmt, 3);
        entries.push_back(std::move(entry));
      }

      return entries;
    }
  }

  Glib::RefPtr<HistoryManager>
  HistoryManager::create(const std::filesystem::path& path)
  {
    return Glib::make_refptr_for_instance<HistoryManager>(
      new HistoryManager(path.empty() ? default_database_path() : path)
    );
  }

  HistoryManager::HistoryManager(const std::filesystem::path& path)
    : Glib::ObjectBase(typeid(HistoryManager))
  {
    open_database(path);
  }

  HistoryManager::~HistoryManager()
  {
    close_database();
  }

  bool
  HistoryManager::is_enabled() const
  {
    return m_enabled;
  }

  void
  HistoryManager::set_enabled(bool enabled)
  {
    m_enabled = enabled;
  }

  bool
  HistoryManager::should_record_uri(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return false;
    }

    static const char* const blocked_prefixes[] = {
      "about:",
      "data:",
      "blob:",
      "javascript:",
      "webkit:",
    };
    const auto lowered = uri.lowercase();

    for (const auto* prefix : blocked_prefixes)
    {
      if (lowered.find(prefix) == 0)
      {
        return false;
      }
    }

    return true;
  }

  bool
  HistoryManager::open_database(const std::filesystem::path& path)
  {
    close_database();

    try
    {
      std::filesystem::create_directories(path.parent_path());
    }
    catch (const std::filesystem::filesystem_error& error)
    {
      g_warning(
        "Failed to create history directory: %s",
        error.what()
      );

      return false;
    }

    if (sqlite3_open(path.string().c_str(), &m_db) != SQLITE_OK)
    {
      g_warning(
        "Failed to open history database: %s",
        m_db ? sqlite3_errmsg(m_db) : "unknown error"
      );
      close_database();

      return false;
    }

    Glib::ustring error;

    if (!exec_sql(m_db, "PRAGMA journal_mode=WAL;", &error) ||
        !exec_sql(m_db, "PRAGMA synchronous=NORMAL;", &error) ||
        !exec_sql(
          m_db,
          "CREATE TABLE IF NOT EXISTS visits ("
          "  id INTEGER PRIMARY KEY,"
          "  uri TEXT NOT NULL UNIQUE,"
          "  title TEXT NOT NULL DEFAULT '',"
          "  visit_time INTEGER NOT NULL,"
          "  visit_count INTEGER NOT NULL DEFAULT 1"
          ");",
          &error
        ) ||
        !exec_sql(
          m_db,
          "CREATE INDEX IF NOT EXISTS visits_time_idx"
          " ON visits(visit_time DESC);",
          &error
        ))
    {
      g_warning("Failed to initialize history database: %s", error.c_str());
      close_database();

      return false;
    }

    return true;
  }

  void
  HistoryManager::close_database()
  {
    if (m_db)
    {
      sqlite3_close(m_db);
      m_db = nullptr;
    }
  }

  void
  HistoryManager::record(const Glib::ustring& uri, const Glib::ustring& title)
  {
    if (!m_enabled || !m_db || !should_record_uri(uri))
    {
      return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
      "INSERT INTO visits (uri, title, visit_time, visit_count)"
      " VALUES (?, ?, ?, 1)"
      " ON CONFLICT(uri) DO UPDATE SET"
      "  title = CASE"
      "    WHEN excluded.title != '' THEN excluded.title"
      "    ELSE visits.title"
      "  END,"
      "  visit_time = excluded.visit_time,"
      "  visit_count = visits.visit_count + 1;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
      g_warning("Failed to prepare history insert: %s", sqlite3_errmsg(m_db));

      return;
    }

    sqlite3_bind_text(stmt, 1, uri.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now_ms());

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      g_warning("Failed to record history visit: %s", sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(stmt);
  }

  void
  HistoryManager::update_title(
    const Glib::ustring& uri,
    const Glib::ustring& title
  )
  {
    if (!m_enabled || !m_db || title.empty() || !should_record_uri(uri))
    {
      return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE visits SET title = ? WHERE uri = ?;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
      g_warning(
        "Failed to prepare history title update: %s",
        sqlite3_errmsg(m_db)
      );

      return;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, uri.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      g_warning("Failed to update history title: %s", sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(stmt);
  }

  std::vector<HistoryManager::Entry>
  HistoryManager::query_recent(int limit) const
  {
    if (!m_db)
    {
      return {};
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
      "SELECT uri, title, visit_time, visit_count"
      " FROM visits"
      " ORDER BY visit_time DESC"
      " LIMIT ?;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
      return {};
    }

    sqlite3_bind_int(stmt, 1, std::max(limit, 0));
    auto entries = read_entries(stmt);

    sqlite3_finalize(stmt);

    return entries;
  }

  std::vector<HistoryManager::Entry>
  HistoryManager::query_search(const Glib::ustring& query, int limit) const
  {
    if (!m_db)
    {
      return {};
    }

    const auto pattern = "%" + escape_like(query) + "%";
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
      "SELECT uri, title, visit_time, visit_count"
      " FROM visits"
      " WHERE uri LIKE ? ESCAPE '\\' COLLATE NOCASE"
      "    OR title LIKE ? ESCAPE '\\' COLLATE NOCASE"
      " ORDER BY visit_time DESC"
      " LIMIT ?;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
      return {};
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, std::max(limit, 0));
    auto entries = read_entries(stmt);

    sqlite3_finalize(stmt);

    return entries;
  }

  void
  HistoryManager::list_recent(
    int limit,
    const list_callback_type& callback
  ) const
  {
    if (!callback)
    {
      return;
    }
    if (!m_db)
    {
      callback({}, "History database is not available.");

      return;
    }

    callback(query_recent(limit), {});
  }

  void
  HistoryManager::search(
    const Glib::ustring& query,
    int limit,
    const list_callback_type& callback
  ) const
  {
    if (!callback)
    {
      return;
    }
    if (!m_db)
    {
      callback({}, "History database is not available.");

      return;
    }
    if (query.empty())
    {
      callback(query_recent(limit), {});

      return;
    }

    callback(query_search(query, limit), {});
  }

  std::vector<Glib::ustring>
  HistoryManager::list_uris(int limit) const
  {
    std::vector<Glib::ustring> uris;

    for (const auto& entry : query_recent(limit))
    {
      uris.push_back(entry.uri);
    }

    return uris;
  }

  void
  HistoryManager::clear(const clear_callback_type& callback)
  {
    if (!m_db)
    {
      if (callback)
      {
        callback(false, "History database is not available.");
      }

      return;
    }

    Glib::ustring error;
    const bool ok = exec_sql(m_db, "DELETE FROM visits;", &error);

    if (callback)
    {
      if (ok)
      {
        callback(true, std::nullopt);
      } else {
        callback(false, error.empty() ? Glib::ustring("Failed to clear history.") : error);
      }
    }
  }
}
