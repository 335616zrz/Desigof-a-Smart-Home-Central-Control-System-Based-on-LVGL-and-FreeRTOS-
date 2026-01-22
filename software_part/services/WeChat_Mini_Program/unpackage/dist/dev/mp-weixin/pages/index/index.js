"use strict";
const common_vendor = require("../../common/vendor.js");
const common_constants = require("../../common/constants.js");
const stores_app = require("../../stores/app.js");
const stores_auth = require("../../stores/auth.js");
const stores_device = require("../../stores/device.js");
const stores_telemetry = require("../../stores/telemetry.js");
const stores_alert = require("../../stores/alert.js");
const stores_alertRule = require("../../stores/alertRule.js");
if (!Array) {
  const _easycom_u_grid_item_1 = common_vendor.resolveComponent("u-grid-item");
  const _easycom_u_grid_1 = common_vendor.resolveComponent("u-grid");
  const _easycom_u_subsection_1 = common_vendor.resolveComponent("u-subsection");
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_skeleton_1 = common_vendor.resolveComponent("u-skeleton");
  (_easycom_u_grid_item_1 + _easycom_u_grid_1 + _easycom_u_subsection_1 + _easycom_u_button_1 + _easycom_u_skeleton_1)();
}
const _easycom_u_grid_item = () => "../../uni_modules/uview-plus/components/u-grid-item/u-grid-item.js";
const _easycom_u_grid = () => "../../uni_modules/uview-plus/components/u-grid/u-grid.js";
const _easycom_u_subsection = () => "../../uni_modules/uview-plus/components/u-subsection/u-subsection.js";
const _easycom_u_button = () => "../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_skeleton = () => "../../uni_modules/uview-plus/components/u-skeleton/u-skeleton.js";
if (!Math) {
  (common_vendor.unref(HeroCard) + common_vendor.unref(OnlineRatePanel) + common_vendor.unref(StatCard) + _easycom_u_grid_item + _easycom_u_grid + _easycom_u_subsection + common_vendor.unref(TelemetryLineChart) + common_vendor.unref(AlertTimeline) + common_vendor.unref(TelemetryTable) + _easycom_u_button + _easycom_u_skeleton)();
}
const AlertTimeline = () => "../../components/AlertTimeline/AlertTimeline.js";
const HeroCard = () => "../../components/HeroCard/HeroCard.js";
const OnlineRatePanel = () => "../../components/OnlineRatePanel/OnlineRatePanel.js";
const StatCard = () => "../../components/StatCard/StatCard.js";
const TelemetryLineChart = () => "../../components/TelemetryLineChart/TelemetryLineChart.js";
const TelemetryTable = () => "../../components/TelemetryTable/TelemetryTable.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "index",
  setup(__props) {
    const app = stores_app.useAppStore();
    const auth = stores_auth.useAuthStore();
    const deviceStore = stores_device.useDeviceStore();
    const telemetryStore = stores_telemetry.useTelemetryStore();
    const alertStore = stores_alert.useAlertStore();
    const ruleStore = stores_alertRule.useAlertRuleStore();
    const loading = common_vendor.ref(false);
    const initialLoading = common_vendor.ref(true);
    const now = common_vendor.ref(Date.now());
    let timer = null;
    const username = common_vendor.computed(() => {
      var _a;
      return ((_a = auth.user) === null || _a === void 0 ? null : _a.username) || "";
    });
    const networkType = common_vendor.computed(() => {
      return app.networkType || "unknown";
    });
    const mqttStatus = common_vendor.computed(() => {
      return app.mqttStatus || "unknown";
    });
    const seasonText = common_vendor.computed(() => {
      var _a;
      return ((_a = ruleStore.season) === null || _a === void 0 ? null : _a.displayName) || "";
    });
    const nowText = common_vendor.computed(() => {
      const d = new Date(now.value);
      return `${d.getHours()}:${String(d.getMinutes()).padStart(2, "0")}`;
    });
    const deviceTotal = common_vendor.computed(() => {
      return (deviceStore.items || []).length;
    });
    const deviceOnline = common_vendor.computed(() => {
      return (deviceStore.items || []).filter((d = null) => {
        const s = String((d === null || d === void 0 ? null : d.status) || "").toUpperCase();
        return s === "ONLINE" || s === "AUTHENTICATED";
      }).length;
    });
    const deviceOffline = common_vendor.computed(() => {
      return (deviceStore.items || []).filter((d = null) => {
        return String((d === null || d === void 0 ? null : d.status) || "").toUpperCase() === "OFFLINE";
      }).length;
    });
    common_vendor.computed(() => {
      const total = deviceTotal.value || 0;
      if (!total)
        return 0;
      return Math.round(deviceOnline.value / total * 100);
    });
    const openAlertsTotal = common_vendor.computed(() => {
      return alertStore.total || 0;
    });
    const chartModeList = [new UTSJSONObject({ name: "实时(近10分钟)" }), new UTSJSONObject({ name: "历史数据" })];
    const chartMode = common_vendor.ref(0);
    const temperatureData = common_vendor.computed(() => {
      const list = telemetryStore.temperatureChartData || [];
      if (chartMode.value !== 0)
        return list;
      const threshold = Date.now() - 10 * 60 * 1e3;
      const recent = list.filter((r = null) => {
        return (r.timestamp || 0) >= threshold;
      });
      return recent.length > 0 ? recent : list.slice(-20);
    });
    const humidityData = common_vendor.computed(() => {
      const list = telemetryStore.humidityChartData || [];
      if (chartMode.value !== 0)
        return list;
      const threshold = Date.now() - 10 * 60 * 1e3;
      const recent = list.filter((r = null) => {
        return (r.timestamp || 0) >= threshold;
      });
      return recent.length > 0 ? recent : list.slice(-20);
    });
    const timelineAlerts = common_vendor.computed(() => {
      return (alertStore.items || []).slice(0, 5);
    });
    const telemetryRows = common_vendor.computed(() => {
      return (telemetryStore.latestByDevice || []).slice(0, 6);
    });
    common_vendor.onShow(() => {
      auth.initFromStorage();
      getNetwork();
      if (!timer) {
        timer = setInterval(() => {
          now.value = Date.now();
        }, 30 * 1e3);
      }
      refresh();
    });
    common_vendor.onHide(() => {
      if (timer) {
        clearInterval(timer);
        timer = null;
      }
    });
    function getNetwork() {
      common_vendor.index.getNetworkType(new UTSJSONObject({
        success: (res = null) => {
          app.setNetworkType((res === null || res === void 0 ? null : res.networkType) || "unknown");
        },
        fail: () => {
          app.setNetworkType("unknown");
        }
      }));
    }
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (loading.value)
          return Promise.resolve(null);
        loading.value = true;
        try {
          yield Promise.all([
            deviceStore.fetchList(),
            ruleStore.fetchCurrentSeason(),
            telemetryStore.fetchHistory(new UTSJSONObject({ pageSize: 200, page: 1 })),
            alertStore.fetchList(new UTSJSONObject({ status: "open", page: 0, pageSize: 5 }))
          ]);
        } finally {
          loading.value = false;
          if (initialLoading.value)
            initialLoading.value = false;
        }
      });
    }
    function onChartModeChange(index) {
      chartMode.value = index;
    }
    function goDevice() {
      common_vendor.index.navigateTo({ url: "/pages/device/list/list" });
    }
    function goAlert() {
      common_vendor.index.navigateTo({ url: "/pages/alert/list/list" });
    }
    function goTelemetry() {
      common_vendor.index.navigateTo({ url: "/pages/telemetry/history/history" });
    }
    function goRule() {
      common_vendor.index.navigateTo({ url: "/pages/alertRule/list/list" });
    }
    function goMusic() {
      common_vendor.index.navigateTo({ url: "/pages/music/music" });
    }
    function goProfile() {
      common_vendor.index.navigateTo({ url: "/pages/profile/profile" });
    }
    function goAlertDetail(a = null) {
      common_vendor.index.navigateTo({ url: `/pages/alert/detail/detail?id=${a === null || a === void 0 ? null : a.id}` });
    }
    function onLogout() {
      auth.logout();
      common_vendor.index.reLaunch({ url: common_constants.ROUTES.LOGIN });
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = {
        a: common_vendor.p({
          title: "指挥中心",
          subtitle: `欢迎，${common_vendor.unref(username) || "用户"}`,
          mqtt: common_vendor.unref(mqttStatus),
          network: common_vendor.unref(networkType),
          season: common_vendor.unref(seasonText),
          time: common_vendor.unref(nowText)
        }),
        b: common_vendor.p({
          online: common_vendor.unref(deviceOnline),
          total: common_vendor.unref(deviceTotal)
        }),
        c: common_vendor.p({
          icon: "grid",
          label: "设备总数",
          value: common_vendor.unref(deviceTotal),
          description: "已接入设备",
          tone: "info"
        }),
        d: common_vendor.p({
          icon: "checkbox-mark",
          label: "在线设备",
          value: common_vendor.unref(deviceOnline),
          description: "状态正常",
          tone: "success"
        }),
        e: common_vendor.p({
          icon: "close-circle",
          label: "离线设备",
          value: common_vendor.unref(deviceOffline),
          description: "需检查网络/供电",
          tone: "warning"
        }),
        f: common_vendor.p({
          icon: "error-circle",
          label: "开放告警",
          value: common_vendor.unref(openAlertsTotal),
          description: "未处理告警",
          tone: "danger"
        }),
        g: common_vendor.p({
          col: 2,
          border: false,
          gap: "12px"
        }),
        h: common_vendor.o(onChartModeChange),
        i: common_vendor.p({
          list: chartModeList,
          current: common_vendor.unref(chartMode)
        }),
        j: common_vendor.p({
          title: "温度趋势",
          data: common_vendor.unref(temperatureData),
          field: "temperature",
          canvasId: "tempChart"
        }),
        k: common_vendor.p({
          title: "湿度趋势",
          data: common_vendor.unref(humidityData),
          field: "humidity",
          canvasId: "humidityChart"
        }),
        l: common_vendor.o(goAlertDetail),
        m: common_vendor.p({
          title: "告警时间线",
          alerts: common_vendor.unref(timelineAlerts)
        }),
        n: common_vendor.p({
          title: "设备最新读数",
          rows: common_vendor.unref(telemetryRows)
        }),
        o: common_vendor.o(goDevice),
        p: common_vendor.p({
          type: "primary",
          size: "mini"
        }),
        q: common_vendor.o(goAlert),
        r: common_vendor.p({
          size: "mini"
        }),
        s: common_vendor.o(goTelemetry),
        t: common_vendor.p({
          size: "mini"
        }),
        v: common_vendor.o(goRule),
        w: common_vendor.p({
          size: "mini"
        }),
        x: common_vendor.o(goMusic),
        y: common_vendor.p({
          size: "mini"
        }),
        z: common_vendor.o(goProfile),
        A: common_vendor.p({
          size: "mini"
        }),
        B: common_vendor.o(refresh),
        C: common_vendor.p({
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        D: common_vendor.o(onLogout),
        E: common_vendor.p({
          size: "mini",
          plain: true
        }),
        F: common_vendor.p({
          loading: common_vendor.unref(initialLoading),
          animate: true,
          rows: 10
        }),
        G: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      };
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/pages/index/index.js.map
