"use strict";
const uni_modules_uviewPlus_libs_i18n_index = require("../../libs/i18n/index.js");
const DatetimePicker = {
  // datetimePicker 组件
  datetimePicker: {
    show: false,
    popupMode: "bottom",
    showToolbar: true,
    value: "",
    title: "",
    mode: "datetime",
    maxDate: new Date((/* @__PURE__ */ new Date()).getFullYear() + 10, 0, 1).getTime(),
    minDate: new Date((/* @__PURE__ */ new Date()).getFullYear() - 10, 0, 1).getTime(),
    minHour: 0,
    maxHour: 23,
    minMinute: 0,
    maxMinute: 59,
    filter: null,
    formatter: null,
    loading: false,
    itemHeight: 44,
    cancelText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.cancel"),
    confirmText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.confirm"),
    cancelColor: "#909193",
    confirmColor: "#3c9cff",
    visibleItemCount: 5,
    closeOnClickOverlay: false,
    defaultIndex: [],
    inputBorder: "surround",
    disabled: false,
    disabledColor: "",
    placeholder: uni_modules_uviewPlus_libs_i18n_index.t("up.common.pleaseChoose"),
    inputProps: {},
    pageInline: false
  }
};
exports.DatetimePicker = DatetimePicker;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-datetime-picker/datetimePicker.js.map
