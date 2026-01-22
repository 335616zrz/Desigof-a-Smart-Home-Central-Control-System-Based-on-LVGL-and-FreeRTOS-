"use strict";
const common_vendor = require("./vendor.js");
function safeStringify(value) {
  try {
    return JSON.stringify(value);
  } catch (e) {
    return "";
  }
}
function safeParse(raw, fallback) {
  if (!raw)
    return fallback;
  try {
    return JSON.parse(raw);
  } catch (e) {
    return fallback;
  }
}
function setString(key, value) {
  common_vendor.index.setStorageSync(key, value ?? "");
}
function getString(key, fallback = "") {
  const v = common_vendor.index.getStorageSync(key);
  if (v === null || v === void 0 || v === "")
    return fallback;
  return String(v);
}
function setJSON(key, value) {
  common_vendor.index.setStorageSync(key, safeStringify(value));
}
function getJSON(key, fallback = null) {
  const raw = common_vendor.index.getStorageSync(key);
  if (typeof raw === "string")
    return safeParse(raw, fallback);
  if (raw === null || raw === void 0 || raw === "")
    return fallback;
  return raw;
}
function remove(key) {
  common_vendor.index.removeStorageSync(key);
}
exports.getJSON = getJSON;
exports.getString = getString;
exports.remove = remove;
exports.setJSON = setJSON;
exports.setString = setString;
//# sourceMappingURL=../../.sourcemap/mp-weixin/common/storage.js.map
