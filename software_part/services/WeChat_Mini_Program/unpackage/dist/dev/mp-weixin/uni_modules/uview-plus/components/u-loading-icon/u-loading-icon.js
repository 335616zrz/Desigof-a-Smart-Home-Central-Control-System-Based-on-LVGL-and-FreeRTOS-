"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uLoadingIcon_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const uni_modules_uviewPlus_libs_function_colorGradient = require("../../libs/function/colorGradient.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-loading-icon",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uLoadingIcon_props.props],
  data() {
    return {
      // Array.form可以通过一个伪数组对象创建指定长度的数组
      // https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Global_Objects/Array/from
      array12: Array.from({
        length: 12
      }),
      // 这里需要设置默认值为360，否则在安卓nvue上，会延迟一个duration周期后才执行
      // 在iOS nvue上，则会一开始默认执行两个周期的动画
      aniAngel: 360,
      webviewHide: false,
      loading: false
      // 是否运行中，针对nvue使用
    };
  },
  computed: {
    // 当为circle类型时，给其另外三边设置一个更轻一些的颜色
    // 之所以需要这么做的原因是，比如父组件传了color为红色，那么需要另外的三个边为浅红色
    // 而不能是固定的某一个其他颜色(因为这个固定的颜色可能浅蓝，导致效果没有那么细腻良好)
    otherBorderColor() {
      const lightColor = uni_modules_uviewPlus_libs_function_colorGradient.colorGradient$1(this.color, "#ffffff", 100)[80];
      if (this.mode === "circle") {
        return this.inactiveColor ? this.inactiveColor : lightColor;
      } else {
        return "transparent";
      }
    }
  },
  watch: {
    show(n = null) {
    }
  },
  mounted() {
    this.init();
  },
  methods: {
    addUnit: uni_modules_uviewPlus_libs_function_index.addUnit,
    addStyle: uni_modules_uviewPlus_libs_function_index.addStyle,
    init() {
      setTimeout(() => {
      }, 20);
    },
    // 监听webview的显示与隐藏
    addEventListenerToWebview() {
      const pages = getCurrentPages();
      const page = pages[pages.length - 1];
      const currentWebview = page.$getAppWebview();
      currentWebview.addEventListener("hide", () => {
        this.webviewHide = true;
      });
      currentWebview.addEventListener("show", () => {
        this.webviewHide = false;
      });
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.show
  }, _ctx.show ? common_vendor.e({
    b: !$data.webviewHide
  }, !$data.webviewHide ? common_vendor.e({
    c: _ctx.mode === "spinner"
  }, _ctx.mode === "spinner" ? {
    d: common_vendor.f($data.array12, (item, index, i0) => {
      return {
        a: index
      };
    })
  } : {}, {
    e: common_vendor.sei("r0-2af81691", "view", "ani"),
    f: common_vendor.n(`u-loading-icon__spinner--${_ctx.mode}`),
    g: _ctx.color,
    h: $options.addUnit(_ctx.size),
    i: $options.addUnit(_ctx.size),
    j: _ctx.color,
    k: $options.otherBorderColor,
    l: $options.otherBorderColor,
    m: $options.otherBorderColor,
    n: `${_ctx.duration}ms`,
    o: _ctx.mode === "semicircle" || _ctx.mode === "circle" ? _ctx.timingFunction : ""
  }) : {}, {
    p: _ctx.text
  }, _ctx.text ? {
    q: common_vendor.t(_ctx.text),
    r: $options.addUnit(_ctx.textSize),
    s: _ctx.textColor
  } : {}, {
    t: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
    v: common_vendor.s($options.addStyle(_ctx.customStyle)),
    w: common_vendor.n(_ctx.vertical && "u-loading-icon--vertical")
  }) : {});
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-2af81691"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-loading-icon/u-loading-icon.js.map
