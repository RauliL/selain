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
#include <selain/download-list.hpp>
#include <selain/download-manager.hpp>
#include <selain/main-window.hpp>

namespace selain
{
  DownloadManager::DownloadManager() = default;

  DownloadManager::~DownloadManager()
  {
    if (m_session && m_download_started_id)
    {
      ::g_signal_handler_disconnect(m_session, m_download_started_id);
      m_download_started_id = 0;
    }

    for (auto& entry : m_entries)
    {
      if (entry.download &&
          (entry.info.state == DownloadState::PENDING ||
           entry.info.state == DownloadState::ACTIVE))
      {
        ::webkit_download_cancel(entry.download);
      }
      disconnect_entry(entry);
      release_download(entry);
    }
    m_entries.clear();
  }

  void
  DownloadManager::attach(MainWindow& window,
                          DownloadList& list,
                          ::WebKitNetworkSession* session)
  {
    m_window = &window;
    m_list = &list;
    m_session = session;
    m_list->set_manager(*this);

    if (m_session)
    {
      m_download_started_id = ::g_signal_connect(
        m_session,
        "download-started",
        G_CALLBACK(on_download_started_cb),
        this
      );
    }
  }

  void
  DownloadManager::cancel(std::uint64_t id)
  {
    if (auto* entry = find_by_id(id))
    {
      if (entry->download &&
          (entry->info.state == DownloadState::PENDING ||
           entry->info.state == DownloadState::ACTIVE))
      {
        ::webkit_download_cancel(entry->download);
      }
    }
  }

