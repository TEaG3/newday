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
    if (format === "zso" && source === "hdl")
      return { id: "zso-hdl-unsupported", engine: "none", ok: false, reason: "unsupported-format-source" };
    if (format === "zso" && preference === "neutrino")
      return { id: "zso-requires-opl", engine: "none", ok: false, reason: "zso-requires-opl" };
    if (preference === "neutrino")
      return { id: "neutrino", engine: "Neutrino", ok: true, reason: "preference-neutrino" };

    if (source === "usb" || source === "bdm") {
      if (!oplReady) return format === "zso" ?
        { id: "zso-opl-missing", engine: "none", ok: false, reason: "opl-runtime-missing" } :
        { id: "neutrino-opl-missing", engine: "Neutrino", ok: true, fallback: true, reason: "opl-runtime-missing" };
      return { id: "opl-bdm", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-bdm" : "auto-opl-bdm" };
    }
    if (source === "smb") {
      if (!networkReady) return format === "zso" ?
        { id: "zso-smb-config-invalid", engine: "none", ok: false, reason: "opl-network-config-invalid" } :
        { id: "neutrino-smb-fallback", engine: "Neutrino", ok: true, fallback: true, reason: "opl-network-config-invalid" };
      if (!oplReady) return format === "zso" ?
        { id: "zso-opl-missing", engine: "none", ok: false, reason: "opl-runtime-missing" } :
        { id: "neutrino-opl-missing", engine: "Neutrino", ok: true, fallback: true, reason: "opl-runtime-missing" };
      return { id: "opl-eth", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-eth" : "auto-opl-eth" };
    }
    if (source === "hdl") {
      if (!oplReady) return { id: "neutrino-opl-missing", engine: "Neutrino", ok: true, fallback: true, reason: "opl-runtime-missing" };
      return { id: "opl-hdd", engine: "OPL", ok: true, reason: preference === "opl" ? "manual-opl-hdd" : "auto-opl-hdd" };
    }
    if (format === "zso")
      return { id: "zso-requires-opl", engine: "none", ok: false, reason: "zso-requires-opl" };
    return {
      id: "neutrino", engine: "Neutrino", ok: true,
      fallback: preference === "opl",
      reason: preference === "opl" ? "opl-source-unavailable" : "auto-neutrino-baseline"
    };
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
        unsupported: "No launch route for this selection",
        "zso-requires-opl": "ZSO requires an available OPL BDM/ETH decoder",
        "zso-opl-missing": "ZSO blocked · OPL runtime unavailable",
        "zso-smb-config-invalid": "ZSO blocked · SMB/ETH configuration invalid",
        "zso-hdl-unsupported": "ZSO unsupported · APA / HD Loader"
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
          "Auto and manual OPL use this adapter for supported ISO/ZSO; explicit Neutrino keeps ISO on Neutrino and rejects ZSO."
        ],
        "opl-eth": [
          "TEa.G3 selects the ISO/ZSO entry from the configured SMB share.",
          "The OPL ETH hand-off passes the PS2 IPv4 settings, server IPv4, share and credentials.",
          "The adapter validates IPv4 values and OPL field lengths. Invalid settings let ISO fall back to Neutrino, but block ZSO instead of sending it to Neutrino.",
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
          "This fallback is for ISO only; ZSO is blocked because the SMB/ETH route cannot use its required OPL decoder. Fix the PS2 IPv4, server, share and credentials before retrying OPL."
        ],
        "neutrino-opl-missing": [
          "The embedded OPL.ELF is missing or has not passed runtime installation checks.",
          "ISO can use the Neutrino fallback; ZSO is blocked because this build has no verified Neutrino ZSO decoder."
        ],
        neutrino: [
          "TEa.G3 keeps supported ISO images on Neutrino when that engine is selected or the source has no OPL adapter.",
          "ZSO is never sent to Neutrino; it requires a ready OPL BDM/ETH adapter and valid SMB configuration where applicable.",
          "If that OPL route is unavailable, the launch is blocked instead of silently falling back."
        ],
        "zso-requires-opl": ["ZSO cannot be sent to Neutrino in this build. It requires a ready OPL BDM/ETH route; otherwise the launch is blocked."],
        "zso-opl-missing": ["The embedded OPL runtime is missing or failed integrity checks. ISO can use a Neutrino fallback, but ZSO is blocked."],
        "zso-smb-config-invalid": ["SMB/ETH network settings are incomplete or invalid. ISO can use Neutrino, but ZSO is blocked because it requires the OPL ETH decoder."],
        "zso-hdl-unsupported": ["ZSO cannot use the APA/HDL mini-launch path. Use a supported ISO for this HDD route."],
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
        unsupported: "لا يوجد مسار تشغيل لهذا الاختيار",
        "zso-requires-opl": "ZSO يتطلب مسار OPL BDM/ETH جاهزاً",
        "zso-opl-missing": "تم حظر ZSO · محرك OPL غير جاهز",
        "zso-smb-config-invalid": "تم حظر ZSO · إعداد SMB/ETH غير صالح",
        "zso-hdl-unsupported": "ZSO غير مدعوم · مسار APA / HD Loader"
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
          "يستخدم auto وopl هذا المحوّل لصيغ ISO/ZSO المدعومة؛ أما اختيار neutrino فيُبقي ISO على Neutrino ويرفض ZSO."
        ],
        "opl-eth": [
          "يحدد TEa.G3 لعبة ISO/ZSO من مشاركة SMB المهيأة.",
          "يمرّر إلى OPL إعدادات IPv4 الخاصة بـ PS2 وعنوان الخادم والمشاركة وبيانات الدخول.",
          "يتحقق المحوّل من عناوين IPv4 وحدود الحقول. إذا كانت الإعدادات غير صالحة يمكن تحويل ISO إلى Neutrino، لكن يُحظر ZSO بدلاً من تمريره إلى Neutrino.",
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
          "هذا التحويل مخصص لـ ISO فقط؛ يُحظر ZSO لأن مسار SMB/ETH لا يستطيع استخدام مفكك OPL المطلوب. أكمل IP الخاص بـ PS2 والخادم والمشاركة وبيانات الدخول قبل إعادة المحاولة."
        ],
        "neutrino-opl-missing": [
          "ملف OPL.ELF المضمّن غير موجود أو لم يجتز التحقق من التثبيت.",
          "يمكن تحويل ISO إلى Neutrino؛ أما ZSO فيُحظر لأن هذا البناء لا يتضمن مفكك ZSO موثوقاً في Neutrino."
        ],
        neutrino: [
          "يبقي TEa.G3 صور ISO المدعومة على Neutrino عند اختيار هذا المحرك أو عند عدم توفر محوّل OPL للمصدر.",
          "لا يمرر ZSO إلى Neutrino مطلقاً؛ بل يحتاج إلى محوّل OPL BDM/ETH جاهز وإعداد SMB صالح عند استخدام الشبكة.",
          "إذا تعذر مسار OPL المطلوب، يُحظر التشغيل بدلاً من التحويل الصامت."
        ],
        "zso-requires-opl": ["لا يمكن تمرير ZSO إلى Neutrino في هذا البناء. يحتاج إلى مسار OPL BDM/ETH جاهز؛ وإلا يُحظر التشغيل."],
        "zso-opl-missing": ["محرك OPL المضمّن غير موجود أو لم يجتز فحص السلامة. يمكن تحويل ISO إلى Neutrino، أما ZSO فيُحظر."],
        "zso-smb-config-invalid": ["إعدادات شبكة SMB/ETH ناقصة أو غير صالحة. يمكن تشغيل ISO عبر Neutrino، أما ZSO فيُحظر لأنه يحتاج إلى مفكك OPL ETH."],
        "zso-hdl-unsupported": ["لا يمكن تشغيل ZSO عبر مسار APA/HDL mini. استخدم ISO مدعوماً لهذا المسار على القرص."],
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
