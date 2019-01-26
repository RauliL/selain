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
#ifndef SELAIN_TAB_WINDOW_HPP_GUARD
#define SELAIN_TAB_WINDOW_HPP_GUARD

#include <selain/tab-status.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  class MainWindow;

  class TabSplitWindow : public TabWindow
  {
  public:
    explicit TabSplitWindow(
      const Glib::RefPtr<TabWindow>& window1,
      const Glib::RefPtr<TabWindow>& window2,
      Gtk::Orientation orientation = Gtk::ORIENTATION_VERTICAL
    );

    inline const Glib::RefPtr<TabWindow>& get_active_window()
    {
      return m_active_window_index == 1 ? m_window1 : m_window2;
    }

    inline Glib::RefPtr<const TabWindow> get_active_window() const
    {
      return m_active_window_index == 1 ? m_window1 : m_window2;
    }

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
    void on_title_changed(TabWindow& window, const Glib::ustring& title);
    void on_favicon_changed(TabWindow& window, ::cairo_surface_t* surface);

  private:
    int m_active_window_index;
    const Glib::RefPtr<TabWindow> m_window1;
    const Glib::RefPtr<TabWindow> m_window2;
    Gtk::Paned m_paned;
  };

  class TabContentWindow : public TabWindow
  {
  public:
    explicit TabContentWindow(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );

    /**
     * Sets the text on the tab status bar.
     *
     * \param text      Text to be displayed on the status bar.
     * \param permanent Whether the text should be considered to be "permanent"
     *                  e.g. it's a text that should be displayed if the normal
     *                  status text is being cleared.
     */
    void set_status(const Glib::ustring& text, bool permanent = true);

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
    Gtk::Box m_box;
    ::WebKitWebView* m_web_view;
    const Glib::RefPtr<Gtk::Widget> m_web_view_widget;
    TabStatus m_status;
  };
}

#endif /* !SELAIN_TAB_WINDOW_HPP_GUARD */
