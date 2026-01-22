"use strict";
const uni_modules_uviewPlus_libs_i18n_index = require("../../libs/i18n/index.js");
const Picker = {
  // picker
  picker: {
    show: false,
    popupMode: "bottom",
    showToolbar: true,
    title: "",
    columns: [],
    loading: false,
    itemHeight: 44,
    cancelText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.cancel"),
    confirmText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.confirm"),
    cancelColor: "#909193",
    confirmColor: "",
    visibleItemCount: 5,
    keyName: "text",
    valueName: "value",
    closeOnClickOverlay: false,
    defaultIndex: [],
    immediateChange: true,
    zIndex: 10076,
    disabled: false,
    disabledColor: "",
    placeholder: uni_modules_uviewPlus_libs_i18n_index.t("up.common.pleaseChoose"),
    inputProps: {},
    bgColor: "",
    round: 0,
    duration: 300,
    overlayOpacity: 0.5,
    pageInline: false
  }
};
exports.Picker = Picker;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-picker/picker.js.map
