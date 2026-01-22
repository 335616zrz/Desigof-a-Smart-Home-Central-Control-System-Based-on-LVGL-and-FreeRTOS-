"use strict";
const common_vendor = require("../../../../../../../common/vendor.js");
const uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_uCharts = require("../../js_sdk/u-charts/u-charts.js");
const uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts = require("../../js_sdk/u-charts/config-ucharts.js");
function deepCloneAssign(origin = new UTSJSONObject({}), ...args) {
  for (let i in args) {
    for (let key in args[i]) {
      if (args[i].hasOwnProperty(key)) {
        origin[key] = args[i][key] && typeof args[i][key] === "object" ? deepCloneAssign(Array.isArray(args[i][key]) ? [] : new UTSJSONObject({}), origin[key], args[i][key]) : args[i][key];
      }
    }
  }
  return origin;
}
function formatterAssign(args = null, formatter = null) {
  for (let key in args) {
    if (args.hasOwnProperty(key) && args[key] !== null && typeof args[key] === "object") {
      formatterAssign(args[key], formatter);
    } else if (key === "format" && typeof args[key] === "string") {
      args["formatter"] = formatter[args[key]] ? formatter[args[key]] : void 0;
    }
  }
  return args;
}
function getFormatDate(date = null) {
  var seperator = "-";
  var year = date.getFullYear();
  var month = date.getMonth() + 1;
  var strDate = date.getDate();
  if (month >= 1 && month <= 9) {
    month = "0" + month;
  }
  if (strDate >= 0 && strDate <= 9) {
    strDate = "0" + strDate;
  }
  var currentdate = year + seperator + month + seperator + strDate;
  return currentdate;
}
var lastMoveTime = null;
function debounce(fn = null, wait = null) {
  let timer = false;
  return function() {
    clearTimeout(timer);
    timer && clearTimeout(timer);
    timer = setTimeout(() => {
      timer = false;
      fn.apply(this, arguments);
    }, wait);
  };
}
const _sfc_main = common_vendor.defineComponent({
  name: "qiun-data-charts",
  mixins: [common_vendor.er.mixinDatacom],
  props: {
    type: {
      type: String,
      default: null
    },
    canvasId: {
      type: String,
      default: "uchartsid"
    },
    canvas2d: {
      type: Boolean,
      default: false
    },
    background: {
      type: String,
      default: "rgba(0,0,0,0)"
    },
    animation: {
      type: Boolean,
      default: true
    },
    chartData: {
      type: Object,
      default() {
        return new UTSJSONObject({
          categories: [],
          series: []
        });
      }
    },
    opts: {
      type: Object,
      default() {
        return new UTSJSONObject({});
      }
    },
    eopts: {
      type: Object,
      default() {
        return new UTSJSONObject({});
      }
    },
    loadingType: {
      type: Number,
      default: 2
    },
    errorShow: {
      type: Boolean,
      default: true
    },
    errorReload: {
      type: Boolean,
      default: true
    },
    errorMessage: {
      type: String,
      default: null
    },
    inScrollView: {
      type: Boolean,
      default: false
    },
    reshow: {
      type: Boolean,
      default: false
    },
    reload: {
      type: Boolean,
      default: false
    },
    disableScroll: {
      type: Boolean,
      default: false
    },
    optsWatch: {
      type: Boolean,
      default: true
    },
    onzoom: {
      type: Boolean,
      default: false
    },
    ontap: {
      type: Boolean,
      default: true
    },
    ontouch: {
      type: Boolean,
      default: false
    },
    onmouse: {
      type: Boolean,
      default: true
    },
    onmovetip: {
      type: Boolean,
      default: false
    },
    echartsH5: {
      type: Boolean,
      default: false
    },
    echartsApp: {
      type: Boolean,
      default: false
    },
    tooltipShow: {
      type: Boolean,
      default: true
    },
    tooltipFormat: {
      type: String,
      default: void 0
    },
    tooltipCustom: {
      type: Object,
      default: void 0
    },
    startDate: {
      type: String,
      default: void 0
    },
    endDate: {
      type: String,
      default: void 0
    },
    textEnum: {
      type: Array,
      default() {
        return [];
      }
    },
    groupEnum: {
      type: Array,
      default() {
        return [];
      }
    },
    pageScrollTop: {
      type: Number,
      default: 0
    },
    directory: {
      type: String,
      default: "/"
    },
    tapLegend: {
      type: Boolean,
      default: true
    },
    menus: {
      type: Array,
      default() {
        return [];
      }
    }
  },
  data() {
    return {
      cid: "uchartsid",
      inWx: false,
      inAli: false,
      inTt: false,
      inBd: false,
      inH5: false,
      inApp: false,
      inWin: false,
      type2d: true,
      disScroll: false,
      openmouse: false,
      pixel: 1,
      cWidth: 375,
      cHeight: 250,
      showchart: false,
      echarts: false,
      echartsResize: false,
      uchartsOpts: new UTSJSONObject({}),
      echartsOpts: new UTSJSONObject({}),
      drawData: new UTSJSONObject({}),
      lastDrawTime: null
    };
  },
  created() {
    this.cid = this.canvasId;
    if (this.canvasId == "uchartsid" || this.canvasId == "") {
      let t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
      let len = t.length;
      let id = "";
      for (let i = 0; i < 32; i++) {
        id += t.charAt(Math.floor(Math.random() * len));
      }
      this.cid = id;
    }
    const systemInfo = common_vendor.index.getSystemInfoSync();
    if (systemInfo.platform === "windows" || systemInfo.platform === "mac") {
      this.inWin = true;
    }
    this.inWx = true;
    if (this.canvas2d === false) {
      this.type2d = false;
    } else {
      this.type2d = true;
      this.pixel = systemInfo.pixelRatio;
    }
    this.disScroll = this.disableScroll;
  },
  mounted() {
    this.$nextTick(() => {
      this.beforeInit();
    });
    const time = this.inH5 ? 500 : 200;
    const _this = this;
    common_vendor.index.onWindowResize(debounce(function(res = null) {
      if (_this.mixinDatacomLoading == true) {
        return null;
      }
      let errmsg = _this.mixinDatacomErrorMessage;
      if (errmsg !== null && errmsg !== "null" && errmsg !== "") {
        return null;
      }
      if (_this.echarts) {
        _this.echartsResize = !_this.echartsResize;
      } else {
        _this.resizeHandler();
      }
    }, time));
  },
  destroyed() {
    if (this.echarts === true) {
      delete cfe.option[this.cid];
      delete cfe.instance[this.cid];
    } else {
      delete uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[this.cid];
      delete uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[this.cid];
    }
    common_vendor.index.offWindowResize(() => {
    });
  },
  watch: {
    chartDataProps: {
      handler(val = null, oldval = null) {
        if (typeof val === "object") {
          if (UTS.JSON.stringify(val) !== UTS.JSON.stringify(oldval)) {
            this._clearChart();
            if (val.series && val.series.length > 0) {
              this.beforeInit();
            } else {
              this.mixinDatacomLoading = true;
              this.showchart = false;
              this.mixinDatacomErrorMessage = null;
            }
          }
        } else {
          this.mixinDatacomLoading = false;
          this._clearChart();
          this.showchart = false;
          this.mixinDatacomErrorMessage = "参数错误：chartData数据类型错误";
        }
      },
      immediate: false,
      deep: true
    },
    localdata: {
      handler(val = null, oldval = null) {
        if (UTS.JSON.stringify(val) !== UTS.JSON.stringify(oldval)) {
          if (val.length > 0) {
            this.beforeInit();
          } else {
            this.mixinDatacomLoading = true;
            this._clearChart();
            this.showchart = false;
            this.mixinDatacomErrorMessage = null;
          }
        }
      },
      immediate: false,
      deep: true
    },
    optsProps: {
      handler(val = null, oldval = null) {
        if (typeof val === "object") {
          if (UTS.JSON.stringify(val) !== UTS.JSON.stringify(oldval) && this.echarts === false && this.optsWatch == true) {
            this.checkData(this.drawData);
          }
        } else {
          this.mixinDatacomLoading = false;
          this._clearChart();
          this.showchart = false;
          this.mixinDatacomErrorMessage = "参数错误：opts数据类型错误";
        }
      },
      immediate: false,
      deep: true
    },
    eoptsProps: {
      handler(val = null, oldval = null) {
        if (typeof val === "object") {
          if (UTS.JSON.stringify(val) !== UTS.JSON.stringify(oldval) && this.echarts === true) {
            this.checkData(this.drawData);
          }
        } else {
          this.mixinDatacomLoading = false;
          this.showchart = false;
          this.mixinDatacomErrorMessage = "参数错误：eopts数据类型错误";
        }
      },
      immediate: false,
      deep: true
    },
    reshow(val = null, oldval = null) {
      if (val === true && this.mixinDatacomLoading === false) {
        setTimeout(() => {
          this.mixinDatacomErrorMessage = null;
          this.echartsResize = !this.echartsResize;
          this.checkData(this.drawData);
        }, 200);
      }
    },
    reload(val = null, oldval = null) {
      if (val === true) {
        this.showchart = false;
        this.mixinDatacomErrorMessage = null;
        this.reloading();
      }
    },
    mixinDatacomErrorMessage(val = null, oldval = null) {
      if (val) {
        this.emitMsg(new UTSJSONObject({ name: "error", params: new UTSJSONObject({ type: "error", errorShow: this.errorShow, msg: val, id: this.cid }) }));
        if (this.errorShow) {
          common_vendor.index.__f__("log", "at uni_modules/mrsong-charts/components/mrsong-charts/qiun-data-charts/components/qiun-data-charts/qiun-data-charts.vue:610", "[秋云图表组件]" + val);
        }
      }
    },
    errorMessage(val = null, oldval = null) {
      if (val && this.errorShow && val !== null && val !== "null" && val !== "") {
        this.showchart = false;
        this.mixinDatacomLoading = false;
        this.mixinDatacomErrorMessage = val;
      } else {
        this.showchart = false;
        this.mixinDatacomErrorMessage = null;
        this.reloading();
      }
    }
  },
  computed: {
    optsProps() {
      return UTS.JSON.parse(UTS.JSON.stringify(this.opts));
    },
    eoptsProps() {
      return UTS.JSON.parse(UTS.JSON.stringify(this.eopts));
    },
    chartDataProps() {
      return UTS.JSON.parse(UTS.JSON.stringify(this.chartData));
    }
  },
  methods: {
    beforeInit() {
      this.mixinDatacomErrorMessage = null;
      if (typeof this.chartData === "object" && this.chartData != null && this.chartData.series !== void 0 && this.chartData.series.length > 0) {
        this.drawData = deepCloneAssign(new UTSJSONObject({}), this.chartData);
        this.mixinDatacomLoading = false;
        this.showchart = true;
        this.checkData(this.chartData);
      } else if (this.localdata.length > 0) {
        this.mixinDatacomLoading = false;
        this.showchart = true;
        this.localdataInit(this.localdata);
      } else if (this.collection !== "") {
        this.mixinDatacomLoading = false;
        this.getCloudData();
      } else {
        this.mixinDatacomLoading = true;
      }
    },
    localdataInit(resdata = null) {
      if (this.groupEnum.length > 0) {
        for (let i = 0; i < resdata.length; i++) {
          for (let j = 0; j < this.groupEnum.length; j++) {
            if (resdata[i].group === this.groupEnum[j].value) {
              resdata[i].group = this.groupEnum[j].text;
            }
          }
        }
      }
      if (this.textEnum.length > 0) {
        for (let i = 0; i < resdata.length; i++) {
          for (let j = 0; j < this.textEnum.length; j++) {
            if (resdata[i].text === this.textEnum[j].value) {
              resdata[i].text = this.textEnum[j].text;
            }
          }
        }
      }
      let needCategories = false;
      let tmpData = new UTSJSONObject({ categories: [], series: [] });
      let tmpcategories = [];
      let tmpseries = [];
      if (this.echarts === true) {
        needCategories = cfe.categories.includes(this.type);
      } else {
        needCategories = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.categories.includes(this.type);
      }
      if (needCategories === true) {
        if (this.chartData && this.chartData.categories && this.chartData.categories.length > 0) {
          tmpcategories = this.chartData.categories;
        } else {
          if (this.startDate && this.endDate) {
            let idate = new Date(this.startDate);
            let edate = new Date(this.endDate);
            while (idate <= edate) {
              tmpcategories.push(getFormatDate(idate));
              idate = idate.setDate(idate.getDate() + 1);
              idate = new Date(idate);
            }
          } else {
            let tempckey = new UTSJSONObject({});
            resdata.map(function(item = null, index = null) {
              if (item.text != void 0 && !tempckey[item.text]) {
                tmpcategories.push(item.text);
                tempckey[item.text] = true;
              }
            });
          }
        }
        tmpData.categories = tmpcategories;
      }
      let tempskey = new UTSJSONObject({});
      resdata.map(function(item = null, index = null) {
        if (item.group != void 0 && !tempskey[item.group]) {
          tmpseries.push({ name: item.group, data: [] });
          tempskey[item.group] = true;
        }
      });
      if (tmpseries.length == 0) {
        tmpseries = [{ name: "默认分组", data: [] }];
        if (needCategories === true) {
          for (let j = 0; j < tmpcategories.length; j++) {
            let seriesdata = 0;
            for (let i = 0; i < resdata.length; i++) {
              if (resdata[i].text == tmpcategories[j]) {
                seriesdata = resdata[i].value;
              }
            }
            tmpseries[0].data.push(seriesdata);
          }
        } else {
          for (let i = 0; i < resdata.length; i++) {
            tmpseries[0].data.push(new UTSJSONObject({ "name": resdata[i].text, "value": resdata[i].value }));
          }
        }
      } else {
        for (let k = 0; k < tmpseries.length; k++) {
          if (tmpcategories.length > 0) {
            for (let j = 0; j < tmpcategories.length; j++) {
              let seriesdata = 0;
              for (let i = 0; i < resdata.length; i++) {
                if (tmpseries[k].name == resdata[i].group && resdata[i].text == tmpcategories[j]) {
                  seriesdata = resdata[i].value;
                }
              }
              tmpseries[k].data.push(seriesdata);
            }
          } else {
            for (let i = 0; i < resdata.length; i++) {
              if (tmpseries[k].name == resdata[i].group) {
                tmpseries[k].data.push(resdata[i].value);
              }
            }
          }
        }
      }
      tmpData.series = tmpseries;
      this.drawData = deepCloneAssign(new UTSJSONObject({}), tmpData);
      this.checkData(tmpData);
    },
    reloading() {
      if (this.errorReload === false) {
        return null;
      }
      this.showchart = false;
      this.mixinDatacomErrorMessage = null;
      if (this.collection !== "") {
        this.mixinDatacomLoading = false;
        this.onMixinDatacomPropsChange(true);
      } else {
        this.beforeInit();
      }
    },
    checkData(anyData = null) {
      let cid = this.cid;
      if (this.echarts === true) {
        cfe.option[cid] = deepCloneAssign(new UTSJSONObject({}), this.eopts);
        cfe.option[cid].id = cid;
        cfe.option[cid].type = this.type;
      } else {
        if (this.type && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.type.includes(this.type)) {
          uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid] = deepCloneAssign(new UTSJSONObject({}), uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu[this.type], this.opts);
          uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].canvasId = cid;
        } else {
          this.mixinDatacomLoading = false;
          this.showchart = false;
          this.mixinDatacomErrorMessage = "参数错误：props参数中type类型不正确";
        }
      }
      let newData = deepCloneAssign(new UTSJSONObject({}), anyData);
      if (newData.series !== void 0 && newData.series.length > 0) {
        this.mixinDatacomErrorMessage = null;
        if (this.echarts === true) {
          cfe.option[cid].chartData = newData;
          this.$nextTick(() => {
            this.init();
          });
        } else {
          uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].categories = newData.categories;
          uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].series = newData.series;
          this.$nextTick(() => {
            this.init();
          });
        }
      }
    },
    resizeHandler() {
      let currTime = Date.now();
      let lastDrawTime = this.lastDrawTime ? this.lastDrawTime : currTime - 3e3;
      let duration = currTime - lastDrawTime;
      if (duration < 1e3)
        return null;
      common_vendor.index.createSelectorQuery().in(this).select("#ChartBoxId" + this.cid).boundingClientRect((data = null) => {
        this.showchart = true;
        if (data.width > 0 && data.height > 0) {
          if (data.width !== this.cWidth || data.height !== this.cHeight) {
            this.checkData(this.drawData);
          }
        }
      }).exec();
    },
    getCloudData() {
      if (this.mixinDatacomLoading == true) {
        return null;
      }
      this.mixinDatacomLoading = true;
      this.mixinDatacomGet().then((res) => {
        this.mixinDatacomResData = res.result.data;
        this.localdataInit(this.mixinDatacomResData);
      }).catch((err = null) => {
        this.mixinDatacomLoading = false;
        this.showchart = false;
        this.mixinDatacomErrorMessage = "请求错误：" + err;
      });
    },
    onMixinDatacomPropsChange(needReset = null, changed = null) {
      if (needReset == true && this.collection !== "") {
        this.showchart = false;
        this.mixinDatacomErrorMessage = null;
        this._clearChart();
        this.getCloudData();
      }
    },
    _clearChart() {
      let cid = this.cid;
      if (this.echrts !== true && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid] && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context) {
        const ctx = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context;
        if (typeof ctx === "object" && !uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].update) {
          ctx.clearRect(0, 0, this.cWidth, this.cHeight);
          ctx.draw();
        }
      }
    },
    init() {
      let cid = this.cid;
      common_vendor.index.createSelectorQuery().in(this).select("#ChartBoxId" + cid).boundingClientRect((data = null) => {
        if (data.width > 0 && data.height > 0) {
          this.mixinDatacomLoading = false;
          this.showchart = true;
          this.lastDrawTime = Date.now();
          this.cWidth = data.width;
          this.cHeight = data.height;
          if (this.echarts !== true) {
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].background = this.background == "rgba(0,0,0,0)" ? "#FFFFFF" : this.background;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].canvas2d = this.type2d;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].pixelRatio = this.pixel;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].animation = this.animation;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].width = data.width * this.pixel;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].height = data.height * this.pixel;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].onzoom = this.onzoom;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].ontap = this.ontap;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].ontouch = this.ontouch;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].onmouse = this.openmouse;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].onmovetip = this.onmovetip;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipShow = this.tooltipShow;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat = this.tooltipFormat;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipCustom = this.tooltipCustom;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].inScrollView = this.inScrollView;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].lastDrawTime = this.lastDrawTime;
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tapLegend = this.tapLegend;
          }
          if (this.inH5 || this.inApp) {
            if (this.echarts == true) {
              cfe.option[cid].ontap = this.ontap;
              cfe.option[cid].onmouse = this.openmouse;
              cfe.option[cid].tooltipShow = this.tooltipShow;
              cfe.option[cid].tooltipFormat = this.tooltipFormat;
              cfe.option[cid].tooltipCustom = this.tooltipCustom;
              cfe.option[cid].lastDrawTime = this.lastDrawTime;
              this.echartsOpts = deepCloneAssign(new UTSJSONObject({}), cfe.option[cid]);
            } else {
              uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].rotateLock = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].rotate;
              this.uchartsOpts = deepCloneAssign(new UTSJSONObject({}), uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid]);
            }
          } else {
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid] = formatterAssign(uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid], uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.formatter);
            this.mixinDatacomErrorMessage = null;
            this.mixinDatacomLoading = false;
            this.showchart = true;
            this.$nextTick(() => {
              if (this.type2d === true) {
                const query = common_vendor.index.createSelectorQuery().in(this);
                query.select("#" + cid).fields(new UTSJSONObject({ node: true, size: true })).exec((res) => {
                  if (res[0]) {
                    const canvas = res[0].node;
                    const ctx = canvas.getContext("2d");
                    uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context = ctx;
                    uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].rotateLock = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].rotate;
                    if (uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid] && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid] && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].update === true) {
                      this._updataUChart(cid);
                    } else {
                      canvas.width = data.width * this.pixel;
                      canvas.height = data.height * this.pixel;
                      canvas._width = data.width * this.pixel;
                      canvas._height = data.height * this.pixel;
                      setTimeout(() => {
                        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context.restore();
                        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context.save();
                        this._newChart(cid);
                      }, 100);
                    }
                  } else {
                    this.showchart = false;
                    this.mixinDatacomErrorMessage = "参数错误：开启2d模式后，未获取到dom节点，canvas-id:" + cid;
                  }
                });
              } else {
                if (this.inAli) {
                  uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].rotateLock = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].rotate;
                }
                uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context = common_vendor.index.createCanvasContext(cid, this);
                if (uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid] && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid] && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].update === true) {
                  this._updataUChart(cid);
                } else {
                  setTimeout(() => {
                    uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context.restore();
                    uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].context.save();
                    this._newChart(cid);
                  }, 100);
                }
              }
            });
          }
        } else {
          this.mixinDatacomLoading = false;
          this.showchart = false;
          if (this.reshow == true) {
            this.mixinDatacomErrorMessage = "布局错误：未获取到父元素宽高尺寸！canvas-id:" + cid;
          }
        }
      }).exec();
    },
    saveImage() {
      common_vendor.index.canvasToTempFilePath({
        canvasId: this.cid,
        success: (res) => {
          common_vendor.index.saveImageToPhotosAlbum({
            filePath: res.tempFilePath,
            success: function() {
              common_vendor.index.showToast({
                title: "保存成功",
                duration: 2e3
              });
            }
          });
        }
      }, this);
    },
    getImage() {
      if (this.type2d == false) {
        common_vendor.index.canvasToTempFilePath({
          canvasId: this.cid,
          success: (res) => {
            this.emitMsg(new UTSJSONObject({ name: "getImage", params: new UTSJSONObject({ type: "getImage", base64: res.tempFilePath }) }));
          }
        }, this);
      } else {
        const query = common_vendor.index.createSelectorQuery().in(this);
        query.select("#" + this.cid).fields(new UTSJSONObject({ node: true, size: true })).exec((res) => {
          if (res[0]) {
            const canvas = res[0].node;
            this.emitMsg(new UTSJSONObject({ name: "getImage", params: new UTSJSONObject({ type: "getImage", base64: canvas.toDataURL("image/png") }) }));
          }
        });
      }
    },
    _newChart(cid = null) {
      if (this.mixinDatacomLoading == true) {
        return null;
      }
      this.showchart = true;
      uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid] = new uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_uCharts.uCharts(uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid]);
      uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].addEventListener("renderComplete", () => {
        this.emitMsg(new UTSJSONObject({ name: "complete", params: new UTSJSONObject({ type: "complete", complete: true, id: cid }) }));
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].delEventListener("renderComplete");
      });
      uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].addEventListener("scrollLeft", () => {
        this.emitMsg(new UTSJSONObject({ name: "scrollLeft", params: new UTSJSONObject({ type: "scrollLeft", scrollLeft: true, id: cid }) }));
      });
      uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].addEventListener("scrollRight", () => {
        this.emitMsg(new UTSJSONObject({ name: "scrollRight", params: new UTSJSONObject({ type: "scrollRight", scrollRight: true, id: cid }) }));
      });
    },
    _updataUChart(cid = null) {
      uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].updateData(uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid]);
    },
    _tooltipDefault(item = null, category = null, index = null, opts = null) {
      if (category) {
        let data = item.data;
        if (typeof item.data === "object") {
          data = item.data.value;
        }
        return category + " " + item.name + ":" + data;
      } else {
        if (item.properties && item.properties.name) {
          return item.properties.name;
        } else {
          return item.name + ":" + item.data;
        }
      }
    },
    _showTooltip(e = null) {
      let cid = this.cid;
      let tc = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipCustom;
      if (tc && tc !== void 0 && tc !== null) {
        let offset = void 0;
        if (tc.x >= 0 && tc.y >= 0) {
          offset = { x: tc.x, y: tc.y + 10 };
        }
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].showToolTip(e, new UTSJSONObject({
          index: tc.index,
          offset,
          textList: tc.textList,
          formatter: (item = null, category = null, index = null, opts = null) => {
            if (typeof uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat === "string" && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.formatter[uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat]) {
              return uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.formatter[uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat](item, category, index, opts);
            } else {
              return this._tooltipDefault(item, category, index, opts);
            }
          }
        }));
      } else {
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].showToolTip(e, new UTSJSONObject({
          formatter: (item = null, category = null, index = null, opts = null) => {
            if (typeof uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat === "string" && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.formatter[uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat]) {
              return uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.formatter[uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].tooltipFormat](item, category, index, opts);
            } else {
              return this._tooltipDefault(item, category, index, opts);
            }
          }
        }));
      }
    },
    _tap(e = null, move = null) {
      let cid = this.cid;
      let currentIndex = null;
      let legendIndex = null;
      if (this.inScrollView === true || this.inAli) {
        common_vendor.index.createSelectorQuery().in(this).select("#ChartBoxId" + cid).boundingClientRect((data = null) => {
          e.changedTouches = [];
          if (this.inAli) {
            e.changedTouches.unshift(new UTSJSONObject({ x: e.detail.clientX - data.left, y: e.detail.clientY - data.top }));
          } else {
            e.changedTouches.unshift(new UTSJSONObject({ x: e.detail.x - data.left, y: e.detail.y - data.top - this.pageScrollTop }));
          }
          if (move) {
            if (this.tooltipShow === true) {
              this._showTooltip(e);
            }
          } else {
            currentIndex = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].getCurrentDataIndex(e);
            legendIndex = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].getLegendDataIndex(e);
            if (this.tapLegend === true) {
              uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].touchLegend(e);
            }
            if (this.tooltipShow === true) {
              this._showTooltip(e);
            }
            this.emitMsg(new UTSJSONObject({ name: "getIndex", params: new UTSJSONObject({ type: "getIndex", event: new UTSJSONObject({ x: e.detail.x - data.left, y: e.detail.y - data.top }), currentIndex, legendIndex, id: cid, opts: uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].opts }) }));
          }
        }).exec();
      } else {
        if (move) {
          if (this.tooltipShow === true) {
            this._showTooltip(e);
          }
        } else {
          e.changedTouches = [];
          e.changedTouches.unshift(new UTSJSONObject({ x: e.detail.x - e.currentTarget.offsetLeft, y: e.detail.y - e.currentTarget.offsetTop }));
          currentIndex = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].getCurrentDataIndex(e);
          legendIndex = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].getLegendDataIndex(e);
          if (this.tapLegend === true) {
            uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].touchLegend(e);
          }
          if (this.tooltipShow === true) {
            this._showTooltip(e);
          }
          this.emitMsg(new UTSJSONObject({ name: "getIndex", params: new UTSJSONObject({ type: "getIndex", event: new UTSJSONObject({ x: e.detail.x, y: e.detail.y - e.currentTarget.offsetTop }), currentIndex, legendIndex, id: cid, opts: uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].opts }) }));
        }
      }
    },
    _touchStart(e = null) {
      let cid = this.cid;
      lastMoveTime = Date.now();
      if (uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].enableScroll === true && e.touches.length == 1) {
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].scrollStart(e);
      }
      this.emitMsg(new UTSJSONObject({ name: "getTouchStart", params: new UTSJSONObject({ type: "touchStart", event: e.changedTouches[0], id: cid }) }));
    },
    _touchMove(e = null) {
      let cid = this.cid;
      let currMoveTime = Date.now();
      let duration = currMoveTime - lastMoveTime;
      let touchMoveLimit = uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].touchMoveLimit || 24;
      if (duration < Math.floor(1e3 / touchMoveLimit))
        return null;
      lastMoveTime = currMoveTime;
      if (uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].enableScroll === true && e.changedTouches.length == 1) {
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].scroll(e);
      }
      if (this.ontap === true && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].enableScroll === false && this.onmovetip === true) {
        this._tap(e, true);
      }
      if (this.ontouch === true && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].enableScroll === true && this.onzoom === true && e.changedTouches.length == 2) {
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].dobuleZoom(e);
      }
      this.emitMsg(new UTSJSONObject({ name: "getTouchMove", params: new UTSJSONObject({ type: "touchMove", event: e.changedTouches[0], id: cid }) }));
    },
    _touchEnd(e = null) {
      let cid = this.cid;
      if (uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].enableScroll === true && e.touches.length == 0) {
        uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.instance[cid].scrollEnd(e);
      }
      this.emitMsg(new UTSJSONObject({ name: "getTouchEnd", params: new UTSJSONObject({ type: "touchEnd", event: e.changedTouches[0], id: cid }) }));
      if (this.ontap === true && uni_modules_mrsongCharts_components_mrsongCharts_qiunDataCharts_js_sdk_uCharts_configUcharts.cfu.option[cid].enableScroll === false && this.onmovetip === true) {
        this._tap(e, true);
      }
    },
    _error(e = null) {
      this.mixinDatacomErrorMessage = e.detail.errMsg;
    },
    emitMsg(msg = null) {
      this.$emit(msg.name, msg.params);
    },
    getRenderType() {
      if (this.echarts === true && this.mixinDatacomLoading === false) {
        this.beforeInit();
      }
    },
    toJSON() {
      return this;
    }
  }
});
if (!Array) {
  const _easycom_qiun_loading2 = common_vendor.resolveComponent("qiun-loading");
  const _easycom_qiun_error2 = common_vendor.resolveComponent("qiun-error");
  (_easycom_qiun_loading2 + _easycom_qiun_error2)();
}
const _easycom_qiun_loading = () => "../../../../../../qiun-data-charts/components/qiun-loading/qiun-loading.js";
const _easycom_qiun_error = () => "../../../../../../qiun-data-charts/components/qiun-error/qiun-error.js";
if (!Math) {
  (_easycom_qiun_loading + _easycom_qiun_error)();
}
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.mixinDatacomLoading
  }, _ctx.mixinDatacomLoading ? {
    b: common_vendor.p({
      loadingType: $props.loadingType
    })
  } : {}, {
    c: _ctx.mixinDatacomErrorMessage && $props.errorShow
  }, _ctx.mixinDatacomErrorMessage && $props.errorShow ? {
    d: common_vendor.p({
      errorMessage: $props.errorMessage
    }),
    e: common_vendor.o((...args) => $options.reloading && $options.reloading(...args))
  } : {}, {
    f: $data.type2d
  }, $data.type2d ? common_vendor.e({
    g: $props.ontouch
  }, $props.ontouch ? {
    h: common_vendor.sei($data.cid, "canvas"),
    i: $data.cid,
    j: $data.cWidth + "px",
    k: $data.cHeight + "px",
    l: $props.background,
    m: $data.disScroll,
    n: common_vendor.o((...args) => $options._touchStart && $options._touchStart(...args)),
    o: common_vendor.o((...args) => $options._touchMove && $options._touchMove(...args)),
    p: common_vendor.o((...args) => $options._touchEnd && $options._touchEnd(...args)),
    q: common_vendor.o((...args) => $options._error && $options._error(...args)),
    r: $data.showchart,
    s: common_vendor.o((...args) => $options._tap && $options._tap(...args))
  } : {}, {
    t: !$props.ontouch
  }, !$props.ontouch ? {
    v: common_vendor.sei($data.cid, "canvas"),
    w: $data.cid,
    x: $data.cWidth + "px",
    y: $data.cHeight + "px",
    z: $props.background,
    A: $data.disScroll,
    B: common_vendor.o((...args) => $options._error && $options._error(...args)),
    C: $data.showchart,
    D: common_vendor.o((...args) => $options._tap && $options._tap(...args))
  } : {}) : {}, {
    E: !$data.type2d
  }, !$data.type2d ? common_vendor.e({
    F: $props.ontouch
  }, $props.ontouch ? common_vendor.e({
    G: $data.showchart
  }, $data.showchart ? {
    H: common_vendor.sei($data.cid, "canvas"),
    I: $data.cid,
    J: $data.cWidth + "px",
    K: $data.cHeight + "px",
    L: $props.background,
    M: common_vendor.o((...args) => $options._touchStart && $options._touchStart(...args)),
    N: common_vendor.o((...args) => $options._touchMove && $options._touchMove(...args)),
    O: common_vendor.o((...args) => $options._touchEnd && $options._touchEnd(...args)),
    P: $data.disScroll,
    Q: common_vendor.o((...args) => $options._error && $options._error(...args))
  } : {}, {
    R: common_vendor.o((...args) => $options._tap && $options._tap(...args))
  }) : {}, {
    S: !$props.ontouch
  }, !$props.ontouch ? common_vendor.e({
    T: $data.showchart
  }, $data.showchart ? {
    U: common_vendor.sei($data.cid, "canvas"),
    V: $data.cid,
    W: $data.cWidth + "px",
    X: $data.cHeight + "px",
    Y: $props.background,
    Z: $data.disScroll,
    aa: common_vendor.o((...args) => $options._tap && $options._tap(...args)),
    ab: common_vendor.o((...args) => $options._error && $options._error(...args))
  } : {}) : {}) : {}, {
    ac: common_vendor.sei(common_vendor.gei(_ctx, "ChartBoxId" + $data.cid), "view")
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-9d887d5d"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../../../../.sourcemap/mp-weixin/uni_modules/mrsong-charts/components/mrsong-charts/qiun-data-charts/components/qiun-data-charts/qiun-data-charts.js.map
