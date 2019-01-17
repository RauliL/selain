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
#include <selain/command-entry.hpp>
#include <selain/theme.hpp>
#include <selain/utils.hpp>

namespace selain
{
  CommandEntry::CommandEntry()
  {
    m_entry.override_font(utils::get_monospace_font());
    m_entry.get_style_context()->add_provider(
      theme::get_command_entry_style_provider(),
      1000
    );

    m_entry.signal_activate().connect(sigc::mem_fun(
      this,
      &CommandEntry::on_activate
    ));

    add(m_entry);
    show_all();
  }

  void
  CommandEntry::grab_focus()
  {
    m_entry.grab_focus_without_selecting();
    m_entry.set_position(m_entry.get_text().length());
  }

  void
  CommandEntry::on_activate()
  {
    const auto text = m_entry.get_text();

    m_entry.set_text(Glib::ustring());
    m_signal_command_received.emit(text);
  }
}
