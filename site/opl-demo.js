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
    if (!["iso", "zso", "ul"].includes(format) || !["usb", "other"].includes(source) ||
        !["auto", "neutrino", "opl"].includes(preference)) {
      return { id: "unsupported", engine: "none", ok: false, reason: "invalid" };
    }
    if (format === "ul") {
      return source === "usb" ? { id: "opl-ul", engine: "OPL", ok: true, reason: "ul-usb" } :
        { id: "unsupported", engine: "none", ok: false, reason: "ul-needs-usb" };
    }
    if (preference === "opl") {
      return source === "usb" ? { id: "opl-usb", engine: "OPL", ok: true, reason: "manual-opl-usb" } :
        { id: "unsupported", engine: "none", ok: false, reason: "opl-needs-usb" };
    }
    return { id: "neutrino", engine: "Neutrino", ok: true, reason: "default-neutrino" };
  }

  const COPY = {
    en: {
      result: "Selected launch route", unsupported: "No OPL hand-off is wired for this selection",
      oplUlTitle: "Embedded OPL · USBExtreme UL", oplUsbTitle: "Embedded OPL · USB ISO/ZSO direct boot",
      neutrinoTitle: "Neutrino · existing launch path",
      ulUsb: [
        "TEa.G3 scans ul.cfg on the USB device.",
        "Each record is parsed and the game is listed only when every declared ul.* split part is present.",
        "TEa.G3 passes the selected UL title, startup ID, media type and part count to the embedded OPL UL launch path.",
        "OPL performs the USBExtreme split-part reads."
      ],
      oplUsb: [
        "TEa.G3 selects the USB ISO/ZSO game entry.",
        "The [juegos] motor = opl preference selects the USB OPL direct-boot route.",
        "The embedded OPL receives the selected image and its game metadata.",
        "This is a manual OPL route; auto keeps ISO/ZSO on Neutrino."
      ],
      neutrino: [
        "TEa.G3 keeps the selected ISO/ZSO on its existing Neutrino path.",
        "The auto and neutrino preferences both resolve to Neutrino for ISO/ZSO.",
        "TEa.G3 passes the selected source, image path and supported game options to Neutrino."
      ],
      ulNeedsUsb: [
        "The UL scanner and direct-boot adapter currently accept USB only.",
        "A UL game on the selected non-USB source is not routed to OPL or Neutrino by this integration.",
        "Move the UL set to USB before using the integrated OPL UL path."
      ],
      oplNeedsUsb: [
        "The manual OPL direct-boot adapter in TEa.G3 currently accepts USB ISO/ZSO only.",
        "No OPL hand-off is implemented here for the selected non-USB source.",
        "Choose auto or neutrino to keep the existing Neutrino route for this source."
      ],
      invalid: ["Choose a supported format, source and engine preference."],
      hardware: "Browser simulation only · no PS2 ELF, emulator or game is executed here.",
      supported: "Supported by the current TEa.G3 routing code", blocked: "Not routed by the current OPL adapter",
      sourceUsb: "USB", sourceOther: "Other source", auto: "Auto", neutrino: "Neutrino", opl: "OPL (manual)",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    },
    ar: {
      result: "مسار التشغيل المختار", unsupported: "لا يوجد ربط OPL لهذا الاختيار حالياً",
      oplUlTitle: "محرك OPL المضمّن · USBExtreme UL", oplUsbTitle: "محرك OPL المضمّن · تشغيل USB ISO/ZSO مباشرة",
      neutrinoTitle: "Neutrino · مسار التشغيل الحالي",
      ulUsb: [
        "يفحص TEa.G3 ملف ul.cfg الموجود على وحدة USB.",
        "يحلّل كل سجل، ولا يعرض اللعبة إلا عند وجود جميع أجزاء ul.* المعلنة.",
        "يمرّر TEa.G3 اسم لعبة UL ورقم بدء التشغيل ونوع الوسيط وعدد الأجزاء إلى مسار UL في OPL المضمّن.",
        "يتولى OPL قراءة أجزاء USBExtreme."
      ],
      oplUsb: [
        "يحدد TEa.G3 لعبة USB بصيغة ISO أو ZSO.",
        "يؤدي اختيار [juegos] motor = opl إلى تحديد مسار الإقلاع المباشر عبر OPL لوحدة USB.",
        "يستقبل OPL المضمّن ملف اللعبة وبياناته.",
        "هذا مسار OPL يدوي؛ أما auto فيُبقي ISO/ZSO على Neutrino."
      ],
      neutrino: [
        "يبقي TEa.G3 لعبة ISO/ZSO على مسار Neutrino الحالي.",
        "كل من auto وneutrino يختاران Neutrino لألعاب ISO/ZSO.",
        "يمرّر TEa.G3 مصدر اللعبة ومسار الصورة وخيارات اللعبة المدعومة إلى Neutrino."
      ],
      ulNeedsUsb: [
        "ماسح UL ومحوّل الإقلاع المباشر الحالي يقبلان USB فقط.",
        "لعبة UL الموجودة على مصدر آخر لا يوجّهها هذا التكامل إلى OPL أو Neutrino.",
        "انقل مجموعة UL إلى USB لاستخدام مسار OPL المضمّن."
      ],
      oplNeedsUsb: [
        "محوّل الإقلاع المباشر اليدوي إلى OPL في TEa.G3 يقبل حالياً USB ISO/ZSO فقط.",
        "لا يوجد في هذا التكامل تمرير OPL لمصدر التخزين الآخر المحدد.",
        "اختر auto أو neutrino للإبقاء على مسار Neutrino الحالي لهذا المصدر."
      ],
      invalid: ["اختر صيغة ومصدراً وتفضيل محرك مدعوماً."],
      hardware: "محاكاة داخل المتصفح فقط · لا يتم تشغيل ELF أو محاكي أو لعبة PS2 هنا.",
      supported: "مدعوم في منطق التوجيه الحالي لـ TEa.G3", blocked: "غير موصول عبر محوّل OPL الحالي",
      sourceUsb: "USB", sourceOther: "مصدر آخر", auto: "تلقائي", neutrino: "Neutrino", opl: "OPL (يدوي)",
      iso: "ISO", zso: "ZSO", ul: "USBExtreme UL"
    }
  };

  function render(form) {
    const output = form.closest("[data-opl-widget]").querySelector("[data-opl-result]");
    const ar = form.ownerDocument.documentElement.lang === "ar", c = ar ? COPY.ar : COPY.en;
    const input = { format: form.elements.format.value, source: form.elements.source.value, preference: form.elements.preference.value };
    const route = resolveRoute(input);
    const details = route.id === "opl-ul" ? c.ulUsb : route.id === "opl-usb" ? c.oplUsb :
      route.id === "neutrino" ? c.neutrino : route.reason === "ul-needs-usb" ? c.ulNeedsUsb :
      route.reason === "opl-needs-usb" ? c.oplNeedsUsb : c.invalid;
    const title = route.id === "opl-ul" ? c.oplUlTitle : route.id === "opl-usb" ? c.oplUsbTitle :
      route.id === "neutrino" ? c.neutrinoTitle : c.unsupported;
    const formatLabel = c[input.format] || input.format;
    const sourceLabel = input.source === "usb" ? c.sourceUsb : c.sourceOther;
    const preferenceLabel = c[input.preference] || input.preference;
    output.textContent = "";
    output.className = "opl-result " + (route.ok ? "is-supported" : "is-blocked");

    const head = form.ownerDocument.createElement("div"); head.className = "opl-result-head";
    const titleNode = form.ownerDocument.createElement("h3"); titleNode.textContent = title;
    const status = form.ownerDocument.createElement("span"); status.className = "opl-status";
    status.textContent = route.ok ? c.supported : c.blocked;
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
        form.elements.preference.value = "auto"; render(form);
      });
      ["format", "source", "preference"].forEach(name => form.elements[name].addEventListener("change", () => render(form)));
      render(form);
    });
    document.addEventListener("nuld:languagechange", () => document.querySelectorAll("[data-opl-form]").forEach(render));
  }
  return { resolveRoute, mount };
});
