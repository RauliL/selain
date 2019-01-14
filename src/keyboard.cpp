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

namespace selain
{
  static const int keypress_timeout = 2;
  static std::shared_ptr<keyboard::Mapping> top_mapping;

  static bool key_event_normal_mode(Tab*, ::GdkEventKey*);
  static bool key_event_insert_mode(Tab*, ::GdkEventKey*);
  static void add_mapping(const std::u32string&, const keyboard::Binding&);

  static void bind_mode_command(Tab*);
  static void bind_mode_insert(Tab*);
  static void bind_tab_reload(Tab*);
  static void bind_tab_reload_bypass_cache(Tab*);
  static void bind_tab_open(Tab*);
  static void bind_tab_close(Tab*);
  static void bind_tab_prev(Tab*);
  static void bind_tab_next(Tab*);
  static void bind_scroll_left(Tab*);
  static void bind_scroll_down(Tab*);
  static void bind_scroll_up(Tab*);
  static void bind_scroll_right(Tab*);
  static void bind_scroll_top(Tab*);
  static void bind_scroll_bottom(Tab*);
  static void bind_history_prev(Tab*);
  static void bind_history_next(Tab*);
  static void bind_complete_open(Tab*);
  static void bind_complete_open_tab(Tab*);
  static void bind_paste(Tab*);
  static void bind_paste_open_tab(Tab*);
  static void bind_yank(Tab*);
  static void bind_search_forwards(Tab*);
  static void bind_search_backwards(Tab*);
  static void bind_search_next(Tab*);
  static void bind_search_prev(Tab*);

  namespace keyboard
  {
    void
    initialize()
    {
      // Switching between modes.
      add_mapping(U":", bind_mode_command);
      add_mapping(U"i", bind_mode_insert);

      // Tab management.
      add_mapping(U"r", bind_tab_reload);
      add_mapping(U"R", bind_tab_reload_bypass_cache);
      add_mapping(U"t", bind_tab_open);
      add_mapping(U"x", bind_tab_close);
      add_mapping(U"J", bind_tab_prev);
      add_mapping(U"K", bind_tab_next);

      // Navigation.
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
      add_mapping(U"yy", bind_yank);

      // Searching.
      add_mapping(U"/", bind_search_forwards);
      add_mapping(U"?", bind_search_backwards);
      add_mapping(U"n", bind_search_next);
      add_mapping(U"N", bind_search_prev);
    }
  }

  bool
  Tab::on_web_view_key_press(GdkEventKey* event)
  {
    switch (m_mode)
    {
      case Mode::NORMAL:
        return key_event_normal_mode(this, event);

      case Mode::INSERT:
        return key_event_insert_mode(this, event);
    }

    return GDK_EVENT_PROPAGATE;
  }

  static bool
  key_event_normal_mode(Tab* tab, ::GdkEventKey* event)
  {
    static std::shared_ptr<keyboard::Mapping> last_mapping;
    static std::chrono::time_point<std::chrono::system_clock> last_keypress;
    const bool is_control = event->state & GDK_CONTROL_MASK;
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
      entry = last_mapping->control_mapping.find(event->keyval);
      if (entry == std::end(last_mapping->control_mapping))
      {
        return GDK_EVENT_STOP;
      }
    } else {
      entry = last_mapping->mapping.find(event->keyval);
      if (entry == std::end(last_mapping->mapping))
      {
        return GDK_EVENT_STOP;
      }
    }

    if (entry->second->binding)
    {
      entry->second->binding(tab);
      last_mapping.reset();
    }
    else if (!entry->second->mapping.empty())
    {
      last_mapping = entry->second;
    }

