// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Aurore's website (site/README.md). The aurora behind the home page's header
// and hero (canvas#sky), Boréale's own (boreale/site/assets/site.js): three
// soft curtains under a few stars, moving only while the page is visible and
// the visitor has not asked for less motion. Without script, site.css's
// gradient stands in.
(function () {
  const canvas = document.getElementById("sky");
  if (!canvas || !canvas.getContext) return;
  const ctx = canvas.getContext("2d");
  // The curtains are drawn here at a quarter of the size, then laid on the
  // sky blurred and scaled up in one go: blurring each strip would be slow.
  const glow = document.createElement("canvas");
  const gtx = glow.getContext("2d");
  const still = window.matchMedia("(prefers-reduced-motion: reduce)");
  const curtains = [
    { rgb: [0, 255, 204], y: 0.34, amp: 0.08, freq: 1.2, speed: 0.00012, height: 0.42, alpha: 0.34 },
    { rgb: [51, 187, 255], y: 0.26, amp: 0.06, freq: 1.7, speed: -0.00009, height: 0.34, alpha: 0.26 },
    { rgb: [138, 92, 255], y: 0.18, amp: 0.05, freq: 0.9, speed: 0.00007, height: 0.3, alpha: 0.26 },
  ];
  let w = 0, h = 0, stars = [];

  function size() {
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    w = canvas.clientWidth; h = canvas.clientHeight;
    canvas.width = Math.max(1, Math.round(w * ratio));
    canvas.height = Math.max(1, Math.round(h * ratio));
    ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    glow.width = Math.max(1, Math.round(w / 4));
    glow.height = Math.max(1, Math.round(h / 4));
    let seed = 7; // the same stars at each load
    const random = () => (seed = (seed * 16807) % 2147483647) / 2147483647;
    stars = Array.from({ length: Math.round((w * h) / 2600) }, () => ({
      x: random() * w, y: random() * h * 0.75, r: random() * 1.1 + 0.2, a: random() * 0.6 + 0.2,
    }));
  }

  function draw(t) {
    const gw = glow.width, gh = glow.height;
    gtx.globalCompositeOperation = "source-over";
    gtx.clearRect(0, 0, gw, gh);
    gtx.globalCompositeOperation = "lighter";
    const step = Math.max(2, gw / 120);
    for (const c of curtains) {
      const [r, g, b] = c.rgb;
      const phase = t * c.speed;
      for (let x = -step; x < gw + step; x += step) {
        const u = x / gw;
        const base = gh * (c.y + c.amp * Math.sin((u * c.freq + phase) * 2 * Math.PI) + 0.03 * Math.sin(u * 17 + phase * 20));
        const length = gh * c.height * (0.6 + 0.4 * Math.sin(u * 9 + phase * 11));
        const flicker = 0.65 + 0.35 * Math.sin(u * 31 + phase * 40);
        const fill = gtx.createLinearGradient(0, base - length, 0, base);
        fill.addColorStop(0, `rgba(${r}, ${g}, ${b}, 0)`);
        fill.addColorStop(0.75, `rgba(${r}, ${g}, ${b}, ${c.alpha * flicker * 0.6})`);
        fill.addColorStop(1, `rgba(${r}, ${g}, ${b}, ${c.alpha * flicker})`);
        gtx.fillStyle = fill;
        gtx.fillRect(x, base - length, step + 1, length);
      }
    }

    ctx.fillStyle = "#0b1424";
    ctx.fillRect(0, 0, w, h);
    ctx.filter = "blur(8px)"; // where the browser can; scaling up softens it anyway
    ctx.drawImage(glow, 0, 0, w, h);
    ctx.filter = "none";
    for (const s of stars) {
      ctx.fillStyle = `rgba(232, 238, 247, ${s.a})`;
      ctx.beginPath(); ctx.arc(s.x, s.y, s.r, 0, 2 * Math.PI); ctx.fill();
    }
  }

  let frame = 0;
  function loop(t) { draw(t); frame = requestAnimationFrame(loop); }
  function start() {
    cancelAnimationFrame(frame);
    if (still.matches || document.hidden) draw(0);
    else frame = requestAnimationFrame(loop);
  }

  size();
  start();
  window.addEventListener("resize", () => { size(); draw(performance.now()); });
  document.addEventListener("visibilitychange", start);
  still.addEventListener("change", start);
})();

