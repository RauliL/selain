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
#ifndef SELAIN_TAB_HPP_GUARD
#define SELAIN_TAB_HPP_GUARD

#include <selain/status-bar.hpp>

#include <webkit2/webkit2.h>

namespace selain
{
  class MainWindow;

  /**
   * GTK widget for a browser tab.
   */
  class Tab : public Gtk::Box
  {
  public:
    explicit Tab();

    /**
     * Returns pointer to the main window where this tab is being displayed, or
     * null pointer if this tab isn't being displayed on a window.
     */
    MainWindow* get_main_window();

    /**
     * Returns pointer to the main window where this tab is being displayed, or
     * null pointer if this tab isn't being displayed on a window.
     */
    const MainWindow* get_main_window() const;

    inline StatusBar& status_bar()
    {
      return m_status_bar;
    }

    inline const StatusBar& status_bar() const
    {
      return m_status_bar;
    }

    inline Gtk::Entry& command_entry()
    {
      return m_command_entry;
    }

    inline const Gtk::Entry& command_entry() const
    {
      return m_command_entry;
    }

    inline Mode get_mode() const
    {
      return m_mode;
    }

    void set_mode(Mode mode);

    Glib::ustring get_uri() const;
    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);

    void execute_command(const Glib::ustring& command);
    void execute_script(const Glib::ustring& script);

    void go_back();
    void go_forward();

    void search(const Glib::ustring& text, bool forwards = true);
    void search_next();
    void search_prev();

    void grab_focus();

  private:
    void on_show();
    bool on_command_entry_key_press(::GdkEventKey* event);
    void on_command_received();

  private:
    Mode m_mode;
    ::WebKitWebView* m_web_view;
    Glib::RefPtr<Gtk::Widget> m_web_view_widget;
    StatusBar m_status_bar;
    Gtk::Entry m_command_entry;
  };
}

#endif /* !SELAIN_TAB_HPP_GUARD */
