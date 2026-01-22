"use strict";
const common_vendor = require("../../common/vendor.js");
const api_music = require("../../api/music.js");
const utils_permission = require("../../utils/permission.js");
const utils_validator = require("../../utils/validator.js");
const stores_auth = require("../../stores/auth.js");
if (!Array) {
  const _easycom_u_search_1 = common_vendor.resolveComponent("u-search");
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_input_1 = common_vendor.resolveComponent("u-input");
  const _easycom_u_form_item_1 = common_vendor.resolveComponent("u-form-item");
  const _easycom_u_form_1 = common_vendor.resolveComponent("u-form");
  const _easycom_u_modal_1 = common_vendor.resolveComponent("u-modal");
  (_easycom_u_search_1 + _easycom_u_button_1 + _easycom_u_input_1 + _easycom_u_form_item_1 + _easycom_u_form_1 + _easycom_u_modal_1)();
}
const _easycom_u_search = () => "../../uni_modules/uview-plus/components/u-search/u-search.js";
const _easycom_u_button = () => "../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_input = () => "../../uni_modules/uview-plus/components/u-input/u-input.js";
const _easycom_u_form_item = () => "../../uni_modules/uview-plus/components/u-form-item/u-form-item.js";
const _easycom_u_form = () => "../../uni_modules/uview-plus/components/u-form/u-form.js";
const _easycom_u_modal = () => "../../uni_modules/uview-plus/components/u-modal/u-modal.js";
if (!Math) {
  (_easycom_u_search + _easycom_u_button + common_vendor.unref(EmptyState) + _easycom_u_input + _easycom_u_form_item + _easycom_u_form + _easycom_u_modal)();
}
const EmptyState = () => "../../components/EmptyState/EmptyState.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "music",
  setup(__props) {
    const auth = stores_auth.useAuthStore();
    const isAdminUser = common_vendor.computed(() => {
      return utils_permission.isAdmin(auth.user);
    });
    const keyword = common_vendor.ref("");
    const loading = common_vendor.ref(false);
    const tracks = common_vendor.ref([]);
    const createVisible = common_vendor.ref(false);
    const createForm = common_vendor.reactive(new UTSJSONObject({
      title: "",
      url: ""
    }));
    common_vendor.onShow(() => {
      auth.initFromStorage();
      refresh();
    });
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (loading.value)
          return Promise.resolve(null);
        loading.value = true;
        try {
          tracks.value = yield api_music.listTracks(keyword.value);
        } catch (e) {
          common_vendor.index.showToast({ title: "加载失败", icon: "none" });
        } finally {
          loading.value = false;
        }
      });
    }
    function copyUrl(url) {
      common_vendor.index.setClipboardData({
        data: url || "",
        success: () => {
          return common_vendor.index.showToast({ title: "已复制", icon: "none" });
        }
      });
    }
    function openCreate() {
      createForm.title = "";
      createForm.url = "";
      createVisible.value = true;
    }
    function confirmCreate() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!utils_validator.required(createForm.title) || !utils_validator.required(createForm.url)) {
          common_vendor.index.showToast({ title: "请填写标题和URL", icon: "none" });
          return Promise.resolve(null);
        }
        try {
          yield api_music.createTrack(new UTSJSONObject({ title: createForm.title, url: createForm.url }));
          common_vendor.index.showToast({ title: "已创建", icon: "none" });
          yield refresh();
        } catch (e) {
          common_vendor.index.showToast({ title: "创建失败", icon: "none" });
        }
      });
    }
    function onDelete(t = null) {
      common_vendor.index.showModal(new UTSJSONObject({
        title: "确认删除",
        content: `删除音乐 ${t === null || t === void 0 ? null : t.title} ?`,
        success: (res = null) => {
          return common_vendor.__awaiter(this, void 0, void 0, function* () {
            if (!(res === null || res === void 0 ? null : res.confirm))
              return Promise.resolve(null);
            try {
              yield api_music.deleteTrack(t.id);
              common_vendor.index.showToast({ title: "已删除", icon: "none" });
              yield refresh();
            } catch (e) {
              common_vendor.index.showToast({ title: "删除失败", icon: "none" });
            }
          });
        }
      }));
    }
    return (_ctx, _cache) => {
      "raw js";
      const __returned__ = common_vendor.e({
        a: common_vendor.o(($event) => {
          return common_vendor.isRef(keyword) ? keyword.value = $event : null;
        }),
        b: common_vendor.p({
          placeholder: "搜索音乐",
          showAction: false,
          modelValue: common_vendor.unref(keyword)
        }),
        c: common_vendor.o(refresh),
        d: common_vendor.p({
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        e: common_vendor.unref(isAdminUser)
      }, common_vendor.unref(isAdminUser) ? {
        f: common_vendor.o(openCreate),
        g: common_vendor.p({
          type: "primary",
          size: "mini"
        })
      } : {}, {
        h: common_vendor.f(common_vendor.unref(tracks), (t, k0, i0) => {
          return common_vendor.e({
            a: common_vendor.t(t.title)
          }, common_vendor.unref(isAdminUser) ? {
            b: common_vendor.o(() => {
              return onDelete(t);
            }, t.id),
            c: "03b2dd7c-3-" + i0,
            d: common_vendor.p({
              size: "mini",
              type: "error",
              plain: true
            })
          } : {}, {
            e: common_vendor.t(t.url),
            f: common_vendor.o(() => {
              return copyUrl(t.url);
            }, t.id),
            g: "03b2dd7c-4-" + i0,
            h: t.id
          });
        }),
        i: common_vendor.unref(isAdminUser),
        j: common_vendor.p({
          size: "mini"
        }),
        k: !common_vendor.unref(loading) && common_vendor.unref(tracks).length === 0
      }, !common_vendor.unref(loading) && common_vendor.unref(tracks).length === 0 ? {
        l: common_vendor.p({
          text: "暂无音乐"
        })
      } : {}, {
        m: common_vendor.o(($event) => {
          return common_vendor.unref(createForm).title = $event;
        }),
        n: common_vendor.p({
          placeholder: "必填",
          clearable: true,
          modelValue: common_vendor.unref(createForm).title
        }),
        o: common_vendor.p({
          label: "标题",
          labelWidth: "72"
        }),
        p: common_vendor.o(($event) => {
          return common_vendor.unref(createForm).url = $event;
        }),
        q: common_vendor.p({
          placeholder: "必填",
          clearable: true,
          modelValue: common_vendor.unref(createForm).url
        }),
        r: common_vendor.p({
          label: "URL",
          labelWidth: "72"
        }),
        s: common_vendor.p({
          model: common_vendor.unref(createForm)
        }),
        t: common_vendor.o(confirmCreate),
        v: common_vendor.o(($event) => {
          return common_vendor.isRef(createVisible) ? createVisible.value = $event : null;
        }),
        w: common_vendor.p({
          title: "新增音乐",
          showCancelButton: true,
          show: common_vendor.unref(createVisible)
        }),
        x: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/pages/music/music.js.map