  void
  DownloadManager::clear_finished()
  {
    for (auto it = m_entries.begin(); it != m_entries.end();)
    {
      if (it->info.state == DownloadState::FINISHED ||
          it->info.state == DownloadState::FAILED ||
          it->info.state == DownloadState::CANCELLED)
      {
        if (m_list)
        {
          m_list->remove(it->info.id);
        }
        disconnect_entry(*it);
        release_download(*it);
        it = m_entries.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  void
  DownloadManager::remove_entry(std::uint64_t id)
  {
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
      if (it->info.id != id)
      {
        continue;
      }
      if (it->info.state == DownloadState::PENDING ||
          it->info.state == DownloadState::ACTIVE)
      {
        return;
      }
      if (m_list)
      {
        m_list->remove(it->info.id);
      }
      disconnect_entry(*it);
      release_download(*it);
      m_entries.erase(it);
      return;
    }
  }

  void
  DownloadManager::open_containing_folder(std::uint64_t id)
  {
    const auto* entry = find_by_id(id);

    if (!entry || entry->info.destination.empty() || !m_window)
    {
      return;
    }

    const auto file = Gio::File::create_for_path(entry->info.destination.raw());
    const auto launcher = Gtk::FileLauncher::create(file);

    launcher->open_containing_folder(
      *m_window,
      [launcher](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try
        {
          launcher->open_containing_folder_finish(result);
        }
        catch (const Glib::Error&)
        {
        }
      }
    );
  }

  void
  DownloadManager::on_download_started(::WebKitDownload* download)
  {
    g_object_ref(download);

    Entry entry;

    entry.info.id = m_next_id++;
    entry.info.state = DownloadState::PENDING;
    entry.download = download;

    if (auto* request = ::webkit_download_get_request(download))
    {
      if (const auto* uri = ::webkit_uri_request_get_uri(request))
      {
        entry.info.uri = uri;
      }
    }

    entry.decide_destination_id = ::g_signal_connect(
      download,
      "decide-destination",
      G_CALLBACK(on_decide_destination_cb),
      this
    );
    entry.received_data_id = ::g_signal_connect(
      download,
      "received-data",
      G_CALLBACK(on_received_data_cb),
      this
    );
    entry.failed_id = ::g_signal_connect(
      download,
      "failed",
      G_CALLBACK(on_failed_cb),
      this
    );
    entry.finished_id = ::g_signal_connect(
      download,
      "finished",
      G_CALLBACK(on_finished_cb),
      this
    );

    m_entries.push_back(std::move(entry));
    notify_list(m_entries.back());

    if (m_list)
    {
      m_list->show_panel();
    }
  }

  bool
  DownloadManager::on_decide_destination(::WebKitDownload* download,
                                         const char* suggested_filename)
  {
    auto* entry = find_by_download(download);

    if (!entry)
    {
      return false;
    }

    Glib::ustring suggested = suggested_filename ? suggested_filename : "download";

    entry->info.filename = suggested;
    entry->choosing_destination = true;
    notify_list(*entry);
    prompt_destination(*entry, suggested);

    return true;
  }

  void
  DownloadManager::prompt_destination(Entry& entry,
                                      const Glib::ustring& suggested)
  {
    if (!m_window || !entry.download)
    {
      return;
    }

    const auto dialog = Gtk::FileDialog::create();
    const auto id = entry.info.id;
    const auto downloads_dir = Glib::get_user_special_dir(
      Glib::UserDirectory::DOWNLOAD
    );

    dialog->set_title("Save File");
    dialog->set_modal(true);
    dialog->set_initial_name(suggested.raw());
    if (!downloads_dir.empty())
    {
      dialog->set_initial_folder(Gio::File::create_for_path(downloads_dir));
    }

    dialog->save(
      *m_window,
      [this, id, dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
        on_save_finished(id, result, dialog);
      }
    );
  }

  void
  DownloadManager::on_save_finished(
    std::uint64_t id,
    const Glib::RefPtr<Gio::AsyncResult>& result,
    const Glib::RefPtr<Gtk::FileDialog>& dialog
  )
  {
    auto* entry = find_by_id(id);

    if (!entry || !entry->download)
    {
      return;
    }

    entry->choosing_destination = false;

    try
    {
      const auto file = dialog->save_finish(result);

      if (!file)
      {
        ::webkit_download_cancel(entry->download);
        return;
      }

      const auto path = file->get_path();

      if (path.empty())
      {
        ::webkit_download_cancel(entry->download);
        return;
      }

      entry->info.destination = path;
      entry->info.filename = file->get_basename();
      entry->info.state = DownloadState::ACTIVE;
      ::webkit_download_set_allow_overwrite(entry->download, TRUE);
      ::webkit_download_set_destination(entry->download, path.c_str());
      notify_list(*entry);
    }
    catch (const Gtk::DialogError& error)
    {
      if (error.code() == Gtk::DialogError::DISMISSED ||
          error.code() == Gtk::DialogError::CANCELLED)
      {
        ::webkit_download_cancel(entry->download);
        return;
      }
      if (m_window)
      {
        m_window->show_error(error.what());
      }
      ::webkit_download_cancel(entry->download);
    }
    catch (const Glib::Error& error)
    {
      if (m_window)
      {
        m_window->show_error(error.what());
      }
      ::webkit_download_cancel(entry->download);
    }
  }

  void
  DownloadManager::on_received_data(::WebKitDownload* download)
  {
    auto* entry = find_by_download(download);

    if (!entry)
    {
      return;
    }

    entry->info.state = DownloadState::ACTIVE;
    entry->info.progress = ::webkit_download_get_estimated_progress(download);
    entry->info.received_bytes = ::webkit_download_get_received_data_length(
      download
    );
    notify_list(*entry);
  }

  void
  DownloadManager::on_failed(::WebKitDownload* download, const ::GError* error)
  {
    auto* entry = find_by_download(download);

    if (!entry)
    {
      return;
    }

    const bool cancelled = error &&
      error->domain == WEBKIT_DOWNLOAD_ERROR &&
      error->code == WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER;

    if (cancelled)
    {
      entry->info.state = DownloadState::CANCELLED;
      entry->info.error_message.clear();
    }
    else
    {
      entry->info.state = DownloadState::FAILED;
      entry->info.error_message = error && error->message
        ? error->message
        : "Download failed";
      if (m_window)
      {
        m_window->show_error(
          "Download failed: " + entry->info.filename +
          (entry->info.error_message.empty()
            ? Glib::ustring()
            : Glib::ustring(" (") + entry->info.error_message + ")")
        );
      }
    }
    notify_list(*entry);
  }

  void
  DownloadManager::on_finished(::WebKitDownload* download)
  {
    auto* entry = find_by_download(download);

    if (!entry)
    {
      return;
    }

    entry->info.progress = ::webkit_download_get_estimated_progress(download);
    entry->info.received_bytes = ::webkit_download_get_received_data_length(
      download
    );

    if (entry->info.state == DownloadState::ACTIVE ||
        entry->info.state == DownloadState::PENDING)
    {
      entry->info.state = DownloadState::FINISHED;
      entry->info.progress = 1.0;
      if (m_window)
      {
        m_window->show_info("Downloaded " + entry->info.filename);
      }
    }

    notify_list(*entry);
    disconnect_entry(*entry);
    release_download(*entry);
  }

  DownloadManager::Entry*
  DownloadManager::find_by_download(::WebKitDownload* download)
  {
    for (auto& entry : m_entries)
    {
      if (entry.download == download)
      {
        return &entry;
      }
    }

    return nullptr;
  }

  DownloadManager::Entry*
  DownloadManager::find_by_id(std::uint64_t id)
  {
    for (auto& entry : m_entries)
    {
      if (entry.info.id == id)
      {
        return &entry;
      }
    }

    return nullptr;
  }

  void
  DownloadManager::disconnect_entry(Entry& entry)
  {
    if (!entry.download)
    {
      return;
    }

    if (entry.decide_destination_id)
    {
      ::g_signal_handler_disconnect(entry.download, entry.decide_destination_id);
      entry.decide_destination_id = 0;
    }
    if (entry.received_data_id)
    {
      ::g_signal_handler_disconnect(entry.download, entry.received_data_id);
      entry.received_data_id = 0;
    }
    if (entry.failed_id)
    {
      ::g_signal_handler_disconnect(entry.download, entry.failed_id);
      entry.failed_id = 0;
    }
    if (entry.finished_id)
    {
      ::g_signal_handler_disconnect(entry.download, entry.finished_id);
      entry.finished_id = 0;
    }
  }

  void
  DownloadManager::notify_list(const Entry& entry)
  {
    if (m_list)
    {
      m_list->upsert(entry.info);
    }
  }

  void
  DownloadManager::release_download(Entry& entry)
  {
    if (entry.download)
    {
      g_object_unref(entry.download);
      entry.download = nullptr;
    }
  }

  void
  DownloadManager::on_download_started_cb(::WebKitNetworkSession*,
                                          ::WebKitDownload* download,
                                          gpointer user_data)
  {
    static_cast<DownloadManager*>(user_data)->on_download_started(download);
  }

  ::gboolean
  DownloadManager::on_decide_destination_cb(::WebKitDownload* download,
                                            ::gchar* suggested_filename,
                                            gpointer user_data)
  {
    return static_cast<DownloadManager*>(user_data)->on_decide_destination(
      download,
      suggested_filename
    ) ? TRUE : FALSE;
  }

  void
  DownloadManager::on_received_data_cb(::WebKitDownload* download,
                                       ::guint64,
                                       gpointer user_data)
  {
    static_cast<DownloadManager*>(user_data)->on_received_data(download);
  }

  void
  DownloadManager::on_failed_cb(::WebKitDownload* download,
                                ::GError* error,
                                gpointer user_data)
  {
    static_cast<DownloadManager*>(user_data)->on_failed(download, error);
  }

  void
  DownloadManager::on_finished_cb(::WebKitDownload* download,
                                  gpointer user_data)
  {
    static_cast<DownloadManager*>(user_data)->on_finished(download);
  }
}
