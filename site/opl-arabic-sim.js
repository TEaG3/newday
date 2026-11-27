(() => {
  "use strict";
  const screenshots = {
    games: {
      title: "قائمة ألعاب USB", heading: "USB Games", source: "دليل مستخدم OPL",
      url: "https://www.ps2homebrew.org/Open-PS2-Loader-User-Guide/_images/3341986682-usb.png",
      alt: "OPL USB Games list with God of War II, Okami and Resident Evil 4",
      rows: [{ar:"God of War II",en:"SCES_542.06",value:"اللعبة المحددة"},{ar:"Okami",en:"USB GAME",value:"متاحة"},{ar:"Resident Evil 4",en:"USB GAME",value:"متاحة"}]
    },
    menu: {
      title: "القائمة الرئيسية", heading: "Main Menu", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/menu-open-ps2-loader-opl_ps2ss1-6404b96c40956-6404b96c805c9.jpg",
      alt: "Real OPL main menu screenshot",
      rows: [{ar:"الإعدادات",en:"Settings"},{ar:"إعدادات العرض",en:"Display Settings"},{ar:"إعدادات الصوت",en:"Audio Settings"},{ar:"إعدادات وحدة التحكم",en:"Controller Settings"},{ar:"إعدادات الرقابة الأبوية",en:"Parental Lock Settings"},{ar:"إعدادات الشبكة",en:"Network Settings"},{ar:"تحديث الشبكة",en:"Network Update"},{ar:"حول",en:"About"},{ar:"حفظ التغييرات",en:"Save Changes"},{ar:"خروج",en:"Exit"},{ar:"إيقاف التشغيل",en:"Power Off"}]
    },
    settings: {
      title: "إعدادات الأجهزة", heading: "Settings", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/bdm-device-start-mode-on-settings-open-ps2-loader-opl_ps2ss1-640618bce374e-640618bd30980-768x576.jpg",
      alt: "Real OPL settings screenshot showing BDM, HDD and ETH start modes",
      rows: [{ar:"وضع بدء جهاز USB",en:"BDM Start Mode",value:"Auto"},{ar:"وضع بدء القرص الداخلي",en:"HDD Device Start Mode",value:"Off"},{ar:"وضع بدء الشبكة",en:"ETH Device Start Mode",value:"Off"},{ar:"وضع بدء التطبيقات",en:"Applications Start Mode",value:"Off"},{ar:"القائمة الافتراضية",en:"Default Menu",value:"BDM Games"},{ar:"فحص تجزئة USB",en:"USB Fragmentation",value:"On"}]
    },
    display: {
      title: "إعدادات العرض", heading: "Display Settings", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/cover-art-on-display-settings-open-ps2-loader-opl_ps2ss1-6405e500adb0a-6405e500ed733-768x576.jpg",
      alt: "Real OPL display settings screenshot",
      rows: [{ar:"السمة",en:"Theme",value:"<OPL>"},{ar:"اللغة",en:"Language",value:"English (internal)"},{ar:"الترتيب التلقائي",en:"Automatic Sorting",value:"On"},{ar:"التحديث التلقائي",en:"Automatic Refresh",value:"Off"},{ar:"أغلفة الألعاب",en:"Cover Art",value:"On"},{ar:"الإشعارات",en:"Notifications",value:"On"},{ar:"نمط الفيديو",en:"Video Mode",value:"Auto"},{ar:"الشاشة العريضة",en:"Widescreen",value:"Off"}]
    },
    game: {
      title: "قائمة خيارات اللعبة", heading: "Final Fantasy X", source: "Le Crabe Info",
      url: "https://lecrabeinfo.net/app/uploads/2020/12/game-settings-open-ps2-loader-opl_ps2ss1-6405e097178f9-6405e097597be.jpg",
      alt: "Real OPL per-game options menu for Final Fantasy X",
      rows: [{ar:"إعدادات اللعبة",en:"Game Settings"},{ar:"إعدادات الغش",en:"Cheat Settings"},{ar:"ضبط GSM",en:"Configure GSM"},{ar:"بطاقة الذاكرة الافتراضية",en:"Configure VMC"},{ar:"ضبط PADEMU",en:"Configure PADEMU"},{ar:"حفظ التغييرات",en:"Save Changes"},{ar:"اختبار التغييرات",en:"Test Changes"},{ar:"إزالة الإعدادات",en:"Remove Settings"}]
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
    note.textContent = "المحدد: " + selected().ar + (selected().value ? " — " + selected().value : "") + ". استخدم × للاختيار و○ للرجوع.";
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
  function confirm() {
    if (state.screen === "games") { setScreen("game",true); note.textContent = "تم فتح قائمة خيارات اللعبة. هذه محاكاة للواجهة فقط."; return; }
    if (state.screen === "menu") {
      if (state.index === 0) setScreen("settings",true);
      else if (state.index === 1) setScreen("display",true);
      else note.textContent = "تم اختيار: " + selected().ar + ". العرض داخل المتصفح فقط.";
      return;
    }
    if (state.screen === "game" && state.index === 0) { note.textContent = "فتحت إعدادات اللعبة — التنقل تجريبي ولا يغيّر إعدادات ملف حقيقي."; return; }
    const row = selected();
    if (row.value === "On") row.value = "Off";
    else if (row.value === "Off") row.value = "On";
    else if (row.value === "Auto") row.value = "Manual";
    else if (row.value === "Manual") row.value = "Off";
    else if (row.value === "BDM Games") row.value = "USB Games";
    render();
    note.textContent = "قيمة تجريبية في عرض المتصفح: " + row.ar + " = " + (row.value || row.en);
  }
  function back() {
    const previous = state.history.pop();
    if (previous) setScreen(previous,false);
    else setScreen("games",false);
  }
  function input(key) {
    if (key === "up" || key === "left") move(-1);
    else if (key === "down" || key === "right") move(1);
    else if (key === "cross") confirm();
    else if (key === "circle") back();
    else if (key === "triangle") setScreen("game",true);
    else if (key === "square") { state.index = 0; render(); note.textContent = "تم تحديث العرض التجريبي."; }
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