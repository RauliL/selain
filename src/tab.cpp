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
#include <selain/utils.hpp>

namespace selain
{
  static void set_webkit_settings(::WebKitSettings*);
  static void set_command_entry_style(const Glib::RefPtr<Gtk::StyleContext>&);
  static void on_load_changed(
    ::WebKitWebView*,
    ::WebKitLoadEvent,
    gpointer
  );
  static gboolean on_decide_policy(
    ::WebKitWebView*,
    ::WebKitPolicyDecision*,
    ::WebKitPolicyDecisionType,
    gpointer
  );

  Tab::Tab()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    , m_mode(Mode::NORMAL)
    , m_web_view(WEBKIT_WEB_VIEW(::webkit_web_view_new()))
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    m_command_entry.override_font(utils::get_monospace_font());

    m_command_entry.signal_key_press_event().connect(sigc::mem_fun(
      this,
      &Tab::on_command_entry_key_press
    ));
    m_command_entry.signal_activate().connect(sigc::mem_fun(
      this,
      &Tab::on_command_received
    ));
    m_web_view_widget->signal_key_press_event().connect(sigc::mem_fun(
      this,
      &Tab::on_web_view_key_press
    ));
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "load-changed",
      G_CALLBACK(on_load_changed),
      static_cast<gpointer>(this)
    );
    ::g_signal_connect(
      G_OBJECT(m_web_view),
      "decide-policy",
      G_CALLBACK(on_decide_policy),
      static_cast<gpointer>(this)
    );

    pack_start(*m_web_view_widget.get());
    pack_start(m_status_bar, Gtk::PACK_SHRINK);
    pack_start(m_command_entry, Gtk::PACK_SHRINK);

    override_background_color(theme::window_background);
    ::webkit_web_view_set_background_color(
      m_web_view,
      theme::window_background.gobj()
    );

    set_webkit_settings(::webkit_web_view_get_settings(m_web_view));
    set_command_entry_style(m_command_entry.get_style_context());
  }

  void
  Tab::set_mode(Mode mode)
  {
    m_mode = mode;
    m_status_bar.set_mode(mode);
    switch (mode)
    {
      case Mode::COMMAND:
        m_command_entry.show();
        m_command_entry.grab_focus_without_selecting();
        m_command_entry.set_position(m_command_entry.get_text().length());
        break;

      case Mode::NORMAL:
      case Mode::INSERT:
        m_command_entry.hide();
        m_command_entry.set_text("");
        m_web_view_widget->grab_focus();
        break;
    }
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
  Tab::execute_script(const Glib::ustring& script)
  {
    ::webkit_web_view_run_javascript(
      m_web_view,
      script.c_str(),
      nullptr,
      nullptr,
      nullptr
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
    if (m_mode == Mode::COMMAND)
    {
      m_command_entry.grab_focus_without_selecting();
      m_command_entry.set_position(m_command_entry.get_text().length());
    } else {
      m_web_view_widget->grab_focus();
    }
  }

  void
  Tab::on_show()
  {
    Gtk::Box::on_show();
    if (m_mode == Mode::NORMAL)
    {
      m_command_entry.hide();
    }
  }

  bool
  Tab::on_command_entry_key_press(::GdkEventKey* event)
  {
    if (event->keyval == GDK_KEY_Escape)
    {
      set_mode(Mode::NORMAL);

      return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
  }

  void
  Tab::on_command_received()
  {
    const auto command = m_command_entry.get_text();

    set_mode(Mode::NORMAL);
    execute_command(command);
  }

  static void
  set_webkit_settings(::WebKitSettings* settings)
  {
    ::webkit_settings_set_enable_java(settings, false);
    ::webkit_settings_set_enable_plugins(settings, false);
    ::webkit_settings_set_user_agent_with_application_details(
      settings,
      "Selain",
      "1.0"
    );
  }

  static void
  set_command_entry_style(const Glib::RefPtr<Gtk::StyleContext>& context)
  {
    context->add_provider(theme::get_command_entry_style_provider(), 1000);
  }

  static void
  on_load_changed(::WebKitWebView* web_view,
                  ::WebKitLoadEvent load_event,
                  gpointer data)
  {
    auto tab = static_cast<Tab*>(data);
    auto main_window = static_cast<MainWindow*>(tab->get_toplevel());
    auto& status_bar = tab->status_bar();

    switch (load_event)
    {
      case WEBKIT_LOAD_STARTED:
        if (main_window)
        {
          main_window->set_tab_title(tab, "Loading...");
        }
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          status_bar.set_permanent_status(uri);
          status_bar.set_status(Glib::ustring("Loading ") + uri + "...");
        }
        break;

      case WEBKIT_LOAD_REDIRECTED:
        if (main_window)
        {
          main_window->set_tab_title(tab, "Redirecting...");
        }
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          status_bar.set_status(
            Glib::ustring("Redirecting to ") + uri + "..."
          );
        }
        break;

      case WEBKIT_LOAD_COMMITTED:
        if (auto uri = ::webkit_web_view_get_uri(web_view))
        {
          status_bar.set_permanent_status(uri);
        }
        break;

      case WEBKIT_LOAD_FINISHED:
      {
        auto title = ::webkit_web_view_get_title(web_view);

        if (!title || !*title)
        {
          title = "Untitled";
        }
        if (main_window)
        {
          main_window->set_tab_title(tab, title);
        }
        break;
      }
    }
  }

  static gboolean
  on_decide_policy(::WebKitWebView* web_view,
                   ::WebKitPolicyDecision* decision,
                   ::WebKitPolicyDecisionType decision_type,
                   gpointer data)
  {
    auto tab = static_cast<Tab*>(data);
    auto main_window = static_cast<MainWindow*>(tab->get_toplevel());

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

    return true;
  }
}
