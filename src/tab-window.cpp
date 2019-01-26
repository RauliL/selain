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
#include <selain/main-window.hpp>
#include <selain/theme.hpp>

namespace selain
{
  static void on_load_changed(
    ::WebKitWebView*,
    ::WebKitLoadEvent,
    TabContentWindow*
  );
  static ::gboolean on_decide_policy(
    ::WebKitWebView*,
    ::WebKitPolicyDecision*,
    ::WebKitPolicyDecisionType,
    TabContentWindow*
  );
  static void on_mouse_target_changed(
    ::WebKitWebView*,
    ::WebKitHitTestResult*,
    ::guint,
    TabContentWindow*
  );
  static void on_notify_title(
    ::WebKitWebView*,
    ::GParamSpec*,
    TabContentWindow*
  );
  static void on_notify_favicon(
    ::WebKitWebView*,
    ::GParamSpec*,
    TabContentWindow*
  );

  namespace keyboard
  {
    /**
     * GTK signal callback used for key press events inside web view of a tab.
     */
    ::gboolean on_tab_key_press(::WebKitWebView*, ::GdkEventKey*, TabWindow*);
  }

  Glib::ustring
  TabSplitWindow::get_uri() const
  {
    return get_active_window()->get_uri();
  }

  void
  TabSplitWindow::load_uri(const Glib::ustring& uri)
  {
    get_active_window()->load_uri(uri);
  }

  void
  TabSplitWindow::reload(bool bypass_cache)
  {
    get_active_window()->reload(bypass_cache);
  }

  void
  TabSplitWindow::stop_loading()
  {
    get_active_window()->stop_loading();
  }

  void
  TabSplitWindow::go_back_in_history()
  {
    get_active_window()->go_back_in_history();
  }

  void
  TabSplitWindow::go_forward_in_history()
  {
    get_active_window()->go_forward_in_history();
  }

  void
  TabSplitWindow::find(const Glib::ustring& text, bool forwards)
  {
    get_active_window()->find(text, forwards);
  }

  void
  TabSplitWindow::find_next()
  {
    get_active_window()->find_next();
  }

  void
  TabSplitWindow::find_previous()
  {
    get_active_window()->find_previous();
  }

  void
  TabSplitWindow::execute_script(const Glib::ustring& script,
                                 ::GCancellable* cancellable,
                                 ::GAsyncReadyCallback callback,
                                 void* data)
  {
    get_active_window()->execute_script(script, cancellable, callback, data);
  }

  TabContentWindow::TabContentWindow(const Glib::RefPtr<WebContext>& context,
                                     const Glib::RefPtr<WebSettings>& settings)
    : TabWindow()
    , m_box(Gtk::ORIENTATION_VERTICAL)
    , m_web_view(context->create_web_view())
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    m_box.pack_start(*m_web_view_widget.get());
    m_box.pack_start(m_status, Gtk::PACK_SHRINK);
    m_box.show_all();
    add(m_box);
    show_all();

