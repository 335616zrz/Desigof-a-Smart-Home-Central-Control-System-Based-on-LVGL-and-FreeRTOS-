"use strict";
const common_vendor = require("./common/vendor.js");
const uni_modules_uviewPlus_components_uIcon_icons = require("./uni_modules/uview-plus/components/u-icon/icons.js");
const uni_modules_uviewPlus_components_uIcon_props = require("./uni_modules/uview-plus/components/u-icon/props.js");
const uni_modules_uviewPlus_libs_config_config = require("./uni_modules/uview-plus/libs/config/config.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("./uni_modules/uview-plus/libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("./uni_modules/uview-plus/libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("./uni_modules/uview-plus/libs/function/index.js");
const uni_modules_uviewPlus_components_uIcon_util = require("./uni_modules/uview-plus/components/u-icon/util.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-icon",
  beforeCreate() {
    if (!uni_modules_uviewPlus_components_uIcon_util.fontUtil.params.loaded) {
      uni_modules_uviewPlus_components_uIcon_util.fontUtil.loadFont();
    }
  },
  data() {
    return {};
  },
  emits: ["click"],
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uIcon_props.props],
  computed: {
    uClasses() {
      let classes = [];
      classes.push(this.customPrefix + "-" + this.name);
      if (this.customPrefix == "uicon") {
        classes.push("u-iconfont");
      } else {
        classes.push(this.customPrefix);
      }
      if (this.color && uni_modules_uviewPlus_libs_config_config.config.type.includes(this.color))
        classes.push("u-icon__icon--" + this.color);
      return classes;
    },
    iconStyle() {
      let style = new UTSJSONObject({});
      style = new UTSJSONObject({
        fontSize: uni_modules_uviewPlus_libs_function_index.addUnit(this.size),
        lineHeight: uni_modules_uviewPlus_libs_function_index.addUnit(this.size),
        fontWeight: this.bold ? "bold" : "normal",
        // 某些特殊情况需要设置一个到顶部的距离，才能更好的垂直居中
        top: uni_modules_uviewPlus_libs_function_index.addUnit(this.top)
      });
      if (this.customPrefix !== "uicon") {
        style.fontFamily = this.customPrefix;
      }
      if (this.color && !uni_modules_uviewPlus_libs_config_config.config.type.includes(this.color))
        style.color = this.color;
      return style;
    },
    // 判断传入的name属性，是否图片路径，只要带有"/"均认为是图片形式
    isImg() {
      return this.name.indexOf("/") !== -1;
    },
    imgStyle() {
      let style = new UTSJSONObject(
        {}
        // 如果设置width和height属性，则优先使用，否则使用size属性
      );
      style.width = this.width ? uni_modules_uviewPlus_libs_function_index.addUnit(this.width) : uni_modules_uviewPlus_libs_function_index.addUnit(this.size);
      style.height = this.height ? uni_modules_uviewPlus_libs_function_index.addUnit(this.height) : uni_modules_uviewPlus_libs_function_index.addUnit(this.size);
      return style;
    },
    // 通过图标名，查找对应的图标
    icon() {
      if (this.customPrefix !== "uicon") {
        return uni_modules_uviewPlus_libs_config_config.config.customIcons[this.name] || this.name;
      }
      return uni_modules_uviewPlus_components_uIcon_icons.icons["uicon-" + this.name] || this.name;
    }
  },
  methods: {
    addStyle: uni_modules_uviewPlus_libs_function_index.addStyle,
    addUnit: uni_modules_uviewPlus_libs_function_index.addUnit,
    clickHandler(e = null) {
      this.$emit("click", this.index, e);
      this.stop && this.preventEvent(e);
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: $options.isImg
  }, $options.isImg ? {
    b: _ctx.name,
    c: _ctx.imgMode,
    d: common_vendor.s($options.imgStyle),
    e: common_vendor.s($options.addStyle(_ctx.customStyle))
  } : {
    f: common_vendor.t($options.icon),
    g: common_vendor.n($options.uClasses),
    h: common_vendor.s($options.iconStyle),
    i: common_vendor.s($options.addStyle(_ctx.customStyle)),
    j: _ctx.hoverClass
  }, {
    k: _ctx.label !== ""
  }, _ctx.label !== "" ? {
    l: common_vendor.t(_ctx.label),
    m: _ctx.labelColor,
    n: $options.addUnit(_ctx.labelSize),
    o: _ctx.labelPos == "right" ? $options.addUnit(_ctx.space) : 0,
    p: _ctx.labelPos == "bottom" ? $options.addUnit(_ctx.space) : 0,
    q: _ctx.labelPos == "left" ? $options.addUnit(_ctx.space) : 0,
    r: _ctx.labelPos == "top" ? $options.addUnit(_ctx.space) : 0
  } : {}, {
    s: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
    t: common_vendor.o((...args) => $options.clickHandler && $options.clickHandler(...args)),
    v: common_vendor.n("u-icon--" + _ctx.labelPos)
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-ac70166d"]]);
exports.Component = Component;
//# sourceMappingURL=../.sourcemap/mp-weixin/u-icon.js.map
