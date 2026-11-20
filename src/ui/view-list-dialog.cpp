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
#include <algorithm>

#include <selain/ui/main-window.hpp>
#include <selain/ui/view-list-dialog.hpp>
#include <selain/utils.hpp>

namespace selain::ui
{
  namespace
  {
    Glib::ustring
    view_label(const View& view)
    {
      const auto title = view.get_title();

      if (!title.empty())
      {
        return title;
      }

      const auto uri = view.get_uri();

      return uri.empty() ? Glib::ustring("Untitled") : uri;
    }

    Glib::ustring
    view_haystack(const View& view)
    {
      return view_label(view) + ' ' + view.get_uri();
    }
  }

  class ViewListDialog::Row : public Gtk::ListBoxRow
  {
  public:
    explicit Row(View& view)
      : m_view(&view)
      , m_box(Gtk::Orientation::HORIZONTAL)
      , m_text_box(Gtk::Orientation::VERTICAL)
      , m_title(view_label(view))
      , m_uri(view.get_uri())
    {
      add_css_class("view-row");

      m_icon.add_css_class("favicon");
      m_icon.set_from_icon_name("text-x-generic");
      if (const auto favicon = view.get_favicon())
      {
        m_icon.set(favicon);
      }
      m_icon.set_margin_end(8);
      m_icon.set_pixel_size(16);
      m_icon.set_valign(Gtk::Align::START);

      m_title.add_css_class("title-label");
      m_title.set_halign(Gtk::Align::START);
      m_title.set_hexpand(true);
      m_title.set_ellipsize(Pango::EllipsizeMode::END);
      m_title.set_xalign(0.0f);

      m_uri.add_css_class("uri-label");
      m_uri.set_halign(Gtk::Align::START);
      m_uri.set_hexpand(true);
      m_uri.set_ellipsize(Pango::EllipsizeMode::END);
      m_uri.set_xalign(0.0f);

      m_text_box.set_hexpand(true);
      m_text_box.append(m_title);
      m_text_box.append(m_uri);

      m_box.set_margin_start(8);
      m_box.set_margin_end(8);
      m_box.set_margin_top(4);
      m_box.set_margin_bottom(4);
      m_box.append(m_icon);
      m_box.append(m_text_box);
      set_child(m_box);
    }

    View* get_view() const
    {
      return m_view;
    }

  private:
    View* m_view;
    Gtk::Box m_box;
    Gtk::Box m_text_box;
    Gtk::Image m_icon;
    Gtk::Label m_title;
    Gtk::Label m_uri;
  };

  void
  ViewListDialog::show(MainWindow& window)
  {
    auto* dialog = new ViewListDialog(window);

    dialog->present();
    dialog->m_filter.grab_focus();
  }

  ViewListDialog::ViewListDialog(MainWindow& window)
    : m_window(window)
    , m_box(Gtk::Orientation::VERTICAL)
    , m_key_controller(Gtk::EventControllerKey::create())
  {
    set_title("Views");
    set_modal(true);
    set_transient_for(window);
    set_destroy_with_parent(true);
    set_default_size(480, 360);
    set_hide_on_close(true);
    add_css_class("selain-view-list-dialog");

    m_filter.add_css_class("filter-entry");
    m_filter.set_placeholder_text("Filter views…");
    m_filter.set_hexpand(true);
    m_filter.set_margin_start(8);
    m_filter.set_margin_end(8);
    m_filter.set_margin_top(8);
    m_filter.set_margin_bottom(4);

    m_list.add_css_class("view-list");
    m_list.set_selection_mode(Gtk::SelectionMode::SINGLE);
    m_list.set_activate_on_single_click(true);

    m_scrolled.set_policy(
      Gtk::PolicyType::AUTOMATIC,
      Gtk::PolicyType::AUTOMATIC
    );
    m_scrolled.set_vexpand(true);
    m_scrolled.set_child(m_list);

    m_box.set_spacing(0);
    m_box.append(m_filter);
    m_box.append(m_scrolled);
    set_child(m_box);

    m_filter.signal_changed().connect(sigc::mem_fun(
      *this,
      &ViewListDialog::on_filter_changed
    ));
    m_filter.signal_activate().connect(sigc::mem_fun(
      *this,
      &ViewListDialog::activate_selection
    ));
    m_list.signal_row_activated().connect(sigc::mem_fun(
      *this,
      &ViewListDialog::on_row_activated
    ));

    m_key_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    m_key_controller->signal_key_pressed().connect(
      sigc::mem_fun(*this, &ViewListDialog::on_key_pressed),
      false
    );
    add_controller(m_key_controller);

    signal_close_request().connect(
      [this]() -> bool
      {
        close_dialog();

        return true;
      },
      false
    );

    populate();
  }

