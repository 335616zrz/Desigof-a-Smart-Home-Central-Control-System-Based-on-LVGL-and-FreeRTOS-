import { login, register, fetchProfile } from '../api/auth.js'
import { ROUTES, STORAGE_KEYS } from '../common/constants.js'
import { getJSON, getString, remove, setJSON, setString } from '../common/storage.js'
import { defineStore } from './pinia.js'

function normalizeAuthPayload(resp) {
  // Our request wrapper resolves either raw data or an {code,message,data} envelope.
  return resp?.data ?? resp ?? null
}

export const useAuthStore = defineStore('auth', {
  state: () => ({
    user: null,
    token: '',
    refreshToken: '',
    isAuthenticated: false,
  }),
  actions: {
    initFromStorage() {
      const token = getString(STORAGE_KEYS.TOKEN, '')
      const refreshToken = getString(STORAGE_KEYS.REFRESH_TOKEN, '')
      const user = getJSON(STORAGE_KEYS.USER, null)

      this.token = token
      this.refreshToken = refreshToken
      this.user = user
      this.isAuthenticated = Boolean(token)
    },

    async login(username, password) {
      // Avoid logging auth payloads/tokens in devtools (can leak credentials and bloat memory).
      const DEBUG_AUTH = false
      const resp = await login(username, password)
      const payload = normalizeAuthPayload(resp)
      if (DEBUG_AUTH) console.log('[auth.js] login response payload:', JSON.stringify(payload))
      if (!payload) return null

      this.token = payload.accessToken || ''
      this.refreshToken = payload.refreshToken || ''
      this.user = {
        username: payload.username,
        role: payload.role,
      }
      this.isAuthenticated = Boolean(this.token)

      if (DEBUG_AUTH) console.log('[auth.js] saving token:', this.token ? `${this.token.substring(0, 20)}...` : '(empty)')
      setString(STORAGE_KEYS.TOKEN, this.token)
      setString(STORAGE_KEYS.REFRESH_TOKEN, this.refreshToken)
      setJSON(STORAGE_KEYS.USER, this.user)
      setString(STORAGE_KEYS.LAST_LOGIN_AT, String(Date.now()))

      return payload
    },

    async register(username, password, email) {
      const resp = await register(username, password, email)
      const payload = normalizeAuthPayload(resp)
      if (!payload) return null

      this.token = payload.accessToken || ''
      this.refreshToken = payload.refreshToken || ''
      this.user = {
        username: payload.username,
        role: payload.role,
      }
      this.isAuthenticated = Boolean(this.token)

      setString(STORAGE_KEYS.TOKEN, this.token)
      setString(STORAGE_KEYS.REFRESH_TOKEN, this.refreshToken)
      setJSON(STORAGE_KEYS.USER, this.user)
      setString(STORAGE_KEYS.LAST_LOGIN_AT, String(Date.now()))

      return payload
    },

    async fetchProfile() {
      const resp = await fetchProfile()
      const profile = normalizeAuthPayload(resp)
      if (!profile) return null

      this.user = profile
      setJSON(STORAGE_KEYS.USER, this.user)
      return profile
    },

    logout() {
      this.user = null
      this.token = ''
      this.refreshToken = ''
      this.isAuthenticated = false

      remove(STORAGE_KEYS.TOKEN)
      remove(STORAGE_KEYS.REFRESH_TOKEN)
      remove(STORAGE_KEYS.USER)

      try {
        uni.reLaunch({ url: ROUTES.LOGIN })
      } catch (e) {
        // ignore
      }
    },
  },
})