    if (settings)
    {
      settings->install(m_web_view);
    }

    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "load-changed",
      G_CALLBACK(on_load_changed),
      static_cast<::gpointer>(this)
    );
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "decide-policy",
      G_CALLBACK(on_decide_policy),
      static_cast<::gpointer>(this)
    );
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "mouse-target-changed",
      G_CALLBACK(on_mouse_target_changed),
      static_cast<::gpointer>(this)
    );
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "key-press-event",
      G_CALLBACK(keyboard::on_tab_key_press),
      static_cast<::gpointer>(this)
    );
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "notify::title",
      G_CALLBACK(on_notify_title),
      static_cast<::gpointer>(this)
    );
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "notify::favicon",
      G_CALLBACK(on_notify_favicon),
      static_cast<::gpointer>(this)
    );
  }

  void
  TabContentWindow::set_status(const Glib::ustring& text, bool permanent)
  {
    if (permanent)
    {
      m_status.set_permanent_text(text);
    } else {
      m_status.set_text(text);
    }
  }

  Glib::ustring
  TabContentWindow::get_uri() const
  {
    const auto uri = ::webkit_web_view_get_uri(m_web_view);

    if (!uri || !*uri)
    {
      return Glib::ustring();
    }

    return uri;
  }

  void
  TabContentWindow::load_uri(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return;
    }
    if (auto scheme = ::g_uri_parse_scheme(uri.c_str()))
    {
      ::webkit_web_view_load_uri(m_web_view, uri.c_str());
      ::g_free(scheme);
    } else {
      ::webkit_web_view_load_uri(m_web_view, ("http://" + uri).c_str());
    }
  }

  void
  TabContentWindow::reload(bool bypass_cache)
  {
    if (bypass_cache)
    {
      ::webkit_web_view_reload_bypass_cache(m_web_view);
    } else {
      ::webkit_web_view_reload(m_web_view);
    }
  }

  void
  TabContentWindow::stop_loading()
  {
    ::webkit_web_view_stop_loading(m_web_view);
  }

  void
  TabContentWindow::go_back_in_history()
  {
    ::webkit_web_view_go_back(m_web_view);
  }

  void
  TabContentWindow::go_forward_in_history()
  {
    ::webkit_web_view_go_forward(m_web_view);
  }

  void
  TabContentWindow::find(const Glib::ustring& text, bool forwards)
  {
    auto controller = ::webkit_web_view_get_find_controller(m_web_view);
    ::guint32 options = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

    if (text.empty())
    {
      return;
    }
    if (!forwards)
    {
      options |= WEBKIT_FIND_OPTIONS_BACKWARDS;
    }
    ::webkit_find_controller_search(
      controller,
      text.c_str(),
      options,
      150
    );
  }

  void
  TabContentWindow::find_next()
  {
    ::webkit_find_controller_search_next(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  TabContentWindow::find_previous()
  {
    ::webkit_find_controller_search_previous(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  TabContentWindow::execute_script(const Glib::ustring& script,
                                   ::GCancellable* cancellable,
                                   ::GAsyncReadyCallback callback,
                                   void* data)
  {
    ::webkit_web_view_run_javascript(
      m_web_view,
      script.c_str(),
      cancellable,
      callback,
      static_cast<::gpointer>(data)
    );
  }

  void
  TabContentWindow::grab_focus()
  {
    m_web_view_widget->grab_focus();
  }

  static void
  on_load_changed(::WebKitWebView* web_view,
                  ::WebKitLoadEvent load_event,
                  TabContentWindow* window)
  {
    switch (load_event)
    {
      case WEBKIT_LOAD_STARTED:
        window->signal_title_changed().emit(*window, "Loading\xe2\x80\xa6");
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          window->set_status(uri, true);
          window->set_status(Glib::ustring("Loading ") + uri + U'\u2026');
        }
        break;

      case WEBKIT_LOAD_REDIRECTED:
        window->signal_title_changed().emit(
          *window,
          "Redirecting\xe2\x80\xa6"
        );
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          window->set_status(
            Glib::ustring("Redirecting to ") +
            uri +
            U'\u2026'
          );
        }
        break;

      case WEBKIT_LOAD_COMMITTED:
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          window->set_status(uri, true);
        }
        break;

      case WEBKIT_LOAD_FINISHED:
        window->set_status(Glib::ustring());
        break;
    }
  }

  static ::gboolean
  on_decide_policy(::WebKitWebView* web_view,
                   ::WebKitPolicyDecision* decision,
                   ::WebKitPolicyDecisionType decision_type,
                   TabContentWindow* window)
  {
    const auto main_window = window->get_main_window();

    // Open links clicked with middle mouse button in a new tab in the
    // background.
    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    {
      auto navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
      auto action = ::webkit_navigation_policy_decision_get_navigation_action(
        navigation_decision
      );
      auto action_type = ::webkit_navigation_action_get_navigation_type(
        action
      );

      if (::webkit_navigation_action_get_mouse_button(action) == 2 &&
          action_type == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED &&
          main_window)
      {
        main_window->open_tab(
          ::webkit_uri_request_get_uri(
            ::webkit_navigation_action_get_request(action)
          ),
          false
        );
        ::webkit_policy_decision_ignore(decision);
      }
    }
    // Open new windows into tabs.
    else if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
    {
      auto navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
      auto action = ::webkit_navigation_policy_decision_get_navigation_action(
        navigation_decision
      );
      auto action_type = ::webkit_navigation_action_get_navigation_type(
        action
      );

      if (action_type == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED && main_window)
      {
        main_window->open_tab(
          ::webkit_uri_request_get_uri(
            ::webkit_navigation_action_get_request(action)
          ),
          false
        );
        ::webkit_policy_decision_ignore(decision);
      }
    }

    return true;
  }

  static void
  on_mouse_target_changed(::WebKitWebView*,
                          ::WebKitHitTestResult* hit_test_result,
                          ::guint,
                          TabContentWindow* window)
  {
    const auto uri = ::webkit_hit_test_result_get_link_uri(hit_test_result);

    window->set_status(!uri || !*uri ? Glib::ustring() : uri);
  }

  static void
  on_notify_title(::WebKitWebView* web_view,
                  ::GParamSpec*,
                  TabContentWindow* window)
  {
    const auto title = ::webkit_web_view_get_title(web_view);

    window->signal_title_changed().emit(
      *window,
      title && *title ? title : Glib::ustring("Untitled")
    );
  }

  static void
  on_notify_favicon(::WebKitWebView* web_view,
                    ::GParamSpec*,
                    TabContentWindow* window)
  {
    window->signal_favicon_changed().emit(
      *window,
      ::webkit_web_view_get_favicon(web_view)
    );
  }
}
