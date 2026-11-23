# Password manager integration (`pass`)

Plan for integrating the [pass](https://www.passwordstore.org/) CLI as a
credential provider for login forms. Selain is keyboard-first (Vim-like); the
integration should follow that model rather than Chrome-style autofill popups.

## Recommended model: on-demand, not auto-popup

Do not auto-launch `pass` on every password-field focus. That fights Vim flow
and can trigger unexpected GPG/pinentry prompts. Prefer:

1. **Detect** that the page has (or the user is in) a login form.
2. **Offer** a keyboard path (`:pass` / e.g. `zl`) that searches by domain and
   fills credentials.
3. Optionally **signal** focus (status text / mode hint) so the user knows the
   binding will work — without spawning GPG until they ask.

This matches existing patterns: `gi` / `:insert-input` already finds inputs via
JS; a pass command is the same idea plus credentials.

## 1. Detecting a login form / focus

WebKitGTK has no password-manager autofill API to hook. Detection is page JS
plus a bridge back to C++.

### A. On-demand (simplest, good first step)

When `:pass` runs, inject JS that:

- Finds forms with an `input[type=password]`
- Or inspects `document.activeElement` if it is username/email/password
- Returns structured info, e.g.
  `{ hasLoginForm, focusedKind: "username"|"password"|null, formIndex }`

Same style as `cmd_insert_input` in `src/core/command.cpp`.

### B. Live focus tracking (for “you’re on a login field” awareness)

Inject a persistent user script (via `WebKitUserContentManager`, already used
for content filters) that listens to `focusin` / `focusout` and posts messages
when the focused node is:

- `input[type=password]`, or
- a text/email field in a form that also has a password field

Host side registers a script message handler and updates something like `Tab` /
status: “login field focused”. No `pass` yet — only UI affordance.

### Heuristics

| Signal | Use |
|--------|-----|
| `input[type=password]` in form | Strong login marker |
| Nearby `type=text` / `email` / `username` / `autocomplete=username` | Username field |
| `autocomplete=current-password` / `new-password` | Distinguishes login vs register |
| Visible + not `disabled` / not `readonly` | Same filters as insert-input |

SPA caveat: forms often appear after load. A live listener or re-query on
command is safer than a one-shot on `load-finished`.

## 2. Domain → pass entry search

Use `Tab::get_uri()` → parse host with GLib URI (`g_uri_get_host`), same family
as hierarchy helpers in `src/utils.cpp`.

### Matching strategy (qute-pass style)

Try in order:

1. Full host: `login.github.com`
2. Strip subdomains: `github.com`
3. Optional: private/IP hosts as-is

### Listing candidates without decrypting everything

- Prefer a filesystem scan of `$PASSWORD_STORE_DIR` (default
  `~/.password-store`) for `*.gpg` paths containing the domain segment — no GPG
  until selection.
- Or run `pass find <domain>` / `pass ls` and filter (heavier; still no decrypt
  until `pass show`).

Typical layouts that should match:

- `github.com/rauli`
- `websites/github.com`
- `Com/GitHub/rauli` (case-insensitive path match)

### Username extraction (configurable later)

- Default: last path segment (`github.com/rauli` → `rauli`)
- Alternative: parse decrypted body for `login:` / `user:` / `username:` lines;
  password = first line (pass convention)

First subprocess use in the project: `Gio::Subprocess` / `g_spawn` for
`pass show <entry>` (async so the UI does not freeze on pinentry).

## 3. UX flow (keyboard-native)

Reuse the **ViewListDialog** pattern (filter entry + fuzzy list + ↑↓/Enter/Esc):

```
:pass  (or zl)
  → get current tab host
  → list matching store paths (no decrypt)
  → if 0: error; if 1: optional auto-pick; if many: picker (seed filter with host)
  → pass show <entry>  (pinentry if needed)
  → JS fill form
```

### Fill strategy

More reliable than fake-key for WebKit:

1. Prefer the focused form; else the first visible form with a password field.
2. Set username on text/email/username field; password on `type=password`.
3. Dispatch `input` + `change` (and often `InputEvent`) so React/Vue forms notice.
4. Escape secrets with existing `utils::js_string_escape`.
5. Optionally move focus to password or submit; enter `INSERT` if useful.

Command variants (like qutebrowser):

- `:pass -u` — username only
- `:pass -p` — password only

Useful when focus is already on one field.

## 4. Suggested architecture

| Piece | Role |
|-------|------|
| `src/pass-store.cpp` (+ header) | Resolve store dir, list path matches by domain, `pass show` async, parse secret |
| `utils` host helper | `url_host(uri)` (+ maybe eTLD+1 later) |
| Injected JS (resource or string) | Detect login form / fill fields |
| Optional user script + message handler | Live “login focused” signal |
| `PassPickerDialog` | Clone of `ViewListDialog` |
| `:pass` in `command.cpp` + bind in `keyboard.cpp` | Entry point |
| Settings | `pass.enabled`, store dir override, username-from path vs secret |

No new web engine features required — URI + `execute_script` + modal picker
already exist; subprocess + pass parsing are the new bits.

### Key existing hooks

| Concern | Location |
|---------|----------|
| Focus / find inputs | `cmd_insert_input` in `src/core/command.cpp` |
| Hint inputs | `src/hint-mode.js` |
| Current URL | `Tab::get_uri()` / `View::get_uri()` |
| JS evaluation | `View::execute_script` |
| Fuzzy picker UI | `src/ui/view-list-dialog.cpp` |
| Fuzzy scoring / JS escape | `utils::fuzzy_match_score`, `utils::js_string_escape` |
| Commands / bindings | `src/core/command.cpp`, `src/keyboard.cpp` |

## 5. C++ ↔ page JavaScript bridge

Selain already has two bridges. For `:pass`, the MVP mostly uses the first;
live “login focused” status uses the second (same idea as hint mode).

### 5.1 One-shot: C++ runs JS, page returns a value

This is what `gi` / `:insert-input` does via `View::execute_script` →
`webkit_web_view_evaluate_javascript`. Good for detect-on-command and fill.

**Detect login form / focus:**

```javascript
(() => {
  const isVisible = (el) => {
    if (typeof el.checkVisibility === "function") {
      return el.checkVisibility({ checkOpacity: false, checkVisibilityCSS: true });
    }
    const s = getComputedStyle(el);
    return s.display !== "none" && s.visibility !== "hidden";
  };

  const usable = (el) =>
    el && !el.disabled && !el.readOnly && isVisible(el);

  const forms = [...document.querySelectorAll("form")].filter((form) =>
    [...form.querySelectorAll('input[type="password"]')].some(usable)
  );

  const active = document.activeElement;
  let focusedKind = null;

  if (active && active.matches?.('input[type="password"]')) {
    focusedKind = "password";
  } else if (
    active &&
    active.matches?.(
      'input:not([type]),input[type="text"],input[type="email"],input[type="username"]'
    )
  ) {
    const form = active.closest("form");
    if (form && form.querySelector('input[type="password"]')) {
      focusedKind = "username";
    }
  }

  return JSON.stringify({
    hasLoginForm: forms.length > 0,
    formCount: forms.length,
    focusedKind,
  });
})()
```

C++ side: `tab.execute_script(script, [](JSCValue* value, GError*) { ... })`,
then `jsc_value_to_string` / parse JSON (hint mode already uses
`jsc_value_to_json`).

**Fill after `pass show`:**

```javascript
((username, password) => {
  const setValue = (el, value) => {
    el.focus();
    el.value = value;
    el.dispatchEvent(new Event("input", { bubbles: true }));
    el.dispatchEvent(new Event("change", { bubbles: true }));
  };

  const form =
    document.activeElement?.closest?.("form") ||
    [...document.querySelectorAll("form")].find((f) =>
      f.querySelector('input[type="password"]')
    );

  if (!form) return false;

  const user = form.querySelector(
    'input[type="email"],input[type="text"],input[type="username"],input:not([type])'
  );
  const pass = form.querySelector('input[type="password"]');

  if (user && username !== null) setValue(user, username);
  if (pass && password !== null) setValue(pass, password);

  return true;
})("USER", "PASS")
```

C++ builds that string with `utils::js_string_escape` for the secrets (same as
protocol helpers). Never concatenate untrusted page data into the script — only
escaped credential strings from `pass`.

### 5.2 Persistent: page → C++ via `webkit.messageHandlers`

This is the hint-mode pattern: register a handler in C++, page calls
`postMessage`, optionally with a reply.

**Page (user script / injected once):**

```javascript
(() => {
  const post = (message) =>
    webkit.messageHandlers.passAutofill.postMessage(message);

  const isLoginFocus = (el) => {
    if (!(el instanceof HTMLInputElement)) return false;
    if (el.type === "password") return true;
    if (!["text", "email", "username", ""].includes(el.type)) return false;
    return !!el.closest("form")?.querySelector('input[type="password"]');
  };

  document.addEventListener(
    "focusin",
    (e) => {
      if (isLoginFocus(e.target)) {
        post({
          event: "login-focus",
          kind: e.target.type === "password" ? "password" : "username",
        });
      }
    },
    true
  );

  document.addEventListener(
    "focusout",
    (e) => {
      if (isLoginFocus(e.target)) {
        post({ event: "login-blur" });
      }
    },
    true
  );
})();
```

**C++ registration** (mirrors `hint-context.cpp`):

```cpp
webkit_user_content_manager_register_script_message_handler_with_reply(
  manager, "passAutofill", /* world */ nullptr);
// connect "script-message-with-reply-received::passAutofill"
```

From the page, the channel name must match:
`webkit.messageHandlers.passAutofill`.

For a **command loop** (C++ sends actions back), hint mode awaits the reply:

```javascript
const response = await webkit.messageHandlers.hintMode.postMessage(message);
// response is whatever C++ returned via webkit_script_message_reply_return_value
```

Pass autofill usually does not need that loop: focus events are fire-and-forget;
fill stays a one-shot `execute_script`.

### 5.3 Which bridge to use for `:pass`

| Direction | Mechanism |
|-----------|-----------|
| “Is there a login form / what’s focused?” on `:pass` | One-shot `execute_script` + return JSON |
| “Fill these credentials” | One-shot `execute_script` with escaped args |
| “User just focused a password field” (status hint) | Persistent script + `messageHandlers.passAutofill` |

MVP can be entirely the one-shot path — same as `cmd_insert_input`. The message
handler path is only needed if live status should update without the user
running `:pass` first.

## 6. Phased delivery

1. **MVP:** `:pass` → domain match → picker → `pass show` → JS fill. No live
   focus tracking.
2. **Focus-aware:** user script reports login focus; status/hint; maybe default
   binding only “active” then.
3. **Polish:** username/password-only modes, OTP (`pass otp` if present),
   subdomain fallback UI, settings.

## Open decisions

Defaults recommended in parentheses:

1. **Trigger:** key-only vs also soft prompt when a password field gains focus?
   (**key-only**)
2. **Username source:** path segment vs metadata lines in the secret?
   (**path segment**)
3. **Auto-fill on single match** vs always show the picker?
   (**auto if exactly one match; picker when many**)
4. **Scope of abstraction:** `pass`-only first, or a thin `PasswordProvider`
   interface for gopass / Bitwarden CLI later? (**`pass`-only first**)

These defaults are closest to qutebrowser’s `qute-pass` and least surprising for
a Vim-style browser.
