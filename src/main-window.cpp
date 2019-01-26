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
#include <selain/main-window.hpp>
#include <selain/theme.hpp>

namespace selain
{
  static const int DEFAULT_WIDTH = 640;
  static const int DEFAULT_HEIGHT = 480;

  MainWindow::MainWindow(const Glib::RefPtr<Gtk::Application>& application)
    : Gtk::ApplicationWindow(application)
    , m_web_context(WebContext::create())
    , m_web_settings(WebSettings::create())
    , m_box(Gtk::ORIENTATION_VERTICAL)
  {
    initialize_commands();

    set_title("Selain");
    set_icon_name("selain");
    set_border_width(0);
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    m_box.pack_start(m_notebook);
    m_box.pack_start(m_command_entry, Gtk::PACK_SHRINK);
    add(m_box);

    m_command_entry.signal_key_press_event().connect(sigc::mem_fun(
      this,
      &MainWindow::on_command_entry_key_press
    ));
    m_command_entry.signal_command_received().connect(sigc::mem_fun(
      this,
      &MainWindow::on_command_received
    ));

    m_box.override_background_color(theme::window_background);

    show_all();
    maximize();
  }

  Tab*
  MainWindow::get_current_tab()
  {
    const auto index = m_notebook.get_current_page();

    return index >= 0 ? get_nth_tab(index) : nullptr;
  }

  const Tab*
  MainWindow::get_current_tab() const
  {
    const auto index = m_notebook.get_current_page();

    return index >= 0 ? get_nth_tab(index) : nullptr;
  }

  Tab*
  MainWindow::get_nth_tab(int index)
  {
    const auto widget = m_notebook.get_nth_page(index);

    return widget ? static_cast<Tab*>(widget) : nullptr;
  }

  const Tab*
  MainWindow::get_nth_tab(int index) const
  {
    const auto widget = m_notebook.get_nth_page(index);

    return widget ? static_cast<const Tab*>(widget) : nullptr;
  }

  Glib::RefPtr<Tab>
  MainWindow::open_tab(const Glib::ustring& uri, bool focus)
  {
    const auto tab = Glib::RefPtr<Tab>(new Tab(m_web_context, m_web_settings));

    m_notebook.append_page(*tab.get(), tab->get_tab_label());
    show_all_children();
    tab->signal_mode_changed().connect(sigc::mem_fun(
      *this,
      &MainWindow::on_tab_mode_change
    ));
    tab->signal_close().connect(sigc::mem_fun(
      *this,
      &MainWindow::on_tab_closed
    ));
    if (!uri.empty())
    {
      tab->load_uri(uri);
    }
    if (focus)
    {
      set_current_tab(tab);
      tab->grab_focus();
    }

    return tab;
  }

  void
  MainWindow::set_current_tab(const Glib::RefPtr<Tab>& tab)
  {
    const auto index = m_notebook.page_num(*tab.get());

    if (index >= 0)
    {
      m_notebook.set_current_page(index);
    }
  }

  void
  MainWindow::next_tab()
  {
    m_notebook.next_page();
  }

  void
  MainWindow::prev_tab()
  {
    m_notebook.prev_page();
  }

  bool
  MainWindow::on_command_entry_key_press(::GdkEventKey* event)
  {
    if (event->keyval == GDK_KEY_Escape)
    {
      if (const auto tab = get_current_tab())
      {
        tab->set_mode(Mode::NORMAL);
      }

      return true;
    }
    else if (event->keyval == GDK_KEY_Tab)
    {
      return true;
    }

    return false;
  }

  void
  MainWindow::on_command_received(const Glib::ustring& command)
  {
    if (const auto tab = get_current_tab())
    {
      tab->set_mode(Mode::NORMAL);
      tab->execute_command(command);
    }
  }

  void
  MainWindow::on_tab_mode_change(View& view, Mode mode)
  {
    if (get_current_tab() != &view)
    {
      return;
    }
    if (mode == Mode::COMMAND)
    {
      m_command_entry.grab_focus();
    } else {
      m_command_entry.set_text(Glib::ustring());
      view.grab_focus();
    }
  }

  void
  MainWindow::on_tab_closed(View& view)
  {
    const auto index = m_notebook.page_num(view);

    if (index < 0)
    {
      return;
    }
    m_notebook.remove_page(index);
    if (m_notebook.get_current_page() < 0)
    {
      std::exit(EXIT_SUCCESS);
    }
  }
}
