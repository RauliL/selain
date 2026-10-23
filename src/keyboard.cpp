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
#include <selain/keyboard.hpp>
#include <selain/main-window.hpp>

#include <chrono>
#include <format>

namespace selain
{
  static const int keypress_timeout = 2;
  static std::shared_ptr<keyboard::Mapping> top_mapping;

  static ::gboolean key_event_normal_mode(
    MainWindow&,
    Tab&,
    ::guint keyval,
    ::GdkModifierType state
  );
  static ::gboolean key_event_insert_mode(MainWindow&, ::guint keyval);
  static ::gboolean key_event_hint_mode(
    MainWindow&,
    Tab&,
    ::guint keyval,
    ::GdkModifierType state
  );
  static void add_mapping(
    const std::u32string&,
    const keyboard::BindingCallback&
  );

  static void bind_mode_command(MainWindow&, Tab&);
  static void bind_mode_insert(MainWindow&, Tab&);
  static void bind_mode_hint(MainWindow&, Tab&);
  static void bind_mode_hint_new_tab(MainWindow&, Tab&);
  static void bind_tab_reload(MainWindow&, Tab&);
  static void bind_tab_reload_bypass_cache(MainWindow&, Tab&);
  static void bind_tab_open(MainWindow&, Tab&);
  static void bind_tab_close(MainWindow&, Tab&);
  static void bind_tab_prev(MainWindow&, Tab&);
  static void bind_tab_next(MainWindow&, Tab&);
  static void bind_split(MainWindow&, Tab&);
  static void bind_vsplit(MainWindow&, Tab&);
  static void bind_split_close(MainWindow&, Tab&);
  static void bind_split_only(MainWindow&, Tab&);
  static void bind_split_focus_left(MainWindow&, Tab&);
  static void bind_split_focus_down(MainWindow&, Tab&);
  static void bind_split_focus_up(MainWindow&, Tab&);
  static void bind_split_focus_right(MainWindow&, Tab&);
  static void bind_split_cycle(MainWindow&, Tab&);
  static void bind_split_cycle_prev(MainWindow&, Tab&);
  static void bind_scroll_left(MainWindow&, Tab&);
  static void bind_scroll_down(MainWindow&, Tab&);
  static void bind_scroll_up(MainWindow&, Tab&);
  static void bind_scroll_page_down(MainWindow&, Tab&);
  static void bind_scroll_page_up(MainWindow&, Tab&);
  static void bind_scroll_right(MainWindow&, Tab&);
  static void bind_scroll_top(MainWindow&, Tab&);
  static void bind_scroll_bottom(MainWindow&, Tab&);
  static void bind_history_prev(MainWindow&, Tab&);
  static void bind_history_next(MainWindow&, Tab&);
  static void bind_complete_open(MainWindow&, Tab&);
  static void bind_complete_open_tab(MainWindow&, Tab&);
  static void bind_paste(MainWindow&, Tab&);
  static void bind_paste_open_tab(MainWindow&, Tab&);
  static void bind_yank(MainWindow&, Tab&);
  static void bind_search_forwards(MainWindow&, Tab&);
  static void bind_search_backwards(MainWindow&, Tab&);
  static void bind_search_next(MainWindow&, Tab&);
  static void bind_search_prev(MainWindow&, Tab&);

