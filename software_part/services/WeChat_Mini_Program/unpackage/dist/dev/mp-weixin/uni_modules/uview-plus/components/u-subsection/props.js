"use strict";
const uni_modules_uviewPlus_libs_vue = require("../../libs/vue.js");
const uni_modules_uviewPlus_libs_config_props = require("../../libs/config/props.js");
const props = uni_modules_uviewPlus_libs_vue.defineMixin({
  props: {
    // tab的数据
    list: {
      type: Array,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.list
    },
    // 当前活动的tab的index
    current: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.current
    },
    // 激活的颜色
    activeColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.activeColor
    },
    // 未激活的颜色
    inactiveColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.inactiveColor
    },
    // 模式选择，mode=button为按钮形式，mode=subsection时为分段模式
    mode: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.mode
    },
    // 字体大小
    fontSize: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.fontSize
    },
    // 激活tab的字体是否加粗
    bold: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.bold
    },
    // mode = button时，组件背景颜色
    bgColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.bgColor
    },
    // 从list元素对象中读取的键名
    keyName: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.keyName
    },
    // 从`list`元素对象中读取激活时的颜色  如果存在字段 优先级大于 activeColor
    activeColorKeyName: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.activeColorKeyName
    },
    // 从`list`元素对象中读取未激活时的颜色 如果存在字段 优先级大于 inactiveColor
    inactiveColorKeyName: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.inactiveColorKeyName
    },
    // 是否禁用
    disabled: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.subsection.disabled
    }
  }
});
exports.props = props;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-subsection/props.js.map
