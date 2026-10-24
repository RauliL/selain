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

#include <unordered_map>

#include <gtkmm.h>

#include <selain/download-manager.hpp>

namespace selain
{
  class DownloadManager;

  /**
   * Side panel listing downloads with progress and actions.
   */
  class DownloadList : public Gtk::Box
  {
  public:
    explicit DownloadList();

    void set_manager(DownloadManager& manager);

    void upsert(const DownloadInfo& info);
    void remove(std::uint64_t id);

    void show_panel();
    void hide_panel();
    void toggle_panel();
    bool is_panel_visible() const;

  private:
    class Row : public Gtk::Box
    {
    public:
      explicit Row(std::uint64_t id, DownloadManager& manager);

      void update(const DownloadInfo& info);

    private:
      std::uint64_t m_id;
      DownloadManager* m_manager;
      Gtk::Label m_name_label;
      Gtk::Label m_status_label;
      Gtk::ProgressBar m_progress;
      Gtk::Button m_cancel_button;
      Gtk::Button m_folder_button;
      Gtk::Button m_clear_button;
    };

    static Glib::ustring format_status(const DownloadInfo& info);
    static Glib::ustring format_bytes(std::uint64_t bytes);

  private:
    DownloadManager* m_manager = nullptr;
    Gtk::Label m_title;
    Gtk::ScrolledWindow m_scroll;
    Gtk::Box m_rows;
    Gtk::Button m_clear_all_button;
    std::unordered_map<std::uint64_t, Row*> m_row_by_id;
  };
}
