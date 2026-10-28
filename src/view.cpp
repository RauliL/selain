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

#include <cstring>
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
  static ::gboolean on_web_process_terminated(
    ::WebKitWebView*,
    ::WebKitWebProcessTerminationReason,
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

  static constexpr const char* view_quark = "selain-view";

  View::View(const Glib::RefPtr<WebContext>& context,
             const Glib::RefPtr<WebSettings>& settings)
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , m_web_view(context->create_web_view())
  {
    const auto key_controller = ::gtk_event_controller_key_new();

    ::g_object_set_data(G_OBJECT(m_web_view), view_quark, this);
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
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "web-process-terminated",
      G_CALLBACK(on_web_process_terminated),
      static_cast<::gpointer>(this)
    );

    // append() sinks the floating ref from create_web_view(); the box owns it.
    ::gtk_widget_set_hexpand(GTK_WIDGET(m_web_view), TRUE);
    ::gtk_widget_set_vexpand(GTK_WIDGET(m_web_view), TRUE);
    ::gtk_box_append(GTK_BOX(gobj()), GTK_WIDGET(m_web_view));

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
    prepare_close();
  }

  void
  View::prepare_close()
  {
    if (!m_web_view)
    {
      return;
    }

    if (const auto tab = get_tab())
    {
      if (const auto window = tab->get_main_window())
      {
        window->cancel_gemini_input_for(this);
      }
    }
    ::g_object_set_data(G_OBJECT(m_web_view), view_quark, nullptr);
    ::g_signal_handlers_disconnect_by_data(m_web_view, this);
    if (::gtk_widget_get_parent(GTK_WIDGET(m_web_view)) == GTK_WIDGET(gobj()))
    {
      // Drops the box's ref; web view is destroyed with that last ref.
      ::gtk_box_remove(GTK_BOX(gobj()), GTK_WIDGET(m_web_view));
    }
    m_web_view = nullptr;
  }

  View*
  View::from_web_view(::WebKitWebView* web_view)
  {
    if (!web_view)
    {
      return nullptr;
    }

    return static_cast<View*>(
      ::g_object_get_data(G_OBJECT(web_view), view_quark)
    );
  }

  void
  View::request_gemini_input(
    const Glib::ustring& uri,
    const Glib::ustring& prompt,
    bool sensitive
  )
  {
    const auto tab = get_tab();
    const auto window = tab ? tab->get_main_window() : nullptr;

    if (!window)
    {
      return;
    }

    window->prompt_gemini_input(this, uri, prompt, sensitive);
  }

  bool
  View::consume_gemini_redirect_hop()
  {
    constexpr int max_gemini_redirects = 5;

    if (m_gemini_redirect_hops >= max_gemini_redirects)
    {
      return false;
    }

    ++m_gemini_redirect_hops;

    return true;
  }

  void
  View::reset_gemini_redirect_hops()
  {
    m_gemini_redirect_hops = 0;
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
  View::collect_history_uris(std::vector<Glib::ustring>& uris) const
  {
    const auto append_item = [&uris](::WebKitBackForwardListItem* item) {
      if (!item)
      {
        return;
      }
      if (const auto* uri = ::webkit_back_forward_list_item_get_uri(item))
      {
        if (*uri)
        {
          uris.emplace_back(uri);
        }
      }
    };

    auto* list = ::webkit_web_view_get_back_forward_list(m_web_view);

    if (!list)
    {
      return;
    }

    append_item(::webkit_back_forward_list_get_current_item(list));

    if (auto* back = ::webkit_back_forward_list_get_back_list(list))
    {
      for (auto* node = back; node; node = node->next)
      {
        append_item(WEBKIT_BACK_FORWARD_LIST_ITEM(node->data));
      }
      g_list_free(back);
    }
    if (auto* forward = ::webkit_back_forward_list_get_forward_list(list))
    {
      for (auto* node = forward; node; node = node->next)
      {
        append_item(WEBKIT_BACK_FORWARD_LIST_ITEM(node->data));
      }
      g_list_free(forward);
    }
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
  View::toggle_inspector()
  {
    if (auto* inspector = ::webkit_web_view_get_inspector(m_web_view))
    {
      if (::webkit_web_inspector_is_attached(inspector))
      {
        ::webkit_web_inspector_close(inspector);
      } else {
        ::webkit_web_inspector_show(inspector);
      }
    }
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

    if (window
        && (window->get_mode() == Mode::COMMAND
            || window->get_mode() == Mode::GEMINI_INPUT))
    {
      window->get_command_entry().grab_focus();
    }
    else if (m_web_view)
    {
      ::gtk_widget_grab_focus(GTK_WIDGET(m_web_view));
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
  open_uri_from_action(
    MainWindow* window,
    ::WebKitPolicyDecision* decision,
    ::WebKitNavigationAction* action
  )
  {
    const auto uri = ::webkit_uri_request_get_uri(
      ::webkit_navigation_action_get_request(action)
    );
    // Copy before ignore(): the request (and this pointer) may be released
    // once the policy decision is completed.
    const Glib::ustring open_uri = (uri && *uri) ? uri : Glib::ustring();

    // Resolve the policy before creating another view; loading from inside
    // this callback can deadlock the web process.
    ::webkit_policy_decision_ignore(decision);

    if (window && !open_uri.empty())
    {
      Glib::signal_idle().connect_once([window, open_uri]() {
        window->open_tab(open_uri, false);
      });
    }

    return TRUE;
  }

  static ::gboolean
  on_decide_policy(::WebKitWebView*,
                   ::WebKitPolicyDecision* decision,
                   ::WebKitPolicyDecisionType decision_type,
                   View* view)
  {
    const auto tab = view->get_tab();
    const auto window = tab ? tab->get_main_window() : nullptr;

    if (!window)
    {
      return FALSE;
    }

    if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
        decision_type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
    {
      // RESPONSE etc.: let WebKit's default MIME / download handling run.
      return FALSE;
    }

    auto navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
    auto action = ::webkit_navigation_policy_decision_get_navigation_action(
      navigation_decision
    );
    const auto action_type = ::webkit_navigation_action_get_navigation_type(
      action
    );

    // Middle-click: open in a background tab.
    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
        ::webkit_navigation_action_get_mouse_button(action) == 2 &&
        action_type == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED)
    {
      return open_uri_from_action(window, decision, action);
    }

    // target=_blank / window.open() with a concrete URL: open as a tab.
    // Do not intercept empty/about:blank popups — those need a related-view
    // via the "create" signal, and ignoring them can break sites like Google.
    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
    {
      const auto uri = ::webkit_uri_request_get_uri(
        ::webkit_navigation_action_get_request(action)
      );

      if (uri && *uri && std::strncmp(uri, "about:", 6) != 0)
      {
        return open_uri_from_action(window, decision, action);
      }
    }

    return FALSE;
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

  static ::gboolean
  on_web_process_terminated(::WebKitWebView*,
                            ::WebKitWebProcessTerminationReason,
                            View* view)
  {
    view->set_status("Web process crashed", true);
    if (const auto tab = view->get_tab())
    {
      if (const auto window = tab->get_main_window())
      {
        window->show_error("Web process crashed.");
      }
    }

    return FALSE;
  }
}
