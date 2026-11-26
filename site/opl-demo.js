/* Browser-only OPL route explanation. No ELF, emulator or game is executed. */
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
    const format = input && input.format, source = input && input.source;
    const ready = !input || input.oplRuntime !== "missing" && input.oplRuntime !== false;
    if (!["iso", "zso", "ul"].includes(format) ||
        !["usb", "bdm", "hdl", "other"].includes(source))
      return { id: "unsupported", engine: "none", ok: false, reason: "invalid-selection" };
    if (!ready)
      return { id: "opl-missing", engine: "none", ok: false, reason: "opl-runtime-missing" };
    if (format === "ul")
      return source === "usb" ?
        { id: "opl-ul", engine: "OPL", ok: true, reason: "usb-ul" } :
        { id: "unsupported", engine: "none", ok: false, reason: "ul-requires-usb" };
    if (source === "usb" || source === "bdm")
      return { id: "opl-bdm", engine: "OPL", ok: true, reason: "opl-bdm-adapter" };
    if (source === "hdl" && format === "iso")
      return { id: "opl-hdd", engine: "OPL", ok: true, reason: "opl-hdd-adapter" };
    return { id: "unsupported", engine: "none", ok: false, reason: "no-opl-source-adapter" };
  }

  const COPY = {
    en: {
      result: "Selected route",
      titles: {
        "opl-ul": "OPL · USBExtreme UL",
        "opl-bdm": "OPL · BDM direct boot",
        "opl-hdd": "OPL · HD Loader mini-launch",
        "opl-missing": "Launch blocked · OPL runtime unavailable",
        unsupported: "Launch blocked · no OPL adapter"
      },
      sources: { usb: "USB", bdm: "MX4SIO / iLink / ATA HDD", hdl: "APA / HD Loader", other: "Disabled network or MMCE source" },
      details: {
        "opl-ul": [
          "TEa.G3 checks ul.cfg and verifies every declared split part before launch.",
          "The hand-off carries the title, startup ID, media type, part count and return-stub path.",
          "OPL owns the USBExtreme split-part read path after hand-off.",
          "Real-console boot and sustained-read validation are still required."
        ],
        "opl-bdm": [
          "TEa.G3 selects a USB ISO/ZSO or a supported BDM device entry.",
          "The hand-off passes the exact BDM driver, device unit, filename and startup ID to OPL.",
          "OPL reopens the device and performs game-sector reads; launcher-side scanning is not proof of playback.",
          "Hardware validation is still pending for each physical source."
        ],
        "opl-hdd": [
          "TEa.G3 reads APA/HD Loader partition metadata and validates the game start sector, size and disc type.",
          "The mini-launch arguments are passed to OPL; OPL reads its existing configured HDD partition.",
          "The launcher does not create, format or rewrite HDD partitions.",
          "Hardware boot and sustained-read tests are still required."
        ],
        "opl-missing": [
          "The embedded OPL.ELF is missing or failed its integrity check.",
          "TEa.G3 refuses the launch. It never switches to a different game engine."
        ],
        unsupported: [
          "This format/source pair has no enabled OPL adapter in this build.",
          "SMB, UDPBD, UDPFS game streaming and MMCE game-source launching are disabled.",
          "Use a supported local source or wait until an OPL-only reader and its hardware tests are complete."
        ]
      },
      hardware: "Browser simulation only · no PS2 ELF, emulator or game is executed.",
      supported: "Adapter exists · physical-PS2 test pending",
      blocked: "Unsupported / blocked",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    },
    ar: {
      result: "مسار التشغيل المختار",
      titles: {
        "opl-ul": "OPL · USBExtreme UL",
        "opl-bdm": "OPL · تشغيل BDM مباشر",
        "opl-hdd": "OPL · وضع HD Loader المصغر",
        "opl-missing": "تم حظر التشغيل · محرك OPL غير جاهز",
        unsupported: "تم حظر التشغيل · لا يوجد محول OPL"
      },
      sources: { usb: "USB", bdm: "MX4SIO / iLink / ATA HDD", hdl: "APA / HD Loader", other: "مصدر شبكة أو MMCE معطل" },
      details: {
        "opl-ul": [
          "يفحص TEa.G3 ملف ul.cfg ويتحقق من وجود كل الأجزاء المعلنة قبل التشغيل.",
          "يمرّر إلى OPL اسم اللعبة ورقم بدء التشغيل ونوع الوسيط وعدد الأجزاء ومسار العودة.",
          "يتولى OPL قراءة أجزاء USBExtreme بعد تسليم التحكم إليه.",
          "ما زال يلزم اختبار الإقلاع والقراءة المستمرة على جهاز حقيقي."
        ],
        "opl-bdm": [
          "يحدد TEa.G3 لعبة USB ISO/ZSO أو لعبة من جهاز BDM مدعوم.",
          "يمرّر إلى OPL اسم محرك BDM ورقم الجهاز واسم الملف ورقم بدء التشغيل بدقة.",
          "يعيد OPL فتح الجهاز ويتولى قراءة قطاعات اللعبة؛ فحص الملفات من الواجهة لا يثبت نجاح التشغيل.",
          "ما زال اختبار العتاد مطلوباً لكل مصدر تخزين فعلي."
        ],
        "opl-hdd": [
          "يقرأ TEa.G3 بيانات قسم APA/HD Loader ويتحقق من قطاع البداية والحجم ونوع القرص.",
          "يمرّر معاملات mini-launch إلى OPL، ويقرأ OPL قسم HDD المهيأ لديه.",
          "لا ينشئ المشغّل أقسام القرص ولا يهيئها ولا يغيّرها.",
          "ما زال يلزم اختبار الإقلاع والقراءة المستمرة على جهاز حقيقي."
        ],
        "opl-missing": [
          "ملف OPL.ELF المضمّن غير موجود أو لم يجتز فحص السلامة.",
          "يرفض TEa.G3 التشغيل ولا ينتقل إلى أي محرك ألعاب آخر."
        ],
        unsupported: [
          "لا يوجد في هذا البناء محول OPL مفعّل لهذه الصيغة ومصدر التخزين.",
          "تشغيل الألعاب عبر SMB وUDPBD وUDPFS، وكذلك MMCE كمصدر ألعاب، معطّل.",
          "استخدم مصدراً محلياً مدعوماً أو انتظر اكتمال قارئ OPL واختباره على العتاد."
        ]
      },
      hardware: "محاكاة داخل المتصفح فقط · لا يتم تشغيل ELF أو محاكي أو لعبة PS2 هنا.",
      supported: "المحول موجود · اختبار PS2 الفعلي متبقٍ",
      blocked: "غير مدعوم / محظور",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    }
  };

  function render(form) {
    const output = form.closest("[data-opl-widget]").querySelector("[data-opl-result]");
    const ar = form.ownerDocument.documentElement.lang === "ar", c = ar ? COPY.ar : COPY.en;
    const input = {
      format: form.elements.format.value,
      source: form.elements.source.value,
      oplRuntime: form.elements.oplRuntime ? form.elements.oplRuntime.value : "ready"
    };
    const route = resolveRoute(input);
    const details = c.details[route.id] || c.details.unsupported;
    output.textContent = "";
    output.className = "opl-result " + (route.ok ? "is-supported" : "is-blocked");
    const head = form.ownerDocument.createElement("div"); head.className = "opl-result-head";
    const title = form.ownerDocument.createElement("h3"); title.textContent = c.titles[route.id] || c.titles.unsupported;
    const status = form.ownerDocument.createElement("span"); status.className = "opl-status";
    status.textContent = route.ok ? c.supported : c.blocked;
    head.append(title, status); output.appendChild(head);
    const meta = form.ownerDocument.createElement("p"); meta.className = "opl-result-meta";
    meta.textContent = c.result + ": " + c[input.format] + " · " + c.sources[input.source];
    output.appendChild(meta);
    const list = form.ownerDocument.createElement("ol"); list.className = "opl-route-steps";
    details.forEach((line, i) => {
      const item = form.ownerDocument.createElement("li");
      const number = form.ownerDocument.createElement("span"); number.className = "step-number";
      number.textContent = String(i + 1).padStart(2, "0");
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
        form.elements.format.value = "iso";
        form.elements.source.value = "usb";
        if (form.elements.oplRuntime) form.elements.oplRuntime.value = "ready";
        render(form);
      });
      ["format", "source", "oplRuntime"].forEach(name => {
        if (form.elements[name]) form.elements[name].addEventListener("change", () => render(form));
      });
      render(form);
    });
    document.addEventListener("tea-g3:languagechange", () =>
      document.querySelectorAll("[data-opl-form]").forEach(render));
  }
  return { resolveRoute, mount };
});
