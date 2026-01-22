"use strict";
const common_vendor = require("../../common/vendor.js");
if (!Array) {
  const _easycom_u_circle_progress_1 = common_vendor.resolveComponent("u-circle-progress");
  _easycom_u_circle_progress_1();
}
const _easycom_u_circle_progress = () => "../../uni_modules/uview-plus/components/u-circle-progress/u-circle-progress.js";
if (!Math) {
  _easycom_u_circle_progress();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "OnlineRatePanel",
  props: {
    online: { type: Number, default: 0 },
    total: { type: Number, default: 0 }
  },
  setup(__props) {
    const props = __props;
    const rate = common_vendor.computed(() => {
      const t = props.total || 0;
      if (!t)
        return 0;
      return Math.round(props.online / t * 100);
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = {
        a: common_vendor.p({
          percentage: common_vendor.unref(rate)
        }),
        b: common_vendor.t(common_vendor.unref(rate)),
        c: common_vendor.t(__props.online),
        d: common_vendor.t(__props.total),
        e: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      };
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/OnlineRatePanel/OnlineRatePanel.js.map
