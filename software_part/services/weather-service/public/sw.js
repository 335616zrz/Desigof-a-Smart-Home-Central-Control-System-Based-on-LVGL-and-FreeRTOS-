/* Simple PWA service worker: cache-first for static assets, network-first for API */
const VERSION = 'v1.0.2';
const STATIC_CACHE = `static-${VERSION}`;
const STATIC_ASSETS = [
  '/',
  '/index.html',
  '/css/styles.css',
  '/js/app.js',
  '/assets/icons.svg',
  '/assets/favicon.svg',
  '/assets/icon-192.png',
  '/assets/icon-512.png'
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(STATIC_CACHE).then((cache) => cache.addAll(STATIC_ASSETS))
  );
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) => Promise.all(keys.map((k) => {
      if (!k.startsWith('static-') || k === STATIC_CACHE) return Promise.resolve();
      return caches.delete(k);
    })))
  );
  self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url);
  // API: network-first
  if (url.pathname.startsWith('/api/')) {
    event.respondWith(
      fetch(event.request).then((res) => res).catch(() => caches.match(event.request))
    );
    return;
  }
  // Static: cache-first
  event.respondWith(
    caches.match(event.request).then((cached) => cached || fetch(event.request))
  );
});
