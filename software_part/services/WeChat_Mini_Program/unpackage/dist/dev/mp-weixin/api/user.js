"use strict";
const api_http = require("./http.js");
function listUsers(page = 1, pageSize = 20) {
  return api_http.request({
    url: "/api/users",
    method: "GET",
    data: { page, pageSize }
  });
}
function createUser(payload) {
  return api_http.request({
    url: "/api/users",
    method: "POST",
    data: payload || {}
  });
}
function deleteUser(id) {
  return api_http.request({
    url: `/api/users/${id}`,
    method: "DELETE"
  });
}
exports.createUser = createUser;
exports.deleteUser = deleteUser;
exports.listUsers = listUsers;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/user.js.map
