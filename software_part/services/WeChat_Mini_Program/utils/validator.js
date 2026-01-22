export function isEmail(value) {
  if (!value) return false
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(String(value))
}

export function isPhoneCN(value) {
  if (!value) return false
  return /^1[3-9]\d{9}$/.test(String(value))
}

export function isStrongPassword(value, minLen = 6) {
  if (!value) return false
  const v = String(value)
  if (v.length < minLen) return false
  // Require at least 2 groups among: letters, numbers, symbols
  const groups = [
    /[a-zA-Z]/.test(v),
    /\d/.test(v),
    /[^a-zA-Z0-9]/.test(v),
  ].filter(Boolean).length
  return groups >= 2
}

export function required(value) {
  if (value === 0) return true
  return value !== null && value !== undefined && String(value).trim() !== ''
}

export function minLength(value, len) {
  return String(value ?? '').length >= len
}

