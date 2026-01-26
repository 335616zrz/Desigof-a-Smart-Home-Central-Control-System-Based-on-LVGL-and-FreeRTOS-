"use strict";const e=require("./pinia.js"),s=e.defineStore("app",{state:()=>({networkType:"unknown",mqttStatus:"unknown",season:""}),actions:{setNetworkType(t){this.networkType=t||"unknown"},setMqttStatus(t){this.mqttStatus=t||"unknown"},setSeason(t){this.season=t||""}}});exports.useAppStore=s;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/app.js.map
