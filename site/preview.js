// TEa.G3 home screen on a canvas, for the theme maker (themes.html): the twelve themes' scenes, selections, splashes and
// launch screens as the prototype "TEa.G3 live themes" drew them (v1.3's design; the console's are themes.c and
// torres.c), with real covers. themes.js drives it through window.TEaG3Preview.
(() => {
const W = 1280, H = 720, TAU = Math.PI * 2;
const cv = document.getElementById('preview'), g = cv.getContext('2d');
const mk = (w, h) => { const c = document.createElement('canvas'); c.width = w; c.height = h; return c; };
const clamp = (v, a = 0, b = 1) => v < a ? a : v > b ? b : v, lerp = (a, b, k) => a + (b - a) * k;
const ease = k => { k = clamp(k); return k * k * (3 - 2 * k); };
const rgb = (c, a = 1) => `rgba(${c[0] | 0},${c[1] | 0},${c[2] | 0},${a})`;
const hash = (i, j) => { const n = Math.sin(i * 127.1 + j * 311.7) * 43758.5453; return n - Math.floor(n); };
const saw = x => 2 * (x - Math.floor(x + .5));
const FT = '700 38px Archivo, sans-serif',
  FU = '500 17px "Geeza Pro", "Noto Naskh Arabic", "Noto Sans Arabic", Tahoma, Archivo, sans-serif',
  FM = '400 14px "IBM Plex Mono", monospace';
const font = (f, wide) => { g.font = f; if ('fontStretch' in g) g.fontStretch = wide ? 'expanded' : 'normal'; };
const Q = new URLSearchParams(location.search), T0 = +(Q.get('t') || 0); // local testing: start the clock later, in a given view

// ---- library: real covers, baked like the launcher bakes them (256x368, 184x264, 128x184; RGB555, Floyd-Steinberg) ----
const img = src => { const i = new Image(); i.src = src; return i; };
const G = [['Gran Turismo 4', 'SCUS-97328', 3, 0], ['Tekken 5', 'SLUS-21059', 2, 0], ['Black', 'SLUS-21376', 2, 56],
  ['Shadow of the Colossus', 'SCUS-97472', 1, 34], ['Burnout 3: Takedown', 'SLUS-21050', 4, 0], ['Okami', 'SLUS-21115', 1, 48],
  ['Rez', 'SLUS-20344', 1, 0], ['God of War', 'SCUS-97399', 2, 71], ['Metal Gear Solid 3: Snake Eater', 'SLUS-20915', 5, 0],
  ['Final Fantasy X', 'SLUS-20312', 6, 62], ['Katamari Damacy', 'SLUS-21008', 1, 0], ['Grand Theft Auto: San Andreas', 'SLUS-20946', 3, 0],
  ['Silent Hill 2', 'SLUS-20228', 2, 40], ['Kingdom Hearts', 'SLUS-20370', 4, 0], ['Need for Speed: Underground 2', 'SLUS-21065', 2, 0],
  ['SSX Tricky', 'SLUS-20326', 1, 0]].map(([t, f, n, ach]) => ({ t, n, ach, s: f,
  im: (i => [i, i, i])(img('img/covers/' + f + '.jpg')), mono: [], acc: [200, 255, 46], accD: [90, 110, 20], blur: null, lum: null }));
const N = G.length;

// per cover, once: a blurred copy (tiny, then enlarged: what the console would do), its accent colour, its luminance
function derive(o) {
  const c = mk(24, 34), x = c.getContext('2d');
  x.imageSmoothingQuality = 'high';
  x.drawImage(o.im[2], 0, 0, 24, 34);
  const m = mk(192, 276), mx = m.getContext('2d');
  mx.imageSmoothingQuality = 'high';
  mx.drawImage(c, 0, 0, 192, 276);
  o.blur = m;
  try {
    const d = x.getImageData(0, 0, 24, 34).data;
    let r = 0, gg = 0, b = 0, w = 0;
    o.lum = new Float32Array(24 * 34);
    for (let i = 0; i < d.length; i += 4) {
      const R = d[i], Gc = d[i + 1], B = d[i + 2], hi = Math.max(R, Gc, B), lo = Math.min(R, Gc, B);
      const s = hi ? (hi - lo) / hi : 0, k = s * s * hi / 255 + 1e-4;
      r += R * k; gg += Gc * k; b += B * k; w += k;
      o.lum[i >> 2] = (R * .3 + Gc * .59 + B * .11) / 255;
    }
    r /= w; gg /= w; b /= w; // the dominant saturated colour, pushed to a light, vivid tone
    const hi = Math.max(r, gg, b, 1), lo = Math.min(r, gg, b), sat = (hi - lo) / hi;
    const boost = c0 => clamp((hi - (hi - c0) * (sat < .5 ? Math.min(.5 / (sat + .01), 3) : 1)) * 235 / hi, 0, 255);
    const out = [boost(r), boost(gg), boost(b)];
    o.acc = out.map(v => v + (255 - v) * .12);
    o.accD = out.map(v => v * .5); // the same hue, dark: for light themes
  } catch (e) { /* a tainted canvas (file://): keep the default accent */ }
}
function mono(o, k) { // a green-phosphor copy of a cover
  if (!o.mono[k]) {
    const im = o.im[k], c = mk(im.naturalWidth, im.naturalHeight), x = c.getContext('2d');
    x.drawImage(im, 0, 0); x.globalCompositeOperation = 'saturation'; x.fillStyle = '#000'; x.fillRect(0, 0, c.width, c.height);
    x.globalCompositeOperation = 'multiply'; x.fillStyle = '#3BE873'; x.fillRect(0, 0, c.width, c.height);
    o.mono[k] = c;
  }
  return o.mono[k];
}

// ---- state: one box per cover that eases to its place in the current view, as in the launcher ----
const VNAME = ['CAROUSEL', 'GRID', 'LIST'];
const isArabic = () => document.documentElement.lang === 'ar';
const viewName = i => isArabic() ? ['دائري', 'شبكة', 'قائمة'][i] : VNAME[i];
let now = 0, sel = 2, prev = 2, dir = 1, pos = 2, view = 0, gtop = 0, ltop = 0, lscroll = 0, pillY = 166 + 2 * 52, lpres = 0;
let tMove = 0, tTheme = 0, tView = -9, tAnim = 0, lastInput = -9, autoDir = 1, autoN = 0, T = null, A = [200, 255, 46], Afrom = A, sunX = 640, tint = [30, 90, 200];
const B = G.map(() => ({ x: 640, y: 376, w: 0, h: 0, a: 0, f: 0 })), F = G.map(() => ({}));
const auto = { checked: true };
if (matchMedia('(prefers-reduced-motion: reduce)').matches) auto.checked = false;
const edgeFade = (y, h, top, bottom, soft) => { let a = 1; if (y < top) a = 1 - (top - y) / soft; if (y + h > bottom) a = Math.min(a, 1 - (y + h - bottom) / soft); return clamp(a); };
function place(i) { // where cover i belongs in the current view
  if (view === 0) { const di = i - sel, w = di ? 184 : 256, h = di ? 264 : 368; return { x: 640 + di * 212 + (di < 0 ? -36 : di > 0 ? 36 : 0) - w / 2, y: 376 - h / 2, w, h, a: di ? .82 : 1, f: di ? 0 : 1 }; }
  if (view === 1) { // 7 columns of 128x184 tiles, two rows on screen, the focused one 12 % bigger
    const r = Math.floor(i / 7) - gtop, fo = i === sel, sc = fo ? 1.12 : 1, ty = 176 + r * 220, w = 128 * sc, h = 184 * sc;
    return { x: 72 + i % 7 * 168 + (128 - w) / 2, y: ty + (184 - h) / 2, w, h, a: (fo ? 1 : .82) * edgeFade(ty, 184, 168, 588, 60), f: fo ? 1 : 0 };
  }
  if (i === sel) return { x: 880, y: 182, w: 256, h: 368, a: 1, f: 1 }; // list: the selected cover on the right
  return { x: 64, y: 166 + (i - ltop) * 52, w: 36, h: 52, a: 0, f: 0 }; // the others fold into their rows
}
function settle() { // scroll positions follow the selection; boxes restart their move
  const r = Math.floor(sel / 7); if (r < gtop) gtop = r; if (r > gtop + 1) gtop = r - 1;
  if (sel < ltop) ltop = sel; if (sel > ltop + 7) ltop = sel - 7;
  B.forEach((b, i) => Object.assign(F[i], b)); tAnim = now;
}
function move(d) {
  const n = clamp(sel + d, 0, N - 1); if (n === sel) return false;
  prev = sel; sel = n; dir = Math.sign(d); tMove = now; Afrom = A; settle(); return true;
}
function setView(v) { view = (v + 3) % 3; tView = now; settle(); }
function animate(t, dt) {
  const k = 1 - Math.exp(-dt * 10), st = Math.ceil(clamp((t - tAnim) / .16) * 4) / 4;
  B.forEach((b, i) => { const tg = place(i); for (const p of ['x', 'y', 'w', 'h', 'a', 'f']) b[p] = T.step ? lerp(F[i][p], tg[p], st) : (Math.abs(tg[p] - b[p]) < .004 ? tg[p] : b[p] + (tg[p] - b[p]) * k); });
  pos += (sel - pos) * k; lscroll += (ltop - lscroll) * k; pillY += (166 + (sel - ltop) * 52 - pillY) * (T.step ? 1 : Math.min(1, k * 1.6));
  lpres += ((view === 2 ? 1 : 0) - lpres) * Math.min(1, dt * 7);
  sunX += (B[sel].x + B[sel].w / 2 - sunX) * Math.min(1, dt * 2.2);
}

// ---- drawing helpers ----
function rr(x, y, w, h, r) {
  g.beginPath();
  if (!r) { g.rect(x, y, w, h); return; }
  g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r);
  g.closePath();
}
const LA = mk(160, 90), LB = mk(40, 23), la = LA.getContext('2d'), lb = LB.getContext('2d');
function snap() { // what is on screen now, small and soft: the glass panels show it
  lb.imageSmoothingQuality = la.imageSmoothingQuality = 'high';
  lb.drawImage(cv, 0, 0, 40, 23); la.drawImage(LB, 0, 0, 160, 90);
}
function panel(x, y, w, h, r, tintc, edge) {
  rr(x, y, w, h, r);
  if (T.glass) {
    g.save(); g.clip(); g.imageSmoothingEnabled = true; g.drawImage(LA, 0, 0, W, H);
    g.fillStyle = tintc || T.pn; g.fillRect(x, y, w, h);
    const hl = g.createLinearGradient(0, y, 0, y + h); hl.addColorStop(0, 'rgba(255,255,255,.14)'); hl.addColorStop(.5, 'rgba(255,255,255,0)');
    g.fillStyle = hl; g.fillRect(x, y, w, h); g.restore();
  } else { g.fillStyle = tintc || T.pn; g.fill(); }
  rr(x + .5, y + .5, w - 1, h - 1, r); g.strokeStyle = edge || T.ed; g.lineWidth = 1; g.stroke();
}
function backdrop(al, t) { // the selected cover, blurred, filling the screen; cross-fades when the selection moves
  const k = ease((t - tMove) / .5), w = W * 1.25, h = w * 276 / 192, x = (W - w) / 2 + Math.sin(t * .07) * 50, y = (H - h) / 2 + Math.cos(t * .05) * 70;
  g.imageSmoothingEnabled = true; g.imageSmoothingQuality = 'high';
  const a = G[prev], b = G[sel];
  if (a.blur && k < 1) { g.globalAlpha = al; g.drawImage(a.blur, x, y, w, h); }
  if (b.blur) { g.globalAlpha = al * (a.blur ? k : 1); g.drawImage(b.blur, x, y, w, h); }
  g.globalAlpha = 1;
}
const bd = (al, t) => { if (T.back) backdrop(al, t); };
function fit(s, max) { if (g.measureText(s).width <= max) return s; while (s.length > 1 && g.measureText(s + '…').width > max) s = s.slice(0, -1); return s.trimEnd() + '…'; }
function glyph(kind, x, y, c) { // the pad's symbols, 14 px
  g.strokeStyle = c; g.lineWidth = 2; g.beginPath();
  if (kind === 'x') { g.moveTo(x + 2, y + 2); g.lineTo(x + 12, y + 12); g.moveTo(x + 12, y + 2); g.lineTo(x + 2, y + 12); }
  else if (kind === 'o') g.arc(x + 7, y + 7, 5.5, 0, TAU);
  else if (kind === 't') { g.moveTo(x + 7, y + 2); g.lineTo(x + 13, y + 12); g.lineTo(x + 1, y + 12); g.closePath(); }
  else g.rect(x + 2, y + 2, 10, 10);
  g.stroke();
}
const vignette = a => { const v = g.createRadialGradient(640, 360, 300, 640, 360, 800); v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, `rgba(0,0,0,${a})`); g.fillStyle = v; g.fillRect(0, 0, W, H); };
// two-step at 174 BPM: kick on beats 0 and 2.5, snare on 1 and 3; 1 at the hit, falling off
const kick = t => { const b = (t * 174 / 60) % 4; return Math.max(Math.exp(-b * 5), Math.exp(-((b + 1.5) % 4) * 5), .6 * Math.exp(-((b + 3) % 4) * 7), .6 * Math.exp(-((b + 1) % 4) * 7)); };

