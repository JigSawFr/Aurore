// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The demo's page in a browser (web/README.md): starts aurore_demo (the
// Emscripten build of the demo) in the canvas, with the theme and language of
// the address (?theme=winter&lang=fr), and gives it an on-screen pad.
'use strict';

(function () {
  const params = new URLSearchParams(location.search);
  const FR = /^fr\b/i.test(navigator.language || '');
  const TEXTS = {
    home: 'Aurore', theme: 'Thème', language: 'Langue', theme_default: 'Par défaut (Cadran)',
    theme_folders: 'Dossiers de thème', theme_files: 'Fichiers de thème', theme_presets: 'Préréglages',
    lang_auto: 'Celle du navigateur', pad: 'Manette', fullscreen: 'Plein écran', screen: 'La démo d\'Aurore',
    loading: 'Chargement d\'Aurore…', title: 'La démo d\'Aurore, dans votre navigateur',
    intro: 'La vraie bibliothèque, compilée avec Emscripten : le même code que sur une Switch, dessiné en WebGL 2. Cliquez sur l\'écran, puis :',
    key_move: 'déplacer le focus', key_buttons: 'les boutons du même nom', key_inspector: 'l\'inspecteur',
    touch: 'Un clic est un toucher ; la manette (affichée d\'office sur un écran tactile) a les boutons de la console.',
    link_site: 'Le site', link_start: 'Aurore en 15 minutes',
  };
  const MESSAGES = {
    isolation: ['This browser cannot run the demo: it needs threads (SharedArrayBuffer), which need a service worker here. Try a recent Chrome, Edge, Firefox or Safari, outside a private window.',
                'Ce navigateur ne peut pas lancer la démo : il lui faut des threads (SharedArrayBuffer), qui demandent ici un service worker. Essayez un Chrome, Edge, Firefox ou Safari récent, hors navigation privée.'],
    webgl: ['This browser or this device has no WebGL 2, which the demo draws with.',
            'Ce navigateur ou cet appareil n\'a pas WebGL 2, avec lequel la démo dessine.'],
    crashed: ['The demo stopped: ', 'La démo s\'est arrêtée : '],
    downloading: ['Downloading', 'Téléchargement'],
    starting: ['Starting…', 'Démarrage…'],
  };
  const say = (key) => MESSAGES[key][FR ? 1 : 0];

  if (FR) {
    document.documentElement.lang = 'fr';
    document.title = 'Démo d\'Aurore · dans votre navigateur';
    for (const e of document.querySelectorAll('[data-i18n]')) {
      const text = TEXTS[e.dataset.i18n];
      if (!text) continue;
      const span = e.querySelector(':scope > span:not([data-i18n])');
      if (e.tagName === 'A' && e.querySelector('img')) e.setAttribute('aria-label', text);
      else if (span && e.childElementCount) span.textContent = text;
      else e.textContent = text;
    }
    for (const e of document.querySelectorAll('[data-i18n-label]')) e.label = TEXTS[e.dataset.i18nLabel] || e.label;
    for (const e of document.querySelectorAll('[data-i18n-aria]')) e.setAttribute('aria-label', TEXTS[e.dataset.i18nAria]);
  }

  // The theme and the language: in the address, a change starts the demo again.
  const theme = document.getElementById('theme');
  const lang = document.getElementById('lang');
  theme.value = params.get('theme') || '';
  lang.value = params.get('lang') || '';
  for (const [select, key] of [[theme, 'theme'], [lang, 'lang']]) {
    if (params.get(key) && select.value !== params.get(key)) select.value = '';
    select.addEventListener('change', () => {
      if (select.value) params.set(key, select.value); else params.delete(key);
      const query = params.toString();
      location.href = location.pathname + (query ? `?${query}` : '');
    });
  }

  const loading = document.getElementById('loading');
  const loadingText = document.getElementById('loading-text');
  const progress = document.getElementById('progress');
  const loadingSize = document.getElementById('loading-size');
  function fail(message) {
    loading.hidden = true;
    document.getElementById('error').hidden = false;
    document.getElementById('error-text').textContent = message;
  }

  // Threads need a cross-origin isolated page: GitHub Pages sends no
  // headers, sw.js adds them, and the page loads once more under it.
  if (!window.crossOriginIsolated) {
    if (!('serviceWorker' in navigator)) return fail(say('isolation'));
    let tried = false;
    try { tried = sessionStorage.getItem('aurore-coi') === '1'; } catch (e) { /* storage refused */ }
    if (tried) return fail(say('isolation'));
    try { sessionStorage.setItem('aurore-coi', '1'); } catch (e) { /* idem */ }
    navigator.serviceWorker.register('sw.js').then(() => navigator.serviceWorker.ready)
      .then(() => location.reload(), () => fail(say('isolation')));
    return;
  }
  try { sessionStorage.removeItem('aurore-coi'); } catch (e) { /* idem */ }
  if (!document.createElement('canvas').getContext('webgl2')) return fail(say('webgl'));

  const canvas = document.getElementById('canvas');
  const args = [];
  // A theme folder or file of the demo's resources (packaged at /app/resources), or a preset
  if (theme.value) args.push('-t', theme.selectedOptions[0].dataset.file ? `/app/resources/${theme.selectedOptions[0].dataset.file}` : theme.value);
  const mb = (bytes) => `${(bytes / 1048576).toFixed(1)} MB`;

  let ready = false;
  window.Module = {
    canvas,
    arguments: args,
    preRun: [() => { if (lang.value) window.Module.ENV.AURORE_LANG = lang.value; }],
    setStatus(text) {
      // "Downloading data... (12345/67890)"
      const m = /\((\d+(?:\.\d+)?)\/(\d+)\)/.exec(text || '');
      if (m) {
        const done = Number(m[1]), total = Number(m[2]);
        progress.style.width = `${Math.round((done / total) * 100)}%`;
        loadingSize.textContent = `${say('downloading')} · ${mb(done)} / ${mb(total)}`;
      } else if (text) {
        loadingText.textContent = say('starting');
      }
    },
    onRuntimeInitialized() {
      ready = true;
      // The first frame is drawn after main(): the screen shows then.
      requestAnimationFrame(() => requestAnimationFrame(() => { loading.hidden = true; canvas.focus(); }));
    },
    onAbort(what) { fail(say('crashed') + what); },
    print: (text) => console.log(text),
    printErr: (text) => console.warn(text),
  };
  const script = document.createElement('script');
  script.src = 'aurore_demo.js';
  document.body.appendChild(script);

  // The on-screen pad: a press while a finger or the mouse is on a button.
  const pad = document.getElementById('pad');
  const toggle = document.getElementById('pad-toggle');
  function showPad(on) {
    pad.hidden = !on;
    toggle.setAttribute('aria-pressed', String(on));
    try { localStorage.setItem('aurore-pad', on ? '1' : '0'); } catch (e) { /* idem */ }
  }
  let padOn = matchMedia('(pointer: coarse)').matches;
  try { const kept = localStorage.getItem('aurore-pad'); if (kept !== null) padOn = kept === '1'; } catch (e) { /* idem */ }
  showPad(padOn);
  toggle.addEventListener('click', () => showPad(pad.hidden));

  function press(button, down) {
    if (ready) window.Module.ccall('aurore_web_button', null, ['number', 'number'], [button, down ? 1 : 0]);
  }
  for (const button of pad.querySelectorAll('[data-button]')) {
    const code = Number(button.dataset.button);
    const down = (e) => { e.preventDefault(); button.setPointerCapture && button.setPointerCapture(e.pointerId); button.classList.add('held'); press(code, true); };
    const up = () => { if (!button.classList.contains('held')) return; button.classList.remove('held'); press(code, false); };
    button.addEventListener('pointerdown', down);
    button.addEventListener('pointerup', up);
    button.addEventListener('pointercancel', up);
    button.addEventListener('lostpointercapture', up);
    button.addEventListener('contextmenu', (e) => e.preventDefault());
  }

  // Full screen: the screen and the pad only.
  const full = document.getElementById('fullscreen');
  if (!document.documentElement.requestFullscreen) full.hidden = true;
  full.addEventListener('click', () => document.documentElement.requestFullscreen().catch(() => {}));
  document.addEventListener('fullscreenchange', () => {
    document.body.classList.toggle('full', Boolean(document.fullscreenElement));
    canvas.focus();
  });
  document.body.addEventListener('dblclick', (e) => { if (document.fullscreenElement && e.target !== canvas) document.exitFullscreen(); });
})();
