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
#include <algorithm>

#include <selain/config.hpp>

#if defined(SELAIN_ENABLE_BUKU_INTEGRATION)
# include <selain/integration/buku.hpp>
#endif
#if defined(SELAIN_ENABLE_PASS_INTEGRATION)
# include <selain/integration/pass.hpp>
#endif
#include <selain/ui/main-window.hpp>
#include <selain/ui/view.hpp>

namespace selain::ui
{
  static constexpr int DEFAULT_WIDTH = 640;
  static constexpr int DEFAULT_HEIGHT = 480;

  MainWindow::MainWindow(const Glib::RefPtr<Gtk::Application>& application)
    : Gtk::ApplicationWindow(application)
    , m_web_context(webkit::Context::create())
    , m_web_settings(webkit::Settings::create())
    , m_mode(Mode::NORMAL)
    , m_box(Gtk::Orientation::VERTICAL)
    , m_command_entry_key_controller(Gtk::EventControllerKey::create())
  {
#if defined(SELAIN_ENABLE_BUKU_INTEGRATION)
    m_bookmark_manager = integration::buku::BookmarkManager::create();
#endif
#if defined(SELAIN_ENABLE_PASS_INTEGRATION)
    m_password_manager = integration::pass::PasswordManager::create();
#endif
    m_history_manager = HistoryManager::create();

    set_title("Selain");
    set_icon_name("selain");
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    m_notebook.set_hexpand(true);
    m_notebook.set_vexpand(true);
    m_notebook.set_scrollable(true);
    m_box.append(m_notebook);
    m_box.append(m_status_bar);
    m_box.append(m_command_entry);
    set_child(m_box);

    m_notebook.signal_switch_page().connect(sigc::mem_fun(
      *this,
      &MainWindow::on_tab_switch
    ));
    m_command_entry_key_controller->signal_key_pressed().connect(
      sigc::mem_fun(*this, &MainWindow::on_command_entry_key_pressed),
      false
    );
    m_command_entry.add_controller(m_command_entry_key_controller);
    m_command_entry.signal_command_received().connect(sigc::mem_fun(
      *this,
      &MainWindow::on_command_received
    ));

    add_css_class("selain-window");

    maximize();
  }

  MainWindow::~MainWindow()
  {
    while (m_notebook.get_n_pages() > 0)
    {
      if (auto* tab = get_nth_tab(0))
      {
        tab->prepare_close();
      }
      m_notebook.remove_page(0);
    }
    while (auto* child = m_box.get_first_child())
    {
      m_box.remove(*child);
    }
  }

  void
  MainWindow::schedule_close()
  {
    Glib::signal_idle().connect_once([this]() {
      // Destroy WebKit views while the window is still fully alive; tearing
      // them down from ~MainWindow is too late and trips GObject criticals.
      for (int i = 0; i < m_notebook.get_n_pages(); ++i)
      {
        if (auto* tab = get_nth_tab(i))
        {
          tab->prepare_close();
        }
      }
      close();
    });
  }

  void
  MainWindow::set_mode(Mode mode)
  {
    const auto tab = get_current_tab();

    if ((m_mode == Mode::HINT || m_mode == Mode::HINT_NEW_TAB) && tab)
    {
      if (auto& context = tab->get_hint_context())
      {
        context->uninstall(*tab);
        context.reset();
      }
    }
    if (m_mode == Mode::PROMPT && mode != Mode::PROMPT)
    {
      clear_prompt_ui();
      m_prompt.reset();
    }
    m_status_bar.set_mode(mode);
    switch (m_mode = mode)
    {
      case Mode::COMMAND:
        m_command_entry.set_visibility(true);
        m_command_entry.set_placeholder_text(Glib::ustring());
        m_command_entry.grab_focus();
        break;

      case Mode::PROMPT:
        m_command_entry.grab_focus();
        break;

      case Mode::HINT:
      case Mode::HINT_NEW_TAB:
        if (tab)
        {
          tab->set_hint_context(webkit::HintContext::create(
            m_mode == Mode::HINT_NEW_TAB
          ));
        }

      default:
        m_command_entry.set_text(Glib::ustring());
        m_command_entry.set_visibility(true);
        m_command_entry.set_placeholder_text(Glib::ustring());
        if (tab)
        {
          tab->grab_focus();
        }
        break;
    }
  }