// ---- covers ----
function cover(o, b, t, on) {
  let { x, y, w, h } = b;
  if (b.a < .01 || w < 8 || x > W + 60 || x + w < -60) return;
  if (T.bob && view === 0) y += Math.sin(t * 1.1 + G.indexOf(o) * 1.7) * T.bob;
  const rest = [128, 184, 256].some(s => Math.abs(w - s) < .6), exact = rest && !(T.bob && view === 0);
  if (exact) { x = Math.round(x); y = Math.round(y); }
  if (rest) { w = Math.round(w); h = Math.round(h); }
  const k = w > 185 ? 2 : w > 129 ? 1 : 0, raw = o.im[k], ok = raw.complete && raw.naturalWidth;
  const im = ok && T.mono && b.f < .5 ? mono(o, k) : raw, iw = raw.naturalWidth, ih = raw.naturalHeight, al = b.a * (T.dimmer && !on ? T.dimmer : 1);
  if (T.shadow) { g.save(); g.globalAlpha = b.a; g.shadowColor = T.shadow; g.shadowBlur = 34; g.shadowOffsetY = 16; g.fillStyle = '#000'; rr(x + 6, y + 6, w - 12, h - 12, T.cr); g.fill(); g.restore(); }
  g.imageSmoothingEnabled = !exact;
  if (T.reflect && ok && (view === 0 || on)) { // on the floor or the water: the cover upside down, fading out in thin strips
    const RH = 76;
    for (let j = 0; j < RH; j += 2) {
      g.globalAlpha = T.reflect * al * Math.pow(1 - j / RH, 1.6);
      const ox = T.ripple ? Math.sin(j * .33 + t * 2.6 + x * .02) * (1 + j * .07) : 0;
      g.drawImage(im, 0, ih * (1 - (j + 2) / h), iw, ih * 2 / h, x + ox, y + h + 4 + j, w, 2);
    }
  }
  g.globalAlpha = al;
  const cr = T.cr ? Math.max(3, T.cr * w / 256) : 0;
  if (cr) { g.save(); rr(x, y, w, h, cr); g.clip(); }
  if (ok) g.drawImage(im, x, y, w, h); else { g.fillStyle = '#12151B'; g.fillRect(x, y, w, h); }
  if (cr) g.restore();
  const lw = T.cw || 1;
  rr(x + lw / 2, y + lw / 2, w - lw, h - lw, cr); g.strokeStyle = T.cedge; g.lineWidth = lw; g.stroke();
  g.globalAlpha = 1;
}
function covers(t) {
  for (let i = 0; i < N; i++) if (i !== sel) cover(G[i], B[i], t, false);
  const s = { ...B[sel], sc: B[sel].w / 256 };
  if (T.bob && view === 0) s.y += Math.sin(t * 1.1 + sel * 1.7) * T.bob;
  if (T.selBack) T.selBack(s, t);
  cover(G[sel], B[sel], t, true);
  if (T.selFront) T.selFront(s, t);
}
function listRows(t) { // the list view's text side: 8 rows of 52 px, the cursor row behind the selected one
  if (lpres < .01) return;
  const acc = rgb(A), py = Math.round(pillY);
  g.globalAlpha = ease(lpres);
  if (T.lp) panel(46, 150, 692, 448, T.R);
  if (T.glass) panel(64, py, 656, 46, T.r ? 10 : 0, T.rowTint || 'rgba(255,255,255,.14)');
  else { g.fillStyle = T.row || '#161A21'; g.fillRect(64, py, 656, 46); }
  g.fillStyle = acc; g.fillRect(64, py + (T.r ? 8 : 0), 3, 46 - (T.r ? 16 : 0));
  for (let p = Math.max(0, Math.floor(lscroll) - 1); p < N && p <= Math.floor(lscroll) + 9; p++) {
    const y = 166 + (p - lscroll) * 52, row = clamp(lpres * 1.6 - (p - Math.floor(lscroll)) * .06), a = ease(row) * edgeFade(y, 46, 162, 586, 26);
    if (a < .01) continue;
    const x = 64 - 40 * (1 - ease(row)), cy = Math.round(y) + 23, on = p === sel, o = G[p];
    g.globalAlpha = a;
    g.strokeStyle = on ? acc : (T.blb || T.lb); g.lineWidth = 1.5; g.beginPath(); g.arc(x + 29, cy, 8, 0, TAU); g.moveTo(x + 31.5, cy); g.arc(x + 29, cy, 2.5, 0, TAU); g.stroke();
    font(FU); g.fillStyle = on ? (T.rowTx || T.tx) : (T.btx || T.t2); g.fillText(fit(o.t, 466), x + 52, cy + 6);
    font(FM); g.fillStyle = on ? (T.rowTx || T.t2) : (T.blb || T.lb); g.fillText(o.s, x + 636 - g.measureText(o.s).width, cy + 5);
  }
  g.globalAlpha = 1;
}

// ---- header and footer: the same layout in every theme, drawn with the theme's tokens ----
function title(o, t) {
  const k = clamp((t - tMove) / .3), old = G[prev];
  font(FT, true);
  const draw = (s, x, a) => {
    if (a <= 0) return 0;
    s = fit(s, 740);
    if (T.ghost) { g.globalAlpha = a * .8; g.fillStyle = T.ghost; g.fillText(s, x + 3, 71); }
    g.globalAlpha = a;
    if (T.chrome) { const c = g.createLinearGradient(0, 40, 0, 74); T.chrome.forEach((v, i) => c.addColorStop([0, .5, .56, 1][i], v)); g.fillStyle = c; } else g.fillStyle = T.tx;
    if (T.glow) { g.shadowColor = T.glow; g.shadowBlur = 16; }
    g.fillText(s, x, 70);
    g.shadowBlur = 0; g.globalAlpha = 1;
    return g.measureText(s).width;
  };
  if (T.wipe) { // hard cut, revealed in four steps
    const w = g.measureText(fit(o.t, 740)).width;
    g.save(); g.beginPath(); g.rect(140, 30, Math.ceil(k * 4) / 4 * (w + 8), 50); g.clip(); draw(o.t, 140, 1); g.restore();
  } else if (T.glitch && k < 1) { // the title arrives in sliced bands
    for (let b = 0; b < 6; b++) {
      g.save(); g.beginPath(); g.rect(0, 34 + b * 8, W, 8); g.clip();
      draw(o.t, 140 + Math.round((hash(b, Math.floor(t * 30)) - .5) * 70 * (1 - k)), .4 + .6 * k); g.restore();
    }
  } else if (T.type) { // typed out, with a block cursor
    const s = fit(o.t, 700), n = Math.ceil(clamp((t - tMove) / .5) * s.length), w = n ? draw(s.slice(0, n), 140, 1) : 0;
    if (Math.floor(t * 2.5) % 2 === 0 || n < s.length) { g.fillStyle = T.tx; g.fillRect(140 + w + 6, 44, 18, 28); }
  } else { draw(old.t, 140 - dir * 22 * ease(k), 1 - ease(k * 1.6)); draw(o.t, 140 + dir * 22 * (1 - ease(k)), ease(k)); }
}
function header(t) {
  const o = G[sel], acc = rgb(A);
  mark(92, 73, 24, T.tx, T.aTag && T.step ? T.aTag : acc);

  if (T.cardFill) { g.fillStyle = T.cardFill(); g.fillRect(932, 36, 284, 88); } else panel(932, 36, 284, 88, T.R);
  g.strokeStyle = T.cardTx || acc; g.lineWidth = 2; g.beginPath(); // the saves card
  g.rect(968, 62, 28, 36); g.moveTo(974, 62); g.lineTo(974, 72); g.lineTo(990, 72); g.lineTo(990, 62); g.moveTo(974, 88); g.lineTo(990, 88); g.stroke();
  font(FU); g.fillStyle = T.cardTx || T.tx; putText(uiText('Virtual memory card', 'بطاقة ذاكرة افتراضية'), 1028, 74);
  font(isArabic() ? FU : FM); g.fillStyle = T.cardTx || T.t2; putText(o.n + (isArabic() ? (o.n === 1 ? ' عملية حفظ · ' : ' عمليات حفظ · ') : (o.n === 1 ? ' SAVE · ' : ' SAVES · ')) + (o.n * 98 + 14) + ' KB', 1028, 98);

  title(o, t);
  const tags = [[o.s, 0], ['PS2', 1], ['USB', 2], ['480P', 0]];
  if (o.ach) tags.push([(isArabic() ? 'الإنجازات ' : 'ACHIEVEMENTS ') + o.ach, 2]);
  let x = 140;
  font(isArabic() && o.ach ? FU : FM);
  tags.forEach(([s, kind], i) => {
    const w = Math.round(g.measureText(s).width) + 24, ac = T.aTag || acc;
    g.globalAlpha = T.wipe || T.type ? (t - tMove > .06 * i ? 1 : 0) : ease((t - tMove - .04 * i) / .22);
    if (kind === 1) { rr(x, 86, w, 26, T.r); g.fillStyle = T.fBg ? T.fBg() : T.tx; g.fill(); g.fillStyle = T.fTx || T.inv; }
    else if (T.glass) { panel(x, 86, w, 26, T.r, T.tg, kind === 2 ? ac : 0); g.fillStyle = kind === 2 ? ac : T.t2; font(FM); }
    else { rr(x + .5, 86.5, w - 1, 25, T.r); if (T.tg) { g.fillStyle = T.tg; g.fill(); } g.strokeStyle = kind === 2 ? ac : (T.tagEd ? T.tagEd() : T.ed); g.lineWidth = 1; g.stroke(); g.fillStyle = kind === 2 ? ac : T.t2; }
    putText(s, x + 12, 104);
    x += w + 10;
  });
  g.globalAlpha = 1;
  if (T.rule) { g.fillStyle = T.ed; g.fillRect(64, 138, 1152, 1); }
}
function footer(t) {
  const o = G[sel], acc = rgb(A), y = T.fy || 655;
  if (T.dock) panel(44, 639, 1192, 54, 27);
  if (T.bar) panel(-1, 626, W + 2, 96, 0);
  font(isArabic() ? FU : FM);
  let x = 64;
  const count = String(sel + 1).padStart(2, '0') + ' / ' + N;
  g.fillStyle = T.lb; g.fillText(count, x, y + 16); x += Math.round(g.measureText(count).width) + 16;
  const filters = isArabic() ? ['L1', 'الكل', 'PS2', 'PS1', 'التطبيقات', 'R1'] : ['L1', 'ALL', 'PS2', 'PS1', 'APPS', 'R1'];
  filters.forEach((s, i) => {
    const key = i === 0 || i === 5, w = Math.round(g.measureText(s).width) + (key ? 12 : 18);
    if (key) { rr(x + .5, y + .5, w - 1, 21, T.kr || 0); g.strokeStyle = T.lb; g.lineWidth = 1; g.stroke(); }
    if (i === 1) { rr(x, y, w, 22, T.fr || 0); g.fillStyle = acc; g.fill(); }
    g.fillStyle = i === 1 ? T.oa : T.t2; putText(s, x + (key ? 6 : 9), y + 16);
    x += w + 6;
  });
  x = W - 64;
  const hints = [['s', isArabic() ? 'العرض' : 'View'], ['t', isArabic() ? 'الخيارات' : 'Options']]; if (o.ach) hints.push(['o', isArabic() ? 'الإنجازات' : 'Achievements']); hints.push(['x', isArabic() ? 'تشغيل' : 'Play']);
  font(FU);
  for (const [kd, s] of hints) {
    x -= Math.round(g.measureText(s).width); g.fillStyle = T.tx; putText(s, x, y + 17);
    x -= 22; glyph(kd, x, y + 4, acc); x -= 24;
  }
  const vl = t - tView; // the view's name, framed in the accent, about a second after a change
  if (vl < 1.15) {
    const va = vl < .3 ? ease(vl / .3) : ease((1.15 - vl) / .8), s = (isArabic() ? 'العرض  ' : 'VIEW  ') + viewName(view);
    font(isArabic() ? FU : FM); if ('letterSpacing' in g) g.letterSpacing = '3px';
    const w = Math.round(g.measureText(s).width) + 56, vx = Math.round((W - w) / 2), vy = 590 + Math.round(8 * (1 - va));
    g.globalAlpha = va; g.fillStyle = T.vp || T.pn; g.fillRect(vx, vy, w, 36); g.strokeStyle = acc; g.lineWidth = 1; g.strokeRect(vx + .5, vy + .5, w - 1, 35);
    g.fillStyle = T.vtx || T.tx; putText(s, vx + 28, vy + 23);
    if ('letterSpacing' in g) g.letterSpacing = '0px';
    g.globalAlpha = 1;
  }
}

// ---- sprites shared by the scenes ----
const ORB = mk(64, 64); { const c = ORB.getContext('2d'), r = c.createRadialGradient(32, 32, 0, 32, 32, 32);
  r.addColorStop(0, 'rgba(255,255,255,1)'); r.addColorStop(.18, 'rgba(190,230,255,.9)'); r.addColorStop(.45, 'rgba(90,150,255,.35)'); r.addColorStop(1, 'rgba(40,90,255,0)');
  c.fillStyle = r; c.fillRect(0, 0, 64, 64); }
const curtain = (r, gr, b) => { const s = mk(1, 256), c = s.getContext('2d'), l = c.createLinearGradient(0, 0, 0, 256);
  l.addColorStop(0, `rgba(${r},${gr},${b},0)`); l.addColorStop(.72, `rgba(${r},${gr},${b},.5)`); l.addColorStop(.93, `rgba(${Math.min(255, r + 90)},${Math.min(255, gr + 60)},${Math.min(255, b + 60)},.95)`); l.addColorStop(1, `rgba(${r},${gr},${b},0)`);
  c.fillStyle = l; c.fillRect(0, 0, 1, 256); return s; };
const CUR = [{ sp: curtain(30, 240, 170), y: 606, h: 470, a: .72 }, { sp: curtain(120, 80, 255), y: 566, h: 430, a: .62 }, { sp: curtain(255, 80, 160), y: 624, h: 360, a: .34 }];
const scan = a => { const s = mk(4, 3), c = s.getContext('2d'); c.fillStyle = `rgba(0,0,0,${a})`; c.fillRect(0, 2, 4, 1); return s; };
const SCAN = scan(.2), SCAN2 = scan(.42);
const GRAIN = mk(128, 128); { const c = GRAIN.getContext('2d'), d = c.createImageData(128, 128); for (let i = 0; i < d.data.length; i += 4) { d.data[i] = d.data[i + 1] = d.data[i + 2] = hash(i, 3) > .5 ? 0 : 255; d.data[i + 3] = hash(i, 9) * 14; } c.putImageData(d, 0, 0); }
const PIL = [], PART = [];
for (let i = -8; i <= 8; i++) for (let j = -8; j <= 8; j++) if (hash(i, j) > .4)
  PIL.push({ x: i, z: j, h: .25 + Math.pow(hash(j + 9, i - 3), 2) * 2.8, d: Math.hypot(i, j), ph: hash(i, j) * 9 });
