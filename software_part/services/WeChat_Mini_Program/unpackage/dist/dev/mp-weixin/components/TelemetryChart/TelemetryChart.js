"use strict";
const common_vendor = require("../../common/vendor.js");
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "TelemetryChart",
  props: {
    records: {
      type: Array,
      default: () => {
        return [];
      }
    },
    field: {
      type: String,
      default: "value"
    },
    title: {
      type: String,
      default: ""
    },
    height: {
      type: Number,
      default: 120
    }
  },
  setup(__props) {
    const props = __props;
    function toNumber(v = null) {
      const n = Number(v);
      if (isNaN(n))
        return NaN;
      return n;
    }
    function pickValue(payloadStr) {
      if (!payloadStr)
        return NaN;
      try {
        const obj = UTS.JSON.parse(payloadStr);
        return toNumber(obj === null || obj === void 0 ? null : obj[props.field]);
      } catch (e) {
        return NaN;
      }
    }
    const points = common_vendor.computed(() => {
      const list = props.records || [];
      const values = [];
      list.forEach((r = null) => {
        const v = pickValue(String((r === null || r === void 0 ? null : r.payload) || ""));
        if (!isNaN(v))
          values.push(v);
      });
      if (values.length === 0)
        return [];
      const min = Math.min.apply(null, values);
      const max = Math.max.apply(null, values);
      const span = max - min || 1;
      const tail = values.slice(Math.max(0, values.length - 30));
      return tail.map((v, idx) => {
        const h = (v - min) / span * 100;
        return new UTSJSONObject({ key: String(idx), h });
      });
    });
    const metaText = common_vendor.computed(() => {
      var _a;
      const list = props.records || [];
      if (list.length === 0)
        return "";
      const v = pickValue(String(((_a = list[0]) === null || _a === void 0 ? null : _a.payload) || ""));
      if (isNaN(v))
        return "";
      return `${props.field}: ${v}`;
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: __props.title
      }, __props.title ? {
        b: common_vendor.t(__props.title)
      } : {}, {
        c: common_vendor.f(common_vendor.unref(points), (p, k0, i0) => {
          return {
            a: p.key,
            b: p.h + "%"
          };
        }),
        d: __props.height + "px",
        e: common_vendor.unref(metaText)
      }, common_vendor.unref(metaText) ? {
        f: common_vendor.t(common_vendor.unref(metaText))
      } : {}, {
        g: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/TelemetryChart/TelemetryChart.js.map