    return GDK_EVENT_STOP;
  }

  static bool
  key_event_insert_mode(Tab* tab, ::GdkEventKey* event)
  {
    if (event->keyval == GDK_KEY_Escape)
    {
      tab->set_mode(Mode::NORMAL);

      return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
  }

  static void
  add_mapping(const std::u32string& sequence, const keyboard::Binding& binding)
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
      const auto c = ::gdk_unicode_to_keyval(sequence[i]);
      bool is_control = false;
      keyboard::Mapping::mapping_type::const_iterator entry;

      if (c == '^' && i + 1 < length)
      {
        ++i;
        is_control = true;
        entry = current_mapping->control_mapping.find(c);
      } else {
        entry = current_mapping->mapping.find(c);
      }
      if (entry == std::end(current_mapping->mapping))
      {
        const auto owner = current_mapping;

        current_mapping = std::make_shared<keyboard::Mapping>();
        current_mapping->value = c;
        if (is_control)
        {
          owner->control_mapping[c] = current_mapping;
        } else {
          owner->mapping[c] = current_mapping;
        }
      }
    }

    current_mapping->binding = binding;
  }

  static void
  bind_mode_command(Tab* tab)
  {
    tab->command_input().set_text(":");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  bind_mode_insert(Tab* tab)
  {
    tab->set_mode(Mode::INSERT);
  }

  static void
  bind_tab_reload(Tab* tab)
  {
    tab->reload();
  }

  static void
  bind_tab_reload_bypass_cache(Tab* tab)
  {
    tab->reload(true);
  }

  static void
  bind_tab_open(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->open_tab();
  }

  static void
  bind_tab_close(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->close_tab(tab);
  }

  static void
  bind_tab_next(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->next_tab();
  }

  static void
  bind_tab_prev(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->prev_tab();
  }

  static void
  bind_scroll_left(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ left: -100 });");
  }

  static void
  bind_scroll_right(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ left: 100 });");
  }

  static void
  bind_scroll_up(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ top: -100 });");
  }

  static void
  bind_scroll_down(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ top: 100 });");
  }

  static void
  bind_scroll_top(Tab* tab)
  {
    tab->execute_script("window.scrollTo({ top: 0 });");
  }

  static void
  bind_scroll_bottom(Tab* tab)
  {
    tab->execute_script(
      "window.scrollTo({ top: document.body.scrollHeight });"
    );
  }

  static void
  bind_history_prev(Tab* tab)
  {
    tab->go_back();
  }

  static void
  bind_history_next(Tab* tab)
  {
    tab->go_forward();
  }

  static void
  bind_complete_open(Tab* tab)
  {
    tab->command_input().set_text(":open ");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  bind_complete_open_tab(Tab* tab)
  {
    tab->command_input().set_text(":open-tab ");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  bind_paste(Tab* tab)
  {
    const auto clipboard = Gtk::Clipboard::get();
    Glib::ustring uri;

    if (!clipboard->wait_is_text_available())
    {
      return;
    }
    uri = clipboard->wait_for_text();
    if (!uri.empty())
    {
      tab->load_uri(uri);
    }
  }

  static void
  bind_paste_open_tab(Tab* tab)
  {
    const auto clipboard = Gtk::Clipboard::get();
    const auto container = tab->get_toplevel();
    Glib::ustring uri;

    if (!clipboard->wait_is_text_available() || !container)
    {
      return;
    }
    uri = clipboard->wait_for_text();
    if (!uri.empty())
    {
      static_cast<MainWindow*>(container)->open_tab(uri);
    }
  }

  static void
  bind_yank(Tab* tab)
  {
    const auto clipboard = Gtk::Clipboard::get();
    const auto uri = tab->get_uri();

    if (!uri.empty())
    {
      clipboard->set_text(uri);
      // TODO: Display message to the user which informs that the URI has been
      // yanked.
    }
  }

  static void
  bind_search_forwards(Tab* tab)
  {
    tab->command_input().set_text("/");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  bind_search_backwards(Tab* tab)
  {
    tab->command_input().set_text("?");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  bind_search_next(Tab* tab)
  {
    tab->search_next();
  }

  static void
  bind_search_prev(Tab* tab)
  {
    tab->search_prev();
  }
}
