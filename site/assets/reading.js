// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The documentation read on a light ground, when chosen before (site.js's
// "Light reading" button), set before the first paint so the page does not
// flash dark first. A file of its own: the pages' Content-Security-Policy
// runs no inline script.
(function () {
  var light = null;
  try { light = localStorage.getItem('aurore-reading'); } catch (e) { /* storage refused */ }
  if (light === 'light') document.documentElement.dataset.reading = 'light';
})();

// The language a visitor chose before boreale-site's lang.js, which keeps it
// as "boreale-lang", the menu's data-lang (the folder, "en-US" for English):
// the old "aurore-lang" (a code, "en" or "fr" from the site's two-language
// days) moved there once, then removed. Here rather than in site.js: this
// file runs in <head> before lang.js, whose first visit reads the choice.
// To delete after 2027-01-31 (workbench plans/aurore/suivi.md).
(function () {
  try {
    var old = localStorage.getItem('aurore-lang');
    if (old === null) return;
    var code = old.toLowerCase();
    var known = ['en-us', 'fr', 'fr-ca', 'de', 'es', 'es-419', 'it', 'nl', 'pt', 'pt-br', 'ru', 'ja', 'ko',
      'zh-hans', 'zh-hant'];
    var folder = code === 'en' || code === 'en-us' ? 'en-US' : known.indexOf(code) >= 0 ? code : null;
    if (folder && localStorage.getItem('boreale-lang') === null) localStorage.setItem('boreale-lang', folder);
    localStorage.removeItem('aurore-lang');
  } catch (e) { /* storage refused: nothing kept */ }
})();
