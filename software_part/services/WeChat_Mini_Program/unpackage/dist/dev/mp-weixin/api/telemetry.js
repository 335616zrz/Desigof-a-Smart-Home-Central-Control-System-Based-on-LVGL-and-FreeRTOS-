"use strict";
const api_http = require("./http.js");
function fetchRealtime(deviceId, limit = 50) {
  return api_http.request({
    url: "/api/telemetry/latest",
    method: "GET",
    data: { deviceId, limit }
  });
}
function fetchHistory(params) {
  return api_http.request({
    url: "/api/telemetry",
    method: "GET",
    data: params || {}
  });
}
exports.fetchHistory = fetchHistory;
exports.fetchRealtime = fetchRealtime;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/telemetry.js.map