  ViewListDialog::~ViewListDialog()
  {
    while (auto* child = m_box.get_first_child())
    {
      m_box.remove(*child);
    }
  }

  void
  ViewListDialog::populate()
  {
    // collect_views() replaces its output vector, so gather per-tab then merge.
    m_views.clear();
    for (int i = 0; i < m_window.get_tab_count(); ++i)
    {
      if (auto* tab = m_window.get_nth_tab(i))
      {
        std::vector<View*> tab_views;

        tab->collect_views(tab_views);
        m_views.insert(
          std::end(m_views),
          std::begin(tab_views),
          std::end(tab_views)
        );
      }
    }

    apply_filter();
  }

  void
  ViewListDialog::apply_filter()
  {
    const auto query = m_filter.get_text();
    struct Ranked
    {
      View* view;
      int score;
    };
    std::vector<Ranked> ranked;

    ranked.reserve(m_views.size());
    for (auto* view : m_views)
    {
      if (!view)
      {
        continue;
      }
      if (query.empty())
      {
        ranked.push_back({ view, 0 });
        continue;
      }
      if (const auto score = utils::fuzzy_match_score(
            view_haystack(*view),
            query
          ))
      {
        ranked.push_back({ view, *score });
      }
    }

    if (!query.empty())
    {
      std::stable_sort(
        std::begin(ranked),
        std::end(ranked),
        [](const Ranked& a, const Ranked& b)
        {
          return a.score > b.score;
        }
      );
    }

    View* preferred = nullptr;

    if (auto* tab = m_window.get_current_tab())
    {
      preferred = tab->get_active_view();
    }

    m_list.remove_all();
    m_filtered.clear();
    m_filtered.reserve(ranked.size());

    int preferred_index = 0;

    for (const auto& entry : ranked)
    {
      auto* row = Gtk::make_managed<Row>(*entry.view);

      if (entry.view == preferred)
      {
        preferred_index = static_cast<int>(m_filtered.size());
      }
      m_list.append(*row);
      m_filtered.push_back(entry.view);
    }

    if (!m_filtered.empty())
    {
      select_index(query.empty() ? preferred_index : 0);
    }
  }

  void
  ViewListDialog::select_index(int index)
  {
    if (m_filtered.empty())
    {
      return;
    }

    const auto count = static_cast<int>(m_filtered.size());
    const auto clamped = std::clamp(index, 0, count - 1);

    if (auto* row = m_list.get_row_at_index(clamped))
    {
      m_list.select_row(*row);
      row->grab_focus();
      // Keep typing focus in the filter; selection is visual only.
      m_filter.grab_focus_without_selecting();
    }
  }

  void
  ViewListDialog::move_selection(int delta)
  {
    if (m_filtered.empty())
    {
      return;
    }

    auto* selected = m_list.get_selected_row();
    const int current = selected ? selected->get_index() : 0;
    const auto count = static_cast<int>(m_filtered.size());
    auto next = current + delta;

    if (next < 0)
    {
      next = count - 1;
    }
    else if (next >= count)
    {
      next = 0;
    }
    select_index(next);
  }

  void
  ViewListDialog::activate_selection()
  {
    auto* row = m_list.get_selected_row();

    if (!row && !m_filtered.empty())
    {
      row = m_list.get_row_at_index(0);
    }
    if (!row)
    {
      return;
    }

    on_row_activated(row);
  }

  void
  ViewListDialog::close_dialog()
  {
    set_visible(false);
    Glib::signal_idle().connect_once([this]() {
      delete this;
    });
  }

  bool
  ViewListDialog::on_key_pressed(guint keyval, guint, Gdk::ModifierType)
  {
    switch (keyval)
    {
      case GDK_KEY_Escape:
        close_dialog();
        return true;

      case GDK_KEY_Up:
      case GDK_KEY_KP_Up:
        move_selection(-1);
        return true;

      case GDK_KEY_Down:
      case GDK_KEY_KP_Down:
        move_selection(1);
        return true;

      case GDK_KEY_Return:
      case GDK_KEY_KP_Enter:
        activate_selection();
        return true;

      default:
        return false;
    }
  }

  void
  ViewListDialog::on_filter_changed()
  {
    apply_filter();
  }

  void
  ViewListDialog::on_row_activated(Gtk::ListBoxRow* row)
  {
    auto* view_row = dynamic_cast<Row*>(row);

    if (!view_row || !view_row->get_view())
    {
      return;
    }

    auto* view = view_row->get_view();

    close_dialog();
    m_window.focus_view(*view);
  }
}
