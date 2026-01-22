"use strict";
const common_vendor = require("../../common/vendor.js");
if (!Array) {
  const _easycom_mrsong_charts_1 = common_vendor.resolveComponent("mrsong-charts");
  _easycom_mrsong_charts_1();
}
const _easycom_mrsong_charts = () => "../../uni_modules/mrsong-charts/components/mrsong-charts/mrsong-charts.js";
if (!Math) {
  (_easycom_mrsong_charts + common_vendor.unref(TelemetryLineChart))();
}
const TelemetryLineChart = () => "../../components/TelemetryLineChart/TelemetryLineChart.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "chartTest",
  setup(__props) {
    const now = Date.now();
    const mockData = common_vendor.ref([
      new UTSJSONObject({ temperature: 22, humidity: 55, timestamp: now - 10 * 60 * 1e3 }),
      new UTSJSONObject({ temperature: 23, humidity: 58, timestamp: now - 8 * 60 * 1e3 }),
      new UTSJSONObject({ temperature: 23.5, humidity: 56, timestamp: now - 6 * 60 * 1e3 }),
      new UTSJSONObject({ temperature: 24, humidity: 60, timestamp: now - 4 * 60 * 1e3 }),
      new UTSJSONObject({ temperature: 23.8, humidity: 62, timestamp: now - 2 * 60 * 1e3 }),
      new UTSJSONObject({ temperature: 24.2, humidity: 59, timestamp: now })
    ]);
    const lineChartData = common_vendor.ref(new UTSJSONObject({
      categories: ["10:00", "10:02", "10:04", "10:06", "10:08", "10:10"],
      series: [new UTSJSONObject({
        name: "温度",
        data: [22, 23, 23.5, 24, 23.8, 24.2]
      })]
    }));
    const lineConfig = new UTSJSONObject({
      itemCount: 6,
      scrollShow: false,
      rotateLabel: false,
      enableScroll: false,
      color: ["#0aa8ff"]
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = {
        a: common_vendor.p({
          type: "line",
          title: "",
          chartsData: common_vendor.unref(lineChartData),
          config: lineConfig
        }),
        b: common_vendor.p({
          title: "温度趋势",
          data: common_vendor.unref(mockData),
          field: "temperature",
          canvasId: "testTempChart"
        }),
        c: common_vendor.p({
          title: "湿度趋势",
          data: common_vendor.unref(mockData),
          field: "humidity",
          canvasId: "testHumidityChart"
        }),
        d: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      };
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/pages/chartTest/chartTest.js.map
