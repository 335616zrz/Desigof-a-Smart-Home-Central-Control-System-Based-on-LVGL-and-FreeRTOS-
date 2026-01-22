"use strict";
const common_vendor = require("../../common/vendor.js");
const common_theme = require("../../common/theme.js");
if (!Array) {
  const _easycom_u_icon_1 = common_vendor.resolveComponent("u-icon");
  const _easycom_u_count_to_1 = common_vendor.resolveComponent("u-count-to");
  (_easycom_u_icon_1 + _easycom_u_count_to_1)();
}
const _easycom_u_icon = () => "../../uni_modules/uview-plus/components/u-icon/u-icon2.js";
const _easycom_u_count_to = () => "../../uni_modules/uview-plus/components/u-count-to/u-count-to.js";
if (!Math) {
  (_easycom_u_icon + _easycom_u_count_to)();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "StatCard",
  props: {
    icon: { type: String, required: true },
    label: { type: String, required: true },
    value: { type: Number, default: 0 },
    description: { type: String, default: "" },
    tone: { type: String, default: "info" }
    // info, success, warning, danger
  },
  setup(__props) {
    const props = __props;
    const iconColors = new UTSJSONObject({
      info: common_theme.COLORS.primary,
      success: common_theme.COLORS.success,
      warning: common_theme.COLORS.warning,
      danger: common_theme.COLORS.danger
    });
    const iconColor = common_vendor.computed(() => {
      const k = props.tone;
      return iconColors[k] || iconColors.info;
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.p({
          name: __props.icon,
          color: common_vendor.unref(iconColor),
          size: "22"
        }),
        b: common_vendor.t(__props.label),
        c: common_vendor.p({
          startVal: 0,
          endVal: __props.value,
          duration: 800,
          bold: true
        }),
        d: __props.description
      }, __props.description ? {
        e: common_vendor.t(__props.description)
      } : {}, {
        f: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
        g: common_vendor.n(__props.tone)
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/StatCard/StatCard.js.map
