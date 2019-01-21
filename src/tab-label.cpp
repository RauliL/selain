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
#include <selain/tab-label.hpp>

namespace selain
{
  TabLabel::TabLabel()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , m_text("Untitled")
  {
    m_icon.set_from_icon_name("text-x-generic", Gtk::ICON_SIZE_BUTTON);
    m_icon.set_margin_end(5);

    m_close_button.set_relief(Gtk::RELIEF_NONE);
    m_close_button.set_focus_on_click(false);
    m_close_button.set_image_from_icon_name(
      "window-close",
      Gtk::ICON_SIZE_BUTTON
    );
    m_close_button.set_margin_start(5);

    pack_start(m_icon, Gtk::PACK_SHRINK);
    pack_start(m_text);
    pack_start(m_close_button, Gtk::PACK_SHRINK);
    show_all();
  }

  void
  TabLabel::set_text(const Glib::ustring& text)
  {
    const auto length = text.length();

    m_text.set_text(length > 20 ? text.substr(0, 19) + U'\u2026' : text);
  }

  void
  TabLabel::set_icon(const Glib::RefPtr<Gdk::Pixbuf>& icon)
  {
    if (icon)
    {
      m_icon.set(icon);
    } else {
      m_icon.set_from_icon_name("text-x-generic", Gtk::ICON_SIZE_BUTTON);
    }
  }
}
