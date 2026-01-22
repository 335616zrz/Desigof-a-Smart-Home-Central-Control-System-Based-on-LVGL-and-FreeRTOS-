import request from './http.js'

export function listUsers(page = 1, pageSize = 20) {
  return request({
    url: '/api/users',
    method: 'GET',
    data: { page, pageSize },
  })
}

export function createUser(payload) {
  return request({
    url: '/api/users',
    method: 'POST',
    data: payload || {},
  })
}

export function deleteUser(id) {
  return request({
    url: `/api/users/${id}`,
    method: 'DELETE',
  })
}

export function upgradeUser(id) {
  return request({
    url: `/api/users/${id}/upgrade`,
    method: 'POST',
    data: {},
  })
}

export function downgradeUser(id) {
  return request({
    url: `/api/users/${id}/downgrade`,
    method: 'POST',
    data: {},
  })
}

