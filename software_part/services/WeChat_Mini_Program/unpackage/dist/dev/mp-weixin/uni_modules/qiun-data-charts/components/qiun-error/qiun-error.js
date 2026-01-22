"use strict";
const common_vendor = require("../../../../common/vendor.js");
const _sfc_main = common_vendor.defineComponent({
  name: "qiun-error",
  props: {
    errorMessage: {
      type: String,
      default: null
    }
  },
  data() {
    return {};
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return {
    a: common_vendor.t($props.errorMessage == null ? "请点击重试" : $props.errorMessage),
    b: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
  };
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/qiun-data-charts/components/qiun-error/qiun-error.js.map
