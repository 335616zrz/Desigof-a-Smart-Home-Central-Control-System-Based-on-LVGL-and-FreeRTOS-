"use strict";
const uni_modules_uviewPlus_libs_vue = require("../../libs/vue.js");
const uni_modules_uviewPlus_libs_config_props = require("../../libs/config/props.js");
const props = uni_modules_uviewPlus_libs_vue.defineMixin({
  props: {
    // 分成几列
    col: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.grid.col
    },
    // 是否显示边框
    border: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.grid.border
    },
    // 宫格对齐方式，表现为数量少的时候，靠左，居中，还是靠右
    align: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.grid.align
    },
    // 间隔
    gap: {
      type: String,
      default: "0px"
    }
  }
});
exports.props = props;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-grid/props.js.map
