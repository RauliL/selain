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
   * Abstract base class for a browser view.
   */
  class View : public Gtk::Bin
  {
  public:
    using signal_close_type = sigc::signal<void, View&>;
    using signal_mode_changed_type = sigc::signal<
      void,
      View&,
      Mode
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

    explicit View();

    /**
     * Returns current mode of the view.
     */
    virtual Mode get_mode() const = 0;

    /**
     * Sets current mode of the view.
     */
    virtual void set_mode(Mode mode) = 0;

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

    void execute_command(const Glib::ustring& command);

    virtual void close() = 0;
    virtual void grab_focus() = 0;
    virtual void execute_script(
      const Glib::ustring& script,
      ::GCancellable* cancellable = nullptr,
      ::GAsyncReadyCallback callback = nullptr,
      void* data = nullptr
    ) = 0;
    virtual Glib::ustring get_uri() const = 0;
    virtual void load_uri(const Glib::ustring& uri) = 0;
    virtual void reload(bool bypass_cache = false) = 0;
    virtual void stop_loading() = 0;
    virtual void go_back_in_history() = 0;
    virtual void go_forward_in_history() = 0;
    virtual void find(const Glib::ustring& text, bool forwards = true) = 0;
    virtual void find_next() = 0;
    virtual void find_previous() = 0;
    virtual void add_hint_char(Glib::ustring::value_type c) = 0;
    virtual void remove_hint_char() = 0;
    virtual void activate_current_hint() = 0;

    /**
     * Attempts to split the current view into two different views.
     *
     * \param context     Web settings used for the splitted view.
     * \param settings    Web context used for the splitted view.
     * \param orientation Orientation of the split.
     * \return            Reference to the new view which was created because
     *                    of the split, or null reference if splitting this
     *                    view isn't possible for some reason.
     */
    virtual Glib::RefPtr<View> split(
      const Glib::RefPtr<WebContext>& context,
      const Glib::RefPtr<WebSettings>& settings,
      Gtk::Orientation orientation = Gtk::ORIENTATION_HORIZONTAL
    );

    inline signal_close_type& signal_close()
    {
      return m_signal_close;
    }

    inline const signal_close_type& signal_close() const
    {
      return m_signal_close;
    }

    inline signal_mode_changed_type& signal_mode_changed()
    {
      return m_signal_mode_changed;
    }

    inline const signal_mode_changed_type& signal_mode_changed() const
    {
      return m_signal_mode_changed;
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
    signal_mode_changed_type m_signal_mode_changed;
    signal_title_changed_type m_signal_title_changed;
    signal_favicon_changed_type m_signal_favicon_changed;
  };
}

#endif /* !SELAIN_VIEW_HPP_GUARD */
