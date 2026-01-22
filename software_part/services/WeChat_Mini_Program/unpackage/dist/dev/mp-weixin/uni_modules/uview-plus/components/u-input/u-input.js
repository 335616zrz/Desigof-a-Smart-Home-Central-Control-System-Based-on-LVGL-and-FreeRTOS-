"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uInput_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-input",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uInput_props.props],
  data() {
    return {
      // 清除操作
      clearInput: false,
      // 输入框的值
      innerValue: "",
      // 是否处于获得焦点状态
      focused: false,
      // value是否第一次变化，在watch中，由于加入immediate属性，会在第一次触发，此时不应该认为value发生了变化
      firstChange: true,
      // value绑定值的变化是由内部还是外部引起的
      changeFromInner: false,
      // 过滤处理方法
      innerFormatter: (value = null) => {
        return value;
      },
      showPassword: false
    };
  },
  created() {
    if (this.formatter) {
      this.innerFormatter = this.formatter;
    }
  },
  watch: {
    modelValue: {
      immediate: true,
      handler(newVal = null, oldVal = null) {
        if (this.changeFromInner || this.innerValue === newVal) {
          this.changeFromInner = false;
          return null;
        }
        this.innerValue = newVal;
        if (this.firstChange === false && this.changeFromInner === false) {
          this.valueChange(this.innerValue, true);
        } else {
          if (!this.firstChange)
            uni_modules_uviewPlus_libs_function_index.formValidate(this, "change");
        }
        this.firstChange = false;
        this.changeFromInner = false;
      }
    }
  },
  computed: {
    // 是否密码
    isPassword() {
      let ret = false;
      if (this.password) {
        ret = true;
      } else if (this.type == "password") {
        ret = true;
      } else {
        ret = false;
      }
      if (this.showPassword) {
        ret = false;
      }
      return ret;
    },
    // 是否显示清除控件
    isShowClear() {
      const _a = this, clearable = _a.clearable, readonly = _a.readonly, focused = _a.focused, innerValue = _a.innerValue, onlyClearableOnFocused = _a.onlyClearableOnFocused;
      if (!clearable || readonly) {
        return false;
      }
      if (onlyClearableOnFocused) {
        return !!focused && innerValue !== "";
      } else {
        return innerValue !== "";
      }
    },
    // 组件的类名
    inputClass() {
      let classes = [], _a = this, border = _a.border;
      _a.disabled;
      let shape = _a.shape;
      border === "surround" && (classes = classes.concat(["u-border", "u-input--radius"]));
      classes.push(`u-input--${shape}`);
      border === "bottom" && (classes = classes.concat([
        "u-border-bottom",
        "u-input--no-radius"
      ]));
      return classes.join(" ");
    },
    // 组件的样式
    wrapperStyle() {
      const style = new UTSJSONObject({});
      if (this.disabled) {
        style.backgroundColor = this.disabledColor;
      }
      if (this.border === "none") {
        style.padding = "0";
      } else {
        style.paddingTop = "6px";
        style.paddingBottom = "6px";
        style.paddingLeft = "9px";
        style.paddingRight = "9px";
      }
      return uni_modules_uviewPlus_libs_function_index.deepMerge(style, uni_modules_uviewPlus_libs_function_index.addStyle(this.customStyle));
    },
    // 输入框的样式
    inputStyle() {
      const style = new UTSJSONObject({
        color: this.color,
        fontSize: uni_modules_uviewPlus_libs_function_index.addUnit(this.fontSize),
        textAlign: this.inputAlign
      });
      return style;
    }
  },
  emits: ["update:modelValue", "focus", "blur", "change", "confirm", "clear", "keyboardheightchange", "nicknamereview"],
  methods: {
    // 在微信小程序中，不支持将函数当做props参数，故只能通过ref形式调用
    setFormatter(e = null) {
      this.innerFormatter = e;
    },
    // 当键盘输入时，触发input事件
    onInput(e = null) {
      let _a = (e.detail || new UTSJSONObject({})).value, value = _a == void 0 ? "" : _a;
      this.innerValue = value;
      this.$nextTick(() => {
        let formatValue = this.innerFormatter(value);
        this.innerValue = formatValue;
        this.valueChange(formatValue);
      });
    },
    // 输入框失去焦点时触发
    onBlur(event = null) {
      this.$emit("blur", event.detail.value);
      uni_modules_uviewPlus_libs_function_index.sleep(150).then(() => {
        this.focused = false;
      });
      uni_modules_uviewPlus_libs_function_index.formValidate(this, "blur");
    },
    // 输入框聚焦时触发
    onFocus(event = null) {
      this.focused = true;
      this.$emit("focus");
    },
    doFocus() {
      this.$refs["input-native"].focus();
    },
    doBlur() {
      this.$refs["input-native"].blur();
    },
    // 点击完成按钮时触发
    onConfirm(event = null) {
      this.$emit("confirm", this.innerValue);
    },
    // 键盘高度发生变化的时候触发此事件
    // 兼容性：微信小程序2.7.0+、App 3.1.0+
    onkeyboardheightchange(event = null) {
      this.$emit("keyboardheightchange", event);
    },
    onnicknamereview(event = null) {
      this.$emit("nicknamereview", event);
    },
    // 内容发生变化，进行处理
    valueChange(value = null, isOut = false) {
      if (this.clearInput) {
        this.innerValue = "";
        this.clearInput = false;
      }
      this.$nextTick(() => {
        if (!isOut || this.clearInput) {
          this.changeFromInner = true;
          this.$emit("change", value);
          this.$emit("update:modelValue", value);
        }
        uni_modules_uviewPlus_libs_function_index.formValidate(this, "change");
      });
    },
    // 点击清除控件
    onClear() {
      this.clearInput = true;
      this.innerValue = "";
      this.$nextTick(() => {
        this.valueChange("");
        this.$emit("clear");
      });
    },
    /**
     * 在安卓nvue上，事件无法冒泡
     * 在某些时间，我们希望监听u-from-item的点击事件，此时会导致点击u-form-item内的u-input后
     * 无法触发u-form-item的点击事件，这里通过手动调用u-form-item的方法进行触发
     */
    clickHandler() {
      if (this.disabled || this.readonly) {
        common_vendor.index.hideKeyboard();
      }
    }
  }
});
if (!Array) {
  const _easycom_up_icon2 = common_vendor.resolveComponent("up-icon");
  _easycom_up_icon2();
}
const _easycom_up_icon = () => "../u-icon/u-icon.js";
if (!Math) {
  _easycom_up_icon();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.prefixIcon || _ctx.$slots.prefix
  }, _ctx.prefixIcon || _ctx.$slots.prefix ? {
    b: common_vendor.p({
      name: _ctx.prefixIcon,
      size: "18",
      customStyle: _ctx.prefixIconStyle
    })
  } : {}, {
    c: common_vendor.sei("r0-df79975b", "input", "input-native"),
    d: common_vendor.s($options.inputStyle),
    e: $data.showPassword && "password" == _ctx.type ? "text" : _ctx.type,
    f: _ctx.focus,
    g: _ctx.cursor,
    h: $data.innerValue,
    i: _ctx.autoBlur,
    j: _ctx.disabled || _ctx.readonly,
    k: _ctx.maxlength,
    l: _ctx.placeholder,
    m: _ctx.placeholderStyle,
    n: _ctx.placeholderClass,
    o: _ctx.confirmType,
    p: _ctx.confirmHold,
    q: _ctx.holdKeyboard,
    r: _ctx.cursorColor,
    s: _ctx.cursorSpacing,
    t: _ctx.adjustPosition,
    v: _ctx.selectionEnd,
    w: _ctx.selectionStart,
    x: $options.isPassword,
    y: _ctx.ignoreCompositionEvent,
    z: common_vendor.o((...args) => $options.onInput && $options.onInput(...args)),
    A: common_vendor.o((...args) => $options.onBlur && $options.onBlur(...args)),
    B: common_vendor.o((...args) => $options.onFocus && $options.onFocus(...args)),
    C: common_vendor.o((...args) => $options.onConfirm && $options.onConfirm(...args)),
    D: common_vendor.o((...args) => $options.onkeyboardheightchange && $options.onkeyboardheightchange(...args)),
    E: common_vendor.o((...args) => $options.onnicknamereview && $options.onnicknamereview(...args)),
    F: common_vendor.o((...args) => $options.clickHandler && $options.clickHandler(...args)),
    G: $options.isShowClear
  }, $options.isShowClear ? {
    H: common_vendor.p({
      name: "close",
      size: "11",
      color: "#ffffff",
      customStyle: "line-height: 12px"
    }),
    I: common_vendor.o((...args) => $options.onClear && $options.onClear(...args))
  } : {}, {
    J: (_ctx.type == "password" || _ctx.password) && _ctx.passwordVisibilityToggle
  }, (_ctx.type == "password" || _ctx.password) && _ctx.passwordVisibilityToggle ? {
    K: common_vendor.o(($event) => $data.showPassword = !$data.showPassword),
    L: common_vendor.p({
      name: $data.showPassword ? "eye-off" : "eye-fill",
      size: "18"
    })
  } : {}, {
    M: _ctx.suffixIcon || _ctx.$slots.suffix
  }, _ctx.suffixIcon || _ctx.$slots.suffix ? {
    N: common_vendor.p({
      name: _ctx.suffixIcon,
      size: "18",
      customStyle: _ctx.suffixIconStyle
    })
  } : {}, {
    O: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
    P: common_vendor.n($options.inputClass),
    Q: common_vendor.s($options.wrapperStyle)
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-df79975b"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-input/u-input.js.map
