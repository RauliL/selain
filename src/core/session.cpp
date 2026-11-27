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
#include <fstream>
#include <sstream>

#include <glibmm.h>
#include <inja/inja.hpp>

#include <selain/core/session.hpp>
#include <selain/utils/json.hpp>

namespace selain
{
  namespace
  {
    bool g_restore_enabled = true;
    bool g_save_enabled = true;

    constexpr int k_session_version = 2;
    constexpr int k_closed_tabs_version = 1;

    inja::json
    encode_layout_node(const ui::LayoutNode& node)
    {
      inja::json json;

      if (node.kind == ui::LayoutNode::Kind::Leaf)
      {
        json["type"] = "leaf";
        json["uri"] = node.uri ? node.uri->raw() : std::string();
        json["zoom"] = node.zoom;
        return json;
      }

      json["type"] = "split";
      json["orientation"] =
        node.orientation == Gtk::Orientation::VERTICAL
          ? "vertical"
          : "horizontal";
      if (node.ratio)
      {
        json["ratio"] = *node.ratio;
      }
      if (node.start)
      {
        json["start"] = encode_layout_node(*node.start);
      }
      if (node.end)
      {
        json["end"] = encode_layout_node(*node.end);
      }

      return json;
    }

    std::unique_ptr<ui::LayoutNode>
    decode_layout_node(const inja::json& json)
    {
      if (!json.is_object())
      {
        return nullptr;
      }

      const auto type = utils::json::get_string(json, "type");

      if (type == "leaf")
      {
        auto node = std::make_unique<ui::LayoutNode>();

        node->kind = ui::LayoutNode::Kind::Leaf;
        if (
          const auto uri = utils::json::get_string(json, "uri");
          !uri.empty()
        )
        {
          node->uri = uri;
        }
        if (const auto zoom = utils::json::get_number(json, "zoom"))
        {
          node->zoom = *zoom > 0.0 ? *zoom : 1.0;
        }

        return node;
      }

      if (type != "split")
      {
        return nullptr;
      }

      auto node = std::make_unique<ui::LayoutNode>();

      node->kind = ui::LayoutNode::Kind::Split;
      if (utils::json::get_string(json, "orientation") == "vertical")
      {
        node->orientation = Gtk::Orientation::VERTICAL;
      } else {
        node->orientation = Gtk::Orientation::HORIZONTAL;
      }
      if (const auto ratio = utils::json::get_number(json, "ratio"))
      {
        node->ratio = std::clamp(*ratio, 0.0, 1.0);
      }

      if (const auto it = json.find("start"); it != json.end())
      {
        node->start = decode_layout_node(*it);
      }
      if (const auto it = json.find("end"); it != json.end())
      {
        node->end = decode_layout_node(*it);
      }
      if (!node->start || !node->end)
      {
        return nullptr;
      }

      return node;
    }

    inja::json
    encode_layout_snapshot(const ui::LayoutSnapshot& snapshot)
    {
      inja::json json;

      json["active_leaf"] = snapshot.active_leaf_index;
      if (snapshot.root)
      {
        json["root"] = encode_layout_node(*snapshot.root);
      }

      return json;
    }

    std::optional<ui::LayoutSnapshot>
    decode_layout_snapshot(const inja::json& json)
    {
      if (!json.is_object())
      {
        return std::nullopt;
      }

      ui::LayoutSnapshot snapshot;

      if (const auto it = json.find("active_leaf");
          it != json.end() && it->is_number_integer())
      {
        snapshot.active_leaf_index = it->get<int>();
      }
      if (const auto it = json.find("root"); it != json.end())
      {
        snapshot.root = decode_layout_node(*it);
      }
      if (!snapshot.root || !snapshot.has_any_uri())
      {
        return std::nullopt;
      }
      if (snapshot.active_leaf_index < 0)
      {
        snapshot.active_leaf_index = 0;
      }

      return snapshot;
    }

