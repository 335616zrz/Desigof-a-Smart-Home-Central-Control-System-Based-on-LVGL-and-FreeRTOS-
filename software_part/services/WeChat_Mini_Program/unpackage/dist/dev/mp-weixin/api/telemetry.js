"use strict";const e=require("./http.js");function i(t,r=50){return e.request({url:"/api/telemetry/latest",method:"GET",data:{deviceId:t,limit:r}})}function a(t){return e.request({url:"/api/telemetry",method:"GET",data:t||{}})}exports.fetchHistory=a;exports.fetchRealtime=i;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/telemetry.js.map
