import request from './http.js'

export function listAlerts(params) {
  return request({
    url: '/api/alerts',
    method: 'GET',
    data: params || {},
  })
}

export function getAlert(id) {
  return request({
    url: `/api/alerts/${id}`,
    method: 'GET',
  })
}

export function acknowledgeAlert(id) {
  return request({
    url: `/api/alerts/${id}/ack`,
    method: 'POST',
    data: {},
  })
}

export function closeAlert(id) {
  return request({
    url: `/api/alerts/${id}/close`,
    method: 'POST',
    data: {},
  })
}

