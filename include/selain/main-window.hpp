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
#ifndef SELAIN_MAIN_WINDOW_HPP_GUARD
#define SELAIN_MAIN_WINDOW_HPP_GUARD

#include <gtkmm.h>

#include <selain/tab.hpp>

namespace selain
{
  /**
   * Main window for the browser.
   */
  class MainWindow : public Gtk::Window
  {
  public:
    explicit MainWindow();

    Glib::RefPtr<Tab> open_tab(
      const Glib::ustring& uri = Glib::ustring(),
      bool focus = true
    );

    void close_tab(const Tab* tab);
    void close_tab(const Glib::RefPtr<Tab>& tab);

    void set_current_tab(const Glib::RefPtr<Tab>& tab);
    void next_tab();
    void prev_tab();

    void set_tab_title(Tab* tab, const Glib::ustring& title);

  private:
    Gtk::Notebook m_notebook;
  };
}

#endif /* !SELAIN_MAIN_WINDOW_HPP_GUARD */
