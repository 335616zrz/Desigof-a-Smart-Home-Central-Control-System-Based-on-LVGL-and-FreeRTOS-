"use strict";
Object.defineProperty(exports, Symbol.toStringTag, { value: "Module" });
const common_vendor = require("./common/vendor.js");
const stores_auth = require("./stores/auth.js");
const common_constants = require("./common/constants.js");
const uni_modules_uviewPlus_index = require("./uni_modules/uview-plus/index.js");
const stores_index = require("./stores/index.js");
if (!Math) {
  "./pages/index/index.js";
  "./pages/chartTest/chartTest.js";
  "./pages/login/login.js";
  "./pages/register/register.js";
  "./pages/device/list/list.js";
  "./pages/device/detail/detail.js";
  "./pages/telemetry/history/history.js";
  "./pages/alertRule/list/list.js";
  "./pages/alertRule/edit/edit.js";
  "./pages/alert/list/list.js";
  "./pages/alert/detail/detail.js";
  "./pages/profile/profile.js";
  "./pages/settings/settings.js";
  "./pages/user/user.js";
  "./pages/music/music.js";
}
const WHITE_LIST = [common_constants.ROUTES.LOGIN, common_constants.ROUTES.REGISTER];
function normalizeUrl(url) {
  if (!url)
    return "";
  const pure = url.split("?")[0].split("#")[0];
  return pure.startsWith("/") ? pure : "/" + pure;
}
function isPublic(url) {
  const pure = normalizeUrl(url);
  return WHITE_LIST.indexOf(pure) !== -1;
}
function setupRouteGuard() {
  const auth = stores_auth.useAuthStore();
  auth.initFromStorage();
  const guard = (args = null) => {
    const target = normalizeUrl((args === null || args === void 0 ? null : args.url) || "");
    if (!target)
      return true;
    if (isPublic(target))
      return true;
    if (auth.isAuthenticated)
      return true;
    common_vendor.index.reLaunch({ url: common_constants.ROUTES.LOGIN });
    return false;
  };
  ["navigateTo", "redirectTo", "reLaunch", "switchTab"].forEach((apiName) => {
    common_vendor.index.addInterceptor(apiName, new UTSJSONObject({
      invoke(args = null) {
        return guard(args);
      }
    }));
  });
}
const _sfc_main = common_vendor.defineComponent({
  onLaunch() {
    common_vendor.index.__f__("log", "at App.uvue:47", "App Launch");
    setupRouteGuard();
    const auth = stores_auth.useAuthStore();
    auth.initFromStorage();
    if (!auth.isAuthenticated) {
      common_vendor.index.reLaunch({ url: common_constants.ROUTES.LOGIN });
    }
  },
  onShow() {
    common_vendor.index.__f__("log", "at App.uvue:57", "App Show");
  },
  onHide() {
    common_vendor.index.__f__("log", "at App.uvue:60", "App Hide");
  },
  onExit() {
    common_vendor.index.__f__("log", "at App.uvue:81", "App Exit");
  }
});
function createApp() {
  const app = common_vendor.createSSRApp(_sfc_main);
  app.use(uni_modules_uviewPlus_index.uviewPlus);
  stores_index.setupStore(app);
  return {
    app
  };
}
createApp().app.mount("#app");
exports.createApp = createApp;
//# sourceMappingURL=../.sourcemap/mp-weixin/app.js.map
