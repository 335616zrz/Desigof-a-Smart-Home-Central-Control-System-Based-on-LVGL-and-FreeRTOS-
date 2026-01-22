"use strict";
const common_vendor = require("../../common/vendor.js");
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "AlertBadge",
  props: {
    level: {
      type: String,
      default: ""
    },
    status: {
      type: String,
      default: ""
    }
  },
  setup(__props) {
    const props = __props;
    const normLevel = common_vendor.computed(() => {
      return String(props.level || "").toLowerCase();
    });
    const normStatus = common_vendor.computed(() => {
      return String(props.status || "").toLowerCase();
    });
    const levelText = common_vendor.computed(() => {
      const l = normLevel.value;
      if (l === "critical")
        return "危险";
      if (l === "warning")
        return "警告";
      if (l === "info")
        return "提示";
      return l || "告警";
    });
    const statusText = common_vendor.computed(() => {
      const s = normStatus.value;
      if (!s)
        return "";
      if (s === "open")
        return "未处理";
      if (s === "ack")
        return "已确认";
      if (s === "closed")
        return "已关闭";
      return s;
    });
    const levelClass = common_vendor.computed(() => {
      const l = normLevel.value;
      if (l === "critical")
        return "critical";
      if (l === "warning")
        return "warning";
      if (l === "info")
        return "info";
      return "default";
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(common_vendor.unref(levelText)),
        b: common_vendor.unref(statusText)
      }, common_vendor.unref(statusText) ? {} : {}, {
        c: common_vendor.unref(statusText)
      }, common_vendor.unref(statusText) ? {
        d: common_vendor.t(common_vendor.unref(statusText))
      } : {}, {
        e: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
        f: common_vendor.n(common_vendor.unref(levelClass))
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/AlertBadge/AlertBadge.js.map
