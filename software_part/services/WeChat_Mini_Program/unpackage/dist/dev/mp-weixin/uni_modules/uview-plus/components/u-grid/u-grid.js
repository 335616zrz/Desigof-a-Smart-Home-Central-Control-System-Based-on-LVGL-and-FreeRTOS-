"use strict";
const uni_modules_uviewPlus_components_uGrid_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const common_vendor = require("../../../../common/vendor.js");
const __default__ = common_vendor.defineComponent({
  name: "u-grid",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uGrid_props.props],
  data() {
    return {
      index: 0,
      width: 0
    };
  },
  watch: {
    // 当父组件需要子组件需要共享的参数发生了变化，手动通知子组件
    parentData() {
      if (this.children.length) {
        this.children.map((child = null) => {
          typeof child.updateParentData == "function" && child.updateParentData();
        });
      }
    }
  },
  created() {
    this.children = [];
  },
  computed: {
    // 计算父组件的值是否发生变化
    parentData() {
      return [this.hoverClass, this.col, this.size, this.border];
    },
    // 宫格对齐方式
    gridStyle() {
      let style = new UTSJSONObject({});
      switch (this.align) {
        case "left":
          style.justifyContent = "flex-start";
          break;
        case "center":
          style.justifyContent = "center";
          break;
        case "right":
          style.justifyContent = "flex-end";
          break;
        default:
          style.justifyContent = "flex-start";
      }
      return uni_modules_uviewPlus_libs_function_index.deepMerge(style, uni_modules_uviewPlus_libs_function_index.addStyle(this.customStyle));
    }
  },
  emits: ["click"],
  // 20240409发现抖音小程序如果开启virtualHost会出现严重问题，几乎所有事件包括created等生命周期事件全部失效。
  options: {
    // virtualHost: true ,//将自定义节点设置成虚拟的，更加接近Vue组件的表现。我们不希望自定义组件的这个节点本身可以设置样式、响应 flex 布局等
  },
  methods: {
    // 此方法由u-grid-item触发，用于在u-grid发出事件
    childClick(name = null) {
      this.$emit("click", name);
    }
  }
});
const __injectCSSVars__ = () => {
  common_vendor.useCssVars((_ctx = null) => {
    return new UTSJSONObject({
      "0104dd76": _ctx.gap,
      "0104d020": _ctx.col
    });
  });
};
const __setup__ = __default__.setup;
__default__.setup = __setup__ ? (props, ctx) => {
  __injectCSSVars__();
  return __setup__(props, ctx);
} : __injectCSSVars__;
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return {
    a: common_vendor.sei(common_vendor.gei(_ctx, "", "r0-10b668c8"), "view", "u-grid"),
    b: common_vendor.s($options.gridStyle),
    c: common_vendor.s(_ctx.__cssVars())
  };
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(__default__, [["render", _sfc_render], ["__scopeId", "data-v-10b668c8"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-grid/u-grid.js.map
