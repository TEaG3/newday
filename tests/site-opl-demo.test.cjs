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
assert.ok(i18nSource.includes("legacy settings from orbit/ or nuld/ into tea_g3/"));
assert.ok(!i18nSource.includes("moves your settings from orbit/ to nuld/"));
assert.ok(i18nSource.includes("mass0:/tea_g3/themes/"));
assert.ok(!i18nSource.includes("mass0:/nuld/themes/"));
assert.ok(i18nSource.includes("└── tea_g3/"));
assert.ok(!i18nSource.includes("└── nuld/"));
assert.ok(i18nSource.includes("TEa.G3 can also run from an MMCE"));
assert.ok(!i18nSource.includes("NULD can also run from an MMCE"));
const pagesWorkflow = fs.readFileSync(path.join(__dirname, "../.github/workflows/pages.yml"), "utf8");
assert.ok(pagesWorkflow.includes("name: github-pages"));
assert.ok(pagesWorkflow.includes("path: ${{ runner.temp }}/artifact.tar"));
assert.ok(pagesWorkflow.includes("overwrite: true"));
assert.ok(pagesWorkflow.includes("artifact_name: github-pages"));
assert.ok(pagesWorkflow.includes("actions: read"), "Pages deploy must have read-only artifact metadata permission");
assert.ok(pagesWorkflow.includes("Wait for GitHub Pages artifact metadata"));
assert.ok(pagesWorkflow.includes("GITHUB_RUN_ID}/artifacts?per_page=100"));
assert.ok(pagesWorkflow.includes("sleep 5"), "Pages deploy must wait for artifact metadata propagation");
assert.ok(pagesWorkflow.includes("Archive site for GitHub Pages"));
assert.doesNotMatch(pagesWorkflow, /upload-pages-artifact/);
const releaseFallback = "https://github.com/TEaG3/newday/releases/download/arabic-v1.3.6/TEa.G3-Complete-Package.zip";
for (const page of ["index.html", "opl-lab.html", "manual.html"]) {
  const html = fs.readFileSync(path.join(__dirname, "../site", page), "utf8");
  assert.ok(html.includes(releaseFallback), page + " must have the current verified package as its no-JS fallback");
  assert.doesNotMatch(html, /arabic-v1\.3\.5|\/releases\/latest/, page + " must not fall back to a stale or stable-only release URL");
}

