"use strict";
const common_vendor = require("../common/vendor.js");
const common_config = require("../common/config.js");
const common_constants = require("../common/constants.js");
const common_storage = require("../common/storage.js");
let _redirectingToLogin = false;
function buildUrl(path) {
  if (!path)
    return common_config.getBaseUrl();
  if (/^https?:\/\//i.test(path))
    return path;
  const base = common_config.getBaseUrl().replace(/\/$/, "");
  const p = String(path).startsWith("/") ? path : `/${path}`;
  return `${base}${p}`;
}
function redirectToLogin() {
  if (_redirectingToLogin)
    return;
  _redirectingToLogin = true;
  try {
    common_vendor.index.reLaunch({ url: common_constants.ROUTES.LOGIN });
  } catch (e) {
  }
  setTimeout(() => {
    _redirectingToLogin = false;
  }, 1200);
}
function request(options) {
  const token = common_storage.getString(common_constants.STORAGE_KEYS.TOKEN, "");
  const method = (options.method || "GET").toUpperCase();
  common_vendor.index.__f__("log", "at api/http.js:34", "[http.js] request:", options.url, "token:", token ? `${token.substring(0, 20)}...` : "(empty)");
  const defaultContentType = ["POST", "PUT", "PATCH"].includes(method) ? "application/json" : void 0;
  return new Promise((resolve, reject) => {
    common_vendor.index.request({
      url: buildUrl(options.url),
      method,
      data: options.data || {},
      timeout: options.timeout ?? common_config.REQUEST_TIMEOUT_MS,
      header: {
        ...defaultContentType ? { "Content-Type": defaultContentType } : {},
        ...options.header || {},
        ...token ? { [common_constants.HTTP.AUTH_HEADER]: `${common_constants.HTTP.TOKEN_PREFIX}${token}` } : {}
      },
      success: (res) => {
        const { statusCode, data } = res || {};
        if (statusCode === 401) {
          common_storage.remove(common_constants.STORAGE_KEYS.TOKEN);
          common_storage.remove(common_constants.STORAGE_KEYS.USER);
          redirectToLogin();
          reject(res);
          return;
        }
        if (statusCode === 403) {
          common_vendor.index.showToast({ title: "无权限", icon: "none" });
          reject(res);
          return;
        }
        if (data && typeof data === "object" && "code" in data) {
          const code = data.code;
          if (code !== 200 && code !== 0) {
            common_vendor.index.showToast({ title: data.msg || data.message || "请求失败", icon: "none" });
            reject(data);
            return;
          }
          resolve(data);
          return;
        }
        if (statusCode >= 200 && statusCode < 300) {
          resolve(data);
          return;
        }
        common_vendor.index.showToast({ title: "网络请求失败", icon: "none" });
        reject(res);
      },
      fail: (err) => {
        common_vendor.index.showToast({ title: (err == null ? void 0 : err.errMsg) || "网络异常", icon: "none" });
        reject(err);
      }
    });
  });
}
exports.request = request;
//# sourceMappingURL=../../.sourcemap/mp-weixin/api/http.js.map
