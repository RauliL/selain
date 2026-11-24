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

#include <functional>
#include <vector>

#include <gtkmm.h>

namespace selain::ui
{
  /**
   * One selectable row in a ListDialog: primary title, secondary description,
   * and an optional icon (paintable preferred, otherwise icon_name).
   */
  struct ListDialogEntry
  {
    Glib::ustring title;
    Glib::ustring secondary;
    /** When empty, filtering uses title + ' ' + secondary. */
    Glib::ustring filter_text;
    Glib::RefPtr<Gdk::Paintable> icon;
    Glib::ustring icon_name{"text-x-generic"};
  };

  /**
   * Modal dialog listing entries with fuzzy filtering and keyboard-only
   * navigation (Enter to select, Escape to close).
   */
  class ListDialog : public Gtk::Window
  {
  public:
    /**
     * Creates and presents a list dialog. The dialog destroys itself when
     * closed. @p on_activate receives the index into the original @p entries
     * vector. When the filter is empty, @p preferred_index is selected;
     * otherwise the top-ranked match is selected.
     */
    static void show(
      Gtk::Window& parent,
      const Glib::ustring& title,
      const Glib::ustring& filter_placeholder,
      std::vector<ListDialogEntry> entries,
      std::function<void(std::size_t index)> on_activate,
      int preferred_index = 0
    );

  private:
    class Row;

    ListDialog(
      Gtk::Window& parent,
      const Glib::ustring& title,
      const Glib::ustring& filter_placeholder,
      std::vector<ListDialogEntry> entries,
      std::function<void(std::size_t index)> on_activate,
      int preferred_index
    );
    ~ListDialog() override;

    const Glib::ustring& entry_haystack(std::size_t index) const;
    void apply_filter();
    void select_index(int index);
    void move_selection(int delta);
    void activate_selection();
    void close_dialog();
    bool on_key_pressed(guint keyval, guint, Gdk::ModifierType state);
    void on_filter_changed();
    void on_row_activated(Gtk::ListBoxRow* row);

  private:
    Gtk::Box m_box;
    Gtk::Entry m_filter;
    Gtk::ScrolledWindow m_scrolled;
    Gtk::ListBox m_list;
    Glib::RefPtr<Gtk::EventControllerKey> m_key_controller;
    std::vector<ListDialogEntry> m_entries;
    std::vector<Glib::ustring> m_haystacks;
    std::vector<std::size_t> m_filtered;
    std::function<void(std::size_t index)> m_on_activate;
    int m_preferred_index;
  };
}
