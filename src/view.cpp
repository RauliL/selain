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
#include <selain/main-window.hpp>
#include <selain/theme.hpp>

#include <format>

namespace selain
{
  static void on_load_changed(
    ::WebKitWebView*,
    ::WebKitLoadEvent,
    View*
  );
  static ::gboolean on_decide_policy(
    ::WebKitWebView*,
    ::WebKitPolicyDecision*,
    ::WebKitPolicyDecisionType,
    View*
  );
  static void on_mouse_target_changed(
    ::WebKitWebView*,
    ::WebKitHitTestResult*,
    ::guint,
    View*
  );
  static void on_notify_title(
    ::WebKitWebView*,
    ::GParamSpec*,
    View*
  );
  static void on_notify_favicon(
    ::WebKitWebView*,
    ::GParamSpec*,
    View*
  );

  namespace keyboard
  {
    ::gboolean on_tab_key_pressed(
      ::GtkEventControllerKey*,
      ::guint keyval,
      ::guint keycode,
      ::GdkModifierType state,
      Tab*
    );

    static ::gboolean
    on_view_key_pressed(::GtkEventControllerKey* controller,
                        ::guint keyval,
                        ::guint keycode,
                        ::GdkModifierType state,
                        View* view)
    {
      const auto tab = view->get_tab();

      if (!tab)
      {
        return FALSE;
      }
      tab->set_active_view(*view);

      return on_tab_key_pressed(controller, keyval, keycode, state, tab);
    }
  }

