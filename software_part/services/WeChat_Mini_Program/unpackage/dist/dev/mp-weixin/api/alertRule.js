"use strict";
const api_http = require("./http.js");
function listAlertRules(enabled) {
  return api_http.request({
    url: "/api/alert-rules",
    method: "GET",
    data: enabled === void 0 ? {} : { enabled }
  });
}
function listUserRules(deviceId) {
  return api_http.request({
    url: "/api/alert-rules/user-rules",
    method: "GET",
    data: deviceId ? { deviceId } : {}
  });
}
function getAlertRule(id) {
  return api_http.request({
    url: `/api/alert-rules/${id}`,
    method: "GET"
  });
}
function createAlertRule(payload) {
  return api_http.request({
    url: "/api/alert-rules",
    method: "POST",
    data: payload || {}
  });
}
function updateAlertRule(id, payload) {
  return api_http.request({
    url: `/api/alert-rules/${id}`,
    method: "PUT",
    data: payload || {}
  });
}
function toggleAlertRule(id, enabled) {
  return api_http.request({
    url: `/api/alert-rules/${id}/enabled?enabled=${enabled ? "true" : "false"}`,
    method: "PATCH",
    data: {}
  });
}
function deleteAlertRule(id) {
  return api_http.request({
    url: `/api/alert-rules/${id}`,
    method: "DELETE"
  });
}
function getCurrentSeason() {
  return api_http.request({
    url: "/api/alert-rules/current-season",
    method: "GET"
  });
}
function listSystemTemplates(season) {
  return api_http.request({
    url: "/api/alert-rules/templates",
    method: "GET",
    data: season ? { season } : {}
  });
}
exports.createAlertRule = createAlertRule;
exports.deleteAlertRule = deleteAlertRule;
exports.getAlertRule = getAlertRule;
exports.getCurrentSeason = getCurrentSeason;
exports.listAlertRules = listAlertRules;
exports.listSystemTemplates = listSystemTemplates;
exports.listUserRules = listUserRules;
exports.toggleAlertRule = toggleAlertRule;
exports.updateAlertRule = updateAlertRule;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/alertRule.js.map
