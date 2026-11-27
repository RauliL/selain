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

#include <filesystem>
#include <unordered_set>

#include <selain/core/bookmark-manager.hpp>
#include <selain/core/command.hpp>
#include <selain/core/history-manager.hpp>
#include <selain/core/password-manager.hpp>
#include <selain/core/session.hpp>
#include <selain/ui/command-entry.hpp>
#include <selain/ui/status-bar.hpp>
#include <selain/ui/tab-completion.hpp>
#include <selain/ui/tab.hpp>

namespace selain::ui
{
  class View;

  /**
   * Main window for the browser.
   */
  class MainWindow : public Gtk::ApplicationWindow
  {
  public:
    explicit MainWindow(const Glib::RefPtr<Gtk::Application>& application);
    ~MainWindow() override;

    /**
     * Executes an command.
     *
     * @param command The entire command line including name of the command and
     *                it's arguments.
     * @param interactive Set `true` if called from the user input, `false` if
     *                    coming from some non-interactive context such as
     *                    initialization script.
     * @param callback Function that is executed after command execution with
     *                 optional error message given as argument if execution of
     *                 the command was not successful.
     */
    void run_command(
      const Glib::ustring& command,
      bool interactive,
      const command::result_callback_type& callback
    );

    /**
     * Executes an script read from a file.
     */
    void run_script(
      const std::filesystem::path& path,
      const command::result_callback_type& callback
    );

    /**
     * Returns WebKit settings used for all browser tabs.
     */
    inline const Glib::RefPtr<webkit::Settings>& web_settings()
    {
      return m_web_settings;
    }

    /**
     * Returns WebKit settings used for all browser tabs.
     */
    inline Glib::RefPtr<const webkit::Settings> web_settings() const
    {
      return m_web_settings;
    }

    /**
     * Returns bookmark manager used by the browser.
     */
    inline const Glib::RefPtr<BookmarkManager>& bookmark_manager()
    {
      return m_bookmark_manager;
    }

    /**
     * Returns bookmark manager used by the browser.
     */
    inline Glib::RefPtr<const BookmarkManager> bookmark_manager() const
    {
      return m_bookmark_manager;
    }

    /**
     * Returns password manager used by the browser.
     */
    inline const Glib::RefPtr<PasswordManager>& password_manager()
    {
      return m_password_manager;
    }

    /**
     * Returns password manager used by the browser.
     */
    inline Glib::RefPtr<const PasswordManager> password_manager() const
    {
      return m_password_manager;
    }

    /**
     * Returns visit history manager used by the browser.
     */
    inline const Glib::RefPtr<HistoryManager>& history_manager()
    {
      return m_history_manager;
    }

    /**
     * Returns visit history manager used by the browser.
     */
    inline Glib::RefPtr<const HistoryManager> history_manager() const
    {
      return m_history_manager;
    }

    /**
     * Returns the current mode of the window.
     */
    inline Mode get_mode() const
    {
      return m_mode;
    }

    /**
     * Sets the current mode of the window.
     */
    void set_mode(Mode mode);

    /**
     * Returns the status bar of the window.
     */
    inline StatusBar& get_status_bar()
    {
      return m_status_bar;
    }

    /**
     * Returns the status bar of the window.
     */
    inline const StatusBar& get_status_bar() const
    {
      return m_status_bar;
    }

    /**
     * Returns the text boxed where commands are being typed.
     */
    inline CommandEntry& get_command_entry()
    {
      return m_command_entry;
    }

    /**
     * Returns the text boxed where commands are being typed.
     */
    inline const CommandEntry& get_command_entry() const
    {
      return m_command_entry;
    }

    /**
     * Returns all tabs that the window has.
     */
    std::vector<Tab*> get_tabs();

    /**
     * Returns all tabs that the window has.
     */
    std::vector<const Tab*> get_tabs() const;

    /**
     * Returns pointer to the current tab, or null pointer if no tabs are open.
     */
    Tab* get_current_tab();

    /**
     * Returns pointer to the current tab, or null pointer if no tabs are open.
     */
    const Tab* get_current_tab() const;

    void set_current_tab(const Glib::RefPtr<Tab>& tab);

    /**
      * Switches to the tab at the given zero-based index. Does nothing if the
      * index is out of range.
      */
    void set_current_tab(int index);

    /**
     * Returns pointer to tab from given index, or null pointer if given index
     * is out of bounds.
     */
    Tab* get_nth_tab(int index);

    /**
     * Returns pointer to tab from given index, or null pointer if given index
     * is out of bounds.
     */
    const Tab* get_nth_tab(int index) const;

