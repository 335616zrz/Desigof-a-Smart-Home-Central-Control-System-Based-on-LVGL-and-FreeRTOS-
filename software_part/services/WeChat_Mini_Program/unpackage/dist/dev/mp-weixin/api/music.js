"use strict";
const api_http = require("./http.js");
function listTracks(keyword) {
  return api_http.request({
    url: "/api/music",
    method: "GET",
    data: keyword ? { keyword } : {}
  });
}
function createTrack(payload) {
  return api_http.request({
    url: "/api/music",
    method: "POST",
    data: payload || {}
  });
}
function deleteTrack(id) {
  return api_http.request({
    url: `/api/music/${id}`,
    method: "DELETE"
  });
}
exports.createTrack = createTrack;
exports.deleteTrack = deleteTrack;
exports.listTracks = listTracks;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/music.js.map