const homepage = fs.readFileSync(path.join(__dirname, "../site/index.html"), "utf8");
assert.ok(homepage.includes('href="https://github.com/TEaG3/newday/releases"'), "homepage must keep a separate link to the complete historical release archive");
assert.ok(homepage.includes("historical ORBIT/NULD releases"), "archive link must identify the legacy release history");

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
      if (preference === "neutrino") {
        expected = "engine-removed";
      } else if (format === "ul") {
        expected = source === "usb" ? "opl-ul" : "unsupported-source";
      } else if (format === "zso" && source === "hdl") {
        expected = "zso-hdl-unsupported";
      } else if (source === "usb" || source === "bdm") {
        expected = "opl-bdm";
      } else if (source === "smb") {
        expected = "opl-eth";
      } else if (source === "hdl") {
        expected = "opl-hdd";
      } else if (format === "zso") {
        expected = "zso-requires-opl";
      } else {
        expected = "unsupported-source";
      }
      const input = { format, source, preference };
      const actual = resolveRoute(input);
      assert.equal(actual.id, expected, JSON.stringify(input));
      assert.equal(actual.ok, expected.startsWith("opl-"), JSON.stringify(input));
      tested += 1;
    }
  }
}
assert.equal(resolveRoute({ format: "vcd", source: "usb", preference: "auto" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "ata", preference: "opl" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "smb", preference: "auto", network: "missing" }).id, "opl-network-config-invalid");
assert.equal(resolveRoute({ format: "zso", source: "smb", preference: "opl", network: "missing" }).id, "opl-network-config-invalid");
assert.equal(resolveRoute({ format: "zso", source: "usb", preference: "auto", oplRuntime: "missing" }).id, "opl-runtime-missing");
assert.equal(resolveRoute({ format: "zso", source: "hdl", preference: "opl" }).id, "zso-hdl-unsupported");
assert.equal(resolveRoute({ format: "zso", source: "other", preference: "auto" }).id, "zso-requires-opl");
assert.equal(resolveRoute({ format: "iso", source: "usb", preference: "opl", oplRuntime: "missing" }).id, "opl-runtime-missing");
assert.equal(resolveRoute({ format: "ul", source: "usb", preference: "auto", oplRuntime: "missing" }).ok, false);
assert.equal(resolveRoute({ format: "iso", source: "other", preference: "opl" }).fallback, undefined);
const readme = fs.readFileSync(path.join(__dirname, "../README.md"), "utf8");
assert.match(readme, /OPL is the only PS2 game engine/);
const engineDocs = fs.readFileSync(path.join(__dirname, "../docs/opl-engine-integration.md"), "utf8");
assert.match(engineDocs, /OPL is the only PS2 game engine/);
assert.match(engineDocs, /TEaG3\/OPL/);
assert.match(engineDocs, /46bad5a4a469c51e45eb1ed879fe43c100b8cb7b/);
assert.match(engineDocs, /RTL=1/);
const formatsDocs = fs.readFileSync(path.join(__dirname, "../docs/game-formats.md"), "utf8");
assert.match(formatsDocs, /TEaG3\/OPL/);
assert.doesNotMatch(formatsDocs, /checks out Open PS2 Loader at/);
const smbDocs = fs.readFileSync(path.join(__dirname, "../docs/smb.md"), "utf8");
assert.match(smbDocs, /OPL ETH/);
assert.match(smbDocs, /OPL ETH direct launch/);
assert.doesNotMatch(smbDocs, /Neutrino.*fallback|Neutrino.*ISO-only|ZSO is never sent to Neutrino/);
assert.match(smbDocs, /USBExtreme UL.*USB-only/);
assert.match(smbDocs, /dopen.*dclose.*dread/);
assert.match(smbDocs, /TEAG3\.GAMES/);
assert.doesNotMatch(smbDocs, /does not enumerate directories/);
const smbPatch = fs.readFileSync(path.join(__dirname, "../tools/patch_smb_ioman.py"), "utf8");
const smbBuilder = fs.readFileSync(path.join(__dirname, "../tools/build_smb_support.sh"), "utf8");
const launcherSource = fs.readFileSync(path.join(__dirname, "../launcher.c"), "utf8");
assert.match(smbBuilder, /patch_smb_ioman\.py/);
assert.match(smbBuilder, /make -C iop\/smap/);
assert.match(smbBuilder, /make -C iop\/ministack/);
assert.match(smbBuilder, /make -C iop\/smb SMB_IOMAN=1/);
assert.doesNotMatch(smbBuilder, /make -C ee/);
assert.match(smbPatch, /static int smb_dev_dopen/);
assert.match(smbPatch, /static int smb_dev_dclose/);
assert.match(smbPatch, /static int smb_dev_dread/);
assert.match(smbPatch, /USMB2_FEATURE_OPENDIR/);
assert.match(launcherSource, /fopen\("smb:\/TEAG3\.GAMES", "rb"\)/);
assert.match(engineDocs, /gfx_resume\(\)/);
assert.doesNotMatch(engineDocs, /No automatic return to the full TEa\.G3 UI is claimed/);
assert.doesNotMatch(engineDocs, /keep ordinary ISO\/ZSO on Neutrino/);
const manual = fs.readFileSync(path.join(__dirname, "../site/manual.html"), "utf8");
assert.match(manual, /ZSO is routed only through OPL/);
assert.doesNotMatch(manual, /auto uses Neutrino for ordinary ISO\/ZSO/);
for (const page of ["index.html", "opl-lab.html"]) {
  const html = fs.readFileSync(path.join(__dirname, "../site", page), "utf8");
  assert.match(html, /Embedded OPL · BDM direct boot/);
  assert.match(html, /option value="opl" selected>opl/);
  assert.equal((html.match(/<option value="opl"/g) || []).length, 1);
  assert.doesNotMatch(html, /option value="auto" selected>auto/);
  assert.doesNotMatch(html, /Auto keeps ordinary ISO\/ZSO games on Neutrino/);
}
assert.doesNotMatch(i18nSource, /The regular ISO\/ZSO auto route remains Neutrino/);
const demoSource = fs.readFileSync(path.join(__dirname, "../site/opl-demo.js"), "utf8");
assert.match(demoSource, /form\.elements\.preference\.value = "opl"/);
const smokePlan = fs.readFileSync(path.join(__dirname, "PCSX2_SMOKE_TEST.md"), "utf8");
assert.ok(smokePlan.includes("motor = opl") && smokePlan.includes("OPL BDM"));
assert.ok(smokePlan.includes("motor = neutrino") && /rejected/i.test(smokePlan));
assert.ok(!smokePlan.includes("Neutrino ISO/ZSO"));
const pcsx2Readme = fs.readFileSync(path.join(__dirname, "../tools/pcsx2/README.md"), "utf8");
assert.ok(pcsx2Readme.includes("TEa.G3.ELF"));
assert.ok(pcsx2Readme.includes("tea_g3/"));
assert.ok(pcsx2Readme.includes("tools/pcsx2/work/usb/TEa.G3.ELF"));
assert.doesNotMatch(pcsx2Readme, /launcher\.elf|orbit\//i);
const pcsx2ImageScript = fs.readFileSync(path.join(__dirname, "../tools/pcsx2/mkimg.sh"), "utf8");
assert.ok(pcsx2ImageScript.includes("-n TEAG3"));
assert.doesNotMatch(pcsx2ImageScript, /ORBIT/);
console.log("site OPL-only route matrix ok: " + tested + " format/source/preference combinations + SMB/ETH and HDD paths, fail-closed Neutrino legacy settings, and emulator-harness checks");