  namespace keyboard
  {
    void
    initialize()
    {
      // Switching between modes.
      add_mapping(U":", bind_mode_command);
      add_mapping(U"i", bind_mode_insert);
      add_mapping(U"f", bind_mode_hint);
      add_mapping(U"F", bind_mode_hint_new_tab);

      // Tab management.
      add_mapping(U"r", bind_tab_reload);
      add_mapping(U"R", bind_tab_reload_bypass_cache);
      add_mapping(U"t", bind_tab_open);
      add_mapping(U"x", bind_tab_close);
      add_mapping(U"J", bind_tab_prev);
      add_mapping(U"K", bind_tab_next);

      // Split management (Vim Ctrl-W).
      add_mapping(U"^ws", bind_split);
      add_mapping(U"^wS", bind_split);
      add_mapping(U"^wv", bind_vsplit);
      add_mapping(U"^wq", bind_split_close);
      add_mapping(U"^wc", bind_split_close);
      add_mapping(U"^wo", bind_split_only);
      add_mapping(U"^wh", bind_split_focus_left);
      add_mapping(U"^wj", bind_split_focus_down);
      add_mapping(U"^wk", bind_split_focus_up);
      add_mapping(U"^wl", bind_split_focus_right);
      add_mapping(U"^ww", bind_split_cycle);
      add_mapping(U"^w^w", bind_split_cycle);
      add_mapping(U"^wW", bind_split_cycle_prev);

      // Navigation.
      add_mapping(U"d", bind_scroll_page_down);
      add_mapping(U"h", bind_scroll_left);
      add_mapping(U"j", bind_scroll_down);
      add_mapping(U"k", bind_scroll_up);
      add_mapping(U"l", bind_scroll_right);
      add_mapping(U"gg", bind_scroll_top);
      add_mapping(U"G", bind_scroll_bottom);
      add_mapping(U"H", bind_history_prev);
      add_mapping(U"L", bind_history_next);
      add_mapping(U"o", bind_complete_open);
      add_mapping(U"O", bind_complete_open_tab);
      add_mapping(U"p", bind_paste);
      add_mapping(U"P", bind_paste_open_tab);
      add_mapping(U"u", bind_scroll_page_up);
      add_mapping(U"yy", bind_yank);

      // Searching.
      add_mapping(U"/", bind_search_forwards);
      add_mapping(U"?", bind_search_backwards);
      add_mapping(U"n", bind_search_next);
      add_mapping(U"N", bind_search_prev);
    }

    ::gboolean
    on_tab_key_pressed(::GtkEventControllerKey*,
                       ::guint keyval,
                       ::guint,
                       ::GdkModifierType state,
                       Tab* tab)
    {
      const auto window = tab->get_main_window();

      if (!window)
      {
        return FALSE;
      }
      switch (window->get_mode())
      {
        case Mode::NORMAL:
          return key_event_normal_mode(*window, *tab, keyval, state);

        case Mode::INSERT:
          return key_event_insert_mode(*window, keyval);

        case Mode::HINT:
        case Mode::HINT_NEW_TAB:
          return key_event_hint_mode(*window, *tab, keyval, state);

        default:
          return FALSE;
      }
    }
  }

  static ::gboolean
  key_event_normal_mode(MainWindow& window,
                        Tab& tab,
                        ::guint keyval,
                        ::GdkModifierType state)
  {
    static std::shared_ptr<keyboard::Mapping> last_mapping;
    static std::chrono::time_point<std::chrono::system_clock> last_keypress;
    const bool is_control = state & GDK_CONTROL_MASK;
    const auto now = std::chrono::system_clock::now();
    keyboard::Mapping::mapping_type::const_iterator entry;

    if (last_mapping)
    {
      const auto difference = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_keypress
      ).count();

      if (difference >= keypress_timeout)
      {
        last_mapping.reset();
      }
    }

    if (!last_mapping)
    {
      last_mapping = top_mapping;
    }
    last_keypress = now;

    if (is_control)
    {
      entry = last_mapping->control_mapping.find(keyval);
      if (entry == std::end(last_mapping->control_mapping))
      {
        return TRUE;
      }
    } else {
      entry = last_mapping->mapping.find(keyval);
      if (entry == std::end(last_mapping->mapping))
      {
        return TRUE;
      }
    }

    if (entry->second->callback)
    {
      entry->second->callback(window, tab);
      last_mapping.reset();
    }
    else if (!entry->second->mapping.empty() ||
             !entry->second->control_mapping.empty())
    {
      last_mapping = entry->second;
    }

