/*
 * Copyright (c) 2019, Rauli Laine
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
#include <selain/command-entry.hpp>
#include <selain/theme.hpp>
#include <selain/utils.hpp>

namespace selain
{
  CommandEntry::CommandEntry()
  {
    const auto& font = utils::get_monospace_font();

    override_background_color(theme::window_background);

    m_entry.override_font(font);
    m_entry.get_style_context()->add_provider(
      theme::get_command_entry_style_provider(),
      1000
    );
    m_entry.signal_activate().connect(sigc::mem_fun(
      this,
      &CommandEntry::on_activate
    ));

    m_info_label.set_halign(Gtk::ALIGN_START);
    m_info_label.override_background_color(theme::window_background);
    m_info_label.override_color(theme::window_foreground);
    m_info_label.override_font(font);

    m_error_label.set_halign(Gtk::ALIGN_START);
    m_error_label.override_background_color(theme::status_bar_error_background);
    m_error_label.override_color(theme::status_bar_error_foreground);
    m_error_label.override_font(font);

    add(m_entry, "entry");
    add(m_info_label, "info-notification");
    add(m_error_label, "error-notification");

    set_visible_child(m_entry);
  }

  void
  CommandEntry::show_notification(const Glib::ustring& text,
                                  NotificationType type,
                                  unsigned int timeout)
  {
    const Notification notification(text, type, timeout);
    std::lock_guard<std::mutex> guard(m_notification_queue_mutex);

    m_notification_queue.push(notification);
    if (m_notification_queue.size() > 1)
    {
      return;
    }
    Glib::signal_timeout().connect_once(
      sigc::mem_fun(*this, &CommandEntry::on_notification_timeout),
      timeout * 1000
    );
    show_notification(notification);
  }

  void
  CommandEntry::show_notification(const Notification& notification)
  {
    const auto& text = std::get<0>(notification);

    switch (std::get<1>(notification))
    {
      case NotificationType::INFO:
        set_visible_child(m_info_label);
        m_info_label.set_text(text);
        break;

      case NotificationType::ERROR:
        set_visible_child(m_error_label);
        m_error_label.set_text(text);
        break;
    }
  }

  void
  CommandEntry::grab_focus()
  {
    m_entry.grab_focus_without_selecting();
    m_entry.set_position(m_entry.get_text().length());
  }

  void
  CommandEntry::on_activate()
  {
    const auto text = m_entry.get_text();

    m_entry.set_text(Glib::ustring());
    m_signal_command_received.emit(text);
  }

  void
  CommandEntry::on_notification_timeout()
  {
    std::lock_guard<std::mutex> guard(m_notification_queue_mutex);

    if (m_notification_queue.empty())
    {
      return;
    }
    m_notification_queue.pop();
    if (m_notification_queue.empty())
    {
      set_visible_child(m_entry);
    } else {
      const auto& next_notification = m_notification_queue.front();

      show_notification(next_notification);
      Glib::signal_timeout().connect_once(
        sigc::mem_fun(*this, &CommandEntry::on_notification_timeout),
        std::get<2>(next_notification) * 1000
      );
    }
  }
}
