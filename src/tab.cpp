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
#include <selain/version.hpp>

namespace selain
{
  ::WebKitWebContext* create_web_context();

  static void set_webkit_settings(::WebKitSettings*);
  static void on_load_changed(
    ::WebKitWebView*,
    ::WebKitLoadEvent,
    Tab*
  );
  static ::gboolean on_decide_policy(
    ::WebKitWebView*,
    ::WebKitPolicyDecision*,
    ::WebKitPolicyDecisionType,
    Tab*
  );
  static void on_mouse_target_changed(
    ::WebKitWebView*,
    ::WebKitHitTestResult*,
    ::guint,
    Tab*
  );
  static void on_notify_title(
    ::WebKitWebView*,
    ::GParamSpec*,
    Tab*
  );
  static void on_notify_favicon(
    ::WebKitWebView*,
    ::GParamSpec*,
    Tab*
  );

  namespace keyboard
  {
    /**
     * GTK signal callback used for key press events inside web view of a tab.
     */
    ::gboolean on_tab_key_press(::WebKitWebView*, ::GdkEventKey*, Tab*);
  }

  Tab::Tab()
    : m_web_view(WEBKIT_WEB_VIEW(::webkit_web_view_new_with_context(
        create_web_context()
      )))
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    m_tab_label.signal_close_button_clicked().connect(sigc::mem_fun(
      this,
      &Tab::on_close_button_clicked
    ));
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

    add(*m_web_view_widget.get());

    override_background_color(theme::window_background);
    ::webkit_web_view_set_background_color(
      m_web_view,
      theme::window_background.gobj()
    );

