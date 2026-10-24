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
#include <chrono>

#include <selain/keyboard.hpp>
#include <selain/main-window.hpp>

namespace selain::keyboard
{
  struct Mapping
  {
    using callback_type = std::function<void(MainWindow&, Tab&)>;
    using mapping_type = std::unordered_map<
      std::uint32_t,
      std::shared_ptr<Mapping>
    >;

    std::uint32_t value;
    callback_type callback;
    mapping_type mapping;
    mapping_type control_mapping;
  };

  static constexpr int keypress_timeout = 2;
  static std::shared_ptr<Mapping> top_mapping;

  void initialize()
  {
    // Switching between modes.
    bind(":", "command :");
    bind("i", "insert");
    bind("f", "hint");
    bind("F", "hint-tab");

    // Tab management.
    bind("r", "reload");
    bind("R", "reload!");
    bind("t", "open-tab");
    bind("x", "quit");
    bind("J", "tab-prev");
    bind("K", "tab-next");

    // Split management (Vim Ctrl-W).
    bind("^ws", "split");
    bind("^wS", "split");
    bind("^wv", "vsplit");
    bind("^wq", "quit");
    bind("^wc", "quit");
    bind("^wo", "only");
    bind("^wh", "split-focus left");
    bind("^wj", "split-focus down");
    bind("^wk", "split-focus up");
    bind("^wl", "split-focus right");
    bind("^ww", "split-focus next");
    bind("^w^w", "split-focus next");
    bind("^wW", "split-focus prev");

    // Navigation.
    bind("d", "scroll page-down");
    bind("<Page_Down>", "scroll page-down");
    bind("h", "scroll left");
    bind("j", "scroll down");
    bind("k", "scroll up");
    bind("l", "scroll right");
    bind("gg", "scroll top");
    bind("<Home>", "scroll top");
    bind("G", "scroll bottom");
    bind("<End>", "scroll bottom");
    bind("H", "back");
    bind("L", "forward");
    bind("o", "command :open ");
    bind("O", "command :open-tab ");
    bind("p", "paste");
    bind("P", "paste-tab");
    bind("u", "scroll page-up");
    bind("<Page_Up>", "scroll page-up");
    bind("yy", "yank");

    // Searching.
    bind("/", "command /");
    bind("?", "command ?");
    bind("n", "search-next");
    bind("N", "search-prev");

    // Developer tools.
    bind("<F12>", "inspector");
  }

  static ::guint
  parse_key_token(const Glib::ustring& sequence, Glib::ustring::size_type& i)
  {
    if (sequence[i] == '<')
    {
      const auto end = sequence.find('>', i + 1);

      if (end != Glib::ustring::npos && end > i + 1)
      {
        std::string name;

        name.reserve(end - i - 1);
        for (auto j = i + 1; j < end; ++j)
        {
          name += static_cast<char>(sequence[j]);
        }

        const auto keyval = ::gdk_keyval_from_name(name.c_str());

        if (keyval != GDK_KEY_VoidSymbol)
        {
          i = end;

          return keyval;
        }
      }
    }

    return ::gdk_unicode_to_keyval(sequence[i]);
  }

  static void
  bind(const Glib::ustring& sequence, const Mapping::callback_type& callback)
  {
    const auto length = sequence.length();
    std::shared_ptr<Mapping> current_mapping;

    if (!length)
    {
      return;
    }

    if (!top_mapping)
    {
      top_mapping = std::make_shared<keyboard::Mapping>();
    }
    current_mapping = top_mapping;

    for (Glib::ustring::size_type i = 0; i < length; ++i)
    {
      bool is_control = false;

      if (sequence[i] == '^' && i + 1 < length)
      {
        ++i;
        is_control = true;
      }

      const auto c = parse_key_token(sequence, i);
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

  void
  bind(const Glib::ustring& sequence, const Glib::ustring& command)
  {
    bind(
      sequence,
      [command](MainWindow& window, Tab&)
      {
        window.run_command(command, false);
      }
    );
  }

  static ::gboolean
  key_event_normal_mode(
    MainWindow& window,
    Tab& tab,
    ::guint keyval,
    ::GdkModifierType state
  )
  {
    using namespace std::chrono;

    static std::shared_ptr<Mapping> last_mapping;
    static time_point<system_clock> last_keypress;

    const bool is_control = state & GDK_CONTROL_MASK;
    const auto now = system_clock::now();
    Mapping::mapping_type::const_iterator entry;

    if (last_mapping)
    {
      const auto difference = duration_cast<seconds>(
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
    else if (
      !entry->second->mapping.empty() ||
      !entry->second->control_mapping.empty()
    )
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
  key_event_hint_mode(
    MainWindow& window,
    Tab& tab,
    ::guint keyval,
    ::GdkModifierType state
  )
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

  ::gboolean
  on_tab_key_pressed(
    ::GtkEventControllerKey*,
    ::guint keyval,
    ::guint,
    ::GdkModifierType state,
    Tab* tab
  )
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
