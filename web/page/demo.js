// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The demo's page in a browser (web/README.md), as PlayGuard's and
// PlayShelf's simulators: starts aurore_demo (the Emscripten build of the
// demo) in the canvas with the settings of the address, gives it an
// on-screen pad and the tools under the screen.
//
// - The address says the settings: ?simulate=3ds (a console drawn around
//   the screen, AURORE_SIMULATE), ?theme=winter (a theme folder or file of
//   the demo's resources, or a preset: -t), ?lang=fr (AURORE_LANG) and
//   ?variant=light (the page's theme, and the demo's light or dark one:
//   AURORE_THEME). The side panel changes the first two; **Apply** loads
//   the page again with them, the demo starting over.
// - The page is in the consoles' 15 languages (i18n/<code>.json, English
//   underneath), the browser's unless the address says `lang=`; the header's
//   language menu is the website's.
// - Its theme is dark by default; the header chooses Auto (the browser's) or
//   Light instead, kept for the next visits. The demo reads its own at start:
//   when the page's changes, the page offers to start it again.
// - <body data-demo> says where it is: loading, running, ready or error
//   (tools/web_smoke.py waits on it).
'use strict';

const LANGUAGES = [
  ['en-US', 'English'], ['fr', 'Français'], ['fr-CA', 'Français (Canada)'], ['de', 'Deutsch'],
  ['es', 'Español'], ['es-419', 'Español (Latinoamérica)'], ['it', 'Italiano'], ['nl', 'Nederlands'],
  ['pt', 'Português'], ['pt-BR', 'Português (Brasil)'], ['ru', 'Русский'], ['ja', '日本語'],
  ['ko', '한국어'], ['zh-Hans', '简体中文'], ['zh-Hant', '繁體中文'],
];
// Each language's flag, as on the website (tools/build_site.py's FLAGS;
// twemoji-flags.woff2 draws them the same everywhere, Windows included).
const FLAGS = {
  'en-US': 'US', fr: 'FR', 'fr-CA': 'CA', de: 'DE', es: 'ES', 'es-419': 'MX', it: 'IT', nl: 'NL',
  pt: 'PT', 'pt-BR': 'BR', ru: 'RU', ja: 'JP', ko: 'KR', 'zh-Hans': 'CN', 'zh-Hant': 'TW',
};
const flag = (code) => [...(FLAGS[code] || '')].map((c) => String.fromCodePoint(0x1F1E6 + c.charCodeAt(0) - 65)).join('');
// The website's folder of each language (tools/build_site.py's LANGUAGES).
const folder = (code) => (code === 'en-US' ? '' : `${code.toLowerCase()}/`);
const VARIANTS = ['auto', 'light', 'dark'];
const DEFAULT_VARIANT = 'dark';
// The consoles of demo/src/simulation.cpp, the desktop's window first.
const DEVICES = ['desktop', 'switch', 'vita', 'ps4', '3ds', 'phone'];
const DEVICE_NAMES = { switch: 'Switch', vita: 'PS Vita', ps4: 'PS4', '3ds': 'New 3DS' };
// The settings the panel changes, as the address names them.
const FIELDS = ['simulate', 'theme'];
const REPO = 'JigSawFr/Aurore';

const $ = (id) => document.getElementById(id);
const canvas = $('canvas');
let text = {};                 // the page's language, English underneath
let language = 'en-US';
let started = false;
let ready = false;             // the demo's runtime is up: ccall works
let startVariant = 'dark';     // the theme the demo was started with
let current = new URLSearchParams(location.search);

const t = (key) => text[key] ?? key;

// Links out (the rule "liens sortants anonymes"): our own sites (boreale.dev
// and its subdomains, the repositories of github.com/JigSawFr/) get the
// origin alone, any other site nothing. A fork's repository is another site.
function ownSite(href) {
  let u;
  try { u = new URL(href); } catch { return false; }
  const host = u.hostname.toLowerCase();
  return u.protocol === 'https:' && (host === 'boreale.dev' || host.endsWith('.boreale.dev') ||
    (host === 'github.com' && u.pathname.toLowerCase().startsWith('/jigsawfr/')));
}

