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
const pagesWorkflow = fs.readFileSync(path.join(__dirname, "../.github/workflows/pages.yml"), "utf8");
assert.ok(pagesWorkflow.includes("name: github-pages"));
assert.ok(pagesWorkflow.includes("path: ${{ runner.temp }}/artifact.tar"));
assert.ok(pagesWorkflow.includes("overwrite: true"));
assert.ok(pagesWorkflow.includes("artifact_name: github-pages"));
assert.ok(pagesWorkflow.includes("Archive site for GitHub Pages"));
assert.doesNotMatch(pagesWorkflow, /upload-pages-artifact/);
const releaseFallback = "https://github.com/TEaG3/newday/releases/download/arabic-v1.3.6/TEa.G3-Complete-Package.zip";
for (const page of ["index.html", "opl-lab.html", "manual.html"]) {
  const html = fs.readFileSync(path.join(__dirname, "../site", page), "utf8");
  assert.ok(html.includes(releaseFallback), page + " must have the current verified package as its no-JS fallback");
  assert.doesNotMatch(html, /arabic-v1\.3\.5|\/releases\/latest/, page + " must not fall back to a stale or stable-only release URL");
}

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
      } else if (format === "zso" && source === "hdl") {
        expected = "zso-hdl-unsupported";
      } else if (format === "zso" && preference === "neutrino") {
        expected = "zso-requires-opl";
      } else if (format === "zso" && (source === "usb" || source === "bdm")) {
        expected = "opl-bdm";
      } else if (format === "zso" && source === "smb") {
        expected = "opl-eth";
      } else if (format === "zso") {
        expected = "zso-requires-opl";
      } else if (preference === "neutrino") {
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
      const blocked = expected === "unsupported" || expected.startsWith("zso-");
      assert.equal(actual.ok, !blocked, JSON.stringify(input));
      tested += 1;
    }
  }
}
assert.equal(resolveRoute({ format: "vcd", source: "usb", preference: "auto" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "ata", preference: "opl" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "smb", preference: "auto", network: "missing" }).id, "neutrino-smb-fallback");
assert.equal(resolveRoute({ format: "zso", source: "smb", preference: "auto", network: "missing" }).id, "zso-smb-config-invalid");
assert.equal(resolveRoute({ format: "zso", source: "usb", preference: "auto", oplRuntime: "missing" }).id, "zso-opl-missing");
assert.equal(resolveRoute({ format: "zso", source: "usb", preference: "neutrino" }).id, "zso-requires-opl");
assert.equal(resolveRoute({ format: "zso", source: "hdl", preference: "opl" }).id, "zso-hdl-unsupported");
assert.equal(resolveRoute({ format: "zso", source: "other", preference: "auto" }).id, "zso-requires-opl");
assert.equal(resolveRoute({ format: "iso", source: "usb", preference: "opl", oplRuntime: "missing" }).id, "neutrino-opl-missing");
assert.equal(resolveRoute({ format: "ul", source: "usb", preference: "auto", oplRuntime: "missing" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "other", preference: "opl" }).fallback, true);
const readme = fs.readFileSync(path.join(__dirname, "../README.md"), "utf8");
assert.match(readme, /ZSO is routed only through ready OPL BDM\/ETH adapters/);
const engineDocs = fs.readFileSync(path.join(__dirname, "../docs/opl-engine-integration.md"), "utf8");
assert.match(engineDocs, /ZSO is OPL-only in this build/);
assert.doesNotMatch(engineDocs, /keep ordinary ISO\/ZSO on Neutrino/);
const manual = fs.readFileSync(path.join(__dirname, "../site/manual.html"), "utf8");
assert.match(manual, /ZSO is OPL-only/);
assert.doesNotMatch(manual, /auto uses Neutrino for ordinary ISO\/ZSO/);
for (const page of ["index.html", "opl-lab.html"]) {
  const html = fs.readFileSync(path.join(__dirname, "../site", page), "utf8");
  assert.match(html, /Embedded OPL · BDM direct boot/);
  assert.doesNotMatch(html, /Auto keeps ordinary ISO\/ZSO games on Neutrino/);
}
assert.doesNotMatch(i18nSource, /The regular ISO\/ZSO auto route remains Neutrino/);
const smokePlan = fs.readFileSync(path.join(__dirname, "PCSX2_SMOKE_TEST.md"), "utf8");
assert.match(smokePlan, /motor = auto[\\s\\S]*OPL BDM/);
assert.match(smokePlan, /motor = neutrino[\\s\\S]*rejected/i);
assert.doesNotMatch(smokePlan, /Neutrino ISO\\/ZSO/);
console.log("site OPL route matrix ok: " + tested + " format/source/preference combinations + ZSO fail-closed, ISO fallback and documentation checks");