// The screenshots open larger over the page: the arrows (or a swipe) go
// from one to the next, Esc or the close button (or a swipe down) closes.
(function () {
  const box = document.querySelector("dialog.lightbox");
  const links = Array.from(document.querySelectorAll("a[data-caption]"));
  if (!box || !box.showModal || !links.length) return;
  const img = box.querySelector("img");
  const caption = box.querySelector("figcaption");
  let at = 0;
  function show(i) {
    at = (i + links.length) % links.length;
    img.src = links[at].href;
    img.alt = links[at].querySelector("img") ? links[at].querySelector("img").alt : "";
    caption.textContent = links[at].dataset.caption;
  }
  links.forEach((link, i) => link.addEventListener("click", (event) => {
    if (event.metaKey || event.ctrlKey || event.shiftKey) return;
    event.preventDefault();
    show(i);
    box.showModal();
  }));
  box.querySelector(".lightbox-close").addEventListener("click", () => box.close());
  box.querySelector(".lightbox-prev").addEventListener("click", () => show(at - 1));
  box.querySelector(".lightbox-next").addEventListener("click", () => show(at + 1));
  box.addEventListener("click", (event) => { if (event.target === box || event.target.tagName === "FIGURE") box.close(); });
  box.addEventListener("keydown", (event) => {
    if (event.key === "ArrowLeft") show(at - 1);
    if (event.key === "ArrowRight") show(at + 1);
  });
  let start = null;
  box.addEventListener("touchstart", (e) => { start = e.touches[0]; }, { passive: true });
  box.addEventListener("touchend", (e) => {
    if (!start) return;
    const dx = e.changedTouches[0].clientX - start.clientX, dy = e.changedTouches[0].clientY - start.clientY;
    if (Math.abs(dx) > 50 && Math.abs(dx) > Math.abs(dy)) show(at + (dx < 0 ? 1 : -1));
    else if (dy > 80) box.close();
    start = null;
  });
})();

// A copy button on each block of code (not on the names changed of the page
// on borealis, a diff: .no-copy).
(function () {
  const texts = document.querySelector("[data-copy]");   // the body: the page's language
  const label = texts ? texts.dataset.copy : "Copy";
  const done = texts ? texts.dataset.copied : "Copied";
  if (!navigator.clipboard) return;
  for (const pre of document.querySelectorAll("pre:not(.no-copy)")) {
    const wrap = document.createElement("div");
    wrap.className = "code-wrap";
    pre.parentNode.insertBefore(wrap, pre);
    wrap.appendChild(pre);
    const button = document.createElement("button");
    button.type = "button";
    button.className = "copy";
    button.textContent = label;
    button.addEventListener("click", () => {
      navigator.clipboard.writeText(pre.innerText.replace(/\n$/, "")).then(() => {
        button.textContent = done;
        button.classList.add("done");
        setTimeout(() => { button.textContent = label; button.classList.remove("done"); }, 1600);
      });
    });
    wrap.appendChild(button);
  }
})();

// The section in view, marked in the page's table of contents.
(function () {
  const links = Array.from(document.querySelectorAll(".toc a"));
  if (!links.length || !("IntersectionObserver" in window)) return;
  const byId = new Map(links.map((a) => [decodeURIComponent(a.hash.slice(1)), a]));
  const visible = new Set();
  const observer = new IntersectionObserver((entries) => {
    for (const e of entries) e.isIntersecting ? visible.add(e.target.id) : visible.delete(e.target.id);
    const first = Array.from(byId.keys()).find((id) => visible.has(id));
    if (!first) return;
    links.forEach((a) => a.classList.toggle("active", byId.get(first) === a));
  }, { rootMargin: "0px 0px -70% 0px" });
  for (const id of byId.keys()) {
    const heading = document.getElementById(id);
    if (heading) observer.observe(heading);
  }
})();

