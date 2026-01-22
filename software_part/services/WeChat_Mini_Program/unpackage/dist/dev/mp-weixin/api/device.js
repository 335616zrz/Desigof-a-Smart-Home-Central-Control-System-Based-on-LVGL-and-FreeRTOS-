"use strict";
const api_http = require("./http.js");
function listDevices(params) {
  return api_http.request({
    url: "/api/devices",
    method: "GET",
    data: params || {}
  });
}
function getDevice(deviceId) {
  return api_http.request({
    url: `/api/devices/${deviceId}`,
    method: "GET"
  });
}
function createDevice(payload) {
  return api_http.request({
    url: "/api/devices",
    method: "POST",
    data: payload || {}
  });
}
function updateDevice(deviceId, payload) {
  return api_http.request({
    url: `/api/devices/${deviceId}`,
    method: "PUT",
    data: payload || {}
  });
}
function deleteDevice(deviceId) {
  return api_http.request({
    url: `/api/devices/${deviceId}`,
    method: "DELETE"
  });
}
function provisionDevice(deviceId = "auto", payload) {
  return api_http.request({
    url: `/api/devices/${deviceId}/provision`,
    method: "POST",
    data: payload || {}
  });
}
exports.createDevice = createDevice;
exports.deleteDevice = deleteDevice;
exports.getDevice = getDevice;
exports.listDevices = listDevices;
exports.provisionDevice = provisionDevice;
exports.updateDevice = updateDevice;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/device.js.map
