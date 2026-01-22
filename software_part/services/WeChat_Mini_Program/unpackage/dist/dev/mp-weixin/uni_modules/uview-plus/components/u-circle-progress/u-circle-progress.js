"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uCircleProgress_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-circle-progress",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uCircleProgress_props.props],
  data() {
    return {
      leftBorderColor: "rgb(200, 200, 200)",
      rightBorderColor: "rgb(200, 200, 200)"
    };
  },
  computed: {
    leftSyle() {
      const style = new UTSJSONObject({});
      style.borderTopColor = this.leftBorderColor;
      style.borderRightColor = this.leftBorderColor;
      return style;
    },
    rightSyle() {
      const style = new UTSJSONObject({});
      style.borderLeftColor = this.rightBorderColor;
      style.borderBottomColor = this.rightBorderColor;
      return style;
    }
  },
  mounted() {
    uni_modules_uviewPlus_libs_function_index.sleep().then(() => {
      this.rightBorderColor = "rgb(66, 185, 131)";
    });
  },
  methods: {
    init() {
      animation.transition(this.$refs["right-circle"].ref, new UTSJSONObject({
        styles: new UTSJSONObject({
          transform: "rotate(45deg)",
          transformOrigin: "center center"
        })
      }), () => {
        this.rightBorderColor = "rgb(66, 185, 131)";
      });
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return {
    a: common_vendor.sei("r0-8379d4ea", "view", "left-circle"),
    b: common_vendor.s($options.leftSyle),
    c: common_vendor.sei("r1-8379d4ea", "view", "right-circle"),
    d: common_vendor.s($options.rightSyle),
    e: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
  };
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-8379d4ea"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-circle-progress/u-circle-progress.js.map
