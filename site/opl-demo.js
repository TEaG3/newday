/* Browser-only explanation of engine_select() in engine.c.
   This never executes an ELF or starts a game. The route matrix mirrors the C self-tests. */
(function (root, factory) {
  const api = factory();
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  if (root && root.document) {
    root.TEaG3OPLLab = api;
    const start = () => api.mount(root.document);
    if (root.document.readyState === "loading") root.document.addEventListener("DOMContentLoaded", start, { once: true });
    else start();
  }
})(typeof window !== "undefined" ? window : null, function () {
  "use strict";
  function resolveRoute(input) {
    const format = input && input.format, source = input && input.source, preference = input && input.preference;
    const oplReady = !input || input.oplRuntime !== "missing" && input.oplRuntime !== false;
    if (!["iso", "zso", "ul"].includes(format) ||
        !["usb", "bdm", "smb", "hdl", "other"].includes(source) ||
        !["auto", "neutrino", "opl"].includes(preference)) {
      return { id: "unsupported", engine: "none", ok: false, reason: "invalid" };
    }
    if (preference === "neutrino")
      return { id: "engine-removed", engine: "none", ok: false, reason: "engine-removed" };
    if (format === "ul") {
      if (source !== "usb")
        return { id: "unsupported-source", engine: "none", ok: false, reason: "ul-needs-usb" };
      return oplReady ? { id: "opl-ul", engine: "OPL", ok: true, reason: "ul-usb" } :
        { id: "unsupported", engine: "none", ok: false, reason: "opl-runtime-missing" };
    }
    if (source === "smb" && input.network === "missing")
      return { id: "opl-network-config-invalid", engine: "none", ok: false, reason: "opl-network-config-invalid" };
    if (source !== "usb" && source !== "bdm") {
      if (format === "zso" && source === "hdl")
        return { id: "zso-hdl-unsupported", engine: "none", ok: false, reason: "unsupported-format-source" };
      return { id: format === "zso" ? "zso-source-unsupported" : "unsupported-source",
        engine: "none", ok: false, reason: "opl-source-unavailable" };
    }
    if (!oplReady) return format === "zso" ?
      { id: "zso-opl-missing", engine: "none", ok: false, reason: "opl-runtime-missing" } :
      { id: "opl-runtime-missing", engine: "none", ok: false, reason: "opl-runtime-missing" };
    return { id: "opl-bdm", engine: "OPL", ok: true, reason: "opl-bdm-direct" };
  }

  const COPY = {
    en: {
      result: "Selected launch route",
      titles: {
        "opl-ul": "Embedded OPL · USBExtreme UL",
        "opl-bdm": "Embedded OPL · BDM direct boot",
        "opl-runtime-missing": "Launch blocked · OPL runtime unavailable",
        "zso-opl-missing": "ZSO blocked · OPL runtime unavailable",
        "zso-hdl-unsupported": "ZSO unsupported · APA / HD Loader",
        "zso-source-unsupported": "ZSO blocked · source has no OPL adapter",
        "unsupported-source": "No OPL adapter for this source",
        "engine-removed": "OPL only · legacy Neutrino preference rejected",
        "opl-network-config-invalid": "SMB launch blocked · OPL network configuration is incomplete",
        unsupported: "No launch route for this selection"
      },
      sources: {
        usb: "USB", bdm: "MX4SIO / iLink / FAT-HDD", smb: "SMB share",
        hdl: "APA / HD Loader", other: "MMCE / UDPBD / UDPFS"
      },
      details: {
        "opl-ul": [
          "TEa.G3 scans ul.cfg on the USB device and validates each record.",
          "A game is listed only when every declared ul.* split part is present.",
          "TEa.G3 passes ul:<name>, startup ID, media type, mass device and the return-stub path to OPL.",
          "OPL performs the USBExtreme split-part reads."
        ],
        "opl-bdm": [
          "TEa.G3 selects the game from a local BDM source such as USB, MX4SIO, iLink or an ATA-backed disk.",
          "The launcher passes the exact massN selector, startup ID and CD/DVD media type to OPL.",
          "The TEa.G3 return ELF is passed as OPL's IGR exit path.",
          "The pinned OPL fork performs the game read; the selected device mapping still needs real-console verification."
        ],
        "opl-runtime-missing": [
          "The embedded OPL.ELF is missing or failed its runtime integrity check.",
          "No Neutrino fallback exists. The launch is blocked."
        ],
        "zso-opl-missing": [
          "The embedded OPL.ELF is missing or failed its runtime integrity check.",
          "ZSO is never sent to another engine; the launch is blocked."
        ],
        "zso-hdl-unsupported": [
          "The current TEa.G3 OPL adapter does not support the APA/HDL mini-launch route for ZSO."
        ],
        "zso-source-unsupported": [
          "This source has no verified OPL direct-launch adapter in the current TEa.G3 build.",
          "ZSO is blocked rather than sent to a different engine."
        ],
        "unsupported-source": [
          "This source has no verified OPL direct-launch adapter in the current TEa.G3 build.",
          "Network, MMCE and APA/HDL routes remain disabled until their OPL adapters are implemented and tested."
        ],
        "engine-removed": [
          "OPL is the only PS2 game engine in this build.",
          "The retired Neutrino preference is rejected; no fallback exists."
        ],
        "opl-network-config-invalid": [
          "The SMB network settings are incomplete for the OPL route.",
          "The source remains blocked; no Neutrino fallback is attempted."
        ],
        unsupported: ["Choose ISO, ZSO or USBExtreme UL and a source with an OPL adapter."]
      },
      hardware: "Browser simulation only · no PS2 ELF, emulator or game is executed here.",
      supported: "Route wired · hardware test pending",
      blocked: "Unsupported selection",
      auto: "Automatic OPL", neutrino: "Removed", opl: "OPL",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    },
    ar: {
      result: "مسار التشغيل المختار",
      titles: {
        "opl-ul": "OPL المضمّن · USBExtreme UL",
        "opl-bdm": "OPL المضمّن · تشغيل BDM مباشر",
        "opl-runtime-missing": "تم حظر التشغيل · محرك OPL غير متاح",
        "zso-opl-missing": "تم حظر ZSO · محرك OPL غير متاح",
        "zso-hdl-unsupported": "ZSO غير مدعوم · APA / HD Loader",
        "zso-source-unsupported": "تم حظر ZSO · لا يوجد محول OPL لهذا المصدر",
        "unsupported-source": "لا يوجد محول OPL لهذا المصدر",
        "engine-removed": "OPL فقط · تم رفض تفضيل Neutrino القديم",
        "opl-network-config-invalid": "تم حظر SMB · إعداد شبكة OPL غير مكتمل",
        unsupported: "لا يوجد مسار تشغيل لهذا الاختيار"
      },
      sources: {
        usb: "USB", bdm: "MX4SIO / iLink / HDD FAT",
        smb: "مشاركة SMB", hdl: "APA / HD Loader", other: "MMCE / UDPBD / UDPFS"
      },
      details: {
        "opl-ul": [
          "يفحص TEa.G3 ملف ul.cfg على USB ويتحقق من كل سجل.",
          "لا يعرض اللعبة إلا عند وجود جميع أجزاء ul.* المعلنة.",
          "يمرّر TEa.G3 اسم ul ورقم بدء التشغيل ونوع الوسيط وجهاز mass ومسار العودة إلى OPL.",
          "يتولى OPL قراءة أجزاء USBExtreme."
        ],
        "opl-bdm": [
          "يحدد TEa.G3 اللعبة من مصدر BDM محلي مثل USB أو MX4SIO أو iLink أو قرص ATA.",
          "يمرّر المشغل محدد massN ورقم بدء التشغيل ونوع الوسيط CD/DVD إلى OPL.",
          "يمرّر ملف العودة إلى TEa.G3 بوصفه مسار خروج IGR في OPL.",
          "يتولى إصدار OPL المحدد قراءة اللعبة؛ وما زال تعيين الجهاز بحاجة إلى اختبار فعلي."
        ],
        "opl-runtime-missing": [
          "ملف OPL.ELF المضمّن غير موجود أو فشل التحقق من سلامته.",
          "لا يوجد رجوع إلى Neutrino؛ يتم حظر التشغيل."
        ],
        "zso-opl-missing": [
          "ملف OPL.ELF المضمّن غير موجود أو فشل التحقق من سلامته.",
          "لا يتم تمرير ZSO إلى محرك آخر؛ يتم حظر التشغيل."
        ],
        "zso-hdl-unsupported": [
          "محوّل TEa.G3 الحالي لا يدعم تشغيل ZSO عبر مسار APA/HDL mini."
        ],
        "zso-source-unsupported": [
          "لا يوجد لهذا المصدر محول تشغيل مباشر موثّق مع OPL في إصدار TEa.G3 الحالي.",
          "يتم حظر ZSO بدلاً من تمريره إلى محرك آخر."
        ],
        "unsupported-source": [
          "لا يوجد لهذا المصدر محول تشغيل مباشر موثّق مع OPL في إصدار TEa.G3 الحالي.",
          "تبقى مصادر الشبكة وMMCE وAPA/HDL معطلة حتى بناء محولات OPL واختبارها."
        ],
        "engine-removed": [
          "OPL هو محرك ألعاب PS2 الوحيد في هذا الإصدار.",
          "يتم رفض تفضيل Neutrino المتقاعد ولا يحدث أي انتقال إلى محرك بديل."
        ],
        "opl-network-config-invalid": [
          "إعدادات شبكة SMB غير مكتملة لمسار OPL.",
          "يبقى المصدر محظوراً ولا تتم محاولة تشغيل Neutrino."
        ],
        unsupported: ["اختر ISO أو ZSO أو USBExtreme UL ومصدراً يملك محول OPL."]
      },
      hardware: "محاكاة داخل المتصفح فقط · لا يتم تشغيل ELF أو محاكي أو لعبة PS2 هنا.",
      supported: "مسار موصول · اختبار الجهاز الفعلي متبقٍ",
      blocked: "اختيار غير مدعوم",
      auto: "تلقائي عبر OPL", neutrino: "محذوف", opl: "OPL",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    }
  };

  function render(form) {
    const output = form.closest("[data-opl-widget]").querySelector("[data-opl-result]");
    const ar = form.ownerDocument.documentElement.lang === "ar", c = ar ? COPY.ar : COPY.en;
    const input = {
      format: form.elements.format.value,
      source: form.elements.source.value,
      preference: form.elements.preference.value,
      oplRuntime: form.elements.oplRuntime ? form.elements.oplRuntime.value : "ready",
      network: form.elements.network ? form.elements.network.value : "ready"
    };
    const smbConfig = form.querySelector("[data-smb-config]");
    if (smbConfig) smbConfig.hidden = input.source !== "smb";
    const route = resolveRoute(input);
    const details = c.details[route.id] || c.details.unsupported;
    const title = c.titles[route.id] || c.titles.unsupported;
    const formatLabel = c[input.format] || input.format;
    const sourceLabel = c.sources[input.source] || input.source;
    const preferenceLabel = c[input.preference] || input.preference;
    output.textContent = "";
    output.className = "opl-result " + (route.ok ? "is-supported" : "is-blocked");

    const head = form.ownerDocument.createElement("div"); head.className = "opl-result-head";
    const titleNode = form.ownerDocument.createElement("h3"); titleNode.textContent = title;
    const status = form.ownerDocument.createElement("span"); status.className = "opl-status";
    status.textContent = !route.ok ? c.blocked : route.fallback ? c.fallback : c.supported;
    head.append(titleNode, status); output.appendChild(head);

    const meta = form.ownerDocument.createElement("p"); meta.className = "opl-result-meta";
    meta.textContent = c.result + ": " + formatLabel + " · " + sourceLabel + " · " + preferenceLabel;
    output.appendChild(meta);

    const list = form.ownerDocument.createElement("ol"); list.className = "opl-route-steps";
    details.forEach((line, index) => {
      const item = form.ownerDocument.createElement("li"), number = form.ownerDocument.createElement("span");
      number.className = "step-number"; number.textContent = String(index + 1).padStart(2, "0");
      const text = form.ownerDocument.createElement("span"); text.textContent = line;
      item.append(number, text); list.appendChild(item);
    });
    output.appendChild(list);
    const note = form.ownerDocument.createElement("p"); note.className = "opl-sim-note"; note.textContent = c.hardware;
    output.appendChild(note);
  }

  function mount(document) {
    document.querySelectorAll("[data-opl-form]").forEach(form => {
      if (form.dataset.oplReady) return;
      form.dataset.oplReady = "1";
      form.addEventListener("submit", event => { event.preventDefault(); render(form); });
      const reset = form.querySelector("[data-opl-reset]");
      if (reset) reset.addEventListener("click", () => {
        form.elements.format.value = "iso"; form.elements.source.value = "usb";
        form.elements.preference.value = "opl";
        if (form.elements.oplRuntime) form.elements.oplRuntime.value = "ready";
        if (form.elements.network) form.elements.network.value = "ready";
        render(form);
      });
      ["format", "source", "preference", "oplRuntime", "network"].forEach(name => {
        if (form.elements[name]) form.elements[name].addEventListener("change", () => render(form));
      });
      render(form);
    });
    document.addEventListener("tea-g3:languagechange", () => document.querySelectorAll("[data-opl-form]").forEach(render));
  }
  return { resolveRoute, mount };
});
