"use strict";
const common_constants = require("../common/constants.js");
function hasRole(user, role) {
  if (!user)
    return false;
  const r = user.role;
  if (r && r === role)
    return true;
  const roles = user.roles;
  if (Array.isArray(roles))
    return roles.includes(role);
  return false;
}
function isAdmin(user) {
  return hasRole(user, common_constants.USER_ROLES.PRIMARY_ADMIN) || hasRole(user, common_constants.USER_ROLES.SECONDARY_ADMIN) || hasRole(user, common_constants.USER_ROLES.ADMIN);
}
exports.isAdmin = isAdmin;
//# sourceMappingURL=../../.sourcemap/mp-weixin/utils/permission.js.map
