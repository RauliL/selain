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
#ifndef SELAIN_SPLIT_VIEW_HPP_GUARD
#define SELAIN_SPLIT_VIEW_HPP_GUARD

#include <selain/view.hpp>

namespace selain
{
  /**
   * View implementation which is split into two child views.
   */
  class SplitView : public View
  {
  public:
    explicit SplitView(
      const Glib::RefPtr<View>& child1,
      const Glib::RefPtr<View>& child2,
      Gtk::Orientation orientation = Gtk::ORIENTATION_VERTICAL
    );

    /**
     * Returns the currently active child view.
     */
    inline const Glib::RefPtr<View>& get_active_child_view()
    {
      return m_active_child_index == 1 ? m_child1 : m_child2;
    }

    /**
     * Returns the currently active child view.
     */
    inline Glib::RefPtr<const View> get_active_child_view() const
    {
      return m_active_child_index == 1 ? m_child1 : m_child2;
    }

    void close();

    inline void execute_script(const Glib::ustring& script,
                               ::GCancellable* cancellable = nullptr,
                               ::GAsyncReadyCallback callback = nullptr,
                               void* data = nullptr)
    {
      get_active_child_view()->execute_script(
        script,
        cancellable,
        callback,
        data
      );
    }

    inline void set_current_mode(Mode mode)
    {
      get_active_child_view()->set_current_mode(mode);
    }

    inline Glib::ustring get_uri() const
    {
      return get_active_child_view()->get_uri();
    }

    inline void load_uri(const Glib::ustring& uri)
    {
      get_active_child_view()->load_uri(uri);
    }

    inline void reload(bool bypass_cache = false)
    {
      get_active_child_view()->reload(bypass_cache);
    }

    inline void stop_loading()
    {
      get_active_child_view()->stop_loading();
    }

    inline void go_back_in_history()
    {
      get_active_child_view()->go_back_in_history();
    }

    inline void go_forward_in_history()
    {
      get_active_child_view()->go_forward_in_history();
    }

    inline void find(const Glib::ustring& text,
                     FindDirection direction = FindDirection::FORWARD)
    {
      get_active_child_view()->find(text, direction);
    }

    inline void find_next()
    {
      get_active_child_view()->find_next();
    }

    inline void find_previous()
    {
      get_active_child_view()->find_previous();
    }

    inline void add_hint_character(Glib::ustring::value_type c)
    {
      get_active_child_view()->add_hint_character(c);
    }

    inline void remove_hint_character()
    {
      get_active_child_view()->remove_hint_character();
    }

    inline void activate_current_hint()
    {
      get_active_child_view()->activate_current_hint();
    }

  private:
    bool on_focus_in();
    void on_title_changed(View& view, const Glib::ustring& title);
    void on_favicon_changed(View& view, ::cairo_surface_t* icon);

  private:
    int m_active_child_index;
    Glib::RefPtr<View> m_child1;
    Glib::RefPtr<View> m_child2;
    Gtk::Paned m_paned;
  };
}

#endif /* !SELAIN_SPLIT_VIEW_HPP_GUARD */
