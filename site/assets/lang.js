// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The language at a first visit: the English home page (the root, marked
// data-first-visit) sends a visitor to the browser's language when the site
// has it, unless a language was chosen before. Choosing one in the header's
// menu keeps it (aurore-lang). Loaded in <head>, before the page is drawn, so
// the move shows nothing. The site's 15 languages are build_site.py's
// LANGUAGES: code and folder.
(function () {
  const KEY = "aurore-lang";
  const FOLDERS = {
    "en-US": "", fr: "fr", "fr-CA": "fr-ca", de: "de", es: "es", "es-419": "es-419", it: "it", nl: "nl",
    pt: "pt", "pt-BR": "pt-br", ru: "ru", ja: "ja", ko: "ko", "zh-Hans": "zh-hans", "zh-Hant": "zh-hant",
  };
  const find = (code) => Object.keys(FOLDERS).find((c) => c.toLowerCase() === String(code).toLowerCase());

  // A browser's language tag as one of the site's (null: English).
  const site = (tag) => {
    const t = String(tag).toLowerCase();
    const [base, region = ""] = t.split("-");
    if (base === "fr") return region === "ca" ? "fr-CA" : "fr";
    if (base === "es") return !region || region === "es" ? "es" : "es-419";
    if (base === "pt") return region === "br" ? "pt-BR" : "pt";
    if (base === "zh") return ["tw", "hk", "mo", "hant"].includes(region) || t.includes("hant") ? "zh-Hant" : "zh-Hans";
    if (["de", "it", "nl", "ru", "ja", "ko"].includes(base)) return base;
    return null;
  };

  // The visitor's language: the one chosen before ("en" and "fr" from the
  // site's two-language days still read), else the browser's.
  const visitor = () => {
    let chosen = null;
    try { chosen = localStorage.getItem(KEY); } catch (e) { /* storage refused: no memory */ }
    if (chosen) return find(chosen === "en" ? "en-US" : chosen) || "en-US";
    for (const tag of navigator.languages || [navigator.language || ""]) {
      if (String(tag).toLowerCase().split("-")[0] === "en") return "en-US";
      const code = site(tag);
      if (code) return code;
    }
    return "en-US";
  };
  window.auroreLanguages = { folders: FOLDERS, visitor };

  const root = document.documentElement;
  if (root.hasAttribute("data-first-visit")) {
    const referrer = document.referrer && new URL(document.referrer, location.href).origin === location.origin;
    const wanted = visitor();
    if (!referrer && wanted !== "en-US") {
      location.replace(`${FOLDERS[wanted]}/${location.hash}`);
      return;
    }
  }

  document.addEventListener("click", (event) => {
    const link = event.target.closest && event.target.closest("#languages a[data-lang]");
    if (!link) return;
    try { localStorage.setItem(KEY, link.dataset.lang); } catch (e) { /* idem */ }
  });
})();
