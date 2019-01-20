SELAIN_JS_STRINGIFY((() => {
  const original = window.SelainHintMode;
  const queryExpression = '//*[@onclick or @onmouseover or @onmousedown or ' +
    '@onmouseup or @oncommand or @href] | //input[not(@type="hidden")] | ' +
    '//a[href] | //area | //textarea | //button | //select';
  const maxAllowedHints = 500;
  const hints = [];
  let hintContainer = null;
  let currentSequence = 0;

  const install = () => {
    const topWindow = window;
    const topWindowHeight = topWindow.innerHeight;
    const topWindowWidth = topWindow.innerWidth;
    let hintCount = 0;

    const drawHintsToWindow = (win, offsetX, offsetY) => {
      const doc = win.document;
      const winHeight = win.height;
      const winWidth = win.width;

      // Bounds
      const minX = offsetX < 0 ? -offsetX : 0;
      const minY = offsetY < 0 ? -offsetY : 0;
      const maxX = (
        offsetX + winWidth > topWindowWidth
          ? topWindowWidth - offsetX
          : topWindowWidth
      );
      const maxY = (
        offsetY + winHeight > topWindowHeight
          ? topWindowHeight - offsetY
          : topWindowHeight
      );

      const { scrollX, scrollY } = win;

      hintContainer = doc.createElement('div');

      const result = doc.evaluate(
        queryExpression,
        doc,
        () => 'http://www.w3.org/1999/xhtml',
        XPathResult.ORDERED_NODE_SNAPSHOT_TYPE,
        null
      );

      const hintSpan = doc.createElement('span');
      hintSpan.style.zIndex = '100000';
      hintSpan.style.position = 'absolute';
      hintSpan.style.padding = '0.2em';
      hintSpan.style.borderColor = 'rgba(0, 0, 0, 0.4)';
      hintSpan.style.borderWidth = '0.15em';
      hintSpan.style.border = '';
      hintSpan.style.backgroundColor = '#ffd76e';
      hintSpan.style.color = '#000000';
      hintSpan.style.fontFamily = 'monospace';
      hintSpan.style.fontSize = '1em';
      hintSpan.style.lineHeight = '1';
      hintSpan.style.fontWeight = 'bold';
      hintSpan.style.whiteSpace = 'nowrap';
      hintSpan.style.textShadow = 'none';

      for (let i = 0; i < result.snapshotLength; ++i) {
        if (hintCount >= maxAllowedHints) {
          break;
        }

        const element = result.snapshotItem(i);
        const rectangle = element.getBoundingClientRect();

        if (!rectangle ||
            rectangle.left > maxX ||
            rectangle.right < minX ||
            rectangle.top > maxY ||
            rectangle.bottom < minY) {
          continue;
        }

        const style = topWindow.getComputedStyle(element, '');
        if (style.display === 'none' || style.visibility !== 'visible') {
          continue;
        }

        const leftPos = Math.max(rectangle.left + scrollX, scrollX);
        const topPos = Math.max(rectangle.top + scrollY, scrollY);

        let hintNumber = hintCount;
        if (element.nodeName.toLowerCase() === 'a') {
          for (let j = 0; j < hints.length; ++j) {
            const existingHint = hints[j];

            if (existingHint.element.nodeName.toLowerCase() !== 'a') {
              continue;
            }
            if (existingHint.element.href === element.href) {
              hintNumber = existingHint.number - 1;
              break;
            }
          }
        }

        const hint = hintSpan.cloneNode(false);
        hint.style.left = `${leftPos}px`;
        hint.style.top = `${topPos}px`;
        hint.innerText = `${hintNumber + 1}`;
        hintContainer.appendChild(hint);

        if (hintNumber === hintCount) {
          ++hintCount;
        } else {
          hintNumber = -2;
        }

        hints.push({
          element,
          number: hintNumber + 1,
          span: hint,
        });
      }

      doc.documentElement.appendChild(hintContainer);

      // Also go through the frames contained in the document.
      ['frame', 'iframe'].forEach((tagName) => {
        const frames = doc.getElementsByTagName(tagName);

        for (let i = 0; i < frames.length; ++i) {
          const element = frames[i];
          const rectangle = element.getBoundingClientRect();

          if (!element.contentWindow ||
              !rectangle ||
              rectangle.left > maxX ||
              rectangle.right < minX ||
              rectangle.top > maxY ||
              rectangle.bottom < minY) {
            continue;
          }
          drawHintsToWindow(
            element.contentWindow,
            offsetX + rectangle.left,
            offsetY + rectangle.top
          );
        }
      });
    };

    drawHintsToWindow(topWindow, 0, 0);
  };

  const uninstall = () => {
    hints
      .filter((hint) => typeof hint.element !== 'undefined')
      .forEach((hint) => {
        hint.span.parentNode.removeChild(hint.span);
      });
    hints.length = 0;
    if (hintContainer) {
      hintContainer.parentNode.removeChild(hintContainer);
      hintContainer = null;
    }
    window.SelainHintMode = original;
  };

  const getNearestMatches = () => hints.filter(
    (hint) => `${hint.number}`.startsWith(`${currentSequence}`)
  );

  const activateHint = (hint) => {
    if (!hint || typeof hint.element === 'undefined') {
      return 'ignore';
    }

    const { element } = hint;
    const tagName = element.nodeName.toLowerCase();

    uninstall();

    if (['input', 'select', 'textarea'].indexOf(tagName) >= 0) {
      element.focus();

      return 'mode::insert';
    } else if (['frame', 'iframe'].indexOf(tagName) >= 0) {
      element.focus();
    } else {
      // TODO: Add support for the "open in new tab" -feature.
      element.click();
    }

    return 'mode::normal';
  };

  const addDigit = (digit) => {
    if (typeof digit !== 'number' || digit < 0 || digit > 9) {
      return;
    }

    currentSequence = (currentSequence * 10) + digit;

    hints
      .filter((hint) => hint.span.style.visiblity !== 'hidden')
      .filter((hint) => !`${hint.number}`.startsWith(`${currentSequence}`))
      .forEach((hint) => {
        hint.span.style.visibility = 'hidden';
      });

    const nearestMatches = getNearestMatches();

    if (nearestMatches.length === 1) {
      return activateHint(nearestMatches[0]);
    }

    return 'ignore';
  };

  const removeDigit = () => {
    if (currentSequence <= 0) {
      return;
    }

    currentSequence = Math.floor(currentSequence / 10);

    let hintsToBeShown = hints
      .filter((hint) => hint.span.style.visibility === 'hidden');

    if (currentSequence > 0) {
      hintsToBeShown = hintsToBeShown.filter(
        (hint) => `${hint.number}`.startsWith(`${currentSequence}`)
      );
    }

    hintsToBeShown.forEach((hint) => {
      hint.span.style.visibility = 'visible';
    });
  };

  const activateCurrentMatch = () => {
    if (currentSequence <= 0) {
      return 'ignore';
    }

    const match = hints.find(
      (hint) => `${hint.number}` === `${currentSequence}`
    );

    if (match) {
      return activateHint(match);
    }

    return 'ignore';
  };

  install();

  window.SelainHintMode = {
    activateCurrentMatch,
    addDigit,
    removeDigit,
    uninstall
  };
})();)
