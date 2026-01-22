"use strict";
function isEmail(value) {
  if (!value)
    return false;
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(String(value));
}
function isStrongPassword(value, minLen = 6) {
  if (!value)
    return false;
  const v = String(value);
  if (v.length < minLen)
    return false;
  const groups = [
    /[a-zA-Z]/.test(v),
    /\d/.test(v),
    /[^a-zA-Z0-9]/.test(v)
  ].filter(Boolean).length;
  return groups >= 2;
}
function required(value) {
  if (value === 0)
    return true;
  return value !== null && value !== void 0 && String(value).trim() !== "";
}
exports.isEmail = isEmail;
exports.isStrongPassword = isStrongPassword;
exports.required = required;
//# sourceMappingURL=../../.sourcemap/mp-weixin/utils/validator.js.map
