"use strict";
const common_vendor = require("../../common/vendor.js");
if (!Math) {
  common_vendor.unref(StatusBadge)();
}
const StatusBadge = () => "../StatusBadge/StatusBadge.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "DeviceCard",
  props: {
    device: {
      type: Object,
      default: null
    }
  },
  emits: ["click"],
  setup(__props, _a) {
    var __emit = _a.emit;
    const props = __props;
    const emit = __emit;
    function onClick() {
      emit("click", props.device);
    }
    return (_ctx, _cache) => {
      "raw js";
      var _a2, _b, _c, _d;
      const __returned__ = {
        a: common_vendor.t(((_a2 = __props.device) == null ? void 0 : _a2.name) || "-"),
        b: common_vendor.p({
          status: ((_b = __props.device) == null ? void 0 : _b.status) || ""
        }),
        c: common_vendor.t(((_c = __props.device) == null ? void 0 : _c.deviceId) || "-"),
        d: common_vendor.t(((_d = __props.device) == null ? void 0 : _d.firmwareVersion) || "-"),
        e: common_vendor.sei(common_vendor.gei(_ctx, ""), "view"),
        f: common_vendor.o(onClick)
      };
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/DeviceCard/DeviceCard.js.map
