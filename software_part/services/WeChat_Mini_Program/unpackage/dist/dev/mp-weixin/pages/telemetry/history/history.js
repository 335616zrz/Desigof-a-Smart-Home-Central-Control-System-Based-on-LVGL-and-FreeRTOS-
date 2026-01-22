"use strict";
const common_vendor = require("../../../common/vendor.js");
const stores_device = require("../../../stores/device.js");
const stores_telemetry = require("../../../stores/telemetry.js");
if (!Array) {
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_input_1 = common_vendor.resolveComponent("u-input");
  const _easycom_u_empty_1 = common_vendor.resolveComponent("u-empty");
  (_easycom_u_button_1 + _easycom_u_input_1 + _easycom_u_empty_1)();
}
const _easycom_u_button = () => "../../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_input = () => "../../../uni_modules/uview-plus/components/u-input/u-input.js";
const _easycom_u_empty = () => "../../../uni_modules/uview-plus/components/u-empty/u-empty.js";
if (!Math) {
  (_easycom_u_button + _easycom_u_input + common_vendor.unref(TelemetryChart) + _easycom_u_empty)();
}
const TelemetryChart = () => "../../../components/TelemetryChart/TelemetryChart.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "history",
  setup(__props) {
    const deviceStore = stores_device.useDeviceStore();
    const store = stores_telemetry.useTelemetryStore();
    const selectedDeviceId = common_vendor.ref("");
    const field = common_vendor.ref("temperature");
    const loading = common_vendor.computed(() => {
      return Boolean(store.loading);
    });
    const records = common_vendor.computed(() => {
      return store.history || [];
    });
    const deviceLabel = common_vendor.computed(() => {
      if (!selectedDeviceId.value)
        return "全部设备";
      const d = (deviceStore.items || []).find((x = null) => {
        return (x === null || x === void 0 ? null : x.deviceId) === selectedDeviceId.value;
      });
      return (d === null || d === void 0 ? null : d.name) ? `${d.name}` : selectedDeviceId.value;
    });
    common_vendor.onShow(() => {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!deviceStore.items || deviceStore.items.length === 0) {
          yield deviceStore.fetchList();
        }
        yield refresh();
      });
    });
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        yield store.fetchHistory(new UTSJSONObject({
          deviceId: selectedDeviceId.value,
          pageSize: 100,
          page: 1
        }));
      });
    }
    function pickDevice() {
      const items = [new UTSJSONObject({ id: "", name: "全部设备" })];
      (deviceStore.items || []).forEach((d = null) => {
        items.push(new UTSJSONObject({ id: (d === null || d === void 0 ? null : d.deviceId) || "", name: (d === null || d === void 0 ? null : d.name) || (d === null || d === void 0 ? null : d.deviceId) || "" }));
      });
      common_vendor.index.showActionSheet({
        itemList: items.map((x = null) => {
          return x.name;
        }),
        success: (res = null) => {
          var _a;
          const idx = (_a = res === null || res === void 0 ? null : res.tapIndex) !== null && _a !== void 0 ? _a : -1;
          if (idx < 0)
            return null;
          selectedDeviceId.value = items[idx].id;
          refresh();
        }
      });
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(common_vendor.unref(deviceLabel)),
        b: common_vendor.o(pickDevice),
        c: common_vendor.p({
          size: "mini"
        }),
        d: common_vendor.o(($event) => {
          return common_vendor.isRef(field) ? field.value = $event : null;
        }),
        e: common_vendor.p({
          placeholder: "字段名(如 temperature)",
          clearable: true,
          modelValue: common_vendor.unref(field)
        }),
        f: common_vendor.o(refresh),
        g: common_vendor.p({
          type: "primary",
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        h: common_vendor.p({
          records: common_vendor.unref(records),
          field: common_vendor.unref(field),
          title: "趋势(简易)"
        }),
        i: common_vendor.f(common_vendor.unref(records), (r, k0, i0) => {
          return {
            a: common_vendor.t(r.reportedAt || "-"),
            b: common_vendor.t(r.payload || ""),
            c: r.id
          };
        }),
        j: !common_vendor.unref(loading) && common_vendor.unref(records).length === 0
      }, !common_vendor.unref(loading) && common_vendor.unref(records).length === 0 ? {
        k: common_vendor.p({
          text: "暂无遥测数据",
          mode: "data"
        })
      } : {}, {
        l: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/telemetry/history/history.js.map
