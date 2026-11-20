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

#include <vector>

#include <gtkmm.h>

namespace selain::ui
{
  class MainWindow;
  class View;

  /**
   * Modal dialog listing all open views with favicon and title. Supports fuzzy
   * filtering and keyboard-only navigation (Enter to select, Escape to close).
   */
  class ViewListDialog : public Gtk::Window
  {
  public:
    /**
     * Creates and presents a view-list dialog for @p window. The dialog
     * destroys itself when closed.
     */
    static void show(MainWindow& window);

  private:
    class Row;

    explicit ViewListDialog(MainWindow& window);
    ~ViewListDialog() override;

    void populate();
    void apply_filter();
    void select_index(int index);
    void move_selection(int delta);
    void activate_selection();
    void close_dialog();
    bool on_key_pressed(guint keyval, guint, Gdk::ModifierType state);
    void on_filter_changed();
    void on_row_activated(Gtk::ListBoxRow* row);

  private:
    MainWindow& m_window;
    Gtk::Box m_box;
    Gtk::Entry m_filter;
    Gtk::ScrolledWindow m_scrolled;
    Gtk::ListBox m_list;
    Glib::RefPtr<Gtk::EventControllerKey> m_key_controller;
    std::vector<View*> m_views;
    std::vector<View*> m_filtered;
  };
}
