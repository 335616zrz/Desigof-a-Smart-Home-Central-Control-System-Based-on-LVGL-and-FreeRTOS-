"use strict";
const api_http = require("./http.js");
function login(username, password) {
  return api_http.request({
    url: "/api/auth/login",
    method: "POST",
    data: { username, password }
  });
}
function register(username, password, email) {
  return api_http.request({
    url: "/api/auth/register",
    method: "POST",
    data: { username, password, email }
  });
}
function fetchProfile() {
  return api_http.request({
    url: "/api/auth/profile",
    method: "POST",
    data: {}
  });
}
exports.fetchProfile = fetchProfile;
exports.login = login;
exports.register = register;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/auth.js.map
