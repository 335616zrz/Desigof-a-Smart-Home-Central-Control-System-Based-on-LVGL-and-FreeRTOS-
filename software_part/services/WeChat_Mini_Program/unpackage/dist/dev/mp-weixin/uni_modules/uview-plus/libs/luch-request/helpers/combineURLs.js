"use strict";
function combineURLs(baseURL, relativeURL) {
  return relativeURL ? `${baseURL.replace(/\/+$/, "")}/${relativeURL.replace(/^\/+/, "")}` : baseURL;
}
exports.combineURLs = combineURLs;
//# sourceMappingURL=../../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/libs/luch-request/helpers/combineURLs.js.map
