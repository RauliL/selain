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

#include <glibmm.h>

namespace selain::ui
{
  class MainWindow;
}

namespace selain::command
{
  /**
   * Called when command execution finishes. Receives `std::nullopt` on success,
   * or an error message when execution failed.
   */
  using result_callback_type = std::function<void(
    const std::optional<Glib::ustring>& error
  )>;

  /**
   * Builtin command handler. Must invoke @p callback exactly once when the
   * command has finished (including any asynchronous work it starts).
   */
  using callback_type = std::function<void(
    ui::MainWindow& window,
    Glib::ustring args,
    result_callback_type callback
  )>;

  struct Info
  {
    Glib::ustring name;
    std::vector<Glib::ustring> aliases;
    Glib::ustring description;
    callback_type callback;
  };

  /**
   * Returns names of all commands, alphabetically sorted.
   */
  std::vector<Glib::ustring> names();

  /**
   * Initializes the builtin command mapping. This must be called during
   * program startup.
   */
  void initialize();
}
