"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uTransition_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const uni_modules_uviewPlus_components_uTransition_transitionMixin = require("./transitionMixin.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-transition",
  data() {
    return {
      inited: false,
      viewStyle: new UTSJSONObject({}),
      status: "",
      transitionEnded: false,
      display: false,
      classes: ""
      // 应用的类名
    };
  },
  emits: ["click", "beforeEnter", "enter", "afterEnter", "beforeLeave", "leave", "afterLeave"],
  computed: {
    mergeStyle() {
      const _a = this, viewStyle = _a.viewStyle, customStyle = _a.customStyle;
      return new UTSJSONObject(Object.assign(Object.assign({
        transitionDuration: `${this.duration}ms`,
        // display: `${this.display ? '' : 'none'}`,
        transitionTimingFunction: this.timingFunction
      }, uni_modules_uviewPlus_libs_function_index.addStyle(customStyle)), viewStyle));
    }
  },
  // 将mixin挂在到组件中，实际上为一个vue格式对象。
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uTransition_transitionMixin.transitionMixin, uni_modules_uviewPlus_components_uTransition_props.props],
  watch: {
    show: {
      handler(newVal = null) {
        newVal ? this.vueEnter() : this.vueLeave();
      },
      // 表示同时监听初始化时的props的show的意思
      immediate: true
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: $data.inited
  }, $data.inited ? {
    b: common_vendor.sei(common_vendor.gei(_ctx, "", "r0-5cec8177"), "view", "u-transition"),
    c: common_vendor.o((...args) => _ctx.clickHandler && _ctx.clickHandler(...args)),
    d: common_vendor.n($data.classes),
    e: common_vendor.s($options.mergeStyle),
    f: common_vendor.o((...args) => _ctx.noop && _ctx.noop(...args))
  } : {});
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-5cec8177"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-transition/u-transition.js.map