function linkOutAttributes(href) {
  if (!/^https?:/i.test(href)) return {};
  return ownSite(href) ? { referrerpolicy: 'strict-origin' } : { rel: 'noreferrer' };
}

function el(tag, attrs = {}, ...children) {
  const node = document.createElement(tag);
  if (tag === 'a') {
    const out = linkOutAttributes(attrs.href || '');
    attrs = { ...out, ...attrs, ...(out.rel && attrs.rel ? { rel: `${attrs.rel} ${out.rel}` } : {}) };
  }
  for (const [k, v] of Object.entries(attrs)) {
    if (v === false || v == null) continue;
    if (k === 'text') node.textContent = v;
    else node.setAttribute(k, v === true ? '' : v);
  }
  node.append(...children);
  return node;
}

// ---- Texts ------------------------------------------------------------

function browserLanguage() {
  for (const tag of navigator.languages ?? [navigator.language]) {
    const found = consoleLanguage(tag);
    if (found) return found;
  }
  return 'en-US';
}

// The console's language a tag is (fr-BE: fr, es-MX: es-419, zh-TW:
// zh-Hant…), or null.
function consoleLanguage(tag) {
  if (!tag) return null;
  const lower = tag.toLowerCase();
  const exact = LANGUAGES.find(([code]) => code.toLowerCase() === lower);
  if (exact) return exact[0];
  const [base, ...rest] = lower.split('-');
  if (base === 'zh') return rest.some((r) => ['hant', 'tw', 'hk', 'mo'].includes(r)) ? 'zh-Hant' : 'zh-Hans';
  if (base === 'pt') return rest.includes('br') ? 'pt-BR' : 'pt';
  if (base === 'fr') return rest.includes('ca') ? 'fr-CA' : 'fr';
  if (base === 'es') return rest.length && !rest.includes('es') ? 'es-419' : 'es';
  if (base === 'en') return 'en-US';
  return LANGUAGES.find(([code]) => code.toLowerCase() === base)?.[0] ?? null;
}

async function loadTexts(code) {
  const get = async (c) => {
    try {
      const res = await fetch(`i18n/${c}.json`);
      return res.ok ? await res.json() : {};
    } catch { return {}; }
  };
  const [en, own] = await Promise.all([get('en-US'), code === 'en-US' ? {} : get(code)]);
  text = { ...en, ...own };
}

function translatePage() {
  document.documentElement.lang = language;
  document.title = `Aurore · ${t('title')}`;
  for (const node of document.querySelectorAll('[data-i18n]')) node.textContent = t(node.dataset.i18n);
  for (const node of document.querySelectorAll('[data-i18n-html]')) node.innerHTML = t(node.dataset.i18nHtml);
  for (const node of document.querySelectorAll('[data-i18n-title]')) node.title = t(node.dataset.i18nTitle);
  for (const node of document.querySelectorAll('[data-i18n-label]')) node.setAttribute('aria-label', t(node.dataset.i18nLabel));
  for (const node of document.querySelectorAll('[data-i18n-group]')) node.label = t(node.dataset.i18nGroup);
}

// ---- The address ------------------------------------------------------------

const linkFor = (values) => {
  const query = values.toString();
  return `${location.pathname}${query ? `?${query}` : ''}`;
};
const apply = (values) => { location.href = linkFor(values); };

// ---- The theme ------------------------------------------------------------

const dark = matchMedia('(prefers-color-scheme: dark)');

function chosenVariant() {
  const fromUrl = current.get('variant');
  if (VARIANTS.includes(fromUrl)) return fromUrl;
  let kept = null;
  try { kept = localStorage.getItem('aurore-variant'); } catch { /* storage refused */ }
  return VARIANTS.includes(kept) ? kept : DEFAULT_VARIANT;
}
const effectiveVariant = (choice) => (choice === 'auto' ? (dark.matches ? 'dark' : 'light') : choice);

