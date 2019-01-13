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

#include <functional>
#include <unordered_map>

namespace selain
{
  using binding_callback = std::function<void(Tab*)>;

  struct binding
  {
    binding_callback callback;
    bool control_mask;
  };

  static bool key_event_normal_mode(Tab*, GdkEventKey*);
  static bool key_event_insert_mode(Tab*, GdkEventKey*);

  bool
  Tab::on_web_view_key_press(GdkEventKey* event)
  {
    switch (m_mode)
    {
      case Mode::NORMAL:
        if (key_event_normal_mode(this, event))
        {
          return GDK_EVENT_STOP;
        }
        break;

      case Mode::INSERT:
        if (key_event_insert_mode(this, event))
        {
          return GDK_EVENT_STOP;
        }
        break;
    }

    return GDK_EVENT_PROPAGATE;
  }

  static void
  normal_mode_colon(Tab* tab)
  {
    tab->command_input().set_text(":");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  normal_mode_division(Tab* tab)
  {
    tab->command_input().set_text("/");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  normal_mode_question(Tab* tab)
  {
    tab->command_input().set_text("?");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  normal_mode_i(Tab* tab)
  {
    tab->set_mode(Mode::INSERT);
  }

  static void
  normal_mode_j(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ top: 100 });");
  }

  static void
  normal_mode_k(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ top: -100 });");
  }

  static void
  normal_mode_h(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ left: -100 });");
  }

  static void
  normal_mode_l(Tab* tab)
  {
    tab->execute_script("window.scrollBy({ left: 100 });");
  }

  static void
  normal_mode_capital_k(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->next_tab();
  }

  static void
  normal_mode_capital_j(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->prev_tab();
  }

  static void
  normal_mode_t(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->open_tab();
  }

  static void
  normal_mode_x(Tab* tab)
  {
    const auto container = tab->get_toplevel();

    if (!container)
    {
      return;
    }
    static_cast<MainWindow*>(container)->close_tab(tab);
  }

  static void
  normal_mode_o(Tab* tab)
  {
    tab->command_input().set_text(":open ");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  normal_mode_capital_o(Tab* tab)
  {
    tab->command_input().set_text(":tabnew ");
    tab->set_mode(Mode::COMMAND);
  }

  static void
  normal_mode_capital_h(Tab* tab)
  {
    tab->go_back();
  }

  static void
  normal_mode_capital_l(Tab* tab)
  {
    tab->go_forward();
  }

  static const std::unordered_map<::guint, binding> normal_mode_bindings =
  {
    // Various modes.
    { ::gdk_unicode_to_keyval(U':'), { normal_mode_colon, false } },
    { ::gdk_unicode_to_keyval(U'/'), { normal_mode_division, false } },
    { ::gdk_unicode_to_keyval(U'?'), { normal_mode_question, false } },
    { ::gdk_unicode_to_keyval(U'i'), { normal_mode_i, false } },

    // Scrolling.
    { ::gdk_unicode_to_keyval(U'j'), { normal_mode_j, false } },
    { ::gdk_unicode_to_keyval(U'k'), { normal_mode_k, false } },
    { ::gdk_unicode_to_keyval(U'h'), { normal_mode_h, false } },
    { ::gdk_unicode_to_keyval(U'l'), { normal_mode_l, false } },

    // Tab management.
    { ::gdk_unicode_to_keyval(U'J'), { normal_mode_capital_j, false } },
    { ::gdk_unicode_to_keyval(U'K'), { normal_mode_capital_k, false } },
    { ::gdk_unicode_to_keyval(U't'), { normal_mode_t, false } },
    { ::gdk_unicode_to_keyval(U'x'), { normal_mode_x, false } },

    // Navigation.
    { ::gdk_unicode_to_keyval(U'o'), { normal_mode_o, false } },
    { ::gdk_unicode_to_keyval(U'O'), { normal_mode_capital_o, false } },
    { ::gdk_unicode_to_keyval(U'H'), { normal_mode_capital_h, false } },
    { ::gdk_unicode_to_keyval(U'L'), { normal_mode_capital_l, false } },
  };

  static bool
  key_event_normal_mode(Tab* tab, GdkEventKey* event)
  {
    const bool control_mask = event->state & GDK_CONTROL_MASK;
    const auto binding = normal_mode_bindings.find(event->keyval);

    if (binding != std::end(normal_mode_bindings) &&
        binding->second.control_mask == control_mask)
    {
      binding->second.callback(tab);

      return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
  }

  static bool
  key_event_insert_mode(Tab* tab, GdkEventKey* event)
  {
    if (event->keyval == GDK_KEY_Escape)
    {
      tab->set_mode(Mode::NORMAL);

      return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
  }
}
