/*
 * Copyright (c) 2019-2026, Rauli Laine
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

#include <memory>

#include <selain/ui/tab-label.hpp>
#include <selain/ui/view.hpp>
#include <selain/webkit/hint-context.hpp>

namespace selain::ui
{
  class MainWindow;

  /**
   * Node in a closed-tab layout tree: either a view leaf or a binary paned
   * split.
   */
  struct LayoutNode
  {
    enum class Kind
    {
      Leaf,
      Split
    };

    Kind kind = Kind::Leaf;
    std::optional<Glib::ustring> uri;
    double zoom = 1.0;
    Gtk::Orientation orientation = Gtk::Orientation::HORIZONTAL;
    /** Start-child fraction of the paned size in [0, 1], when known. */
    std::optional<double> ratio;
    std::unique_ptr<LayoutNode> start;
    std::unique_ptr<LayoutNode> end;
  };

  /**
   * Snapshot of a tab's split layout and focused view, used to reopen the last
   * closed tab.
   */
  struct LayoutSnapshot
  {
    std::unique_ptr<LayoutNode> root;
    int active_leaf_index = 0;

    /**
     * Returns true if any leaf has a non-empty URI.
     */
    bool has_any_uri() const;
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
      const Glib::RefPtr<webkit::Context>& context,
      const Glib::RefPtr<webkit::Settings>& settings
    );
    ~Tab() override;

    inline Glib::RefPtr<webkit::HintContext>& get_hint_context()
    {
      return m_hint_context;
    }

    inline Glib::RefPtr<const webkit::HintContext> get_hint_context() const
    {
      return m_hint_context;
    }

    void set_hint_context(const Glib::RefPtr<webkit::HintContext>& hint_context);

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
     * Splits the active view. Optional URI is loaded in the new view;
     * otherwise the current URI is reused.
     */
    void split(
      Gtk::Orientation orientation = Gtk::Orientation::HORIZONTAL,
      const std::optional<Glib::ustring>& uri = std::nullopt
    );

    /**
     * Closes the active split view. If it is the only view, closes the tab.
     */
    void close_active_view();

    /**
     * Closes all split views except the active one.
     */
    void close_other_views();

    /**
     * Unparents and releases all split views. Call before removing the tab
     * from the notebook so GTK does not finalize the tab with children still
     * attached.
     */
    void prepare_close();

    /**
     * Focuses the nearest split view in the given direction.
     */
    void focus_view(Gtk::DirectionType direction);

    /**
     * Cycles focus to the next or previous split view.
     */
    void cycle_view(bool backwards = false);

    /**
     * Replaces \p output with pointers to every split view in this tab.
     */
    void collect_views(std::vector<View*>& output) const;

    /**
     * Captures the current paned split tree, view URIs, and focused leaf.
     */
    LayoutSnapshot capture_layout() const;

    /**
     * Replaces the current layout with the given snapshot (views and splits).
     */
    void restore_layout(const LayoutSnapshot& snapshot);

    /**
     * Returns URI that currently active view is displaying or `std::nullopt`
     * if there is no active view or it's not displaying anything.
     */
    inline std::optional<Glib::ustring> get_uri() const
    {
      if (m_active_view)
      {
        return m_active_view->get_uri();
      }

      return std::nullopt;
    }

    /**
     * Returns the hostname part of URI that currently active view is
     * displaying at the moment, or `std::nullopt` if it cannot be determined.
     */
    inline std::optional<Glib::ustring> get_host() const
    {
      if (m_active_view)
      {
        return m_active_view->get_host();
      }

      return std::nullopt;
    }

    /**
     * Appends URIs from every split view's browsing history into \p uris.
     */
    void collect_history_uris(std::vector<Glib::ustring>& uris) const;

    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);
    void stop_loading();

    void execute_command(const Glib::ustring& command);

    inline void execute_script(
      const Glib::ustring& script,
      const View::script_callback_type& callback = nullptr,
      const Glib::RefPtr<Gio::Cancellable>& cancellable = nullptr,
      const char* world_name = nullptr
    )
    {
      if (m_active_view)
      {
        m_active_view->execute_script(
          script,
          callback,
          cancellable,
          world_name
        );
      }
    }

    void go_back();
    void go_forward();

    /**
     * Returns the active view's page zoom factor (1.0 = 100%).
     */
    double get_zoom_level() const;

    /**
     * Sets the active view's page zoom factor (1.0 = 100%).
     */
    void set_zoom_level(double level);

    /**
     * Toggles WebKit developer tools for the active view.
     */
    void toggle_inspector();

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
    Gtk::Paned& create_paned(Gtk::Orientation orientation);
    Gtk::Widget& build_layout_node(
      const LayoutNode& node,
      std::vector<View*>& leaves
    );
    void replace_view_with_paned(
      View& current,
      View& next,
      Gtk::Orientation orientation
    );
    void promote_sibling(Gtk::Paned& paned, Gtk::Widget& sibling);
    void sync_active_chrome();
    void on_close_button_clicked();

  private:
    Glib::RefPtr<webkit::Context> m_web_context;
    Glib::RefPtr<webkit::Settings> m_web_settings;
    Glib::RefPtr<webkit::HintContext> m_hint_context;
    TabLabel m_tab_label;
    std::vector<Glib::RefPtr<View>> m_views;
    View* m_active_view;
    status_changed_signal_type m_signal_status_changed;
  };
}
