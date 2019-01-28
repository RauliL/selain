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
#ifndef SELAIN_TAB_HPP_GUARD
#define SELAIN_TAB_HPP_GUARD

#include <selain/tab-label.hpp>
#include <selain/view.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  class MainWindow;

  /**
   * GTK widget for a browser tab.
   */
  class Tab : public Gtk::Bin
  {
  public:
    explicit Tab(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings
    );

    /**
     * Returns the GTK widget used as label for the tab.
     */
    inline TabLabel& get_tab_label()
    {
      return m_tab_label;
    }

    /**
     * Returns the GTK widget used as label for the tab.
     */
    inline const TabLabel& get_tab_label() const
    {
      return m_tab_label;
    }

    /**
     * Returns the top level view being displayed by the tab.
     */
    inline const Glib::RefPtr<View>& get_view()
    {
      return m_view;
    }

    /**
     * Returns the top level view being displayed by the tab.
     */
    inline Glib::RefPtr<const View> get_view() const
    {
      return m_view;
    }

  private:
    void on_close_button_clicked();
    bool on_focus_in();
    void on_close();
    void on_title_changed(const Glib::ustring& title);
    void on_favicon_changed(::cairo_surface_t* icon);

  private:
    TabLabel m_tab_label;
    Glib::RefPtr<View> m_view;
  };
}

#endif /* !SELAIN_TAB_HPP_GUARD */
