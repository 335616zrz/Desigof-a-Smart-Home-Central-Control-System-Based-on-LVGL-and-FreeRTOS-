"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_components_uSkeleton_props = require("./props.js");
const uni_modules_uviewPlus_libs_mixin_mpMixin = require("../../libs/mixin/mpMixin.js");
const uni_modules_uviewPlus_libs_mixin_mixin = require("../../libs/mixin/mixin.js");
const uni_modules_uviewPlus_libs_function_index = require("../../libs/function/index.js");
const uni_modules_uviewPlus_libs_function_test = require("../../libs/function/test.js");
const _sfc_main = common_vendor.defineComponent({
  name: "u-skeleton",
  mixins: [uni_modules_uviewPlus_libs_mixin_mpMixin.mpMixin, uni_modules_uviewPlus_libs_mixin_mixin.mixin, uni_modules_uviewPlus_components_uSkeleton_props.props],
  data() {
    return {
      width: 0
    };
  },
  watch: {
    loading() {
      this.getComponentWidth();
    }
  },
  computed: {
    rowsArray() {
      if (/%$/.test(this.rowsHeight)) {
        uni_modules_uviewPlus_libs_function_index.error("rowsHeight参数不支持百分比单位");
      }
      const rows = [];
      for (let i = 0; i < this.rows; i++) {
        let item = new UTSJSONObject({}), rowWidth = uni_modules_uviewPlus_libs_function_test.test.array(this.rowsWidth) ? this.rowsWidth[i] || (i === this.rows - 1 ? "70%" : "100%") : i === this.rows - 1 ? "70%" : this.rowsWidth, rowHeight = uni_modules_uviewPlus_libs_function_test.test.array(this.rowsHeight) ? this.rowsHeight[i] || "18px" : this.rowsHeight;
        item.marginTop = !this.title && i === 0 ? 0 : this.title && i === 0 ? "20px" : "12px";
        if (/%$/.test(rowWidth)) {
          item.width = uni_modules_uviewPlus_libs_function_index.addUnit(this.width * parseInt(rowWidth) / 100);
        } else {
          item.width = uni_modules_uviewPlus_libs_function_index.addUnit(rowWidth);
        }
        item.height = uni_modules_uviewPlus_libs_function_index.addUnit(rowHeight);
        rows.push(item);
      }
      return rows;
    },
    uTitleWidth() {
      let tWidth = 0;
      if (/%$/.test(this.titleWidth)) {
        tWidth = uni_modules_uviewPlus_libs_function_index.addUnit(this.width * parseInt(this.titleWidth) / 100);
      } else {
        tWidth = uni_modules_uviewPlus_libs_function_index.addUnit(this.titleWidth);
      }
      return uni_modules_uviewPlus_libs_function_index.addUnit(tWidth);
    }
  },
  mounted() {
    this.init();
  },
  methods: {
    addUnit: uni_modules_uviewPlus_libs_function_index.addUnit,
    init() {
      this.getComponentWidth();
    },
    setNvueAnimation() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
      });
    },
    // 获取组件的宽度
    getComponentWidth() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        yield uni_modules_uviewPlus_libs_function_index.sleep(20);
        this.$uGetRect(".u-skeleton__wrapper__content").then((size = null) => {
          this.width = size.width;
        });
      });
    }
  }
});
function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
  "raw js";
  return common_vendor.e({
    a: _ctx.loading
  }, _ctx.loading ? common_vendor.e({
    b: _ctx.avatar
  }, _ctx.avatar ? {
    c: common_vendor.n(`u-skeleton__wrapper__avatar--${_ctx.avatarShape}`),
    d: common_vendor.n(_ctx.animate && "animate"),
    e: $options.addUnit(_ctx.avatarSize),
    f: $options.addUnit(_ctx.avatarSize)
  } : {}, {
    g: _ctx.title
  }, _ctx.title ? {
    h: $options.uTitleWidth,
    i: $options.addUnit(_ctx.titleHeight),
    j: common_vendor.n(_ctx.animate && "animate")
  } : {}, {
    k: common_vendor.f($options.rowsArray, (item, index, i0) => {
      return {
        a: index,
        b: item.width,
        c: item.height,
        d: item.marginTop
      };
    }),
    l: common_vendor.n(_ctx.animate && "animate"),
    m: common_vendor.sei("r0-b376446f", "view", "u-skeleton__wrapper__content"),
    n: common_vendor.sei("r1-b376446f", "view", "u-skeleton__wrapper")
  }) : {}, {
    o: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
  });
}
const Component = /* @__PURE__ */ common_vendor._export_sfc(_sfc_main, [["render", _sfc_render], ["__scopeId", "data-v-b376446f"]]);
wx.createComponent(Component);
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/components/u-skeleton/u-skeleton.js.map
