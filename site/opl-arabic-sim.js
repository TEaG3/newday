(() => {
  "use strict";
  const screenshots = {
    games: {
      title: "قائمة ألعاب USB", heading: "USB Games", source: "دليل OPL",
      url: "https://www.ps2homebrew.org/Open-PS2-Loader-User-Guide/_images/3341986682-usb.png",
      sourceUrl: "https://www.ps2homebrew.org/Open-PS2-Loader-User-Guide/usb-mode.html",
      alt: "OPL USB Games list with God of War II, Okami and Resident Evil 4",
      rows: [
        {ar:"God of War II",en:"SCES_542.06",value:"المحدد"},
        {ar:"Okami",en:"USB GAME"},
        {ar:"Resident Evil 4",en:"USB GAME"}
      ]
    },
    hdd: {
      title: "ألعاب القرص الداخلي", heading: "HDD Games", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/jeux-hdd-disque-dur-sans-covers-open-ps2-loader-opl_ps2ss1-6405e53f42fe6-6405e53f84643.jpg",
      sourceUrl: "https://lecrabeinfo.net/tutoriels/ps2-installer-et-configurer-open-ps2-loader-opl/",
      alt: "Real OPL HDD Games list screenshot",
      rows: [
        {ar:"Devil May Cry",en:"HDD GAME"},
        {ar:"Final Fantasy X",en:"HDD GAME",value:"المحدد"},
        {ar:"Gran Turismo 4",en:"HDD GAME"},
        {ar:"Grand Theft Auto: Vice City",en:"HDD GAME"},
        {ar:"Grand Theft Auto III",en:"HDD GAME"},
        {ar:"Kingdom Hearts II",en:"HDD GAME"},
        {ar:"Pro Evolution Soccer",en:"HDD GAME"},
        {ar:"Silent Hill 2",en:"HDD GAME"},
        {ar:"Silent Hill 3",en:"HDD GAME"}
      ]
    },
    menu: {
      title: "القائمة الرئيسية", heading: "Main Menu", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/menu-open-ps2-loader-opl_ps2ss1-6404b96c40956-6404b96c805c9.jpg",
      sourceUrl: "https://lecrabeinfo.net/tutoriels/ps2-installer-et-configurer-open-ps2-loader-opl/",
      alt: "Real OPL main menu screenshot",
      rows: [
        {ar:"الإعدادات",en:"Settings"},
        {ar:"إعدادات العرض",en:"Display Settings"},
        {ar:"إعدادات الصوت",en:"Audio Settings"},
        {ar:"إعدادات وحدة التحكم",en:"Controller Settings"},
        {ar:"إعدادات الرقابة الأبوية",en:"Parental Lock Settings"},
        {ar:"إعدادات الشبكة",en:"Network Settings"},
        {ar:"تحديث الشبكة",en:"Network Update"},
        {ar:"حول",en:"About"},
        {ar:"حفظ التغييرات",en:"Save Changes"},
        {ar:"خروج",en:"Exit"},
        {ar:"إيقاف التشغيل",en:"Power Off"}
      ]
    },
    settings: {
      title: "الإعدادات الرئيسية", heading: "Settings", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/bdm-device-start-mode-on-settings-open-ps2-loader-opl_ps2ss1-640618bce374e-640618bd30980-768x576.jpg",
      sourceUrl: "https://lecrabeinfo.net/tutoriels/ps2-installer-et-configurer-open-ps2-loader-opl/",
      alt: "Real OPL main settings screen with device start modes",
      rows: [
        {ar:"تعطيل ألوان التشخيص",en:"Disable Debug Colors",value:"Off"},
        {ar:"مسار إعادة التشغيل داخل اللعبة",en:"IGR Path",value:"غير محدد"},
        {ar:"السماح بعمليات الكتابة",en:"Enable write operations",value:"Off"},
        {ar:"تذكّر آخر لعبة",en:"Remember last played game",value:"Off"},
        {ar:"فحص تجزئة USB",en:"Check USB Game Fragmentation",value:"On"},
        {ar:"مسارات USB وETH",en:"USB / ETH Prefix Paths",value:"غير محدد"},
        {ar:"إيقاف القرص تلقائيًا",en:"Automatic HDD spin down",value:"20 min"},
        {ar:"بدء جهاز USB",en:"USB device start mode",value:"Auto"},
        {ar:"بدء القرص الداخلي",en:"HDD device start mode",value:"Off"},
        {ar:"بدء الشبكة",en:"ETH device start mode",value:"Off"},
        {ar:"بدء التطبيقات",en:"Applications start mode",value:"Off"},
        {ar:"القائمة الافتراضية",en:"Default menu",value:"USB Games"}
      ]
    },
    display: {
      title: "إعدادات العرض", heading: "Display Settings", source: "دليل مستخدم OPL",
      url: "https://www.ps2homebrew.org/Open-PS2-Loader-User-Guide/_images/2489977405-display-settings.png",
      sourceUrl: "https://www.ps2homebrew.org/Open-PS2-Loader-User-Guide/display-settings.html",
      alt: "Real OPL display settings screenshot",
      rows: [
        {ar:"السمة",en:"Theme",value:"<OPL>"},
        {ar:"اللغة",en:"Language",value:"English (internal)"},
        {ar:"سرعة التمرير",en:"Scrolling",value:"Medium"},
        {ar:"الترتيب التلقائي",en:"Automatic sorting",value:"On"},
        {ar:"التحديث التلقائي",en:"Automatic Refresh",value:"Off"},
        {ar:"إظهار أغلفة الألعاب",en:"Enable Cover Art",value:"On"},
        {ar:"صفحة معلومات اللعبة",en:"Display info page",value:"Off"},
        {ar:"لون النص",en:"Text color",value:"Default"},
        {ar:"لون الحوار",en:"Dialog color",value:"Default"},
        {ar:"لون التحديد",en:"Selected color",value:"Default"},
        {ar:"لون الخلفية",en:"Background color",value:"Default"},
        {ar:"نمط الفيديو",en:"Video Mode",value:"Auto"},
        {ar:"الشاشة العريضة",en:"Wide screen",value:"Off"}
      ]
    },
    game: {
      title: "خيارات اللعبة", heading: "Game Settings", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/modes-game-settings-open-ps2-loader-opl_ps2ss1-6405e0e3da87a-6405e0e412f36-1024x475.jpg",
      sourceUrl: "https://lecrabeinfo.net/tutoriels/ps2-lire-des-jeux-ps2-depuis-une-cle-usb-avec-opl/",
      alt: "Real OPL Game Settings screen showing compatibility mode toggles",
      rows: [
        {ar:"الوضع 1 — القراءة الدقيقة",en:"Mode 1 · Accurate Reads",value:"Off"},
        {ar:"الوضع 2 — القراءة المتزامنة",en:"Mode 2 · Synchronous Mode",value:"Off"},
        {ar:"الوضع 3 — إلغاء ربط Syscalls",en:"Mode 3 · Unhook Syscalls",value:"Off"},
        {ar:"الوضع 4 — وضع PSS صفر",en:"Mode 4 · 0 PSS mode",value:"Off"},
        {ar:"الوضع 5 — محاكاة DVD-DL",en:"Mode 5 · Emulate DVD-DL",value:"Off"},
        {ar:"الوضع 6 — تعطيل IGR",en:"Mode 6 · Disable IGR",value:"Off"},
        {ar:"الوضع 7 — تخزين الوحدات العالي",en:"Mode 7 · High module storage",value:"Off"},
        {ar:"الوضع 8 — إخفاء DEV9",en:"Mode 8 · Hide DEV9 module",value:"Off"},
        {ar:"إعداد GSM",en:"Configure GSM"},
        {ar:"وضع DMA",en:"DMA mode",value:"غير محدد"},
        {ar:"إعداد بطاقة الذاكرة الافتراضية",en:"Configure VMC"},
        {ar:"معرّف اللعبة",en:"Game ID",value:"غير محدد"},
        {ar:"ملف ELF مخصص",en:"Custom ELF",value:"غير محدد"},
        {ar:"حفظ التغييرات",en:"Save Changes"},
        {ar:"اختبار",en:"Test"},
        {ar:"تنزيل الإعدادات الافتراضية",en:"Download defaults"},
        {ar:"إزالة جميع الإعدادات",en:"Remove all settings"}
      ]
    }
  };
  const state = {screen:"games",index:0,history:[]};
  const image = document.getElementById("mainScreen");
  const title = document.getElementById("screenTitle");
  const heading = document.getElementById("menuHeading");
  const list = document.getElementById("menuList");
  const source = document.getElementById("imageSource");
  const current = document.getElementById("currentSelection");
  const note = document.getElementById("menuNote");
  const switchButtons = Array.from(document.querySelectorAll("[data-screen]"));
  const data = () => screenshots[state.screen];
  const selected = () => data().rows[state.index] || data().rows[0];

  function render() {
    const screen = data();
    if (image.getAttribute("src") !== screen.url) image.src = screen.url;
    image.alt = screen.alt;
    title.textContent = screen.title;
    heading.textContent = screen.heading;
    source.textContent = "المصدر: " + screen.source;
    current.textContent = "المحدد: " + selected().ar;
    list.replaceChildren();
    screen.rows.forEach((row,index) => {
      const button = document.createElement("button");
      button.type = "button";
      button.className = "menu-row";
      button.setAttribute("role","option");
      button.setAttribute("aria-current",String(index === state.index));
      const label = document.createElement("span");
      label.textContent = row.ar;
      const meta = document.createElement("small");
      meta.textContent = row.value ? row.en + " · " + row.value : row.en;
      button.append(label,meta);
      button.addEventListener("click",() => { state.index = index; render(); });
      list.appendChild(button);
    });
    switchButtons.forEach(button => button.setAttribute("aria-pressed",String(button.dataset.screen === state.screen)));
    note.textContent = "المحدد: " + selected().ar + (selected().value ? " — " + selected().value : "") + ". " + controlHint();
  }

  function controlHint() {
    if (state.screen === "games" || state.screen === "hdd") return "↑↓ للقائمة، ←→ لتبديل USB/HDD، Start للقائمة، △ لخيارات اللعبة، × للتشغيل التجريبي.";
    if (state.screen === "menu") return "↑↓ للتنقّل، × للدخول، ○ للعودة.";
    return "↑↓ للتنقّل، ←→ لتغيير القيمة، × للتأكيد، ○ للعودة.";
  }

  function setScreen(id,remember) {
    if (!screenshots[id]) return;
    if (remember && state.screen !== id) state.history.push(state.screen);
    state.screen = id;
    state.index = 0;
    render();
  }

  function move(delta) {
    const length = data().rows.length;
    state.index = (state.index + delta + length) % length;
    render();
  }

  function adjustValue(delta) {
    const row = selected();
    if (!row.value) {
      note.textContent = "هذا العنصر يفتح صفحة فرعية في OPL؛ العرض التفاعلي يشرح القائمة ولا ينفّذ إعدادًا حقيقيًا.";
      return;
    }
    const choices = {
      "Off":["Off","On"],
      "On":["On","Off"],
      "Auto":["Auto","Manual","Off"],
      "Manual":["Manual","Auto","Off"],
      "Medium":["Slot","Medium","Fast"],
      "USB Games":["USB Games","HDD Games"],
      "20 min":["0 min","5 min","10 min","15 min","20 min"]
    };
    const values = choices[row.value] || [row.value,"غير محدد"];
    const idx = values.indexOf(row.value);
    row.value = values[(idx + (delta < 0 ? values.length - 1 : 1)) % values.length];
    render();
    note.textContent = "قيمة عرض تجريبي فقط: " + row.ar + " = " + row.value + " — لا تُحفظ في ملفات OPL.";
  }

  function confirm() {
    if (state.screen === "games" || state.screen === "hdd") {
      note.textContent = "محاكاة تشغيل: " + selected().ar + " — لا يتم تشغيل لعبة أو ملف ELF في المتصفح.";
      return;
    }
    if (state.screen === "menu") {
      const index = state.index;
      if (index === 0) setScreen("settings",true);
      else if (index === 1) setScreen("display",true);
      else note.textContent = "تم تحديد " + selected().ar + " في قائمة OPL — هذا عرض تفاعلي فقط.";
      return;
    }
    if (selected().value) adjustValue(1);
    else note.textContent = "تم تحديد " + selected().ar + " — لم يتم تغيير ملفات أو إعدادات حقيقية.";
  }

  function back() {
    const previous = state.history.pop();
    if (previous) setScreen(previous,false);
    else setScreen("games",false);
  }

  function input(key) {
    if (key === "up") move(-1);
    else if (key === "down") move(1);
    else if (key === "left" || key === "right") {
      if (state.screen === "games") setScreen("hdd",true);
      else if (state.screen === "hdd") setScreen("games",true);
      else adjustValue(key === "left" ? -1 : 1);
    }
    else if (key === "cross") confirm();
    else if (key === "circle") back();
    else if (key === "triangle" && (state.screen === "games" || state.screen === "hdd")) setScreen("game",true);
    else if (key === "square") { state.index = 0; render(); note.textContent = "انتقلت إلى أول عنصر في القائمة."; }
    else if (key === "start") setScreen("menu",true);
  }
  switchButtons.forEach(button => button.addEventListener("click",() => setScreen(button.dataset.screen,true)));
  document.querySelectorAll("[data-key]").forEach(button => button.addEventListener("click",() => input(button.dataset.key)));
  document.addEventListener("keydown",event => {
    const tag = event.target && event.target.tagName;
    if (tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT" || event.altKey || event.ctrlKey || event.metaKey) return;
    const keys = {ArrowUp:"up",ArrowDown:"down",ArrowLeft:"left",ArrowRight:"right",Enter:"cross",Escape:"circle",Backspace:"circle",x:"cross",X:"cross",o:"circle",O:"circle",q:"triangle",Q:"triangle"," ":"square"};
    if (event.key === "s" || event.key === "S" || event.key === "F1") keys[event.key] = "start";
    if (keys[event.key]) { event.preventDefault(); input(keys[event.key]); }
  });
  let previousButtons = [];
  function pollGamepad() {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    const pad = Array.from(pads).find(Boolean);
    if (pad) {
      const mapping = [[0,"cross"],[1,"circle"],[2,"square"],[3,"triangle"],[9,"start"],[12,"up"],[13,"down"],[14,"left"],[15,"right"]];
      mapping.forEach(([index,action]) => {
        const pressed = !!(pad.buttons[index] && pad.buttons[index].pressed);
        if (pressed && !previousButtons[index]) input(action);
        previousButtons[index] = pressed;
      });
      const axes = [[0,pad.axes[0] < -.7,"left"],[1,pad.axes[0] > .7,"right"],[2,pad.axes[1] < -.7,"up"],[3,pad.axes[1] > .7,"down"]];
      axes.forEach(([index,pressed,action]) => {
        if (pressed && !previousButtons["axis" + index]) input(action);
        previousButtons["axis" + index] = pressed;
      });
    } else previousButtons = [];
    requestAnimationFrame(pollGamepad);
  }
  const viewer = document.getElementById("viewer");
  const viewerImage = document.getElementById("viewerImage");
  const viewerTitle = document.getElementById("viewerTitle");
  document.querySelectorAll("[data-shot]").forEach(button => button.addEventListener("click",event => {
    if (event.target.closest("a")) return;
    const screen = screenshots[button.dataset.shot];
    if (!screen) return;
    viewerImage.src = screen.url;
    viewerImage.alt = screen.alt;
    viewerTitle.textContent = screen.title;
    viewer.showModal();
  }));
  document.getElementById("viewerClose").addEventListener("click",() => viewer.close());
  viewer.addEventListener("click",event => { if (event.target === viewer) viewer.close(); });
  render();
  requestAnimationFrame(pollGamepad);
})();