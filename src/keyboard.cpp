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
#include <vector>

#include <selain/keyboard.hpp>
#include <selain/main-window.hpp>

namespace selain::keyboard
{
  struct Mapping
  {
    using mapping_type = std::unordered_map<
      std::uint32_t,
      std::shared_ptr<Mapping>
    >;

    std::uint32_t value;
    Glib::ustring command;
    mapping_type mapping;
    mapping_type control_mapping;
    mapping_type alt_mapping;
  };

  enum class Modifier
  {
    NONE,
    CONTROL,
    ALT
  };

  static Mapping::mapping_type&
  select_mapping(Mapping& mapping, Modifier modifier)
  {
    switch (modifier)
    {
      case Modifier::CONTROL:
        return mapping.control_mapping;

      case Modifier::ALT:
        return mapping.alt_mapping;

      default:
        return mapping.mapping;
    }
  }

  static inline bool
  has_child_mappings(const Mapping& mapping)
  {
    return (
      !mapping.mapping.empty() ||
      !mapping.control_mapping.empty() ||
      !mapping.alt_mapping.empty()
    );
  }

  static Modifier
  parse_modifier(const Glib::ustring& sequence, Glib::ustring::size_type& i)
  {
    if (i + 1 >= sequence.length())
    {
      return Modifier::NONE;
    }
    if (sequence[i] == '^')
    {
      ++i;

      return Modifier::CONTROL;
    }
    if (sequence[i] == '~')
    {
      ++i;

      return Modifier::ALT;
    }

    return Modifier::NONE;
  }

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
    bind("~1", "tab 1");
    bind("~2", "tab 2");
    bind("~3", "tab 3");
    bind("~4", "tab 4");
    bind("~5", "tab 5");
    bind("~6", "tab 6");
    bind("~7", "tab 7");
    bind("~8", "tab 8");
    bind("~9", "tab 9");
    bind("~0", "tab 10");

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

  void
  bind(const Glib::ustring& sequence, const Glib::ustring& command)
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
      const auto modifier = parse_modifier(sequence, i);
      const auto c = parse_key_token(sequence, i);
      auto& map = select_mapping(*current_mapping, modifier);
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

    current_mapping->command = command;
  }

  bool
  unbind(const Glib::ustring& sequence)
  {
    struct Step
    {
      std::shared_ptr<Mapping> parent;
      Modifier modifier;
      std::uint32_t key;
    };

    const auto length = sequence.length();
    std::shared_ptr<Mapping> current_mapping;
    std::vector<Step> path;

    if (!length || !top_mapping)
    {
      return false;
    }
    current_mapping = top_mapping;

    for (Glib::ustring::size_type i = 0; i < length; ++i)
    {
      const auto modifier = parse_modifier(sequence, i);
      const auto c = parse_key_token(sequence, i);
      auto& map = select_mapping(*current_mapping, modifier);
      auto entry = map.find(c);

      if (entry == std::end(map))
      {
        return false;
      }

      path.push_back({current_mapping, modifier, c});
      current_mapping = entry->second;
    }

    if (current_mapping->command.empty())
    {
      return false;
    }
    current_mapping->command.clear();

    // Remove empty mappings from the leaf upward so orphaned prefixes
    // do not linger after the binding has been removed.
    for (auto it = path.rbegin(); it != path.rend(); ++it)
    {
      auto& map = select_mapping(*it->parent, it->modifier);
      auto entry = map.find(it->key);

      if (entry == std::end(map))
      {
        break;
      }

      const auto& node = entry->second;

      if (!node->command.empty() || has_child_mappings(*node))
      {
        break;
      }
      map.erase(entry);
    }

    return true;
  }

  static ::gboolean
  key_event_normal_mode(
    MainWindow& window,
    ::guint keyval,
    ::GdkModifierType state
  )
  {
    using namespace std::chrono;

    static std::shared_ptr<Mapping> last_mapping;
    static time_point<system_clock> last_keypress;

    const bool is_control = state & GDK_CONTROL_MASK;
    const bool is_alt = (state & GDK_ALT_MASK) && !is_control;
    const auto modifier = is_control
      ? Modifier::CONTROL
      : (is_alt ? Modifier::ALT : Modifier::NONE);
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

    const auto& map = select_mapping(*last_mapping, modifier);

    entry = map.find(keyval);
    if (entry == std::end(map))
    {
      return TRUE;
    }

    if (!entry->second->command.empty())
    {
      window.run_command(entry->second->command, false);
      last_mapping.reset();
    }
    else if (has_child_mappings(*entry->second))
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
        return key_event_normal_mode(*window, keyval, state);

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
