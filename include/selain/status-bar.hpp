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
#ifndef SELAIN_STATUS_BAR_HPP_GUARD
#define SELAIN_STATUS_BAR_HPP_GUARD

#include <selain/mode.hpp>

#include <gtkmm.h>

namespace selain
{
  /**
   * GTK widget for browser status bar.
   */
  class StatusBar : public Gtk::Box
  {
  public:
    enum class NotificationType
    {
      NORMAL,
      ERROR
    };

    explicit StatusBar();

    void set_mode(Mode mode);

    void set_status(const Glib::ustring& status);
    void set_permanent_status(const Glib::ustring& status);

    void add_notification(
      const Glib::ustring& status,
      NotificationType type = NotificationType::NORMAL,
      int timeout = 5
    );

  private:
    void on_notification_reset();

  private:
    Gtk::Label m_mode_label;
    Gtk::Label m_status_label;
    Glib::ustring m_permanent_status;
  };
}

#endif /* !SELAIN_STATUS_BAR_HPP_GUARD */
