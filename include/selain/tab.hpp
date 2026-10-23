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

#include <vector>

#include <selain/hint-context.hpp>
#include <selain/tab-label.hpp>
#include <selain/view.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  class MainWindow;

  /**
   * Direction used when focusing a neighbouring split view.
   */
  enum class FocusDirection
  {
    LEFT,
    RIGHT,
    UP,
    DOWN
  };

  /**
   * GTK widget for a browser tab page (Vim "tab"), which may contain one or
   * more split views (Vim "windows").
   */
  class Tab : public Gtk::Box
  {
  public:
    using status_changed_signal_type = sigc::signal<
      void(Tab*, const Glib::ustring&)
    >;

    explicit Tab(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );
    ~Tab() override;

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

    /**
     * Returns the currently focused split view inside this tab.
     */
    View* get_active_view();
    const View* get_active_view() const;

    void set_active_view(View& view);

    /**
     * Splits the active view. When vertical is false, creates a horizontal
     * split (:split). When vertical is true, creates a vertical split
     * (:vsplit). Optional URI is loaded in the new view; otherwise the
     * current URI is reused.
     */
    void split(bool vertical, const Glib::ustring& uri = Glib::ustring());

    /**
     * Closes the active split view. If it is the only view, closes the tab.
     */
    void close_active_view();

    /**
     * Closes all split views except the active one.
     */
    void close_other_views();

    /**
     * Focuses the nearest split view in the given direction.
     */
    void focus_view(FocusDirection direction);

    /**
     * Cycles focus to the next or previous split view.
     */
    void cycle_view(bool backwards = false);

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

    void on_view_status_changed(View& view, const Glib::ustring& status);
    void on_view_title_changed(View& view);
    void on_view_favicon_changed(View& view);

    inline status_changed_signal_type& signal_status_changed()
    {
      return m_signal_status_changed;
    }

    inline const status_changed_signal_type& signal_status_changed() const
    {
      return m_signal_status_changed;
    }

  private:
    Glib::RefPtr<View> create_view();
    void replace_view_with_paned(
      View& current,
      View& next,
      Gtk::Orientation orientation
    );
    void promote_sibling(Gtk::Paned& paned, Gtk::Widget& sibling);
    void collect_views(std::vector<View*>& output) const;
    void sync_active_chrome();
    void on_close_button_clicked();

  private:
    Glib::RefPtr<WebContext> m_web_context;
    Glib::RefPtr<WebSettings> m_web_settings;
    Glib::RefPtr<HintContext> m_hint_context;
    TabLabel m_tab_label;
    std::vector<Glib::RefPtr<View>> m_views;
    View* m_active_view;
    status_changed_signal_type m_signal_status_changed;
  };
}