for (let i = 0; i < 46; i++) PART.push({ x: (hash(i, 1) - .5) * 15, z: (hash(i, 2) - .5) * 15, y: hash(i, 3) * 5, v: .12 + hash(i, 4) * .25, ph: hash(i, 5) * 9 });
function hud(s, t, len, th) { // corner brackets that close in on the cover
  const k = ease((t - tMove) / .32), m = (12 + (1 - k) * 30) * s.sc, L = len * s.sc, x0 = Math.round(s.x - m), y0 = Math.round(s.y - m), x1 = Math.round(s.x + s.w + m), y1 = Math.round(s.y + s.h + m);
  g.globalAlpha = s.f * k; g.fillStyle = rgb(A);
  for (const [x, y, dx, dy] of [[x0, y0, 1, 1], [x1, y0, -1, 1], [x0, y1, 1, -1], [x1, y1, -1, -1]]) { g.fillRect(dx > 0 ? x : x - L, dy > 0 ? y : y - th, L, th); g.fillRect(dx > 0 ? x : x - th, dy > 0 ? y : y - L, th, L); }
  g.globalAlpha = 1;
}
function halo(s, t, a, r0, r1) {
  const cx = s.x + s.w / 2, cy = s.y + s.h / 2, R = r1 * s.sc, r = g.createRadialGradient(cx, cy, r0 * s.sc, cx, cy, R);
  r.addColorStop(0, rgb(A, a * s.f)); r.addColorStop(1, rgb(A, 0)); g.fillStyle = r; g.fillRect(cx - R, cy - R, 2 * R, 2 * R);
}
function ring(s, c, lw, pad, r) { g.globalAlpha = s.f; rr(s.x - pad, s.y - pad, s.w + 2 * pad, s.h + 2 * pad, r); g.strokeStyle = c; g.lineWidth = lw; g.stroke(); g.globalAlpha = 1; }

// ---- splash and loading screens: a theme may bring its own; the rest get a plain one made from their tokens ----
let mode = 'home', tMode = 0, tFade = 0, CAM = null;
const SPL = 6.4, LCH = 4.4, HOW_EN = 'STARTING WITH NEUTRINO';
const how = () => isArabic() ? 'جارٍ تشغيل NEUTRINO' : HOW_EN;
const STEPS_EN = [[0, 'STARTING'], [1.1, 'USB MODULES'], [2.2, 'READING GAMES  7 / 16'], [3.3, 'READING GAMES  16 / 16'], [4.4, '16 GAMES · READY']];
const STEPS_AR = [[0, 'جارٍ البدء'], [1.1, 'وحدات USB'], [2.2, 'جارٍ فحص الألعاب  ٧ / ١٦'], [3.3, 'جارٍ فحص الألعاب  ١٦ / ١٦'], [4.4, '١٦ لعبة · جاهز']];
const status = u => { const steps = isArabic() ? STEPS_AR : STEPS_EN; let s = steps[0][1]; for (const [a, m] of steps) if (u >= a) s = m; return s; };
const uiText = (en, ar) => isArabic() ? ar : en;
const putText = (s, x, y, align = 'left') => {
  const oldDirection = g.direction, oldAlign = g.textAlign, oldFont = g.font;
  const arabicRun = isArabic() && /[\u0600-\u06FF]/.test(s);
  if (arabicRun) g.font = FU;
  g.direction = arabicRun ? 'rtl' : 'ltr';
  g.textAlign = align; g.fillText(s, x, y);
  g.textAlign = oldAlign; g.direction = oldDirection; g.font = oldFont;
};
const ink = () => T.btx || T.tx;
function mark(cx, cy, r, ringc, slash) { // NULD's mark: a ring and a slash
  const th = r * .18, d = r * .985; g.lineCap = 'butt';
  g.strokeStyle = ringc; g.lineWidth = th; g.beginPath(); g.arc(cx, cy, r - th / 2, 0, TAU); g.stroke();
  g.strokeStyle = slash; g.beginPath(); g.moveTo(cx - d, cy + d); g.lineTo(cx + d, cy - d); g.stroke();
}
function wordmark(cx, cy, cap, inkc, slash, dx = 0) { // the mark and NULD in Michroma, centred on (cx, cy); cap: capital height
  font(`400 ${cap * 128 / 96}px Michroma, Archivo, sans-serif`); if ('letterSpacing' in g) g.letterSpacing = cap * .08 + 'px';
  const r = cap * .6875, ww = g.measureText('TEa.G3').width, x = cx - (2 * r + cap * .42 + ww) / 2 + dx;
  g.fillStyle = inkc; g.fillText('TEa.G3', x + 2 * r + cap * .42, cy + cap / 2);
  if ('letterSpacing' in g) g.letterSpacing = '0px';
  mark(x + r, cy, r, inkc, slash);
}
function centred(s, y, f, c, track) {
  const arabicRun = isArabic() && /[\u0600-\u06FF]/.test(s);
  font(arabicRun ? FU : f, !arabicRun && f === FT); if (track && 'letterSpacing' in g) g.letterSpacing = track + 'px';
  const oldDirection = g.direction; g.direction = arabicRun ? 'rtl' : 'ltr';
  g.fillStyle = c; const w = g.measureText(s).width; g.fillText(s, Math.round(640 - w / 2), y);
  g.direction = oldDirection;
  if ('letterSpacing' in g) g.letterSpacing = '0px'; return w;
}
function corners(x, y, w, h, len, c) {
  g.fillStyle = c;
  for (const [px, py, dx, dy] of [[x, y, 1, 1], [x + w, y, -1, 1], [x, y + h, 1, -1], [x + w, y + h, -1, -1]]) { g.fillRect(dx > 0 ? px : px - len, dy > 0 ? py : py - 2, len, 2); g.fillRect(dx > 0 ? px : px - 2, dy > 0 ? py : py - len, 2, len); }
}
function floor(s, c, amax) { // splash_floor: lines across coming towards you, depth lines to the vanishing point
  const ph = (s * .9) % 1;
  for (let i = 0; i < 16; i++) { const z = i + 1 - ph; if (z < .5) continue; const y = 432 + 288 / z, a = amax * (1 - (z - 1) / 9); if (a > .01 && y < H) { g.fillStyle = rgb(c, a); g.fillRect(0, Math.floor(y), W, 1); } }
  const l = g.createLinearGradient(0, 912, 0, 461); l.addColorStop(0, rgb(c, amax)); l.addColorStop(1, rgb(c, 0));
  g.strokeStyle = l; g.lineWidth = 1; g.beginPath(); for (let i = -14; i <= 14; i++) { g.moveTo(640 + 500 * i, 912); g.lineTo(640 + 30 * i, 460.8); } g.stroke();
}
function sphere(t, c) { // the splash's wire sphere: 9 meridians, 5 parallels, brighter towards you
  const R = 196, ct = Math.cos(-.28), st = Math.sin(-.28), rot = t * TAU / 28; g.lineWidth = 1;
  for (let m = 0; m < 14; m++) {
    let x0 = 0, y0 = 0, a0 = 0; const n = m < 9 ? 24 : 36;
    for (let i = 0; i <= n; i++) {
      const lat = m < 9 ? -Math.PI / 2 + Math.PI * i / n : (m - 11) * Math.PI / 6, lon = m < 9 ? m * Math.PI / 9 + rot : TAU * i / n + rot;
      const x = Math.cos(lat) * Math.cos(lon), y = Math.sin(lat), z = Math.cos(lat) * Math.sin(lon), sx = 640 + R * x, sy = 300 - R * (y * ct - z * st), a = (18 + (y * st + z * ct + 1) * 28) / 128;
      if (i) { g.strokeStyle = rgb(c, (a + a0) / 2); g.beginPath(); g.moveTo(x0, y0); g.lineTo(sx, sy); g.stroke(); }
      x0 = sx; y0 = sy; a0 = a;
    }
  }
}
function genSplash(u, t) { T.bg(t); if (T.glass) snap(); wordmark(640, 300, 80, ink(), rgb(A)); centred(status(u), 452, FM, T.blb || T.t2, 2); if (T.fx) T.fx(t); }
function genLaunch(u, t, o) {
  T.bg(t); if (T.glass) snap();
  const k = ease(u / .4);
  cover(o, { x: 512, y: 150 + 16 * (1 - k), w: 256, h: 368, a: k, f: 1 }, t, true);
  g.globalAlpha = k; font(FT, true); centred(fit(o.t, 1000), 574, FT, ink()); centred(HOW, 606, FM, T.blb || T.lb, 3); g.globalAlpha = 1;
  if (T.fx) T.fx(t);
}