  void
  MainWindow::prompt(
    View* view,
    const Glib::ustring& message,
    bool sensitive,
    prompt_callback_type on_submit
  )
  {
    if (!view || !on_submit)
    {
      return;
    }

    m_prompt = PromptRequest{
      .view = view,
      .message = message,
      .sensitive = sensitive,
      .on_submit = std::move(on_submit)
    };
    m_command_entry.set_text(Glib::ustring());
    m_command_entry.set_visibility(!sensitive);
    m_command_entry.set_placeholder_text(
      sensitive
        ? "Sensitive input (Enter to submit, Esc to cancel)"
        : "Input (Enter to submit, Esc to cancel)"
    );
    set_mode(Mode::PROMPT);
    m_status_bar.set_status(
      message.empty() ? "Input required" : message
    );
  }

  void
  MainWindow::cancel_prompt_for(View* view)
  {
    if (!m_prompt || m_prompt->view != view)
    {
      return;
    }

    m_prompt.reset();
    if (m_mode == Mode::PROMPT)
    {
      set_mode(Mode::NORMAL);
    }
  }

  void
  MainWindow::clear_prompt_ui()
  {
    m_command_entry.set_visibility(true);
    m_command_entry.set_placeholder_text(Glib::ustring());
    m_command_entry.set_text(Glib::ustring());
  }

  void
  MainWindow::complete_prompt(const Glib::ustring& text)
  {
    auto request = std::move(m_prompt);

    m_prompt.reset();
    clear_prompt_ui();
    set_mode(Mode::NORMAL);

    if (!request || !request->on_submit)
    {
      return;
    }

    request->on_submit(text);
  }

  std::vector<Tab*>
  MainWindow::get_tabs()
  {
    std::vector<Tab*> result;
    const auto count = get_tab_count();

    result.reserve(count);
    for (int i = 0; i < count; ++i)
    {
      if (auto* tab = get_nth_tab(i))
      {
        result.push_back(tab);
      }
    }

    return result;
  }

  std::vector<const Tab*>
  MainWindow::get_tabs() const
  {
    std::vector<const Tab*> result;
    const auto count = get_tab_count();

    result.reserve(count);
    for (int i = 0; i < count; ++i)
    {
      if (const auto* tab = get_nth_tab(i))
      {
        result.push_back(tab);
      }
    }

    return result;
  }

  Tab*
  MainWindow::get_current_tab()
  {
    const auto index = m_notebook.get_current_page();

    return index >= 0 ? get_nth_tab(index) : nullptr;
  }

  const Tab*
  MainWindow::get_current_tab() const
  {
    const auto index = m_notebook.get_current_page();

    return index >= 0 ? get_nth_tab(index) : nullptr;
  }

  Tab*
  MainWindow::get_nth_tab(int index)
  {
    const auto widget = m_notebook.get_nth_page(index);

    return widget ? static_cast<Tab*>(widget) : nullptr;
  }

  const Tab*
  MainWindow::get_nth_tab(int index) const
  {
    const auto widget = m_notebook.get_nth_page(index);

    return widget ? static_cast<const Tab*>(widget) : nullptr;
  }

  Glib::RefPtr<Tab>
  MainWindow::open_tab(const Glib::ustring& uri, bool focus)
  {
    const auto tab = Glib::make_refptr_for_instance<Tab>(
      new Tab(m_web_context, m_web_settings)
    );
    const auto scroll_controller = Gtk::EventControllerScroll::create();

    m_notebook.append_page(*tab.get(), tab->get_tab_label());
    m_notebook.get_page(*tab.get())->property_tab_expand() = true;
    scroll_controller->set_flags(
      Gtk::EventControllerScroll::Flags::VERTICAL |
      Gtk::EventControllerScroll::Flags::DISCRETE
    );
    scroll_controller->signal_scroll().connect(
      sigc::mem_fun(*this, &MainWindow::on_tab_list_scroll),
      false
    );
    tab->get_tab_label().add_controller(scroll_controller);
    tab->signal_status_changed().connect(sigc::mem_fun(
      *this,
      &MainWindow::on_tab_status_change
    ));
    if (!uri.empty())
    {
      tab->load_uri(uri);
    }
    if (focus)
    {
      set_current_tab(tab);
      tab->grab_focus();
    }

    return tab;
  }

  void
  MainWindow::close_tab(Tab& tab)
  {
    const auto index = m_notebook.page_num(tab);

    if (index < 0)
    {
      return;
    }
    // Last tab: defer window close so we do not destroy MainWindow while still
    // inside a command/signal handler (e.g. :q).
    if (m_notebook.get_n_pages() <= 1)
    {
      schedule_close();
      return;
    }
    tab.prepare_close();
    m_notebook.remove_page(index);
    if (const auto current = get_current_tab())
    {
      current->grab_focus();
    }
  }

