"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uSubsection_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-subsection",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uSubsection_props.props],
  data() {
    return {
      // 组件尺寸
      itemRect: new UTSJSONObject({
        width: 0,
        height: 0
      }),
      innerCurrent: "",
      windowResizeCallback: new UTSJSONObject({})
    };
  },
  watch: {
    list(newValue = null, oldValue = null) {
      this.init();
    },
    current: new UTSJSONObject({
      immediate: true,
      handler(n = null) {
        if (n !== this.innerCurrent) {
          this.innerCurrent = Number(n);
        }
      }
    })
  },
  computed: {
    wrapperStyle() {
      const style = new UTSJSONObject({});
      if (this.mode === "button") {
        style.backgroundColor = this.bgColor;
      }
      return style;
    },
    // 滑块的样式
    barStyle() {
      const style = new UTSJSONObject({});
      style.width = `${this.itemRect.width}px`;
      style.height = `${this.itemRect.height}px`;
      style.transform = `translateX(${this.innerCurrent * this.itemRect.width}px)`;
      if (this.mode === "subsection") {
        style.backgroundColor = this.activeColor;
      }
      return style;
    },
    // 分段器item的样式
    itemStyle(index = null) {
      return (index2 = null) => {
        const style = new UTSJSONObject({});
        if (this.mode === "subsection") {
          style.borderColor = this.activeColor;
          style.borderWidth = "1px";
          style.borderStyle = "solid";
        }
        return style;
      };
    },
    // 分段器文字颜色
    textStyle(index = null, item = null) {
      return (index2 = null, item2 = null) => {
        const style = new UTSJSONObject({});
        style.fontWeight = this.bold && this.innerCurrent === index2 ? "bold" : "normal";
        style.fontSize = uni_modules_uviewPlus_libs_function_index.addUnit(this.fontSize);
        let activeColorTemp = null;
        let inactiveColorTemp = null;
        if (typeof item2 === "object" && item2[this.activeColorKeyName]) {
          activeColorTemp = item2[this.activeColorKeyName];
        }
        if (typeof item2 === "object" && item2[this.inactiveColorKeyName]) {
          inactiveColorTemp = item2[this.inactiveColorKeyName];
        }
        if (this.mode === "subsection") {
          if (this.innerCurrent === index2) {
            style.color = activeColorTemp ? activeColorTemp : "#FFF";
          } else {
            style.color = inactiveColorTemp ? inactiveColorTemp : this.inactiveColor;
          }
        } else {
          if (this.innerCurrent === index2) {
            style.color = activeColorTemp ? activeColorTemp : this.activeColor;
          } else {
            style.color = inactiveColorTemp ? inactiveColorTemp : this.inactiveColor;
          }
        }
        return style;
      };
    }
  },
  mounted() {
    this.init();
  },
  beforeUnmount() {
  },
  emits: ["change", "update:current"],
  methods: {
    addStyle: uni_modules_uviewPlus_libs_function_index.addStyle,
    init() {
      this.innerCurrent = this.current;
      uni_modules_uviewPlus_libs_function_index.sleep().then(() => {
        return this.getRect();
      });
    },
    // 判断展示文本
    getText(item = null) {
      return typeof item === "object" ? item[this.keyName] : item;
    },
    // 获取组件的尺寸
    getRect() {
      this.$uGetRect(".u-subsection__item--0").then((size = null) => {
        this.itemRect = size;
      });
    },
    clickHandler(index = null) {
      if (this.disabled) {
        return null;
      }
      this.innerCurrent = index;
      this.$emit("update:current", index);
      this.$emit("change", index);
    },
    /**
     * 获取当前文字区域的 class禁用样式
     * @param index
     */
    getTextViewDisableClass(index = null) {
      if (this.disabled) {
        if (this.mode === "button") {
          return "item-button--disabled";
        } else {
          return "item-subsection--disabled";
        }
      }
      return "";
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return {
    a: common_vendor.sei("r0-b5ccb67e", "view", "u-subsection__bar"),
    b: common_vendor.s($options.barStyle),
    c: common_vendor.n(_ctx.mode === "button" && "u-subsection--button__bar"),
    d: common_vendor.n($data.innerCurrent === 0 && _ctx.mode === "subsection" && "u-subsection__bar--first"),
    e: common_vendor.n($data.innerCurrent > 0 && $data.innerCurrent < _ctx.list.length - 1 && _ctx.mode === "subsection" && "u-subsection__bar--center"),
    f: common_vendor.n($data.innerCurrent === _ctx.list.length - 1 && _ctx.mode === "subsection" && "u-subsection__bar--last"),
    g: common_vendor.f(_ctx.list, (item, index, i0) => {
      return {
        a: common_vendor.t($options.getText(item)),
        b: common_vendor.s($options.textStyle(index, item)),
        c: common_vendor.sei("r1-b5ccb67e-" + index, "view", `u-subsection__item--${index}`, {
          "f": 1
        }),
        d: common_vendor.n(`u-subsection__item--${index}`),
        e: common_vendor.n(index < _ctx.list.length - 1 && "u-subsection__item--no-border-right"),
        f: common_vendor.n(index === 0 && "u-subsection__item--first"),
        g: common_vendor.n(index === _ctx.list.length - 1 && "u-subsection__item--last"),
        h: common_vendor.n($options.getTextViewDisableClass(index)),
        i: `u-subsection__item--${index}`,
        j: common_vendor.s($options.itemStyle(index)),
        k: common_vendor.o(($event) => $options.clickHandler(index), index),
        l: index
      };
    }),
    h: common_vendor.n(_ctx.disabled ? "u-subsection--disabled" : ""),
    i: common_vendor.sei(common_vendor.gei(_ctx, "", "r2-b5ccb67e"), "view", "u-subsection"),
    j: common_vendor.n(`u-subsection--${_ctx.mode}`),
    k: common_vendor.s($options.addStyle(_ctx.customStyle)),
    l: common_vendor.s($options.wrapperStyle)
  };
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-b5ccb67e"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-subsection/u-subsection.js.map
