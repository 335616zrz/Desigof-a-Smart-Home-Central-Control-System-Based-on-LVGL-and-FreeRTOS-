"use strict";
const common_vendor = require("../../common/vendor.js");
const common_constants = require("../../common/constants.js");
const stores_auth = require("../../stores/auth.js");
if (!Array) {
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  _easycom_u_button_1();
}
const _easycom_u_button = () => "../../uni_modules/uview-plus/components/u-button/u-button.js";
if (!Math) {
  _easycom_u_button();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "profile",
  setup(__props) {
    const auth = stores_auth.useAuthStore();
    const loading = common_vendor.ref(false);
    const user = common_vendor.computed(() => {
      return auth.user;
    });
    common_vendor.onShow(() => {
      auth.initFromStorage();
      refresh();
    });
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (loading.value)
          return Promise.resolve(null);
        loading.value = true;
        try {
          yield auth.fetchProfile();
        } catch (e) {
        } finally {
          loading.value = false;
        }
      });
    }
    function goSettings() {
      common_vendor.index.navigateTo({ url: "/pages/settings/settings" });
    }
    function onLogout() {
      auth.logout();
      common_vendor.index.reLaunch({ url: common_constants.ROUTES.LOGIN });
    }
    return (_ctx, _cache) => {
      "raw js";
      var _a, _b, _c;
      const __returned__ = {
        a: common_vendor.t(((_a = common_vendor.unref(user)) == null ? void 0 : _a.username) || "-"),
        b: common_vendor.t(((_b = common_vendor.unref(user)) == null ? void 0 : _b.role) || "-"),
        c: common_vendor.t(((_c = common_vendor.unref(user)) == null ? void 0 : _c.email) || "-"),
        d: common_vendor.o(refresh),
        e: common_vendor.p({
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        f: common_vendor.o(goSettings),
        g: common_vendor.p({
          size: "mini",
          plain: true
        }),
        h: common_vendor.o(onLogout),
        i: common_vendor.p({
          size: "mini",
          type: "error",
          plain: true
        }),
        j: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      };
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/pages/profile/profile.js.map
