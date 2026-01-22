"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uEmpty_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const uni_modules_uviewPlus_libs_i18n_index = require("../../libs/i18n/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-empty",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uEmpty_props.props],
  data() {
    return {
      icons: new UTSJSONObject({
        car: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.car"),
        page: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.page"),
        search: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.search"),
        address: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.address"),
        wifi: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.wifi"),
        order: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.order"),
        coupon: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.coupon"),
        favor: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.favor"),
        permission: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.permission"),
        history: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.history"),
        news: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.news"),
        message: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.message"),
        list: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.list"),
        data: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.data"),
        comment: uni_modules_uviewPlus_libs_i18n_index.t("up.empty.comment")
      })
    };
  },
  computed: {
    // 组件样式
    emptyStyle() {
      const style = new UTSJSONObject({});
      style.marginTop = uni_modules_uviewPlus_libs_function_index.addUnit(this.marginTop);
      return uni_modules_uviewPlus_libs_function_index.deepMerge(uni_modules_uviewPlus_libs_function_index.addStyle(this.customStyle), style);
    },
    // 文本样式
    textStyle() {
      const style = new UTSJSONObject({});
      style.color = this.textColor;
      style.fontSize = uni_modules_uviewPlus_libs_function_index.addUnit(this.textSize);
      return style;
    },
    // 判断icon是否图片路径
    isSrc() {
      return this.icon.indexOf("/") >= 0;
    }
  },
  methods: {
    addUnit: uni_modules_uviewPlus_libs_function_index.addUnit
  }
});
if (!Array) {
  const _easycom_up_icon2 = common_vendor.resolveComponent("up-icon");
  _easycom_up_icon2();
}
const _easycom_up_icon = () => "../u-icon/u-icon.js";
if (!Math) {
  _easycom_up_icon();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.show
  }, _ctx.show ? common_vendor.e({
    b: !$options.isSrc
  }, !$options.isSrc ? {
    c: common_vendor.p({
      name: _ctx.mode === "message" ? "chat" : `empty-${_ctx.mode}`,
      size: _ctx.iconSize,
      color: _ctx.iconColor,
      ["margin-top"]: "14"
    })
  } : {
    d: $options.addUnit(_ctx.width),
    e: $options.addUnit(_ctx.height),
    f: _ctx.icon
  }, {
    g: common_vendor.t(_ctx.text ? _ctx.text : $data.icons[_ctx.mode]),
    h: common_vendor.s($options.textStyle),
    i: _ctx.$slots.default || _ctx.$slots.$default
  }, _ctx.$slots.default || _ctx.$slots.$default ? {} : {}, {
    j: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
    k: common_vendor.s($options.emptyStyle)
  }) : {});
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-8dd5928e"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-empty/u-empty.js.map
