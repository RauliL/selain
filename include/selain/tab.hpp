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

#include <selain/hint-context.hpp>
#include <selain/tab-label.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  class MainWindow;

  /**
   * GTK widget for a browser tab.
   */
  class Tab : public Gtk::Bin
  {
  public:
    using status_changed_signal_type = sigc::signal<
      void,
      Tab*,
      const Glib::ustring&
    >;

    explicit Tab(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );

    inline Glib::RefPtr<HintContext>& get_hint_context()
    {
      return m_hint_context;
    }

    inline Glib::RefPtr<const HintContext> get_hint_context() const
    {
      return m_hint_context;
    }

    void set_hint_context(const Glib::RefPtr<HintContext>& hint_context);

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

    /**
     * Returns the GTK widget used as label for the tab.
     */
    inline TabLabel& get_tab_label()
    {
      return m_tab_label;
    }

    /**
     * Returns the GTK widget used as label for the tab.
     */
    inline const TabLabel& get_tab_label() const
    {
      return m_tab_label;
    }

    Glib::ustring get_uri() const;
    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);
    void stop_loading();

    void execute_command(const Glib::ustring& command);
    void execute_script(
      const Glib::ustring& script,
      ::GCancellable* cancellable = nullptr,
      ::GAsyncReadyCallback callback = nullptr,
      void* user_data = nullptr
    );

    void go_back();
    void go_forward();

    void search(const Glib::ustring& text, bool forwards = true);
    void search_next();
    void search_prev();

    void grab_focus();

    const Glib::ustring& get_status() const;
    void set_status(const Glib::ustring& status, bool permanent = false);

    inline status_changed_signal_type& signal_status_changed()
    {
      return m_signal_status_changed;
    }

    inline const status_changed_signal_type& signal_status_changed() const
    {
      return m_signal_status_changed;
    }

  private:
    void on_close_button_clicked();

  private:
    Glib::RefPtr<HintContext> m_hint_context;
    TabLabel m_tab_label;
    ::WebKitWebView* m_web_view;
    Glib::RefPtr<Gtk::Widget> m_web_view_widget;
    Glib::ustring m_status;
    Glib::ustring m_permanent_status;
    status_changed_signal_type m_signal_status_changed;
  };
}

#endif /* !SELAIN_TAB_HPP_GUARD */
