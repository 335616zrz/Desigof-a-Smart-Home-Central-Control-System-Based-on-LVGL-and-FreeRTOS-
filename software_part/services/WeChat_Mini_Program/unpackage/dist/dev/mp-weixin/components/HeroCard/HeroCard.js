"use strict";
const common_vendor = require("../../common/vendor.js");
if (!Array) {
  const _easycom_u_tag_1 = common_vendor.resolveComponent("u-tag");
  _easycom_u_tag_1();
}
const _easycom_u_tag = () => "../../uni_modules/uview-plus/components/u-tag/u-tag.js";
if (!Math) {
  _easycom_u_tag();
}
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "HeroCard",
  props: {
    title: { type: String, default: "指挥中心" },
    subtitle: { type: String, default: "实时可见 · 可靠可控" },
    mqtt: { type: String, default: "unknown" },
    network: { type: String, default: "unknown" },
    season: { type: String, default: "" },
    time: { type: String, default: "" }
  },
  setup(__props) {
    const props = __props;
    const networkText = common_vendor.computed(() => {
      return props.network || "unknown";
    });
    const seasonText = common_vendor.computed(() => {
      return props.season || "";
    });
    const timeText = common_vendor.computed(() => {
      return props.time || "";
    });
    const mqttTagType = common_vendor.computed(() => {
      const s = String(props.mqtt || "").toLowerCase();
      if (s === "connected")
        return "success";
      if (s === "disconnected")
        return "error";
      return "info";
    });
    const mqttTagText = common_vendor.computed(() => {
      const s = String(props.mqtt || "").toLowerCase();
      if (s === "connected")
        return "MQTT: 已连接";
      if (s === "disconnected")
        return "MQTT: 断开";
      return "MQTT: 未知";
    });
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.t(__props.title),
        b: common_vendor.t(__props.subtitle),
        c: common_vendor.p({
          type: common_vendor.unref(mqttTagType),
          size: "mini",
          plain: true,
          plainFill: true,
          text: common_vendor.unref(mqttTagText)
        }),
        d: common_vendor.p({
          type: "info",
          size: "mini",
          plain: true,
          plainFill: true,
          text: `网络: ${common_vendor.unref(networkText)}`
        }),
        e: common_vendor.unref(seasonText)
      }, common_vendor.unref(seasonText) ? {
        f: common_vendor.p({
          type: "warning",
          size: "mini",
          plain: true,
          plainFill: true,
          text: `季节: ${common_vendor.unref(seasonText)}`
        })
      } : {}, {
        g: common_vendor.unref(timeText)
      }, common_vendor.unref(timeText) ? {
        h: common_vendor.p({
          type: "primary",
          size: "mini",
          plain: true,
          plainFill: true,
          text: common_vendor.unref(timeText)
        })
      } : {}, {
        i: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createComponent(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/components/HeroCard/HeroCard.js.map