// The documentation's search, over docs/search.json (the API's over
// api/search.json): each word typed must be
// in the section; its name counts more than its text.
(function () {
  const box = document.querySelector(".doc-search");
  if (!box || !window.fetch) return;
  const input = box.querySelector("input");
  const list = box.querySelector("ol");
  // The links' base: the index's language folder, or data-base (the API's).
  const base = input.dataset.base ?? input.dataset.index.replace(/docs\/search\.json$/, "");
  let index = null;
  box.hidden = false;
  function load() {
    if (index) return Promise.resolve(index);
    return fetch(input.dataset.index).then((r) => r.json()).then((data) => (index = data));
  }
  function fold(s) { return s.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, ""); }
  function search() {
    const words = fold(input.value).split(/\s+/).filter((w) => w.length > 1);
    if (!words.length) { list.hidden = true; list.textContent = ""; return; }
    load().then((entries) => {
      const found = [];
      for (const e of entries) {
        const name = fold(`${e.p} ${e.h}`), text = fold(e.t);
        let score = 0;
        for (const w of words) {
          if (name.includes(w)) score += 10;
          else if (text.includes(w)) score += 1;
          else { score = 0; break; }
        }
        if (score) found.push([score, e]);
      }
      found.sort((a, b) => b[0] - a[0]);
      list.textContent = "";
      if (!found.length) {
        const p = document.createElement("p");
        p.textContent = input.dataset.none;
        list.appendChild(p);
      }
      for (const [, e] of found.slice(0, 12)) {
        const li = document.createElement("li");
        const a = document.createElement("a");
        a.href = base + e.u;
        a.textContent = e.h || e.p;
        if (e.h) {
          const small = document.createElement("small");
          small.textContent = e.p;
          a.appendChild(small);
        }
        li.appendChild(a);
        list.appendChild(li);
      }
      list.hidden = false;
    });
  }
  input.addEventListener("input", search);
  input.addEventListener("focus", load, { once: true });
  input.addEventListener("keydown", (event) => {
    if (event.key === "Escape") { input.value = ""; search(); }
    if (event.key === "Enter") { const a = list.querySelector("a"); if (a) location.href = a.href; }
  });
  // "/" goes to the search, as on GitHub.
  document.addEventListener("keydown", (event) => {
    if (event.key === "/" && !/^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName)) {
      event.preventDefault();
      input.focus();
    }
  });
})();

// The page not found, served at any address: in the address' language
// (/fr/…, /zh-hant/…), else the one chosen in the header's menu
// (boreale-site's lang.js keeps it as "boreale-lang": the folder, "en-US"
// for English), else the browser's. Its blocks' data-lang is the code
// (fr-CA), their folder the code in lower case. boreale-site's lang.js
// sends a page not found to its language's own 404.html (data-lost); Aurore
// has one 404.html holding the 15, so it chooses here, the browser's
// language read as lang.js does (lang.js does not expose its choice).
(function () {
  const blocks = Array.from(document.querySelectorAll(".not-found[data-lang]"));
  if (blocks.length < 2) return;
  const byFolder = (f) => blocks.find((b) => (b.dataset.lang === "en-US" ? "en-US" : b.dataset.lang.toLowerCase()) === f);
  const browser = () => {
    for (const tag of navigator.languages || [navigator.language || ""]) {
      const t = String(tag).toLowerCase();
      const [base, region = ""] = t.split("-");
      if (base === "en") return "en-US";
      if (base === "fr") return region === "ca" ? "fr-ca" : "fr";
      if (base === "es") return !region || region === "es" ? "es" : "es-419";
      if (base === "pt") return region === "br" ? "pt-br" : "pt";
      if (base === "zh") return ["tw", "hk", "mo", "hant"].includes(region) || t.includes("hant") ? "zh-hant" : "zh-hans";
      if (byFolder(base)) return base;
    }
    return "en-US";
  };
  let chosen = null;
  try { chosen = localStorage.getItem("boreale-lang"); } catch (e) { /* storage refused: no memory */ }
  const first = location.pathname.toLowerCase().split("/").filter(Boolean)[0] || "";
  const block = (first !== "en-us" && byFolder(first)) || byFolder(chosen) || byFolder(browser());
  if (!block) return;
  for (const b of blocks) b.hidden = b !== block;
  document.documentElement.lang = block.lang;
  document.title = `${block.querySelector("h1").textContent} · Aurore`;
})();