function showVariant() {
  const effective = effectiveVariant(chosenVariant());
  document.documentElement.dataset.themeChoice = chosenVariant();
  document.documentElement.dataset.theme = effective;
  // Started with the other one (Auto following a change of the system's):
  // the demo reads its theme at start, so offer to start it again.
  $('variant-notice').hidden = !started || effective === startVariant;
}

function chooseVariant(choice) {
  try { localStorage.setItem('aurore-variant', choice); } catch { /* storage refused */ }
  const values = new URLSearchParams(current);
  if (choice === DEFAULT_VARIANT) values.delete('variant');
  else values.set('variant', choice);
  if (effectiveVariant(choice) === startVariant || !started) {
    // The demo already has it: only the address changes.
    current = values;
    history.replaceState(null, '', linkFor(values));
    showVariant();
    return;
  }
  apply(values);
}

// ---- The header ------------------------------------------------------------

// The website's pages in the page's language (tools/build_site.py's
// folders): its home, and the header's links as on the website.
function homeLinks() {
  const home = `../${folder(language)}`;
  $('home-link').href = home;
  $('nav-features').href = `${home}#features`;
  $('nav-docs').href = `${home}docs/getting-started/`;
  // Boréale's other sites, in the page's language (the same folders).
  for (const link of document.querySelectorAll('#sites [data-site]')) link.href = link.dataset.site + folder(language);
  $('sites-button').setAttribute('aria-label', `Aurore · ${t('sites_label')}`);
}

// Boréale's site switcher: the menu of the four sites opens under the name, as
// on the website (site/assets/site.js' "site switcher" block). Without
// popovers (an old browser), the menu stays closed.
function setUpSites() {
  const menu = $('sites');
  const anchor = menu.closest('.switcher');
  if (!menu.showPopover) return;
  menu.addEventListener('beforetoggle', (event) => {
    if (event.newState !== 'open') return;
    const box = anchor.getBoundingClientRect();
    const screen = document.documentElement.clientWidth;
    const width = Math.min(400, screen - 24); // demo.css' width
    menu.style.top = `${box.bottom + window.scrollY + 6}px`;
    menu.style.left = `${Math.max(12, Math.min(box.left, screen - width - 12)) + window.scrollX}px`;
  });
}

// The header's languages, as on the website: a button with the page's flag
// and name (its code on a phone), and under it their menu (a popover: the
// browser closes it on Esc or a click elsewhere, and gives the focus back to
// the button), each with its flag, the page's marked. Each is a link to
// this page in it (the address' settings kept), the demo starting again in
// it; the arrows move through them.
function setUpLanguages() {
  const button = $('language-button');
  const menu = $('languages');
  const name = LANGUAGES.find(([code]) => code === language)?.[1] ?? language;
  $('language-flag').textContent = flag(language);
  $('language-name').textContent = name;
  $('language-code').textContent = language.split('-')[0].toUpperCase();
  button.setAttribute('aria-label', `${t('language')} · ${name}`);
  const values = (code) => { const v = new URLSearchParams(current); v.set('lang', code); return v; };
  const links = LANGUAGES.map(([code, label]) => el('a', { href: linkFor(values(code)), 'data-lang': code, lang: code, hreflang: code, 'aria-current': code === language ? 'true' : false },
    el('span', { class: 'flag', 'aria-hidden': 'true', text: flag(code) }), label));
  menu.replaceChildren(...links.map((link) => el('li', {}, link)));
  if (!menu.showPopover) return;
  const inUse = () => links.find((a) => a.dataset.lang === language) ?? links[0];
  menu.addEventListener('beforetoggle', (event) => {
    if (event.newState !== 'open') return;
    // The settings as they are now (the theme): the links follow.
    for (const link of links) link.href = linkFor(values(link.dataset.lang));
    // Under the button, its right edge on the button's, within the screen.
    const box = button.getBoundingClientRect();
    const screen = document.documentElement.clientWidth;
    const width = Math.min(460, screen - 24); // demo.css' width
    menu.style.top = `${box.bottom + window.scrollY + 6}px`;
    menu.style.right = `${Math.max(12, Math.min(screen - box.right, screen - width - 12))}px`;
    requestAnimationFrame(() => inUse().scrollIntoView({ block: 'nearest' }));
  });
  // The arrows: down (or up) on the button opens the menu at the language in
  // use; in the menu, through the two columns, Home and End to either end.
  button.addEventListener('keydown', (e) => {
    if (e.key !== 'ArrowDown' && e.key !== 'ArrowUp') return;
    e.preventDefault();
    menu.showPopover();
    inUse().focus();
  });
  menu.addEventListener('keydown', (e) => {
    const i = links.indexOf(document.activeElement);
    const step = { ArrowRight: 1, ArrowLeft: -1, ArrowDown: 2, ArrowUp: -2 }[e.key];
    let next = null;
    if (step) next = links[i < 0 ? 0 : i + step] ?? null;
    else if (e.key === 'Home') next = links[0];
    else if (e.key === 'End') next = links[links.length - 1];
    else return;
    e.preventDefault();
    next?.focus();
  });
}

