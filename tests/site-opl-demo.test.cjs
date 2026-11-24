const assert = require("node:assert/strict");
const { resolveRoute } = require("../site/opl-demo.js");

const formats = ["iso", "zso", "ul"];
const sources = ["usb", "bdm", "smb", "hdl", "other"];
const preferences = ["auto", "neutrino", "opl"];
let tested = 0;

for (const format of formats) {
  for (const source of sources) {
    for (const preference of preferences) {
      let expected;
      if (format === "ul") {
        expected = source === "usb" ? "opl-ul" : "unsupported";
      } else if (preference !== "opl") {
        expected = "neutrino";
      } else if (source === "usb" || source === "bdm") {
        expected = "opl-bdm";
      } else if (source === "smb") {
        expected = "opl-eth";
      } else if (source === "hdl") {
        expected = "opl-hdd";
      } else {
        expected = "neutrino";
      }
      const input = { format, source, preference };
      const actual = resolveRoute(input);
      assert.equal(actual.id, expected, JSON.stringify(input));
      assert.equal(actual.ok, expected !== "unsupported", JSON.stringify(input));
      tested += 1;
    }
  }
}

assert.equal(resolveRoute({ format: "vcd", source: "usb", preference: "auto" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "ata", preference: "opl" }).ok, false);
console.log("site OPL route matrix ok: " + tested + " format/source/preference combinations + 2 invalid inputs");