// ---- the themes ----
const THEMES = {
  nuld: { name: 'Classic',
    tx: '#E6E8EC', t2: '#A9B0BA', lb: '#8A919C', ed: '#2A303A', pn: '#0C0E12', inv: '#06070A', oa: '#06070A', cedge: '#232833', acc: [200, 255, 46],
    r: 0, R: 0, cr: 0, rule: 1, glitch: 1, back: false,
    bg(t) {
      g.fillStyle = '#06070A'; g.fillRect(0, 0, W, H);
      bd(.2, t);
      floor(t, [230, 232, 236], .14); // the launcher's own floor (splash_floor)
    },
    selFront(s, t) { hud(s, t, 30, 2); },
    fx() { g.fillStyle = g.createPattern(SCAN, 'repeat'); g.fillRect(0, 0, W, H); } },

  cristal: { name: 'Cristal',
    tx: '#FFFFFF', t2: '#EFECF6', lb: '#DAD5E8', ed: 'rgba(255,255,255,.3)', pn: 'rgba(16,13,30,.36)', tg: 'rgba(16,13,30,.3)', inv: '#15101F', oa: '#120E08',
    cedge: 'rgba(255,255,255,.3)', acc: 'cover', glass: 1, r: 13, R: 16, cr: 10, fr: 11, kr: 6, dock: 1, lp: 1, reflect: .3, shadow: 'rgba(0,0,0,.6)', back: true, lock: 1, vp: 'rgba(12,10,24,.82)',
    bg(t) {
      g.fillStyle = '#0B0A14'; g.fillRect(0, 0, W, H);
      backdrop(1, t);
      g.fillStyle = 'rgba(9,8,18,.44)'; g.fillRect(0, 0, W, H);
      const l = g.createLinearGradient(0, 0, 0, H); l.addColorStop(0, 'rgba(6,5,14,.55)'); l.addColorStop(.3, 'rgba(6,5,14,0)'); l.addColorStop(.68, 'rgba(6,5,14,0)'); l.addColorStop(1, 'rgba(6,5,14,.7)');
      g.fillStyle = l; g.fillRect(0, 0, W, H);
    },
    selBack(s, t) { halo(s, t, .5 * (.75 + .25 * Math.sin(t * 2)), 100, 330); },
    selFront(s) { ring(s, rgb(A), 2, 3, 12 * s.sc); } },

  torres: { name: 'Torres',
    tx: '#E8EEFF', t2: '#C3D0F5', lb: '#A6B8EA', ed: 'rgba(150,185,255,.45)', pn: 'rgba(4,10,40,.6)', tg: 'rgba(4,10,40,.45)', inv: '#031236', oa: '#031236',
    cedge: 'rgba(170,200,255,.5)', acc: [143, 216, 255], r: 2, R: 3, cr: 0, fr: 2, rule: 1, lp: 1, row: 'rgba(60,110,255,.3)', glow: 'rgba(90,150,255,.9)', bob: 4, shadow: 'rgba(0,4,30,.8)', back: false,
    bg(t) {
      g.fillStyle = '#01030F'; g.fillRect(0, 0, W, H);
      let gr = g.createRadialGradient(640, 500, 0, 640, 500, 760); gr.addColorStop(0, '#1A3FA8'); gr.addColorStop(.4, '#0A1A5C'); gr.addColorStop(1, 'rgba(1,3,15,0)');
      g.fillStyle = gr; g.fillRect(0, 0, W, H);
      bd(.28, t);
      // the field of towers: turns slowly, and a little with every game you pass
      const C = CAM || { h: 4.2, d: 9, p: .42, yaw: 0, rise: t - tTheme }, a = t * .045 + pos * .1 + C.yaw, ca = Math.cos(a), sa = Math.sin(a), cp = Math.cos(C.p), sp = Math.sin(C.p), f = 900;
      const P = (x, y, z) => { const xr = x * ca - z * sa, zr = x * sa + z * ca + C.d, yr = y - C.h, zv = -yr * sp + zr * cp; return zv < .7 ? null : [640 + f * xr / zv, 300 - f * (yr * cp + zr * sp) / zv, zv]; };
      const rise = C.rise, L = [];
      for (const p of PIL) { const c = P(p.x, 0, p.z); if (c && c[0] > -160 && c[0] < W + 160) L.push([c[2], p]); }
      L.sort((u, v) => v[0] - u[0]);
      g.globalCompositeOperation = 'lighter'; g.lineWidth = 1;
      const quad = q => { g.beginPath(); g.moveTo(q[0][0], q[0][1]); for (let i = 1; i < 4; i++) g.lineTo(q[i][0], q[i][1]); g.closePath(); };
      for (const [zv, p] of L) {
        const hh = p.h * ease(rise * .8 - p.d * .09) * (1 + .05 * Math.sin(t * .5 + p.ph)); if (hh < .03) continue;
        const fog = clamp(1 - (zv - 2.5) / 15), s = .3, cs = [[-s, -s], [s, -s], [s, s], [-s, s]];
        const b = cs.map(c => P(p.x + c[0], 0, p.z + c[1])), u = cs.map(c => P(p.x + c[0], hh, p.z + c[1]));
        if (b.includes(null) || u.includes(null)) continue;
        const sg = g.createLinearGradient(0, u[0][1], 0, b[0][1]); sg.addColorStop(0, `rgba(110,165,255,${.2 * fog})`); sg.addColorStop(1, 'rgba(30,70,220,0)');
        g.fillStyle = sg;
        for (let k = 0; k < 4; k++) { const n = (k + 1) % 4; quad([b[k], b[n], u[n], u[k]]); g.fill(); }
        quad(u); g.fillStyle = `rgba(150,205,255,${.36 * fog})`; g.fill(); g.strokeStyle = `rgba(215,238,255,${.6 * fog})`; g.stroke();
      }
      for (const q of PART) { // drifting lights
        const c = P(q.x, (q.y + t * q.v) % 5, q.z); if (!c) continue;
        const s = clamp(30 / c[2], 2, 10); g.globalAlpha = (.3 + .4 * Math.abs(Math.sin(t * 1.3 + q.ph))) * clamp(1 - (c[2] - 2) / 16);
        g.drawImage(ORB, c[0] - s, c[1] - s, 2 * s, 2 * s);
      }
      g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
      gr = g.createLinearGradient(0, 110, 0, 320); gr.addColorStop(0, '#01030F'); gr.addColorStop(1, 'rgba(1,3,15,0)'); g.fillStyle = gr; g.fillRect(0, 0, W, 320);
      vignette(.55);
    },
    orbs(s, t, front) { // seven orbs on a tilted ring round the cover: behind it, then in front
      const cx = s.x + s.w / 2, cy = s.y + s.h - 36 * s.sc, rx = s.w / 2 + 52 * s.sc, ry = 40 * s.sc;
      g.globalCompositeOperation = 'lighter';
      for (let i = 0; i < 7; i++) for (let tr = 0; tr < 5; tr++) {
        const th = t * 1.25 + i * TAU / 7 - tr * .07, sn = Math.sin(th); if ((sn > 0) !== front) continue;
        const sz = (15 + 6 * sn) * (1 - tr * .13) * Math.max(.6, s.sc); g.globalAlpha = s.f * (tr ? .28 - tr * .05 : 1);
        g.drawImage(ORB, cx + Math.cos(th) * rx - sz, cy + sn * ry - sz, sz * 2, sz * 2);
      }
      g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
    },
    selBack(s, t) { this.orbs(s, t, false); },
    selFront(s, t) { g.globalAlpha = s.f; g.strokeStyle = rgb(A); g.lineWidth = 2; g.strokeRect(s.x - 1, s.y - 1, s.w + 2, s.h + 2); g.globalAlpha = 1; this.orbs(s, t, true); } },

  aurora: { name: 'Aurora',
    tx: '#EEF2FF', t2: '#CBD2EC', lb: '#AEB7D6', ed: 'rgba(255,255,255,.17)', pn: 'rgba(8,12,26,.5)', tg: 'rgba(8,12,26,.42)', inv: '#070A14', oa: '#04221B',
    cedge: 'rgba(255,255,255,.22)', acc: [124, 245, 214], glass: 1, r: 13, R: 14, cr: 8, fr: 11, kr: 6, bar: 1, lp: 1, shadow: 'rgba(0,0,10,.7)', back: false, vp: 'rgba(6,10,22,.85)',
    bg(t) {
      const l = g.createLinearGradient(0, 0, 0, H); l.addColorStop(0, '#03050F'); l.addColorStop(1, '#0A1226'); g.fillStyle = l; g.fillRect(0, 0, W, H);
      bd(.3, t);
      for (let i = 0; i < 90; i++) { g.fillStyle = `rgba(220,230,255,${.25 + .45 * Math.abs(Math.sin(t * (.4 + hash(i, 7)) + i))})`; g.fillRect(Math.floor(hash(i, 8) * W), Math.floor(hash(i, 9) * 520), 1 + (i % 7 === 0), 1 + (i % 7 === 0)); }
      g.globalCompositeOperation = 'lighter'; g.imageSmoothingEnabled = true;
      CUR.forEach((c, ci) => {
        for (let x = 0; x < W; x += 6) {
          const n = Math.sin(x * .0041 + t * .21 + ci * 2.1) + .5 * Math.sin(x * .0113 - t * .17 + ci) + .25 * Math.sin(x * .031 + t * .4);
          const hh = c.h * (.75 + .25 * Math.sin(x * .007 + t * .13 + ci * 4));
          g.globalAlpha = c.a * FXK * (.3 + .7 * Math.pow(.5 + .5 * Math.sin(x * .017 + t * .5 + ci * 3), 2));
          g.drawImage(c.sp, x, c.y + n * 46 - hh, 6, hh);
        }
      });
      g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
    },
    selBack(s, t) { halo(s, t, .34 * (.8 + .2 * Math.sin(t * 1.6)), 120, 300); },
    selFront(s) { ring(s, 'rgba(255,255,255,.75)', 1, .5, 8 * s.sc); } },

  techstep: { name: 'Techstep',
    tx: '#F4F4F0', t2: '#F4F4F0', lb: '#B8B8B0', ed: '#F4F4F0', pn: '#0A0A0A', inv: '#0A0A0A', oa: '#0A0A0A', cedge: '#0A0A0A', cw: 2, acc: [255, 214, 10],
    r: 0, R: 0, cr: 0, wipe: 1, step: 1, fy: 684, aTag: '#FF5A36', fTx: '#0A0A0A', cardTx: '#0A0A0A', btx: '#0A0A0A', blb: '#2E2800', row: '#0A0A0A', back: true,
    fBg: () => rgb(A), cardFill: () => rgb(A), tagEd: () => rgb(A),
    bg(t) {
      g.fillStyle = rgb(A); g.fillRect(0, 0, W, H);
      const k = clamp((t - tMove) / .16), o = G[sel], p = G[prev], st = Math.ceil(k * 4) / 4;
      if (T.back && o.lum) { // halftone: the cover as dots, one tone darker than the ground
        g.fillStyle = 'rgba(0,0,0,.13)';
        for (let y = 160; y < 640; y += 16) for (let x = 8 + (y / 16 % 2) * 8; x < W; x += 16) {
          const u = Math.min(23, x / W * 24 | 0), v = clamp(17 + (y - 395) / 1840 * 34, 0, 33) | 0, id = v * 24 + u;
          const r = (1 - lerp(p.lum ? p.lum[id] : .5, o.lum[id], st)) * 8.4; if (r < .8) continue;
          g.beginPath(); g.arc(x, y, r, 0, TAU); g.fill();
        }
      }
      g.fillStyle = '#0A0A0A';
      if (view === 0 && mode === 'home') { // the position, huge, half hidden under the covers; the new number is pushed in from below in four steps
        font('800 300px Archivo, sans-serif', true);
        g.save(); g.beginPath(); g.rect(0, 150, 520, 490); g.clip();
        const num = i => String(i + 1).padStart(2, '0');
        if (st < 1) g.fillText(num(prev), 44, 636 - st * 300);
        g.fillText(num(sel), 44, 636 + (1 - st) * 300);
        g.restore();
        g.fillRect(540, 604, 676, 2); for (let x = 540; x <= 1216; x += 52) g.fillRect(x, 596, 2, 8); // a ruler and its small print
        font(FM); putText(isArabic() ? 'المكتبة / ' + String(N).replace(/[0-9]/g, d => '٠١٢٣٤٥٦٧٨٩'[d]) + ' لعبة' : 'LIBRARY / ' + N + ' TITLES', 540, 626);
        const tx = 'NTSC-U · 480P · TEa.G3 1.3'; g.fillText(tx, 1216 - g.measureText(tx).width, 626);
        if ((t * 174 / 60) % 1 < .5) g.fillRect(1216 - g.measureText(tx).width - 20, 615, 10, 10); // blinks on the beat
      }
      g.fillRect(0, 0, W, 138); // header: a solid bar and marching hazard stripes
      g.save(); g.beginPath(); g.rect(0, 138, W, 12); g.clip();
      const off = (t * 30) % 24; for (let x = -24; x < W + 24; x += 24) { g.beginPath(); g.moveTo(x + off, 138); g.lineTo(x + off + 12, 138); g.lineTo(x + off, 150); g.lineTo(x + off - 12, 150); g.fill(); }
      g.restore();
    },
    selBack(s, t) { const b = ((t * 174 / 60) % 1 < .14 ? 5 : 0) + 12 * s.sc; g.globalAlpha = s.f; g.fillStyle = '#0A0A0A'; g.fillRect(Math.round(s.x + b), Math.round(s.y + b), Math.round(s.w), Math.round(s.h)); g.globalAlpha = 1; },
    selFront(s) {
      if (view === 1) return;
      font(FM); const tx = 'SEL ' + String(sel + 1).padStart(2, '0') + ' / ' + N, w = Math.round(g.measureText(tx).width) + 24, x = Math.round(s.x), y = Math.round(s.y) - 26;
      g.fillStyle = '#0A0A0A'; g.fillRect(x, y, w, 26); g.fillStyle = rgb(A); g.fillText(tx, x + 12, y + 18);
    },
    mid(t) { // footer bar, with a ticker running along its top
      g.fillStyle = '#0A0A0A'; g.fillRect(0, 640, W, 80);
      g.fillStyle = rgb(A); g.fillRect(0, 668, W, 1);
      font(FM); const s = 'TEa.G3 SYS 1.3  ///  ' + N + ' TITLES  ///  SRC USB  ///  NTSC-U  ///  174 BPM  ///  ', w = g.measureText(s).width;
      for (let x = -((t * 60) % w); x < W; x += w) g.fillText(s, x, 660);
    } },

  liquid: { name: 'Liquid',
    tx: '#FFFFFF', t2: '#F0EDFA', lb: '#D6D6F0', ed: 'rgba(255,255,255,.34)', pn: 'rgba(255,255,255,.12)', tg: 'rgba(255,255,255,.1)', inv: '#0B1F45', oa: '#06282A',
    cedge: 'rgba(255,255,255,.4)', acc: [127, 240, 224], glass: 1, r: 13, R: 20, cr: 12, fr: 11, kr: 6, lp: 1, reflect: .42, ripple: 1, bob: 5, shadow: 'rgba(20,6,50,.55)', back: false,
    chrome: ['#FFFFFF', '#FFF0E4', '#E0BEDF', '#FBEAF6'], vp: 'rgba(20,10,50,.82)',
    bg(t) {
      const HZ = 522, sx = lerp(640, sunX, .75);
      let l = g.createLinearGradient(0, 0, 0, HZ);
      [[0, '#050E24'], [.3, '#0E2F60'], [.55, '#5B3F8F'], [.78, '#D9588A'], [.93, '#FF9E6E'], [1, '#FFD09A']].forEach(s => l.addColorStop(s[0], s[1]));
      g.fillStyle = l; g.fillRect(0, 0, W, HZ);
      bd(.3, t);
      g.save(); g.beginPath(); g.rect(0, 0, W, HZ); g.clip(); // the sun, behind the game you are on
      const r = g.createRadialGradient(sx, SUNY, 200, sx, SUNY, 520); r.addColorStop(0, 'rgba(255,190,130,.5)'); r.addColorStop(1, 'rgba(255,120,140,0)'); g.fillStyle = r; g.fillRect(0, 0, W, HZ);
      l = g.createLinearGradient(0, 100, 0, HZ); l.addColorStop(0, '#FFF6D0'); l.addColorStop(.6, '#FFB36E'); l.addColorStop(1, '#FF6F7D');
      g.fillStyle = l; g.beginPath(); g.arc(sx, SUNY, 262, 0, TAU); g.fill();
      g.restore();
      l = g.createLinearGradient(0, HZ, 0, H); l.addColorStop(0, '#6B3A86'); l.addColorStop(.25, '#2A1F66'); l.addColorStop(1, '#060920'); g.fillStyle = l; g.fillRect(0, HZ, W, H - HZ);
      for (let y = HZ + 4; y < 706; y += 5) { // the sun's road on the water
        const d = (y - HZ) / 184, w = (230 - d * 120) * (.62 + .38 * Math.sin(y * .7 + t * 2.2)) + 20;
        g.fillStyle = `rgba(255,${200 - d * 60 | 0},${150 + d * 20 | 0},${.5 * (1 - d * .8)})`; g.fillRect(sx - w, y, w * 2, 2);
      }
      g.lineCap = 'round'; // swell
      for (let i = 0; i < 6; i++) {
        const y0 = HZ + 14 + i * 30; g.strokeStyle = `rgba(${255 - i * 18},${170 + i * 6},${200 + i * 9},${.34 - i * .04})`; g.lineWidth = 1.5 + i * .7; g.beginPath();
        for (let x = -10; x <= W + 10; x += 14) { const y = y0 + Math.sin(x * (.012 - i * .001) + t * (.9 + i * .13) + i * 1.9) * (4 + i * 1.6) + Math.sin(x * .031 - t * .6 + i) * 2; x < 0 ? g.moveTo(x, y) : g.lineTo(x, y); }
        g.stroke();
      }
      g.lineCap = 'butt'; g.fillStyle = 'rgba(255,225,200,.5)'; g.fillRect(0, HZ, W, 1);
    },
    selFront(s) { ring(s, '#FFFFFF', 2, 2, 13 * s.sc); } },

  onda: { name: 'Onda',
    tx: '#FFFFFF', t2: '#EEF1F8', lb: '#D5DBEA', ed: 'rgba(255,255,255,.26)', pn: 'rgba(0,0,0,.2)', tg: 'rgba(0,0,0,.16)', inv: '#101622', oa: '#0C0F16',
    cedge: 'rgba(255,255,255,.28)', acc: [255, 255, 255], glass: 1, r: 4, R: 6, cr: 4, fr: 3, kr: 3, lp: 1, shadow: 'rgba(0,0,0,.5)', back: false, vp: 'rgba(0,0,0,.6)',
    bg(t) {
      const c = tint, l = g.createLinearGradient(0, 0, W * .3, H); // the cover's colour, deep
      l.addColorStop(0, rgb(c.map(v => v * .16))); l.addColorStop(.55, rgb(c.map(v => v * .5))); l.addColorStop(1, rgb(c.map(v => v * .22)));
      g.fillStyle = l; g.fillRect(0, 0, W, H);
      bd(.25, t);
      g.globalCompositeOperation = 'lighter';
      for (let i = 0; i < 6; i++) { // the ribbon: bands between two drifting curves
        const top = x => 470 + i * 9 + Math.sin(x * .0042 + t * (.32 + i * .03) + i * .5) * (70 - i * 6) + Math.sin(x * .0105 - t * .24 + i * 1.3) * 26;
        const gr = g.createLinearGradient(0, 330, 0, 660); gr.addColorStop(0, 'rgba(255,255,255,0)'); gr.addColorStop(.5, `rgba(255,255,255,${.1 + i * .012})`); gr.addColorStop(1, 'rgba(255,255,255,0)');
        g.fillStyle = gr; g.beginPath();
        for (let x = 0; x <= W; x += 16) g.lineTo(x, top(x));
        for (let x = W; x >= 0; x -= 16) g.lineTo(x, top(x) + 46 + i * 12 + Math.sin(x * .006 + t * .5 + i) * (26 + i * 5));
        g.fill();
        g.strokeStyle = `rgba(255,255,255,${.3 - i * .035})`; g.lineWidth = 1; g.beginPath(); for (let x = 0; x <= W; x += 16) g.lineTo(x, top(x)); g.stroke();
      }
      for (let i = 0; i < 26; i++) { // sparks riding it
        const x = (hash(i, 2) * W + t * (18 + hash(i, 3) * 30)) % W, y = 480 + Math.sin(x * .0042 + t * .32) * 70 + (hash(i, 4) - .5) * 90, s = 2 + hash(i, 5) * 5;
        g.globalAlpha = .25 + .5 * Math.abs(Math.sin(t * 1.4 + i)); g.drawImage(ORB, x - s, y - s, 2 * s, 2 * s);
      }
      g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
      vignette(.4);
    },
    selBack(s, t) { g.save(); g.globalAlpha = s.f * (.7 + .3 * Math.sin(t * 2.2)); g.shadowColor = 'rgba(255,255,255,.9)'; g.shadowBlur = 44 * s.sc; g.fillStyle = '#fff'; g.fillRect(s.x + 4, s.y + 4, s.w - 8, s.h - 8); g.restore(); },
    selFront(s) { ring(s, '#FFFFFF', 1, .5, 4); } },

  orbit: { name: 'Orbit',
    tx: '#EEF3FF', t2: '#C4CFEA', lb: '#A9B6D3', ed: 'rgba(127,231,255,.38)', pn: 'rgba(13,23,54,.72)', tg: 'rgba(13,23,54,.5)', inv: '#04060E', oa: '#04121A',
    cedge: 'rgba(192,139,255,.5)', acc: [127, 231, 255], r: 13, R: 12, cr: 8, fr: 11, kr: 6, lp: 1, row: 'rgba(127,231,255,.16)', reflect: .22, shadow: 'rgba(2,4,20,.8)', back: false,
    chrome: ['#FFFFFF', '#DCE8FF', '#7F92C4', '#D2DCF6'], glow: 'rgba(127,231,255,.45)',
    bg(t) {
      g.fillStyle = '#04060E'; g.fillRect(0, 0, W, H);
      const r = g.createRadialGradient(640, 420, 0, 640, 420, 760); r.addColorStop(0, '#13235A'); r.addColorStop(.5, '#0A1232'); r.addColorStop(1, 'rgba(4,6,14,0)'); g.fillStyle = r; g.fillRect(0, 0, W, H);
      bd(.26, t);
      g.globalCompositeOperation = 'lighter';
      [[560, 150, -.14, .1, [127, 231, 255]], [690, 205, .1, -.07, [192, 139, 255]], [460, 95, -.05, .16, [127, 231, 255]]].forEach(([rx, ry, rot, sp, c], i) => { // orbits, each with its body
        g.save(); g.translate(640, 420); g.rotate(rot + Math.sin(t * .1 + i) * .03);
        g.strokeStyle = rgb(c, .34); g.lineWidth = 1.5; g.beginPath(); g.ellipse(0, 0, rx, ry, 0, 0, TAU); g.stroke();
        g.strokeStyle = rgb(c, .1); g.lineWidth = 7; g.beginPath(); g.ellipse(0, 0, rx, ry, 0, 0, TAU); g.stroke();
        const a = t * sp * 3 + i * 2; for (let k = 0; k < 9; k++) { const s = 13 - k; g.globalAlpha = k ? .3 - k * .03 : 1; g.drawImage(ORB, Math.cos(a - k * .03 * Math.sign(sp)) * rx - s, Math.sin(a - k * .03 * Math.sign(sp)) * ry - s, 2 * s, 2 * s); }
        g.restore();
      });
      g.globalAlpha = 1; g.lineWidth = 1;
      for (let i = 0; i < 26; i++) { // four-point sparkles
        const x = Math.floor(hash(i, 21) * W) + .5, y = Math.floor(hash(i, 22) * 600) + .5, tw = Math.pow(Math.abs(Math.sin(t * (.5 + hash(i, 23)) + i * 2)), 6), s = 2 + tw * 9;
        g.strokeStyle = rgb(i % 3 ? [127, 231, 255] : [192, 139, 255], .25 + tw * .75); g.beginPath(); g.moveTo(x - s, y); g.lineTo(x + s, y); g.moveTo(x, y - s); g.lineTo(x, y + s); g.stroke();
      }
      g.globalCompositeOperation = 'source-over';
    },
    selBack(s, t) { halo(s, t, .3, 110, 300); },
    selFront(s, t) {
      const gr = g.createLinearGradient(s.x, s.y, s.x + s.w, s.y + s.h); gr.addColorStop(0, '#7FE7FF'); gr.addColorStop(1, '#C08BFF'); ring(s, gr, 2, 3, 10 * s.sc);
      const a = t * .9, cx = clamp(s.x + s.w / 2 + Math.cos(a) * s.w, s.x - 3, s.x + s.w + 3), cy = clamp(s.y + s.h / 2 + Math.sin(a) * s.h, s.y - 3, s.y + s.h + 3); // a glint running round the frame
      g.globalCompositeOperation = 'lighter'; g.globalAlpha = s.f; g.drawImage(ORB, cx - 16, cy - 16, 32, 32); g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
    } },

  wireframe: { name: 'Wireframe',
    tx: '#E8FBFF', t2: '#B6E6F0', lb: '#8CC7D4', ed: 'rgba(57,230,255,.5)', pn: 'rgba(0,8,14,.78)', tg: 'rgba(0,8,14,.6)', inv: '#00090C', oa: '#00161A',
    cedge: 'rgba(255,79,216,.7)', acc: [57, 230, 255], aTag: '#FF6FE0', r: 0, R: 0, cr: 0, rule: 1, lp: 1, row: 'rgba(57,230,255,.16)', ghost: 'rgba(255,60,210,.9)', dimmer: .78, back: false,
    bg(t) {
      g.fillStyle = '#00030A'; g.fillRect(0, 0, W, H);
      const hz = g.createLinearGradient(0, 240, 0, 420); hz.addColorStop(0, 'rgba(255,60,210,0)'); hz.addColorStop(.5, 'rgba(255,60,210,.22)'); hz.addColorStop(1, 'rgba(255,60,210,0)'); g.fillStyle = hz; g.fillRect(0, 240, W, 180);
      bd(.22, t);
      const sc = t * 1.7, fr = sc % 1, base = Math.floor(sc), amp = 1 + 1.3 * kick(t);
      g.globalCompositeOperation = 'lighter'; g.lineWidth = 1;
      let last = null;
      for (let i = 22; i >= 0; i--) { // rows from the horizon to your feet; hills at the sides, a valley in the middle
        const z = .75 + (i - fr) * .22, pz = 1 / z, row = [];
        for (let u = -14; u <= 14; u++) { const hgt = Math.max(0, hash(u, i + base) - .25) * Math.min(1, Math.max(0, Math.abs(u) - 1) / 2) * amp; row.push([640 + u * 300 * pz, 318 + 300 * pz - hgt * 110 * pz]); }
        const k = clamp(pz * 1.1);
        g.strokeStyle = `rgba(255,60,210,${.1 + .3 * k})`; g.beginPath(); g.moveTo(0, 318 - 180 * pz); g.lineTo(W, 318 - 180 * pz); // a flat ceiling, mirrored
        row.forEach(p => { g.moveTo(p[0], 318 - 180 * pz); g.lineTo(640 + (p[0] - 640) * (1 + .22 * pz), 318 - 180 / Math.max(.2, z - .22)); }); g.stroke();
        g.strokeStyle = `rgba(${lerp(255, 57, k) | 0},${lerp(60, 230, k) | 0},${lerp(210, 255, k) | 0},${.18 + .6 * k})`; g.beginPath();
        row.forEach((p, n) => n ? g.lineTo(p[0], p[1]) : g.moveTo(p[0], p[1]));
        if (last) row.forEach((p, n) => { g.moveTo(p[0], p[1]); g.lineTo(last[n][0], last[n][1]); });
        g.stroke(); last = row;
      }
      g.globalCompositeOperation = 'source-over';
    },
    selFront(s, t) { const j = kick(t) * 5; g.globalAlpha = s.f; g.lineWidth = 2; g.strokeStyle = '#FF4FD8'; g.strokeRect(s.x - 3 + j, s.y - 3, s.w + 6, s.h + 6); g.strokeStyle = '#39E6FF'; g.strokeRect(s.x - 3 - j, s.y - 3, s.w + 6, s.h + 6); g.globalAlpha = 1; } },

  neuro: { name: 'Neuro',
    tx: '#F2EFEA', t2: '#C2BCB3', lb: '#9A948B', ed: '#34302C', pn: '#131211', inv: '#0A0A0A', oa: '#0A0A0A', cedge: '#2B2826', acc: [255, 98, 36],
    r: 0, R: 0, cr: 0, row: '#1D1A18', dimmer: .62, back: false,
    bg(t) {
      const l = g.createLinearGradient(0, 0, 0, H); l.addColorStop(0, '#0B0A0A'); l.addColorStop(1, '#141110'); g.fillStyle = l; g.fillRect(0, 0, W, H);
      bd(.16, t);
      g.fillStyle = 'rgba(255,255,255,.05)'; for (let y = 160; y < 640; y += 24) for (let x = 16; x < W; x += 24) g.fillRect(x, y, 1, 1);
      const e = kick(t), amp = 8 + 24 * e, wave = x => 600 + amp * (saw(x * .011 - t * 2.1) * .5 + saw(x * .0113 - t * 2.16) * .5) * (.6 + .4 * Math.sin(x * .002 + t)) + Math.sin(x * .09 + t * 30) * e * 3;
      g.globalCompositeOperation = 'lighter'; g.lineJoin = 'round';
      for (const [lw, a] of [[9, .1], [4, .22], [1.5, 1]]) { g.strokeStyle = rgb(A, a); g.lineWidth = lw; g.beginPath(); for (let x = 0; x <= W; x += 4) g.lineTo(x, wave(x)); g.stroke(); }
      g.globalCompositeOperation = 'source-over'; g.lineJoin = 'miter';
      const lit = Math.round((.18 + .8 * e) * 96); // the rule under the header is a level meter
      for (let i = 0; i < 96; i++) { g.fillStyle = i < lit ? (i > 80 ? '#FF2A2A' : rgb(A)) : '#2A2623'; g.fillRect(64 + i * 12, 136, 10, 4); }
      vignette(.5);
    },
    selBack(s, t) { halo(s, t, .22 + .25 * kick(t), 120, 280); },
    selFront(s, t) { const p = kick(t) * 4; g.globalAlpha = s.f; g.strokeStyle = rgb(A); g.lineWidth = 2; g.strokeRect(s.x - 2 - p, s.y - 2 - p, s.w + 4 + 2 * p, s.h + 4 + 2 * p); g.globalAlpha = 1; } },

  fosforo: { name: 'Fósforo',
    tx: '#6BFF96', t2: '#4FE07C', lb: '#3DB965', ed: 'rgba(80,255,130,.5)', pn: 'rgba(2,20,8,.85)', tg: 'rgba(2,20,8,.6)', inv: '#021006', oa: '#021006', cedge: 'rgba(80,255,130,.6)', acc: [107, 255, 150],
    r: 0, R: 0, cr: 0, rule: 1, type: 1, mono: 1, row: 'rgba(80,255,130,.16)', glow: 'rgba(80,255,130,.8)', back: false,
    bg(t) {
      g.fillStyle = '#010703'; g.fillRect(0, 0, W, H);
      const r = g.createRadialGradient(640, 360, 0, 640, 360, 760); r.addColorStop(0, 'rgba(20,90,40,.42)'); r.addColorStop(1, 'rgba(1,7,3,0)'); g.fillStyle = r; g.fillRect(0, 0, W, H);
      bd(.14, t);
      if (view === 0) { // memory addresses scrolling behind the covers
        font(FM); g.fillStyle = 'rgba(80,255,130,.16)';
        for (let c = 0; c < 6; c++) for (let i = 0; i < 28; i++) { const row = i + Math.floor(t * 3); g.fillText((0x80000 + row * 16 + c * 0x4000).toString(16).toUpperCase().padStart(8, '0') + ' ' + (hash(row, c) * 0xFFFFFF | 0).toString(16).toUpperCase().padStart(6, '0'), 64 + c * 200, 166 + i * 18 - (t * 3 % 1) * 18); }
        g.fillStyle = '#010703'; g.fillRect(0, 0, W, 139);
      }
    },
    selFront(s, t) { hud(s, t, 22, 2); g.globalAlpha = s.f; g.strokeStyle = rgb(A); g.lineWidth = 1; g.strokeRect(s.x - 3.5, s.y - 3.5, s.w + 7, s.h + 7); g.globalAlpha = 1; },
    fx(t) {
      g.fillStyle = g.createPattern(SCAN2, 'repeat'); g.fillRect(0, 0, W, H);
      const y = (t * 90) % (H + 240) - 120, l = g.createLinearGradient(0, y - 60, 0, y + 60); l.addColorStop(0, 'rgba(120,255,160,0)'); l.addColorStop(.5, 'rgba(120,255,160,.07)'); l.addColorStop(1, 'rgba(120,255,160,0)');
      g.fillStyle = l; g.fillRect(0, y - 60, W, 120); // the refresh bar rolling down
      g.fillStyle = `rgba(0,0,0,${.03 + .03 * Math.sin(t * 47)})`; g.fillRect(0, 0, W, H); vignette(.6);
    } },

  papel: { name: 'Papel',
    tx: '#1B1A17', t2: '#45423B', lb: '#666158', ed: '#CFC9BD', pn: '#FBFAF6', tg: '#FBFAF6', inv: '#F4F1EA', oa: '#FFFFFF', cedge: 'rgba(0,0,0,.18)', acc: 'coverDark',
    r: 3, R: 4, cr: 3, fr: 3, kr: 3, rule: 1, row: '#FFFFFF', shadow: 'rgba(70,55,30,.3)', back: true,
    bg(t) {
      g.fillStyle = '#F1EEE7'; g.fillRect(0, 0, W, H);
      bd(.2, t);
      const l = g.createLinearGradient(0, 380, 0, H); l.addColorStop(0, 'rgba(241,238,231,0)'); l.addColorStop(1, 'rgba(226,221,210,.9)'); g.fillStyle = l; g.fillRect(0, 380, W, 340);
      g.fillStyle = g.createPattern(GRAIN, 'repeat'); g.fillRect(0, 0, W, H);
    },
    selBack(s) { g.save(); g.globalAlpha = s.f; g.shadowColor = 'rgba(60,45,20,.4)'; g.shadowBlur = 50 * s.sc; g.shadowOffsetY = 26 * s.sc; g.fillStyle = '#000'; g.fillRect(s.x + 10, s.y + 10, s.w - 20, s.h - 20); g.restore(); },
    selFront(s) { ring(s, rgb(A), 3, 5, 5); } }
};

