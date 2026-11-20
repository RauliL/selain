/*
 * Copyright (c) 2026, Rauli Laine
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

#include <gtkmm.h>

namespace selain::gtk_compat
{
  /**
   * Unsets the start child of a paned.
   *
   * Gtk::Paned::unset_start_child() exists only since gtkmm 4.16.
   */
  inline void
  unset_start_child(Gtk::Paned& paned)
  {
#if GTKMM_CHECK_VERSION(4, 16, 0)
    paned.unset_start_child();
#else
    ::gtk_paned_set_start_child(paned.gobj(), nullptr);
#endif
  }

  /**
   * Unsets the end child of a paned.
   *
   * Gtk::Paned::unset_end_child() exists only since gtkmm 4.16.
   */
  inline void
  unset_end_child(Gtk::Paned& paned)
  {
#if GTKMM_CHECK_VERSION(4, 16, 0)
    paned.unset_end_child();
#else
    ::gtk_paned_set_end_child(paned.gobj(), nullptr);
#endif
  }

  /**
   * Removes all rows from a list box.
   *
   * Gtk::ListBox::remove_all() exists only since gtkmm 4.12.
   */
  inline void
  remove_all(Gtk::ListBox& list)
  {
#if GTKMM_CHECK_VERSION(4, 12, 0)
    list.remove_all();
#else
    while (auto* child = list.get_first_child())
    {
      list.remove(*child);
    }
#endif
  }
}
