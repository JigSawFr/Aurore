// The demo's service worker (web/README.md): the headers GitHub Pages cannot
// send and threads need (Cross-Origin-Opener-Policy and -Embedder-Policy),
// added to every response of the page's folder, as coi-serviceworker does
// (MIT, Guido Zuidhof: https://github.com/gzuidhof/coi-serviceworker).
'use strict';

self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', (event) => event.waitUntil(self.clients.claim()));

self.addEventListener('fetch', (event) => {
  const request = event.request;
  // A request Chrome refuses to answer from a worker.
  if (request.cache === 'only-if-cached' && request.mode !== 'same-origin') return;
  event.respondWith(fetch(request).then((response) => {
    if (response.status === 0) return response;
    const headers = new Headers(response.headers);
    headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
    headers.set('Cross-Origin-Resource-Policy', 'cross-origin');
    headers.set('Cross-Origin-Opener-Policy', 'same-origin');
    return new Response(response.body, { status: response.status, statusText: response.statusText, headers });
  }));
});
