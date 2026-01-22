import request from './http.js'

export function listTracks(keyword) {
  return request({
    url: '/api/music',
    method: 'GET',
    data: keyword ? { keyword } : {},
  })
}

export function getTrack(id) {
  return request({
    url: `/api/music/${id}`,
    method: 'GET',
  })
}

export function createTrack(payload) {
  return request({
    url: '/api/music',
    method: 'POST',
    data: payload || {},
  })
}

export function updateTrack(id, payload) {
  return request({
    url: `/api/music/${id}`,
    method: 'PUT',
    data: payload || {},
  })
}

export function deleteTrack(id) {
  return request({
    url: `/api/music/${id}`,
    method: 'DELETE',
  })
}