    inja::json
    encode_session(const Session& session)
    {
      inja::json json;

      json["version"] = k_session_version;
      json["active_tab"] = session.active_tab;
      json["tabs"] = inja::json::array();
      for (const auto& tab : session.tabs)
      {
        json["tabs"].push_back(encode_layout_snapshot(tab));
      }

      return json;
    }

    std::optional<Session>
    decode_session(const inja::json& json)
    {
      if (!json.is_object())
      {
        return std::nullopt;
      }

      {
        const auto it = json.find("version");

        if (it == json.end() || !it->is_number_integer())
        {
          return std::nullopt;
        }

        const auto version = it->get<int>();

        // v1: URIs only; v2: optional zoom/ratio on layout nodes.
        if (version != 1 && version != k_session_version)
        {
          return std::nullopt;
        }
      }

      const auto tabs_it = json.find("tabs");

      if (tabs_it == json.end() || !tabs_it->is_array())
      {
        return std::nullopt;
      }

      Session session;

      if (const auto it = json.find("active_tab");
          it != json.end() && it->is_number_integer())
      {
        session.active_tab = it->get<int>();
      }

      for (const auto& tab_json : *tabs_it)
      {
        if (auto snapshot = decode_layout_snapshot(tab_json))
        {
          session.tabs.push_back(std::move(*snapshot));
        }
      }

      if (session.tabs.empty())
      {
        return std::nullopt;
      }
      if (session.active_tab < 0)
      {
        session.active_tab = 0;
      }
      if (session.active_tab >= static_cast<int>(session.tabs.size()))
      {
        session.active_tab = static_cast<int>(session.tabs.size()) - 1;
      }

      return session;
    }

    bool
    ensure_parent_directory(const std::filesystem::path& path)
    {
      std::error_code error;

      std::filesystem::create_directories(path.parent_path(), error);
      if (error)
      {
        g_warning(
          "Selain: failed to create session directory: %s",
          error.message().c_str()
        );
        return false;
      }

      return true;
    }

    bool
    write_atomic(const std::filesystem::path& target, const std::string& text)
    {
      if (!ensure_parent_directory(target))
      {
        return false;
      }

      const auto temp = target.string() + ".tmp";
      {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);

        if (!out)
        {
          g_warning(
            "Selain: failed to open temp file: %s",
            temp.c_str()
          );
          return false;
        }
        out << text;
        if (!out)
        {
          g_warning(
            "Selain: failed to write temp file: %s",
            temp.c_str()
          );
          std::filesystem::remove(temp);
          return false;
        }
      }

      std::error_code error;

      std::filesystem::rename(temp, target, error);
      if (error)
      {
        g_warning(
          "Selain: failed to replace file %s: %s",
          target.string().c_str(),
          error.message().c_str()
        );
        std::filesystem::remove(temp);
        return false;
      }

      return true;
    }

    std::optional<inja::json>
    read_json_file(const std::filesystem::path& target)
    {
      if (!std::filesystem::exists(target))
      {
        return std::nullopt;
      }

      std::ifstream in(target, std::ios::binary);

      if (!in)
      {
        g_warning(
          "Selain: failed to open file: %s",
          target.string().c_str()
        );
        return std::nullopt;
      }

      std::ostringstream buffer;

      buffer << in.rdbuf();
      if (!in && !in.eof())
      {
        g_warning(
          "Selain: failed to read file: %s",
          target.string().c_str()
        );
        return std::nullopt;
      }

      const auto parsed = inja::json::parse(buffer.str(), nullptr, false);

      if (parsed.is_discarded())
      {
        g_warning(
          "Selain: ignoring corrupt file: %s",
          target.string().c_str()
        );
        return std::nullopt;
      }

      return parsed;
    }

    bool
    remove_file(const std::filesystem::path& target)
    {
      if (!std::filesystem::exists(target))
      {
        return true;
      }

      std::error_code error;

      std::filesystem::remove(target, error);
      if (error)
      {
        g_warning(
          "Selain: failed to remove file %s: %s",
          target.string().c_str(),
          error.message().c_str()
        );
        return false;
      }

      return true;
    }

