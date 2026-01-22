"use strict";
const uni_modules_uviewPlus_libs_vue = require("../../libs/vue.js");
const uni_modules_uviewPlus_libs_config_props = require("../../libs/config/props.js");
const props = uni_modules_uviewPlus_libs_vue.defineMixin({
  props: {
    // 标签类型info、primary、success、warning、error
    type: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.type
    },
    // 不可用
    disabled: {
      type: [Boolean, String],
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.disabled
    },
    // 标签的大小，large，medium，mini
    size: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.size
    },
    // tag的形状，circle（两边半圆形）, square（方形，带圆角）
    shape: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.shape
    },
    // 标签文字
    text: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.text
    },
    // 背景颜色，默认为空字符串，即不处理
    bgColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.bgColor
    },
    // 标签字体颜色，默认为空字符串，即不处理
    color: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.color
    },
    // 标签的边框颜色
    borderColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.borderColor
    },
    // 关闭按钮图标的颜色
    closeColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.closeColor
    },
    // 点击时返回的索引值，用于区分例遍的数组哪个元素被点击了
    name: {
      type: [String, Number],
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.name
    },
    // // 模式选择，dark|light|plain
    // mode: {
    // 	type: String,
    // 	default: 'light'
    // },
    // 镂空时是否填充背景色
    plainFill: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.plainFill
    },
    // 是否镂空
    plain: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.plain
    },
    // 是否可关闭
    closable: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.closable
    },
    // 是否显示
    show: {
      type: Boolean,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.show
    },
    // 内置图标，或绝对路径的图片
    icon: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.icon
    },
    // 图标颜色
    iconColor: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.iconColor
    },
    // 自定义尺寸字体大小
    textSize: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.textSize
    },
    // 自定义尺寸高度
    height: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.height
    },
    // 自定义尺寸padding
    padding: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.padding
    },
    // 自定义尺寸
    borderRadius: {
      type: String,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.borderRadius
    },
    // 自动计算背景色
    autoBgColor: {
      type: Number,
      default: () => uni_modules_uviewPlus_libs_config_props.props.tag.autoBgColor
    }
  }
});
exports.props = props;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-tag/props.js.map
