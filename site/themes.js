// TEa.G3 theme maker: a theme folder (theme.ini + fondo.jpg, docs/themes.md) made in the page. No libraries, nothing
// leaves the browser. The preview is preview.js: the home, the splash and the launch screen of the base, animated.
(() => {
  // the palettes of themes.c's table, in theme.ini's key order
  const KEYS = [
    ['fondo', 'Ground', 'under the picture and where nothing is drawn; the base’s own background keeps its colours'],
    ['panel', 'Panel', 'panels, the detail card, the pane behind the list'],
    ['fila', 'Row', 'the selected row; the frame of options that are off'],
    ['borde', 'Edge', '1-px edges and rules'],
    ['tenue', 'Dim', 'what is off: unchecked options'],
    ['texto', 'Text', 'titles and main text'],
    ['texto2', 'Text 2', 'secondary text'],
    ['etiqueta', 'Label', 'serials, counters, the status line'],
    ['acento', 'Accent', 'the selection'],
    ['acero', 'Steel', 'quiet fills: options that are on, bar tracks'],
    ['inverso', 'Inverse', 'text on an accent or text fill'],
  ];
  const BASES = {
    torres: ['Torres', '01030F 050C2E 16296B 4962A5 3A4A80 E8EEFF C3D0F5 A6B8EA 8FD8FF 13235A 031236'],
    nuld: ['Classic', '06070A 0C0E12 161A21 2A303A 4A515C E6E8EC A9B0BA 8A919C C8FF2E 1A1D23 06070A'],
    cristal: ['Cristal', '0B0A14 15122A 2E2A48 6A6880 4A4860 FFFFFF EFECF6 DAD5E8 FFB454 26233A 15101F'],
    aurora: ['Aurora', '03050F 080C1A 18233F 3A4058 39405C EEF2FF CBD2EC AEB7D6 7CF5D6 121A30 070A14'],
    liquid: ['Liquid', '050E24 1A1440 3A3470 9A94C0 6A6590 FFFFFF F0EDFA D6D6F0 7FF0E0 3A3560 0B1F45'],
    onda: ['Onda', '101622 0A0E18 2A3244 8A90A0 5A6070 FFFFFF EEF1F8 D5DBEA FFFFFF 303848 101622'],
    orbit: ['Orbit', '04060E 0D1736 1B3050 3C6A8C 3A4670 EEF3FF C4CFEA A9B6D3 7FE7FF 16224A 04060E'],
    techstep: ['Techstep', '0A0A0A 0A0A0A 242420 F4F4F0 6A6A62 F4F4F0 F4F4F0 B8B8B0 FFD60A 2A2A26 0A0A0A'],
    wireframe: ['Wireframe', '00030A 00080E 0A2A34 1C7384 2A5560 E8FBFF B6E6F0 8CC7D4 39E6FF 0A2630 00090C'],
    neuro: ['Neuro', '0B0A0A 131211 1D1A18 34302C 4A4540 F2EFEA C2BCB3 9A948B FF6224 24201D 0A0A0A'],
    fosforo: ['Fósforo', '010703 021408 0E3A1C 2A8046 1E5A30 6BFF96 4FE07C 3DB965 6BFF96 0A2A14 021006'],
    papel: ['Papel', 'F1EEE7 FBFAF6 FFFFFF CFC9BD B0AA9E 1B1A17 45423B 666158 B4442C E4DFD4 F4F1EA'],
  };
  const OCASO = '0E0714 1A0F22 3A2140 6A4A66 5A4458 FFF4E8 E8D2C4 C9A9A0 FF8A3C 2A1830 0E0714';
  const $ = id => document.getElementById(id);
  const state = { base: 'nuld', view: 'carousel', pal: {}, picture: null, coverAccent: false };
  const setPal = s => s.split(' ').forEach((v, i) => { state.pal[KEYS[i][0]] = v; });

  // ---- the form ----
  const baseSel = $('base');
  for (const k in BASES) baseSel.add(new Option(BASES[k][0], k, k === 'nuld', k === 'nuld'));
  const rows = {};
  for (const [key, label, what] of KEYS) {
    const row = document.createElement('div');
    row.className = 'colour';
    const pick = document.createElement('input');
    pick.type = 'color';
    pick.setAttribute('aria-label', label);
    const hex = document.createElement('input');
    hex.type = 'text';
    hex.maxLength = 7;
    hex.spellcheck = false;
    hex.setAttribute('aria-label', label + ', hex');
    const text = document.createElement('span');
    const title = document.createElement('b');
    title.textContent = label;
    const code = document.createElement('code');
    code.textContent = key;
    const detail = document.createElement('small');
    detail.textContent = what;
    text.append(title, ' ', code, detail);
    row.append(pick, hex, text);
    pick.oninput = () => { state.pal[key] = pick.value.slice(1).toUpperCase(); sync(); };
    hex.onchange = () => {
      const m = /^#?([0-9a-f]{6})$/i.exec(hex.value.trim());
      if (m) state.pal[key] = m[1].toUpperCase();
      sync();
    };
    rows[key] = [pick, hex];
    $('colours').append(row);
  }
  baseSel.onchange = () => { state.base = baseSel.value; setPal(BASES[state.base][1]); sync(); }; // a base brings its colours
  $('reset').onclick = () => { setPal(BASES[state.base][1]); sync(); };
  $('name').oninput = () => sync();
  $('cover-accent').onchange = e => { state.coverAccent = e.target.checked; sync(); };
  $('picture').onchange = async e => {
    const f = e.target.files[0];
    if (!f) return;
    try {
      const bmp = await createImageBitmap(f);
      const c = document.createElement('canvas');
      c.width = 1280, c.height = 720;
      const k = Math.max(1280 / bmp.width, 720 / bmp.height), w = bmp.width * k, h = bmp.height * k; // cover the screen, centred
      c.getContext('2d').drawImage(bmp, (1280 - w) / 2, (720 - h) / 2, w, h);
      state.picture = c;
    } catch { alert(document.documentElement.lang === 'ar' ? 'تعذرت قراءة الملف على أنه صورة.' : 'That file could not be read as a picture.'); }
    e.target.value = '';
    sync();
  };
  $('no-picture').onclick = () => { state.picture = null; sync(); };
  for (const tab of document.querySelectorAll('.tabs [data-view]')) tab.onclick = () => {
    state.view = tab.dataset.view;
    document.querySelectorAll('.tabs [data-view]').forEach(t => t.setAttribute('aria-selected', t === tab));
    draw();
  };

  // ---- the folder ----
  const BUILTIN = Object.keys(BASES);
  function folder() { // the folder's name: what config.ini stores. Plain letters and digits, 31 at most, not a built-in theme's
    let s = $('name').value.normalize('NFD').replace(/[̀-ͯ]/g, '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '').slice(0, 24);
    if (!s) s = 'my-theme';
    return BUILTIN.includes(s) ? s + '-2' : s;
  }
  function ini() {
    const name = $('name').value.replace(/[;#\[\]=\r\n]/g, '').trim() || folder(); // ; and # would start a comment
    const note = document.documentElement.lang === 'ar' ? `; ${name}: سمة لمشغّل TEa.G3 (الإصدار 1.3 أو أحدث). انسخ هذا المجلد إلى tea_g3/themes/ في وحدة USB.\n` : `; ${name}: a theme for TEa.G3 (v1.3 or newer). Copy this folder to tea_g3/themes/ on the USB drive.\n`;
    let t = note +
      `nombre = ${name}\nbase = ${state.base}\n\n[colores]\n`;
    for (const [key] of KEYS) t += `${key} = ${key === 'acento' && state.coverAccent ? 'portada' : state.pal[key]}\n`;
    return t;
  }
  // a zip with its files stored, not compressed (the JPG already is)
  const CRC = Array.from({ length: 256 }, (_, n) => { for (let k = 0; k < 8; k++) n = n & 1 ? 0xEDB88320 ^ (n >>> 1) : n >>> 1; return n >>> 0; });
  const crc32 = b => { let c = ~0; for (const x of b) c = CRC[(c ^ x) & 255] ^ (c >>> 8); return ~c >>> 0; };
  function zip(files) { // [[name, Uint8Array]]
    const enc = new TextEncoder(), parts = [], dir = [];
    let at = 0;
    const now = new Date(), date = (now.getFullYear() - 1980) << 9 | (now.getMonth() + 1) << 5 | now.getDate();
    const rec = (sig, extra, name, crc, n, tail) => {
      const h = new DataView(new ArrayBuffer(extra + 26 + tail));
      h.setUint32(0, sig, true);
      let o = 4;
      if (extra > 4) h.setUint16(o, 20, true), o += 2; // version made by (central directory only)
      h.setUint16(o, 20, true), h.setUint16(o + 2, 0x800, true); // version needed; names in UTF-8
      h.setUint16(o + 8, date, true);
      h.setUint32(o + 10, crc, true), h.setUint32(o + 14, n, true), h.setUint32(o + 18, n, true), h.setUint16(o + 22, name.length, true);
      return h;
    };
    for (const [path, data] of files) {
      const name = enc.encode(path), crc = crc32(data);
      const local = rec(0x04034b50, 4, name, crc, data.length, 0);
      const central = rec(0x02014b50, 6, name, crc, data.length, 14);
      central.setUint32(42, at, true); // where its local header is
      parts.push(local.buffer, name, data);
      dir.push(central.buffer, name);
      at += 30 + name.length + data.length;
    }
    const size = dir.reduce((n, p) => n + p.byteLength, 0), end = new DataView(new ArrayBuffer(22));
    end.setUint32(0, 0x06054b50, true), end.setUint16(8, files.length, true), end.setUint16(10, files.length, true);
    end.setUint32(12, size, true), end.setUint32(16, at, true);
    return new Blob([...parts, ...dir, end.buffer], { type: 'application/zip' });
  }
  $('download').onclick = async () => {
    const dir = folder(), files = [[`${dir}/theme.ini`, new TextEncoder().encode(ini())]];
    if (state.picture) {
      const jpg = await new Promise(r => state.picture.toBlob(r, 'image/jpeg', 0.9));
      files.push([`${dir}/fondo.jpg`, new Uint8Array(await jpg.arrayBuffer())]);
    }
    const a = document.createElement('a');
    a.href = URL.createObjectURL(zip(files)), a.download = dir + '.zip';
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
  };
  $('copy').onclick = async () => {
    await navigator.clipboard.writeText(ini());
    $('copy').textContent = 'Copied';
    setTimeout(() => { $('copy').textContent = 'Copy theme.ini'; }, 1500);
  };

  // ---- what would be hard to read on a TV ----
  const lum = h => [0, 2, 4].map(i => parseInt(h.substr(i, 2), 16) / 255).map(c => c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4)
    .reduce((s, c, i) => s + c * [0.2126, 0.7152, 0.0722][i], 0);
  const contrast = (a, b) => { const x = lum(a) + 0.05, y = lum(b) + 0.05; return x > y ? x / y : y / x; };
  function warnings() {
    const p = state.pal, w = [], ar = document.documentElement.lang === 'ar';
    const check = (a, b, min, en, arabic) => {
      const c = contrast(p[a], p[b]);
      if (c < min) w.push(ar
        ? `${a} مع ${b}: التباين ${c.toFixed(1)}، والمستوى المقترح ${min} أو أعلى. ${arabic}.`
        : `${a} on ${b} is hard to read (contrast ${c.toFixed(1)}, aim for ${min} or more): ${en}.`);
    };
    check('texto', 'panel', 4.5, 'the text of every panel', 'قد يصعب قراءة نصوص اللوحات');
    check('texto2', 'panel', 3, 'the list rows not selected', 'قد يصعب قراءة نصوص الصفوف غير المحددة');
    check('texto', 'fila', 4.5, 'the selected row', 'قد يصعب قراءة نص الصف المحدد');
    check('etiqueta', 'fondo', 3, 'serials and the status line', 'قد يصعب قراءة الأرقام التسلسلية وسطر الحالة');
    if (!state.coverAccent) check('inverso', 'acento', 4.5, 'the lit filter and options', 'قد يصعب قراءة النص داخل المرشحات والخيارات المميزة');
    check('inverso', 'texto', 4.5, 'the filled tag next to the serial', 'قد يصعب قراءة النص داخل الوسم المعبأ بجانب الرقم التسلسلي');
    if (state.picture) w.push(ar
      ? 'عند إضافة صورة خلفية، راجع وضوح العنوان والتوضيح في المعاينة؛ فاللوحة تظهر خلف القائمة فقط.'
      : 'With a picture, check the title and the legend against it in the preview: only the list gets a pane behind it.');
    const list = $('warnings');
    list.replaceChildren();
    for (const message of w) {
      const item = document.createElement('li');
      item.textContent = message;
      list.append(item);
    }
  }
  document.addEventListener('tea-g3:languagechange', () => { warnings(); draw(); });

  // ---- the preview: preview.js draws the home; the setup panel over it is drawn here, with the palette as it is ----
  const UI = '500 17px "Geeza Pro", "Noto Naskh Arabic", "Noto Sans Arabic", Tahoma, Arial, sans-serif', MONO = '500 14px "IBM Plex Mono", monospace';
  function setup(g) {
    const P = state.pal, ACC = state.coverAccent ? 'FFB454' : P.acento;
    const ar = document.documentElement.lang === 'ar';
    const rect = (x, y, w, h, c, a = 1) => { g.globalAlpha = a, g.fillStyle = '#' + c, g.fillRect(x, y, w, h), g.globalAlpha = 1; };
    const frame = (x, y, w, h, c) => { g.fillStyle = '#' + c; g.fillRect(x, y, w, 1), g.fillRect(x, y + h - 1, w, 1), g.fillRect(x, y, 1, h), g.fillRect(x + w - 1, y, 1, h); };
    const text = (font, x, y, s, c, align = 'left') => {
      const previousDirection = g.direction;
      g.direction = ar && /[\u0600-\u06FF]/.test(s) ? 'rtl' : 'ltr';
      g.font = font, g.fillStyle = '#' + c, g.textAlign = align, g.textBaseline = 'top', g.fillText(s, x, y);
      g.direction = previousDirection;
    };
    const width = s => { g.font = ar && /[\u0600-\u06FF]/.test(s) ? UI : MONO; g.direction = ar ? 'rtl' : 'ltr'; return Math.round(g.measureText(s).width); };
    g.setTransform(1, 0, 0, 1, 0, 0);
    rect(0, 0, 1280, 720, '000000', 0.25); // on the theme's row the home stays in sight
    rect(270, 98, 740, 524, P.panel), frame(270, 98, 740, 524, P.borde);
    text(UI, 320, 118, ar ? 'إعدادات TEa.G3' : 'TEa.G3 setup', P.texto), rect(294, 156, 692, 1, P.borde);
    const value = (y, s, cur) => {
      const w = width(s) + 56, x = 986 - w;
      if (cur) rect(x, y + 5, w, 30, ACC); else rect(x, y + 5, w, 30, P.panel), frame(x, y + 5, w, 30, P.borde);
      text(ar && /[\u0600-\u06FF]/.test(s) ? UI : MONO, x + w / 2, y + 12, `<  ${s}  >`, cur ? P.inverso : P.texto, 'center');
    };
    (ar ? ['اللغة', 'السمة', 'مصادر الألعاب', 'بطاقة الذاكرة', 'الفيديو', 'الشبكة'] : ['Language', 'Theme', 'Games from', 'Memory card', 'Video', 'Network']).forEach((l, r) => {
      const y = 166 + 46 * r;
      if (r === 1) rect(282, y, 716, 40, P.fila), rect(282, y, 3, 40, ACC);
      text(UI, 298, y + 9, l, r === 1 ? P.texto : P.texto2);
    });
    value(166, ar ? 'العربية' : 'English', 0), value(212, $('name').value || (ar ? 'سمتي' : 'My theme'), 1);
    let x = 986;
    for (const [s, on] of [['UDPFS', 0], ['MMCE', 0], ['MX4SIO', 0], ['HDD', 1], ['USB', 1]]) { // a row of checks
      const w = width(s) + 24;
      x -= w;
      if (on) rect(x, 263, w, 30, P.acero); else frame(x, 263, w, 30, P.fila);
      text(MONO, x + 12, 270, s, on ? P.texto : P.tenue), x -= 6;
    }
    value(304, ar ? 'افتراضية لكل لعبة' : 'Per-game virtual', 0), value(350, '480p', 0), value(396, 'DHCP', 0);
    text(UI, 640, 470, ar ? 'حفظ' : 'Save', ACC, 'center');
    rect(294, 520, 692, 6, P.acero), rect(294, 520, 240, 6, ACC); // a bar and its track
    text(ar ? UI : MONO, 294, 540, ar ? 'الأغلفة ٧ / ١٦' : 'COVERS 7 / 16', P.etiqueta);
    g.textAlign = 'left', g.textBaseline = 'alphabetic';
  }
  const VIEWS = { carousel: 0, grid: 1, list: 2, setup: 1 };
  function draw() {
    const def = {};
    BASES[state.base][1].split(' ').forEach((v, i) => { def[KEYS[i][0]] = v; });
    TEaG3Preview.set(state.base, state.pal, def, state.picture, state.coverAccent);
    TEaG3Preview.view(VIEWS[state.view]);
    TEaG3Preview.overlay(state.view === 'setup' ? setup : null);
  }
  $('see-splash').onclick = () => TEaG3Preview.show('splash');
  $('see-launch').onclick = () => TEaG3Preview.show('launch');
  function sync() {
    for (const [key] of KEYS) rows[key][0].value = '#' + state.pal[key], rows[key][1].value = state.pal[key];
    rows.acento.forEach(i => { i.disabled = state.coverAccent; });
    $('no-picture').hidden = !state.picture;
    $('ini').textContent = ini();
    const credit = $('maker-credit');
    if (credit) {
      credit.style.color = '#' + state.pal.texto;
      credit.style.backgroundColor = '#' + state.pal.panel;
      credit.style.borderColor = '#' + state.pal.acento;
    }
    warnings();
    draw();
  }
  setPal(OCASO);
  const from = new URLSearchParams(location.search).get('base'); // themes.html?base=torres starts from that theme
  if (BASES[from]) state.base = baseSel.value = from, $('name').value = document.documentElement.lang === 'ar' ? 'سمتي ' + ({torres:'الأبراج',nuld:'الكلاسيكية',cristal:'كريستال',aurora:'الشفق القطبي',liquid:'السائل',onda:'الموجة',orbit:'المدار',techstep:'تيك ستيب',wireframe:'الإطار السلكي',neuro:'نيورو',fosforo:'الفوسفور',papel:'الورق'}[from] || BASES[from][0]) : 'My ' + BASES[from][0], setPal(BASES[from][1]);
  const first = document.querySelector(`.tabs [data-view="${location.hash.slice(1)}"]`); // themes.html#list opens on that screen
  if (first) first.click();
  sync();
})();