function setUpHeader() {
  setUpSites();
  setUpLanguages();
  const variant = $('variant-select');
  for (const choice of VARIANTS) variant.append(el('option', { value: choice, text: t(`variant_${choice}`) }));
  variant.value = chosenVariant();
  variant.addEventListener('change', () => chooseVariant(variant.value));
  dark.addEventListener('change', showVariant);
  $('variant-restart').addEventListener('click', () => location.reload());
}

// The build's commit (build.json, written by CI), nothing for a local build.
async function showBuild() {
  try {
    const res = await fetch('build.json', { cache: 'no-cache' });
    if (!res.ok) return null;
    const b = await res.json();
    const repo = `https://github.com/${b.repo || REPO}`;
    const where = b.pr ? el('a', { href: `${repo}/pull/${b.pr}`, text: `PR #${b.pr}` }) : el('span', { text: b.ref || 'main' });
    const sha = el('a', { href: `${repo}/commit/${b.sha}` }, el('code', { text: String(b.sha).slice(0, 7) }));
    $('build').replaceChildren(where, ' · ', sha, b.title ? ` · ${b.title}` : '');
    $('build').title = b.title ?? '';
    return b;
  } catch { return null; }
}

// ---- The panel ------------------------------------------------------------

function setUpPanel() {
  const layout = document.querySelector('.layout');
  const toggle = $('panel-toggle');
  const show = (on) => {
    layout.classList.toggle('no-panel', !on);
    toggle.setAttribute('aria-expanded', String(on));
    try { localStorage.setItem('aurore-panel', on ? '1' : '0'); } catch { /* storage refused */ }
  };
  let kept = null;
  try { kept = localStorage.getItem('aurore-panel'); } catch { /* storage refused */ }
  show(kept !== '0');
  toggle.addEventListener('click', () => {
    // Narrow, the panel is under the screen: the button goes there.
    if (matchMedia('(max-width: 900px)').matches) { $('panel').scrollIntoView({ behavior: 'smooth' }); return; }
    show(layout.classList.contains('no-panel'));
  });

  // The tabs: the settings, the demo's log.
  const tabs = [...document.querySelectorAll('.tab')];
  const select = (tab) => {
    for (const other of tabs) {
      const on = other === tab;
      other.setAttribute('aria-selected', String(on));
      other.tabIndex = on ? 0 : -1;
      document.querySelector(`[data-body="${other.dataset.tab}"]`).hidden = !on;
    }
  };
  for (const tab of tabs) tab.addEventListener('click', () => select(tab));
  $('tab-settings').parentElement.addEventListener('keydown', (e) => {
    const step = { ArrowRight: 1, ArrowLeft: -1 }[e.key];
    if (!step) return;
    const next = tabs[(tabs.indexOf(document.activeElement) + step + tabs.length) % tabs.length];
    select(next);
    next.focus();
  });

  // The consoles, by their names in the page's language.
  const simulate = $('simulate');
  for (const id of DEVICES) simulate.append(el('option', { value: id === 'desktop' ? '' : id, text: DEVICE_NAMES[id] ?? t(`simulate_${id}`) }));
  for (const name of FIELDS) {
    const field = $(name);
    field.value = current.get(name) ?? '';
    // One the list does not have (a newer build's): the default, and noted.
    if (current.get(name) && field.value !== current.get(name)) {
      field.value = '';
      log(`${name}=${current.get(name)}: not one of this page's, left out`, true);
    }
    field.addEventListener('change', markChanges);
  }
  $('apply').addEventListener('click', () => apply(formValues()));
  $('copy').addEventListener('click', async () => {
    const link = new URL(linkFor(formValues()), location.href).href;
    try { await navigator.clipboard.writeText(link); toast(t('copied')); } catch { prompt(t('copy'), link); }
  });
  $('reset').addEventListener('click', () => {
    for (const name of FIELDS) $(name).value = '';
    markChanges();
  });
}

