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
#ifndef SELAIN_WEB_VIEW_HPP_GUARD
#define SELAIN_WEB_VIEW_HPP_GUARD

#include <selain/hint-context.hpp>
#include <selain/status-bar.hpp>
#include <selain/view.hpp>

namespace selain
{
  /**
   * Implementation of a view which displays web contents, e.g. the actual
   * browser implementation.
   */
  class WebView : public View
  {
  public:
    explicit WebView(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );

    /**
     * Sets the text on the status bar of the view.
     *
     * \param text      Text to be displayed on the status bar.
     * \param permanent Whether the new status text should be considered to be
     *                  permanent, e.g. it should be displayed when the text on
     *                  the status bar is cleared instead of empty status bar.
     */
    void set_status(const Glib::ustring& text, bool permanent = false);

    void execute_script(
      const Glib::ustring& script,
      ::GCancellable* cancellable = nullptr,
      ::GAsyncReadyCallback callback = nullptr,
      void* data = nullptr
    );
    void set_current_mode(Mode mode);
    Glib::ustring get_uri() const;
    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);
    void stop_loading();
    void go_back_in_history();
    void go_forward_in_history();
    void find(
      const Glib::ustring& text,
      FindDirection direction = FindDirection::FORWARD
    );
    void find_next();
    void find_previous();
    void add_hint_character(Glib::ustring::value_type c);
    void remove_hint_character();
    void activate_current_hint();

  private:
    bool on_focus_in();

  private:
    Glib::ustring m_permanent_status;
    Glib::RefPtr<HintContext> m_hint_context;
    Gtk::Box m_box;
    ::WebKitWebView* m_web_view;
    const Glib::RefPtr<Gtk::Widget> m_web_view_widget;
    StatusBar m_status_bar;
  };
}

#endif /* !SELAIN_WEB_VIEW_HPP_GUARD */
