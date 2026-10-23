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

#include <selain/main-window.hpp>
#include <selain/setting.hpp>
#include <selain/tab-completion.hpp>
#include <selain/utils.hpp>

namespace selain
{
  namespace
  {
    Glib::ustring
    longest_common_prefix(const std::vector<Glib::ustring>& matches)
    {
      if (matches.empty())
      {
        return Glib::ustring();
      }

      Glib::ustring prefix = matches.front();

      for (std::size_t i = 1; i < matches.size(); ++i)
      {
        const auto& candidate = matches[i];
        std::size_t n = 0;
        const auto limit = std::min(prefix.length(), candidate.length());

        while (n < limit && prefix[n] == candidate[n])
        {
          ++n;
        }
        prefix = prefix.substr(0, n);
        if (prefix.empty())
        {
          break;
        }
      }

      return prefix;
    }

    std::vector<Glib::ustring>
    filter_prefix(const std::vector<Glib::ustring>& candidates,
                  const Glib::ustring& prefix)
    {
      std::vector<Glib::ustring> matches;

      for (const auto& candidate : candidates)
      {
        if (candidate.size() >= prefix.size() &&
            candidate.compare(0, prefix.size(), prefix) == 0)
        {
          matches.push_back(candidate);
        }
      }

      return matches;
    }

    bool
    is_set_command(const Glib::ustring& name)
    {
      return name == "set" || name == "se";
    }

    bool
    is_uri_command(const Glib::ustring& name)
    {
      return name == "open" || name == "o" ||
             name == "open-tab" || name == "ot" ||
             name == "split" || name == "sp" ||
             name == "vsplit" || name == "vs" || name == "vsp";
    }
  }

  void
  TabCompletion::reset()
  {
    m_matches.clear();
    m_index = 0;
  }

  void
  TabCompletion::complete(MainWindow& window, bool reverse)
  {
    auto& entry = window.get_command_entry();
    const auto text = entry.get_text();

    if (text.empty() || text[0] != ':')
    {
      return;
    }

    const auto body = text.substr(1);
    const auto space = body.find(' ');
    const bool completing_setting = space != Glib::ustring::npos;
    Glib::ustring line_prefix;
    Glib::ustring token;
    std::vector<Glib::ustring> candidates;

    if (!completing_setting)
    {
      // Complete command name: ":op" → ":open"
      line_prefix = ":";
      token = body;
      for (const auto& [key, command] : window.get_command_mapping())
      {
        if (key == command.name ||
            (command.name_shortcut && key == command.name_shortcut))
        {
          candidates.push_back(key);
        }
      }
      std::sort(std::begin(candidates), std::end(candidates));
      candidates.erase(
        std::unique(std::begin(candidates), std::end(candidates)),
        std::end(candidates)
      );
    } else {
      const auto command_name = utils::trim(body.substr(0, space));
      const auto args = body.substr(space + 1);

      line_prefix = ":" + command_name + " ";
      token = utils::trim(args);

      if (is_set_command(command_name))
      {
        if (args.find('=') != Glib::ustring::npos)
        {
          return;
        }
        // Complete setting name: ":set en" → ":set enable-javascript"
        candidates = setting::names();
      }
      else if (is_uri_command(command_name))
      {
        // Complete from browsing history of every tab/split.
        candidates = window.get_history_uris();
      } else {
        return;
      }
    }

    const auto token_is_current_match = std::find(
      std::begin(m_matches),
      std::end(m_matches),
      token
    ) != std::end(m_matches);

    if (!token_is_current_match)
    {
      m_matches = filter_prefix(candidates, token);
      m_index = 0;

      if (m_matches.empty())
      {
        return;
      }

      const auto common = longest_common_prefix(m_matches);

      // Bash-style: first expand to the shared prefix when it grows the token.
      if (common.length() > token.length() && common != m_matches.front())
      {
        entry.set_text(line_prefix + common);
        return;
      }
    }
    else if (m_matches.size() > 1)
    {
      const auto current = std::find(
        std::begin(m_matches),
        std::end(m_matches),
        token
      );

      m_index = static_cast<std::size_t>(
        std::distance(std::begin(m_matches), current)
      );
      if (reverse)
      {
        m_index = (m_index + m_matches.size() - 1) % m_matches.size();
      } else {
        m_index = (m_index + 1) % m_matches.size();
      }
    }

    entry.set_text(line_prefix + m_matches[m_index]);
  }
}
