"use strict";
const uni_modules_uviewPlus_libs_vue = require("../../libs/vue.js");
const uni_modules_uviewPlus_libs_config_props = require("../../libs/config/props.js");
const props = uni_modules_uviewPlus_libs_vue.defineMixin({
  props: {
    // 当前form的需要验证字段的集合
    model: {
      type: Object,
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.model
    },
    // 验证规则
    rules: {
      type: [Object, Function, Array],
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.rules
    },
    // 有错误时的提示方式，message-提示信息，toast-进行toast提示
    // border-bottom-下边框呈现红色，none-无提示
    errorType: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.errorType
    },
    // 是否显示表单域的下划线边框
    borderBottom: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.borderBottom
    },
    // label的位置，left-左边，top-上边
    labelPosition: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.labelPosition
    },
    // label的宽度，单位px
    labelWidth: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.labelWidth
    },
    // lable字体的对齐方式
    labelAlign: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.labelAlign
    },
    // lable的样式，对象形式
    labelStyle: {
      type: Object,
      default: () => uni_modules_uviewPlus_libs_config_props.props.form.labelStyle
    }
  }
});
exports.props = props;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-form/props.js.map
