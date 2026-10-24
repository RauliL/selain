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

#include <algorithm>
#include <format>

namespace selain
{
  DownloadList::DownloadList()
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , m_title("Downloads")
    , m_rows(Gtk::Orientation::VERTICAL, 6)
    , m_clear_all_button("Clear finished")
  {
    add_css_class("selain-download-list");
    set_size_request(280, -1);
    set_visible(false);

    m_title.add_css_class("download-list-title");
    m_title.set_halign(Gtk::Align::START);
    m_title.set_margin(8);

    m_rows.set_margin_start(8);
    m_rows.set_margin_end(8);
    m_rows.set_margin_bottom(8);
    m_rows.set_valign(Gtk::Align::START);

    m_scroll.set_child(m_rows);
    m_scroll.set_policy(
      Gtk::PolicyType::NEVER,
      Gtk::PolicyType::AUTOMATIC
    );
    m_scroll.set_vexpand(true);
    m_scroll.set_hexpand(false);

    m_clear_all_button.set_margin(8);
    m_clear_all_button.signal_clicked().connect([this]() {
      if (m_manager)
      {
        m_manager->clear_finished();
      }
    });

    append(m_title);
    append(m_scroll);
    append(m_clear_all_button);
  }

  void
  DownloadList::set_manager(DownloadManager& manager)
  {
    m_manager = &manager;
  }

  void
  DownloadList::upsert(const DownloadInfo& info)
  {
    auto it = m_row_by_id.find(info.id);

    if (it == m_row_by_id.end())
    {
      if (!m_manager)
      {
        return;
      }

      auto* row = Gtk::make_managed<Row>(info.id, *m_manager);

      row->update(info);
      m_rows.prepend(*row);
      m_row_by_id.emplace(info.id, row);
    }
    else
    {
      it->second->update(info);
    }
  }

  void
  DownloadList::remove(std::uint64_t id)
  {
    const auto it = m_row_by_id.find(id);

    if (it == m_row_by_id.end())
    {
      return;
    }

    m_rows.remove(*it->second);
    m_row_by_id.erase(it);
  }

  void
  DownloadList::show_panel()
  {
    set_visible(true);
  }

  void
  DownloadList::hide_panel()
  {
    set_visible(false);
  }

  void
  DownloadList::toggle_panel()
  {
    set_visible(!get_visible());
  }

  bool
  DownloadList::is_panel_visible() const
  {
    return get_visible();
  }

  DownloadList::Row::Row(std::uint64_t id, DownloadManager& manager)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 4)
    , m_id(id)
    , m_manager(&manager)
    , m_cancel_button("Cancel")
    , m_folder_button("Folder")
    , m_clear_button("Clear")
  {
    add_css_class("download-row");

    m_name_label.set_halign(Gtk::Align::START);
    m_name_label.set_ellipsize(Pango::EllipsizeMode::END);
    m_name_label.add_css_class("download-name");

    m_status_label.set_halign(Gtk::Align::START);
    m_status_label.set_ellipsize(Pango::EllipsizeMode::END);
    m_status_label.add_css_class("download-status");

    m_progress.set_show_text(false);
    m_progress.set_fraction(0.0);

    auto actions = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 4);

    m_cancel_button.signal_clicked().connect([this]() {
      if (m_manager)
      {
        m_manager->cancel(m_id);
      }
    });
    m_folder_button.signal_clicked().connect([this]() {
      if (m_manager)
      {
        m_manager->open_containing_folder(m_id);
      }
    });
    m_clear_button.signal_clicked().connect([this]() {
      if (m_manager)
      {
        m_manager->remove_entry(m_id);
      }
    });

    actions->append(m_cancel_button);
    actions->append(m_folder_button);
    actions->append(m_clear_button);

    append(m_name_label);
    append(m_status_label);
    append(m_progress);
    append(*actions);
  }

  void
  DownloadList::Row::update(const DownloadInfo& info)
  {
    m_name_label.set_text(
      info.filename.empty() ? info.uri : info.filename
    );
    m_status_label.set_text(format_status(info));

    const bool active = info.state == DownloadState::PENDING ||
                        info.state == DownloadState::ACTIVE;
    const bool done = info.state == DownloadState::FINISHED;
    const bool failed = info.state == DownloadState::FAILED ||
                        info.state == DownloadState::CANCELLED;

    m_progress.set_visible(active || done);
    if (info.progress > 0.0)
    {
      m_progress.set_fraction(std::clamp(info.progress, 0.0, 1.0));
    }
    else if (done)
    {
      m_progress.set_fraction(1.0);
    }
    else
    {
      m_progress.pulse();
    }

    m_cancel_button.set_visible(active);
    m_folder_button.set_visible(done && !info.destination.empty());
    m_clear_button.set_visible(done || failed);
  }

  Glib::ustring
  DownloadList::format_status(const DownloadInfo& info)
  {
    switch (info.state)
    {
      case DownloadState::PENDING:
        return "Waiting for save location…";

      case DownloadState::ACTIVE:
        if (info.progress > 0.0)
        {
          return std::format(
            "{:.0f}% · {}",
            info.progress * 100.0,
            format_bytes(info.received_bytes).raw()
          );
        }
        return format_bytes(info.received_bytes);

      case DownloadState::FINISHED:
        return "Finished · " + format_bytes(info.received_bytes);

      case DownloadState::FAILED:
        return info.error_message.empty()
          ? Glib::ustring("Failed")
          : Glib::ustring("Failed: ") + info.error_message;

      case DownloadState::CANCELLED:
        return "Cancelled";
    }

    return {};
  }

  Glib::ustring
  DownloadList::format_bytes(std::uint64_t bytes)
  {
    constexpr double kib = 1024.0;
    constexpr double mib = kib * 1024.0;
    constexpr double gib = mib * 1024.0;

    if (bytes >= static_cast<std::uint64_t>(gib))
    {
      return std::format("{:.1f} GiB", bytes / gib);
    }
    if (bytes >= static_cast<std::uint64_t>(mib))
    {
      return std::format("{:.1f} MiB", bytes / mib);
    }
    if (bytes >= static_cast<std::uint64_t>(kib))
    {
      return std::format("{:.1f} KiB", bytes / kib);
    }

    return std::format("{} B", bytes);
  }
}
