/*
 * Copyright (c) 2026, Rauli Laine
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
#include <iostream>
#include <sstream>
#include <vector>

#include <glibmm.h>

#include <selain/userscript/metadata.hpp>
#include <selain/webkit/userscript.hpp>

namespace selain::webkit::userscript
{
  namespace
  {
    ::WebKitUserContentManager* g_manager = nullptr;
    std::vector<::WebKitUserScript*> g_scripts;
    bool g_enabled = true;

    bool
    is_script_filename(const std::filesystem::path& path)
    {
      const auto name = path.filename().string();

      if (name.empty() || name.front() == '.')
      {
        return false;
      }

      const auto ext = path.extension().string();

      return ext == ".js";
    }

    std::string
    read_file(const std::filesystem::path& path)
    {
      std::ifstream input(path, std::ios::binary);

      if (!input)
      {
        return {};
      }

      std::ostringstream buffer;

      buffer << input.rdbuf();

      return buffer.str();
    }

    std::vector<const gchar*>
    to_c_string_list(const std::vector<std::string>& patterns)
    {
      std::vector<const gchar*> list;

      list.reserve(patterns.size() + 1);
      for (const auto& pattern : patterns)
      {
        list.push_back(pattern.c_str());
      }
      list.push_back(nullptr);

      return list;
    }

    void
    clear_installed_scripts()
    {
      if (g_manager)
      {
        for (auto* script : g_scripts)
        {
          ::webkit_user_content_manager_remove_script(g_manager, script);
        }
      }
      for (auto* script : g_scripts)
      {
        ::webkit_user_script_unref(script);
      }
      g_scripts.clear();
    }

    void
    install_parsed(
      const selain::userscript::ParsedScript& parsed,
      const std::filesystem::path& path
    )
    {
      for (const auto& warning : parsed.warnings)
      {
        std::cerr
          << "selain: userscript " << path.filename().string()
          << ": " << warning
          << std::endl;
      }

      if (!parsed.is_installable())
      {
        return;
      }

      const auto allow = to_c_string_list(parsed.allow_list);
      const auto block = to_c_string_list(parsed.block_list);
      const auto frames = parsed.noframes
        ? WEBKIT_USER_CONTENT_INJECT_TOP_FRAME
        : WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES;
      const auto time = parsed.run_at == selain::userscript::RunAt::DocumentStart
        ? WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START
        : WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END;
      auto* script = ::webkit_user_script_new(
        parsed.source.c_str(),
        frames,
        time,
        allow.data(),
        parsed.block_list.empty() ? nullptr : block.data()
      );

      if (!script)
      {
        std::cerr
          << "selain: failed to create userscript from "
          << path.string()
          << std::endl;
        return;
      }

      g_scripts.push_back(script);

      if (g_manager && g_enabled)
      {
        ::webkit_user_content_manager_add_script(g_manager, script);
      }
    }

    void
    load_directory()
    {
      clear_installed_scripts();

      const auto directory = scripts_directory();
      std::error_code ec;

      if (!std::filesystem::exists(directory, ec))
      {
        return;
      }
      if (!std::filesystem::is_directory(directory, ec))
      {
        std::cerr
          << "selain: userscripts path is not a directory: "
          << directory.string()
          << std::endl;
        return;
      }

      std::vector<std::filesystem::path> files;

      for (const auto& entry :
           std::filesystem::directory_iterator(directory, ec))
      {
        if (ec)
        {
          break;
        }
        if (!entry.is_regular_file(ec) || ec)
        {
          continue;
        }
        if (!is_script_filename(entry.path()))
        {
          continue;
        }
        files.push_back(entry.path());
      }

      std::sort(files.begin(), files.end());

      for (const auto& path : files)
      {
        const auto source = read_file(path);

        if (source.empty())
        {
          std::cerr
            << "selain: failed to read userscript: "
            << path.string()
            << std::endl;
          continue;
        }

        const auto parsed = selain::userscript::parse(source);

        if (!parsed)
        {
          std::cerr
            << "selain: userscript "
            << path.filename().string()
            << ": missing ==UserScript== metadata block; skipped."
            << std::endl;
          continue;
        }

        install_parsed(*parsed, path);
      }
    }
  }

  std::filesystem::path
  scripts_directory()
  {
    return std::filesystem::path(
      Glib::build_filename(
        Glib::get_user_config_dir(),
        "selain",
        "scripts"
      )
    );
  }

  void
  initialize(::WebKitUserContentManager* manager)
  {
    if (!manager)
    {
      return;
    }

    g_manager = manager;
    load_directory();
  }

  void
  reload(::WebKitUserContentManager* manager)
  {
    if (manager)
    {
      g_manager = manager;
    }
    if (!g_manager)
    {
      return;
    }

    load_directory();
  }

  void
  shutdown()
  {
    clear_installed_scripts();
    g_manager = nullptr;
    g_enabled = true;
  }

  bool
  is_enabled()
  {
    return g_enabled;
  }

  void
  set_enabled(bool enabled)
  {
    if (g_enabled == enabled)
    {
      return;
    }

    g_enabled = enabled;

    if (!g_manager)
    {
      return;
    }

    if (!enabled)
    {
      for (auto* script : g_scripts)
      {
        ::webkit_user_content_manager_remove_script(g_manager, script);
      }
      return;
    }

    for (auto* script : g_scripts)
    {
      ::webkit_user_content_manager_add_script(g_manager, script);
    }
  }
}
