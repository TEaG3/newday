#pragma once
// Interface language (phase 18): config.ini [ui] idioma = es | en | pt, read first by load_config. Each on-screen text
// is written L("español", "English") where it is used; Portuguese is looked up from the English one (lang_pt.c).
// Spanish until the config is read (the first splash frames). Tables of texts have one row per language, [lang].
enum { LANG_ES, LANG_EN, LANG_PT, LANG_AR, LANG_N };
extern int lang;
const char *lang_pt(const char *en);
#define L(es, en) (lang == LANG_EN ? (en) : lang == LANG_PT ? lang_pt(en) : (es))
