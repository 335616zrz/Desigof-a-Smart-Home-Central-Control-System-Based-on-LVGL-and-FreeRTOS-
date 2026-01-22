"use strict";
const uni_modules_uviewPlus_libs_i18n_index = require("../../libs/i18n/index.js");
const Calendar = {
  // calendar 组件
  calendar: {
    title: uni_modules_uviewPlus_libs_i18n_index.t("up.calendar.chooseDates"),
    showTitle: true,
    showSubtitle: true,
    mode: "single",
    startText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.start"),
    endText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.end"),
    customList: [],
    color: "#3c9cff",
    minDate: 0,
    maxDate: 0,
    defaultDate: null,
    maxCount: Number.MAX_SAFE_INTEGER,
    // Infinity
    rowHeight: 56,
    formatter: null,
    showLunar: false,
    showMark: true,
    confirmText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.confirm"),
    confirmDisabledText: uni_modules_uviewPlus_libs_i18n_index.t("up.common.confirm"),
    show: false,
    closeOnClickOverlay: false,
    readonly: false,
    showConfirm: true,
    maxRange: Number.MAX_SAFE_INTEGER,
    // Infinity
    rangePrompt: "",
    showRangePrompt: true,
    allowSameDay: false,
    round: 0,
    monthNum: 3,
    weekText: [uni_modules_uviewPlus_libs_i18n_index.t("up.week.one"), uni_modules_uviewPlus_libs_i18n_index.t("up.week.two"), uni_modules_uviewPlus_libs_i18n_index.t("up.week.three"), uni_modules_uviewPlus_libs_i18n_index.t("up.week.four"), uni_modules_uviewPlus_libs_i18n_index.t("up.week.five"), uni_modules_uviewPlus_libs_i18n_index.t("up.week.six"), uni_modules_uviewPlus_libs_i18n_index.t("up.week.seven")],
    forbidDays: [],
    forbidDaysToast: uni_modules_uviewPlus_libs_i18n_index.t("up.calendar.disabled"),
    monthFormat: "",
    pageInline: false
  }
};
exports.Calendar = Calendar;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-calendar/calendar.js.map
