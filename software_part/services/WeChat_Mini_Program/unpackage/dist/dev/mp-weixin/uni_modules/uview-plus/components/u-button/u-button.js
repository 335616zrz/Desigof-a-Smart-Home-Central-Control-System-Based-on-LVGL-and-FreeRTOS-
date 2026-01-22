"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_libs_mixin_button = require("../../libs/mixin/button.js");
const uni_modules_uviewPlus_libs_mixin_openType = require("../../libs/mixin/openType.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_components_uButton_props = require("./props.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const uni_modules_uviewPlus_libs_function_throttle = require("../../libs/function/throttle.js");
const uni_modules_uviewPlus_libs_config_color = require("../../libs/config/color.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-button",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_libs_mixin_button.buttonMixin, uni_modules_uviewPlus_libs_mixin_openType.openType, uni_modules_uviewPlus_components_uButton_props.props],
  data() {
    return {};
  },
  computed: {
    // 生成bem风格的类名
    bemClass() {
      if (!this.color) {
        return this.bem("button", ["type", "shape", "size"], ["disabled", "plain", "hairline"]);
      } else {
        return this.bem("button", ["shape", "size"], ["disabled", "plain", "hairline"]);
      }
    },
    loadingColor() {
      if (this.plain) {
        return this.color ? this.color : uni_modules_uviewPlus_libs_config_color.color[`u-${this.type}`];
      }
      if (this.type === "info") {
        return "#c9c9c9";
      }
      return "rgb(200, 200, 200)";
    },
    iconColorCom() {
      if (this.iconColor)
        return this.iconColor;
      if (this.plain) {
        return this.color ? this.color : this.type;
      } else {
        return this.type === "info" ? "#000000" : "#ffffff";
      }
    },
    baseColor() {
      let style = new UTSJSONObject({});
      if (this.color) {
        style.color = this.plain ? this.color : "white";
        if (!this.plain) {
          style["background-color"] = this.color;
        }
        if (this.color.indexOf("gradient") !== -1) {
          style.borderTopWidth = 0;
          style.borderRightWidth = 0;
          style.borderBottomWidth = 0;
          style.borderLeftWidth = 0;
          if (!this.plain) {
            style.backgroundImage = this.color;
          }
        } else {
          style.borderColor = this.color;
          style.borderWidth = "1px";
          style.borderStyle = "solid";
        }
      }
      return style;
    },
    // nvue版本按钮的字体不会继承父组件的颜色，需要对每一个text组件进行单独的设置
    nvueTextStyle() {
      let style = new UTSJSONObject({});
      if (this.type === "info") {
        style.color = "#323233";
      }
      if (this.color) {
        style.color = this.plain ? this.color : "white";
      }
      style.fontSize = this.textSize + "px";
      return style;
    },
    // 字体大小
    textSize() {
      let fontSize = 14, size = this.size;
      if (size === "large")
        fontSize = 16;
      if (size === "normal")
        fontSize = 14;
      if (size === "small")
        fontSize = 12;
      if (size === "mini")
        fontSize = 10;
      return fontSize;
    }
  },
  emits: [
    "click",
    "getphonenumber",
    "getuserinfo",
    "error",
    "opensetting",
    "launchapp",
    "agreeprivacyauthorization"
  ],
  methods: {
    addStyle: uni_modules_uviewPlus_libs_function_index.addStyle,
    clickHandler(e = null) {
      if (!this.disabled && !this.loading) {
        uni_modules_uviewPlus_libs_function_throttle.throttle(() => {
          this.$emit("click", e);
        }, this.throttleTime);
      }
      this.stop && this.preventEvent(e);
    },
    // 下面为对接uniapp官方按钮开放能力事件回调的对接
    getphonenumber(res = null) {
      this.$emit("getphonenumber", res);
    },
    getuserinfo(res = null) {
      this.$emit("getuserinfo", res);
    },
    error(res = null) {
      this.$emit("error", res);
    },
    opensetting(res = null) {
      this.$emit("opensetting", res);
    },
    launchapp(res = null) {
      this.$emit("launchapp", res);
    },
    agreeprivacyauthorization(res = null) {
      this.$emit("agreeprivacyauthorization", res);
    }
  }
});
if (!Array) {
  const _easycom_u_loading_icon2 = common_vendor.resolveComponent("u-loading-icon");
  const _easycom_up_icon2 = common_vendor.resolveComponent("up-icon");
  (_easycom_u_loading_icon2 + _easycom_up_icon2)();
}
const _easycom_u_loading_icon = () => "../u-loading-icon/u-loading-icon.js";
const _easycom_up_icon = () => "../u-icon/u-icon.js";
if (!Math) {
  (_easycom_u_loading_icon + _easycom_up_icon)();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.loading
  }, _ctx.loading ? {
    b: common_vendor.p({
      mode: _ctx.loadingMode,
      size: _ctx.loadingSize * 1.15,
      color: $options.loadingColor
    }),
    c: common_vendor.t(_ctx.loadingText || _ctx.text),
    d: common_vendor.s({
      fontSize: $options.textSize + "px"
    })
  } : common_vendor.e({
    e: _ctx.icon
  }, _ctx.icon ? {
    f: common_vendor.p({
      name: _ctx.icon,
      color: $options.iconColorCom,
      size: $options.textSize * 1.35,
      customStyle: {
        marginRight: "2px"
      }
    })
  } : {}, {
    g: common_vendor.t(_ctx.text),
    h: common_vendor.s({
      fontSize: $options.textSize + "px"
    })
  }), {
    i: common_vendor.sei(common_vendor.gei(_ctx, ""), "button"),
    j: Number(_ctx.hoverStartTime),
    k: Number(_ctx.hoverStayTime),
    l: _ctx.formType,
    m: _ctx.openType,
    n: _ctx.appParameter,
    o: _ctx.hoverStopPropagation,
    p: _ctx.sendMessageTitle,
    q: _ctx.sendMessagePath,
    r: _ctx.lang,
    s: _ctx.dataName,
    t: _ctx.sessionFrom,
    v: _ctx.sendMessageImg,
    w: _ctx.showMessageCard,
    x: common_vendor.o((...args) => $options.getphonenumber && $options.getphonenumber(...args)),
    y: common_vendor.o((...args) => $options.getuserinfo && $options.getuserinfo(...args)),
    z: common_vendor.o((...args) => $options.error && $options.error(...args)),
    A: common_vendor.o((...args) => $options.opensetting && $options.opensetting(...args)),
    B: common_vendor.o((...args) => $options.launchapp && $options.launchapp(...args)),
    C: common_vendor.o((...args) => $options.agreeprivacyauthorization && $options.agreeprivacyauthorization(...args)),
    D: !_ctx.disabled && !_ctx.loading ? "u-button--active" : "",
    E: common_vendor.s($options.baseColor),
    F: common_vendor.s($options.addStyle(_ctx.customStyle)),
    G: common_vendor.o((...args) => $options.clickHandler && $options.clickHandler(...args)),
    H: common_vendor.n($options.bemClass)
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-5ce41ee6"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-button/u-button.js.map
