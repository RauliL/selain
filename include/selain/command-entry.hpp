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
#ifndef SELAIN_COMMAND_ENTRY_HPP_GUARD
#define SELAIN_COMMAND_ENTRY_HPP_GUARD

#include <gtkmm.h>

namespace selain
{
  class CommandEntry : public Gtk::Bin
  {
  public:
    using signal_command_received_type = sigc::signal<
      void,
      const Glib::ustring&
    >;

    explicit CommandEntry();

    inline Glib::ustring get_text() const
    {
      return m_entry.get_text();
    }

    inline void set_text(const Glib::ustring& text)
    {
      m_entry.set_text(text);
    }

    void grab_focus();

    inline signal_command_received_type& signal_command_received()
    {
      return m_signal_command_received;
    }

  private:
    void on_activate();

  private:
    Gtk::Entry m_entry;
    signal_command_received_type m_signal_command_received;
  };
}

#endif /* !SELAIN_COMMAND_ENTRY_HPP_GUARD */
