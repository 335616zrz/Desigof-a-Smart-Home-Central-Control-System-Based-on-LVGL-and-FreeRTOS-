"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uGridItem_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-grid-item",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uGridItem_props.props],
  data() {
    return {
      parentData: new UTSJSONObject({
        col: 0,
        border: true
        // 是否显示边框，根据父组件决定
      }),
      classes: []
      // 类名集合，用于判断是否显示右边和下边框
    };
  },
  mounted() {
    this.init();
  },
  emits: ["click"],
  //  微信小程序中 options 选项
  options: {
    virtualHost: true
    //将自定义节点设置成虚拟的，更加接近Vue组件的表现。我们不希望自定义组件的这个节点本身可以设置样式、响应 flex 布局等
  },
  computed: {
    itemStyle() {
      const style = new UTSJSONObject({
        background: this.bgColor
      });
      style["width"] = "100%";
      return uni_modules_uviewPlus_libs_function_index.deepMerge(style, uni_modules_uviewPlus_libs_function_index.addStyle(this.customStyle));
    }
  },
  methods: {
    init() {
      common_vendor.index.$on("$uGridItem", () => {
        this.gridItemClasses();
      });
      this.updateParentData();
      common_vendor.index.$emit("$uGridItem");
      this.gridItemClasses();
    },
    // 获取父组件的参数
    updateParentData() {
      this.getParentData("u-grid");
    },
    clickHandler() {
      var _a;
      let name = this.name;
      const children = (_a = this.parent) === null || _a === void 0 ? null : _a.children;
      if (children && this.name === null) {
        name = children.findIndex((child = null) => {
          return child === this;
        });
      }
      this.parent && this.parent.childClick(name);
      this.$emit("click", name);
    },
    getItemWidth() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        let width = 0;
        if (this.parent) {
          const parentWidth = yield this.getParentWidth();
          width = parentWidth / Number(this.parentData.col) + "px";
        }
        this.width = width;
      });
    },
    // 获取父元素的尺寸
    getParentWidth() {
    },
    gridItemClasses() {
      if (this.parentData.border) {
        let classes = [];
        this.parent.children.map((child = null, index = null) => {
          if (this === child) {
            const len = this.parent.children.length;
            if ((index + 1) % this.parentData.col !== 0 && index + 1 !== len) {
              classes.push("u-border-right");
            }
            const lessNum = len % this.parentData.col === 0 ? this.parentData.col : len % this.parentData.col;
            if (index < len - lessNum) {
              classes.push("u-border-bottom");
            }
          }
        });
        this.classes = classes;
      }
    }
  },
  beforeUnmount() {
    common_vendor.index.$off("$uGridItem");
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: $data.parentData.col > 0
  }, $data.parentData.col > 0 ? {
    b: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
    c: common_vendor.o((...args) => $options.clickHandler && $options.clickHandler(...args)),
    d: common_vendor.n($data.classes),
    e: common_vendor.s($options.itemStyle)
  } : {});
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-0a78094b"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-grid-item/u-grid-item.js.map
