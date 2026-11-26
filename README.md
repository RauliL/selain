# selain

Modal Web browser with [Vim] like user interfaces that uses [WebKit] as it's
engine. I originally started building it out of frustration after [Firefox
removed support for XUL addons] and I could no longer use the excellent [VimFx]
addon that provided similar user interface to Firefox.

## Features

- Vim style modes and keyboard bindings.
- Vim style splitted views.
- Persistent, searchable visit history (`:history`, `:history-clear`).
- GreaseMonkey-style userscripts from `~/.config/selain/scripts/` (metadata
  injection only; see [doc/userscripts.md](doc/userscripts.md)).
- Ad blocking via WebKit content filters (EasyList and other lists in
  `~/.config/selain/blocklists/`; see [doc/adblock.md](doc/adblock.md)).
- [Finger](https://datatracker.ietf.org/doc/html/rfc1288) protocol support
  (`finger://`).
- [Gemini](https://geminiprotocol.net/) protocol support (`gemini://`).
- [Gopher](https://datatracker.ietf.org/doc/html/rfc1436) protocol support
  (`gopher://` and `gophers://`).

### Optional integrations

- [pass](https://www.passwordstore.org/) for password management.
- [buku](https://github.com/jarun/buku) for bookmark management.

## How to compile

Selain requires these libraries to be installed:

- [GTKmm]
- [WebKitGTK]
- [SQLite3]

You also need [CMake] to compile this application. On Ubuntu, all of these can
be installed with following command:

```shell
sudo apt install libgtkmm-4.0-dev libwebkitgtk-6.0-dev libsqlite3-dev cmake build-essential
```

After dependencies have been installed, you can compile the application like
this:

```shell
git clone https://github.com/RauliL/selain.git
cd selain
cmake -S . -B build
cmake --build build
```

After this, you can run the application by executing `selain` binary found in
the `build` directory, or you can alternatively install it to your system with
`sudo cmake --install build`.

[Vim]: https://vim.org
[WebKit]: https://webkit.org/
[Firefox removed support for XUL addons]: https://yoric.github.io/post/why-did-mozilla-remove-xul-addons/
[VimFx]: https://github.com/akhodakivskiy/VimFx
[GTKmm]: https://www.gtkmm.org/
[WebKitGTK]: https://webkitgtk.org/
[SQLite3]: https://sqlite.org/
[CMake]: https://cmake.org/
