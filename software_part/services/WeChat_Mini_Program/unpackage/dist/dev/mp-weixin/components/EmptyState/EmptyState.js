"use strict";
const common_vendor = require("../../common/vendor.js");
if (!Array) {
  const _easycom_u_empty_1 = common_vendor.resolveComponent("u-empty");
  _easycom_u_empty_1();
}
const _easycom_u_empty = () => "../../uni_modules/uview-plus/components/u-empty/u-empty.js";
if (!Math) {
  _easycom_u_empty();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "EmptyState",
  props: {
    text: {
      type: String,
      default: "暂无数据"
    }
  },
  setup(__props) {
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = {
        a: common_vendor.gei(_ctx, ""),
        b: common_vendor.p({
          text: __props.text,
          mode: "data",
          id: common_vendor.gei(_ctx, "")
        })
      };
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/EmptyState/EmptyState.js.map
