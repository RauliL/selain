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

#include <selain/ui/gtk-compat.hpp>
#include <selain/ui/list-dialog.hpp>
#include <selain/utils/string.hpp>

namespace selain::ui
{
  static Glib::ustring
  default_haystack(const ListDialogEntry& entry)
  {
    if (!entry.filter_text.empty())
    {
      return entry.filter_text;
    }

    return entry.title + ' ' + entry.secondary;
  }

  class ListDialog::Row : public Gtk::ListBoxRow
  {
  public:
    Row(const ListDialogEntry& entry, std::size_t index)
      : m_index(index)
      , m_box(Gtk::Orientation::HORIZONTAL)
      , m_text_box(Gtk::Orientation::VERTICAL)
      , m_title(entry.title)
      , m_secondary(entry.secondary)
    {
      add_css_class("list-row");

      m_icon.add_css_class("favicon");
      if (entry.icon)
      {
        m_icon.set(entry.icon);
      }
      else
      {
        m_icon.set_from_icon_name(entry.icon_name);
      }
      m_icon.set_margin_end(8);
      m_icon.set_pixel_size(16);
      m_icon.set_valign(Gtk::Align::START);

      m_title.add_css_class("title-label");
      m_title.set_halign(Gtk::Align::START);
      m_title.set_hexpand(true);
      m_title.set_ellipsize(Pango::EllipsizeMode::END);
      m_title.set_xalign(0.0f);

      m_secondary.add_css_class("secondary-label");
      m_secondary.set_halign(Gtk::Align::START);
      m_secondary.set_hexpand(true);
      m_secondary.set_ellipsize(Pango::EllipsizeMode::END);
      m_secondary.set_xalign(0.0f);

      m_text_box.set_hexpand(true);
      m_text_box.append(m_title);
      m_text_box.append(m_secondary);

      m_box.set_margin_start(8);
      m_box.set_margin_end(8);
      m_box.set_margin_top(4);
      m_box.set_margin_bottom(4);
      m_box.append(m_icon);
      m_box.append(m_text_box);
      set_child(m_box);
    }

    std::size_t get_index() const
    {
      return m_index;
    }

  private:
    std::size_t m_index;
    Gtk::Box m_box;
    Gtk::Box m_text_box;
    Gtk::Image m_icon;
    Gtk::Label m_title;
    Gtk::Label m_secondary;
  };

  void
  ListDialog::show(
    Gtk::Window& parent,
    const Glib::ustring& title,
    const Glib::ustring& filter_placeholder,
    std::vector<ListDialogEntry> entries,
    std::function<void(std::size_t index)> on_activate,
    int preferred_index,
    const Glib::ustring& initial_filter
  )
  {
    auto* dialog = new ListDialog(
      parent,
      title,
      filter_placeholder,
      std::move(entries),
      std::move(on_activate),
      preferred_index,
      initial_filter
    );

    dialog->present();
    dialog->m_filter.grab_focus();
  }

  ListDialog::ListDialog(
    Gtk::Window& parent,
    const Glib::ustring& title,
    const Glib::ustring& filter_placeholder,
    std::vector<ListDialogEntry> entries,
    std::function<void(std::size_t index)> on_activate,
    int preferred_index,
    const Glib::ustring& initial_filter
  )
    : m_box(Gtk::Orientation::VERTICAL)
    , m_key_controller(Gtk::EventControllerKey::create())
    , m_entries(std::move(entries))
    , m_on_activate(std::move(on_activate))
    , m_preferred_index(preferred_index)
  {
    set_title(title);
    set_modal(true);
    set_transient_for(parent);
    set_destroy_with_parent(true);
    set_default_size(480, 360);
    set_hide_on_close(true);
    add_css_class("selain-list-dialog");

    m_haystacks.reserve(m_entries.size());
    for (const auto& entry : m_entries)
    {
      m_haystacks.push_back(default_haystack(entry));
    }

    m_filter.add_css_class("filter-entry");
    m_filter.set_placeholder_text(filter_placeholder);
    m_filter.set_hexpand(true);
    m_filter.set_margin_start(8);
    m_filter.set_margin_end(8);
    m_filter.set_margin_top(8);
    m_filter.set_margin_bottom(4);
    if (!initial_filter.empty())
    {
      m_filter.set_text(initial_filter);
    }

    m_list.add_css_class("entry-list");
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
      &ListDialog::on_filter_changed
    ));
    m_filter.signal_activate().connect(sigc::mem_fun(
      *this,
      &ListDialog::activate_selection
    ));
    m_list.signal_row_activated().connect(sigc::mem_fun(
      *this,
      &ListDialog::on_row_activated
    ));

    m_key_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    m_key_controller->signal_key_pressed().connect(
      sigc::mem_fun(*this, &ListDialog::on_key_pressed),
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

    apply_filter();
  }

  ListDialog::~ListDialog()
  {
    while (auto* child = m_box.get_first_child())
    {
      m_box.remove(*child);
    }
  }

  const Glib::ustring&
  ListDialog::entry_haystack(std::size_t index) const
  {
    return m_haystacks[index];
  }

  void
  ListDialog::apply_filter()
  {
    const auto query = m_filter.get_text();
    struct Ranked
    {
      std::size_t index;
      int score;
    };
    std::vector<Ranked> ranked;

    ranked.reserve(m_entries.size());
    for (std::size_t i = 0; i < m_entries.size(); ++i)
    {
      if (query.empty())
      {
        ranked.push_back({ i, 0 });
        continue;
      }
      if (const auto score = utils::string::fuzzy_match_score(
        entry_haystack(i),
        query
      ))
      {
        ranked.push_back({ i, *score });
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

    gtk_compat::remove_all(m_list);
    m_filtered.clear();
    m_filtered.reserve(ranked.size());

    int preferred_row = 0;

    for (const auto& entry : ranked)
    {
      auto* row = Gtk::make_managed<Row>(
        m_entries[entry.index],
        entry.index
      );

      if (static_cast<int>(entry.index) == m_preferred_index)
      {
        preferred_row = static_cast<int>(m_filtered.size());
      }
      m_list.append(*row);
      m_filtered.push_back(entry.index);
    }

    if (!m_filtered.empty())
    {
      select_index(query.empty() ? preferred_row : 0);
    }
  }

  void
  ListDialog::select_index(int index)
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
  ListDialog::move_selection(int delta)
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
  ListDialog::activate_selection()
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
  ListDialog::close_dialog()
  {
    set_visible(false);
    Glib::signal_idle().connect_once([this]() {
      delete this;
    });
  }

  bool
  ListDialog::on_key_pressed(guint keyval, guint, Gdk::ModifierType)
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
  ListDialog::on_filter_changed()
  {
    apply_filter();
  }

  void
  ListDialog::on_row_activated(Gtk::ListBoxRow* row)
  {
    auto* list_row = dynamic_cast<Row*>(row);

    if (!list_row || !m_on_activate)
    {
      return;
    }

    const auto index = list_row->get_index();
    auto on_activate = std::move(m_on_activate);

    m_on_activate = nullptr;
    close_dialog();
    on_activate(index);
  }
}