    Glib::RefPtr<Tab> open_tab(
      const Glib::ustring& uri = Glib::ustring(),
      bool focus = true
    );

    void close_tab(Tab& tab);
    void close_tab(const Glib::RefPtr<Tab>& tab);

    /**
     * Opens a new tab and restores the last closed tab layout. Returns false
     * if there is no stored layout with at least one URI.
     */
    bool restore_last_closed_tab();

    /**
     * Captures every open tab with at least one URI, plus the active tab
     * index among those tabs.
     */
    Session capture_session() const;

    /**
     * Opens tabs from @p session without restoring homepage. Returns false
     * when nothing usable was restored.
     */
    bool restore_session(const Session& session);

    /**
     * Schedules a debounced write of the current session to disk.
     */
    void request_session_save();

    /**
     * Cancels any pending debounced save and writes the session immediately
     * when session saving is enabled. Pass @p force to write even when the
     * `session-save` setting is off (e.g. `:session-save`).
     */
    void flush_session_save(bool force = false);

    /**
     * Deletes the on-disk session file.
     */
    bool clear_session();

    /**
     * Returns pointer to view that is currently active in this window, or
     * `nullptr` if this window does not have active view at the moment.
     */
    View* get_active_view();

    /**
     * Returns pointer to view that is currently active in this window, or
     * `nullptr` if this window does not have active view at the moment.
     */
    const View* get_active_view() const;

    /**
     * Closes the window on the next main-loop idle. Use this from command /
     * signal handlers so MainWindow is not destroyed mid-callback.
     */
    void schedule_close();

    void next_tab();
    void prev_tab();

    /**
     * Returns the number of open tabs.
     */
    int get_tab_count() const;

    /**
     * Focuses @p view: switches to its tab and activates the split.
     */
    void focus_view(View& view);

    /**
     * Opens the keyboard-driven view list dialog.
     */
    void show_view_list_dialog();

    /**
     * Displays an info message in command entry.
     */
    void show_info(const Glib::ustring& text)
    {
      m_command_entry.show_notification(text, Notification::Type::INFO);
    }

    /**
     * Displays an error message in command entry. The given error message will
     * be prefixed with "Error: ".
     */
    void show_error(const Glib::ustring& text)
    {
      m_command_entry.show_notification(
        "Error: " + text,
        Notification::Type::ERROR
      );
    }

    /**
     * Unique URIs from session back/forward lists plus persistent visit
     * history, sorted alphabetically.
     */
    std::vector<Glib::ustring> get_history_uris() const;

    /**
     * Callback invoked when the user submits text collected by prompt().
     */
    using prompt_callback_type = std::function<void(const Glib::ustring&)>;

    /**
     * Focuses the command bar for free-form text input. Enter calls
     * @p on_submit with the typed value; Escape cancels without calling it.
     */
    void prompt(
      View* view,
      const Glib::ustring& message,
      bool sensitive,
      prompt_callback_type on_submit
    );

    /**
     * Cancels an in-progress prompt for @p view, if any.
     */
    void cancel_prompt_for(View* view);

  private:
    struct PromptRequest
    {
      View* view = nullptr;
      Glib::ustring message;
      bool sensitive = false;
      prompt_callback_type on_submit;
    };

    bool on_command_entry_key_pressed(
      guint keyval,
      guint,
      Gdk::ModifierType state
    );
    void on_command_received(const Glib::ustring& command);
    void on_tab_status_change(Tab* tab, const Glib::ustring& status);
    void on_tab_switch(Gtk::Widget* widget, guint page_number);
    bool on_tab_list_scroll(double dx, double dy);
    void clear_prompt_ui();
    void complete_prompt(const Glib::ustring& text);

  private:
    Glib::RefPtr<webkit::Context> m_web_context;
    Glib::RefPtr<webkit::Settings> m_web_settings;
    Glib::RefPtr<BookmarkManager> m_bookmark_manager;
    Glib::RefPtr<PasswordManager> m_password_manager;
    Glib::RefPtr<HistoryManager> m_history_manager;
    Mode m_mode;
    Gtk::Box m_box;
    Gtk::Notebook m_notebook;
    StatusBar m_status_bar;
    CommandEntry m_command_entry;
    Glib::RefPtr<Gtk::EventControllerKey> m_command_entry_key_controller;
    TabCompletion m_tab_completion;
    std::optional<PromptRequest> m_prompt;
    std::optional<LayoutSnapshot> m_last_closed_tab;
    std::unordered_set<Tab*> m_tabs_closing;
    sigc::connection m_session_save_connection;
    bool m_session_restoring = false;
  };
}