// ---- splash and loading screens of the three themes that have their own so far ----
Object.assign(THEMES.nuld, {
  splash(u, t) { // the launcher's splash as it is today: floor, wire sphere, the logo cut in with a glitch, the status line
    const acid = [200, 255, 46], fg = '#E6E8EC', k = Math.floor(u / 3) % 2, into = u % 3, cut = into < .18 || into > 2.9;
    g.fillStyle = '#06070A'; g.fillRect(0, 0, W, H); floor(t, acid, .5); sphere(t, acid);
    const logo = dx => {
      if (!k) { wordmark(640, 300, 96, fg, rgb(acid), dx); return; }
      font('800 132px Archivo, sans-serif', true); const x = 640 - g.measureText('TEa.G3//').width / 2 + dx, wn = g.measureText('TEa.G3').width + 3;
      g.fillStyle = fg; g.fillText('TEa.G3', x, 345); g.fillStyle = rgb(acid); g.fillText('//', x + wn, 345);
    };
    if (!cut) logo(0);
    else {
      const off = [-10, 7, -4, 9, -6], f = Math.floor(t * 60);
      for (let b = 0; b < 5; b++) { g.save(); g.beginPath(); g.rect(0, 216 + b * 34, W, 34); g.clip(); logo(off[(b + f) % 5]); g.restore(); }
      if (into < .05) { g.fillStyle = 'rgba(230,232,236,.09)'; g.fillRect(0, 0, W, H); }
    }
    font(FM); const s = status(u), w = g.measureText(s).width, x = Math.round(640 - w / 2);
    g.fillStyle = '#06070A'; g.fillRect(x - 12, 482, w + 38, 24); g.fillStyle = fg; g.fillText(s, x, 500);
    if (Math.floor(t * 1.9) % 2 === 0) { g.fillStyle = rgb(acid); g.fillRect(x + w + 4, 486, 8, 16); }
    corners(24, 24, W - 48, H - 48, 28, rgb(acid));
    font(FM); if ('letterSpacing' in g) g.letterSpacing = '1px';
    g.fillStyle = '#8A919C'; g.fillText('TEa.G3 1.3', 64, 44); const r = 'EE R5900 · 294.912 MHz'; g.fillText(r, W - 64 - g.measureText(r).width, 44);
    g.fillText('SCPH-75001', W - 64 - g.measureText('SCPH-75001').width, H - 30);
    if ('letterSpacing' in g) g.letterSpacing = '0px';
    this.fx();
  },
  launch(u, t, o) { // today's launch screen, plus corners that close in and a scanner while it loads
    const k = ease(u / .34), acid = rgb(T.acc), m = 34 * (1 - ease(u / .5));
    g.fillStyle = '#06070A'; g.fillRect(0, 0, W, H); floor(t, [230, 232, 236], .14);
    g.globalAlpha = k; mark(640, 278, 28, '#E6E8EC', acid);
    font(FT, true); centred(fit(o.t, 1000), 372, FT, '#E6E8EC'); centred(HOW, 414, FM, '#8A919C', 3);
    if (o.ach) centred(uiText('ACHIEVEMENTS', 'الإنجازات'), 444, isArabic() ? FU : FM, acid, 3);
    corners(340 - m, 214 - m, 600 + 2 * m, 276 + 2 * m, 28, acid);
    g.fillStyle = '#2A303A'; g.fillRect(520, 466, 240, 2); g.fillStyle = acid; g.fillRect(520 + (t * 170) % 200, 466, 40, 2);
    g.globalAlpha = 1; this.fx();
  }
});
Object.assign(THEMES.cristal, {
  splash(u, t) { // the whole library, out of focus, drifting behind a plate of glass
    const warm = [255, 180, 84], k = ease(u / .7), y = 204 + 14 * (1 - k), dx = (t * 14) % 230;
    g.fillStyle = '#0B0A14'; g.fillRect(0, 0, W, H); g.imageSmoothingEnabled = true; g.globalAlpha = .85;
    for (let r = 0; r < 3; r++) for (let c = -1; c < 7; c++) { const o = G[((r * 5 + c) % N + N) % N]; if (o.blur) g.drawImage(o.blur, c * 230 - dx + (r % 2) * 115, r * 250 - 30, 256, 300); }
    g.globalAlpha = 1; g.fillStyle = 'rgba(9,8,18,.5)'; g.fillRect(0, 0, W, H); vignette(.6);
    snap();
    g.globalAlpha = k; panel(350, y, 580, 200, 30);
    g.save(); rr(350, y, 580, 200, 30); g.clip(); // a light passing over the glass
    const sx = 100 + (u * 320) % 1300, l = g.createLinearGradient(sx, 0, sx + 200, 0); l.addColorStop(0, 'rgba(255,255,255,0)'); l.addColorStop(.5, 'rgba(255,255,255,.17)'); l.addColorStop(1, 'rgba(255,255,255,0)');
    g.fillStyle = l; g.fillRect(sx, y, 200, 200); g.restore();
    g.globalAlpha = k; wordmark(640, y + 100, 60, '#FFFFFF', rgb(warm));
    g.globalAlpha = k; panel(470, 452, 340, 44, 22, T.tg); centred(status(u), 480, FM, T.t2, 1);
    g.fillStyle = 'rgba(255,255,255,.22)'; rr(490, 518, 300, 4, 2); g.fill(); g.fillStyle = rgb(warm); rr(490, 518, Math.max(4, 300 * clamp(u / 5.4)), 4, 2); g.fill();
    g.globalAlpha = 1;
  },
  launch(u, t, o) { // the game's cover fills the screen out of focus; its details on a pane of glass
    const k = ease(u / .45), acc = rgb(o.acc), s = { x: 262 - 30 * (1 - k), y: 150, w: 256, h: 368, a: k, f: k, sc: 1 }, px = 590 + 40 * (1 - k);
    g.fillStyle = '#0B0A14'; g.fillRect(0, 0, W, H);
    if (o.blur) { const w = W * (1.3 + u * .05), h = w * 276 / 192; g.imageSmoothingEnabled = true; g.drawImage(o.blur, (W - w) / 2, (H - h) / 2, w, h); }
    g.fillStyle = 'rgba(9,8,18,.42)'; g.fillRect(0, 0, W, H); vignette(.55); snap();
    A = o.acc; halo(s, t, .5, 100, 330); cover(o, s, t, true); ring(s, acc, 2, 3, 12);
    g.globalAlpha = k; panel(px, 176, 470, 316, 22);
    font(FT, true); g.fillStyle = '#FFFFFF'; g.fillText(fit(o.t, 410), px + 30, 246);
    font(FM); g.fillStyle = T.t2; g.fillText(o.s + '  ·  PS2  ·  USB  ·  480P', px + 30, 282);
    font(FU); g.fillStyle = T.tx; putText(uiText('Virtual memory card', 'بطاقة ذاكرة افتراضية'), px + 30, 334);
    if (o.ach) { g.fillStyle = acc; putText(uiText('Achievements · ' + o.ach, 'الإنجازات · ' + o.ach), px + 30, 362); }
    font(FM); if ('letterSpacing' in g) g.letterSpacing = '3px'; g.fillStyle = T.lb; putText(HOW, px + 30, 430); if ('letterSpacing' in g) g.letterSpacing = '0px';
    g.fillStyle = 'rgba(255,255,255,.22)'; rr(px + 30, 446, 410, 4, 2); g.fill(); g.fillStyle = acc; rr(px + 30, 446, Math.max(4, 410 * ease(u / 3.8)), 4, 2); g.fill();
    g.globalAlpha = 1;
  }
});
Object.assign(THEMES.torres, {
  splash(u, t) { // from high above the towers down to where the menu looks at them; the lights gather and leave the name
    const k = ease(u / 5), ice = '#8FD8FF', m = clamp(u / 2.6), rad = 430 * Math.pow(1 - ease(m), 1.2), fl = u - 2.6, w = ease((u - 2.7) / 1.1);
    CAM = { h: lerp(11, 4.2, k), d: lerp(3, 9, k), p: lerp(1.15, .42, k), yaw: (1 - k) * 1.5, rise: u * 1.1 }; this.bg(t); CAM = null;
    g.globalCompositeOperation = 'lighter';
    if (m < 1) for (let i = 0; i < 7; i++) for (let tr = 0; tr < 6; tr++) {
      const a = i * TAU / 7 + u * 2.4 - tr * .06, s = (16 + 10 * m) * (1 - tr * .12); g.globalAlpha = tr ? .3 - tr * .04 : 1;
      g.drawImage(ORB, 640 + Math.cos(a) * rad - s, 300 + Math.sin(a) * rad * .5 - s, 2 * s, 2 * s);
    }
    if (fl > 0 && fl < 1.2) { const R = 80 + fl * 520; g.globalAlpha = Math.pow(1 - fl / 1.2, 2); g.drawImage(ORB, 640 - R, 300 - R * .6, 2 * R, 1.2 * R); }
    g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
    if (w > 0) { g.globalAlpha = w; g.shadowColor = 'rgba(90,150,255,.95)'; g.shadowBlur = 26; wordmark(640, 300, 72, '#E8EEFF', ice); g.shadowBlur = 0; centred(status(u), 442, FM, T.lb, 3); g.globalAlpha = 1; }
  },
  launch(u, t, o) { // the towers sink, the orbs close in on the game and become one light
    const k = ease(u / .45), s = { x: 512, y: 120, w: 256, h: 368, a: k, f: k, sc: 1 }, sh = 1 - ease((u - .5) / 2.6);
    CAM = { h: 4.2, d: 9, p: .42, yaw: u * .25, rise: Math.max(0, 2.6 - u * 1.1) }; this.bg(t); CAM = null;
    g.fillStyle = `rgba(1,3,15,${.7 * ease(u / 3)})`; g.fillRect(0, 0, W, H);
    const orb = front => {
      g.globalCompositeOperation = 'lighter';
      for (let i = 0; i < 7; i++) for (let tr = 0; tr < 5; tr++) {
        const th = t * 1.25 + u * u * 1.4 + i * TAU / 7 - tr * .07 * (1 + u), sn = Math.sin(th); if ((sn > 0) !== front) continue;
        const sz = (15 + 6 * sn) * (1 - tr * .13); g.globalAlpha = k * (tr ? .28 - tr * .05 : 1);
        g.drawImage(ORB, 640 + Math.cos(th) * 180 * sh - sz, lerp(304, 452, sh) + sn * 40 * sh - sz, 2 * sz, 2 * sz);
      }
      g.globalAlpha = 1; g.globalCompositeOperation = 'source-over';
    };
    orb(false); cover(o, s, t, true);
    g.globalAlpha = k; g.strokeStyle = rgb(T.acc); g.lineWidth = 2; g.strokeRect(s.x - 1, s.y - 1, s.w + 2, s.h + 2); g.globalAlpha = 1;
    orb(true);
    if (sh < .05) { const R = 70 + 26 * Math.sin(t * 6) + (u - 3.1) * 130; g.globalCompositeOperation = 'lighter'; g.globalAlpha = .9; g.drawImage(ORB, 640 - R, 304 - R, 2 * R, 2 * R); g.globalAlpha = 1; g.globalCompositeOperation = 'source-over'; }
    g.globalAlpha = k; g.shadowColor = 'rgba(90,150,255,.9)'; g.shadowBlur = 16; font(FT, true); centred(fit(o.t, 1000), 548, FT, '#E8EEFF'); g.shadowBlur = 0;
    centred(HOW, 584, FM, T.lb, 3); g.globalAlpha = 1;
  }
});
// ---- and of the other nine ----
let FXK = 1, SUNY = 360;
const hueRgb = h => [0, 8, 4].map(n => { const k = (n + h / 30) % 12; return 255 * (.5 - .5 * Math.max(-1, Math.min(k - 3, 9 - k, 1))); });
const st4 = n => Math.ceil(clamp(n) * 4) / 4; // four hard steps
function hazard(y0, t) { g.save(); g.beginPath(); g.rect(0, y0, W, 12); g.clip(); const off = (t * 30) % 24; for (let x = -24; x < W + 24; x += 24) { g.beginPath(); g.moveTo(x + off, y0); g.lineTo(x + off + 12, y0); g.lineTo(x + off, y0 + 12); g.lineTo(x + off - 12, y0 + 12); g.fill(); } g.restore(); }
function cube(cx, cy, R, a, c) { // a wire cube turning on two axes
  const v = [], ca = Math.cos(a), sa = Math.sin(a), cb = Math.cos(a * .7), sb = Math.sin(a * .7);
  for (let i = 0; i < 8; i++) { let x = i & 1 ? 1 : -1, y = i & 2 ? 1 : -1, z = i & 4 ? 1 : -1; [x, z] = [x * ca - z * sa, x * sa + z * ca]; [y, z] = [y * cb - z * sb, y * sb + z * cb]; const q = 3.2 / (3.2 + z); v.push([cx + x * R * q, cy + y * R * q]); }
  g.strokeStyle = c; g.lineWidth = 1; g.beginPath(); for (let i = 0; i < 8; i++) for (const b of [1, 2, 4]) if (!(i & b)) { g.moveTo(v[i][0], v[i][1]); g.lineTo(v[i | b][0], v[i | b][1]); } g.stroke();
}
function bass(y0, amp, t, c) { // the bass wave, with its afterglow
  g.globalCompositeOperation = 'lighter'; g.lineJoin = 'round';
  for (const [lw, a] of [[9, .1], [4, .22], [1.5, 1]]) { g.strokeStyle = rgb(c, a); g.lineWidth = lw; g.beginPath(); for (let x = 0; x <= W; x += 4) g.lineTo(x, y0 + amp * (saw(x * .011 - t * 2.1) * .5 + saw(x * .0113 - t * 2.16) * .5) * (.6 + .4 * Math.sin(x * .002 + t))); g.stroke(); }
  g.globalCompositeOperation = 'source-over'; g.lineJoin = 'miter';
}
function meter(y, lit, c) { for (let i = 0; i < 96; i++) { g.fillStyle = i < lit ? (i > 80 ? '#FF2A2A' : c) : '#2A2623'; g.fillRect(64 + i * 12, y, 10, 4); } }
const chromeAt = (y0, y1, stops) => { const c = g.createLinearGradient(0, y0, 0, y1); stops.forEach((v, i) => c.addColorStop([0, .5, .56, 1][i], v)); return c; };

