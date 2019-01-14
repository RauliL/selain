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
#include <selain/tab.hpp>

namespace selain
{
  static const int DEFAULT_WIDTH = 640;
  static const int DEFAULT_HEIGHT = 480;

  MainWindow::MainWindow()
  {
    set_title("Selain");
    set_border_width(0);
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    maximize();
    add(m_notebook);
    show_all();

    open_tab("https://duckduckgo.com");
  }

  Glib::RefPtr<Tab>
  MainWindow::open_tab(const Glib::ustring& uri, bool focus)
  {
    const auto tab = Glib::RefPtr<Tab>(new Tab());

    m_notebook.append_page(*tab.get(), "New tab");
    show_all_children();
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
  MainWindow::close_tab(const Tab* tab)
  {
    const auto index = m_notebook.page_num(*tab);

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

  void
  MainWindow::close_tab(const Glib::RefPtr<Tab>& tab)
  {
    close_tab(tab.get());
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

  void
  MainWindow::set_tab_title(Tab* tab, const Glib::ustring& title)
  {
    const auto index = m_notebook.page_num(*tab);

    if (index < 0)
    {
      return;
    }
    m_notebook.set_tab_label_text(
      *tab,
      title.length() > 20 ? title.substr(0, 19) + U'\u2026' : title
    );
  }
}
