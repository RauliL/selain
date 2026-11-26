(() => {
  function findSearchInputs() {
    const elements = document.querySelectorAll(`
        input:not([type="hidden"]):not([type="password"]),
        textarea,
        [contenteditable="true"]
    `);

    const results = [];

    for (const el of elements) {
        if (el.disabled || el.readOnly)
            continue;

        const style = getComputedStyle(el);

        if (style.display === "none" ||
            style.visibility === "hidden" ||
            el.getBoundingClientRect().width === 0 ||
            el.getBoundingClientRect().height === 0)
            continue;

        const attrs = [
            el.type,
            el.name,
            el.id,
            el.getAttribute("role"),
            el.getAttribute("aria-label"),
            el.getAttribute("placeholder"),
            el.getAttribute("title"),
            el.getAttribute("autocomplete")
        ].filter(Boolean).join(" ").toLowerCase();

        let score = 0;

        if (el.matches('input[type="search"]'))
            score += 100;

        if (el.getAttribute("role") === "searchbox")
            score += 90;

        if (el.closest('form[role="search"]'))
            score += 80;

        if (/search|query|keyword|find/.test(attrs))
            score += 40;

        if (/search/.test(el.getAttribute("aria-label") || ""))
            score += 30;

        if (/search/.test(el.getAttribute("placeholder") || ""))
            score += 20;

        if (/search/.test(el.getAttribute("name") || ""))
            score += 20;

        if (/search/.test(el.id || ""))
            score += 20;

        if (/search/.test(el.getAttribute("autocomplete") || ""))
            score += 10;

        if (score > 0) {
            results.push({ element: el, score });
        }
    }

    return results.sort((a, b) => b.score - a.score);
  }

  const matches = findSearchInputs();

  if (matches.length > 0) {
    matches[0].element.focus();

    return true;
  }

  return false;
})()
