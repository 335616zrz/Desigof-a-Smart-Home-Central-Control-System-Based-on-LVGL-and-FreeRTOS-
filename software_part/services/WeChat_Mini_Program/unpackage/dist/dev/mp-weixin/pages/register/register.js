"use strict";
const common_vendor = require("../../common/vendor.js");
const stores_auth = require("../../stores/auth.js");
const common_constants = require("../../common/constants.js");
const utils_validator = require("../../utils/validator.js");
if (!Array) {
  const _easycom_u_input_1 = common_vendor.resolveComponent("u-input");
  const _easycom_u_form_item_1 = common_vendor.resolveComponent("u-form-item");
  const _easycom_u_form_1 = common_vendor.resolveComponent("u-form");
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  (_easycom_u_input_1 + _easycom_u_form_item_1 + _easycom_u_form_1 + _easycom_u_button_1)();
}
const _easycom_u_input = () => "../../uni_modules/uview-plus/components/u-input/u-input.js";
const _easycom_u_form_item = () => "../../uni_modules/uview-plus/components/u-form-item/u-form-item.js";
const _easycom_u_form = () => "../../uni_modules/uview-plus/components/u-form/u-form.js";
const _easycom_u_button = () => "../../uni_modules/uview-plus/components/u-button/u-button.js";
if (!Math) {
  (_easycom_u_input + _easycom_u_form_item + _easycom_u_form + _easycom_u_button)();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "register",
  setup(__props) {
    const auth = stores_auth.useAuthStore();
    const loading = common_vendor.ref(false);
    const form = common_vendor.reactive(new UTSJSONObject({
      username: "",
      password: "",
      email: ""
    }));
    function goLogin() {
      common_vendor.index.navigateBack();
    }
    function onRegister() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (loading.value)
          return Promise.resolve(null);
        if (!utils_validator.required(form.username)) {
          common_vendor.index.showToast({ title: "请输入用户名", icon: "none" });
          return Promise.resolve(null);
        }
        if (!utils_validator.isStrongPassword(form.password, 6)) {
          common_vendor.index.showToast({ title: "密码至少 6 位，且包含两种字符", icon: "none" });
          return Promise.resolve(null);
        }
        if (utils_validator.required(form.email) && !utils_validator.isEmail(form.email)) {
          common_vendor.index.showToast({ title: "邮箱格式不正确", icon: "none" });
          return Promise.resolve(null);
        }
        loading.value = true;
        try {
          yield auth.register(form.username, form.password, form.email);
          common_vendor.index.reLaunch({ url: common_constants.ROUTES.INDEX });
        } catch (e) {
          common_vendor.index.showToast({ title: "注册失败", icon: "none" });
        } finally {
          loading.value = false;
        }
      });
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = {
        a: common_vendor.o(($event) => {
          return common_vendor.unref(form).username = $event;
        }),
        b: common_vendor.p({
          placeholder: "3-64 位",
          clearable: true,
          modelValue: common_vendor.unref(form).username
        }),
        c: common_vendor.p({
          label: "用户名",
          labelWidth: "80"
        }),
        d: common_vendor.o(($event) => {
          return common_vendor.unref(form).password = $event;
        }),
        e: common_vendor.p({
          type: "password",
          placeholder: "至少 6 位",
          password: true,
          modelValue: common_vendor.unref(form).password
        }),
        f: common_vendor.p({
          label: "密码",
          labelWidth: "80"
        }),
        g: common_vendor.o(($event) => {
          return common_vendor.unref(form).email = $event;
        }),
        h: common_vendor.p({
          placeholder: "可选",
          clearable: true,
          modelValue: common_vendor.unref(form).email
        }),
        i: common_vendor.p({
          label: "邮箱",
          labelWidth: "80"
        }),
        j: common_vendor.p({
          model: common_vendor.unref(form)
        }),
        k: common_vendor.o(onRegister),
        l: common_vendor.p({
          type: "primary",
          loading: common_vendor.unref(loading),
          disabled: common_vendor.unref(loading)
        }),
        m: common_vendor.o(goLogin),
        n: common_vendor.p({
          plain: true
        }),
        o: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      };
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/pages/register/register.js.map
