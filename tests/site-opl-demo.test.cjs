const assert = require("node:assert/strict");
const { resolveRoute } = require("../site/opl-demo.js");
const cases = [
  [{ format: "iso", source: "usb", preference: "auto" }, "neutrino"],
  [{ format: "zso", source: "usb", preference: "auto" }, "neutrino"],
  [{ format: "ul", source: "usb", preference: "auto" }, "opl-ul"],
  [{ format: "ul", source: "usb", preference: "neutrino" }, "opl-ul"],
  [{ format: "ul", source: "usb", preference: "opl" }, "opl-ul"],
  [{ format: "iso", source: "usb", preference: "opl" }, "opl-usb"],
  [{ format: "zso", source: "usb", preference: "opl" }, "opl-usb"],
  [{ format: "iso", source: "other", preference: "opl" }, "unsupported"],
  [{ format: "zso", source: "other", preference: "opl" }, "unsupported"],
  [{ format: "iso", source: "other", preference: "auto" }, "neutrino"],
  [{ format: "zso", source: "other", preference: "neutrino" }, "neutrino"],
  [{ format: "ul", source: "other", preference: "auto" }, "unsupported"],
  [{ format: "ul", source: "other", preference: "opl" }, "unsupported"]
];
for (const [input, expected] of cases) assert.equal(resolveRoute(input).id, expected, JSON.stringify(input));
assert.equal(resolveRoute({ format: "vcd", source: "usb", preference: "auto" }).ok, false);
console.log("site OPL route matrix ok: 14 cases");