// The address with the panel's settings in it.
function formValues() {
  const values = new URLSearchParams(current);
  for (const name of FIELDS) {
    const value = $(name).value;
    if (value) values.set(name, value); else values.delete(name);
  }
  return values;
}

// What the panel changed and Apply would start: shown on the field and the button.
function markChanges() {
  let changed = false;
  for (const name of FIELDS) {
    const differs = ($(name).value || '') !== (current.get(name) ?? '');
    $(`field-${name}`).classList.toggle('changed', differs);
    changed ||= differs;
  }
  $('apply').classList.toggle('pending', changed);
}

// ---- The log, the status and the toasts ----------------------------------------

const logLines = [];
function log(line, error) {
  logLines.push([line, Boolean(error)]);
  if (logLines.length > 2000) logLines.shift();
  const pre = $('log');
  pre.append(error ? el('span', { class: 'err', text: `${line}\n` }) : `${line}\n`);
  if (pre.childNodes.length > 2000) pre.firstChild.remove();
  (error ? console.warn : console.log)(line);
}

function showStatus(message) { $('status').textContent = message; }

const mb = (bytes) => `${(bytes / 1048576).toFixed(1)} MB`;
function showProgress(done, total) {
  const bar = $('progress');
  bar.hidden = false;
  bar.max = total;
  bar.value = done;
  $('loading-size').textContent = done < total ? `${mb(done)} / ${mb(total)}` : '';
}

let toastTimer = 0;
function toast(message) {
  const box = $('toast');
  box.textContent = message;
  box.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { box.hidden = true; }, 2200);
}

function showError(message) {
  const overlay = $('overlay');
  overlay.hidden = false;
  overlay.classList.add('error');
  showStatus(message);
  $('progress').hidden = true;
  $('loading-size').textContent = '';
  document.body.dataset.demo = 'error';
  log(message, true);
}

// ---- The demo ------------------------------------------------------------

