"use strict";
const common_vendor = require("../../common/vendor.js");
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "TelemetryTable",
  props: {
    title: { type: String, default: "遥测数据" },
    rows: { type: Array, default: () => {
      return [];
    } }
    // [{deviceId, temperature, humidity, pressure, reportedAt, timestamp}]
  },
  setup(__props) {
    function format(v = null) {
      if (v === null || v === void 0)
        return "-";
      const n = Number(v);
      if (isNaN(n))
        return "-";
      return String(n);
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(__props.title),
        b: common_vendor.f(__props.rows, (r, k0, i0) => {
          return {
            a: common_vendor.t(r.deviceId),
            b: common_vendor.t(format(r.temperature)),
            c: common_vendor.t(format(r.humidity)),
            d: common_vendor.t(format(r.pressure)),
            e: r.deviceId
          };
        }),
        c: !__props.rows || __props.rows.length === 0
      }, !__props.rows || __props.rows.length === 0 ? {} : {}, {
        d: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/TelemetryTable/TelemetryTable.js.map
