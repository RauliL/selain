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
#ifndef SELAIN_VIEW_HPP_GUARD
#define SELAIN_VIEW_HPP_GUARD

#include <gtkmm.h>

#include <selain/mode.hpp>
#include <selain/web-context.hpp>
#include <selain/web-settings.hpp>

namespace selain
{
  class MainWindow;

  /**
   * Abstract base class for a view which is being displayed in a tab.
   */
  class View : public Gtk::Bin
  {
  public:
    using signal_close_type = sigc::signal<
      void,
      View&,
      const Glib::RefPtr<View>&
    >;
    using signal_split_type = sigc::signal<
      void,
      View&,
      const Glib::RefPtr<WebContext>&,
      const Glib::RefPtr<WebSettings>&,
      const Glib::ustring&,
      Gtk::Orientation
    >;
    using signal_title_changed_type = sigc::signal<
      void,
      View&,
      const Glib::ustring&
    >;
    using signal_favicon_changed_type = sigc::signal<
      void,
      View&,
      ::cairo_surface_t*
    >;

    enum class FindDirection
    {
      FORWARD,
      BACKWARD
    };

    explicit View();

    /**
     * Returns pointer to the main window where this view is being displayed,
     * or null pointer if this view isn't being displayed on a window.
     */
    MainWindow* get_main_window();

    /**
     * Returns pointer to the main window where this view is being displayed,
     * or null pointer if this view isn't being displayed on a window.
     */
    const MainWindow* get_main_window() const;

    virtual void close();

    void split(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings,
      const Glib::ustring& uri = Glib::ustring(),
      Gtk::Orientation orientation = Gtk::ORIENTATION_VERTICAL
    );

    void execute_command(const Glib::ustring& command);

    virtual void execute_script(
      const Glib::ustring& script,
      ::GCancellable* cancellable = nullptr,
      ::GAsyncReadyCallback callback = nullptr,
      void* data = nullptr
    ) = 0;
    virtual void set_current_mode(Mode mode) = 0;
    virtual Glib::ustring get_uri() const = 0;
    virtual void load_uri(const Glib::ustring& uri) = 0;
    virtual void reload(bool bypass_cache = false) = 0;
    virtual void stop_loading() = 0;
    virtual void go_back_in_history() = 0;
    virtual void go_forward_in_history() = 0;
    virtual void find(
      const Glib::ustring& text,
      FindDirection direction = FindDirection::FORWARD
    ) = 0;
    virtual void find_next() = 0;
    virtual void find_previous() = 0;
    virtual void add_hint_character(Glib::ustring::value_type c) = 0;
    virtual void remove_hint_character() = 0;
    virtual void activate_current_hint() = 0;

    inline signal_close_type& signal_close()
    {
      return m_signal_close;
    }

    inline const signal_close_type& signal_close() const
    {
      return m_signal_close;
    }

    inline signal_split_type& signal_split()
    {
      return m_signal_split;
    }

    inline const signal_split_type& signal_split() const
    {
      return m_signal_split;
    }

    inline signal_title_changed_type& signal_title_changed()
    {
      return m_signal_title_changed;
    }

    inline const signal_title_changed_type& signal_title_changed() const
    {
      return m_signal_title_changed;
    }

    inline signal_favicon_changed_type& signal_favicon_changed()
    {
      return m_signal_favicon_changed;
    }

    inline const signal_favicon_changed_type& signal_favicon_changed() const
    {
      return m_signal_favicon_changed;
    }

  private:
    signal_close_type m_signal_close;
    signal_split_type m_signal_split;
    signal_title_changed_type m_signal_title_changed;
    signal_favicon_changed_type m_signal_favicon_changed;
  };
}

#endif /* !SELAIN_VIEW_HPP_GUARD */