function start() {
  // Threads need a cross-origin isolated page: GitHub Pages sends no
  // headers, sw.js adds them, and the page loads once more under it.
  if (!window.crossOriginIsolated) {
    if (!('serviceWorker' in navigator) || !window.isSecureContext) { showError(t('no_isolation')); return; }
    let tried = false;
    try { tried = sessionStorage.getItem('aurore-coi') === '1'; } catch { /* storage refused */ }
    if (tried) { showError(t('no_isolation')); return; }
    try { sessionStorage.setItem('aurore-coi', '1'); } catch { /* idem */ }
    navigator.serviceWorker.register('sw.js').then(() => navigator.serviceWorker.ready)
      .then(() => location.reload(), () => showError(t('no_isolation')));
    return;
  }
  try { sessionStorage.removeItem('aurore-coi'); } catch { /* idem */ }
  if (!document.createElement('canvas').getContext('webgl2')) { showError(t('no_webgl')); return; }

  // Screenshots and videos read the canvas after the frame: keep its buffer.
  const getContext = HTMLCanvasElement.prototype.getContext;
  HTMLCanvasElement.prototype.getContext = function (type, attrs) {
    if (this === canvas && /^webgl/.test(type)) attrs = { ...(attrs || {}), preserveDrawingBuffer: true };
    return getContext.call(this, type, attrs);
  };

  startVariant = effectiveVariant(chosenVariant());
  const args = [];
  // A theme folder or file of the demo's resources (packaged at /app/resources), or a preset
  const theme = $('theme');
  if (current.get('theme') && theme.value) {
    const file = theme.selectedOptions[0].dataset.file;
    args.push('-t', file ? `/app/resources/${file}` : theme.value);
  }
  const simulate = $('simulate').value;
  log(`settings: ${current.toString() || '(defaults)'}; language ${language}, theme ${startVariant}`);
  showStatus(t('downloading'));
  window.Module = {
    canvas,
    arguments: args,
    preRun: [(m) => {
      // The keyboard reaches the demo from the screen only: Esc on a menu
      // of the page closes the menu, without a B press in the demo.
      m.ENV.SDL_EMSCRIPTEN_KEYBOARD_ELEMENT = '#canvas';
      m.ENV.AURORE_LANG = language;
      m.ENV.AURORE_THEME = startVariant.toUpperCase();
      if (simulate) m.ENV.AURORE_SIMULATE = simulate;
    }],
    setStatus(status) {
      if (!status) return;
      // "Downloading data... (12345/67890)": how far the download is.
      const m = /\((\d+(?:\.\d+)?)\/(\d+)\)/.exec(status);
      if (m) showProgress(Number(m[1]), Number(m[2]));
      else showStatus(t('starting'));
    },
    onRuntimeInitialized() {
      ready = true;
      document.body.dataset.demo = 'running';
      showStatus(t('starting'));
      // The first frame is drawn after main(): the screen shows then.
      requestAnimationFrame(() => requestAnimationFrame(() => {
        $('overlay').hidden = true;
        document.body.dataset.demo = 'ready';
        canvas.focus();
      }));
    },
    onAbort: (what) => showError(`${t('error')} ${what ?? ''}`),
    print: (line) => log(line),
    printErr: (line) => log(line, true),
  };
  started = true;
  document.body.append(el('script', { src: 'aurore_demo.js' }));
}

// ---- Input ------------------------------------------------------------

function press(button, down) {
  if (ready) window.Module.ccall('aurore_web_button', null, ['number', 'number'], [button, down ? 1 : 0]);
}

function setUpPad() {
  for (const button of $('pad').querySelectorAll('[data-button]')) {
    const code = Number(button.dataset.button);
    const up = (e) => {
      if (!button.classList.contains('active')) return;
      button.classList.remove('active');
      press(code, false);
      e.preventDefault();
    };
    button.addEventListener('pointerdown', (e) => {
      button.setPointerCapture?.(e.pointerId);
      button.classList.add('active');
      // A finger's press felt, as on a controller (Android; iOS has no vibration).
      if (e.pointerType === 'touch') navigator.vibrate?.(10);
      press(code, true);
      e.preventDefault();
    });
    button.addEventListener('pointerup', up);
    button.addEventListener('pointercancel', up);
    button.addEventListener('lostpointercapture', up);
    button.addEventListener('contextmenu', (e) => e.preventDefault());
  }
  const toggle = $('pad-toggle');
  const show = (on) => {
    $('pad').hidden = !on;
    toggle.setAttribute('aria-pressed', String(on));
    try { localStorage.setItem('aurore-pad', on ? '1' : '0'); } catch { /* storage refused */ }
  };
  let saved = null;
  try { saved = localStorage.getItem('aurore-pad'); } catch { /* storage refused */ }
  show(saved ? saved === '1' : matchMedia('(pointer: coarse)').matches);
  toggle.addEventListener('click', () => show($('pad').hidden));
}

// Full screen, where the browser allows it (Android, a computer; not an
// iPhone's Safari): only the screen and the pad (html.game), turned to
// landscape on a phone.
function setUpFullscreen() {
  const root = document.documentElement;
  if (!document.fullscreenEnabled || !root.requestFullscreen) return;
  const button = $('fullscreen');
  button.hidden = false;
  button.addEventListener('click', async () => {
    try {
      await root.requestFullscreen({ navigationUI: 'hide' });
      if (matchMedia('(pointer: coarse)').matches) await screen.orientation?.lock?.('landscape');
    } catch { /* refused, or no orientation lock: full screen as it is */ }
  });
  $('fs-exit').addEventListener('click', () => document.exitFullscreen().catch(() => {}));
  document.addEventListener('fullscreenchange', () => {
    const on = document.fullscreenElement === root;
    root.classList.toggle('game', on);
    if (!on) screen.orientation?.unlock?.();
    canvas.focus();
  });
}

