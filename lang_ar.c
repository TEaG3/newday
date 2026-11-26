// Arabic translations for the English source strings used by L("English text").
#include <string.h>
#include "lang.h"

static const char *const ar[][2] = {
        {"DISC", "قرص"},
        {"SHARED VIRTUAL", "افتراضي مشترك"},
        {"PER-GAME VIRTUAL", "افتراضي لكل لعبة"},
        {"MMCE memory card", "بطاقة ذاكرة MMCE"},
        {"SWITCHES TO THE GAME'S ON PLAY", "يبدل إلى بطاقة اللعبة عند التشغيل"},
        {"Virtual memory card", "بطاقة ذاكرة افتراضية"},
        {"ON THE SERVER", "على الخادم"},
        {"CREATED ON FIRST PLAY (8 MB)", "تنشأ عند أول تشغيل (8 ميغابايت)"},
        {"No memory card", "لا توجد بطاقة ذاكرة"},
        {"INSERT ONE IN MC1 / MC2", "أدخل واحدة في MC1 / MC2"},
        {"No saves", "لا توجد حفظات"},
        {"NONE IN MC1 · MC2", "لا يوجد في MC1 · MC2"},
        {"SHARED", "مشترك"},
        {"network drivers", "برامج تشغيل الشبكة"},
        {"cable not detected", "لم يكتشف الكابل"},
        {"no DHCP answer", "لا استجابة من DHCP"},
        {"connection failed", "فشل الاتصال"},
        {"bad response", "استجابة غير صالحة"},
        {"the adapter belongs to UDPBD", "المحول مخصص ل UDPBD"},
        
        
        {"OPL runtime failed its integrity check", "فشل التحقق من سلامة محرك OPL"},
        {"Invalid game selection", "اختيار اللعبة غير صالح"},
        {"Invalid game record", "سجل اللعبة غير صالح"},
        {"Game source is no longer available", "مصدر اللعبة لم يعد متاحا"},
        {"The game image is no longer readable from its source", "تعذرت قراءة ملف اللعبة من مصدر التخزين"},
        
        
        {"STARTING GAME", "جار تشغيل اللعبة"},
        {"No network (%s): the server's game catalog was not read", "لا توجد شبكة (%s): لم تتم قراءة كتالوج ألعاب الخادم"},
        {"The catalog at %s:18290 did not answer (%s): is the TEa.G3 catalog service running? (none: empty servidor)",
         "الكتالوج على %s:18290 لم يستجب (%s): هل تعمل خدمة فهرسة TEa.G3؟ (بدونه: خادم فارغ)"},
        {"No disc", "لا يوجد قرص"},
        {"Reading disc...", "جاري قراءة القرص..."},
        {"Unsupported disc", "قرص غير مدعوم"},
        {"PS2 disc", "قرص PS2"},
        {"PS1 disc", "قرص PS1"},
        {"Unknown game source in config.ini: using USB", "مصدر ألعاب غير معروف في config.ini: استخدام USB"},
        {"Default (%s)", "افتراضي (%s)"},
        {"Physical (HD Loader)", "مادي (HD Loader)"},
        {"Yes", "نعم"},
        {"No", "لا"},
        {"Video", "الفيديو"},
        {"Starting USB", "بدء USB"},
        {"Reading memory cards", "قراءة بطاقات الذاكرة"},
        {"Loading games  %02d / %02d", "تحميل الألعاب  %02d / %02d"},
        
        {"Video compatibility", "توافق الفيديو"},
        {"saved", "تم الحفظ"},
        
        {"ERROR saving", "خطأ في الحفظ"},
        {"menu + TEa.G3", "القائمة + TEa.G3"},
        {"%s: downloading...", "%s: جاري التنزيل..."},
        {"%s: up to date", "%s: محدث"},
        {"%s: writing...", "%s: جاري الكتابة..."},
        {"downloaded and checked", "تم التنزيل والتحقق"},
        {"download ERROR", "خطأ في التنزيل"},
        {"ERROR: does not match", "خطأ: غير متطابق"},
        {"%s: install ERROR (%s.old kept)", "%s: خطأ في التثبيت (%s.old محفوظ)"},
        {"TEa.G3 %s installed", "تم تثبيت TEa.G3 %s"},
        {"Not updated", "غير محدث"},
        
        
        {"Physical", "مادي"},
        {"Native", "أصلي"},
        {"Reading...", "جاري القراءة..."},
        {"FreeMcBoot not found", "لم يتم العثور على FreeMcBoot"},
        {"No change (auto: %s)", "لا تغيير (تلقائي: %s)"},
        {"Autoboot", "الإقلاع التلقائي"},
        {"Button %s", "الزر %s"},
        {"Item in FMCB's menu", "عنصر في قائمة FMCB"},
        {"Update TEa.G3 to %s", "تحديث TEa.G3 إلى %s"},
        {"X: close", "X: إغلاق"},
        {"Setup saved", "تم حفظ الإعدادات"},
        {"TEa.G3 setup", "إعدادات TEa.G3"},
        {"X: restart TEa.G3", "X: إعادة تشغيل TEa.G3"},
        {"That button now runs: %s", "هذا الزر الآن يشغل: %s"},
        {"Could not read the list: the xeRAbora client must be reachable on the local network",
         "تعذرت قراءة القائمة: لا يمكن الوصول إلى عميل xeRAbora عبر الشبكة المحلية"},
        {"Loading achievements...", "جاري تحميل الإنجازات..."},
        {"READING...", "جاري القراءة..."},
        {"DRIVE", "محرك"},
        {"ACHIEVEMENTS %d", "الإنجازات %d"},
        {"ACHIEVEMENTS", "الإنجازات"},
        {"Close", "إغلاق"},
        {"Change", "تغيير"},
        {"Save", "حفظ"},
        {"Data", "بيانات"},
        {"View", "عرض"},
        
        {"Achievements", "الإنجازات"},
        {"Play", "تشغيل"},
        {"SHOW  %s", "إظهار  %s"},
        {"VIEW  %s", "عرض  %s"},
        {"COVERS: CONNECTING TO THE NETWORK", "الأغلفة: جاري الاتصال بالشبكة"},
        {"COVERS: DOWNLOADING  %d / %d", "الأغلفة: جاري التنزيل  %d / %d"},
        {"COVERS: %d NEW OF %d", "الأغلفة: %d جديدة من %d"},
        {"COVERS: NO NETWORK LINK (CABLE?)", "الأغلفة: لا يوجد اتصال شبكة (الكابل؟)"},
        {"COVERS: NO IP ADDRESS (DHCP)", "الأغلفة: لا يوجد عنوان IP (DHCP)"},
        {"COVERS: NO DNS (INTERNET?)", "الأغلفة: لا يوجد DNS (هل هناك إنترنت؟)"},
        {"COVERS: GITHUB UNREACHABLE", "الأغلفة: GitHub غير قابل للوصول"},
        {"COVERS: TLS ERROR", "الأغلفة: خطأ TLS"},
        {"COVERS: THE GAMES USE THE NETWORK (UDP)", "الأغلفة: الشبكة مشغولة بالألعاب (UDP)"},
        {"COVERS: NETWORK ERROR (%d)", "الأغلفة: خطأ شبكة (%d)"},
        {"%-7s OK, %d games\n", "%-7s OK، %d لعبة\n"},
        {"%-7s not found\n", "%-7s غير موجود\n"},
        {"NETWORK not used\n", "الشبكة  غير مستخدمة\n"},
        {"NETWORK %s\n", "الشبكة  %s\n"},
        {"NETWORK %s (%s)\n", "الشبكة  %s (%s)\n"},
        {"fixed", "ثابت"},
        {"POPSTARTER.ELF and POPS_IOX.PAK missing in mass0:/POPS/", "POPSTARTER.ELF و POPS_IOX.PAK مفقودان في mass0:/POPS/"},
        {"No disc ready", "لا يوجد قرص جاهز"},
        {"This disc cannot be started", "لا يمكن بدء تشغيل هذا القرص"},
        
        {"Covers still downloading: try again", "الأغلفة لا تزال قيد التنزيل: حاول مرة أخرى"},
        {"Not enough memory to download the app", "لا توجد ذاكرة كافية لتنزيل التطبيق"},
        {"Could not write %s", "تعذرت كتابة %s"},
        {"The server did not send the app (%s)", "الخادم لم يرسل التطبيق (%s)"},
        
        
        
        
        
        
        
        
        {"%s  (SELECT: status)", "%s  (SELECT: الحالة)"},
        {"  NO USB", "  لا USB"},
        {"TEa.G3 boot stub installed: BOOT/TEa.G3.ELF", "تم تثبيت ملف إقلاع TEa.G3 في بطاقة الذاكرة: BOOT/TEa.G3.ELF"},
        {"TEa.G3 %s available: START to update", "يتوفر TEa.G3 %s: اضغط START للتحديث"},
        {"Achievements: no network", "الإنجازات: لا شبكة"},
        {"Achievements: no server", "الإنجازات: لا خادم"},
        
        {"Could not start", "تعذر البدء"},
        {"HD Loader partition metadata is incomplete", "بيانات قسم HD Loader غير مكتملة"},
        {"  (SAVED TO USB)", "  (تم الحفظ على USB)"},
        {"  (SAVE ERROR)", "  (خطأ في الحفظ)"},
        
        
        
        
        
        
        {"WINDOW %u (%d FRAMES): MEDIAN %u us  MAX %u us  MISSED VSYNC %u%s\nGOAL: MAX <= 8333 us AND 0 MISSED.  LOAD: %d COVERS IN %d ms\nHANDS OFF FOR 5 s, THE SELECTION MOVES BY ITSELF", "النافذة %u (%d إطارا): الوسيط %u us  الحد الأقصى %u us  تجاوزات المزامنة %u%s\nالهدف: الحد الأقصى <= 8333 us دون تجاوز.  تحميل %d غلافا خلال %d ms\nلا تلمس الأزرار لمدة 5 ثوان، سيتحرك التحديد تلقائيا"},
        
        {"A memory-card return stub is required before this launch", "يلزم تثبيت ملف العودة على بطاقة ذاكرة قبل تشغيل اللعبة"},
        {"Preparing the independent return path; try again shortly", "جار تجهيز مسار العودة المستقل، حاول مرة أخرى"},
		{"Could not load the hard disk driver", "تعذر تحميل برنامج تشغيل القرص الصلب"},
		
		{"USB drive not found", "لم يتم العثور على قرص USB"},
		
		
		{"MX4SIO card not found", "لم يتم العثور على بطاقة MX4SIO"},
		{"iLink disk not found", "لم يتم العثور على قرص iLink"},
		
		
		
		
		
		
		
		
        {"AV 480i/576i", "AV ‏480i/576i"},
        {"HD 720p", "عالي الدقة 720p"},
        {"Hard disk not found (FAT32 or HD Loader)", "لم يتم العثور على القرص الصلب (FAT32 أو HD Loader)"},
        {"MMCE game launching is not yet adapted to OPL", "تشغيل ألعاب MMCE غير مهيأ بعد للعمل مع OPL"},
        {"NETWORK legacy runtime's (UDPBD / UDPFS / SMB), IP %s\n", "مصادر الشبكة القديمة (UDPBD / UDPFS / SMB)، عنوان IP ‏%s\n"},
        {"No OPL adapter is available for this game source", "لا يوجد محول OPL متاح لمصدر اللعبة هذا"},
        {"OPL compatibility settings", "إعدادات توافق OPL"},
        {"SMB discovery is disabled until an OPL-only reader is integrated", "اكتشاف SMB معطل حتى دمج قارئ مستقل يعمل مع OPL"},
        {"This game format is not supported by the OPL adapter for this source", "تنسيق اللعبة هذا غير مدعوم عبر محول OPL لمصدر التخزين المحدد"},
        {"UDPBD game launching is not supported by OPL integration", "تشغيل الألعاب عبر UDPBD غير مدعوم في تكامل OPL الحالي"},
        {"UDPFS game streaming is not supported by the OPL-only launcher", "بث الألعاب عبر UDPFS غير مدعوم في مشغل OPL-only"},
        {"Background work is still active; try again", "العمل في الخلفية لم ينته بعد، حاول مرة أخرى"},
        {"Modified and developed by Iraqi developer TEa.G3", "تعديل وتطوير المطور العراقي TEa.G3"},
};

const char *lang_ar(const char *en)
{
        for (unsigned i = 0; i < sizeof(ar) / sizeof(ar[0]); i++)
                if (!strcmp(ar[i][0], en)) return ar[i][1];
        return en;
}
