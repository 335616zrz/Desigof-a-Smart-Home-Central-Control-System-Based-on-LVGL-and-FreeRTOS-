"use strict";
const common_vendor = require("../../../common/vendor.js");
const stores_alert = require("../../../stores/alert.js");
if (!Array) {
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_empty_1 = common_vendor.resolveComponent("u-empty");
  (_easycom_u_button_1 + _easycom_u_empty_1)();
}
const _easycom_u_button = () => "../../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_empty = () => "../../../uni_modules/uview-plus/components/u-empty/u-empty.js";
if (!Math) {
  (_easycom_u_button + common_vendor.unref(AlertBadge) + _easycom_u_empty)();
}
const AlertBadge = () => "../../../components/AlertBadge/AlertBadge.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "list",
  setup(__props) {
    const store = stores_alert.useAlertStore();
    const status = common_vendor.ref("");
    const level = common_vendor.ref("");
    const items = common_vendor.computed(() => {
      return store.items || [];
    });
    const loading = common_vendor.computed(() => {
      return Boolean(store.loading);
    });
    const statusLabel = common_vendor.computed(() => {
      if (!status.value)
        return "状态:全部";
      if (status.value === "open")
        return "状态:未处理";
      if (status.value === "ack")
        return "状态:已确认";
      if (status.value === "closed")
        return "状态:已关闭";
      return "状态:" + status.value;
    });
    const levelLabel = common_vendor.computed(() => {
      if (!level.value)
        return "级别:全部";
      if (level.value === "critical")
        return "级别:危险";
      if (level.value === "warning")
        return "级别:警告";
      if (level.value === "info")
        return "级别:提示";
      return "级别:" + level.value;
    });
    common_vendor.onShow(() => {
      refresh();
    });
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        yield store.fetchList(new UTSJSONObject({
          status: status.value || void 0,
          level: level.value || void 0,
          page: 0,
          pageSize: 20
        }));
      });
    }
    function pickStatus() {
      const opts = [
        new UTSJSONObject({ id: "", name: "全部" }),
        new UTSJSONObject({ id: "open", name: "未处理" }),
        new UTSJSONObject({ id: "ack", name: "已确认" }),
        new UTSJSONObject({ id: "closed", name: "已关闭" })
      ];
      common_vendor.index.showActionSheet({
        itemList: opts.map((x) => {
          return x.name;
        }),
        success: (res = null) => {
          status.value = opts[res.tapIndex].id;
          refresh();
        }
      });
    }
    function pickLevel() {
      const opts = [
        new UTSJSONObject({ id: "", name: "全部" }),
        new UTSJSONObject({ id: "critical", name: "危险" }),
        new UTSJSONObject({ id: "warning", name: "警告" }),
        new UTSJSONObject({ id: "info", name: "提示" })
      ];
      common_vendor.index.showActionSheet({
        itemList: opts.map((x) => {
          return x.name;
        }),
        success: (res = null) => {
          level.value = opts[res.tapIndex].id;
          refresh();
        }
      });
    }
    function goDetail(a = null) {
      common_vendor.index.navigateTo({ url: `/pages/alert/detail/detail?id=${a === null || a === void 0 ? null : a.id}` });
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(common_vendor.unref(statusLabel)),
        b: common_vendor.o(pickStatus),
        c: common_vendor.p({
          size: "mini"
        }),
        d: common_vendor.t(common_vendor.unref(levelLabel)),
        e: common_vendor.o(pickLevel),
        f: common_vendor.p({
          size: "mini"
        }),
        g: common_vendor.o(refresh),
        h: common_vendor.p({
          type: "primary",
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        i: common_vendor.f(common_vendor.unref(items), (a, k0, i0) => {
          return {
            a: "dc32d3be-3-" + i0,
            b: common_vendor.p({
              level: a.level,
              status: a.status
            }),
            c: common_vendor.t(a.createdAt || "-"),
            d: common_vendor.t(a.message),
            e: common_vendor.t(a.deviceId),
            f: a.id,
            g: common_vendor.o(($event) => {
              return goDetail(a);
            }, a.id)
          };
        }),
        j: !common_vendor.unref(loading) && common_vendor.unref(items).length === 0
      }, !common_vendor.unref(loading) && common_vendor.unref(items).length === 0 ? {
        k: common_vendor.p({
          text: "暂无告警",
          mode: "list"
        })
      } : {}, {
        l: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../../.sourcemap/mp-weixin/pages/alert/list/list.js.map