    inja::json
    encode_closed_tabs(const std::vector<ui::LayoutSnapshot>& tabs)
    {
      inja::json json;

      json["version"] = k_closed_tabs_version;
      json["tabs"] = inja::json::array();
      for (const auto& tab : tabs)
      {
        json["tabs"].push_back(encode_layout_snapshot(tab));
      }

      return json;
    }

    std::vector<ui::LayoutSnapshot>
    decode_closed_tabs(const inja::json& json)
    {
      std::vector<ui::LayoutSnapshot> tabs;

      if (!json.is_object())
      {
        return tabs;
      }
      if (const auto it = json.find("version");
          it == json.end() || !it->is_number_integer() ||
          it->get<int>() != k_closed_tabs_version)
      {
        return tabs;
      }

      const auto tabs_it = json.find("tabs");

      if (tabs_it == json.end() || !tabs_it->is_array())
      {
        return tabs;
      }

      for (const auto& tab_json : *tabs_it)
      {
        if (auto snapshot = decode_layout_snapshot(tab_json))
        {
          tabs.push_back(std::move(*snapshot));
        }
      }

      return tabs;
    }
  }

  std::filesystem::path
  SessionStore::default_path()
  {
    return std::filesystem::path(
      Glib::build_filename(
        Glib::get_user_data_dir(),
        "selain",
        "session.json"
      )
    );
  }

  bool
  SessionStore::save(
    const Session& session,
    const std::filesystem::path& path
  )
  {
    const auto target = path.empty() ? default_path() : path;

    if (session.tabs.empty())
    {
      return clear(target);
    }

    return write_atomic(target, encode_session(session).dump(2));
  }

  std::optional<Session>
  SessionStore::load(const std::filesystem::path& path)
  {
    const auto target = path.empty() ? default_path() : path;
    const auto parsed = read_json_file(target);

    if (!parsed)
    {
      return std::nullopt;
    }

    auto session = decode_session(*parsed);

    if (!session)
    {
      g_warning(
        "Selain: ignoring invalid session file: %s",
        target.string().c_str()
      );
    }

    return session;
  }

  bool
  SessionStore::clear(const std::filesystem::path& path)
  {
    return remove_file(path.empty() ? default_path() : path);
  }

  std::filesystem::path
  ClosedTabStore::default_path()
  {
    return std::filesystem::path(
      Glib::build_filename(
        Glib::get_user_data_dir(),
        "selain",
        "closed-tabs.json"
      )
    );
  }

  bool
  ClosedTabStore::save(
    const std::vector<ui::LayoutSnapshot>& tabs,
    const std::filesystem::path& path
  )
  {
    const auto target = path.empty() ? default_path() : path;

    if (tabs.empty())
    {
      return clear(target);
    }

    return write_atomic(target, encode_closed_tabs(tabs).dump(2));
  }

  std::vector<ui::LayoutSnapshot>
  ClosedTabStore::load(const std::filesystem::path& path)
  {
    const auto target = path.empty() ? default_path() : path;
    const auto parsed = read_json_file(target);

    if (!parsed)
    {
      return {};
    }

    auto tabs = decode_closed_tabs(*parsed);

    if (tabs.empty() && parsed->contains("tabs"))
    {
      g_warning(
        "Selain: ignoring invalid closed-tabs file: %s",
        target.string().c_str()
      );
    }

    return tabs;
  }

  bool
  ClosedTabStore::clear(const std::filesystem::path& path)
  {
    return remove_file(path.empty() ? default_path() : path);
  }

  namespace session
  {
    bool
    is_restore_enabled()
    {
      return g_restore_enabled;
    }

    void
    set_restore_enabled(bool enabled)
    {
      g_restore_enabled = enabled;
    }

    bool
    is_save_enabled()
    {
      return g_save_enabled;
    }

    void
    set_save_enabled(bool enabled)
    {
      g_save_enabled = enabled;
    }
  }
}