  void
  MainWindow::close_tab(const Glib::RefPtr<Tab>& tab)
  {
    if (tab)
    {
      close_tab(*tab.get());
    }
  }

  void
  MainWindow::set_current_tab(const Glib::RefPtr<Tab>& tab)
  {
    set_current_tab(m_notebook.page_num(*tab.get()));
  }

  void
  MainWindow::set_current_tab(int index)
  {
    if (index >= 0 && index < m_notebook.get_n_pages())
    {
      m_notebook.set_current_page(index);
    }
  }

  void
  MainWindow::next_tab()
  {
    m_notebook.next_page();
  }

  void
  MainWindow::prev_tab()
  {
    m_notebook.prev_page();
  }

  int
  MainWindow::get_tab_count() const
  {
    return m_notebook.get_n_pages();
  }

  void
  MainWindow::focus_view(View& view)
  {
    auto* tab = view.get_tab();

    if (!tab)
    {
      return;
    }

    const auto index = m_notebook.page_num(*tab);

    if (index < 0)
    {
      return;
    }

    set_current_tab(index);
    if (tab->get_active_view() == &view)
    {
      view.grab_focus();
    } else {
      tab->set_active_view(view);
    }
  }

  std::vector<Glib::ustring>
  MainWindow::get_history_uris() const
  {
    std::vector<Glib::ustring> uris;

    for (int i = 0; i < m_notebook.get_n_pages(); ++i)
    {
      if (const auto* tab = get_nth_tab(i))
      {
        tab->collect_history_uris(uris);
      }
    }
    if (m_history_manager)
    {
      const auto persisted = m_history_manager->list_uris();

      uris.insert(std::end(uris), std::begin(persisted), std::end(persisted));
    }

    std::sort(std::begin(uris), std::end(uris));
    uris.erase(std::unique(std::begin(uris), std::end(uris)), std::end(uris));

    return uris;
  }

  bool
  MainWindow::on_command_entry_key_pressed(guint keyval,
                                           guint,
                                           Gdk::ModifierType state)
  {
    if (keyval == GDK_KEY_Escape)
    {
      m_tab_completion.reset();
      set_mode(Mode::NORMAL);

      return true;
    }

    // Prompt input is free-form text; do not run tab completion.
    if (m_mode == Mode::PROMPT)
    {
      return false;
    }

    const bool shift = static_cast<bool>(
      state & static_cast<Gdk::ModifierType>(GDK_SHIFT_MASK)
    );

    if (keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab)
    {
      m_tab_completion.complete(
        *this,
        keyval == GDK_KEY_ISO_Left_Tab || (keyval == GDK_KEY_Tab && shift)
      );

      return true;
    }

    // Typing anything else starts a fresh completion next time Tab is pressed.
    m_tab_completion.reset();

    return false;
  }

  void
  MainWindow::on_command_received(const Glib::ustring& command)
  {
    if (m_mode == Mode::PROMPT || m_prompt)
    {
      complete_prompt(command);

      return;
    }

    set_mode(Mode::NORMAL);
    run_command(
      command,
      true,
      [&](const std::optional<Glib::ustring>& error)
      {
        if (error)
        {
          show_error(*error);
        }
      }
    );
  }

  void
  MainWindow::on_tab_status_change(Tab* tab, const Glib::ustring& status)
  {
    const auto current_index = m_notebook.get_current_page();
    const auto tab_index = m_notebook.page_num(*tab);

    if (current_index >= 0 && tab_index >= 0 && current_index == tab_index)
    {
      m_status_bar.set_status(status);
    }
  }

  void
  MainWindow::on_tab_switch(Gtk::Widget* widget, guint)
  {
    if (widget)
    {
      m_status_bar.set_status(static_cast<Tab*>(widget)->get_status());
    } else {
      m_status_bar.set_status(Glib::ustring());
    }
    // Notebook page changes leave focus on the tab bar (after this signal).
    // Restore it on the web view so J/K and other shortcuts keep working.
    Glib::signal_idle().connect_once(
      [this]()
      {
        if (const auto tab = get_current_tab())
        {
          tab->grab_focus();
        }
      }
    );
  }

  bool
  MainWindow::on_tab_list_scroll(double, double dy)
  {
    if (dy > 0.0)
    {
      next_tab();
    }
    else if (dy < 0.0)
    {
      prev_tab();
    }

    return true;
  }
}