    return TRUE;
  }

  static ::gboolean
  key_event_insert_mode(MainWindow& window, ::guint keyval)
  {
    if (keyval == GDK_KEY_Escape)
    {
      window.set_mode(Mode::NORMAL);

      return TRUE;
    }

    return FALSE;
  }

  static ::gboolean
  key_event_hint_mode(MainWindow& window,
                      Tab& tab,
                      ::guint keyval,
                      ::GdkModifierType state)
  {
    const auto& context = tab.get_hint_context();

    if (keyval == GDK_KEY_Escape)
    {
      if (context)
      {
        context->uninstall(tab);
      }
      window.set_mode(Mode::NORMAL);
    }
    else if (keyval == GDK_KEY_BackSpace)
    {
      if (context)
      {
        context->remove_char(tab);
      }
    }
    else if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
    {
      if (context)
      {
        context->activate_current_match(tab);
      }
    }
    else if (!(state & GDK_CONTROL_MASK))
    {
      const auto c = ::gdk_keyval_to_unicode(keyval);

      if (context && std::isalnum(c))
      {
        context->add_char(tab, c);
      }
    }

    return TRUE;
  }

  static void
  add_mapping(const std::u32string& sequence,
              const keyboard::BindingCallback& callback)
  {
    const auto length = sequence.length();
    std::shared_ptr<keyboard::Mapping> current_mapping;

    if (!length)
    {
      return;
    }

    if (!top_mapping)
    {
      top_mapping = std::make_shared<keyboard::Mapping>();
    }
    current_mapping = top_mapping;

    for (std::u32string::size_type i = 0; i < length; ++i)
    {
      bool is_control = false;

      if (sequence[i] == U'^' && i + 1 < length)
      {
        ++i;
        is_control = true;
      }

      const auto c = ::gdk_unicode_to_keyval(sequence[i]);
      auto& map = is_control
        ? current_mapping->control_mapping
        : current_mapping->mapping;
      auto entry = map.find(c);

      if (entry == std::end(map))
      {
        auto next_mapping = std::make_shared<keyboard::Mapping>();

        next_mapping->value = c;
        map[c] = next_mapping;
        current_mapping = next_mapping;
      } else {
        current_mapping = entry->second;
      }
    }

    current_mapping->callback = callback;
  }

  static void
  bind_mode_command(MainWindow& window, Tab&)
  {
    window.get_command_entry().set_text(":");
    window.set_mode(Mode::COMMAND);
  }

  static void
  bind_mode_insert(MainWindow& window, Tab&)
  {
    window.set_mode(Mode::INSERT);
  }

  static void
  bind_mode_hint(MainWindow& window, Tab&)
  {
    window.set_mode(Mode::HINT);
  }

  static void
  bind_mode_hint_new_tab(MainWindow& window, Tab&)
  {
    window.set_mode(Mode::HINT_NEW_TAB);
  }

  static void
  bind_tab_reload(MainWindow&, Tab& tab)
  {
    tab.reload();
  }

  static void
  bind_tab_reload_bypass_cache(MainWindow&, Tab& tab)
  {
    tab.reload(true);
  }

  static void
  bind_tab_open(MainWindow& window, Tab&)
  {
    window.open_tab();
  }

  static void
  bind_tab_close(MainWindow& window, Tab& tab)
  {
    window.close_tab(tab);
  }

  static void
  bind_tab_next(MainWindow& window, Tab&)
  {
    window.next_tab();
  }

  static void
  bind_tab_prev(MainWindow& window, Tab&)
  {
    window.prev_tab();
  }

  static void
  bind_split(MainWindow&, Tab& tab)
  {
    tab.split(false);
  }

  static void
  bind_vsplit(MainWindow&, Tab& tab)
  {
    tab.split(true);
  }

  static void
  bind_split_close(MainWindow&, Tab& tab)
  {
    tab.close_active_view();
  }

  static void
  bind_split_only(MainWindow&, Tab& tab)
  {
    tab.close_other_views();
  }

  static void
  bind_split_focus_left(MainWindow&, Tab& tab)
  {
    tab.focus_view(FocusDirection::LEFT);
  }

  static void
  bind_split_focus_down(MainWindow&, Tab& tab)
  {
    tab.focus_view(FocusDirection::DOWN);
  }

  static void
  bind_split_focus_up(MainWindow&, Tab& tab)
  {
    tab.focus_view(FocusDirection::UP);
  }

  static void
  bind_split_focus_right(MainWindow&, Tab& tab)
  {
    tab.focus_view(FocusDirection::RIGHT);
  }

  static void
  bind_split_cycle(MainWindow&, Tab& tab)
  {
    tab.cycle_view(false);
  }

  static void
  bind_split_cycle_prev(MainWindow&, Tab& tab)
  {
    tab.cycle_view(true);
  }

  static void
  bind_scroll_left(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollBy({ left: -100 });");
  }

  static void
  bind_scroll_right(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollBy({ left: 100 });");
  }

  static void
  bind_scroll_up(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollBy({ top: -100 });");
  }

  static void
  bind_scroll_down(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollBy({ top: 100 });");
  }

  static void
  bind_scroll_page_up(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollBy({ top: -(window.innerHeight / 2) });");
  }

  static void
  bind_scroll_page_down(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollBy({ top: window.innerHeight / 2 });");
  }

  static void
  bind_scroll_top(MainWindow&, Tab& tab)
  {
    tab.execute_script("window.scrollTo({ top: 0 });");
  }

  static void
  bind_scroll_bottom(MainWindow&, Tab& tab)
  {
    tab.execute_script(
      "window.scrollTo({ top: document.body.scrollHeight });"
    );
  }

  static void
  bind_history_prev(MainWindow&, Tab& tab)
  {
    tab.go_back();
  }

  static void
  bind_history_next(MainWindow&, Tab& tab)
  {
    tab.go_forward();
  }

  static void
  bind_complete_open(MainWindow& window, Tab&)
  {
    window.get_command_entry().set_text(":open ");
    window.set_mode(Mode::COMMAND);
  }

  static void
  bind_complete_open_tab(MainWindow& window, Tab&)
  {
    window.get_command_entry().set_text(":open-tab ");
    window.set_mode(Mode::COMMAND);
  }

  static void
  bind_paste(MainWindow& window, Tab& tab)
  {
    const auto clipboard = window.get_clipboard();

    clipboard->read_text_async(
      [&tab, clipboard](const Glib::RefPtr<Gio::AsyncResult>& result)
      {
        try
        {
          const auto uri = clipboard->read_text_finish(result);

          if (!uri.empty())
          {
            tab.load_uri(uri);
          }
        }
        catch (const Glib::Error&)
        {
        }
      }
    );
  }

  static void
  bind_paste_open_tab(MainWindow& window, Tab&)
  {
    const auto clipboard = window.get_clipboard();

    clipboard->read_text_async(
      [&window, clipboard](const Glib::RefPtr<Gio::AsyncResult>& result)
      {
        try
        {
          const auto uri = clipboard->read_text_finish(result);

          if (!uri.empty())
          {
            window.open_tab(uri);
          }
        }
        catch (const Glib::Error&)
        {
        }
      }
    );
  }

  static void
  bind_yank(MainWindow& window, Tab& tab)
  {
    const auto clipboard = window.get_clipboard();
    const auto uri = tab.get_uri();

    if (uri.empty())
    {
      return;
    }
    clipboard->set_text(uri);
    window.get_command_entry().show_notification(
      std::format("Yanked {}", uri.raw())
    );
  }

  static void
  bind_search_forwards(MainWindow& window, Tab&)
  {
    window.get_command_entry().set_text("/");
    window.set_mode(Mode::COMMAND);
  }

  static void
  bind_search_backwards(MainWindow& window, Tab&)
  {
    window.get_command_entry().set_text("?");
    window.set_mode(Mode::COMMAND);
  }

  static void
  bind_search_next(MainWindow&, Tab& tab)
  {
    tab.search_next();
  }

  static void
  bind_search_prev(MainWindow&, Tab& tab)
  {
    tab.search_prev();
  }
}
