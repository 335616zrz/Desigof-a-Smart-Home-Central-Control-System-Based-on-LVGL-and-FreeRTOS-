import request from './http.js'

export function fetchRealtime(deviceId, limit = 50) {
  return request({
    url: '/api/telemetry/latest',
    method: 'GET',
    data: { deviceId, limit },
  })
}

export function fetchHistory(params) {
  return request({
    url: '/api/telemetry',
    method: 'GET',
    data: params || {},
  })
}