// On a phone, the documentation's list of pages starts folded, under the
// page's title (open in the HTML, for a page read without script).
(function () {
  const menu = document.querySelector(".doc-menu");
  if (menu && window.matchMedia("(max-width: 899px)").matches) menu.open = false;
})();
// The documentation: once the header is scrolled away, a slim bar at the top
// (the way home, the page"s name, the search, the light reading); and the
// light reading itself, kept as aurore-reading (reading.js sets it before the
// first paint).
(function () {
  const root = document.documentElement;
  const toggles = [...document.querySelectorAll(".reading-toggle")];
  const shown = () => toggles.forEach((b) => b.setAttribute("aria-pressed", String(root.dataset.reading === "light")));
  for (const b of toggles) {
    b.addEventListener("click", () => {
      const light = root.dataset.reading !== "light";
      if (light) root.dataset.reading = "light"; else delete root.dataset.reading;
      try { localStorage.setItem("aurore-reading", light ? "light" : "night"); } catch { /* storage refused */ }
      shown();
    });
  }
  shown();

  const bar = document.querySelector(".doc-bar");
  const sky = document.querySelector(".sky");
  if (!bar || !sky || !("IntersectionObserver" in window)) return;
  bar.hidden = false;
  new IntersectionObserver((entries) => {
    bar.classList.toggle("on", !entries[entries.length - 1].isIntersecting);
  }).observe(sky);
  bar.querySelector(".doc-bar-search").addEventListener("click", () => {
    const input = document.querySelector(".doc-search input");
    if (!input) return;
    window.scrollTo({ top: 0 });
    input.focus();
  });
})();

