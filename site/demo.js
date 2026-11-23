// The home page's screen: the launcher's home drawn by preview.js, with a theme and a view to pick.
(() => {
  const P = window.NULDPreview, themes = document.querySelector('[data-themes]'), views = document.querySelector('[data-views]');
  if (!P || !themes) return;
  const NAMES = { torres: 'Torres', nuld: 'TEa.G3', cristal: 'Cristal', aurora: 'Aurora', liquid: 'Liquid', onda: 'Onda', orbit: 'Orbit',
    techstep: 'Techstep', wireframe: 'Wireframe', neuro: 'Neuro', fosforo: 'Fósforo', papel: 'Papel' };
  // each theme's palette (themes.c's table: bg, panel, row, edge, dim, text, text2, label, accent, steel), for the page itself
  const PAL = { torres: '01030F 050C2E 16296B 4962A5 3A4A80 E8EEFF C3D0F5 A6B8EA 8FD8FF 13235A', nuld: '06070A 0C0E12 161A21 2A303A 3A4150 E6E8EC A9B0BA 8A919C C8FF2E 1A1D23',
    cristal: '0B0A14 15122A 2E2A48 6A6880 4A4860 FFFFFF EFECF6 DAD5E8 FFB454 26233A', aurora: '03050F 080C1A 18233F 3A4058 39405C EEF2FF CBD2EC AEB7D6 7CF5D6 121A30',
    liquid: '050E24 1A1440 3A3470 9A94C0 6A6590 FFFFFF F0EDFA D6D6F0 7FF0E0 3A3560', onda: '101622 0A0E18 2A3244 8A90A0 5A6070 FFFFFF EEF1F8 D5DBEA FFFFFF 303848',
    orbit: '04060E 0D1736 1B3050 3C6A8C 3A4670 EEF3FF C4CFEA A9B6D3 7FE7FF 16224A', techstep: '0A0A0A 0A0A0A 242420 F4F4F0 6A6A62 F4F4F0 F4F4F0 B8B8B0 FFD60A 2A2A26',
    wireframe: '00030A 00080E 0A2A34 1C7384 2A5560 E8FBFF B6E6F0 8CC7D4 39E6FF 0A2630', neuro: '0B0A0A 131211 1D1A18 34302C 4A4540 F2EFEA C2BCB3 9A948B FF6224 24201D',
    fosforo: '010703 021408 0E3A1C 2A8046 1E5A30 6BFF96 4FE07C 3DB965 6BFF96 0A2A14', papel: 'F1EEE7 FBFAF6 FFFFFF CFC9BD B0AA9E 1B1A17 45423B 666158 B4442C E4DFD4' };
  const VARS = ['bg', 'panel', null, 'edge', 'edge2', 'fg', 'text2', 'mist', 'acid', 'steel'];
  function paint(id) { // the page takes the theme's colours
    PAL[id].split(' ').forEach((c, i) => { if (VARS[i]) document.documentElement.style.setProperty('--' + VARS[i], '#' + c); });
    document.querySelector('meta[name=theme-color]').content = '#' + PAL[id].slice(0, 6);
  }
  const press = (box, b) => box.querySelectorAll('button').forEach(x => x.setAttribute('aria-pressed', x === b));
  for (const id in NAMES) {
    const b = document.createElement('button');
    b.type = 'button', b.textContent = NAMES[id];
    b.onclick = () => { P.set(id, {}, {}, null, false); press(themes, b); paint(id); };
    themes.append(b);
  }
  ['Carousel', 'Grid', 'List'].forEach((n, v) => {
    const b = document.createElement('button');
    b.type = 'button', b.textContent = n;
    b.onclick = () => { P.view(v); press(views, b); };
    views.append(b);
  });
  P.tour(v => press(views, views.children[v])); // left alone, it goes through the games and the views
  const at = Object.keys(NAMES).indexOf(location.hash.slice(1)); // index.html#papel opens in that theme
  themes.children[at < 0 ? 0 : at].click(), press(views, views.firstChild);
  document.querySelector('[data-splash]').onclick = () => P.show('splash');
  document.querySelector('[data-launch]').onclick = () => P.show('launch');
})();
