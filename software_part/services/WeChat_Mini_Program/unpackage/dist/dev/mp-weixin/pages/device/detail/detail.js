"use strict";
const common_vendor = require("../../../common/vendor.js");
const utils_permission = require("../../../utils/permission.js");
const stores_auth = require("../../../stores/auth.js");
const stores_device = require("../../../stores/device.js");
const utils_validator = require("../../../utils/validator.js");
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
  (common_vendor.unref(StatusBadge) + _easycom_u_input + _easycom_u_form_item + _easycom_u_form + _easycom_u_button)();
}
const StatusBadge = () => "../../../components/StatusBadge/StatusBadge.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "detail",
  setup(__props) {
    const auth = stores_auth.useAuthStore();
    const store = stores_device.useDeviceStore();
    const deviceId = common_vendor.ref("");
    const device = common_vendor.computed(() => {
      return store.current;
    });
    const saving = common_vendor.ref(false);
    const editForm = common_vendor.reactive(new UTSJSONObject({
      name: "",
      firmwareVersion: ""
    }));
    const canDelete = common_vendor.computed(() => {
      return utils_permission.isAdmin(auth.user);
    });
    common_vendor.onLoad((options = null) => {
      deviceId.value = (options === null || options === void 0 ? null : options.deviceId) || "";
    });
    common_vendor.onShow(() => {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!deviceId.value)
          return Promise.resolve(null);
        const data = yield store.fetchDetail(deviceId.value);
        editForm.name = (data === null || data === void 0 ? null : data.name) || "";
        editForm.firmwareVersion = (data === null || data === void 0 ? null : data.firmwareVersion) || "";
      });
    });
    function onSave() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (saving.value)
          return Promise.resolve(null);
        if (!utils_validator.required(editForm.name)) {
          common_vendor.index.showToast({ title: "名称不能为空", icon: "none" });
          return Promise.resolve(null);
        }
        saving.value = true;
        try {
          yield store.update(deviceId.value, new UTSJSONObject({
            name: editForm.name,
            firmwareVersion: editForm.firmwareVersion
          }));
          common_vendor.index.showToast({ title: "已保存", icon: "none" });
          yield store.fetchDetail(deviceId.value);
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
              yield store.remove(deviceId.value);
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
      var _a, _b, _c, _d;
      const __returned__ = common_vendor.e({
        a: common_vendor.t(((_a = common_vendor.unref(device)) == null ? void 0 : _a.name) || "-"),
        b: common_vendor.p({
          status: ((_b = common_vendor.unref(device)) == null ? void 0 : _b.status) || ""
        }),
        c: common_vendor.t(((_c = common_vendor.unref(device)) == null ? void 0 : _c.deviceId) || "-"),
        d: common_vendor.t(((_d = common_vendor.unref(device)) == null ? void 0 : _d.lastSeenAt) || "-"),
        e: common_vendor.o(($event) => {
          return common_vendor.unref(editForm).name = $event;
        }),
        f: common_vendor.p({
          placeholder: "设备名称",
          clearable: true,
          modelValue: common_vendor.unref(editForm).name
        }),
        g: common_vendor.p({
          label: "名称",
          labelWidth: "72"
        }),
        h: common_vendor.o(($event) => {
          return common_vendor.unref(editForm).firmwareVersion = $event;
        }),
        i: common_vendor.p({
          placeholder: "固件版本",
          clearable: true,
          modelValue: common_vendor.unref(editForm).firmwareVersion
        }),
        j: common_vendor.p({
          label: "固件",
          labelWidth: "72"
        }),
        k: common_vendor.p({
          model: common_vendor.unref(editForm)
        }),
        l: common_vendor.o(onSave),
        m: common_vendor.p({
          type: "primary",
          loading: common_vendor.unref(saving),
          disabled: common_vendor.unref(saving)
        }),
        n: common_vendor.unref(canDelete)
      }, common_vendor.unref(canDelete) ? {
        o: common_vendor.o(onDelete),
        p: common_vendor.p({
          type: "error",
          plain: true
        })
      } : {}, {
        q: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/device/detail/detail.js.map
