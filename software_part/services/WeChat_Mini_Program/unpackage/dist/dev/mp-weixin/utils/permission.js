"use strict";const r=require("../common/constants.js");function t(n,i){if(!n)return!1;const s=n.role;if(s&&s===i)return!0;const e=n.roles;return Array.isArray(e)?e.includes(i):!1}function o(n){return t(n,r.USER_ROLES.PRIMARY_ADMIN)||t(n,r.USER_ROLES.SECONDARY_ADMIN)||t(n,r.USER_ROLES.ADMIN)}exports.isAdmin=o;
//# sourceMappingURL=../../.sourcemap/mp-weixin/utils/permission.js.map
