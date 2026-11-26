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
#include <filesystem>

#include <giomm.h>

#include <selain/integration/pass.hpp>
#include <selain/utils/url.hpp>

namespace selain::integration::pass
{
  static inline bool
  is_dot_or_git_dir(const std::filesystem::path& path)
  {
    const auto name = path.filename().string();

    return name == ".git" || (name.size() > 1 && name[0] == '.');
  }

  static std::vector<Glib::ustring>
  list_entries(const std::filesystem::path& root)
  {
    std::vector<Glib::ustring> entries;
    std::error_code ec;

    if (!std::filesystem::is_directory(root, ec))
    {
      return entries;
    }

    const auto options =
      std::filesystem::directory_options::skip_permission_denied;
    auto it = std::filesystem::recursive_directory_iterator(root, options, ec);

    if (ec)
    {
      return entries;
    }

    for (; it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
    {
      if (ec)
      {
        ec.clear();
        continue;
      }

      const auto& path = it->path();

      if (it->is_directory(ec) && is_dot_or_git_dir(path.filename()))
      {
        it.disable_recursion_pending();
        continue;
      }
      if (!it->is_regular_file(ec) || path.extension() != ".gpg")
      {
        continue;
      }

      auto relative = std::filesystem::relative(path, root, ec);

      if (ec || relative.empty())
      {
        continue;
      }
      relative.replace_extension();
      entries.emplace_back(relative.generic_string());
    }

    std::sort(std::begin(entries), std::end(entries));

    return entries;
  }

  static std::vector<Glib::ustring>
  domain_candidates(const Glib::ustring& host)
  {
    std::vector<Glib::ustring> domains;
    const Glib::ustring lowered = host.lowercase();

    // Keep private / IP hosts as-is; do not strip labels.
    if (selain::utils::url::looks_like_ip(lowered))
    {
      domains.push_back(lowered);

      return domains;
    }

    Glib::ustring current = lowered;

    while (!current.empty())
    {
      domains.push_back(current);

      const auto dot = current.find('.');

      if (dot == Glib::ustring::npos)
      {
        break;
      }

      const auto next = current.substr(dot + 1);

      // Do not fall through to a single TLD label (e.g. "com").
      if (next.find('.') == Glib::ustring::npos)
      {
        break;
      }
      current = next;
    }

    return domains;
  }

  static Glib::ustring
  sld_label(const Glib::ustring& domain)
  {
    const auto last_dot = domain.rfind('.');

    if (last_dot == Glib::ustring::npos || last_dot == 0)
    {
      return {};
    }

    const auto rest = domain.substr(0, last_dot);
    const auto prev_dot = rest.rfind('.');

    if (prev_dot == Glib::ustring::npos)
    {
      return rest;
    }

    return rest.substr(prev_dot + 1);
  }

  static bool
  entry_matches(const Glib::ustring& entry, const Glib::ustring& domain)
  {
    const auto entry_cf = entry.casefold();
    const auto domain_cf = domain.casefold();

    if (entry_cf.find(domain_cf) != Glib::ustring::npos)
    {
      return true;
    }

    const auto sld = sld_label(domain_cf);

    if (sld.size() < 3)
    {
      return false;
    }

    std::size_t start = 0;

    while (start <= entry_cf.size())
    {
      const auto slash = entry_cf.find('/', start);
      const auto segment = entry_cf.substr(
        start,
        slash == Glib::ustring::npos ? Glib::ustring::npos : slash - start
      );

      if (segment == sld || segment.find(sld) != Glib::ustring::npos)
      {
        return true;
      }
      if (slash == Glib::ustring::npos)
      {
        break;
      }
      start = slash + 1;
    }

    return false;
  }

  static std::filesystem::path
  store_directory()
  {
    if (const auto env = Glib::getenv("PASSWORD_STORE_DIR"); !env.empty())
    {
      return std::filesystem::path(env);
    }

    return std::filesystem::path(Glib::build_filename(
      Glib::get_home_dir(),
      ".password-store"
    ));
  }

  static Glib::ustring
  first_line(const Glib::ustring& text)
  {
    const auto end = text.find_first_of("\r\n");

    if (end == Glib::ustring::npos)
    {
      return text;
    }

    return text.substr(0, end);
  }

  static Glib::ustring
  trim_error(const Glib::ustring& text)
  {
    auto result = text;

    while (!result.empty())
    {
      const auto last = result[result.size() - 1];

      if (last != '\n' && last != '\r')
      {
        break;
      }
      result.erase(result.size() - 1, 1);
    }

    return result;
  }

  static Glib::ustring
  username_from_entry(const Glib::ustring& entry)
  {
    if (entry.empty())
    {
      return {};
    }

    const auto slash = entry.rfind('/');

    if (slash == Glib::ustring::npos)
    {
      return entry;
    }

    return entry.substr(slash + 1);
  }

  Glib::RefPtr<PasswordManager>
  PasswordManager::create()
  {
    return Glib::make_refptr_for_instance<PasswordManager>(
      new PasswordManager()
    );
  }

  std::vector<Glib::ustring>
  PasswordManager::find_entries(const Glib::ustring& host) const
  {
    if (host.empty())
    {
      return {};
    }

    const auto all = list_entries(store_directory());

    for (const auto& domain : domain_candidates(host))
    {
      std::vector<Glib::ustring> matches;

      for (const auto& entry : all)
      {
        if (entry_matches(entry, domain))
        {
          matches.push_back(entry);
        }
      }
      if (!matches.empty())
      {
        return matches;
      }
    }

    return {};
  }

  void
  PasswordManager::fetch(
    const Glib::ustring& entry,
    const callback_type& callback
  ) const
  {
    if (!callback)
    {
      return;
    }
    if (entry.empty())
    {
      callback(std::nullopt, "Empty pass entry.");

      return;
    }

    const auto pass_path = Glib::find_program_in_path("pass");

    if (pass_path.empty())
    {
      callback(std::nullopt, "pass executable not found in PATH.");

      return;
    }

    const std::vector<std::string> argv = {
      pass_path,
      "show",
      entry.raw(),
    };

    try
    {
      const auto flags =
        Gio::Subprocess::Flags::STDOUT_PIPE |
        Gio::Subprocess::Flags::STDERR_PIPE;
      auto subprocess = Gio::Subprocess::create(argv, flags);

      subprocess->communicate_utf8_async(
        Glib::ustring(),
        [subprocess, callback, entry](const Glib::RefPtr<Gio::AsyncResult>& result)
        {
          try
          {
            const auto [stdout_buf, stderr_buf] =
              subprocess->communicate_utf8_finish(result);

            if (!subprocess->get_successful())
            {
              auto message = trim_error(stderr_buf);

              if (message.empty())
              {
                message = "pass show failed.";
              }
              callback(std::nullopt, message);

              return;
            }

            Entry secret;

            secret.password = first_line(stdout_buf);
            secret.username = username_from_entry(entry);

            if (secret.password.empty())
            {
              callback(std::nullopt, "pass entry has no password.");

              return;
            }
            callback(std::move(secret), {});
          }
          catch (const Glib::Error& error)
          {
            callback(std::nullopt, error.what());
          }
        }
      );
    }
    catch (const Glib::Error& error)
    {
      callback(std::nullopt, error.what());
    }
  }
}
