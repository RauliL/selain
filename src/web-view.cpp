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
#include <selain/web-view.hpp>

namespace selain
{
  /**
   * GTK signal callback used for key press events inside the web view.
   */
  ::gboolean on_web_view_keypress(::WebKitWebView*, ::GdkEventKey*, WebView*);

  static void on_load_changed(
    ::WebKitWebView*,
    ::WebKitLoadEvent,
    WebView*
  );
  static ::gboolean on_decide_policy(
    ::WebKitWebView*,
    ::WebKitPolicyDecision*,
    ::WebKitPolicyDecisionType,
    WebView*
  );
  static void on_mouse_target_changed(
    ::WebKitWebView*,
    ::WebKitHitTestResult*,
    ::guint,
    WebView*
  );
  static void on_notify_title(
    ::WebKitWebView*,
    ::GParamSpec*,
    WebView*
  );
  static void on_notify_favicon(
    ::WebKitWebView*,
    ::GParamSpec*,
    WebView*
  );

  WebView::WebView(const Glib::RefPtr<WebContext>& context,
                   const Glib::RefPtr<WebSettings>& settings)
    : m_box(Gtk::ORIENTATION_VERTICAL)
    , m_web_view(context->create_web_view())
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    if (settings)
    {
      settings->install(m_web_view);
    }

    m_box.pack_start(*m_web_view_widget.get());
    m_box.pack_start(m_status_bar, Gtk::PACK_SHRINK);
    m_box.show_all();

    add(m_box);
    show_all();

    signal_focus_in_event().connect(sigc::hide(sigc::mem_fun(
      this,
      &WebView::on_focus_in
    )));
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
      G_CALLBACK(on_web_view_keypress),
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
  WebView::set_status(const Glib::ustring& text, bool permanent)
  {
    if (permanent)
    {
      m_permanent_status = text;
    }
    m_status_bar.set_status(text.empty() ? m_permanent_status : text);
  }

  void
  WebView::execute_script(const Glib::ustring& script,
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
  WebView::set_current_mode(Mode mode)
  {
    if (mode == Mode::HINT || mode == Mode::HINT_NEW_TAB)
    {
      if (m_hint_context)
      {
        m_hint_context->uninstall(*this);
      } else {
        m_hint_context = HintContext::create(mode == Mode::HINT_NEW_TAB);
      }
      m_hint_context->install(*this);
    }
    else if (m_hint_context)
    {
      m_hint_context->uninstall(*this);
      m_hint_context.reset();
    }
    m_status_bar.set_mode(mode);
  }

  Glib::ustring
  WebView::get_uri() const
  {
    const auto uri = ::webkit_web_view_get_uri(m_web_view);

    if (!uri || !*uri)
    {
      return Glib::ustring();
    }

    return uri;
  }

  void
  WebView::load_uri(const Glib::ustring& uri)
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
  WebView::reload(bool bypass_cache)
  {
    if (bypass_cache)
    {
      ::webkit_web_view_reload_bypass_cache(m_web_view);
    } else {
      ::webkit_web_view_reload(m_web_view);
    }
  }

  void
  WebView::stop_loading()
  {
    ::webkit_web_view_stop_loading(m_web_view);
  }

  void
  WebView::go_back_in_history()
  {
    ::webkit_web_view_go_back(m_web_view);
  }

  void
  WebView::go_forward_in_history()
  {
    ::webkit_web_view_go_forward(m_web_view);
  }

  void
  WebView::find(const Glib::ustring& text, FindDirection direction)
  {
    auto controller = ::webkit_web_view_get_find_controller(m_web_view);
    ::guint32 options = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

    if (text.empty())
    {
      return;
    }
    if (direction == FindDirection::BACKWARD)
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
  WebView::find_next()
  {
    ::webkit_find_controller_search_next(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  WebView::find_previous()
  {
    ::webkit_find_controller_search_previous(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  WebView::add_hint_character(Glib::ustring::value_type c)
  {
    if (m_hint_context)
    {
      m_hint_context->add_hint_character(*this, c);
    } else {
      ::g_warning("No hint mode installed.");
    }
  }

  void
  WebView::remove_hint_character()
  {
    if (m_hint_context)
    {
      m_hint_context->remove_hint_character(*this);
    } else {
      ::g_warning("No hint mode installed.");
    }
  }

  void
  WebView::activate_current_hint()
  {
    if (m_hint_context)
    {
      m_hint_context->activate_current_hint(*this);
    } else {
      ::g_warning("No hint mode installed.");
    }
  }

  bool
  WebView::on_focus_in()
  {
    m_web_view_widget->grab_focus();

    return true;
  }

  static void
  on_load_changed(::WebKitWebView* web_view,
                  ::WebKitLoadEvent load_event,
                  WebView* view)
  {
    switch (load_event)
    {
      case WEBKIT_LOAD_STARTED:
        view->signal_title_changed().emit(*view, "Loading\xe2\x80\xa6");
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          view->set_status(uri, true);
          view->set_status(Glib::ustring("Loading ") + uri + U'\u2026');
        }
        break;

      case WEBKIT_LOAD_REDIRECTED:
        view->signal_title_changed().emit(*view, "Redirecting\xe2\x80\xa6");
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          view->set_status(Glib::ustring("Redirecting to ") + uri + U'\u2026');
        }
        break;

      case WEBKIT_LOAD_COMMITTED:
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          view->set_status(uri, true);
        }
        break;

      case WEBKIT_LOAD_FINISHED:
        view->set_status(Glib::ustring());
        break;
    }
  }

  static ::gboolean
  on_decide_policy(::WebKitWebView* web_view,
                   ::WebKitPolicyDecision* decision,
                   ::WebKitPolicyDecisionType decision_type,
                   WebView* view)
  {
    const auto window = view->get_main_window();

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
          window)
      {
        window->open_tab(
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

      if (action_type == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED && window)
      {
        window->open_tab(
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
                          WebView* view)
  {
    const auto uri = ::webkit_hit_test_result_get_link_uri(hit_test_result);

    view->set_status(!uri || !*uri ? Glib::ustring() : uri);
  }

  static void
  on_notify_title(::WebKitWebView* web_view, ::GParamSpec*, WebView* view)
  {
    const auto title = ::webkit_web_view_get_title(web_view);

    view->signal_title_changed().emit(
      *view,
      title && *title ? title : "Untitled"
    );
  }

  static void
  on_notify_favicon(::WebKitWebView* web_view, ::GParamSpec*, WebView* view)
  {
    view->signal_favicon_changed().emit(
      *view,
      ::webkit_web_view_get_favicon(web_view)
    );
  }
}
