# Commands

Selain currently supports following commands:

|Command       |Shortcut|                                       |
|--------------|--------|---------------------------------------|
|`:back`       |`:b`    |Goes back in browsing history.         |
|`:bind`       |        |Add keyboard binding.                  |
|`:bookmark`   |`:bm`   |Opens or adds bookmarks.               |
|`:command`    |`:c`    |Switches to command mode.              |
|`:echo`       |`:ec`   |Displays given arguments as a notification.|
|`:execute`    |`:ex`   |Evaluates JS; runs result as a command.|
|`:forward`    |`:f`    |Goes forward in browsing history.      |
|`:go-root`    |`:gr`   |Goes up to the root of the URL hierarchy.|
|`:go-up`      |`:gu`   |Goes up one level in the URL hierarchy.|
|`:hint`       |`:h`    |Switches to hint mode.                 |
|`:hint-tab`   |`:ht`   |Switches to open new tab hint mode.    |
|`:insert`     |`:i`    |Switches to insert mode.               |
|`:insert-input`|`:ii`  |Focuses first input/textarea; insert mode.|
|`:inspector`  |`:insp` |Toggles developer tools.               |
|`:js`         |`:j`    |Runs JavaScript in the current tab.    |
|`:login`      |`:l`    |Fills login form.                      |
|`:only`       |`:on`   |Closes all splits except the current.  |
|`:open`       |`:o`    |Opens URI, domain, or search query.    |
|`:open-tab`   |`:ot`   |Same as `:open`, in a new tab.         |
|`:paste`      |`:p`    |Opens clipboard text as URI/search.    |
|`:paste-tab`  |`:pt`   |Same as `:paste`, in a new tab.        |
|`:quit`       |`:q`    |Closes current split, or tab if alone. |
|`:qall`       |`:qa`   |Closes all tabs.                       |
|`:reload`     |`:r`    |Reloads page.                          |
|`:reload!`    |`:r!`   |Reloads page bypassing the cache.      |
|`:scroll`     |`:sc`   |Scrolls the current tab.               |
|`:search-next`|`:sen`  |Goes to next search result.            |
|`:search-prev`|`:sep`  |Goes to previous search result.        |
|`:set`        |`:se`   |Views/changes an setting.              |
|`:split`      |`:sp`   |Splits the current view horizontally.  |
|`:split-focus`|`:spf`  |Changes focus of splitted view.        |
|`:stop`       |`:s`    |Stops page from loading content.       |
|`:tab`        |`:t`    |Switches to tab given as 1-based index.|
|`:tab-next`   |`:tn`   |Switches to next tab.                  |
|`:tab-prev`   |`:tp`   |Switches to previous tab.              |
|`:unbind`     |        |Remove keyboard binding.               |
|`:view-list`  |`:vl`   |Opens a searchable list of all views.  |
|`:vsplit`     |`:vs`   |Splits the current view vertically.    |
|`:yank`       |`:y`    |Copies current URI to clipboard.       |
|`:zoom`       |`:z`    |Changes zoom level of the current view.|

`:execute` evaluates a JavaScript expression in the current tab and runs the
resulting string as a browser command. Example:
`:execute "command :open " + window.location.href`.

`:js` takes a JavaScript expression or statement as argument and shows the
result (or an error) as a notification. Example: `:js document.title`.

`:bookmark` opens bookmarks from the [Buku](https://github.com/jarun/buku) manager
in a fuzzy picker. With no arguments it lists all bookmarks; otherwise it
searches by keyword. Use `:bookmark -t` / `:bookmark --tab` to open the
selection in a new tab, or `:bookmark -a [tags…]` to bookmark the current page
(optional comma-separated tags). Default binding: `zb`.

`:login` looks up the current page host in the [pass](https://www.passwordstore.org/)
store, decrypts a matching entry, and fills the login form. With one match it
fills immediately; with several it opens a fuzzy picker (filter seeded with the
host). Use `:login -u` / `:login -p` to fill only the username or password.
Default binding: `zl`.

`:split` and `:vsplit` accept an optional URI argument. Without one, the new
split loads the same URI as the current view.

`:zoom` sets the page zoom of the current view. Without an argument it shows
the current level. Absolute values use a percentage (`:zoom 120%`); relative
adjustments use a signed number of percentage points (`:zoom +20`,
`:zoom -5`).