    set_webkit_settings(::webkit_web_view_get_settings(m_web_view));
  }

  void
  Tab::set_hint_context(const Glib::RefPtr<HintContext>& hint_context)
  {
    if (m_hint_context)
    {
      m_hint_context->uninstall(*this);
      m_hint_context.reset();
    }
    if ((m_hint_context = hint_context))
    {
      m_hint_context->install(*this);
    }
  }

  MainWindow*
  Tab::get_main_window()
  {
    const auto container = get_toplevel();

    return container ? static_cast<MainWindow*>(container) : nullptr;
  }

  const MainWindow*
  Tab::get_main_window() const
  {
    const auto container = get_toplevel();

    return container ? static_cast<const MainWindow*>(container) : nullptr;
  }

  Glib::ustring
  Tab::get_uri() const
  {
    const auto uri = ::webkit_web_view_get_uri(m_web_view);

    if (!uri || !*uri)
    {
      return Glib::ustring();
    }

    return uri;
  }

  void
  Tab::load_uri(const Glib::ustring& uri)
  {
    if (uri.empty())
    {
      return;
    }
    ::webkit_web_view_load_uri(
      m_web_view,
      (
        uri.find("://") == Glib::ustring::npos
          ? "http://" + uri
          : uri
      ).c_str()
    );
  }

  void
  Tab::reload(bool bypass_cache)
  {
    if (bypass_cache)
    {
      ::webkit_web_view_reload_bypass_cache(m_web_view);
    } else {
      ::webkit_web_view_reload(m_web_view);
    }
  }

  void
  Tab::stop_loading()
  {
    ::webkit_web_view_stop_loading(m_web_view);
  }

  void
  Tab::execute_script(const Glib::ustring& script,
                      ::GCancellable* cancellable,
                      ::GAsyncReadyCallback callback,
                      void* user_data)
  {
    ::webkit_web_view_run_javascript(
      m_web_view,
      script.c_str(),
      cancellable,
      callback,
      static_cast<::gpointer>(user_data)
    );
  }

  void
  Tab::go_back()
  {
    ::webkit_web_view_go_back(m_web_view);
  }

  void
  Tab::go_forward()
  {
    ::webkit_web_view_go_forward(m_web_view);
  }

  void
  Tab::search(const Glib::ustring& text, bool forwards)
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
  Tab::search_next()
  {
    ::webkit_find_controller_search_next(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  Tab::search_prev()
  {
    ::webkit_find_controller_search_previous(
      ::webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  Tab::grab_focus()
  {
    const auto window = get_main_window();

    if (window && window->get_mode() == Mode::COMMAND)
    {
      window->get_command_entry().grab_focus();
    } else {
      m_web_view_widget->grab_focus();
    }
  }

  const Glib::ustring&
  Tab::get_status() const
  {
    return m_status.empty() ? m_permanent_status : m_status;
  }

  void
  Tab::set_status(const Glib::ustring& status, bool permanent)
  {
    if (permanent)
    {
      m_permanent_status = status;
      if (m_status.empty())
      {
        m_signal_status_changed.emit(this, status);
      }
    }
    else if (status.empty())
    {
      m_status.clear();
      m_signal_status_changed.emit(this, m_permanent_status);
    } else {
      m_status = status;
      m_signal_status_changed.emit(this, status);
    }
  }

  void
  Tab::on_close_button_clicked()
  {
    if (const auto window = get_main_window())
    {
      window->close_tab(*this);
    }
  }

  // TODO: Create shared instance of `WebKitSettings` during application
  // startup and use that instead.
  static void
  set_webkit_settings(::WebKitSettings* settings)
  {
    ::webkit_settings_set_enable_java(settings, false);
    ::webkit_settings_set_enable_plugins(settings, false);
    ::webkit_settings_set_enable_developer_extras(settings, true);
    ::webkit_settings_set_user_agent_with_application_details(
      settings,
      "Selain",
      SELAIN_VERSION
    );
  }

  static void
  on_load_changed(::WebKitWebView* web_view,
                  ::WebKitLoadEvent load_event,
                  Tab* tab)
  {
    switch (load_event)
    {
      case WEBKIT_LOAD_STARTED:
        tab->get_tab_label().set_text("Loading\xe2\x80\xa6");
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          tab->set_status(uri, true);
          tab->set_status(Glib::ustring("Loading ") + uri + U'\u2026');
        }
        break;

      case WEBKIT_LOAD_REDIRECTED:
        tab->get_tab_label().set_text("Redirecting\xe2\x80\xa6");
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          tab->set_status(Glib::ustring("Redirecting to ") + uri + U'\u2026');
        }
        break;

      case WEBKIT_LOAD_COMMITTED:
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          tab->set_status(uri, true);
        }
        break;

      case WEBKIT_LOAD_FINISHED:
        tab->set_status(Glib::ustring());
        break;
    }
  }

  static ::gboolean
  on_decide_policy(::WebKitWebView* web_view,
                   ::WebKitPolicyDecision* decision,
                   ::WebKitPolicyDecisionType decision_type,
                   Tab* tab)
  {
    const auto window = tab->get_main_window();

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
                          Tab* tab)
  {
    const auto uri = ::webkit_hit_test_result_get_link_uri(hit_test_result);

    tab->set_status(!uri || !*uri ? Glib::ustring() : uri);
  }

  static void
  on_notify_title(::WebKitWebView* web_view, ::GParamSpec*, Tab* tab)
  {
    const auto title = ::webkit_web_view_get_title(web_view);

    tab->get_tab_label().set_text(title && *title ? title : "Untitled");
  }

  static void
  on_notify_favicon(::WebKitWebView* web_view, ::GParamSpec*, Tab* tab)
  {
    auto& tab_label = tab->get_tab_label();
    const auto surface = ::webkit_web_view_get_favicon(web_view);
    int width;
    int height;

    if (surface && Gtk::IconSize::lookup(Gtk::ICON_SIZE_BUTTON, width, height))
    {
      const auto pixbuf = Gdk::Pixbuf::create(
        Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(surface)),
        0,
        0,
        ::cairo_image_surface_get_width(surface),
        ::cairo_image_surface_get_height(surface)
      );

      tab_label.set_icon(pixbuf->scale_simple(
        width,
        height,
        Gdk::INTERP_BILINEAR
      ));
    } else {
      tab_label.set_icon(Glib::RefPtr<Gdk::Pixbuf>());
    }
  }
}
