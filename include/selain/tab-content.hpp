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
#ifndef SELAIN_TAB_CONTENT_HPP_GUARD
#define SELAIN_TAB_CONTENT_HPP_GUARD

#include <selain/tab.hpp>
#include <selain/tab-status.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  /**
   * The actual tab implementation which contains WebKit web view and acts as a
   * browser.
   */
  class TabContent : public Tab
  {
  public:
    explicit TabContent(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );

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
    /** Current mode of the tab. */
    Mode m_mode;
    /** The wrapped WebKit web view. */
    ::WebKitWebView* m_web_view;
    /** WebKit web view wrapped into GTKmm widget. */
    const Glib::RefPtr<Widget> m_web_view_widget;
    /** Status bar for the tab. */
    TabStatus m_status;
  };
}

#endif /* !SELAIN_TAB_CONTENT_HPP_GUARD */
