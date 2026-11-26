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

#include <gtkmm.h>

#include <selain/webkit/context.hpp>
#include <selain/webkit/settings.hpp>

namespace selain::ui
{
  class MainWindow;
  class Tab;

  /**
   * Single browsable pane (Vim "window") containing a WebKit web view.
   */
  class View : public Gtk::Box
  {
  public:
    using script_callback_type = std::function<void(JSCValue*, GError*)>;
    using prompt_callback_type = std::function<void(const Glib::ustring&)>;

    explicit View(
      const Glib::RefPtr<webkit::Context>& context,
      const Glib::RefPtr<webkit::Settings>& settings
    );
    ~View() override;

    /**
     * Returns pointer to the tab this view is associated with, or `nullptr` if
     * no tab is associated with the view for some reason.
     */
    Tab* get_tab();

    /**
     * Returns pointer to the tab this view is associated with, or `nullptr` if
     * no tab is associated with the view for some reason.
     */
    const Tab* get_tab() const;

    /**
     * Returns pointer to the main window that this view is associated with, or
     * `nullptr` if this view is not visible on any window.
     */
    MainWindow* get_main_window();

    /**
     * Returns pointer to the main window that this view is associated with, or
     * `nullptr` if this view is not visible on any window.
     */
    const MainWindow* get_main_window() const;

    /**
     * Returns the URI that this view is displaying at the moment, or
     * `std::nullopt` if the view isn't displaying anything.
     */
    std::optional<Glib::ustring> get_uri() const;

    /**
     * Returns the hostname part of URI that this view is displaying at the
     * moment, or `std::nullopt` if it cannot be determined.
     */
    std::optional<Glib::ustring> get_host() const;

    /**
     * Returns title of the document that this view is displaying at the
     * moment, or `std::nullopt` if the view isn't displaying an document or
     * the document has no title.
     */
    std::optional<Glib::ustring> get_title() const;

    /**
     * Appends URIs from this view's back/forward list into \p uris.
     */
    void collect_history_uris(std::vector<Glib::ustring>& uris) const;

    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);
    void stop_loading();

    /**
     * Asks the parent window to collect free-form text via the command bar
     * (keyboard only). @p on_submit is called with the typed value on Enter.
     */
    void request_prompt(
      const Glib::ustring& prompt,
      bool sensitive,
      prompt_callback_type on_submit
    );

    /**
     * Returns the View associated with @p web_view, or null if none.
     */
    static View* from_web_view(::WebKitWebView* web_view);

    /**
     * Records one Gemini redirect hop. Returns false if the hop limit was
     * exceeded.
     */
    bool consume_gemini_redirect_hop();

    /**
     * Clears the Gemini redirect hop counter after a non-redirect response.
     */
    void reset_gemini_redirect_hops();

    void execute_script(
      const Glib::ustring& script,
      const script_callback_type& callback = nullptr,
      const Glib::RefPtr<Gio::Cancellable>& cancellable = nullptr,
      const char* world_name = nullptr
    );

    WebKitUserContentManager* get_user_content_manager() const;
    WebKitFindController* get_find_controller() const;

    void go_back();
    void go_forward();

    /**
     * Returns the current page zoom factor (1.0 = 100%).
     */
    double get_zoom_level() const;

    /**
     * Sets the page zoom factor (1.0 = 100%).
     */
    void set_zoom_level(double level);

    /**
     * Toggles WebKit developer tools for this view.
     */
    void toggle_inspector();

    void search(const Glib::ustring& text, bool forwards = true);
    void search_next();
    void search_prev();

    void grab_focus();

    /** Disconnect signals and unparent the web view. */
    void prepare_close();

    const Glib::ustring& get_status() const;
    void set_status(const Glib::ustring& status, bool permanent = false);

    Glib::RefPtr<Gdk::Paintable> get_favicon() const;

    /**
     * Records the current URI in persistent visit history, if appropriate.
     */
    void record_history_visit();

  protected:
    bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);

  private:
    Glib::RefPtr<Gtk::EventControllerKey> m_key_controller;
    WebKitWebView* m_web_view;
    Glib::ustring m_status;
    Glib::ustring m_permanent_status;
    Glib::ustring m_last_history_uri;
    int m_gemini_redirect_hops = 0;
  };
}