// ---- The tools ------------------------------------------------------------

// A file name that says the settings: aurore_demo_simulate_3ds.png.
const fileName = (ext) => `aurore_demo${current.toString() ? `_${current.toString().replace(/[^a-z0-9.]+/gi, '_')}` : ''}.${ext}`;

function download(blob, name) {
  const a = el('a', { href: URL.createObjectURL(blob), download: name });
  a.click();
  setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

function screenshot() {
  canvas.toBlob((blob) => { if (blob) download(blob, fileName('png')); }, 'image/png');
}

// A video of the screen (WebM, 30 frames a second) until Stop.
let recorder = null;
function record() {
  const button = $('record');
  if (recorder) { recorder.stop(); return; }
  const type = ['video/webm;codecs=vp9', 'video/webm'].find((m) => MediaRecorder.isTypeSupported(m));
  const chunks = [];
  recorder = new MediaRecorder(canvas.captureStream(30), type ? { mimeType: type } : {});
  recorder.ondataavailable = (e) => { if (e.data.size) chunks.push(e.data); };
  recorder.onstop = () => {
    recorder = null;
    button.textContent = t('record');
    button.setAttribute('aria-pressed', 'false');
    download(new Blob(chunks, { type: 'video/webm' }), fileName('webm'));
  };
  recorder.start(1000);
  button.textContent = t('stop_recording');
  button.setAttribute('aria-pressed', 'true');
  canvas.focus();
}

// A bug report on GitHub, its form filled in: the link to this state, the
// build, the browser and the end of the log. GitHub takes a URL of 8 KB or so.
function report(build) {
  const where = build ? `commit ${String(build.sha).slice(0, 7)}${build.pr ? `, pull request #${build.pr}` : ''}` : 'a local build';
  const url = (lines) => {
    const tail = logLines.slice(-lines).map(([line]) => line).join('\n');
    const body = `In the demo in a browser (${where}), with these settings:\n${location.href}\n\n` +
                 `Browser: ${navigator.userAgent}\n\nThe end of the log:\n\`\`\`\n${tail}\n\`\`\``;
    return `https://github.com/${build?.repo || REPO}/issues/new?${new URLSearchParams({ title: '[demo] ', body })}`;
  };
  let lines = 40;
  while (lines > 0 && url(lines).length > 8000) lines -= 5;
  // Opened as a link, so the issue page gets the origin alone (el()).
  el('a', { href: url(lines), target: '_blank', rel: 'noopener' }).click();
}

// The tools under the screen: in a row where there is room, behind a
// button on a phone (closed again once one is used).
function setUpTools(build) {
  const tools = $('tools');
  const narrow = matchMedia('(max-width: 600px)');
  const fit = () => { tools.open = !narrow.matches; };
  fit();
  narrow.addEventListener('change', fit);
  tools.querySelector('.tool-list').addEventListener('click', (e) => {
    if (narrow.matches && e.target.closest('button')) tools.open = false;
  });
  $('shot').addEventListener('click', screenshot);
  if (window.MediaRecorder && canvas.captureStream) {
    $('record').hidden = false;
    $('record').addEventListener('click', record);
  }
  $('report').addEventListener('click', async () => report(await build));
}

// ---- Start ------------------------------------------------------------

async function main() {
  language = consoleLanguage(current.get('lang')) ?? browserLanguage();
  showVariant();
  await loadTexts(language);
  translatePage();
  homeLinks();
  setUpHeader();
  setUpPanel();
  setUpPad();
  setUpFullscreen();
  setUpTools(showBuild());
  markChanges();
  start();
  showVariant();
}

document.body.dataset.demo = 'loading';
main();
