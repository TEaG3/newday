// Fills the downloads from the latest GitHub release: the version, the zip's name and size, and the links straight
// to the file. Without JavaScript, or if GitHub does not answer, they keep pointing at the releases page.
(function () {
  fetch("https://api.github.com/repos/TEaG3/newday/releases?per_page=10")
    .then(function (r) { return r.ok ? r.json() : Promise.reject(r.status); })
    .then(function (releases) {
      var rel = (releases || []).find(function (item) {
        return /^arabic-/i.test(item.tag_name || "") && (item.assets || []).some(function (asset) { return /NULD-Arabic-Edition\\.zip$/i.test(asset.name); });
      });
      if (!rel) return Promise.reject("No Arabic NULD release found");
      var v = String(rel.tag_name || "").replace(/^arabic-/i, "").replace(/^v/i, ""), assets = rel.assets || [], zip = null;
      for (var i = 0; i < assets.length; i++) if (/\.zip$/.test(assets[i].name)) zip = assets[i];
      document.querySelectorAll("[data-version]").forEach(function (el) { if (v) el.textContent = el.getAttribute("data-version").replace("{v}", v); });
      if (!zip) return;
      document.querySelectorAll("[data-zip]").forEach(function (a) { a.href = zip.browser_download_url; });
      document.querySelectorAll("[data-zip-name]").forEach(function (el) { el.textContent = zip.name; });
      document.querySelectorAll("[data-zip-size]").forEach(function (el) { el.textContent = (zip.size / 1048576).toFixed(1) + " MB"; });
    })
    .catch(function () { /* keep the direct Arabic release asset links */ });
})();
