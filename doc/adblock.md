# Ad blocking

Selain blocks ads and trackers with WebKit’s content filter API
(`WebKitUserContentFilterStore`), which applies rules at resource-load time.

Toggle with `:set adblock=true` / `:set adblock=false` (default is on). Put
`set adblock=false` in `~/.config/selain/init.selain` to disable at startup.

## Blocklist directory

Place one or more blocklist files in:

```
~/.config/selain/blocklists/
```

Every regular file in that directory (except hidden files starting with `.`) is
loaded when the browser starts. Supported formats:

| Format | Example | Notes |
|--------|---------|-------|
| EasyList / Adblock Plus text | `easylist.txt` | Converted to WebKit rules on load. Rules WebKit cannot express are skipped. |
| Safari / WebKit content-blocker JSON | `easylist.json` | Used as-is. |

A minimal starter list:

```shell
mkdir -p ~/.config/selain/blocklists
curl -fsSL -o ~/.config/selain/blocklists/easylist.txt \
  https://easylist.to/easylist/easylist.txt
```

Compiled filters are cached under `~/.cache/selain/content-filters/`. Source
fingerprints live in `~/.cache/selain/blocklists/*.meta` so unchanged lists are
not reconverted or recompiled on every start.

## Automatic updates (planned)

The loader is built so a future updater can periodically download EasyList into
`~/.config/selain/blocklists/` (or call `selain::adblock::load_file`) without
changing how filters are applied. Until that exists, refresh lists by replacing
files in the blocklists directory and restarting selain.
