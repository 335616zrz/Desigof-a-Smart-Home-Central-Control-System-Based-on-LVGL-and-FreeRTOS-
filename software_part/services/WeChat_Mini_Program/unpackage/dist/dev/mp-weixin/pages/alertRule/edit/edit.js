"use strict";
const common_vendor = require("../../../common/vendor.js");
const utils_validator = require("../../../utils/validator.js");
const stores_alertRule = require("../../../stores/alertRule.js");
if (!Array) {
  const _easycom_u_input_1 = common_vendor.resolveComponent("u-input");
  const _easycom_u_form_item_1 = common_vendor.resolveComponent("u-form-item");
  const _easycom_u_form_1 = common_vendor.resolveComponent("u-form");
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  (_easycom_u_input_1 + _easycom_u_form_item_1 + _easycom_u_form_1 + _easycom_u_button_1)();
}
const _easycom_u_input = () => "../../../uni_modules/uview-plus/components/u-input/u-input.js";
const _easycom_u_form_item = () => "../../../uni_modules/uview-plus/components/u-form-item/u-form-item.js";
const _easycom_u_form = () => "../../../uni_modules/uview-plus/components/u-form/u-form.js";
const _easycom_u_button = () => "../../../uni_modules/uview-plus/components/u-button/u-button.js";
if (!Math) {
  (_easycom_u_input + _easycom_u_form_item + _easycom_u_form + _easycom_u_button)();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "edit",
  setup(__props) {
    const store = stores_alertRule.useAlertRuleStore();
    const ruleId = common_vendor.ref("");
    const saving = common_vendor.ref(false);
    const form = common_vendor.reactive(new UTSJSONObject({
      name: "",
      deviceId: "",
      metric: "",
      operator: ">",
      threshold: "",
      level: "warning",
      enabled: true
    }));
    common_vendor.onLoad((options = null) => {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        var _a;
        ruleId.value = (options === null || options === void 0 ? null : options.id) || "";
        if (!ruleId.value)
          return Promise.resolve(null);
        const r = yield store.fetchDetail(Number(ruleId.value));
        if (!r)
          return Promise.resolve(null);
        form.name = r.name || "";
        form.deviceId = r.deviceId || "";
        form.metric = r.metric || "";
        form.operator = r.operator || ">";
        form.threshold = String((_a = r.threshold) !== null && _a !== void 0 ? _a : "");
        form.level = r.level || "warning";
        form.enabled = Boolean(r.enabled);
      });
    });
    function onSave() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (saving.value)
          return Promise.resolve(null);
        if (!utils_validator.required(form.name) || !utils_validator.required(form.metric) || !utils_validator.required(form.operator) || !utils_validator.required(form.threshold) || !utils_validator.required(form.level)) {
          common_vendor.index.showToast({ title: "请完整填写表单", icon: "none" });
          return Promise.resolve(null);
        }
        const thresholdNum = Number(form.threshold);
        if (isNaN(thresholdNum)) {
          common_vendor.index.showToast({ title: "阈值必须是数字", icon: "none" });
          return Promise.resolve(null);
        }
        saving.value = true;
        try {
          const payload = new UTSJSONObject({
            name: form.name,
            deviceId: form.deviceId || null,
            metric: form.metric,
            operator: form.operator,
            threshold: thresholdNum,
            level: form.level,
            enabled: Boolean(form.enabled)
          });
          if (ruleId.value) {
            yield store.update(Number(ruleId.value), payload);
          } else {
            yield store.create(payload);
          }
          common_vendor.index.showToast({ title: "已保存", icon: "none" });
          common_vendor.index.navigateBack();
        } catch (e) {
          common_vendor.index.showToast({ title: "保存失败", icon: "none" });
        } finally {
          saving.value = false;
        }
      });
    }
    function onDelete() {
      common_vendor.index.showModal(new UTSJSONObject({
        title: "确认删除",
        content: "删除后不可恢复，确定继续？",
        success: (res = null) => {
          return common_vendor.__awaiter(this, void 0, void 0, function* () {
            if (!(res === null || res === void 0 ? null : res.confirm))
              return Promise.resolve(null);
            try {
              yield store.remove(Number(ruleId.value));
              common_vendor.index.showToast({ title: "已删除", icon: "none" });
              common_vendor.index.navigateBack();
            } catch (e) {
              common_vendor.index.showToast({ title: "删除失败", icon: "none" });
            }
          });
        }
      }));
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.o(($event) => {
          return common_vendor.unref(form).name = $event;
        }),
        b: common_vendor.p({
          placeholder: "规则名称",
          clearable: true,
          modelValue: common_vendor.unref(form).name
        }),
        c: common_vendor.p({
          label: "名称",
          labelWidth: "80"
        }),
        d: common_vendor.o(($event) => {
          return common_vendor.unref(form).deviceId = $event;
        }),
        e: common_vendor.p({
          placeholder: "留空表示全部设备",
          clearable: true,
          modelValue: common_vendor.unref(form).deviceId
        }),
        f: common_vendor.p({
          label: "设备ID",
          labelWidth: "80"
        }),
        g: common_vendor.o(($event) => {
          return common_vendor.unref(form).metric = $event;
        }),
        h: common_vendor.p({
          placeholder: "temperature / humidity ...",
          clearable: true,
          modelValue: common_vendor.unref(form).metric
        }),
        i: common_vendor.p({
          label: "指标",
          labelWidth: "80"
        }),
        j: common_vendor.o(($event) => {
          return common_vendor.unref(form).operator = $event;
        }),
        k: common_vendor.p({
          placeholder: ">, <, >=, <=, ==, !=",
          clearable: true,
          modelValue: common_vendor.unref(form).operator
        }),
        l: common_vendor.p({
          label: "操作符",
          labelWidth: "80"
        }),
        m: common_vendor.o(($event) => {
          return common_vendor.unref(form).threshold = $event;
        }),
        n: common_vendor.p({
          placeholder: "数字",
          clearable: true,
          modelValue: common_vendor.unref(form).threshold
        }),
        o: common_vendor.p({
          label: "阈值",
          labelWidth: "80"
        }),
        p: common_vendor.o(($event) => {
          return common_vendor.unref(form).level = $event;
        }),
        q: common_vendor.p({
          placeholder: "critical / warning / info",
          clearable: true,
          modelValue: common_vendor.unref(form).level
        }),
        r: common_vendor.p({
          label: "级别",
          labelWidth: "80"
        }),
        s: Boolean(common_vendor.unref(form).enabled),
        t: common_vendor.o((e) => {
          var _a;
          return common_vendor.unref(form).enabled = Boolean((_a = e == null ? void 0 : e.detail) == null ? void 0 : _a.value);
        }),
        v: common_vendor.p({
          label: "启用",
          labelWidth: "80"
        }),
        w: common_vendor.p({
          model: common_vendor.unref(form)
        }),
        x: common_vendor.o(onSave),
        y: common_vendor.p({
          type: "primary",
          loading: common_vendor.unref(saving),
          disabled: common_vendor.unref(saving)
        }),
        z: common_vendor.unref(ruleId)
      }, common_vendor.unref(ruleId) ? {
        A: common_vendor.o(onDelete),
        B: common_vendor.p({
          type: "error",
          plain: true
        })
      } : {}, {
        C: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/alertRule/edit/edit.js.map
