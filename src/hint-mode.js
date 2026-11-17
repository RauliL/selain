(() => {
  const original = window.SelainHintMode;
  // CSS selectors are typically much faster than XPath for this kind of scan.
  const querySelector =
    "[onclick],[onmouseover],[onmousedown],[onmouseup],[oncommand],[href]," +
    'input:not([type="hidden"]),textarea,button,select,area';
  const maxAllowedHints = 500;
  const hintChars = Array.from("{{ hint-chars }}".toUpperCase());
  const hints = [];
  const hintContainers = [];
  let currentSequence = "";
  let openToNewTab = false;
  const hintStyle =
    "z-index:100000;position:absolute;padding:0.2em;border:none;" +
    "background-color:#ffd76e;color:#000000;font-family:monospace;" +
    "font-size:1em;line-height:1;font-weight:bold;white-space:nowrap;" +
    "text-shadow:none;pointer-events:none;";

  const numberToSequence = (number) => {
    let sequence = "";

    for (let n = number; n > 0; n = (n / 10) | 0) {
      sequence = hintChars[n % 10] + sequence;
    }

    return sequence || hintChars[0];
  };

  const isHintable = (element, rectangle, win) => {
    if (!rectangle || rectangle.width <= 0 || rectangle.height <= 0) {
      return false;
    }

    // Prefer the cheaper visibility API when available.
    if (typeof element.checkVisibility === "function") {
      return element.checkVisibility({
        checkOpacity: false,
        checkVisibilityCSS: true,
      });
    }

    const style = win.getComputedStyle(element);
    return style.display !== "none" && style.visibility === "visible";
  };

  const install = () => {
    const topWindow = window;
    const topWindowHeight = topWindow.innerHeight;
    const topWindowWidth = topWindow.innerWidth;
    let hintCount = 0;

    const drawHintsToWindow = (win, offsetX, offsetY) => {
      const doc = win.document;
      const winHeight = win.innerHeight;
      const winWidth = win.innerWidth;

      // Bounds
      const minX = offsetX < 0 ? -offsetX : 0;
      const minY = offsetY < 0 ? -offsetY : 0;
      const maxX =
        offsetX + winWidth > topWindowWidth
          ? topWindowWidth - offsetX
          : winWidth;
      const maxY =
        offsetY + winHeight > topWindowHeight
          ? topWindowHeight - offsetY
          : winHeight;

      const { scrollX, scrollY } = win;
      const elements = doc.querySelectorAll(querySelector);
      const fragment = doc.createDocumentFragment();
      const hintSpan = doc.createElement("span");

      hintSpan.style.cssText = hintStyle;

      for (let i = 0; i < elements.length; ++i) {
        if (hintCount >= maxAllowedHints) {
          break;
        }

        const element = elements[i];
        const rectangle = element.getBoundingClientRect();

        if (
          rectangle.left > maxX ||
          rectangle.right < minX ||
          rectangle.top > maxY ||
          rectangle.bottom < minY ||
          !isHintable(element, rectangle, win)
        ) {
          continue;
        }

        const leftPos = Math.max(rectangle.left + scrollX, scrollX);
        const topPos = Math.max(rectangle.top + scrollY, scrollY);
        const sequence = numberToSequence(++hintCount);
        const hint = hintSpan.cloneNode(false);

        hint.style.left = `${leftPos}px`;
        hint.style.top = `${topPos}px`;
        hint.textContent = sequence;
        fragment.appendChild(hint);
        hints.push({ element, sequence, span: hint });
      }

      const hintContainer = doc.createElement("div");
      hintContainer.appendChild(fragment);
      doc.documentElement.appendChild(hintContainer);
      hintContainers.push(hintContainer);

      // Also go through the frames contained in the document.
      const frames = doc.querySelectorAll("frame,iframe");

      for (let i = 0; i < frames.length; ++i) {
        const element = frames[i];
        const rectangle = element.getBoundingClientRect();

        if (
          !element.contentWindow ||
          !rectangle ||
          rectangle.left > maxX ||
          rectangle.right < minX ||
          rectangle.top > maxY ||
          rectangle.bottom < minY
        ) {
          continue;
        }

        try {
          drawHintsToWindow(
            element.contentWindow,
            offsetX + rectangle.left,
            offsetY + rectangle.top,
          );
        } catch (_) {
          // Cross-origin frames cannot be inspected.
        }
      }
    };

    drawHintsToWindow(topWindow, 0, 0);
  };

  const uninstall = () => {
    for (let i = 0; i < hintContainers.length; ++i) {
      const container = hintContainers[i];

      if (container.parentNode) {
        container.parentNode.removeChild(container);
      }
    }
    hintContainers.length = 0;
    hints.length = 0;
    window.SelainHintMode = original;
  };

  const activateHint = (hint) => {
    if (!hint || typeof hint.element === "undefined") {
      return "ignore";
    }

    const { element } = hint;
    const tagName = element.nodeName.toLowerCase();

    uninstall();

    if (
      (tagName === "input" ||
        tagName === "select" ||
        tagName === "textarea") &&
      element.type !== "button" &&
      element.type !== "submit"
    ) {
      element.focus();

      return "mode::insert";
    } else if (tagName === "frame" || tagName === "iframe") {
      element.focus();
    } else if (openToNewTab) {
      const oldTarget = element.getAttribute("target");

      element.setAttribute("target", "_blank");
      element.click();
      if (oldTarget === null) {
        element.removeAttribute("target");
      } else {
        element.setAttribute("target", oldTarget);
      }
    } else {
      element.click();
    }

    return "mode::normal";
  };

  const splitHints = () => {
    const matches = [];
    const nonMatches = [];

    if (currentSequence.length > 0) {
      for (let i = 0; i < hints.length; ++i) {
        const hint = hints[i];

        if (hint.sequence.startsWith(currentSequence)) {
          matches.push(hint);
        } else {
          nonMatches.push(hint);
        }
      }
    } else {
      nonMatches.push(...hints);
    }

    return [matches, nonMatches];
  };

  const updateMatches = (matches, nonMatches) => {
    if (currentSequence.length > 0) {
      for (let i = 0; i < matches.length; ++i) {
        const { sequence, span } = matches[i];

        if (span.style.visibility !== "visible") {
          span.style.visibility = "visible";
        }

        span.textContent = "";

        const matchingPart = span.ownerDocument.createElement("span");
        const nonMatchingPart = span.ownerDocument.createElement("span");

        matchingPart.textContent = currentSequence;
        nonMatchingPart.textContent = sequence.slice(currentSequence.length);
        matchingPart.style.color = "#a07555";

        span.appendChild(matchingPart);
        span.appendChild(nonMatchingPart);
      }
      for (let i = 0; i < nonMatches.length; ++i) {
        const { span } = nonMatches[i];

        if (span.style.visibility !== "hidden") {
          span.style.visibility = "hidden";
        }
      }
    } else {
      for (let i = 0; i < hints.length; ++i) {
        const { sequence, span } = hints[i];

        if (span.style.visibility !== "visible") {
          span.style.visibility = "visible";
        }
        span.textContent = sequence;
      }
    }
  };

  const addChar = (ch) => {
    if (
      typeof ch !== "string" ||
      ch.length !== 1 ||
      hintChars.indexOf(ch.toUpperCase()) < 0
    ) {
      return "ignore";
    }

    currentSequence += ch.toUpperCase();

    const [matches, nonMatches] = splitHints();

    if (matches.length === 1) {
      return activateHint(matches[0]);
    }

    updateMatches(matches, nonMatches);

    return "ignore";
  };

  const removeChar = () => {
    if (currentSequence.length > 0) {
      currentSequence = currentSequence.slice(0, -1);
      updateMatches(...splitHints());
    }
  };

  const activateCurrentMatch = () => {
    if (currentSequence.length <= 0) {
      return "ignore";
    }

    const match = hints.find((hint) => hint.sequence === currentSequence);

    return match ? activateHint(match) : "ignore";
  };

  const setOpenToNewTab = () => {
    openToNewTab = true;
  };

  install();

  window.SelainHintMode = {
    activateCurrentMatch,
    addChar,
    removeChar,
    setOpenToNewTab,
    uninstall,
  };
})();
