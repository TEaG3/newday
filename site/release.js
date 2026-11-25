// Fill every download control from the newest published TEa.G3 package.
// Prefer the newest release, including a prerelease: the project intentionally remains prerelease
// until physical-PS2 acceptance tests have been recorded.
(function () {
  var statusCopy = {
    stable: { en: "Stable release", ar: "إصدار مستقر" },
    pre: { en: "Prerelease · physical-PS2 acceptance is still open", ar: "إصدار تجريبي · اختبار القبول على جهاز PS2 الحقيقي لم يكتمل بعد" },
    fallback: { en: "Release information is temporarily unavailable; open all releases.", ar: "تعذرت قراءة بيانات الإصدار مؤقتاً؛ افتح صفحة جميع الإصدارات." }
  };

  function setStatus(kind) {
    var ar = document.documentElement.lang === "ar";
    var text = statusCopy[kind] || statusCopy.fallback;
    document.querySelectorAll("[data-release-status]").forEach(function (el) {
      el.textContent = ar ? text.ar : text.en;
    });
  }

  function load() {
    return fetch("https://api.github.com/repos/TEaG3/newday/releases?per_page=20", {
      headers: { Accept: "application/vnd.github+json" }
    }).then(function (r) {
      return r.ok ? r.json() : Promise.reject(r.status);
    }).then(function (releases) {
      var candidates = (releases || []).filter(function (item) {
        return !item.draft && (item.assets || []).some(function (asset) {
          return /^TEa\.G3-Complete-Package\.zip$/i.test(asset.name || "");
        });
      });
      if (!candidates.length) return Promise.reject("No published TEa.G3 package with the expected asset");

      // GitHub's release list is newest-first; use its first eligible package,
      // regardless of prerelease status, so the site never silently links an older stable build.
      var rel = candidates[0], assets = rel.assets || [], zip = assets.find(function (asset) {
        return /^TEa\.G3-Complete-Package\.zip$/i.test(asset.name || "");
      });
      if (!zip || !zip.browser_download_url) return Promise.reject("The current package asset is missing");

      var v = String(rel.tag_name || "").replace(/^(arabic-)?v?/i, "");
      document.querySelectorAll("[data-version]").forEach(function (el) {
        var template = el.getAttribute("data-version") || "";
        if (v && template) el.textContent = template.replace("{v}", v);
      });
      document.querySelectorAll("[data-zip]").forEach(function (a) {
        a.href = zip.browser_download_url;
        a.setAttribute("download", zip.name);
      });
      document.querySelectorAll("[data-zip-name]").forEach(function (el) { el.textContent = zip.name; });
      document.querySelectorAll("[data-zip-size]").forEach(function (el) {
        el.textContent = (zip.size / 1048576).toFixed(2) + " MB";
      });
      setStatus(rel.prerelease ? "pre" : "stable");
      document.documentElement.dataset.releaseTag = rel.tag_name || "";
      document.documentElement.dataset.releasePrerelease = rel.prerelease ? "true" : "false";
    });
  }

  function start() {
    load().catch(function () { setStatus("fallback"); });
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", start, { once: true });
  else start();
  document.addEventListener("nuld:languagechange", function () {
    var prerelease = document.documentElement.dataset.releasePrerelease;
    setStatus(prerelease === "true" ? "pre" : prerelease === "false" ? "stable" : "fallback");
  });
})();
