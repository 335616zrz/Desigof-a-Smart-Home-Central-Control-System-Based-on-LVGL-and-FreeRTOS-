"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uForm_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_util_asyncValidator = require("../../libs/util/async-validator.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const uni_modules_uviewPlus_libs_function_test = require("../../libs/function/test.js");
uni_modules_uviewPlus_libs_util_asyncValidator.Schema.warning = function() {
};
const _sfc_main = common_vendor.defineComponent({
  name: "up-form",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uForm_props.props],
  provide() {
    return {
      uForm: this
    };
  },
  data() {
    return {
      formRules: new UTSJSONObject({}),
      // 规则校验器
      validator: new UTSJSONObject({}),
      // 原始的model快照，用于resetFields方法重置表单时使用
      originalModel: null
    };
  },
  watch: {
    // 监听规则的变化
    rules: {
      immediate: true,
      handler(n = null) {
        this.setRules(n);
      }
    },
    // 监听属性的变化，通知子组件u-form-item重新获取信息
    propsChange(n = null) {
      var _a;
      if ((_a = this.children) === null || _a === void 0 ? null : _a.length) {
        this.children.map((child = null) => {
          typeof child.updateParentData == "function" && child.updateParentData();
        });
      }
    },
    // 监听model的初始值作为重置表单的快照
    model: {
      immediate: true,
      handler(n = null) {
        if (!this.originalModel) {
          this.originalModel = uni_modules_uviewPlus_libs_function_index.deepClone(n);
        }
      }
    }
  },
  computed: {
    propsChange() {
      return [
        this.errorType,
        this.borderBottom,
        this.labelPosition,
        this.labelWidth,
        this.labelAlign,
        this.labelStyle
      ];
    }
  },
  created() {
    this.children = [];
  },
  methods: {
    // 手动设置校验的规则，如果规则中有函数的话，微信小程序中会过滤掉，所以只能手动调用设置规则
    setRules(rules = null) {
      if (Object.keys(rules).length === 0)
        return null;
      if (Object.keys(this.model).length === 0) {
        uni_modules_uviewPlus_libs_function_index.error("设置rules，model必须设置！如果已经设置，请刷新页面。");
        return null;
      }
      this.formRules = rules;
      this.validator = new uni_modules_uviewPlus_libs_util_asyncValidator.Schema(rules);
    },
    // 清空所有u-form-item组件的内容，本质上是调用了u-form-item组件中的resetField()方法
    resetFields() {
      this.resetModel();
    },
    // 重置model为初始值的快照
    resetModel(obj = null) {
      this.children.map((child = null) => {
        const prop = child === null || child === void 0 ? null : child.prop;
        const value = uni_modules_uviewPlus_libs_function_index.getProperty(this.originalModel, prop);
        uni_modules_uviewPlus_libs_function_index.setProperty(this.model, prop, value);
      });
    },
    // 清空校验结果
    clearValidate(props2 = null) {
      props2 = [].concat(props2);
      this.children.map((child = null) => {
        if (props2[0] === void 0 || props2.includes(child.prop)) {
          child.message = null;
        }
      });
    },
    // 对部分表单字段进行校验
    validateField(value = null, callback = null, event = null, options = null) {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        this.$nextTick(() => {
          const errorsRes = [];
          value = [].concat(value);
          let promises = this.children.map((child = null) => {
            return new Promise((resolve, reject) => {
              const childErrors = [];
              if (value.includes(child.prop)) {
                const propertyVal = uni_modules_uviewPlus_libs_function_index.getProperty(this.model, child.prop);
                const propertyChain = child.prop.split(".");
                const propertyName = propertyChain[propertyChain.length - 1];
                let rule = [];
                if (child.itemRules && child.itemRules.length > 0) {
                  rule = child.itemRules;
                } else {
                  rule = this.formRules[child.prop];
                }
                if (!rule) {
                  resolve();
                  return null;
                }
                const rules = [].concat(rule);
                if (!rules.length) {
                  resolve();
                }
                for (let i = 0; i < rules.length; i++) {
                  const ruleItem = rules[i];
                  const trigger = [].concat(ruleItem === null || ruleItem === void 0 ? null : ruleItem.trigger);
                  if (event && !trigger.includes(event)) {
                    resolve();
                    continue;
                  }
                  const validator = new uni_modules_uviewPlus_libs_util_asyncValidator.Schema(new UTSJSONObject({
                    [propertyName]: ruleItem
                  }));
                  validator.validate(new UTSJSONObject({
                    [propertyName]: propertyVal
                  }), (errors = null, fields = null) => {
                    var _a;
                    if (uni_modules_uviewPlus_libs_function_test.test.array(errors)) {
                      errors.forEach((element = null) => {
                        element.prop = child.prop;
                      });
                      errorsRes.push(...errors);
                      childErrors.push(...errors);
                    }
                    if (!options || (options === null || options === void 0 ? null : options.showErrorMsg) == true) {
                      child.message = ((_a = childErrors[0]) === null || _a === void 0 ? null : _a.message) ? childErrors[0].message : null;
                    }
                    if (i == rules.length - 1) {
                      resolve(errorsRes);
                    }
                  });
                }
              } else {
                resolve({});
              }
            });
          });
          Promise.all(promises).then((results = null) => {
            typeof callback === "function" && callback(errorsRes);
          }).catch((error2 = null) => {
            common_vendor.index.__f__("error", "at uni_modules/uview-plus/components/u-form/u-form.vue:218", "An error occurred:", error2);
          });
        });
      });
    },
    /**
     * 校验全部数据
     * @param {Object} options
     * @param {Boolean} options.showErrorMsg -是否显示校验信息，
     */
    validate(options = null) {
      if (Object.keys(this.formRules).length === 0) {
        uni_modules_uviewPlus_libs_function_index.error("未设置rules，请看文档说明！如果已经设置，请刷新页面。");
        return null;
      }
      return new Promise((resolve, reject) => {
        this.$nextTick(() => {
          const formItemProps = this.children.map((item = null) => {
            return item.prop;
          });
          this.validateField(formItemProps, (errors = null) => {
            if (errors.length) {
              this.errorType === "toast" && uni_modules_uviewPlus_libs_function_index.toast(errors[0].message);
              reject(errors);
            } else {
              resolve(true);
            }
          }, null, options);
        });
      });
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return {
    a: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
  };
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-form/u-form.js.map
