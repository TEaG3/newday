const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { resolveRoute } = require("../site/opl-demo.js");

const runtimeSources = ["i18n.js", "opl-demo.js", "release.js", "themes.js"]
  .map(name => fs.readFileSync(path.join(__dirname, "../site", name), "utf8"))
  .join(" ");
assert.match(runtimeSources, /tea-g3:languagechange/);
assert.doesNotMatch(runtimeSources, /nuld:languagechange/);
const i18nSource = fs.readFileSync(path.join(__dirname, "../site/i18n.js"), "utf8");
assert.ok(i18nSource.includes('"[data-teag3-lang]"'));
assert.ok(i18nSource.includes("b.dataset.teag3Lang"));
assert.ok(i18nSource.includes('const languageKey = "tea-g3-site-lang"'));
assert.ok(i18nSource.includes('const savedLanguage = localStorage.getItem(languageKey)'));
assert.ok(i18nSource.includes('localStorage.removeItem(legacyLanguageKey)'));
assert.ok(i18nSource.includes('localStorage.setItem(languageKey, lang)'));
const siteCss = fs.readFileSync(path.join(__dirname, "../site/site.css"), "utf8");
assert.match(siteCss, /[.]tea-g3-rtl/);
assert.doesNotMatch(siteCss, /[.]nuld-rtl/);

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
assert.equal(resolveRoute({ format: "iso", source: "smb", preference: "opl", network: "missing" }).id, "neutrino-smb-fallback");
assert.equal(resolveRoute({ format: "iso", source: "usb", preference: "opl", oplRuntime: "missing" }).id, "neutrino-opl-missing");
assert.equal(resolveRoute({ format: "ul", source: "usb", preference: "auto", oplRuntime: "missing" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "other", preference: "opl" }).fallback, true);
console.log("site OPL route matrix ok: " + tested + " format/source/preference combinations + 6 invalid, runtime and fallback checks");
