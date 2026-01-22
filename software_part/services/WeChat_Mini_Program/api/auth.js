import request from './http.js'

// Backend base path: /api/auth
export function login(username, password) {
  return request({
    url: '/api/auth/login',
    method: 'POST',
    data: { username, password },
  })
}

export function register(username, password, email) {
  return request({
    url: '/api/auth/register',
    method: 'POST',
    data: { username, password, email },
  })
}

export function refresh(refreshToken) {
  return request({
    url: '/api/auth/refresh',
    method: 'POST',
    data: { refreshToken },
  })
}

export function fetchProfile() {
  return request({
    url: '/api/auth/profile',
    method: 'POST',
    data: {},
  })
}

// Server has no logout endpoint; logout is a client-side state clear.
export function logout() {
  return Promise.resolve({ code: 0, message: 'success', data: null })
}

