"use strict";
const uni_modules_uviewPlus_libs_config_config = require("../../libs/config/config.js");
const uni_modules_uviewPlus_libs_i18n_index = require("../../libs/i18n/index.js");
const {
  color
} = uni_modules_uviewPlus_libs_config_config.config;
const Link = {
  // link超链接组件props参数
  link: {
    color: color["u-primary"],
    fontSize: 15,
    underLine: false,
    href: "",
    mpTips: uni_modules_uviewPlus_libs_i18n_index.t("up.link.copyed"),
    lineColor: "",
    text: ""
  }
};
exports.Link = Link;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-link/link.js.map
