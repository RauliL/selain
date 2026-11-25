# Plan: Harden PDF / scroll helper messaging

## Problem

`src/webkit/scroll-helper.js` is injected into every frame and scrolls on any
`message` event shaped like:

```js
{ type: "selain-scroll", action: "down" }
```

There is no sender authentication. Same-frame abuse is mostly irrelevant (pages
can already scroll themselves), but because the helper runs in **all frames**, a
page can scroll **cross-origin** iframes (including WebKit’s PDF.js viewer) via
`iframe.contentWindow.postMessage(...)`. That weakens the same-origin policy for
scroll position only—low severity, but unnecessary.

`cmd_scroll` today evaluates a small dispatcher in the **page world** and uses
`postMessage`, so anything the page can observe or forge can trigger the helper.

## Goals

- Keep scrolling working for normal HTML and for WebKit PDF.js
  (`webkit-pdfjs-viewer://` iframe).
- Prevent arbitrary web content from triggering the helper (especially across
  origins).
- Avoid exposing a durable secret to page-world JavaScript.
- Keep the change small and aligned with existing WebKit injection patterns
  (user scripts, optional named script worlds as used by hint mode).

## Non-goals

- Replacing PDF.js or disabling inline PDF viewing.
- A general postMessage RPC framework.
- Origin allowlists as the sole control (legitimate PDF scroll is cross-origin:
  outer `file:`/`https:` page → `webkit-pdfjs-viewer://` iframe).

## Recommended approach: isolated-world trigger + session token

Prefer authenticating the scroll command inside WebKit’s isolated user-script
world, not trusting page-world `postMessage`.

### 1. Session token owned by the browser process

- Generate a cryptographically random token once per `webkit::Context` (or app
  lifetime).
- Embed that token into the injected scroll helper at install time (string
  substitute into `scroll-helper.js`, same pattern as hint-mode templating).
- Store the token only in C++ / the isolated helper closure—not in settings
  files or logs.

### 2. Accept only authenticated messages in the helper

In `scroll-helper.js`:

- Require `event.data.type === "selain-scroll"`.
- Require `event.data.token === <injected session token>` (strict equality).
- Keep the existing fixed `action` switch; reject anything else.
- Optionally also require `event.data.action` to be one of the known strings
  before calling `scroll()`.

Without the token, page-forged `postMessage` calls become no-ops.

### 3. Do not put the token in page-world `evaluate_javascript`

If `cmd_scroll` keeps posting from the page world with the token in the
payload, any page can read that evaluation (or hook `postMessage`) and reuse
the token for the rest of the session.

Instead, trigger scroll from a context the page cannot read:

**Option A (preferred):** evaluate the dispatcher with
`webkit_web_view_evaluate_javascript(..., world_name)` in a **dedicated
isolated world** that also receives the user script (or inject the helper into
that named world only). Hint mode already uses a custom script world—follow
that pattern.

**Option B:** keep `postMessage`, but have C++ drive it only after injecting a
one-shot script into the isolated world that posts with the token. Still avoid
passing the token through page-world script text.

**Option C (fallback):** if named-world evaluation cannot reach the PDF.js
iframe cleanly, keep all-frames user script + tokenized `postMessage`, but
ensure the **only** code that learns the token is isolated-world script
installed at context creation—not `cmd_scroll`’s page-world string.

### 4. Wire `cmd_scroll` to the isolated trigger

- Validate actions in C++ (already done).
- Call into the isolated world with `{ action }` only (token applied inside the
  helper / dispatcher, not formatted into page JS).
- Preserve fan-out to iframes **inside** the isolated dispatcher if the main
  frame still cannot touch PDF.js directly; the token travels only in that
  world.

### 5. Hardening checklist for the helper itself

- Keep injection at **document-start** + **all frames** (required for PDF.js).
- Do not attach callable globals on the page (`window.__selainDoScroll`, etc.).
- Do not use `event.origin` alone as the security boundary.
- Avoid reflecting `action` into `eval`, `new Function`, or dynamic property
  access.
- Consider ignoring messages where `event.source` is unexpected only if that
  does not break the PDF path; token auth is the real control.

## Implementation steps

1. Add a session token to `webkit::Context` (generate in constructor).
2. Template `scroll-helper.js` with the token when calling
   `webkit_user_script_new` in `install_scroll_helper`.
3. Introduce a named script world (e.g. `selain-scroll`) or reuse a shared
   selain world; inject the helper into that world.
4. Change `cmd_scroll` to `execute_script(..., world_name)` with a dispatcher
   that does **not** embed the token in page-visible source—either the
   dispatcher lives entirely in the isolated world and already closes over the
   token, or it only sends `{ type, action }` while the isolated listener
   already knows the token… *(see note below)*.
5. Manual test matrix:
   - HTML page: `j` / `k` / `d` / `u` / `gg` / `G`
   - PDF in WebKit: same bindings scroll `#viewerContainer`
   - DevTools / page console: `postMessage({ type: "selain-scroll", action: "bottom" }, "*")` must **not** scroll
   - Page console with a guessed/wrong token must **not** scroll
6. Optional follow-up: remove any leftover page-world `postMessage` API docs or
   comments that imply sites may use it.

### Note on token + isolated `postMessage`

If the listener checks a token, the sender must supply it. Senders in the
**same** isolated world can close over the token without putting it in
evaluable page strings. Cross-frame `postMessage` from the main frame’s
isolated world to the iframe’s isolated world should still deliver the event
to the iframe helper if both were injected as user scripts in that world—
verify this on WebKitGTK 2.52+ during step 5. If isolated-world `postMessage`
does not reach the PDF iframe helper, fall back to Option C only after
documenting the residual risk (token never appears in page-world script).

## Alternatives considered

| Approach | Pros | Cons |
| --- | --- | --- |
| Origin allowlist | Simple | Breaks legitimate PDF cross-origin path |
| Disable PDF.js / force download | Removes iframe problem | Changes product behavior; out of scope |
| Page-world token in `cmd_scroll` | Easy | Token leaks to the page for the session |
| GTK/WebKit wheel event injection | No JS channel | Harder to map exact vim scroll amounts; more platform coupling |

## Success criteria

- Sites cannot scroll frames solely by forging `{ type: "selain-scroll" }`
  messages.
- Selain scroll commands behave as today on HTML and PDF.js.
- No session token appears in page-world script sources or accessible page
  globals.