Object.assign(THEMES.aurora, {
  splash(u, t) { // the curtains come up out of a dark sky
    FXK = ease(u / 3.5); this.bg(t); FXK = 1; snap();
    const k = ease((u - 1) / 1.2); g.globalAlpha = k; g.shadowColor = 'rgba(124,245,214,.8)'; g.shadowBlur = 30; wordmark(640, 290, 72, '#EEF2FF', rgb(T.acc)); g.shadowBlur = 0;
    panel(470, 440, 340, 44, 22, T.tg); centred(status(u), 468, FM, T.t2, 1); g.globalAlpha = 1;
  },
  launch(u, t, o) { // they flare while the game loads
    const k = ease(u / .45), s = { x: 512, y: 96 + 14 * (1 - k), w: 256, h: 368, a: k, f: k, sc: 1 };
    FXK = 1 + ease(u / 3) * .8; this.bg(t); FXK = 1; g.fillStyle = `rgba(3,5,15,${.3 * ease(u)})`; g.fillRect(0, 0, W, H); snap();
    halo(s, t, .45, 120, 340); cover(o, s, t, true); ring(s, 'rgba(255,255,255,.75)', 1, .5, 8);
    g.globalAlpha = k; panel(-1, 520, W + 2, 202, 0); font(FT, true); centred(fit(o.t, 1000), 584, FT, T.tx); centred(HOW, 618, FM, T.lb, 3);
    g.fillStyle = 'rgba(255,255,255,.18)'; g.fillRect(440, 644, 400, 3); g.fillStyle = rgb(T.acc); g.fillRect(440, 644, Math.round(400 * ease(u / 3.8)), 3); g.globalAlpha = 1;
  }
});
Object.assign(THEMES.techstep, {
  splash(u, t) { // a band of colour snaps open, the name is pushed in from the side, all in hard steps
    const Y = rgb(T.acc), K = '#0A0A0A', hgt = Math.round(500 * st4(u / .3)), top = 360 - hgt / 2, sl = (1 - st4((u - .3) / .3)) * 1300;
    g.fillStyle = K; g.fillRect(0, 0, W, H); g.fillStyle = Y; g.fillRect(0, top, W, hgt);
    if (hgt > 40) { g.fillStyle = K; hazard(top, t); hazard(top + hgt - 12, t); }
    font('800 250px Archivo, sans-serif', true); const w = g.measureText('TEa.G3').width, x0 = Math.round(640 - (230 + w) / 2);
    g.save(); g.beginPath(); g.rect(0, top, W, hgt); g.clip(); g.fillStyle = K; g.fillText('TEa.G3', x0 + 230 + sl, 448); mark(x0 + 95 - sl, 362, 95, K, '#FF5A36'); g.restore();
    font(FM);
    if (u > .7) {
      const m = status(u), b = '174 BPM', bw = g.measureText(b).width;
      g.fillStyle = K; g.fillRect(64, 548, Math.round(g.measureText(m).width) + 24, 26); g.fillText(b, 1216 - bw, 566); if ((t * 174 / 60) % 1 < .5) g.fillRect(1216 - bw - 20, 555, 10, 10);
      g.fillStyle = Y; putText(m, 76, 566);
    }
    g.fillStyle = Y; const tk = 'TEa.G3 SYS 1.3  ///  EE R5900  ///  294.912 MHZ  ///  174 BPM  ///  ', tw = g.measureText(tk).width;
    for (let x = -((t * 60) % tw); x < W; x += tw) { g.fillText(tk, x, 62); g.fillText(tk, x - tw / 2, 672); }
  },
  launch(u, t, o) { // a black block slams in with the cover; the load counts up in squares
    const Y = rgb(T.acc), K = '#0A0A0A', wd = Math.round(540 * st4(u / .25)), n = Math.floor(clamp(u / 3.9) * 16);
    this.bg(t);
    g.fillStyle = K; g.fillRect(0, 150, wd, 490);
    if (wd > 500) { g.fillStyle = Y; g.fillRect(154, 208, 256, 368); cover(o, { x: 142, y: 196, w: 256, h: 368, a: 1, f: 1 }, t, true); }
    mark(92, 73, 24, '#F4F4F0', '#FF5A36'); font(FT, true); g.fillStyle = '#F4F4F0'; g.fillText(fit(o.t, 1000), 140, 70);
    font(FM); g.fillStyle = Y; g.fillText(o.s + '  ///  PS2  ///  USB  ///  480P', 140, 104);
    if (u > .3) {
      font('800 230px Archivo, sans-serif', true); g.fillStyle = K; g.fillText(String(sel + 1).padStart(2, '0'), 590, 400);
      font(FM); g.fillRect(596, 440, Math.round(g.measureText(HOW).width) + 24, 26); g.fillStyle = Y; putText(HOW, 608, 458); g.fillStyle = K;
      for (let i = 0; i < 16; i++) { const x = 596 + i * 38; if (i < n) g.fillRect(x, 500, 30, 30); else { g.fillRect(x, 500, 30, 2); g.fillRect(x, 528, 30, 2); g.fillRect(x, 500, 2, 30); g.fillRect(x + 28, 500, 2, 30); } }
      g.fillText(String(Math.round(n / 16 * 100)).padStart(3, ' ') + ' %', 596, 562);
    }
    this.mid(t);
  }
});
Object.assign(THEMES.liquid, {
  splash(u, t) { // sunrise: the sun comes up out of the water and the night lifts
    const k = ease(u / 4.5), w = ease((u - 1.4) / 1.2);
    SUNY = lerp(790, 360, k); this.bg(t); SUNY = 360;
    g.fillStyle = `rgba(4,8,26,${.7 * (1 - k)})`; g.fillRect(0, 0, W, H);
    g.globalAlpha = w; wordmark(640, 608, 52, '#FFFFFF', rgb(T.acc)); centred(status(u), 676, FM, T.lb, 3); g.globalAlpha = 1;
  },
  launch(u, t, o) { // sunset: the sun goes under while the game loads
    const k = ease(u / .45), d = ease(u / 4), s = { x: 512, y: 104, w: 256, h: 368, a: k, f: k, sc: 1 };
    SUNY = lerp(360, 800, d); this.bg(t); SUNY = 360;
    g.fillStyle = `rgba(4,8,26,${.6 * d})`; g.fillRect(0, 0, W, H);
    cover(o, s, t, true); ring({ ...s, y: s.y + Math.sin(t * 1.1 + sel * 1.7) * 5 }, '#FFFFFF', 2, 2, 13);
    g.globalAlpha = k; font(FT, true); centred(fit(o.t, 1000), 618, FT, chromeAt(588, 622, T.chrome)); centred(HOW, 650, FM, T.lb, 3);
    g.fillStyle = 'rgba(255,255,255,.2)'; g.fillRect(440, 668, 400, 2); g.fillStyle = rgb(T.acc); g.fillRect(440, 668, Math.round(400 * ease(u / 3.9)), 2); g.globalAlpha = 1;
  }
});
Object.assign(THEMES.onda, {
  splash(u, t) { // the ribbon over a ground that drifts through every colour
    const k = ease((u - .6) / 1);
    tint = hueRgb((t * 45) % 360); this.bg(t);
    g.globalAlpha = k; wordmark(640, 270, 70, '#FFFFFF', 'rgba(255,255,255,.75)'); centred(status(u), 396, FM, T.lb, 3); g.globalAlpha = 1;
  },
  launch(u, t, o) { // the ground is already the game's colour; a thread of light runs across as it loads
    const k = ease(u / .45), s = { x: 512, y: 88, w: 256, h: 368, a: k, f: k, sc: 1 }, x = W * ease(u / 4);
    this.bg(t); this.selBack(s, t); cover(o, s, t, true); ring(s, '#FFFFFF', 1, .5, 4);
    g.globalAlpha = k; font(FT, true); centred(fit(o.t, 1000), 548, FT, '#FFFFFF'); centred(HOW, 580, FM, T.lb, 3);
    g.fillStyle = 'rgba(255,255,255,.8)'; g.fillRect(0, 612, x, 1); g.globalCompositeOperation = 'lighter'; g.drawImage(ORB, x - 14, 598, 28, 28); g.globalCompositeOperation = 'source-over'; g.globalAlpha = 1;
  }
});
Object.assign(THEMES.orbit, {
  splash(u, t) { // the orbits are drawn one after another, each led by its body; then the name in chrome
    const w = ease((u - 1.8) / 1);
    g.fillStyle = '#04060E'; g.fillRect(0, 0, W, H);
    const r = g.createRadialGradient(640, 420, 0, 640, 420, 760); r.addColorStop(0, '#13235A'); r.addColorStop(.5, '#0A1232'); r.addColorStop(1, 'rgba(4,6,14,0)'); g.fillStyle = r; g.fillRect(0, 0, W, H);
    g.globalCompositeOperation = 'lighter';
    [[560, 150, -.14, [127, 231, 255]], [690, 205, .1, [192, 139, 255]], [460, 95, -.05, [127, 231, 255]]].forEach(([rx, ry, rot, c], i) => {
      const k = ease((u - i * .45) / 1.8), a = -Math.PI / 2 + TAU * k; if (k <= 0) return;
      g.save(); g.translate(640, 330); g.rotate(rot); g.strokeStyle = rgb(c, .5); g.lineWidth = 1.5; g.beginPath(); g.ellipse(0, 0, rx, ry, 0, -Math.PI / 2, a); g.stroke();
      g.drawImage(ORB, Math.cos(a) * rx - 15, Math.sin(a) * ry - 15, 30, 30); g.restore();
    });
    g.lineWidth = 1;
    for (let i = 0; i < 26; i++) { const x = Math.floor(hash(i, 21) * W) + .5, y = Math.floor(hash(i, 22) * 600) + .5, tw = Math.pow(Math.abs(Math.sin(t * (.5 + hash(i, 23)) + i * 2)), 6) * ease(u / 2), sz = 2 + tw * 9;
      g.strokeStyle = rgb(i % 3 ? [127, 231, 255] : [192, 139, 255], (.25 + tw * .75) * ease(u / 2)); g.beginPath(); g.moveTo(x - sz, y); g.lineTo(x + sz, y); g.moveTo(x, y - sz); g.lineTo(x, y + sz); g.stroke(); }
    g.globalCompositeOperation = 'source-over';
    g.globalAlpha = w; g.shadowColor = 'rgba(127,231,255,.6)'; g.shadowBlur = 24; wordmark(640, 330, 72, chromeAt(294, 366, T.chrome), '#C08BFF'); g.shadowBlur = 0; centred(status(u), 470, FM, T.lb, 3); g.globalAlpha = 1;
  },
  launch(u, t, o) { // a line of light travels round the cover: once round, loaded
    const k = ease(u / .45), s = { x: 512, y: 96, w: 256, h: 368, a: k, f: k, sc: 1 }, per = 1330;
    this.bg(t); halo(s, t, .34, 110, 320); cover(o, s, t, true);
    const gr = g.createLinearGradient(s.x, s.y, s.x + s.w, s.y + s.h); gr.addColorStop(0, '#7FE7FF'); gr.addColorStop(1, '#C08BFF');
    g.globalAlpha = k; rr(s.x - 12, s.y - 12, s.w + 24, s.h + 24, 16); g.strokeStyle = 'rgba(127,231,255,.2)'; g.lineWidth = 3; g.stroke();
    g.setLineDash([per * ease(u / 3.9), per * 2]); g.strokeStyle = gr; g.stroke(); g.setLineDash([]);
    font(FT, true); g.shadowColor = 'rgba(127,231,255,.45)'; g.shadowBlur = 16; centred(fit(o.t, 1000), 566, FT, chromeAt(536, 570, T.chrome)); g.shadowBlur = 0; centred(HOW, 598, FM, T.lb, 3); g.globalAlpha = 1;
  }
});
Object.assign(THEMES.wireframe, {
  splash(u, t) { // a wire cube turns over the terrain; the name with its magenta ghost
    const k = ease((u - .5) / .8), j = kick(t) * 4;
    this.bg(t); g.globalCompositeOperation = 'lighter'; cube(640, 300, 170 * ease(u / 1.2), t * .6, 'rgba(57,230,255,.5)'); g.globalCompositeOperation = 'source-over';
    g.globalAlpha = k; wordmark(644 + j, 300, 74, 'rgba(255,60,210,.9)', 'rgba(255,60,210,.9)'); wordmark(640, 300, 74, '#E8FBFF', '#39E6FF'); centred('> ' + status(u), 440, isArabic() ? FU : FM, T.t2, 3); g.globalAlpha = 1;
  },
  launch(u, t, o) { // the cube closes round the cover; the load is a row of segments from cyan to magenta
    const k = ease(u / .45), s = { x: 512, y: 96, w: 256, h: 368, a: k, f: k, sc: 1 }, n = Math.floor(clamp(u / 3.9) * 24), j = kick(t) * 4;
    this.bg(t); g.globalCompositeOperation = 'lighter'; cube(640, 280, lerp(420, 250, ease(u / 3)), t * .9, 'rgba(255,79,216,.45)'); g.globalCompositeOperation = 'source-over';
    cover(o, s, t, true); this.selFront(s, t);
    g.globalAlpha = k; font(FT, true); const ti = fit(o.t, 1000); centred(ti, 557, FT, 'rgba(255,60,210,.9)'); g.fillStyle = '#E8FBFF'; g.fillText(ti, Math.round(640 - g.measureText(ti).width / 2) - 3 - j, 556);
    centred(HOW, 588, FM, T.lb, 3);
    for (let i = 0; i < 24; i++) { const q = i / 23; g.fillStyle = i < n ? rgb([lerp(57, 255, q), lerp(230, 79, q), lerp(255, 216, q)]) : 'rgba(57,230,255,.18)'; g.fillRect(448 + i * 16, 608, 12, 6); }
    g.globalAlpha = 1;
  }
});
Object.assign(THEMES.neuro, {
  splash(u, t) { // a flat line, then the drop: the wave opens up and the name lands with it
    const c = T.acc, drop = u > 1.6, e = drop ? kick(t) : 0, f = u - 1.6, lit = drop ? Math.round((.2 + .8 * e) * 96) : 2;
    const l = g.createLinearGradient(0, 0, 0, H); l.addColorStop(0, '#0B0A0A'); l.addColorStop(1, '#141110'); g.fillStyle = l; g.fillRect(0, 0, W, H);
    bass(300, drop ? 26 + 90 * e : 1 + u * 4, t, c); meter(60, lit, rgb(c)); meter(656, lit, rgb(c));
    if (drop) { g.fillStyle = '#0B0A0A'; g.fillRect(330, 226, 620, 150); wordmark(640, 300, 76, '#F2EFEA', rgb(c)); if (f < .25) { g.fillStyle = `rgba(255,240,230,${.5 * (1 - f / .25)})`; g.fillRect(0, 0, W, H); } }
    font(FM); g.fillStyle = '#9A948B'; g.fillText('174 BPM', 64, 44); centred(status(u), 470, FM, '#C2BCB3', 3); vignette(.5);
  },
  launch(u, t, o) { // the cover takes the hits of kick and snare; the meter fills as it loads
    const c = T.acc, e = kick(t), k = ease(u / .4), q = e * 5, s = { x: 512 - q, y: 110 - q, w: 256 + 2 * q, h: 368 + 2 * q, a: k, f: k, sc: 1 };
    const l = g.createLinearGradient(0, 0, 0, H); l.addColorStop(0, '#0B0A0A'); l.addColorStop(1, '#141110'); g.fillStyle = l; g.fillRect(0, 0, W, H);
    bass(294, 18 + 70 * e, t, c); halo(s, t, .25 + .3 * e, 120, 300); cover(o, s, t, true);
    g.globalAlpha = k; g.strokeStyle = rgb(c); g.lineWidth = 2; g.strokeRect(s.x - 2, s.y - 2, s.w + 4, s.h + 4);
    font(FT, true); centred(fit(o.t, 1000), 548, FT, '#F2EFEA'); meter(574, Math.round(96 * clamp(u / 3.9)), rgb(c)); centred(HOW, 612, FM, '#9A948B', 3); g.globalAlpha = 1; vignette(.5);
  }
});
Object.assign(THEMES.fosforo, {
  ground() { g.fillStyle = '#010703'; g.fillRect(0, 0, W, H); const r = g.createRadialGradient(640, 360, 0, 640, 360, 760); r.addColorStop(0, 'rgba(20,90,40,.42)'); r.addColorStop(1, 'rgba(1,7,3,0)'); g.fillStyle = r; g.fillRect(0, 0, W, H); },
  splash(u, t) { // a boot log types itself out, then the name
    const gr = '#6BFF96', L = isArabic()
      ? ['TEa.G3 BIOS 1.3', 'معالج EE R5900  294.912 MHz ............ OK', 'ذاكرة RAM  32 MB ....................... OK', 'وحدة USB MASS ......................... OK', 'شبكة NET  100 MBIT .................... OK', 'المكتبة  ١٦ لعبة ............. OK']
      : ['TEa.G3 BIOS 1.3', 'EE R5900  294.912 MHz ............ OK', 'RAM  32 MB ....................... OK', 'USB MASS ......................... OK', 'NET  100 MBIT .................... OK', 'LIBRARY  16 GAMES ............. OK'];
    const n = clamp(Math.floor((u - .3) / .5) + 1, 0, L.length), w = ease((u - 3.4) / .6);
    this.ground(); font(isArabic() ? FU : FM); g.shadowColor = 'rgba(80,255,130,.8)'; g.shadowBlur = 8; g.fillStyle = gr;
    L.forEach((m, i) => { const a = (u - .3 - i * .5) / .35; if (a > 0) putText(m.slice(0, Math.ceil(clamp(a) * m.length)), 96, 96 + i * 22); });
    if (Math.floor(t * 2.5) % 2 === 0) g.fillRect(96, 96 + n * 22 - 14, 10, 16);
    if (w > 0) { g.globalAlpha = w; g.shadowBlur = 22; wordmark(640, 420, 84, gr, gr); g.globalAlpha = 1; }
    g.shadowBlur = 0; this.fx(t);
  },
  launch(u, t, o) { // a terminal runs the game; its cover develops from green to full colour
    const gr = '#6BFF96', n = Math.floor(clamp(u / 3.9) * 28), im = o.im[2], ok = im.complete && im.naturalWidth;
    const L = isArabic()
      ? ['> تشغيل ' + o.s.replace('-', '_'), '', 'العنوان  ' + o.t, 'المصدر    USB', 'الفيديو    480P', 'الإنجازات  ' + (o.ach || 'لا يوجد'), '', HOW, '[' + '#'.repeat(n) + '.'.repeat(28 - n) + ']  ' + String(Math.round(n / 28 * 100)).padStart(3, ' ') + ' %']
      : ['> RUN ' + o.s.replace('-', '_'), '', 'TITLE     ' + o.t.toUpperCase(), 'SOURCE    USB', 'VIDEO     480P', 'TROPHIES  ' + (o.ach || 'NO'), '', HOW, '[' + '#'.repeat(n) + '.'.repeat(28 - n) + ']  ' + String(Math.round(n / 28 * 100)).padStart(3, ' ') + ' %'];
    this.ground(); font(FM); g.shadowColor = 'rgba(80,255,130,.8)'; g.shadowBlur = 8; g.fillStyle = gr;
    L.forEach((m, i) => { if (u > .14 * i) putText(i ? fit(m, 640) : m.slice(0, Math.ceil(clamp(u / .5) * m.length)), 96, 210 + i * 26); });
    if (Math.floor(t * 2.5) % 2 === 0) g.fillRect(96, 210 + L.length * 26 - 14, 10, 16);
    g.shadowBlur = 0; g.imageSmoothingEnabled = false;
    if (ok) { g.drawImage(mono(o, 2), 840, 150); g.globalAlpha = ease((u - .8) / 2.4); g.drawImage(im, 840, 150); g.globalAlpha = 1; }
    g.strokeStyle = gr; g.lineWidth = 1; g.strokeRect(839.5, 149.5, 257, 369); corners(826, 136, 284, 396, 22, gr);
    this.fx(t);
  }
});
Object.assign(THEMES.papel, {
  splash(u, t) { // two rules open from the centre and the name is stamped between them, over a wash of the library's colours
    const ink = '#1B1A17', k = ease((u - .5) / .4), len = 430 * ease(u / 1.2), i = Math.floor(u / 1.6) % N, o = G[i], a = Math.sin(Math.PI * ((u / 1.6) % 1));
    g.fillStyle = '#F1EEE7'; g.fillRect(0, 0, W, H);
    if (o.blur) { g.globalAlpha = .24 * a; g.imageSmoothingEnabled = true; g.drawImage(o.blur, -160, -800, 1600, 2300); g.globalAlpha = 1; }
    g.fillStyle = g.createPattern(GRAIN, 'repeat'); g.fillRect(0, 0, W, H);
    g.fillStyle = ink; g.fillRect(640 - len, 222, 2 * len, 1); g.fillRect(640 - len, 380, 2 * len, 1);
    g.save(); g.translate(640, 302); g.scale(1.14 - .14 * k, 1.14 - .14 * k); g.globalAlpha = k; wordmark(0, 0, 76, ink, '#B4442C'); g.restore();
    g.globalAlpha = ease((u - 1) / .5); centred(status(u), 420, FM, '#45423B', 3); g.globalAlpha = 1;
  },
  launch(u, t, o) { // the cover lifts off the paper, its shadow growing; an ink rule draws itself as it loads
    const k = ease(u / .45), up = ease(u / 3.6), g2 = 6 * up, s = { x: 512 - g2, y: 104 - g2 * 1.44 - 6 * up, w: 256 + 2 * g2, h: 368 + 2.88 * g2, a: k, f: k, sc: 1 + up * 1.3 };
    this.bg(t); this.selBack(s); cover(o, s, t, true); ring(s, rgb(A), 3, 5, 5);
    g.globalAlpha = k; font(FT, true); centred(fit(o.t, 1000), 566, FT, '#1B1A17'); centred(HOW, 598, FM, '#666158', 3);
    g.fillStyle = '#CFC9BD'; g.fillRect(440, 618, 400, 2); g.fillStyle = '#1B1A17'; g.fillRect(440, 618, Math.round(400 * ease(u / 3.9)), 2); g.globalAlpha = 1;
  }
});
function show(m) { mode = m; tMode = now; lastInput = now; }

