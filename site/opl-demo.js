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
    if (preference === "neutrino")
      return { id: "engine-removed", engine: "none", ok: false, reason: "engine-removed" };

    if (format === "ul") {
      if (source !== "usb")
        return { id: "unsupported-source", engine: "none", ok: false, reason: "unsupported-format-source" };
      if (!oplReady)
        return { id: "opl-runtime-missing", engine: "none", ok: false, reason: "opl-runtime-missing" };
      return { id: "opl-ul", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-ul" : "auto-opl-ul" };
    }
    if (format === "zso" && source === "hdl")
      return { id: "zso-hdl-unsupported", engine: "none", ok: false, reason: "unsupported-format-source" };

    if (source === "usb" || source === "bdm") {
      if (!oplReady)
        return { id: "opl-runtime-missing", engine: "none", ok: false, reason: "opl-runtime-missing" };
      return { id: "opl-bdm", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-bdm" : "auto-opl-bdm" };
    }
    if (source === "smb") {
      if (!networkReady)
        return { id: "opl-network-config-invalid", engine: "none", ok: false, reason: "opl-network-config-invalid" };
      if (!oplReady)
        return { id: "opl-runtime-missing", engine: "none", ok: false, reason: "opl-runtime-missing" };
      return { id: "opl-eth", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-eth" : "auto-opl-eth" };
    }
    if (source === "hdl") {
      if (!oplReady)
        return { id: "opl-runtime-missing", engine: "none", ok: false, reason: "opl-runtime-missing" };
      return { id: "opl-hdd", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-hdd" : "auto-opl-hdd" };
    }
    if (format === "zso")
      return { id: "zso-requires-opl", engine: "none", ok: false, reason: "zso-requires-opl" };
    return { id: "unsupported-source", engine: "none", ok: false, reason: "opl-source-unavailable" };
  }

  const COPY = {
    en: {
      result: "Selected launch route",
      titles: {
        "opl-ul": "Embedded OPL · USBExtreme UL",
        "opl-bdm": "Embedded OPL · BDM direct boot",
        "opl-eth": "Embedded OPL · SMB / ETH",
        "opl-hdd": "Embedded OPL · APA / HD Loader",
        "engine-removed": "Neutrino retired · no fallback",
        "opl-runtime-missing": "Launch blocked · OPL runtime unavailable",
        "opl-network-config-invalid": "Launch blocked · SMB/ETH configuration invalid",
        "zso-requires-opl": "ZSO requires an available OPL BDM/ETH adapter",
        "zso-hdl-unsupported": "ZSO unsupported · APA / HD Loader",
        "unsupported-source": "No OPL adapter for this source",
        unsupported: "No launch route for this selection"
      },
      sources: {
        usb: "USB", bdm: "MX4SIO / iLink / FAT-HDD", smb: "SMB share",
        hdl: "APA / HD Loader", other: "MMCE / UDPBD / UDPFS (disabled)"
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
          "Both auto and manual OPL preferences select the OPL BDM direct-boot adapter.",
          "The hand-off identifies the requested BDM driver and device unit; OPL does not substitute another device.",
          "ISO and ZSO use this route; physical-console gameplay validation is still pending."
        ],
        "opl-eth": [
          "TEa.G3 reads the TEAG3.GAMES catalog from the configured SMB share using the embedded SMB IOP support modules.",
          "The OPL ETH hand-off passes the PS2 IPv4 settings, server IPv4, share and credentials.",
          "OPL ETH, not Neutrino, opens the game's ISO/ZSO data from the share.",
          "SMB gameplay and sustained reads still require physical-console verification."
        ],
        "opl-hdd": [
          "TEa.G3 reads the main APA partition's exact HDL start LBA and aggregates all matching sub-partition sizes.",
          "It passes the startup ID, start sector and total_size_in_kb to OPL's HDD mini-launch.",
          "OPL reads the existing hdd_partition setting from hdd0:__common/OPL/conf_hdd.cfg using a read-only config mount.",
          "If the configured OPL partition or its existing OPL folder is missing, OPL refuses launch; it does not create or format partitions."
        ],
        "engine-removed": [
          "Neutrino is retired as a PS2 game engine.",
          "Legacy motor=neutrino settings are rejected. No fallback or second game engine is launched."
        ],
        "opl-runtime-missing": [
          "The embedded OPL.ELF is missing or failed its integrity check.",
          "All PS2 game routes are blocked until the OPL runtime is restored; there is no Neutrino fallback."
        ],
        "opl-network-config-invalid": [
          "The SMB route requires a valid static PS2 IPv4 address, server IPv4, share and supported credentials.",
          "The SMB game route remains blocked when these settings are invalid; there is no Neutrino fallback."
        ],
        "zso-requires-opl": [
          "ZSO requires a ready OPL BDM or ETH route.",
          "If the selected source has no OPL adapter, the launch is blocked."
        ],
        "zso-hdl-unsupported": [
          "ZSO cannot use the APA/HDL mini-launch path. Use a supported ISO for this HDD route."
        ],
        "unsupported-source": [
          "MMCE, UDPBD and UDPFS are not active OPL game adapters in this build.",
          "Choose USB/BDM, SMB/ETH or APA/HDL ISO instead."
        ],
        unsupported: ["Choose a supported format, source and engine preference."]
      },
      hardware: "Browser simulation only · no PS2 ELF, emulator or game is executed here.",
      supported: "Route exists in code · hardware test pending",
      blocked: "Unsupported selection",
      auto: "Auto (OPL only)", neutrino: "Retired", opl: "OPL (manual)",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    },
    ar: {
      result: "مسار التشغيل المختار",
      titles: {
        "opl-ul": "OPL المضمّن · USBExtreme UL",
        "opl-bdm": "OPL المضمّن · تشغيل BDM مباشر",
        "opl-eth": "OPL المضمّن · SMB / ETH",
        "opl-hdd": "OPL المضمّن · APA / HD Loader",
        "engine-removed": "تم إيقاف Neutrino · لا يوجد محرك بديل",
        "opl-runtime-missing": "تم حظر التشغيل · محرك OPL غير جاهز",
        "opl-network-config-invalid": "تم حظر التشغيل · إعداد SMB/ETH غير صالح",
        "zso-requires-opl": "ZSO يتطلب محول OPL BDM/ETH جاهزاً",
        "zso-hdl-unsupported": "ZSO غير مدعوم · مسار APA / HD Loader",
        "unsupported-source": "لا يوجد محول OPL لهذا المصدر",
        unsupported: "لا يوجد مسار تشغيل لهذا الاختيار"
      },
      sources: {
        usb: "USB", bdm: "MX4SIO / iLink / HDD FAT",
        smb: "مشاركة SMB", hdl: "APA / HD Loader", other: "MMCE / UDPBD / UDPFS (معطلة)"
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
          "اختيار auto أو opl يوجّه التشغيل إلى محول BDM في OPL.",
          "يمرّر نوع محرك BDM ورقم الجهاز المطلوب، ولا يستبدل OPL الجهاز بآخر.",
          "تستخدم صيغتا ISO وZSO هذا المسار؛ ويبقى اختبار اللعب على جهاز فعلي."
        ],
        "opl-eth": [
          "يقرأ TEa.G3 فهرس TEAG3.GAMES من مشاركة SMB عبر وحدات IOP المساعدة المضمّنة.",
          "يمرّر إلى OPL ETH إعدادات IPv4 الخاصة بـ PS2 وعنوان الخادم والمشاركة وبيانات الدخول.",
          "يتولى OPL ETH قراءة بيانات لعبة ISO/ZSO من المشاركة، وليس Neutrino.",
          "يبقى اختبار الألعاب والقراءة المستمرة عبر SMB بحاجة إلى جهاز PS2 فعلي."
        ],
        "opl-hdd": [
          "يستخرج TEa.G3 قطاع بداية لعبة HDL الدقيق من قسم APA الرئيسي ويجمع أحجام الأقسام الفرعية التابعة لها.",
          "يمرّر Startup ID وقطاع البداية والحجم الكلي total_size_in_kb إلى وضع HDD mini في OPL.",
          "يقرأ OPL إعداد hdd_partition الموجود في hdd0:__common/OPL/conf_hdd.cfg عبر تركيب قراءة فقط.",
          "إذا لم يجد قسم OPL المهيأ أو مجلد OPL الموجود مسبقاً يرفض الإقلاع ولا ينشئ الأقسام أو يهيئها."
        ],
        "engine-removed": [
          "تم إيقاف Neutrino كمحرك تشغيل ألعاب PS2.",
          "يُرفض إعداد motor=neutrino القديم، ولا يحدث رجوع إلى محرك ثانٍ."
        ],
        "opl-runtime-missing": [
          "ملف OPL.ELF المضمّن غير موجود أو لم يجتز فحص السلامة.",
          "تُحظر جميع مسارات ألعاب PS2 حتى استعادة OPL؛ ولا يوجد رجوع إلى Neutrino."
        ],
        "opl-network-config-invalid": [
          "يتطلب مسار SMB عنوان IPv4 ثابتاً صالحاً للـ PS2 والخادم والمشاركة وبيانات دخول ضمن الحدود المدعومة.",
          "يبقى تشغيل SMB محظوراً إذا كانت الإعدادات غير صالحة؛ ولا يتم تشغيل Neutrino بديلاً."
        ],
        "zso-requires-opl": [
          "تتطلب ZSO مسار OPL BDM أو ETH جاهزاً.",
          "إذا لم يكن للمصدر محول OPL، يُحظر التشغيل."
        ],
        "zso-hdl-unsupported": [
          "لا يمكن تشغيل ZSO عبر مسار APA/HDL mini. استخدم ISO مدعوماً لهذا المسار على القرص."
        ],
        "unsupported-source": [
          "مصادر MMCE وUDPBD وUDPFS لا تملك محولات تشغيل OPL مفعّلة في هذا البناء.",
          "اختر USB/BDM أو SMB/ETH أو ISO عبر APA/HDL."
        ],
        unsupported: ["اختر صيغة ومصدراً وتفضيل محرك مدعوماً."]
      },
      hardware: "محاكاة داخل المتصفح فقط · لا يتم تشغيل ELF أو محاكي أو لعبة PS2 هنا.",
      supported: "المسار موجود في الكود · اختبار الجهاز الفعلي متبقٍ",
      blocked: "اختيار غير مدعوم",
      auto: "تلقائي (OPL فقط)", neutrino: "متوقف", opl: "OPL (يدوي)",
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
