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

#include <vector>

#include <selain/hint-context.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  class Tab;

  /**
   * Single browsable pane (Vim "window") containing a WebKit web view.
   */
  class View : public Gtk::Box
  {
  public:
    explicit View(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );
    ~View() override;

    Tab* get_tab();
    const Tab* get_tab() const;

    Glib::ustring get_uri() const;
    Glib::ustring get_title() const;

    /**
     * Appends URIs from this view's back/forward list into \p uris.
     */
    void collect_history_uris(std::vector<Glib::ustring>& uris) const;

    void load_uri(const Glib::ustring& uri);
    void reload(bool bypass_cache = false);
    void stop_loading();

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

    Glib::RefPtr<Gdk::Paintable> get_favicon() const;

  private:
    ::WebKitWebView* m_web_view;
    Glib::RefPtr<Gtk::Widget> m_web_view_widget;
    Glib::ustring m_status;
    Glib::ustring m_permanent_status;
  };
}