// ---- driven by the theme maker (themes.js) ----
let overlay = null, tour = null, seen = true; // tour: the idle demo also changes view, and says so
if ('IntersectionObserver' in window) new IntersectionObserver(e => { seen = e[e.length - 1].isIntersecting; }).observe(cv); // drawn over the home, last: the maker's setup panel
const alphaOf = c => { const m = /rgba\([^)]*,\s*([\d.]+)\)/.exec(c || ''); return m ? +m[1] : 1; };
const tone = (hex, like) => { const n = parseInt(hex, 16); return `rgba(${n >> 16},${n >> 8 & 255},${n & 255},${alphaOf(like)})`; };
window.TEaG3Preview = {
  // base: a built-in theme's key; pal / def: the maker's palette and the base's own (RRGGBB by theme.ini key). Only what
  // differs from the base is touched, so an untouched base looks as it does; picture: a canvas, or null
  set(base, pal, def, picture, coverAccent) {
    const S = THEMES[base] || THEMES.nuld, same = T && Object.getPrototypeOf(T) === S, N_ = Object.create(S), ch = k => pal[k] !== def[k];
    N_.id = base;
    if (ch('texto')) N_.tx = N_.rowTx = N_.btx = '#' + pal.texto;
    if (ch('texto2')) N_.t2 = '#' + pal.texto2;
    if (ch('etiqueta')) N_.lb = N_.blb = '#' + pal.etiqueta;
    if (ch('borde')) N_.ed = tone(pal.borde, S.ed), N_.cedge = tone(pal.borde, S.cedge), N_.tagEd = null;
    if (ch('panel')) N_.pn = tone(pal.panel, S.pn), N_.tg = S.tg ? tone(pal.panel, S.tg) : null, N_.vp = '#' + pal.panel, N_.cardFill = null;
    if (ch('fila')) N_.row = N_.rowTint = tone(pal.fila, S.row || S.rowTint);
    if (ch('inverso')) N_.inv = N_.oa = N_.fTx = '#' + pal.inverso;
    if (coverAccent) N_.acc = 'cover';
    else if (ch('acento') || S.acc === 'cover' || S.acc === 'coverDark') { const n = parseInt(pal.acento, 16); N_.acc = [n >> 16, n >> 8 & 255, n & 255]; if (ch('acento')) N_.aTag = null, N_.fBg = null; }
    if (picture) N_.bg = () => { g.imageSmoothingEnabled = true; g.drawImage(picture, 0, 0, W, H); }, N_.lp = 1, N_.mid = null, N_.back = false;
    T = N_; Afrom = A;
    if (!same) tTheme = tFade = now, settle();
  },
  view(v) { if (mode !== 'home') mode = 'home', tFade = now; if (v !== view) setView(v); lastInput = now; },
  show(m) { mode = m; tMode = now; lastInput = now; },
  overlay(fn) { overlay = fn; },
  tour(fn) { tour = fn; },
  get g() { return g; },
};
cv.addEventListener('click', e => { if (mode !== 'home') { mode = 'home'; tFade = now; lastInput = now; return; } const r = cv.getBoundingClientRect(); const d = (e.clientX - r.left) / r.width < .5 ? -1 : 1; move(view === 2 ? d : d); lastInput = now; });

