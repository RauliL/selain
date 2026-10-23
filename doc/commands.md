# Commands

Selain currently supports following commands:

|Command    |Shortcut|                                       |
|-----------|--------|---------------------------------------|
|`:hint`    |`:h`    |Switches to hint mode.                 |
|`:insert`  |`:i`    |Switches to insert mode.               |
|`:only`    |`:on`   |Closes all splits except the current.  |
|`:open`    |`:o`    |Opens URI given as argument.           |
|`:open-tab`|`:ot`   |Opens URI given as argument in new tab.|
|`:quit`    |`:q`    |Closes current split, or tab if alone. |
|`:qall`    |`:qa`   |Closes all tabs.                       |
|`:reload`  |`:r`    |Reloads page.                          |
|`:reload!` |`:r!`   |Reloads page bypassing the cache.      |
|`:set`     |`:se`   |Views/changes an setting.              |
|`:split`   |`:sp`   |Splits the current view horizontally.  |
|`:stop`    |`:s`    |Stops page from loading content.       |
|`:tabnext` |`:tn`   |Switches to next tab.                  |
|`:tabprev` |`:tp`   |Switches to previous tab.              |
|`:vsplit`  |`:vs`   |Splits the current view vertically.    |

`:split` and `:vsplit` accept an optional URI argument. Without one, the new
split loads the same URI as the current view.
