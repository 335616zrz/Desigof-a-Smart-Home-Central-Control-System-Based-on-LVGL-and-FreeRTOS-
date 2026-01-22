// Thin wrapper around uni storage so call-sites are consistent.

function safeStringify(value) {
  try {
    return JSON.stringify(value)
  } catch (e) {
    return ''
  }
}

function safeParse(raw, fallback) {
  if (!raw) return fallback
  try {
    return JSON.parse(raw)
  } catch (e) {
    return fallback
  }
}

export function setString(key, value) {
  uni.setStorageSync(key, value ?? '')
}

export function getString(key, fallback = '') {
  const v = uni.getStorageSync(key)
  if (v === null || v === undefined || v === '') return fallback
  return String(v)
}

export function setJSON(key, value) {
  uni.setStorageSync(key, safeStringify(value))
}

export function getJSON(key, fallback = null) {
  const raw = uni.getStorageSync(key)
  if (typeof raw === 'string') return safeParse(raw, fallback)
  // Some runtimes may store objects directly.
  if (raw === null || raw === undefined || raw === '') return fallback
  return raw
}

export function remove(key) {
  uni.removeStorageSync(key)
}

export function clear() {
  uni.clearStorageSync()
}

