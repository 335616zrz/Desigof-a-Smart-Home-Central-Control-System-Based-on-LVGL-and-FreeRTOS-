"use strict";
const uni_modules_uviewPlus_libs_vue = require("../../libs/vue.js");
const uni_modules_uviewPlus_libs_config_props = require("../../libs/config/props.js");
const props = uni_modules_uviewPlus_libs_vue.defineMixin({
  props: {
    // 开始的数值，默认从0增长到某一个数
    startVal: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.startVal
    },
    // 要滚动的目标数值，必须
    endVal: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.endVal
    },
    // 滚动到目标数值的动画持续时间，单位为毫秒（ms）
    duration: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.duration
    },
    // 设置数值后是否自动开始滚动
    autoplay: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.autoplay
    },
    // 要显示的小数位数
    decimals: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.decimals
    },
    // 是否在即将到达目标数值的时候，使用缓慢滚动的效果
    useEasing: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.useEasing
    },
    // 十进制分割
    decimal: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.decimal
    },
    // 字体颜色
    color: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.color
    },
    // 字体大小
    fontSize: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.fontSize
    },
    // 是否加粗字体
    bold: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.bold
    },
    // 千位分隔符，类似金额的分割(￥23,321.05中的",")
    separator: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.countTo.separator
    }
  }
});
exports.props = props;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-count-to/props.js.map
