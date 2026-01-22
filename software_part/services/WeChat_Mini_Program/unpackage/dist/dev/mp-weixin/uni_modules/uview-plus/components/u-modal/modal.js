"use strict";
const uni_modules_uviewPlus_libs_i18n_index = require("../../libs/i18n/index.js");
const Modal = {
  // modal 组件
  modal: {
    show: false,
    title: "",
    content: "",
    confirmText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.confirm"),
    cancelText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.cancel"),
    showConfirmButton: true,
    showCancelButton: false,
    confirmColor: "#2979ff",
    cancelColor: "#606266",
    buttonReverse: false,
    zoom: true,
    asyncClose: false,
    closeOnClickOverlay: false,
    negativeTop: 0,
    width: "650rpx",
    confirmButtonShape: "",
    duration: 400,
    contentTextAlign: "left",
    asyncCloseTip: uni_modules_uviewPlus_libs_i18n_index.t("up.common.inOperation") + "...",
    asyncCancelClose: false,
    contentStyle: {}
  }
};
exports.Modal = Modal;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-modal/modal.js.map
