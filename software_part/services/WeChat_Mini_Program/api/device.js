import request from './http.js'

export function listDevices(params) {
  return request({
    url: '/api/devices',
    method: 'GET',
    data: params || {},
  })
}

export function getDevice(deviceId) {
  return request({
    url: `/api/devices/${deviceId}`,
    method: 'GET',
  })
}

export function createDevice(payload) {
  return request({
    url: '/api/devices',
    method: 'POST',
    data: payload || {},
  })
}

export function updateDevice(deviceId, payload) {
  return request({
    url: `/api/devices/${deviceId}`,
    method: 'PUT',
    data: payload || {},
  })
}

export function deleteDevice(deviceId) {
  return request({
    url: `/api/devices/${deviceId}`,
    method: 'DELETE',
  })
}

// Issue device credential for onboarding.
// Backend supports `deviceId = auto` to generate a UUID.
export function provisionDevice(deviceId = 'auto', payload) {
  return request({
    url: `/api/devices/${deviceId}/provision`,
    method: 'POST',
    data: payload || {},
  })
}

// WS2812 night light (white) on/off via MQTT command published by backend.
export function setNightLight(deviceId, on) {
  return request({
    url: `/api/devices/${deviceId}/commands/night-light`,
    method: 'POST',
    data: { on: !!on },
  })
}

export function getDeviceShadow(deviceId) {
  return request({
    url: `/api/devices/${deviceId}/shadow`,
    method: 'GET',
  })
}
