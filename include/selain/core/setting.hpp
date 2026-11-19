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
#pragma once

#include <selain/ui/main-window.hpp>

namespace selain::setting
{
  struct Setting
  {
    /**
     * Returns value of the setting.
     */
    virtual Glib::ustring get(ui::MainWindow& window) const = 0;

    /**
     * Attempts to set new value for the setting. Returns `true` if it was
     * successful and `false` if the new value for setting was somehow invalid.
     *
     * TODO: Make this return error message instead.
     */
    virtual bool set(
      ui::MainWindow& window,
      const Glib::ustring& new_value
    ) = 0;
  };

  std::optional<Glib::ustring> get(
    ui::MainWindow& window,
    const Glib::ustring& key
  );

  bool set(
    ui::MainWindow& window,
    const Glib::ustring& key,
    const Glib::ustring& value
  );

  /**
   * Returns all setting names, alphabetically sorted.
   */
  std::vector<Glib::ustring> names();
}
