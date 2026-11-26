const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { resolveRoute } = require("../site/opl-demo.js");
const ROOT = path.join(__dirname, "..");
const read = p => fs.readFileSync(path.join(ROOT, p), "utf8");

const runtimeSources = ["i18n.js", "opl-demo.js", "release.js", "themes.js"].map(name => read("site/" + name)).join(" ");
assert.match(runtimeSources, /tea-g3:languagechange/);
assert.doesNotMatch(runtimeSources, /nuld:languagechange/);
const i18nSource = read("site/i18n.js");
assert.ok(i18nSource.includes('"[data-teag3-lang]"'));
assert.ok(i18nSource.includes("b.dataset.teag3Lang"));
assert.ok(i18nSource.includes('const languageKey = "tea-g3-site-lang"'));
assert.ok(i18nSource.includes('localStorage.getItem(languageKey)'));
assert.ok(i18nSource.includes('localStorage.setItem(languageKey, lang)'));
assert.ok(i18nSource.includes("legacy settings from orbit/ or nuld/ into tea_g3/"));
assert.ok(i18nSource.includes("mass0:/tea_g3/themes/"));
const pagesWorkflow = read(".github/workflows/pages.yml");
assert.ok(pagesWorkflow.includes("name: github-pages"));
assert.ok(pagesWorkflow.includes("overwrite: true"));
assert.ok(pagesWorkflow.includes("artifact_name: github-pages"));
assert.ok(pagesWorkflow.includes("Wait for GitHub Pages artifact metadata"));
assert.ok(pagesWorkflow.includes("Archive site for GitHub Pages"));
assert.doesNotMatch(pagesWorkflow, /upload-pages-artifact/);

const fallback = "https://github.com/TEaG3/newday/releases/download/arabic-v1.3.6/TEa.G3-Complete-Package.zip";
for (const page of ["index.html", "opl-lab.html", "manual.html"]) {
  const html = read("site/" + page);
  assert.ok(html.includes(fallback), page + " needs the last verified package as no-JS fallback");
  assert.doesNotMatch(html, /arabic-v1\\.3\\.5/);
  assert.doesNotMatch(html, /name="preference"/);
  assert.match(html, /OPL/);
}
const homepage = read("site/index.html");
assert.ok(homepage.includes('href="https://github.com/TEaG3/newday/releases"'));
assert.ok(homepage.includes("Full release archive (includes earlier versions)"));
assert.doesNotMatch(homepage, /option value="neutrino"/i);
const siteCss = read("site/site.css");
assert.match(siteCss, /[.]tea-g3-rtl/);
assert.doesNotMatch(siteCss, /[.]nuld-rtl/);

const formats = ["iso", "zso", "ul"], sources = ["usb", "bdm", "hdl", "other"];
let tested = 0;
for (const format of formats) for (const source of sources) {
  const expected = format === "ul" ? (source === "usb" ? "opl-ul" : "unsupported") :
    (source === "usb" || source === "bdm") ? "opl-bdm" :
    (source === "hdl" && format === "iso") ? "opl-hdd" : "unsupported";
  const actual = resolveRoute({ format, source, oplRuntime: "ready" });
  assert.equal(actual.id, expected, JSON.stringify({format, source}));
  assert.equal(actual.ok, expected !== "unsupported");
  if (actual.ok) assert.equal(actual.engine, "OPL");
  const missing = resolveRoute({ format, source, oplRuntime: "missing" });
  assert.equal(missing.id, "opl-missing");
  assert.equal(missing.ok, false);
  assert.equal(missing.engine, "none");
  tested++;
}
assert.equal(resolveRoute({ format: "vcd", source: "usb" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "smb" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "mmce" }).ok, false);

const demo = read("site/opl-demo.js");
assert.doesNotMatch(demo, /preference|neutrino/i);
assert.match(demo, /oplRuntime/);
const launcher = read("launcher.c"), engine = read("engine.c"), engineHeader = read("engine.h"), sourcesC = read("sources.c");
assert.match(launcher, /static int crt = 1/);
assert.match(launcher, /motor = opl/);
assert.match(engine, /ENGINE_OPL_BDM/);
assert.match(engineHeader, /ENGINE_UNSUPPORTED/);
assert.match(sourcesC, /SMB discovery is disabled/);
assert.match(sourcesC, /UDPFS game streaming is not supported/);
assert.doesNotMatch(launcher + engine + engineHeader + sourcesC, /\bneutrino\b|\bndir\b/i);
assert.match(read("gfx.c"), /set_mode\(1\)/);
assert.match(read("docs/opl-engine-integration.md"), /OPL is the sole PS2 game engine/);
assert.match(read("README.md"), /OPL هو محرك تشغيل ألعاب PS2 الوحيد/);
assert.ok(!fs.existsSync(path.join(ROOT, "docs/smb.md")));
assert.ok(!fs.existsSync(path.join(ROOT, "tools/build_neutrino.sh")));
assert.ok(!fs.existsSync(path.join(ROOT, "third_party/neutrino-igr")));
console.log("OPL-only site and route matrix ok: " + tested + " format/source combinations plus missing-runtime and fail-closed checks");
