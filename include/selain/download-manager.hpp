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
#include <vector>

#include <gtkmm.h>
#include <webkit/webkit.h>

namespace selain
{
  class MainWindow;
  class DownloadList;

  enum class DownloadState
  {
    PENDING,
    ACTIVE,
    FINISHED,
    FAILED,
    CANCELLED,
  };

  struct DownloadInfo
  {
    std::uint64_t id = 0;
    Glib::ustring filename;
    Glib::ustring uri;
    Glib::ustring destination;
    Glib::ustring error_message;
    DownloadState state = DownloadState::PENDING;
    double progress = 0.0;
    std::uint64_t received_bytes = 0;
  };

  /**
   * Tracks WebKit downloads for a main window: Save As dialog, progress, and
   * cancel.
   */
  class DownloadManager
  {
  public:
    DownloadManager();
    ~DownloadManager();

    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;

    void attach(
      MainWindow& window,
      DownloadList& list,
      ::WebKitNetworkSession* session
    );

    void cancel(std::uint64_t id);
    void clear_finished();
    void remove_entry(std::uint64_t id);
    void open_containing_folder(std::uint64_t id);

  private:
    struct Entry
    {
      DownloadInfo info;
      ::WebKitDownload* download = nullptr;
      gulong decide_destination_id = 0;
      gulong received_data_id = 0;
      gulong failed_id = 0;
      gulong finished_id = 0;
      bool choosing_destination = false;
    };

    void on_download_started(::WebKitDownload* download);
    bool on_decide_destination(
      ::WebKitDownload* download,
      const char* suggested_filename
    );
    void prompt_destination(Entry& entry, const Glib::ustring& suggested);
    void on_save_finished(
      std::uint64_t id,
      const Glib::RefPtr<Gio::AsyncResult>& result,
      const Glib::RefPtr<Gtk::FileDialog>& dialog
    );
    void on_received_data(::WebKitDownload* download);
    void on_failed(::WebKitDownload* download, const ::GError* error);
    void on_finished(::WebKitDownload* download);

    Entry* find_by_download(::WebKitDownload* download);
    Entry* find_by_id(std::uint64_t id);
    void disconnect_entry(Entry& entry);
    void notify_list(const Entry& entry);
    void release_download(Entry& entry);

    static void on_download_started_cb(
      ::WebKitNetworkSession*,
      ::WebKitDownload* download,
      gpointer user_data
    );
    static ::gboolean on_decide_destination_cb(
      ::WebKitDownload* download,
      ::gchar* suggested_filename,
      gpointer user_data
    );
    static void on_received_data_cb(
      ::WebKitDownload* download,
      ::guint64,
      gpointer user_data
    );
    static void on_failed_cb(
      ::WebKitDownload* download,
      ::GError* error,
      gpointer user_data
    );
    static void on_finished_cb(
      ::WebKitDownload* download,
      gpointer user_data
    );

  private:
    MainWindow* m_window = nullptr;
    DownloadList* m_list = nullptr;
    ::WebKitNetworkSession* m_session = nullptr;
    gulong m_download_started_id = 0;
    std::uint64_t m_next_id = 1;
    std::vector<Entry> m_entries;
  };
}
