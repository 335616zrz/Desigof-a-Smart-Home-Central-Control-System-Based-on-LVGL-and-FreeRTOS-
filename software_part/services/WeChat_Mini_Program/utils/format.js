function pad2(n) {
  return String(n).padStart(2, '0')
}

function toDate(input) {
  if (input instanceof Date) return input
  if (typeof input === 'number') return new Date(input)
  if (typeof input === 'string') return new Date(input.replace(/-/g, '/'))
  return new Date()
}

// Basic date formatter.
// Supported tokens: YYYY, MM, DD, HH, mm, ss
export function formatDate(input, pattern = 'YYYY-MM-DD') {
  const d = toDate(input)
  const map = {
    YYYY: String(d.getFullYear()),
    MM: pad2(d.getMonth() + 1),
    DD: pad2(d.getDate()),
    HH: pad2(d.getHours()),
    mm: pad2(d.getMinutes()),
    ss: pad2(d.getSeconds()),
  }
  return pattern.replace(/YYYY|MM|DD|HH|mm|ss/g, (k) => map[k])
}

export function formatDateTime(input) {
  return formatDate(input, 'YYYY-MM-DD HH:mm:ss')
}

// Backend expects LocalDateTime params in local timezone (do NOT use Date.toISOString()).
// Format: YYYY-MM-DDTHH:mm:ss
export function toLocalIsoDateTime(input) {
  return formatDate(input, 'YYYY-MM-DDTHH:mm:ss')
}
