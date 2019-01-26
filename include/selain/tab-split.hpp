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
#ifndef SELAIN_TAB_SPLIT_HPP_GUARD
#define SELAIN_TAB_SPLIT_HPP_GUARD

#include <selain/tab.hpp>

namespace selain
{
  /**
   * Tab which has been splitted into two views.
   */
  class TabSplit : public Tab
  {
  public:
    explicit TabSplit(
      const Glib::RefPtr<Tab>& tab1,
      const Glib::RefPtr<Tab>& tab2,
      Gtk::Orientation orientation = Gtk::ORIENTATION_VERTICAL
    );

    inline const Glib::RefPtr<Tab>& get_active_tab()
    {
      return m_active_tab_index == 1 ? m_tab1 : m_tab2;
    }

    inline Glib::RefPtr<const Tab> get_active_tab() const
    {
      return m_active_tab_index == 1 ? m_tab1 : m_tab2;
    }

    Mode get_mode() const;
    void set_mode(Mode mode);
    Glib::ustring get_uri() const;
    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);
    void stop_loading();
    void go_back_in_history();
    void go_forward_in_history();
    void find(const Glib::ustring& text, bool forwards = true);
    void find_next();
    void find_previous();
    void execute_script(
      const Glib::ustring& script,
      ::GCancellable* cancellable = nullptr,
      ::GAsyncReadyCallback callback = nullptr,
      void* data = nullptr
    );
    void grab_focus();

  private:
    void on_title_changed(Tab& tab, const Glib::ustring& title);
    void on_favicon_changed(Tab& tab, ::cairo_surface_t* surface);

  private:
    int m_active_tab_index;
    const Glib::RefPtr<Tab> m_tab1;
    const Glib::RefPtr<Tab> m_tab2;
    const Gtk::Paned m_paned;
  };
}

#endif /* !SELAIN_TAB_SPLIT_HPP_GUARD */
