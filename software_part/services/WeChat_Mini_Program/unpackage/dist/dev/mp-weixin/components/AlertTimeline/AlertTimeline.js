"use strict";
const common_vendor = require("../../common/vendor.js");
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "AlertTimeline",
  props: {
    title: { type: String, default: "告警时间线" },
    alerts: { type: Array, default: () => {
      return [];
    } }
  },
  emits: ["itemClick"],
  setup(__props, _a) {
    var __emit = _a.emit;
    const emit = __emit;
    function onItemClick(alert = null) {
      emit("itemClick", alert);
    }
    function levelClass(level) {
      const l = String(level || "").toLowerCase();
      if (l === "critical")
        return "critical";
      if (l === "warning")
        return "warning";
      if (l === "info")
        return "info";
      return "default";
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(__props.title),
        b: !__props.alerts || __props.alerts.length === 0
      }, !__props.alerts || __props.alerts.length === 0 ? {} : {
        c: common_vendor.f(__props.alerts, (a, k0, i0) => {
          return {
            a: common_vendor.n(levelClass(a.level)),
            b: common_vendor.t(a.message),
            c: common_vendor.t(a.deviceId),
            d: common_vendor.t(a.createdAt || "-"),
            e: a.id,
            f: common_vendor.o(($event) => {
              return onItemClick(a);
            }, a.id)
          };
        })
      }, {
        d: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/AlertTimeline/AlertTimeline.js.map
