"use strict";
const common_vendor = require("../../../common/vendor.js");
const utils_validator = require("../../../utils/validator.js");
const stores_device = require("../../../stores/device.js");
if (!Array) {
  const _easycom_u_search_1 = common_vendor.resolveComponent("u-search");
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_empty_1 = common_vendor.resolveComponent("u-empty");
  const _easycom_u_input_1 = common_vendor.resolveComponent("u-input");
  const _easycom_u_form_item_1 = common_vendor.resolveComponent("u-form-item");
  const _easycom_u_form_1 = common_vendor.resolveComponent("u-form");
  const _easycom_u_modal_1 = common_vendor.resolveComponent("u-modal");
  (_easycom_u_search_1 + _easycom_u_button_1 + _easycom_u_empty_1 + _easycom_u_input_1 + _easycom_u_form_item_1 + _easycom_u_form_1 + _easycom_u_modal_1)();
}
const _easycom_u_search = () => "../../../uni_modules/uview-plus/components/u-search/u-search.js";
const _easycom_u_button = () => "../../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_empty = () => "../../../uni_modules/uview-plus/components/u-empty/u-empty.js";
const _easycom_u_input = () => "../../../uni_modules/uview-plus/components/u-input/u-input.js";
const _easycom_u_form_item = () => "../../../uni_modules/uview-plus/components/u-form-item/u-form-item.js";
const _easycom_u_form = () => "../../../uni_modules/uview-plus/components/u-form/u-form.js";
const _easycom_u_modal = () => "../../../uni_modules/uview-plus/components/u-modal/u-modal.js";
if (!Math) {
  (_easycom_u_search + _easycom_u_button + common_vendor.unref(DeviceCard) + _easycom_u_empty + _easycom_u_input + _easycom_u_form_item + _easycom_u_form + _easycom_u_modal)();
}
const DeviceCard = () => "../../../components/DeviceCard/DeviceCard.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "list",
  setup(__props) {
    const store = stores_device.useDeviceStore();
    const keyword = common_vendor.ref("");
    const addVisible = common_vendor.ref(false);
    const credVisible = common_vendor.ref(false);
    const loading = common_vendor.computed(() => {
      return Boolean(store.loading);
    });
    const addForm = common_vendor.reactive(new UTSJSONObject({
      name: "",
      firmwareVersion: ""
    }));
    const credential = common_vendor.reactive(new UTSJSONObject({
      deviceId: "",
      credentialKey: "",
      credentialSecret: "",
      expiresAt: ""
    }));
    const filtered = common_vendor.computed(() => {
      const items = store.items || [];
      const q = String(keyword.value || "").trim().toLowerCase();
      if (!q)
        return items;
      return items.filter((d = null) => {
        const name = String((d === null || d === void 0 ? null : d.name) || "").toLowerCase();
        const id = String((d === null || d === void 0 ? null : d.deviceId) || "").toLowerCase();
        return name.indexOf(q) !== -1 || id.indexOf(q) !== -1;
      });
    });
    common_vendor.onShow(() => {
      store.fetchList();
    });
    function onCardClick(device = null) {
      const id = (device === null || device === void 0 ? null : device.deviceId) || "";
      if (!id)
        return null;
      common_vendor.index.navigateTo({ url: `/pages/device/detail/detail?deviceId=${id}` });
    }
    function openAdd() {
      addForm.name = "";
      addForm.firmwareVersion = "";
      addVisible.value = true;
    }
    function confirmAdd() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!utils_validator.required(addForm.name)) {
          common_vendor.index.showToast({ title: "请输入设备名称", icon: "none" });
          return Promise.resolve(null);
        }
        try {
          const resp = yield store.provision("auto", new UTSJSONObject({
            name: addForm.name,
            firmwareVersion: addForm.firmwareVersion
          }));
          credential.deviceId = (resp === null || resp === void 0 ? null : resp.deviceId) || "";
          credential.credentialKey = (resp === null || resp === void 0 ? null : resp.credentialKey) || "";
          credential.credentialSecret = (resp === null || resp === void 0 ? null : resp.credentialSecret) || "";
          credential.expiresAt = (resp === null || resp === void 0 ? null : resp.expiresAt) || "";
          credVisible.value = true;
          yield store.fetchList();
        } catch (e) {
          common_vendor.index.showToast({ title: "添加设备失败", icon: "none" });
        }
      });
    }
    function copyCredential() {
      const text = UTS.JSON.stringify(new UTSJSONObject({
        deviceId: credential.deviceId,
        credentialKey: credential.credentialKey,
        credentialSecret: credential.credentialSecret,
        expiresAt: credential.expiresAt
      }));
      common_vendor.index.setClipboardData({
        data: text,
        success: () => {
          return common_vendor.index.showToast({ title: "已复制", icon: "none" });
        }
      });
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.o(($event) => {
          return common_vendor.isRef(keyword) ? keyword.value = $event : null;
        }),
        b: common_vendor.p({
          placeholder: "搜索设备名称/ID",
          showAction: false,
          modelValue: common_vendor.unref(keyword)
        }),
        c: common_vendor.o(openAdd),
        d: common_vendor.p({
          type: "primary",
          size: "mini"
        }),
        e: common_vendor.f(common_vendor.unref(filtered), (item, k0, i0) => {
          return {
            a: item.deviceId,
            b: common_vendor.o(onCardClick, item.deviceId),
            c: "3041c3ab-2-" + i0,
            d: common_vendor.p({
              device: item
            })
          };
        }),
        f: !common_vendor.unref(loading) && common_vendor.unref(filtered).length === 0
      }, !common_vendor.unref(loading) && common_vendor.unref(filtered).length === 0 ? {
        g: common_vendor.p({
          text: "暂无设备",
          mode: "list"
        })
      } : {}, {
        h: common_vendor.o(($event) => {
          return common_vendor.unref(addForm).name = $event;
        }),
        i: common_vendor.p({
          placeholder: "请输入设备名称",
          clearable: true,
          modelValue: common_vendor.unref(addForm).name
        }),
        j: common_vendor.p({
          label: "名称",
          labelWidth: "72"
        }),
        k: common_vendor.o(($event) => {
          return common_vendor.unref(addForm).firmwareVersion = $event;
        }),
        l: common_vendor.p({
          placeholder: "可选",
          clearable: true,
          modelValue: common_vendor.unref(addForm).firmwareVersion
        }),
        m: common_vendor.p({
          label: "固件",
          labelWidth: "72"
        }),
        n: common_vendor.p({
          model: common_vendor.unref(addForm)
        }),
        o: common_vendor.o(confirmAdd),
        p: common_vendor.o(($event) => {
          return common_vendor.isRef(addVisible) ? addVisible.value = $event : null;
        }),
        q: common_vendor.p({
          title: "添加设备",
          showCancelButton: true,
          show: common_vendor.unref(addVisible)
        }),
        r: common_vendor.t(common_vendor.unref(credential).deviceId),
        s: common_vendor.t(common_vendor.unref(credential).credentialKey),
        t: common_vendor.t(common_vendor.unref(credential).credentialSecret),
        v: common_vendor.unref(credential).expiresAt
      }, common_vendor.unref(credential).expiresAt ? {
        w: common_vendor.t(common_vendor.unref(credential).expiresAt)
      } : {}, {
        x: common_vendor.o(copyCredential),
        y: common_vendor.p({
          size: "mini"
        }),
        z: common_vendor.o(($event) => {
          return common_vendor.isRef(credVisible) ? credVisible.value = $event : null;
        }),
        A: common_vendor.p({
          title: "设备凭证",
          confirmText: "我已保存",
          show: common_vendor.unref(credVisible)
        }),
        B: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/device/list/list.js.map
