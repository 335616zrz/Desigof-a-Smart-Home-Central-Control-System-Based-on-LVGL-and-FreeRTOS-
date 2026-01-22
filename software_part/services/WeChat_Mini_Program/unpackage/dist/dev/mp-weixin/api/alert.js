"use strict";
const api_http = require("./http.js");
function listAlerts(params) {
  return api_http.request({
    url: "/api/alerts",
    method: "GET",
    data: params || {}
  });
}
function getAlert(id) {
  return api_http.request({
    url: `/api/alerts/${id}`,
    method: "GET"
  });
}
function acknowledgeAlert(id) {
  return api_http.request({
    url: `/api/alerts/${id}/ack`,
    method: "POST",
    data: {}
  });
}
function closeAlert(id) {
  return api_http.request({
    url: `/api/alerts/${id}/close`,
    method: "POST",
    data: {}
  });
}
exports.acknowledgeAlert = acknowledgeAlert;
exports.closeAlert = closeAlert;
exports.getAlert = getAlert;
exports.listAlerts = listAlerts;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/alert.js.map
