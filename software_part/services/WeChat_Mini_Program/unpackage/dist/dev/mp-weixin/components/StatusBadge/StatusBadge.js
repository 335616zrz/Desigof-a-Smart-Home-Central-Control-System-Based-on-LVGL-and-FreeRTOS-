"use strict";
const common_vendor = require("../../common/vendor.js");
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "StatusBadge",
  props: {
    status: {
      type: String,
      default: ""
    }
  },
  setup(__props) {
    const props = __props;
    const normalized = common_vendor.computed(() => {
      return String(props.status || "").toUpperCase();
    });
    const text = common_vendor.computed(() => {
      const s = normalized.value;
      if (s === "ONLINE" || s === "AUTHENTICATED")
        return "在线";
      if (s === "OFFLINE")
        return "离线";
      if (s === "PROVISIONING")
        return "待配网";
      return "未知";
    });
    const colorClass = common_vendor.computed(() => {
      const s = normalized.value;
      if (s === "ONLINE" || s === "AUTHENTICATED")
        return "ok";
      if (s === "OFFLINE")
        return "off";
      if (s === "PROVISIONING")
        return "warn";
      return "unknown";
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = {
        a: common_vendor.t(common_vendor.unref(text)),
        b: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
        c: common_vendor.n(common_vendor.unref(colorClass))
      };
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/StatusBadge/StatusBadge.js.map
