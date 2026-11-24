#pragma once
// TEa.G3 supports two interface languages: Arabic (primary/default) and English (secondary).
// config.ini [ui] idioma accepts only "ar" or "en". Arabic is used before config is loaded.
enum { LANG_EN, LANG_AR, LANG_N };
extern int lang;
const char *lang_ar(const char *en);
#define L(en) (lang == LANG_AR ? lang_ar(en) : (en))
