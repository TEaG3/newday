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
    const networkReady = !input || input.network !== "missing" && input.network !== false;
    if (!["iso", "zso", "ul"].includes(format) ||
        !["usb", "bdm", "smb", "hdl", "other"].includes(source) ||
        !["auto", "neutrino", "opl"].includes(preference)) {
      return { id: "unsupported", engine: "none", ok: false, reason: "invalid" };
    }
    if (format === "ul") {
      if (source !== "usb") return { id: "unsupported", engine: "none", ok: false, reason: "ul-needs-usb" };
      return oplReady ? { id: "opl-ul", engine: "OPL", ok: true, reason: "ul-usb" } :
        { id: "unsupported", engine: "none", ok: false, reason: "opl-runtime-missing" };
    }
    if (preference !== "opl") return { id: "neutrino", engine: "Neutrino", ok: true, reason: "default-neutrino" };
    if (source === "usb" || source === "bdm")
      return oplReady ? { id: "opl-bdm", engine: "OPL", ok: true, reason: "manual-opl-bdm" } :
        { id: "neutrino-opl-missing", engine: "Neutrino", ok: true, fallback: true, reason: "opl-runtime-missing" };
    if (source === "smb" && !networkReady)
      return { id: "neutrino-smb-fallback", engine: "Neutrino", ok: true, fallback: true, reason: "opl-network-config-invalid" };
    if (source === "smb")
      return oplReady ? { id: "opl-eth", engine: "OPL", ok: true, reason: "manual-opl-eth" } :
        { id: "neutrino-opl-missing", engine: "Neutrino", ok: true, fallback: true, reason: "opl-runtime-missing" };
    if (source === "hdl")
      return oplReady ? { id: "opl-hdd", engine: "OPL", ok: true, reason: "manual-opl-hdd" } :
        { id: "neutrino-opl-missing", engine: "Neutrino", ok: true, fallback: true, reason: "opl-runtime-missing" };
    return { id: "neutrino", engine: "Neutrino", ok: true, reason: "opl-fallback-neutrino" };
  }

  const COPY = {
    en: {
      result: "Selected launch route",
      titles: {
        "opl-ul": "Embedded OPL · USBExtreme UL",
        "opl-bdm": "Embedded OPL · BDM direct boot",
        "opl-eth": "Embedded OPL · SMB / ETH",
        "opl-hdd": "Embedded OPL · APA / HD Loader",
        neutrino: "Neutrino · existing launch path",
        "neutrino-smb-fallback": "Neutrino · SMB configuration fallback",
        "neutrino-opl-missing": "Neutrino · OPL runtime unavailable",
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
          "TEa.G3 passes the fixed-width title, startup ID, media type and part count to OPL.",
          "OPL performs the USBExtreme split-part reads."
        ],
        "opl-bdm": [
          "TEa.G3 selects the game from USB, MX4SIO, iLink or an ATA-backed FAT/exFAT HDD.",
          "The manual [juegos] motor = opl preference selects OPL's BDM direct-boot adapter.",
          "The hand-off identifies the requested BDM driver and device unit; OPL does not substitute another device.",
          "Auto and neutrino keep ordinary ISO/ZSO games on Neutrino."
        ],
        "opl-eth": [
          "TEa.G3 selects the ISO/ZSO entry from the configured SMB share.",
          "The OPL ETH hand-off passes the PS2 IPv4 settings, server IPv4, share and credentials.",
          "The adapter validates IPv4 values and OPL field lengths before starting; invalid settings fall back to Neutrino.",
          "SMB gameplay and sustained reads still require physical-console verification."
        ],
        "opl-hdd": [
          "TEa.G3 reads the main APA partition's exact HDL start LBA and aggregates all matching sub-partition sizes.",
          "It passes the startup ID, start sector and total_size_in_kb to OPL's HDD mini-launch.",
          "OPL reads the existing hdd_partition setting from hdd0:__common/OPL/conf_hdd.cfg using a read-only config mount.",
          "If the configured OPL partition or its existing OPL folder is missing, OPL refuses launch; it does not create or format partitions."
        ],
        "neutrino-smb-fallback": [
          "The SMB network configuration is incomplete, so the C engine controller does not select OPL ETH.",
          "The existing Neutrino route remains the fallback; fix the PS2 IPv4, server, share and credentials before retrying OPL."
        ],
        "neutrino-opl-missing": [
          "The embedded OPL.ELF is missing or has not passed runtime installation checks.",
          "TEa.G3 retains the Neutrino route for ISO/ZSO instead of attempting an OPL hand-off."
        ],
        neutrino: [
          "TEa.G3 keeps the selected ISO/ZSO on its existing Neutrino path.",
          "Auto and neutrino always select Neutrino for ISO/ZSO; unsupported OPL source combinations also retain Neutrino.",
          "The source-specific Neutrino hand-off remains available as the compatibility path."
        ],
        unsupported: ["Choose a supported format, source and engine preference."]
      },
      hardware: "Browser simulation only · no PS2 ELF, emulator or game is executed here.",
      supported: "Route exists in code · hardware test pending", fallback: "Safe Neutrino fallback", blocked: "Unsupported selection",
      auto: "Auto", neutrino: "Neutrino", opl: "OPL (manual)",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    },
    ar: {
      result: "مسار التشغيل المختار",
      titles: {
        "opl-ul": "OPL المضمّن · USBExtreme UL",
        "opl-bdm": "OPL المضمّن · تشغيل BDM مباشر",
        "opl-eth": "OPL المضمّن · SMB / ETH",
        "opl-hdd": "OPL المضمّن · APA / HD Loader",
        neutrino: "Neutrino · مسار التشغيل الحالي",
        "neutrino-smb-fallback": "Neutrino · رجوع آمن بسبب إعداد SMB",
        "neutrino-opl-missing": "Neutrino · محرك OPL غير جاهز",
        unsupported: "لا يوجد مسار تشغيل لهذا الاختيار"
      },
      sources: {
        usb: "USB", bdm: "MX4SIO / iLink / HDD FAT",
        smb: "مشاركة SMB", hdl: "APA / HD Loader", other: "MMCE / UDPBD / UDPFS"
      },
      details: {
        "opl-ul": [
          "يفحص TEa.G3 ملف ul.cfg على USB ويتحقق من سجلاته.",
          "لا يعرض اللعبة إلا عند وجود جميع أجزاء ul.* المعلنة.",
          "يمرّر الاسم الثابت ورقم بدء التشغيل ونوع الوسيط وعدد الأجزاء إلى OPL.",
          "يتولى OPL قراءة أجزاء USBExtreme."
        ],
        "opl-bdm": [
          "يحدد TEa.G3 اللعبة من USB أو MX4SIO أو iLink أو قرص HDD بنظام FAT/exFAT.",
          "اختيار [juegos] motor = opl يحدد محوّل الإقلاع المباشر BDM في OPL.",
          "يمرّر نوع محرك BDM ورقم الجهاز المطلوب، ولا يستبدل OPL الجهاز بآخر.",
          "يبقي الوضعان auto وneutrino ألعاب ISO/ZSO على Neutrino."
        ],
        "opl-eth": [
          "يحدد TEa.G3 لعبة ISO/ZSO من مشاركة SMB المهيأة.",
          "يمرّر إلى OPL إعدادات IPv4 الخاصة بـ PS2 وعنوان الخادم والمشاركة وبيانات الدخول.",
          "يتحقق المحوّل من عناوين IPv4 وحدود الحقول؛ وإذا كانت الإعدادات غير صالحة يعود إلى Neutrino.",
          "يبقى اختبار الألعاب والقراءة المستمرة عبر SMB بحاجة إلى جهاز PS2 فعلي."
        ],
        "opl-hdd": [
          "يستخرج TEa.G3 قطاع بداية لعبة HDL الدقيق من قسم APA الرئيسي ويجمع أحجام الأقسام الفرعية التابعة لها.",
          "يمرّر Startup ID وقطاع البداية والحجم الكلي total_size_in_kb إلى وضع HDD mini في OPL.",
          "يقرأ OPL إعداد hdd_partition الموجود في hdd0:__common/OPL/conf_hdd.cfg عبر تركيب قراءة فقط.",
          "إذا لم يجد قسم OPL المهيأ أو مجلد OPL الموجود مسبقاً يرفض الإقلاع ولا ينشئ الأقسام أو يهيئها."
        ],
        "neutrino-smb-fallback": [
          "إعداد شبكة SMB غير مكتمل؛ لذلك لا يختار متحكم المحركات في TEa.G3 محول OPL ETH.",
          "يبقى مسار Neutrino متاحاً كبديل آمن. أكمل IP الخاص بـ PS2 والخادم والمشاركة وبيانات الدخول قبل إعادة المحاولة."
        ],
        "neutrino-opl-missing": [
          "ملف OPL.ELF المضمّن غير موجود أو لم يجتز التحقق من التثبيت.",
          "يبقي TEa.G3 ألعاب ISO/ZSO على Neutrino بدلاً من تنفيذ تسليم OPL."
        ],
        neutrino: [
          "يبقي TEa.G3 لعبة ISO/ZSO على مسار Neutrino الحالي.",
          "يختار auto وneutrino محرك Neutrino دائماً؛ كما تبقى المصادر التي لا تملك محوّل OPL على مسار Neutrino.",
          "يبقى تمرير Neutrino الخاص بكل مصدر متاحاً كمسار توافق."
        ],
        unsupported: ["اختر صيغة ومصدراً وتفضيل محرك مدعوماً."]
      },
      hardware: "محاكاة داخل المتصفح فقط · لا يتم تشغيل ELF أو محاكي أو لعبة PS2 هنا.",
      supported: "المسار موجود في الكود · اختبار الجهاز الفعلي متبقٍ", fallback: "تحويل آمن إلى Neutrino", blocked: "اختيار غير مدعوم",
      auto: "تلقائي", neutrino: "Neutrino", opl: "OPL (يدوي)",
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
        form.elements.preference.value = "auto";
        if (form.elements.oplRuntime) form.elements.oplRuntime.value = "ready";
        if (form.elements.network) form.elements.network.value = "ready";
        render(form);
      });
      ["format", "source", "preference", "oplRuntime", "network"].forEach(name => {
        if (form.elements[name]) form.elements[name].addEventListener("change", () => render(form));
      });
      render(form);
    });
    document.addEventListener("nuld:languagechange", () => document.querySelectorAll("[data-opl-form]").forEach(render));
  }
  return { resolveRoute, mount };
});
