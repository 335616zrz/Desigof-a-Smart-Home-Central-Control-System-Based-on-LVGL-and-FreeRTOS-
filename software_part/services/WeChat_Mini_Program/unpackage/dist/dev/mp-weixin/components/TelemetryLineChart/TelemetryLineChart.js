"use strict";
const common_vendor = require("../../common/vendor.js");
if (!Math) {
  mrsongCharts();
}
const mrsongCharts = () => "../../uni_modules/mrsong-charts/components/mrsong-charts/mrsong-charts2.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "TelemetryLineChart",
  props: {
    title: { type: String, default: "数据趋势" },
    data: { type: Array, default: () => {
      return [];
    } },
    field: { type: String, default: "temperature" },
    canvasId: { type: String, default: "telemetryLineChart" }
  },
  setup(__props) {
    const props = __props;
    const unit = common_vendor.computed(() => {
      return props.field === "humidity" ? "%" : "°C";
    });
    const chartPoints = common_vendor.computed(() => {
      const arr = props.data || [];
      return arr.map((d = null) => {
        var _a;
        return new UTSJSONObject({
          value: Number((_a = d === null || d === void 0 ? null : d[props.field]) !== null && _a !== void 0 ? _a : 0),
          timestamp: Number((d === null || d === void 0 ? null : d.timestamp) || 0)
        });
      });
    });
    const hasData = common_vendor.computed(() => {
      return chartPoints.value.length > 0;
    });
    const latestValue = common_vendor.computed(() => {
      const pts = chartPoints.value;
      if (pts.length === 0)
        return null;
      return pts[pts.length - 1].value.toFixed(1);
    });
    function formatTime(ts) {
      const d = new Date(ts);
      return `${d.getHours()}:${String(d.getMinutes()).padStart(2, "0")}`;
    }
    const chartData = common_vendor.computed(() => {
      const pts = chartPoints.value;
      return new UTSJSONObject({
        categories: pts.map((p = null) => {
          return formatTime(p.timestamp);
        }),
        series: [new UTSJSONObject({
          name: props.field === "humidity" ? "湿度" : "温度",
          data: pts.map((p = null) => {
            return p.value;
          })
        })]
      });
    });
    const chartConfig = new UTSJSONObject({
      itemCount: 6,
      scrollShow: true,
      scrollAlign: "right",
      rotateLabel: false,
      enableScroll: true,
      color: ["#0aa8ff"]
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(__props.title),
        b: common_vendor.unref(latestValue) !== null
      }, common_vendor.unref(latestValue) !== null ? {
        c: common_vendor.t(common_vendor.unref(latestValue)),
        d: common_vendor.t(common_vendor.unref(unit))
      } : {}, {
        e: common_vendor.unref(hasData)
      }, common_vendor.unref(hasData) ? {
        f: common_vendor.p({
          type: "line",
          title: "",
          chartsData: common_vendor.unref(chartData),
          config: chartConfig
        })
      } : {}, {
        g: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/TelemetryLineChart/TelemetryLineChart.js.map
