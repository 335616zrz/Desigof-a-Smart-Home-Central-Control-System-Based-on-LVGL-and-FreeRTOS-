"use strict";
const common_vendor = require("../../common/vendor.js");
const utils_permission = require("../../utils/permission.js");
const utils_validator = require("../../utils/validator.js");
const stores_auth = require("../../stores/auth.js");
const stores_user = require("../../stores/user.js");
if (!Array) {
  const _easycom_u_button_1 = common_vendor.resolveComponent("u-button");
  const _easycom_u_input_1 = common_vendor.resolveComponent("u-input");
  const _easycom_u_form_item_1 = common_vendor.resolveComponent("u-form-item");
  const _easycom_u_form_1 = common_vendor.resolveComponent("u-form");
  const _easycom_u_modal_1 = common_vendor.resolveComponent("u-modal");
  (_easycom_u_button_1 + _easycom_u_input_1 + _easycom_u_form_item_1 + _easycom_u_form_1 + _easycom_u_modal_1)();
}
const _easycom_u_button = () => "../../uni_modules/uview-plus/components/u-button/u-button.js";
const _easycom_u_input = () => "../../uni_modules/uview-plus/components/u-input/u-input.js";
const _easycom_u_form_item = () => "../../uni_modules/uview-plus/components/u-form-item/u-form-item.js";
const _easycom_u_form = () => "../../uni_modules/uview-plus/components/u-form/u-form.js";
const _easycom_u_modal = () => "../../uni_modules/uview-plus/components/u-modal/u-modal.js";
if (!Math) {
  (_easycom_u_button + common_vendor.unref(EmptyState) + _easycom_u_input + _easycom_u_form_item + _easycom_u_form + _easycom_u_modal)();
}
const EmptyState = () => "../../components/EmptyState/EmptyState.js";
const _sfc_main = /* @__PURE__ */ common_vendor.defineComponent({
  __name: "user",
  setup(__props) {
    const auth = stores_auth.useAuthStore();
    const store = stores_user.useUserStore();
    const loading = common_vendor.computed(() => {
      return Boolean(store.loading);
    });
    const users = common_vendor.computed(() => {
      return store.items || [];
    });
    const isAdminUser = common_vendor.computed(() => {
      return utils_permission.isAdmin(auth.user);
    });
    const createVisible = common_vendor.ref(false);
    const createForm = common_vendor.reactive(new UTSJSONObject({
      username: "",
      password: "",
      role: "ROLE_OPERATOR",
      email: ""
    }));
    common_vendor.onShow(() => {
      auth.initFromStorage();
      refresh();
    });
    function refresh() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!isAdminUser.value)
          return Promise.resolve(null);
        yield store.fetchList(1, 20);
      });
    }
    function openCreate() {
      createForm.username = "";
      createForm.password = "";
      createForm.role = "ROLE_OPERATOR";
      createForm.email = "";
      createVisible.value = true;
    }
    function confirmCreate() {
      return common_vendor.__awaiter(this, void 0, void 0, function* () {
        if (!utils_validator.required(createForm.username) || !utils_validator.required(createForm.password) || !utils_validator.required(createForm.role)) {
          common_vendor.index.showToast({ title: "请填写用户名/密码/角色", icon: "none" });
          return Promise.resolve(null);
        }
        try {
          yield store.create(new UTSJSONObject({
            username: createForm.username,
            password: createForm.password,
            role: createForm.role,
            email: createForm.email
          }));
          common_vendor.index.showToast({ title: "已创建", icon: "none" });
          yield refresh();
        } catch (e) {
          common_vendor.index.showToast({ title: "创建失败", icon: "none" });
        }
      });
    }
    function onDelete(u = null) {
      common_vendor.index.showModal(new UTSJSONObject({
        title: "确认删除",
        content: `删除用户 ${u === null || u === void 0 ? null : u.username} ?`,
        success: (res = null) => {
          return common_vendor.__awaiter(this, void 0, void 0, function* () {
            if (!(res === null || res === void 0 ? null : res.confirm))
              return Promise.resolve(null);
            try {
              yield store.remove(u.id);
              common_vendor.index.showToast({ title: "已删除", icon: "none" });
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
        a: common_vendor.o(refresh),
        b: common_vendor.p({
          size: "mini",
          loading: common_vendor.unref(loading)
        }),
        c: common_vendor.unref(isAdminUser)
      }, common_vendor.unref(isAdminUser) ? {
        d: common_vendor.o(openCreate),
        e: common_vendor.p({
          type: "primary",
          size: "mini"
        })
      } : {}, {
        f: !common_vendor.unref(isAdminUser)
      }, !common_vendor.unref(isAdminUser) ? {
        g: common_vendor.p({
          text: "仅管理员可访问"
        })
      } : common_vendor.e({
        h: common_vendor.f(common_vendor.unref(users), (u, k0, i0) => {
          return {
            a: common_vendor.t(u.username),
            b: common_vendor.o(() => {
              return onDelete(u);
            }, u.id),
            c: "14eab76e-3-" + i0,
            d: common_vendor.t(u.role),
            e: common_vendor.t(u.email || "-"),
            f: u.id
          };
        }),
        i: common_vendor.p({
          size: "mini",
          type: "error",
          plain: true
        }),
        j: !common_vendor.unref(loading) && common_vendor.unref(users).length === 0
      }, !common_vendor.unref(loading) && common_vendor.unref(users).length === 0 ? {
        k: common_vendor.p({
          text: "暂无用户"
        })
      } : {}), {
        l: common_vendor.o(($event) => {
          return common_vendor.unref(createForm).username = $event;
        }),
        m: common_vendor.p({
          placeholder: "必填",
          clearable: true,
          modelValue: common_vendor.unref(createForm).username
        }),
        n: common_vendor.p({
          label: "用户名",
          labelWidth: "80"
        }),
        o: common_vendor.o(($event) => {
          return common_vendor.unref(createForm).password = $event;
        }),
        p: common_vendor.p({
          type: "password",
          placeholder: "必填",
          password: true,
          modelValue: common_vendor.unref(createForm).password
        }),
        q: common_vendor.p({
          label: "密码",
          labelWidth: "80"
        }),
        r: common_vendor.o(($event) => {
          return common_vendor.unref(createForm).role = $event;
        }),
        s: common_vendor.p({
          placeholder: "ROLE_OPERATOR",
          clearable: true,
          modelValue: common_vendor.unref(createForm).role
        }),
        t: common_vendor.p({
          label: "角色",
          labelWidth: "80"
        }),
        v: common_vendor.o(($event) => {
          return common_vendor.unref(createForm).email = $event;
        }),
        w: common_vendor.p({
          placeholder: "可选",
          clearable: true,
          modelValue: common_vendor.unref(createForm).email
        }),
        x: common_vendor.p({
          label: "邮箱",
          labelWidth: "80"
        }),
        y: common_vendor.p({
          model: common_vendor.unref(createForm)
        }),
        z: common_vendor.o(confirmCreate),
        A: common_vendor.o(($event) => {
          return common_vendor.isRef(createVisible) ? createVisible.value = $event : null;
        }),
        B: common_vendor.p({
          title: "新增用户",
          showCancelButton: true,
          show: common_vendor.unref(createVisible)
        }),
        C: common_vendor.sei(common_vendor.gei(_ctx, ""), "view")
      });
      return __returned__;
    };
  }
});
wx.createPage(_sfc_main);
//# sourceMappingURL=../../../.sourcemap/mp-weixin/pages/user/user.js.map
