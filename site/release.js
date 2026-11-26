// Fill every download control from the newest published TEa.G3 package.
// Stable publication is a software-release status, not a claim of physical-PS2 acceptance.
// Release notes state the exact hardware-validation boundary for each package.
(function () {
  var statusCopy = {
    stable: { en: "Stable software release · PS2 hardware acceptance not claimed", ar: "إصدار برمجي مستقر · لم يُدّعَ اجتياز اختبار PS2 الحقيقي" },
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

      // The API collection is not guaranteed to be sorted by publication time.
      // Sort explicitly so a stale older prerelease can never win by array position.
      candidates.sort(function (a, b) {
        var aTime = Date.parse(a.published_at || a.created_at || "") || 0;
        var bTime = Date.parse(b.published_at || b.created_at || "") || 0;
        return bTime - aTime;
      });
      var rel = candidates[0], assets = rel.assets || [], zip = assets.find(function (asset) {
        return /^TEa\.G3-Complete-Package\.zip$/i.test(asset.name || "");
      });
      if (!zip || !zip.browser_download_url) return Promise.reject("The current package asset is missing");
      // Only navigate to this repository's release asset; never trust arbitrary API-provided URLs.
      var downloadUrl = String(zip.browser_download_url);
      if (!/^https:\/\/github\.com\/TEaG3\/newday\/releases\/download\/[^/?#]+\/TEa\.G3-Complete-Package\.zip$/i.test(downloadUrl)) {
        return Promise.reject("The release asset URL is outside the trusted repository");
      }

      var v = String(rel.tag_name || "").replace(/^(arabic-)?v?/i, "");
      document.querySelectorAll("[data-version]").forEach(function (el) {
        var template = el.getAttribute("data-version") || "";
        if (v && template) el.textContent = template.replace("{v}", v);
      });
      document.querySelectorAll("[data-zip]").forEach(function (a) {
        a.href = downloadUrl;
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
  document.addEventListener("tea-g3:languagechange", function () {
    var prerelease = document.documentElement.dataset.releasePrerelease;
    setStatus(prerelease === "true" ? "pre" : prerelease === "false" ? "stable" : "fallback");
  });
})();
