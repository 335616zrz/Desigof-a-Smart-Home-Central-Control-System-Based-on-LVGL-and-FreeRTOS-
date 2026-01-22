"use strict";
const common_vendor = require("../../../common/vendor.js");
const stores_alertRule = require("../../../stores/alertRule.js");
if (!Array) {
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_empty_1 = common_vendor.resolveComponent("u-empty");
  (_easycom_u_button_1 + _easycom_u_empty_1)();
}
const _easycom_u_button = () => "../../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_empty = () => "../../../uni_modules/uview-plus/components/u-empty/u-empty.js";
if (!Math) {
  (_easycom_u_button + _easycom_u_empty)();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "list",
  setup(__props) {
    const store = stores_alertRule.useAlertRuleStore();
    const loading = common_vendor.computed(() => {
      return Boolean(store.loading);
    });
    const rules = common_vendor.computed(() => {
      return store.userRules || [];
    });
    common_vendor.onShow(() => {
      refresh();
    });
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        yield store.fetchUserRules();
      });
    }
    function goCreate() {
      common_vendor.index.navigateTo({ url: "/pages/alertRule/edit/edit" });
    }
    function goEdit(rule = null) {
      common_vendor.index.navigateTo({ url: `/pages/alertRule/edit/edit?id=${rule === null || rule === void 0 ? null : rule.id}` });
    }
    function onToggle(rule = null, e = null) {
      var _a;
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        const enabled = Boolean((_a = e === null || e === void 0 ? null : e.detail) === null || _a === void 0 ? null : _a.value);
        try {
          yield store.toggle(rule.id, enabled);
        } catch (err) {
          common_vendor.index.showToast({ title: "切换失败", icon: "none" });
        }
      });
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.o(goCreate),
        b: common_vendor.p({
          type: "primary",
          size: "mini"
        }),
        c: common_vendor.o(refresh),
        d: common_vendor.p({
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        e: common_vendor.f(common_vendor.unref(rules), (r, k0, i0) => {
          return {
            a: common_vendor.t(r.name),
            b: Boolean(r.enabled),
            c: common_vendor.o((e) => {
              return onToggle(r, e);
            }, r.id),
            d: common_vendor.t(r.deviceId || "全部"),
            e: common_vendor.t(r.metric),
            f: common_vendor.t(r.operator),
            g: common_vendor.t(r.threshold),
            h: common_vendor.t(r.level),
            i: r.id,
            j: common_vendor.o(($event) => {
              return goEdit(r);
            }, r.id)
          };
        }),
        f: !common_vendor.unref(loading) && common_vendor.unref(rules).length === 0
      }, !common_vendor.unref(loading) && common_vendor.unref(rules).length === 0 ? {
        g: common_vendor.p({
          text: "暂无规则",
          mode: "list"
        })
      } : {}, {
        h: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/alertRule/list/list.js.map
