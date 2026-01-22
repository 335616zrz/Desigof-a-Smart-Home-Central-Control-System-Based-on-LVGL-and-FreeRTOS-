"use strict";
const common_vendor = require("./common/vendor.js");
const uni_modules_mrsongCharts_components_mrsongCharts_uchart_config = require("./uni_modules/mrsong-charts/components/mrsong-charts/uchart.config.js");
const qiunDataCharts = () => "./uni_modules/mrsong-charts/components/mrsong-charts/qiun-data-charts/components/qiun-data-charts/qiun-data-charts.js";
const _sfc_main = common_vendor.defineComponent({
  name: "MrsongCharts",
  components: {
    qiunDataCharts
  },
  props: {
    type: {
      type: String,
      default: "column"
    },
    unit: {
      type: String,
      default: ""
    },
    chartsData: {
      type: Object,
      default() {
        return null;
      }
    },
    config: {
      type: Object,
      default() {
        return new UTSJSONObject({
          itemCount: 3,
          scrollShow: false,
          scrollAlign: "left",
          rotateLabel: true,
          min: 0,
          // "max": 150, //Y轴大值
          unit: "",
          enableScroll: false,
          color: [
            // 颜色设置
            "#9A60B4",
            "#ea7ccc"
          ]
        });
      }
    },
    options: {
      type: Object,
      default() {
        return new UTSJSONObject({
          color: ["#008749", "#91CB74", "#FAC858", "#EE6666", "#73C0DE", "#3CA272", "#FC8452", "#9A60B4", "#ea7ccc"],
          xAxis: new UTSJSONObject({
            itemCount: 3,
            scrollShow: false,
            scrollAlign: "left",
            rotateLabel: true
            // X轴label旋转
          }),
          yAxis: new UTSJSONObject({
            data: [
              new UTSJSONObject({
                min: 0,
                // "max": 150, //Y轴大值
                unit: ""
                // Y轴单位
              })
            ]
          }),
          enableScroll: false
          // 开启滚动模式
        });
      }
    },
    title: {
      type: String,
      default: "标题"
    },
    align: {
      type: String,
      default: "center"
    }
  },
  data() {
    return {
      opts: new UTSJSONObject({}),
      yMax: 10,
      currentData: new UTSJSONObject({
        categories: [],
        series: []
      }),
      showChart: false
    };
  },
  computed: {},
  watch: {
    type: {
      handler(val = null) {
        if (val) {
          this.formatType(val);
        }
      },
      deep: true,
      immediate: true
    },
    options: {
      handler(val = null) {
        if (val) {
          let opts = this.assignDeep(this.opts, val);
          if (this.config) {
            let xAxis = opts.xAxis;
            let yAxis = opts.yAxis.length ? opts.yAxis[0] : new UTSJSONObject({});
            let color = this.config.color;
            if (opts.color)
              opts.color = color;
            for (let key in this.config) {
              if (Object.keys(xAxis).includes(key))
                xAxis[key] = this.config[key];
              if (Object.keys(yAxis).includes(key))
                yAxis[key] = this.config[key];
              if (Object.keys(opts).includes(key)) {
                opts[key] = this.config[key];
              }
            }
            this.opts = opts;
          }
        }
      },
      deep: true,
      immediate: true
    },
    chartsData: {
      handler(val = null) {
        if (val) {
          this.currentData = val;
          let list = [];
          new UTSJSONObject({});
          let seriesItem = val.series || [];
          seriesItem.map((item = null) => {
            list = [...list, ...item.data];
          });
          if (this.type == "column" || this.type == "line") {
            let max = Math.max.apply(Math, list.map((item) => {
              return item;
            }));
            this.yMax = max > 10 ? max : 10;
            this.opts.yAxis.data.forEach((item = null) => {
              if (max > 10) {
                delete item.max;
              } else {
                item.max = this.yMax;
              }
            });
          }
        } else {
          this.currentData = this.initchartsData();
        }
      },
      deep: true,
      immediate: true
    }
  },
  mounted() {
    common_vendor.index.showLoading({
      title: "正在加载"
    });
    this.$nextTick(() => {
      this.showChart = true;
      common_vendor.index.hideLoading();
    });
  },
  methods: {
    initchartsData() {
      if (this.type == "mount" || this.type == "pie" || this.type == "ring" || this.type == "funnel") {
        return new UTSJSONObject({
          series: [
            new UTSJSONObject({
              data: [
                new UTSJSONObject({
                  name: "一班",
                  value: 82
                }),
                new UTSJSONObject({
                  name: "二班",
                  value: 63
                }),
                new UTSJSONObject({
                  name: "三班",
                  value: 50
                }),
                new UTSJSONObject({
                  name: "四班",
                  value: 40
                }),
                new UTSJSONObject({
                  name: "五班",
                  value: 30
                })
              ]
            })
          ]
        });
      } else {
        return new UTSJSONObject({
          categories: ["2016", "2017", "2018", "2019", "2020", "2021"],
          series: [
            new UTSJSONObject({
              name: "目标值",
              data: [40, 36, 31, 33, 13, 34]
            }),
            new UTSJSONObject({
              name: "完成量",
              data: [18, 27, 21, 24, 6, 28]
            })
          ]
        });
      }
    },
    assignDeep(target = null) {
      const isPlainObject = (obj = null) => {
        return Object.prototype.toString.call(obj) === "[object Object]";
      };
      const args = Array.from(arguments);
      if (args.length < 2)
        return args[0];
      let result = args[0];
      UTS.arrayShift(args);
      args.forEach((item = null) => {
        if (isPlainObject(item)) {
          if (!isPlainObject(result))
            result = new UTSJSONObject({});
          for (let key in item) {
            if (result[key] && isPlainObject(item[key])) {
              result[key] = this.assignDeep(result[key], item[key]);
            } else {
              result[key] = item[key];
            }
          }
        } else if (UTS.isInstanceOf(item, Array)) {
          if (!UTS.isInstanceOf(result, Array))
            result = [];
          item.forEach((arrItem = null, arrIndex) => {
            if (isPlainObject(arrItem)) {
              result[arrIndex] = this.assignDeep(result[arrIndex]);
            } else {
              result[arrIndex] = arrItem;
            }
          });
        }
      });
      return result;
    },
    formatType(str = null) {
      switch (str) {
        case "column":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initColumnData();
          break;
        case "line":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initLineData();
          break;
        case "funnel":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initFunnelData();
          break;
        case "ring":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initRingData();
          break;
        case "mount":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initMountData();
          break;
        case "bar":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initBarData();
          break;
        case "pie":
          this.opts = uni_modules_mrsongCharts_components_mrsongCharts_uchart_config.initPieData();
          break;
      }
    }
  }
});
if (!Array) {
  const _easycom_qiun_data_charts2 = common_vendor.resolveComponent("qiun-data-charts");
  _easycom_qiun_data_charts2();
}
const _easycom_qiun_data_charts = () => "./uni_modules/qiun-data-charts/components/qiun-data-charts/qiun-data-charts.js";
if (!Math) {
  _easycom_qiun_data_charts();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: $data.showChart
  }, $data.showChart ? common_vendor.e({
    b: common_vendor.t($props.title),
    c: common_vendor.s("text-align:" + $props.align),
    d: $props.unit
  }, $props.unit ? {
    e: common_vendor.t($props.unit ? `(${$props.unit})` : "")
  } : {}, {
    f: common_vendor.p({
      opts: $data.opts,
      type: $props.type,
      canvas2d: true,
      ["chart-data"]: $data.currentData,
      background: "none",
      animation: false,
      ontouch: true
    }),
    g: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
  }) : {});
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-f4f8c60f"]]);
exports.Component = Component;
//# sourceMappingURL=../.sourcemap/mp-weixin/mrsong-charts.js.map
