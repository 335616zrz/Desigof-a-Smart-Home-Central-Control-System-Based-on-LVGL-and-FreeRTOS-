import request from './http.js'

export function listAlertRules(enabled) {
  return request({
    url: '/api/alert-rules',
    method: 'GET',
    data: enabled === undefined ? {} : { enabled },
  })
}

export function listUserRules(deviceId) {
  return request({
    url: '/api/alert-rules/user-rules',
    method: 'GET',
    data: deviceId ? { deviceId } : {},
  })
}

export function getAlertRule(id) {
  return request({
    url: `/api/alert-rules/${id}`,
    method: 'GET',
  })
}

export function createAlertRule(payload) {
  return request({
    url: '/api/alert-rules',
    method: 'POST',
    data: payload || {},
  })
}

export function updateAlertRule(id, payload) {
  return request({
    url: `/api/alert-rules/${id}`,
    method: 'PUT',
    data: payload || {},
  })
}

export function toggleAlertRule(id, enabled) {
  return request({
    url: `/api/alert-rules/${id}/enabled?enabled=${enabled ? 'true' : 'false'}`,
    method: 'PATCH',
    data: {},
  })
}

export function deleteAlertRule(id) {
  return request({
    url: `/api/alert-rules/${id}`,
    method: 'DELETE',
  })
}

export function getCurrentSeason() {
  return request({
    url: '/api/alert-rules/current-season',
    method: 'GET',
  })
}

export function listSystemTemplates(season) {
  return request({
    url: '/api/alert-rules/templates',
    method: 'GET',
    data: season ? { season } : {},
  })
}