  View::View(const Glib::RefPtr<WebContext>& context,
             const Glib::RefPtr<WebSettings>& settings)
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , m_web_view(context->create_web_view())
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    const auto key_controller = ::gtk_event_controller_key_new();

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
      G_OBJECT(key_controller),
      "key-pressed",
      G_CALLBACK(keyboard::on_view_key_pressed),
      static_cast<::gpointer>(this)
    );
    ::gtk_widget_add_controller(
      GTK_WIDGET(m_web_view),
      key_controller
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

    m_web_view_widget->set_hexpand(true);
    m_web_view_widget->set_vexpand(true);
    append(*m_web_view_widget.get());

    set_hexpand(true);
    set_vexpand(true);
    add_css_class("selain-view");
    ::webkit_web_view_set_background_color(
      m_web_view,
      theme::window_background.gobj()
    );

    settings->install(m_web_view);
  }

  View::~View()
  {
    if (m_web_view_widget)
    {
      remove(*m_web_view_widget);
      m_web_view_widget.reset();
    }
    m_web_view = nullptr;
  }

  Tab*
  View::get_tab()
  {
    for (Gtk::Widget* widget = get_parent(); widget; widget = widget->get_parent())
    {
      if (auto* tab = dynamic_cast<Tab*>(widget))
      {
        return tab;
      }
    }

    return nullptr;
  }

  const Tab*
  View::get_tab() const
  {
    for (
      const Gtk::Widget* widget = get_parent();
      widget;
      widget = widget->get_parent()
    )
    {
      if (const auto* tab = dynamic_cast<const Tab*>(widget))
      {
        return tab;
      }
    }

    return nullptr;
  }

  Glib::ustring
  View::get_uri() const
  {
    const auto uri = ::webkit_web_view_get_uri(m_web_view);

    if (!uri || !*uri)
    {
      return Glib::ustring();
    }

    return uri;
  }

  Glib::ustring
  View::get_title() const
  {
    const auto title = ::webkit_web_view_get_title(m_web_view);

    if (!title || !*title)
    {
      return Glib::ustring();
    }

    return title;
  }

  void
  View::load_uri(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return;
    }
    if (auto scheme = ::g_uri_parse_scheme(uri.c_str()))
    {
      ::webkit_web_view_load_uri(m_web_view, uri.c_str());
      g_free(scheme);
    } else {
      ::webkit_web_view_load_uri(
        m_web_view,
        std::format("http://{}", uri.raw()).c_str()
      );
    }
  }

  void
  View::reload(bool bypass_cache)
  {
    if (bypass_cache)
    {
      ::webkit_web_view_reload_bypass_cache(m_web_view);
    } else {
      ::webkit_web_view_reload(m_web_view);
    }
  }

  void
  View::stop_loading()
  {
    ::webkit_web_view_stop_loading(m_web_view);
  }

  void
  View::execute_script(const Glib::ustring& script,
                       ::GCancellable* cancellable,
                       ::GAsyncReadyCallback callback,
                       void* user_data)
  {
    ::webkit_web_view_evaluate_javascript(
      m_web_view,
      script.c_str(),
      static_cast<::gssize>(script.bytes()),
      nullptr,
      nullptr,
      cancellable,
      callback,
      static_cast<::gpointer>(user_data)
    );
  }

  void
  View::go_back()
  {
    ::webkit_web_view_go_back(m_web_view);
  }

  void
  View::go_forward()
  {
    ::webkit_web_view_go_forward(m_web_view);
  }

  void
  View::search(const Glib::ustring& text, bool forwards)
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
  View::search_next()
  {
    ::webkit_find_controller_search_next(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  View::search_prev()
  {
    ::webkit_find_controller_search_previous(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  View::grab_focus()
  {
    const auto tab = get_tab();
    const auto window = tab ? tab->get_main_window() : nullptr;

    if (window && window->get_mode() == Mode::COMMAND)
    {
      window->get_command_entry().grab_focus();
    } else {
      m_web_view_widget->grab_focus();
    }
  }

  const Glib::ustring&
  View::get_status() const
  {
    return m_status.empty() ? m_permanent_status : m_status;
  }

  void
  View::set_status(const Glib::ustring& status, bool permanent)
  {
    if (permanent)
    {
      m_permanent_status = status;
      if (m_status.empty())
      {
        if (const auto tab = get_tab())
        {
          tab->on_view_status_changed(*this, status);
        }
      }
    }
    else if (status.empty())
    {
      m_status.clear();
      if (const auto tab = get_tab())
      {
        tab->on_view_status_changed(*this, m_permanent_status);
      }
    } else {
      m_status = status;
      if (const auto tab = get_tab())
      {
        tab->on_view_status_changed(*this, status);
      }
    }
  }

  Glib::RefPtr<Gdk::Paintable>
  View::get_favicon() const
  {
    const auto texture = ::webkit_web_view_get_favicon(m_web_view);

    if (!texture)
    {
      return Glib::RefPtr<Gdk::Paintable>();
    }

    return Glib::wrap(texture, true);
  }

  static void
  on_load_changed(::WebKitWebView* web_view,
                  ::WebKitLoadEvent load_event,
                  View* view)
  {
    const auto tab = view->get_tab();

    switch (load_event)
    {
      case WEBKIT_LOAD_STARTED:
        if (tab && tab->get_active_view() == view)
        {
          tab->get_tab_label().set_text("Loading\xe2\x80\xa6");
        }
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          view->set_status(uri, true);
          view->set_status(std::format("Loading {}…", uri));
        }
        break;

      case WEBKIT_LOAD_REDIRECTED:
        if (tab && tab->get_active_view() == view)
        {
          tab->get_tab_label().set_text("Redirecting\xe2\x80\xa6");
        }
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          view->set_status(std::format("Redirecting to {}…", uri));
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
  on_decide_policy(::WebKitWebView*,
                   ::WebKitPolicyDecision* decision,
                   ::WebKitPolicyDecisionType decision_type,
                   View* view)
  {
    const auto tab = view->get_tab();
    const auto window = tab ? tab->get_main_window() : nullptr;

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
                          View* view)
  {
    const auto uri = ::webkit_hit_test_result_get_link_uri(hit_test_result);

    view->set_status(!uri || !*uri ? Glib::ustring() : uri);
  }

  static void
  on_notify_title(::WebKitWebView*, ::GParamSpec*, View* view)
  {
    if (const auto tab = view->get_tab())
    {
      tab->on_view_title_changed(*view);
    }
  }

  static void
  on_notify_favicon(::WebKitWebView*, ::GParamSpec*, View* view)
  {
    if (const auto tab = view->get_tab())
    {
      tab->on_view_favicon_changed(*view);
    }
  }
}
