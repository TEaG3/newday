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
        {ar:"إعداد الشبكة",en:"Network Config"},
        {ar:"تحديث الشبكة",en:"Network Update"},
        {ar:"حفظ التغييرات",en:"Save Changes"},
        {ar:"إعدادات الغش",en:"Cheat Settings"},
        {ar:"حول",en:"About"},
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

  const COVER_BASE = "https://raw.githubusercontent.com/xlenore/ps2-covers/main/covers/default/";
  const games = [
    {id:"gow2",title:"God of War II",serial:"SCES-54206",genre:"أكشن",storage:"USB",cover:COVER_BASE+"SCES-54206.jpg",description:"مغامرة أكشن أسطورية تدور حول كريتوس وصراعه مع آلهة الأولمب."},
    {id:"okami",title:"Okami",serial:"SLES-54439",genre:"مغامرات",storage:"USB",cover:COVER_BASE+"SLES-54439.jpg",description:"مغامرة فنية بعالم مستوحى من الأساطير اليابانية ورسم يشبه الحبر."},
    {id:"re4",title:"Resident Evil 4",serial:"SLES-53702",genre:"رعب",storage:"USB",cover:COVER_BASE+"SLES-53702.jpg",description:"رعب وبقاء مع استكشاف ومواجهات في قرية غامضة."},
    {id:"dmc",title:"Devil May Cry",serial:"SLES-50358",genre:"أكشن",storage:"USB",cover:COVER_BASE+"SLES-50358.jpg",description:"قتال سريع بالسيوف والأسلحة مع دانتي في مواجهة قوى الشياطين."},
    {id:"ffx",title:"Final Fantasy X",serial:"SLUS-20312",genre:"مغامرات",storage:"USB",cover:COVER_BASE+"SLUS-20312.jpg",description:"رحلة تقمّص أدوار وقصة سينمائية في عالم سبيرا."},
    {id:"gt4",title:"Gran Turismo 4",serial:"SCES-51719",genre:"سباقات",storage:"HDD",cover:COVER_BASE+"SCES-51719.jpg",description:"محاكاة قيادة وسباقات تضم مجموعة واسعة من السيارات والحلبات."},
    {id:"gtavc",title:"Grand Theft Auto: Vice City",serial:"SLES-51061",genre:"عالم مفتوح",storage:"HDD",cover:COVER_BASE+"SLES-51061.jpg",description:"مدينة مفتوحة بطابع الثمانينيات ومهام قيادة واستكشاف."},
    {id:"gta3",title:"Grand Theft Auto III",serial:"SLES-50330",genre:"عالم مفتوح",storage:"HDD",cover:COVER_BASE+"SLES-50330.jpg",description:"تجربة عالم مفتوح في Liberty City مع مهام ومركبات متنوعة."},
    {id:"kh2",title:"Kingdom Hearts II",serial:"SLES-54114",genre:"مغامرات",storage:"USB",cover:COVER_BASE+"SLES-54114.jpg",description:"مغامرة أكشن وتقمّص أدوار تجمع عوالم خيالية وشخصيات متعددة."},
    {id:"sh2",title:"Silent Hill 2",serial:"SLES-50382",genre:"رعب",storage:"HDD",cover:COVER_BASE+"SLES-50382.jpg",description:"رعب نفسي يعتمد على الأجواء والغموض والاستكشاف."},
    {id:"gow",title:"God of War",serial:"SCUS-97399",genre:"أكشن",storage:"USB",cover:COVER_BASE+"SCUS-97399.jpg",description:"بداية رحلة كريتوس في مغامرة أكشن ومواجهات أسطورية."},
    {id:"gtasa",title:"Grand Theft Auto: San Andreas",serial:"SLES-52541",genre:"عالم مفتوح",storage:"HDD",cover:COVER_BASE+"SLES-52541.jpg",description:"عالم مفتوح واسع يتضمن القيادة والاستكشاف ومجموعة كبيرة من المهام."}
  ];
  const libraryState = {storage:"all",genre:"all",query:"",sort:"featured",view:"grid",favoritesOnly:false,selectedId:"gow2"};
  const favorites = new Set(JSON.parse(localStorage.getItem("oplArabicFavorites") || "[]"));
  function saveFavorites() {
    try { localStorage.setItem("oplArabicFavorites",JSON.stringify(Array.from(favorites))); } catch (_) {}
  }
  const gameGrid = document.getElementById("gameGrid");
  const gameDetail = document.getElementById("gameDetail");
  const libraryCount = document.getElementById("libraryCount");
  const oplStatus = document.getElementById("oplStatus");
  const gameSearch = document.getElementById("gameSearch");
  const genreFilter = document.getElementById("genreFilter");
  const sortGames = document.getElementById("sortGames");
  const favoritesOnlyButton = document.getElementById("favoritesOnly");
  const storageButtons = Array.from(document.querySelectorAll("[data-storage]"));
  const viewButtons = Array.from(document.querySelectorAll("[data-library-view]"));
  function visibleGames() {
    const query = libraryState.query.trim().toLocaleLowerCase();
    let result = games.filter(game =>
      (libraryState.storage === "all" || game.storage === libraryState.storage) &&
      (libraryState.genre === "all" || game.genre === libraryState.genre) &&
      (!libraryState.favoritesOnly || favorites.has(game.id)) &&
      (!query || (game.title + " " + game.serial + " " + game.genre).toLocaleLowerCase().includes(query))
    );
    if (libraryState.sort === "az") result = result.slice().sort((a,b) => a.title.localeCompare(b.title));
    if (libraryState.sort === "za") result = result.slice().sort((a,b) => b.title.localeCompare(a.title));
    return result;
  }
  function selectedGame() {
    const shown = visibleGames();
    return shown.find(game => game.id === libraryState.selectedId) || shown[0] || null;
  }
  function makeCover(game, className) {
    const art = document.createElement("div");
    art.className = className;
    const img = document.createElement("img");
    img.src = game.cover;
    img.alt = "غلاف " + game.title;
    img.loading = "lazy";
    img.decoding = "async";
    img.addEventListener("error",() => {
      art.classList.add("is-fallback");
      img.remove();
      const fallback = document.createElement("span");
      fallback.textContent = game.title;
      art.appendChild(fallback);
    },{once:true});
    art.appendChild(img);
    return art;
  }
  function renderLibrary() {
    if (!gameGrid || !gameDetail) return;
    const shown = visibleGames();
    if (!shown.some(game => game.id === libraryState.selectedId)) {
      libraryState.selectedId = shown[0]?.id || null;
    }
    libraryCount.textContent = shown.length + " / " + games.length + " لعبة";
    if (oplStatus) oplStatus.textContent = (libraryState.storage === "all" ? "USB / HDD" : libraryState.storage) + " · " + shown.length + " GAMES";
    gameGrid.replaceChildren();
    gameGrid.classList.toggle("is-list",libraryState.view === "list");
    gameGrid.classList.toggle("is-opl",libraryState.view === "opl");
    storageButtons.forEach(button => button.setAttribute("aria-pressed",String(button.dataset.storage === libraryState.storage)));
    viewButtons.forEach(button => button.setAttribute("aria-pressed",String(button.dataset.libraryView === libraryState.view)));
    favoritesOnlyButton.setAttribute("aria-pressed",String(libraryState.favoritesOnly));
    if (!shown.length) {
      const empty = document.createElement("div");
      empty.className = "library-empty";
      empty.textContent = libraryState.favoritesOnly ? "لا توجد ألعاب مضافة إلى المفضلة بعد." : "لا توجد ألعاب تطابق البحث أو عوامل التصفية.";
      gameGrid.appendChild(empty);
    }
    shown.forEach(game => {
      const card = document.createElement("button");
      card.type = "button";
      card.className = "game-card";
      card.setAttribute("role","option");
      card.setAttribute("aria-pressed",String(game.id === libraryState.selectedId));
      card.setAttribute("aria-label","اختيار " + game.title);
      card.appendChild(makeCover(game,"game-card-art"));
      const body = document.createElement("span");
      body.className = "game-card-body";
      const name = document.createElement("span");
      name.className = "game-card-title";
      name.textContent = game.title;
      const meta = document.createElement("span");
      meta.className = "game-card-meta";
      const serial = document.createElement("span");
      serial.textContent = game.serial;
      const storage = document.createElement("span");
      storage.textContent = game.storage;
      meta.append(serial,storage);
      const tags = document.createElement("span");
      tags.className = "game-card-tags";
      const genre = document.createElement("span");
      genre.className = "game-tag";
      genre.textContent = game.genre;
      tags.appendChild(genre);
      if (favorites.has(game.id)) {
        const fav = document.createElement("span");
        fav.className = "game-tag";
        fav.textContent = "★ مفضلة";
        tags.appendChild(fav);
      }
      body.append(name,meta,tags);
      card.appendChild(body);
      card.addEventListener("click",() => {
        libraryState.selectedId = game.id;
        renderLibrary();
        if (state.screen === "games" || state.screen === "hdd") note.textContent = "المحدد الآن: " + game.title + " · " + game.serial + " — عرض OPL فقط.";
      });
      gameGrid.appendChild(card);
    });
    gameDetail.replaceChildren();
    const game = selectedGame();
    const kicker = document.createElement("span");
    kicker.className = "detail-kicker";
    kicker.textContent = "SELECTED GAME / OPL";
    gameDetail.appendChild(kicker);
    if (!game) {
      const empty = document.createElement("p");
      empty.className = "detail-description";
      empty.textContent = "اختر لعبة بعد إزالة عوامل التصفية.";
      gameDetail.appendChild(empty);
      return;
    }
    gameDetail.appendChild(makeCover(game,"detail-art"));
    const title = document.createElement("h4");
    title.className = "detail-title";
    title.textContent = game.title;
    const description = document.createElement("p");
    description.className = "detail-description";
    description.textContent = game.description;
    const meta = document.createElement("div");
    meta.className = "detail-meta";
    [["رقم اللعبة",game.serial],["التخزين",game.storage],["التصنيف",game.genre]].forEach(([label,value]) => {
      const row = document.createElement("div");
      row.className = "detail-meta-row";
      const l = document.createElement("span");
      l.textContent = label;
      const v = document.createElement("b");
      v.textContent = value;
      row.append(l,v);
      meta.appendChild(row);
    });
    const actions = document.createElement("div");
    actions.className = "detail-actions";
    const launch = document.createElement("button");
    launch.type = "button";
    launch.className = "detail-launch";
    launch.textContent = "اختيار اللعبة";
    launch.addEventListener("click",() => {
      note.textContent = "تم اختيار " + game.title + " للعرض التجريبي — لا يتم تشغيل لعبة فعلية من المتصفح.";
      current.textContent = "المحدد: " + game.title;
    });
    const favorite = document.createElement("button");
    favorite.type = "button";
    favorite.textContent = favorites.has(game.id) ? "★ إزالة من المفضلة" : "☆ أضف للمفضلة";
    favorite.addEventListener("click",() => {
      if (favorites.has(game.id)) favorites.delete(game.id);
      else favorites.add(game.id);
      saveFavorites();
      renderLibrary();
    });
    actions.append(launch,favorite);
    gameDetail.append(title,description,meta,actions);
    if (current) current.textContent = "المحدد: " + game.title;
    if (oplStatus && game) oplStatus.textContent = game.storage + " · SELECTED · " + game.serial;
  }

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
    renderLibrary();
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
    const shown = visibleGames();
    if ((state.screen === "games" || state.screen === "hdd") && shown.length) {
      const index = shown.findIndex(game => game.id === libraryState.selectedId);
      const next = shown[(index + delta + shown.length) % shown.length];
      libraryState.selectedId = next.id;
      renderLibrary();
      return;
    }
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
      const game = selectedGame();
      note.textContent = game ? "محاكاة اختيار: " + game.title + " — لا يتم تشغيل لعبة أو ملف ELF في المتصفح." : "لا توجد ألعاب مطابقة للعرض.";
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
    else if (key === "square") note.textContent = "زر □: صفحة معلومات اللعبة ليست ضمن اللقطات المرجعية الحالية.";
    else if (key === "start") setScreen("menu",true);
  }

  if (gameSearch) gameSearch.addEventListener("input",() => {
    libraryState.query = gameSearch.value;
    renderLibrary();
  });
  if (genreFilter) genreFilter.addEventListener("change",() => {
    libraryState.genre = genreFilter.value;
    renderLibrary();
  });
  if (sortGames) sortGames.addEventListener("change",() => {
    libraryState.sort = sortGames.value;
    renderLibrary();
  });
  storageButtons.forEach(button => button.addEventListener("click",() => {
    libraryState.storage = button.dataset.storage;
    renderLibrary();
  }));
  viewButtons.forEach(button => button.addEventListener("click",() => {
    libraryState.view = button.dataset.libraryView;
    renderLibrary();
    if (libraryState.view === "opl") note.textContent = "عرض OPL: اختر الغلاف، ثم استخدم × للمحاكاة أو △ لخيارات اللعبة.";
  }));
  favoritesOnlyButton.addEventListener("click",() => {
    libraryState.favoritesOnly = !libraryState.favoritesOnly;
    renderLibrary();
  });

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