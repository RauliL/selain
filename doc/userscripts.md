# Userscripts

Selain can load GreaseMonkey-style userscripts from:

```
~/.config/selain/scripts/
```

Each regular `*.js` / `*.user.js` file that contains a `==UserScript==` metadata
block is parsed at startup and injected through WebKit’s user-content manager
(URL allow/block lists). Hidden files (names starting with `.`) are ignored.

This is **metadata + injection only**: there is no GreaseMonkey `GM_*` API,
`@grant`, `@require`, `@resource`, or `unsafeWindow` support. Planned work is
tracked in [../todo/gm-api.md](../todo/gm-api.md).

Disable with `:set userscripts=false` (or in `~/.config/selain/init.selain`).
Reload from disk without restarting with `:userscripts-reload` (`:usr`).

## Example

```javascript
// ==UserScript==
// @name         Example
// @match        https://example.com/*
// @include      *://*.example.org/*
// @exclude      https://example.com/admin/*
// @run-at       document-end
// @noframes
// ==/UserScript==

document.documentElement.dataset.selainExample = "1";
```

## Supported headers

| Header | Behavior |
|--------|----------|
| `@name` | Shown in diagnostics; defaults to `unnamed`. |
| `@match` | Chrome-style match pattern → WebKit allow list. |
| `@include` | Glob with a scheme (`https://…`, `*://…`) → allow list. `*` and `<all_urls>` become `*://*/*`. |
| `@exclude` | Same as `@include`, but added to the WebKit block list. |
| `@run-at` | `document-start` injects at document start; `document-end` and `document-idle` inject at document end. |
| `@noframes` | Inject only into the top frame. |

Scripts with **no usable `@match` or `@include`** are not installed (to avoid
accidental global injection). Legacy `@include /regex/` forms are skipped with
a warning.

## Notes

- Built-in helpers (scroll helper, theme CSS) are separate; `:userscripts-reload`
  only replaces scripts loaded from this directory.
- After changing scripts, run `:userscripts-reload` or restart Selain. Already
  loaded pages may need a reload for the new script to take effect.
