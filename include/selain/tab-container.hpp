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
#ifndef SELAIN_TAB_CONTAINER_HPP_GUARD
#define SELAIN_TAB_CONTAINER_HPP_GUARD

#include <selain/tab.hpp>
#include <selain/tab-label.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  /**
   * GTK widget which acts as container for a tab, whether it's normal tab
   * content or splitted tab. For convenience reasons, the tab container itself
   * also acts as a tab.
   */
  class TabContainer : public Tab
  {
  public:
    explicit TabContainer(
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

    Mode get_mode() const
    {
      return m_top_level_tab->get_mode();
    }

    void set_mode(Mode mode)
    {
      m_top_level_tab->set_mode(mode);
    }

    Glib::ustring get_uri() const
    {
      return m_top_level_tab->get_uri();
    }

    void load_uri(const Glib::ustring& uri)
    {
      m_top_level_tab->load_uri(uri);
    }

    void reload(bool bypass_cache = false)
    {
      m_top_level_tab->reload(bypass_cache);
    }

    void stop_loading()
    {
      m_top_level_tab->stop_loading();
    }

    void go_back_in_history()
    {
      m_top_level_tab->go_back_in_history();
    }

    void go_forward_in_history()
    {
      m_top_level_tab->go_forward_in_history();
    }

    void find(const Glib::ustring& text, bool forwards = true)
    {
      m_top_level_tab->find(text, forwards);
    }

    void find_next()
    {
      m_top_level_tab->find_next();
    }

    void find_previous()
    {
      m_top_level_tab->find_previous();
    }

    void execute_script(const Glib::ustring& script,
                        ::GCancellable* cancellable = nullptr,
                        ::GAsyncReadyCallback callback = nullptr,
                        void* data = nullptr)
    {
      m_top_level_tab->execute_script(script, cancellable, callback, data);
    }

    void grab_focus();

  private:
    void on_close_button_clicked();
    void on_title_changed(Tab& tab, const Glib::ustring& title);
    void on_favicon_changed(Tab& tab, ::cairo_surface_t* surface);

  private:
    TabLabel m_tab_label;
    Glib::RefPtr<Tab> m_top_level_tab;
  };
}

#endif /* !SELAIN_TAB_CONTAINER_HPP_GUARD */
