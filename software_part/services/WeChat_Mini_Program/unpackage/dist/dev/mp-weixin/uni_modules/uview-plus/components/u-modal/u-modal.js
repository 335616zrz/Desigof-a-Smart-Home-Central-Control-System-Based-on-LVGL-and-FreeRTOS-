"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uModal_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-modal",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uModal_props.props],
  data() {
    return {
      loading: false
    };
  },
  watch: {
    show(n = null) {
      if (n && this.loading)
        this.loading = false;
    }
  },
  emits: ["confirm", "cancel", "close", "update:show", "cancelOnAsync"],
  computed: {
    contentStyleCpu() {
      let style = this.contentStyle;
      style.paddingTop = `${this.title ? 12 : 25}px`;
      return style;
    }
  },
  methods: {
    addUnit: uni_modules_uviewPlus_libs_function_index.addUnit,
    // 点击确定按钮
    confirmHandler() {
      if (this.asyncClose) {
        this.loading = true;
      } else {
        this.$emit("update:show", false);
      }
      this.$emit("confirm");
    },
    // 点击取消按钮
    cancelHandler() {
      if (this.asyncClose && this.loading) {
        if (this.asyncCloseTip) {
          common_vendor.index.showToast({
            title: this.asyncCloseTip,
            icon: "none"
          });
        }
        this.$emit("cancelOnAsync");
      } else {
        if (!this.asyncCancelClose) {
          this.$emit("update:show", false);
        }
      }
      this.$emit("cancel");
    },
    // 点击遮罩
    // 从原理上来说，modal的遮罩点击，并不是真的点击到了遮罩
    // 因为modal依赖于popup的中部弹窗类型，中部弹窗比较特殊，虽有然遮罩，但是为了让弹窗内容能flex居中
    // 多了一个透明的遮罩，此透明的遮罩会覆盖在灰色的遮罩上，所以实际上是点击不到灰色遮罩的，popup内部在
    // 透明遮罩的子元素做了.stop处理，所以点击内容区，也不会导致误触发
    clickHandler() {
      if (this.closeOnClickOverlay) {
        this.$emit("update:show", false);
        this.$emit("close");
      }
    }
  }
});
if (!Array) {
  const _easycom_u_line2 = common_vendor.resolveComponent("u-line");
  const _easycom_u_loading_icon2 = common_vendor.resolveComponent("u-loading-icon");
  const _easycom_u_popup2 = common_vendor.resolveComponent("u-popup");
  (_easycom_u_line2 + _easycom_u_loading_icon2 + _easycom_u_popup2)();
}
const _easycom_u_line = () => "../u-line/u-line.js";
const _easycom_u_loading_icon = () => "../u-loading-icon/u-loading-icon.js";
const _easycom_u_popup = () => "../u-popup/u-popup.js";
if (!Math) {
  (_easycom_u_line + _easycom_u_loading_icon + _easycom_u_popup)();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.title
  }, _ctx.title ? {
    b: common_vendor.t(_ctx.title)
  } : {}, {
    c: common_vendor.t(_ctx.content),
    d: _ctx.contentTextAlign,
    e: common_vendor.s($options.contentStyleCpu),
    f: _ctx.$slots.confirmButton
  }, _ctx.$slots.confirmButton ? {} : common_vendor.e({
    g: _ctx.showCancelButton
  }, _ctx.showCancelButton ? {
    h: common_vendor.t(_ctx.cancelText),
    i: _ctx.cancelColor,
    j: common_vendor.n(_ctx.showCancelButton && !_ctx.showConfirmButton && "u-modal__button-group__wrapper--only-cancel"),
    k: common_vendor.o((...args) => $options.cancelHandler && $options.cancelHandler(...args))
  } : {}, {
    l: _ctx.showConfirmButton && _ctx.showCancelButton
  }, _ctx.showConfirmButton && _ctx.showCancelButton ? {
    m: common_vendor.p({
      direction: "column"
    })
  } : {}, {
    n: _ctx.showConfirmButton
  }, _ctx.showConfirmButton ? common_vendor.e({
    o: $data.loading
  }, $data.loading ? {} : {
    p: common_vendor.t(_ctx.confirmText),
    q: _ctx.confirmColor
  }, {
    r: common_vendor.n(!_ctx.showCancelButton && _ctx.showConfirmButton && "u-modal__button-group__wrapper--only-confirm"),
    s: common_vendor.o((...args) => $options.confirmHandler && $options.confirmHandler(...args))
  }) : {}, {
    t: _ctx.buttonReverse ? "row-reverse" : "row"
  }), {
    v: $options.addUnit(_ctx.width),
    w: common_vendor.gei(_ctx, ""),
    x: common_vendor.n(_ctx.customClass),
    y: common_vendor.o($options.clickHandler),
    z: common_vendor.p({
      mode: "center",
      zoom: _ctx.zoom,
      show: _ctx.show,
      customStyle: {
        borderRadius: "6px",
        overflow: "hidden",
        marginTop: `-${$options.addUnit(_ctx.negativeTop)}`
      },
      closeOnClickOverlay: _ctx.closeOnClickOverlay,
      safeAreaInsetBottom: false,
      duration: _ctx.duration,
      id: common_vendor.gei(_ctx, "")
    })
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-f667648f"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-modal/u-modal.js.map