// The showcase (site.css's block of the same name): the cards' edge lit
// where the pointer is, the home page's sections rising into view once, the
// screens on a wide screen, the one chosen large over a strip of the others,
// and the code's editor: a tab for each file, and the screen's button saying
// what the activity says.
(function () {
  for (const card of document.querySelectorAll(".card")) {
    card.addEventListener("pointermove", (e) => {
      const box = card.getBoundingClientRect();
      card.style.setProperty("--mx", `${e.clientX - box.left}px`);
      card.style.setProperty("--my", `${e.clientY - box.top}px`);
    });
  }

  const still = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  if (!still && "IntersectionObserver" in window) {
    const seen = new IntersectionObserver((entries) => {
      for (const e of entries) {
        if (e.isIntersecting) { e.target.classList.add("in"); seen.unobserve(e.target); }
      }
    }, { rootMargin: "0px 0px -12% 0px" });
    for (const part of document.querySelectorAll(".home main > section")) {
      // Already in view at load (a link to a section): shown at once.
      if (part.getBoundingClientRect().top < window.innerHeight * 0.88) continue;
      part.classList.add("reveal");
      [...part.querySelectorAll(".card")].forEach((card, i) => card.style.setProperty("--i", i % 4));
      seen.observe(part);
    }
  }

  // The editor: the panes become tabs (the arrows, Home and End go from
  // one to the next).
  const editor = document.querySelector(".editor");
  if (editor) {
    const panes = [...editor.querySelectorAll(".editor-pane")];
    const list = document.createElement("div");
    list.className = "editor-tabs";
    list.setAttribute("role", "tablist");
    const tabs = panes.map((pane, i) => {
      const tab = document.createElement("button");
      tab.type = "button";
      tab.className = "editor-tab";
      tab.id = `${pane.id}-tab`;
      tab.setAttribute("role", "tab");
      tab.setAttribute("aria-controls", pane.id);
      tab.textContent = pane.dataset.tab;
      const name = pane.querySelector(".pane-name");
      if (name) tab.setAttribute("aria-label", `${pane.dataset.tab}: ${name.textContent}`);
      pane.setAttribute("role", "tabpanel");
      pane.setAttribute("aria-labelledby", tab.id);
      tab.addEventListener("click", () => choose(i));
      list.append(tab);
      return tab;
    });
    const choose = (at, focus) => {
      tabs.forEach((tab, i) => {
        tab.setAttribute("aria-selected", String(i === at));
        tab.tabIndex = i === at ? 0 : -1;
        panes[i].classList.toggle("on", i === at);
      });
      if (focus) tabs[at].focus();
    };
    list.addEventListener("keydown", (e) => {
      const at = tabs.indexOf(document.activeElement);
      const to = { ArrowRight: at + 1, ArrowLeft: at - 1, Home: 0, End: tabs.length - 1 }[e.key];
      if (at < 0 || to === undefined) return;
      e.preventDefault();
      choose((to + tabs.length) % tabs.length, true);
    });
    editor.querySelector(".editor-bar").append(list);
    editor.classList.add("tabbed");
    choose(0);
  }
  // A block of code wider than its box: its right edge fades while there
  // is more to scroll.
  for (const pre of document.querySelectorAll("pre")) {
    const box = pre.closest(".editor-pane") || (pre.parentElement.classList.contains("code-wrap") ? pre.parentElement : null);
    if (!box) continue;
    const update = () => box.classList.toggle("more", pre.scrollLeft + pre.clientWidth < pre.scrollWidth - 2);
    pre.addEventListener("scroll", update, { passive: true });
    if ("ResizeObserver" in window) new ResizeObserver(update).observe(pre);
    update();
  }
  const hello = document.querySelector(".preview-button");
  const notice = document.querySelector(".preview-notice");
  if (hello && notice) {
    let timer = 0;
    hello.addEventListener("click", () => {
      notice.textContent = hello.dataset.notice;
      notice.classList.add("on");
      clearTimeout(timer);
      timer = setTimeout(() => notice.classList.remove("on"), 2400);
    });
  }

  const gallery = document.querySelector(".gallery");
  if (!gallery) return;
  const figures = [...gallery.querySelectorAll(":scope > figure")];
  if (figures.length < 2) return;
  const stage = document.createElement("figure");
  stage.className = "gallery-stage";
  const link = document.createElement("a");
  const img = document.createElement("img");
  img.width = 1280;
  img.height = 720;
  link.append(img);
  const caption = document.createElement("figcaption");
  const name = document.createElement("strong");
  const what = document.createElement("span");
  caption.append(name, what);
  stage.append(link, caption);
  let chosen = figures[0];
  const choose = (figure) => {
    const shot = figure.querySelector("img");
    chosen = figure;
    for (const f of figures) f.classList.toggle("on", f === figure);
    link.href = figure.querySelector("a").href;
    img.src = shot.currentSrc || shot.src;
    img.alt = shot.alt;
    name.textContent = figure.querySelector("figcaption")?.textContent || "";
    what.textContent = shot.alt === name.textContent ? "" : shot.alt;
  };
  choose(figures[0]);
  gallery.prepend(stage);
  gallery.classList.add("staged");
  const wide = window.matchMedia("(min-width: 861px)");
  // On a wide screen, a screen of the strip comes up large, and the large
  // one opens full screen (the lightbox, on the one chosen); elsewhere (one
  // under the other) each opens full screen, as before. Caught before the
  // lightbox's own listeners, on the links.
  let passing = false;
  gallery.addEventListener("click", (e) => {
    const figure = e.target.closest("figure");
    if (passing || !wide.matches || !figure || e.metaKey || e.ctrlKey || e.shiftKey) return;
    e.preventDefault();
    e.stopPropagation();
    if (figure === stage) {
      passing = true;
      chosen.querySelector("a").click();
      passing = false;
    } else {
      choose(figure);
    }
  }, true);
})();
