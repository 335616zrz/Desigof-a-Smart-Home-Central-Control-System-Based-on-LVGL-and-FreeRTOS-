"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_libs_config_config = require("../../libs/config/config.js");
let params = {
  loaded: false
};
const loadFont = () => {
  if (uni_modules_uviewPlus_libs_config_config.config.loadFontOnce) {
    params.loaded = true;
  }
  common_vendor.index.loadFontFace({
    global: true,
    // 是否全局生效。微信小程序 '2.10.0'起支持全局生效，需在 app.vue 中调用。
    family: "uicon-iconfont",
    source: 'url("' + uni_modules_uviewPlus_libs_config_config.config.iconUrl + '")',
    success() {
    },
    fail() {
    }
  });
  if (uni_modules_uviewPlus_libs_config_config.config.customIcon.family) {
    common_vendor.index.loadFontFace({
      global: true,
      // 是否全局生效。微信小程序 '2.10.0'起支持全局生效，需在 app.vue 中调用。
      family: uni_modules_uviewPlus_libs_config_config.config.customIcon.family,
      source: 'url("' + uni_modules_uviewPlus_libs_config_config.config.customIcon.url + '")',
      success() {
      },
      fail() {
      }
    });
  }
  return true;
};
const fontUtil = {
  params,
  loadFont
};
exports.fontUtil = fontUtil;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-icon/util.js.map
