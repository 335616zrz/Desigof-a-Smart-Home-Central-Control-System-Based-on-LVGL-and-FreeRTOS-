import { getBaseUrl, REQUEST_TIMEOUT_MS } from '../common/config.js'
import { HTTP, ROUTES, STORAGE_KEYS } from '../common/constants.js'
import { getString, remove } from '../common/storage.js'

let _redirectingToLogin = false

function buildUrl(path) {
  if (!path) return getBaseUrl()
  if (/^https?:\/\//i.test(path)) return path
  const base = getBaseUrl().replace(/\/$/, '')
  const p = String(path).startsWith('/') ? path : `/${path}`
  return `${base}${p}`
}

function redirectToLogin() {
  if (_redirectingToLogin) return
  _redirectingToLogin = true
  try {
    uni.reLaunch({ url: ROUTES.LOGIN })
  } catch (e) {
    // Ignore navigation failures (e.g., during app init).
  }
  // Avoid spamming navigation when multiple requests fail together.
  setTimeout(() => {
    _redirectingToLogin = false
  }, 1200)
}

function stripUndefined(input) {
  if (!input || typeof input !== 'object') return input
  if (Array.isArray(input)) return input
  const out = {}
  Object.keys(input).forEach((k) => {
    const v = input[k]
    if (v === undefined) return
    out[k] = v
  })
  return out
}

export function request(options) {
  const token = getString(STORAGE_KEYS.TOKEN, '')
  const method = (options.method || 'GET').toUpperCase()
  const data = stripUndefined(options.data || {})

  // Avoid spamming console logs on mini-program (it may cause devtools memory bloat, especially with 1s polling).
  const DEBUG_HTTP = false
  if (DEBUG_HTTP) {
    console.log('[http.js] request:', options.url, 'hasToken:', Boolean(token))
  }

  // 默认 Content-Type 为 JSON（POST/PUT/PATCH 请求需要）
  const defaultContentType = ['POST', 'PUT', 'PATCH'].includes(method)
    ? 'application/json'
    : undefined

  return new Promise((resolve, reject) => {
    uni.request({
      url: buildUrl(options.url),
      method: method,
      // NOTE: uni.request may stringify undefined into "undefined" for query params,
      // which breaks backend filters (e.g. /api/alerts?status=undefined).
      data,
      timeout: options.timeout ?? REQUEST_TIMEOUT_MS,
      header: {
        ...(defaultContentType ? { 'Content-Type': defaultContentType } : {}),
        ...(options.header || {}),
        ...(token ? { [HTTP.AUTH_HEADER]: `${HTTP.TOKEN_PREFIX}${token}` } : {}),
      },
      success: (res) => {
        const { statusCode, data } = res || {}

        // 401: authentication invalid/expired -> clear auth and go login.
        if (statusCode === 401) {
          remove(STORAGE_KEYS.TOKEN)
          remove(STORAGE_KEYS.USER)
          redirectToLogin()
          reject(res)
          return
        }

        // 403: authenticated but forbidden -> keep auth, let caller handle.
        if (statusCode === 403) {
          uni.showToast({ title: '无权限', icon: 'none' })
          reject(res)
          return
        }

        // If backend wraps responses, prefer its "code/msg" contract.
        if (data && typeof data === 'object' && 'code' in data) {
          const code = data.code
          if (code !== 200 && code !== 0) {
            uni.showToast({ title: data.msg || data.message || '请求失败', icon: 'none' })
            reject(data)
            return
          }
          resolve(data)
          return
        }

        if (statusCode >= 200 && statusCode < 300) {
          resolve(data)
          return
        }

        uni.showToast({ title: '网络请求失败', icon: 'none' })
        reject(res)
      },
      fail: (err) => {
        const raw = String(err?.errMsg || '')
        // Common real-device issues:
        // - ERR_NAME_NOT_RESOLVED: user entered an mDNS/hostname that the phone can't resolve (e.g. servers.local)
        //   or used a full-width colon "：" causing an invalid host.
        // - url not in domain list: WeChat request domain whitelist / urlCheck not disabled.
        if (/ERR_NAME_NOT_RESOLVED/i.test(raw)) {
          uni.showToast({ title: '域名解析失败：请在登录页把服务器改为电脑局域网IP(如 192.168.x.x:8080)', icon: 'none' })
        } else if (/url not in domain list/i.test(raw)) {
          uni.showToast({ title: '域名未加入小程序请求合法域名(真机需配置或关闭校验)', icon: 'none' })
        } else {
          uni.showToast({ title: raw || '网络异常', icon: 'none' })
        }
        reject(err)
      },
    })
  })
}

export default request
