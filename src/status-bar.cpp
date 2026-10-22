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
#include <selain/status-bar.hpp>
#include <selain/theme.hpp>

namespace selain
{
  StatusBar::StatusBar()
    : Gtk::Box(Gtk::Orientation::HORIZONTAL)
    , m_mode_label("NORMAL")
  {
    const auto& style_provider = theme::get_status_bar_style_provider();

    add_css_class("selain-status-bar");
    get_style_context()->add_provider(
      style_provider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    m_mode_label.add_css_class("mode-label");
    m_mode_label.set_halign(Gtk::Align::CENTER);
    m_mode_label.set_justify(Gtk::Justification::CENTER);
    m_mode_label.get_style_context()->add_provider(
      style_provider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    m_status_label.add_css_class("status-label");
    m_status_label.set_halign(Gtk::Align::START);
    m_status_label.set_justify(Gtk::Justification::LEFT);
    m_status_label.set_hexpand(true);
    m_status_label.get_style_context()->add_provider(
      style_provider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    append(m_mode_label);
    append(m_status_label);
  }

  void
  StatusBar::set_mode(Mode mode)
  {
    m_mode_label.set_text(get_mode_text(mode));
    if (mode == Mode::INSERT)
    {
      m_mode_label.add_css_class("mode-insert");
    } else {
      m_mode_label.remove_css_class("mode-insert");
    }
  }

  void
  StatusBar::set_status(const Glib::ustring& status)
  {
    m_status_label.set_text(status);
  }
}
