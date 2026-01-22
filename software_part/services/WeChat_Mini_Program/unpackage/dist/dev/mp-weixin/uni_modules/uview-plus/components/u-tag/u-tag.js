"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uTag_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_test = require("../../libs/function/test.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-tag",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uTag_props.props],
  data() {
    return {};
  },
  computed: {
    style() {
      const style = new UTSJSONObject({});
      if (this.bgColor) {
        style.backgroundColor = this.bgColor;
      }
      if (this.color) {
        style.color = this.color;
      }
      if (this.borderColor) {
        style.borderColor = this.borderColor;
      }
      if (this.height) {
        style.height = uni_modules_uviewPlus_libs_function_index.addUnit(this.height);
        style.lineHeight = uni_modules_uviewPlus_libs_function_index.addUnit(this.height);
      }
      if (this.padding) {
        style.padding = this.padding;
      }
      if (this.borderRadius) {
        style.borderRadius = uni_modules_uviewPlus_libs_function_index.addUnit(this.borderRadius);
      }
      if (this.autoBgColor > 0 && this.color) {
        style.backgroundColor = this.getBagColor(this.color);
      }
      return style;
    },
    // nvue下，文本颜色无法继承父元素
    textColor() {
      const style = new UTSJSONObject({});
      if (this.color) {
        style.color = this.color;
      }
      if (this.textSize) {
        style.textSize = uni_modules_uviewPlus_libs_function_index.addUnit(this.textSize);
      }
      return style;
    },
    imgStyle() {
      const width = this.size === "large" ? "17px" : this.size === "medium" ? "15px" : "13px";
      return new UTSJSONObject({
        width,
        height: width
      });
    },
    // 文本的样式
    closeSize() {
      const size = this.size === "large" ? 15 : this.size === "medium" ? 13 : 12;
      return size;
    },
    // 图标大小
    iconSize() {
      const size = this.size === "large" ? 21 : this.size === "medium" ? 19 : 16;
      return size;
    },
    // 图标颜色
    elIconColor() {
      return this.iconColor ? this.iconColor : this.plain ? this.type : "#ffffff";
    }
  },
  emits: ["click", "close"],
  methods: {
    testImage: uni_modules_uviewPlus_libs_function_test.test.image,
    // 点击关闭按钮
    closeHandler() {
      this.$emit("close", this.name);
    },
    // 点击标签
    clickHandler() {
      this.$emit("click", this.name);
    },
    // 根据颜色计算浅色作为背景
    getBagColor(darkColor = null) {
      return uni_modules_uviewPlus_libs_function_index.genLightColor(darkColor, this.autoBgColor);
    }
  }
});
if (!Array) {
  const _easycom_up_icon2 = common_vendor.resolveComponent("up-icon");
  const _easycom_u_transition2 = common_vendor.resolveComponent("u-transition");
  (_easycom_up_icon2 + _easycom_u_transition2)();
}
const _easycom_up_icon = () => "../u-icon/u-icon.js";
const _easycom_u_transition = () => "../u-transition/u-transition.js";
if (!Math) {
  (_easycom_up_icon + _easycom_u_transition)();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.icon
  }, _ctx.icon ? common_vendor.e({
    b: $options.testImage(_ctx.icon)
  }, $options.testImage(_ctx.icon) ? {
    c: _ctx.icon,
    d: common_vendor.s($options.imgStyle)
  } : {
    e: common_vendor.p({
      color: $options.elIconColor,
      name: _ctx.icon,
      size: $options.iconSize
    })
  }) : {}, {
    f: !_ctx.$slots.content
  }, !_ctx.$slots.content ? common_vendor.e({
    g: !_ctx.$slots.default && !_ctx.$slots.$default
  }, !_ctx.$slots.default && !_ctx.$slots.$default ? {
    h: common_vendor.t(_ctx.text),
    i: common_vendor.s($options.textColor),
    j: common_vendor.n(`u-tag__text--${_ctx.type}`),
    k: common_vendor.n(_ctx.plain && `u-tag__text--${_ctx.type}--plain`),
    l: common_vendor.n(`u-tag__text--${_ctx.size}`)
  } : {
    m: common_vendor.s($options.textColor),
    n: common_vendor.n(`u-tag__text--${_ctx.type}`),
    o: common_vendor.n(_ctx.plain && `u-tag__text--${_ctx.type}--plain`),
    p: common_vendor.n(`u-tag__text--${_ctx.size}`)
  }) : {}, {
    q: common_vendor.n(`u-tag--${_ctx.shape}`),
    r: common_vendor.n(!_ctx.plain && `u-tag--${_ctx.type}`),
    s: common_vendor.n(_ctx.plain && `u-tag--${_ctx.type}--plain`),
    t: common_vendor.n(`u-tag--${_ctx.size}`),
    v: common_vendor.n(_ctx.plain && _ctx.plainFill && `u-tag--${_ctx.type}--plain--fill`),
    w: common_vendor.o((...args) => $options.clickHandler && $options.clickHandler(...args)),
    x: common_vendor.s({
      marginRight: _ctx.closable ? "10px" : 0,
      marginTop: _ctx.closable ? "10px" : 0
    }),
    y: common_vendor.s($options.style),
    z: _ctx.closable
  }, _ctx.closable ? {
    A: common_vendor.p({
      name: "close",
      size: $options.closeSize,
      color: "#ffffff"
    }),
    B: common_vendor.n(`u-tag__close--${_ctx.size}`),
    C: common_vendor.o((...args) => $options.closeHandler && $options.closeHandler(...args)),
    D: _ctx.closeColor
  } : {}, {
    E: common_vendor.gei(_ctx, ""),
    F: common_vendor.p({
      mode: "fade",
      show: _ctx.show,
      id: common_vendor.gei(_ctx, "")
    })
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-2fd891bb"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-tag/u-tag.js.map
