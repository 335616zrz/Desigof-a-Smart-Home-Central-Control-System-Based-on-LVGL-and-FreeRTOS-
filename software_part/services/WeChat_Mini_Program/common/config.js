// Environment configuration for the mini program.
// Keep defaults safe (HTTPS) and easy to override later.

import { getString, remove, setString } from './storage.js'

export const ENV = {
  DEVELOP: 'develop',
  TRIAL: 'trial',
  RELEASE: 'release',
}

// Allow overriding API base URL at runtime (useful for real-device debugging on LAN).
// Example: wx.setStorageSync('W_BASE_URL', 'http://192.168.1.3:8080')
export const STORAGE_KEY_BASE_URL = 'W_BASE_URL'
// Optional: separate base URL for static resources (music preview, etc.).
// Useful when API is behind a gateway/port but static files are served by another host.
// Example: wx.setStorageSync('W_MUSIC_BASE_URL', 'http://192.168.1.3')
export const STORAGE_KEY_MUSIC_BASE_URL = 'W_MUSIC_BASE_URL'
// Default music host (served by Caddy in this repo’s Music_Server workflow).
const DEFAULT_MUSIC_HOST = 'http://music-server.local'

const BASE_URL_MAP = {
  // Default to local backend (see backend `server.port` in `IoT_cloud_platform/backend`).
  // Update these URLs when deploying to a real HTTPS domain (required by WeChat in production).
  [ENV.DEVELOP]: 'http://localhost:8080',
  [ENV.TRIAL]: 'http://localhost:8080',
  [ENV.RELEASE]: 'http://localhost:8080',
}

export const REQUEST_TIMEOUT_MS = 15_000

function normalizeBaseUrl(url) {
  if (!url) return ''
  let u = String(url).trim()
  if (!u) return ''
  // Allow user to input "192.168.1.3:8080" without scheme.
  if (!/^https?:\/\//i.test(u)) u = `http://${u}`
  return u.replace(/\/+$/, '')
}

function deriveNoPortOrigin(url) {
  const u = normalizeBaseUrl(url)
  if (!u) return ''
  const m = u.match(/^(https?:\/\/)([^\/]+)(\/.*)?$/i)
  if (!m) return ''
  const scheme = m[1]
  const hostPort = m[2]
  const host = hostPort.replace(/:\d+$/, '')
  if (!host) return ''
  return `${scheme}${host}`
}

function getWxPlatform() {
  // #ifdef MP-WEIXIN
  try {
    const info = wx?.getSystemInfoSync?.()
    return info?.platform || ''
  } catch (e) {
    return ''
  }
  // #endif
  return ''
}

export function getWxEnvVersion() {
  // #ifdef MP-WEIXIN
  try {
    const info = wx?.getAccountInfoSync?.()
    return info?.miniProgram?.envVersion || ENV.RELEASE
  } catch (e) {
    return ENV.RELEASE
  }
  // #endif

  return ENV.RELEASE
}

export function getBaseUrl() {
  // 1) Runtime override (real device / different networks).
  const override = normalizeBaseUrl(getString(STORAGE_KEY_BASE_URL, ''))
  if (override) return override

  // 2) Defaults.
  const env = getWxEnvVersion()
  const base = BASE_URL_MAP[env] || BASE_URL_MAP[ENV.RELEASE]

  // In WeChat devtools, "localhost" is fine; on real device, it points to the phone itself.
  // Keep default as-is (so devtools works out of the box), and guide the user to set an override
  // on the login page for true-device debugging.
  const platform = getWxPlatform()
  if (platform && platform !== 'devtools' && /:\/\/localhost\b/i.test(base)) {
    // eslint-disable-next-line no-console
    console.warn(
      `[config] Detected real device + localhost baseUrl ("${base}"). ` +
        `Please set API baseUrl via storage "${STORAGE_KEY_BASE_URL}" (e.g. "http://192.168.1.3:8080").`
    )
  }
  return base
}

export function setBaseUrl(url) {
  const normalized = normalizeBaseUrl(url)
  if (!normalized) {
    remove(STORAGE_KEY_BASE_URL)
    return ''
  }
  setString(STORAGE_KEY_BASE_URL, normalized)
  return normalized
}

export function getMusicBaseUrl() {
  const override = normalizeBaseUrl(getString(STORAGE_KEY_MUSIC_BASE_URL, ''))
  if (override) return override

  // Default: use the same origin as API.
  // If your music files are hosted elsewhere (Caddy / CDN / another port), set W_MUSIC_BASE_URL.
  const same = normalizeBaseUrl(getBaseUrl())
  if (same) return same

  return normalizeBaseUrl(DEFAULT_MUSIC_HOST)
}

export function setMusicBaseUrl(url) {
  const normalized = normalizeBaseUrl(url)
  if (!normalized) {
    remove(STORAGE_KEY_MUSIC_BASE_URL)
    return ''
  }
  setString(STORAGE_KEY_MUSIC_BASE_URL, normalized)
  return normalized
}
