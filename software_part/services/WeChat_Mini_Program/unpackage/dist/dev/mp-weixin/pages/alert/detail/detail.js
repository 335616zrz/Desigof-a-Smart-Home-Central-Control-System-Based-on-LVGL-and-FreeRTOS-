"use strict";
const common_vendor = require("../../../common/vendor.js");
const stores_alert = require("../../../stores/alert.js");
if (!Array) {
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  _easycom_u_button_1();
}
const _easycom_u_button = () => "../../../uni_modules/uview-plus/components/u-button/u-button.js";
if (!Math) {
  (common_vendor.unref(AlertBadge) + _easycom_u_button)();
}
const AlertBadge = () => "../../../components/AlertBadge/AlertBadge.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "detail",
  setup(__props) {
    const store = stores_alert.useAlertStore();
    const id = common_vendor.ref("");
    const alert = common_vendor.computed(() => {
      return store.current;
    });
    common_vendor.onLoad((options = null) => {
      id.value = (options === null || options === void 0 ? null : options.id) || "";
    });
    common_vendor.onShow(() => {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!id.value)
          return Promise.resolve(null);
        try {
          yield store.fetchDetail(Number(id.value));
        } catch (e) {
          const status = e === null || e === void 0 ? null : e.statusCode;
          if (status === 403) {
            setTimeout(() => {
              common_vendor.index.navigateBack();
            }, 300);
            return Promise.resolve(null);
          }
          common_vendor.index.showToast({ title: "加载失败", icon: "none" });
        }
      });
    });
    function onAck() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        try {
          yield store.acknowledge(Number(id.value));
          common_vendor.index.showToast({ title: "已确认", icon: "none" });
          yield store.fetchDetail(Number(id.value));
        } catch (e) {
          common_vendor.index.showToast({ title: "操作失败", icon: "none" });
        }
      });
    }
    function onClose() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        try {
          yield store.close(Number(id.value));
          common_vendor.index.showToast({ title: "已关闭", icon: "none" });
          yield store.fetchDetail(Number(id.value));
        } catch (e) {
          common_vendor.index.showToast({ title: "操作失败", icon: "none" });
        }
      });
    }
    return (_ctx, _cache) => {
      "raw js";
      var _a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m;
      const __returned__ = common_vendor.e({
        a: common_vendor.p({
          level: (_a = common_vendor.unref(alert)) == null ? void 0 : _a.level,
          status: (_b = common_vendor.unref(alert)) == null ? void 0 : _b.status
        }),
        b: common_vendor.t(((_c = common_vendor.unref(alert)) == null ? void 0 : _c.id) || "-"),
        c: common_vendor.t(((_d = common_vendor.unref(alert)) == null ? void 0 : _d.message) || "-"),
        d: common_vendor.t(((_e = common_vendor.unref(alert)) == null ? void 0 : _e.deviceId) || "-"),
        e: common_vendor.t(((_f = common_vendor.unref(alert)) == null ? void 0 : _f.ruleId) || "-"),
        f: common_vendor.t(((_g = common_vendor.unref(alert)) == null ? void 0 : _g.createdAt) || "-"),
        g: common_vendor.t(((_h = common_vendor.unref(alert)) == null ? void 0 : _h.acknowledgedAt) || "-"),
        h: common_vendor.t(((_i = common_vendor.unref(alert)) == null ? void 0 : _i.closedAt) || "-"),
        i: ((_j = common_vendor.unref(alert)) == null ? void 0 : _j.status) === "open"
      }, ((_k = common_vendor.unref(alert)) == null ? void 0 : _k.status) === "open" ? {
        j: common_vendor.o(onAck),
        k: common_vendor.p({
          type: "primary"
        })
      } : {}, {
        l: ((_l = common_vendor.unref(alert)) == null ? void 0 : _l.status) !== "closed"
      }, ((_m = common_vendor.unref(alert)) == null ? void 0 : _m.status) !== "closed" ? {
        m: common_vendor.o(onClose),
        n: common_vendor.p({
          type: "error",
          plain: true
        })
      } : {}, {
        o: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/alert/detail/detail.js.map