// ---- the frame ----
// Keep the original NULD animation scheduler: one requestAnimationFrame per frame.
function frame(ms) {
  if (!seen) { requestAnimationFrame(frame); return; } // off screen: nothing to draw
  const t = ms / 1000 + T0, dt = Math.min(.05, Math.max(0, t - now)); now = t;
  for (const o of G) if (!o.blur && o.im[2].complete && o.im[2].naturalWidth) derive(o);
  if (mode === 'home' && !overlay && auto.checked && t - lastInput > 5 && t - Math.max(tMove, tView) > 3.2) {
    if (tour && ++autoN % 4 === 0) setView(view + 1), tour(view); else if (!move(autoDir)) { autoDir = -autoDir; move(autoDir); }
  }
  animate(t, dt);
  const o = G[sel], goal = T.acc === 'cover' ? o.acc : T.acc === 'coverDark' ? o.accD : T.acc, ka = ease(Math.min((t - tMove) / .5, (t - tTheme) / .5));
  A = [0, 1, 2].map(i => lerp(Afrom[i], goal[i], ka));
  tint = tint.map((v, i) => v + (o.acc[i] - v) * Math.min(1, dt * 2.5));

  g.setTransform(1, 0, 0, 1, 0, 0); g.globalAlpha = 1; g.globalCompositeOperation = 'source-over'; g.shadowBlur = 0; g.shadowOffsetY = 0; g.textBaseline = 'alphabetic';
  // The canvas uses the original fixed LTR coordinate system; Arabic labels remain shaped by their text runs.
  g.direction = 'ltr'; g.textAlign = 'left';
  if (mode !== 'home') { // splash or launch screen; keep the animation loop alive until it finishes
    const u = t - tMode, spl = mode === 'splash', end = spl ? SPL : LCH, out = (u - end + .45) / .45;
    if (u < end) (spl ? T.splash || genSplash : T.launch || genLaunch).call(T, u, t, o);
    else { T.bg(t); if (!spl) { const msg = isArabic() ? 'هذه معاينة للواجهة فقط — لا يتم تشغيل ملف لعبة في المتصفح' : 'Interface preview only — no game executable runs in the browser'; centred(msg, 366, isArabic() ? FU : FM, '#8A919C', 3); } }
    const dark = Math.max(u < .45 ? 1 - ease(u / .45) : 0, out > 0 && u < end ? ease(out) : 0);
    if (dark > 0) { g.fillStyle = `rgba(0,0,0,${dark})`; g.fillRect(0, 0, W, H); }
    if (u >= end + (spl ? 0 : 1.3)) { mode = 'home'; tFade = t; if (!spl) tTheme = t; lastInput = Math.max(lastInput, t - 3); }
    requestAnimationFrame(frame);
    return;
  }
  T.bg(t);
  // Developer signature on the website's theme backgrounds only; never alters downloaded PS2 assets.
  g.save();
  g.direction = 'ltr'; g.textAlign = 'center'; g.globalAlpha = .72;
  font('500 12px Archivo, "Geeza Pro", "Noto Sans Arabic", Tahoma, sans-serif');
  const watermark = '( تعديل - وتعريب - TEa.G3 )';
  const watermarkWidth = Math.ceil(g.measureText(watermark).width) + 22;
  g.fillStyle = 'rgba(0,0,0,.38)';
  g.fillRect((W - watermarkWidth) / 2, 690, watermarkWidth, 22);
  g.strokeStyle = 'rgba(210,225,255,.34)'; g.lineWidth = 1;
  g.strokeRect((W - watermarkWidth) / 2 + .5, 690.5, watermarkWidth - 1, 21);
  g.fillStyle = '#F1F4FF'; g.fillText(watermark, W / 2, 705);
  g.restore();
  if (T.glass) snap();
  listRows(t);
  covers(t);
  if (T.mid) T.mid(t);
  header(t); footer(t);
  if (T.fx) T.fx(t);
  if (overlay) { g.save(); overlay(g); g.restore(); }
  const fi = (t - tFade) / .35; if (fi < 1) { g.fillStyle = `rgba(0,0,0,${1 - ease(fi)})`; g.fillRect(0, 0, W, H); }
  requestAnimationFrame(frame);
}
window.TEaG3Preview.set('nuld', {}, {}, null, false);
B.forEach((b, i) => Object.assign(b, place(i)));
tView = -9;
requestAnimationFrame(frame);
})();
