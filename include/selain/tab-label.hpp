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
#pragma once

#include <gtkmm.h>

namespace selain
{
  class TabLabel : public Gtk::Box
  {
  public:
    using close_signal_type = sigc::signal<void()>;

    explicit TabLabel();

    void set_text(const Glib::ustring& text);

    void set_icon(const Glib::RefPtr<Gdk::Paintable>& icon);

    inline close_signal_type& signal_close_requested()
    {
      return m_signal_close_requested;
    }

  private:
    void on_close_button_clicked();
    void on_middle_button_pressed(int n_press, double x, double y);

  private:
    Gtk::Image m_icon;
    Gtk::Label m_text;
    Gtk::Button m_close_button;
    Glib::RefPtr<Gtk::GestureClick> m_middle_click_gesture;
    close_signal_type m_signal_close_requested;
  };
}
