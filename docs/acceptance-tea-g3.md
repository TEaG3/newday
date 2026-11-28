# TEa.G3 — سجل القبول والحالة الحالية

> **OPL هو محرك ألعاب PS2 الوحيد في TEa.G3.** لا تتضمن النسخة الحالية محرك Neutrino أو ملفه التنفيذي أو محمّله أو وحدات IOP/EE الخاصة به. ما بقي تحت `ui/neutrino-igr/` هو مرجع تصميم وأصول عربية فقط، وغير مربوط أو نشط.
>
> نجاح CI يثبت البناء والاختبارات الآلية المحددة فقط؛ ولا يثبت إقلاع لعبة أو استمرار قراءة قطاعاتها على جهاز PlayStation 2 حقيقي.

## 0. أحدث تحقق بعد PR #188–#191 — 28 نوفمبر 2026

- [x] رأس `master` الحالي: `bb75b745ef331c208ef4c9ad855bfd40a3d1779e`، بعد دمج PR #191. PR #29 ما زال مدمجاً بالالتزام `c99ac8695293cf6eaeeb9d2a51247748a0c000db`، والفرع `safety/io-and-runtime-guards` غير موجود.
- [x] PR #188 ربط مسار candidate مستقل بـ launcher: Snapshot/Diff، ترحيل typed sidecars، حفظ الاختيار بالـ stable key، إعادة بناء `pos/order` و`shown_box/wave`، والتبديل على render thread عند نقطة آمنة بين الإطارات.
- [x] PR #189 نظّف نصوص الهوية النشطة. فحص README والصفحات النشطة (الرئيسية والدليل وOPL Lab) لم يجد اسم NULD أو ORBIT؛ بقيت إشارات ORBIT القديمة ومسارات `nuld/` في سجل changelog التاريخي ونصوص نقل الإعدادات، وهي توثيق/توافق وليست هوية العرض الحالية. لا تمحُ التاريخ أو مسارات الترحيل.
- [x] PR #190 جعل فشل retirement قابلاً للرجوع: إذا فشل تحرير sidecars بعد تبديل الحزمة، يعيد الناشر الجيل القديم إلى active قبل فتح بوابة القراء. اختبارات الرجوع وفشل semaphore موجودة.
- [x] PR #191 حدّث سجل القبول بعد #190. هذا القسم هو أحدث ملخص؛ لا تعتمد على الفقرات التاريخية أدناه إذا تعارضت مع هذه الحالة الحالية.
- [x] أحدث بناء على `master`: [PS2 launcher build #46084717348](https://github.com/TEaG3/newday/actions/runs/46084717348) — الوظائف الأربع نجحت: ZSO host decoder fixture، بناء OPL من المصدر المثبّت، بناء TEa.G3 launcher، وتجميع حزمة التكامل.
- [x] أحدث CodeQL: [#46084717341](https://github.com/TEaG3/newday/actions/runs/46084717341) — تحليلات C/C++ وPython وJavaScript/TypeScript الثلاث نجحت.
- [x] سجل المضيف: `Ran 114 tests` ثم `OK`. نجحت ZSO raw/compressed وZSO EE scanner، وHDL/APA scanner، وsource-root probe، والتحقق من ELF payload، وcatalog stage وPS2 semaphore host concurrency، واختبارات ISO وUL الذاتية، ومصفوفة OPL-only ذات 12 تركيبة.
- [x] العربية وRTL في بناء OPL: `check_arabic_translation.py` سجل 326/326 سلسلة، 0 غير مترجمة، وRTL=1 مع الخط المضمن. فحص المشغّل أكد العربية لغة البدء والرجوع الافتراضية، تغطية 38 محرفاً عربياً، واختبار الخط 509 glyphs؛ CSS يفعّل `.tea-g3-rtl` في OPL Lab.
- [x] نُزّلت artifacts من run #46084717348 وفُكّت محلياً. نجح `unzip -t` لكل ZIP، وتطابقت SHA-256 المحسوبة محلياً مع GitHub artifact digests:
  - Integration bundle — [artifact 15239080002](https://github.com/TEaG3/newday/actions/runs/46084717348/artifacts/15239080002)، 17,791,473 بايت، SHA-256 `ea9771d7f2c5ab019ad0856aaf0083f329adcc9e54c10fbe6e99c7698312dcbb`.
  - Launcher artifact — [artifact 15238761980](https://github.com/TEaG3/newday/actions/runs/46084717348/artifacts/15238761980)، 4,290,393 بايت، SHA-256 `affcabb0165c5962af69ae6bdd570f97d6916fcfa0784b1fb3199c2a3ca81bd4`.
  - OPL runtime — [artifact 15237889788](https://github.com/TEaG3/newday/actions/runs/46084717348/artifacts/15237889788)، 2,539,789 بايت، SHA-256 `cccb186734ee5fd3c68e3b660cce2e2c596005c1082d3e49260b1d81321c4242`.
- [x] داخل integration bundle: `TEa.G3.ELF` حجمه 9,579,156 بايت، SHA-256 `48cefc8611803c39d386c8774ee3e16fa0dff721c055edde8b316d32b7f69420`؛ و`OPL.ELF` حجمه 5,792,496 بايت، SHA-256 `233089c24894e329d649142e70758ddfa44c458ab70ad6e88e17b839ad080e4f`. أعادت قراءة ELF تأكيد ELF32 little-endian MIPS executable.
- [x] حزمة المستخدم الداخلية `TEa.G3-Complete-Package.zip` حجمها 6,710,832 بايت، SHA-256 `15833c246258df8671af6ab69e6733b0d6c7c7a3d80d281c609bc05cef0289b6`. طابقت بصمة ELF الموجودة داخل الحزمة البصمة نفسها في integration bundle.
- [x] `runtime-manifest.txt` يعلن `files=1` و`OPL.ELF` فقط. أُعيد حساب CRC32 لمحرك OPL فكانت `aa490716`، مطابقة للبيان. لا يوجد Neutrino ELF أو محرك ألعاب ثانٍ في الحزمة.
- [x] مقطع `PT_LOAD` في ELF يطلب 11,982,620 بايت (11.43 MiB). بطاقة PS2 ذات 8MB هي مساحة تخزين وليست RAM؛ ذاكرة PS2 الرئيسية 32MiB. هذا قياس تحميل ELF وليس قياس استهلاك الذاكرة أثناء اللعب.
- [ ] `CATALOG_LIVE_MONITOR_ENABLED=0` عمداً. مسار candidate/apply وsource polling وإلغاء المسح موجودة ومختبرة على المضيف، لكن لم تُجرَ اختبارات USB removal/reinsert ودورة حياة semaphore وIGR على PS2 حقيقي؛ لذلك لا تفعّل المراقبة.
- [ ] USB ISO/ZSO وUSBExtreme UL اجتازت اختبارات آلية فقط؛ HDD/APA اجتازت اختبارات مضيف فقط. SMB وUDPBD وUDPFS وMMCE كمصادر ألعاب ما زالت معطلة وتفشل مغلقاً. لا توجد بيئة PS2 أو محاكي موثوق هنا لإثبات إقلاع لعبة أو استمرار قراءة القطاعات.
- [ ] PRs #93 و#72 و#6 ما زالت Draft مفتوحة وقديمة؛ لم تُدمج ولم تُستخدم كبديل عن العمل المدمج في master.
- [ ] بيانات المستودع ما زالت تعرض Description باسم `NULD (formerly ORBIT)` وHomepage القديم `https://danielnuld.github.io/nuld/`. فرع `master` يظهر `protected=false`، وواجهة rulesets أعادت قائمة فارغة؛ لكن تفاصيل branch-protection endpoint نفسها ترجع 403. أدوات GitHub المتاحة هنا لا تتضمن تحديث metadata أو حماية الفرع؛ يلزم تعديل الوصف وHomepage والحماية من Settings.
- [ ] آخر Release منشور ما زال `arabic-v1.3.13`. لم يُنشأ Release جديد ولم يُحوّل زر التنزيل إلى artifact مؤقت. ملف `site/release.js` يختار أحدث Release منشور يحتوي الحزمة المتوقعة، بينما fallback الثابت ما زال يشير إلى `arabic-v1.3.13`.

**المرحلة التالية:** اختبارات العتاد الفعلية فقط هي التي تفصل بين جاهزية البناء وقبول التشغيل. إبقاء المراقبة الحية ومصادر الشبكة معطلة هو السلوك الآمن إلى أن تتوفر سجلات PS2 موثوقة.

## سجل تاريخي: التحقق بعد PR #185 وPR #186 — 27 نوفمبر 2026

- [x] `master` الحالي هو `7b9e3c2813370f02525633d6f3f6ae355724bbfd`، بعد دمج PR #185 بالالتزام `9506e69fa7ae553faaecdcdab43ce260cd11d6a5` وPR #186 بالالتزام `7b9e3c2813370f02525633d6f3f6ae355724bbfd`.
- [x] PR #185 **net: make network initialization cancellation-aware**: أضاف `net_up_cancelable` مع إبقاء `net_up` غلاف توافق؛ يستخدم EE semaphore من PS2SDK لتسلسل التهيئة والإيقاف، ويفحص الإلغاء بين استدعاءات تحميل الوحدات غير القابلة للمقاطعة، ويراقب Link وDHCP كل 20ms بمهلة 10 ثوانٍ، ويتراجع عن حالة `ps2ip/netman` عند الإلغاء أو انتهاء المهلة. لا تزال اختبارات دورة حياة semaphore والشبكة على جهاز PS2 مطلوبة.
- [x] سبب فشل محاولة الربط الأولى في run `46059449682` كان الرمز غير المدعوم `__sync_lock_test_and_set_4` في هدف MIPS. استُبدل بقفل PS2SDK semaphore؛ ثم نجح بناء PR #185 ونجح التحقق بعد الدمج.
- [x] PR #186 **package: include complete Arabic and English readmes**: استبدل README العربي القصير بالدليل الكامل، وأضاف README إنجليزياً كاملاً، وأبقى دليلي التثبيت. أضاف اختباراً يمنع عودة README القصير أو فقدان إحدى اللغتين.
- [x] PR #29 مدمج بالالتزام `c99ac8695293cf6eaeeb9d2a51247748a0c000db`؛ الفرع `safety/io-and-runtime-guards` لم يعد موجوداً.
- [x] أحدث بناء `master`: [PS2 launcher build #46060578848](https://github.com/TEaG3/newday/actions/runs/46060578848) — الوظائف الأربع نجحت: بناء OPL من المصدر المثبّت، ZSO host fixture، بناء TEa.G3.ELF، وتجميع حزمة التكامل.
- [x] أحدث CodeQL: [#46060578896](https://github.com/TEaG3/newday/actions/runs/46060578896) — تحليلات C/C++ وPython وJavaScript/TypeScript الثلاث نجحت.
- [x] اختبارات المضيف في بناء master: `Ran 109 tests` ثم `OK`. نجحت اختبارات ZSO الخام/المضغوطة وحدود القراءة، ZSO EE scanner adapter، HDL/APA host scanner، source-root probe، catalog snapshot/diff وsidecar migration والتزامن، واختبارات سياسة OPL وتسليم التشغيل والحزمة.
- [x] داخل بناء المشغّل نجحت `make test` واختبارات الخط (509 glyphs)، وظهر في السجل `net selftest ok` و`iso selftest ok` و`ul selftest ok` و`OPL-only engine decision selftest ok`، إضافة إلى اختبارات gfx وini وVMC وFMCB وruntime installer وcover. رسالة رفض ملف staging الموجود مسبقاً من runtime installer جزء من اختبار سلبي يحافظ على الملف، وليست فشل CI؛ انتهى الاختبار بنجاح.
- [x] فحص العربية: اللغة العربية افتراضية، 38 محرفاً عربياً مستخدماً مغطاة في glyph atlas، وصفوف العربية موجودة في جداول اللغة، واختبار الخط 509 glyphs نجح. فحص OPL Lab يستخدم `.tea-g3-rtl` في CSS، والواجهة تضبط الفئة نفسها؛ شارة المطور العربية ظاهرة.
- [x] أحدث نشر موقع متاح: [Website #46045579764](https://github.com/TEaG3/newday/actions/runs/46045579764) نجح في التحقق من JavaScript ومصفوفة محاكي OPL والروابط والرسوم ثم نشر GitHub Pages. لم تتغير ملفات الموقع بين commit النشر `f9e4a6039194` و`master` الحالي؛ لذلك رابط التنزيل الحالي ما زال صالحاً ولا يحتاج إعادة نشر بسبب PR #185/#186.

### Artifacts التي نُزّلت وفُحصت من بناء master

- [x] Integration bundle — [artifact 15229146425](https://github.com/TEaG3/newday/actions/runs/46060578848/artifacts/15229146425)، 17,789,205 بايت، SHA-256 `59de06c4e22d69804e0df202479dd94fac7c4062dcf4f74b2edd9c45d991bae3`.
- [x] TEa.G3 launcher — [artifact 15228198613](https://github.com/TEaG3/newday/actions/runs/46060578848/artifacts/15228198613)، 4,289,552 بايت، SHA-256 `e9eb5369c3f85cef424617fec94f0e49fecc10ab71f5f9cc275c9191fe074e77`.
- [x] OPL runtime — [artifact 15228103769](https://github.com/TEaG3/newday/actions/runs/46060578848/artifacts/15228103769)، 2,539,789 بايت، SHA-256 `e69423b4a8e3a900f236536f940d2d98955a9d6d70ef50c24c148ca0bf85ffaa`.
- [x] Build log — [artifact 15228118924](https://github.com/TEaG3/newday/actions/runs/46060578848/artifacts/15228118924)، 1,864 بايت، SHA-256 `92780e7463c46061d10293d09ac62e1d513cb336b58274be44ef1698c155e087`.
- [x] نُزّلت الحزم الثلاث وفُحصت محلياً: `unzip -t` نجح، وبصمات ZIP تطابق GitHub artifact digests. ملف `TEa.G3.ELF` حجمه 9,575,924 بايت وبصمته `110cf423e0a4a604f2ce7bb7d3d71f15438655db7dad01bc8ecd86445bd34138`. ملف `OPL.ELF` حجمه 5,792,496 بايت وبصمته `233089c24894e329d649142e70758ddfa44c458ab70ad6e88e17b839ad080e4f`.
- [x] `runtime-manifest.txt` يعلن `files=1` و`OPL.ELF` فقط، بحجم 5,792,496 بايت وCRC32 `aa490716`. لا يوجد `neutrino.elf` ولا محرك PS2 ثانٍ.
- [x] حزمة المستخدم الداخلية `TEa.G3-Complete-Package.zip` حجمها 6,710,406 بايت، SHA-256 `1a5a4a7b819402b024763436dc3534667536e2640d32420d6dfc3180945dba76`؛ وفحص ZIP الداخلي نجح. تحتوي `README-AR.md` (4,251 بايت) و`README-EN.md` (3,515 بايت) ودليلي التثبيت والترخيص وملف ELF ومجلدات USB الإرشادية. `SHA256SUMS.txt` داخل حزمة الاختبار أكد تطابق بصمة الحزمة وبصمة ELF.
- [x] مقطع ELF من نوع `PT_LOAD` يحمّل `11,325,084` بايت تقريباً `10.80 MiB`. بطاقة PS2 ذات 8MB هي مساحة تخزين وليست RAM؛ ذاكرة PS2 الرئيسية 32MiB. حجم ELF على القرص ليس مقدار RAM الكلي المستهلك أثناء التشغيل.

### الحالة التي لم تكتمل

- [ ] **المراقبة الحية غير مفعّلة.** ما زال يلزم إنشاء candidate مستقل لدورة المسح الحي وإغلاق/إتلاف موارده عند الإلغاء، ثم ترحيل typed sidecars وإعادة بناء caches العرض (`shown_box/wave/pos/order`) والتبديل الذري للمؤشرات والعدد على render thread بين الإطارات. وجود primitives واختبارات host لا يعني أن هذا المسار موصول بالمشغّل.
- [ ] يلزم source polling مع mount checks خاصة بكل مصدر، وإلغاء متعاون، واختبارات USB removal/reinsert وHDD/APA وSMB وMMCE. لا تفترض وجود hot-plug event API.
- [ ] يلزم اختبار دورة حياة semaphore وتهيئة الشبكة وTCP connect/send على جهاز PS2. لا توجد بيئة جهاز أو محاكي موثوق متاحة في هذا التشغيل لإثبات إقلاع ألعاب ISO/ZSO/UL أو استمرار القراءة.
- [ ] ISO/ZSO وUSBExtreme UL اجتازت اختبارات آلية فقط؛ HDD/APA اجتاز اختبارات مضيف فقط. SMB وUDPBD وUDPFS لبث الألعاب وMMCE كمصدر ألعاب ما زالت معطلة وتفشل مغلقاً. لا تُعرض كميزات مدعومة.
- [ ] PRs المفتوحة #93 و#72 و#6 ما زالت Draft ومتباعدة عن master (على الترتيب 268 و287 و812 التزاماً خلف master في المقارنة). لم تُدمج؛ لا تستخدمها كمسار دمج دون إعادة تأسيس ومراجعة وفحوص جديدة.
- [ ] صفحات README والتعليمات والصفحات النشطة التي فُحصت خالية من NULD/ORBIT. بقيت أسماء داخلية مثل `tools/nuld_catalog.py` و`tests/test_nuld_catalog.py`، وحقول/مسارات ترحيل `nuld/orbit` وملفات إقلاع `NULD.ELF`/`ORBIT.ELF` لدعم الإعدادات وFreeMcBoot القديمة، إضافة إلى سجل changelog التاريخي. هذه ليست هوية العرض الحالية؛ لا تحذفها قبل مسار ترحيل آمن.
- [ ] بيانات GitHub الوصفية ما زالت تعرض Description باسم `NULD (formerly ORBIT)` وHomepage القديم `https://danielnuld.github.io/nuld/`. لم تتوفر عملية تعديل بيانات المستودع عبر وصلة GitHub الحالية. قائمة rulesets رجعت فارغة، لكن قراءة branch protection أعادت 403؛ لذلك لا يمكن تأكيد حالة حماية `master` من هذه الوصلة.
- [ ] أحدث Release منشور ما زال `arabic-v1.3.13`. زر الموقع في `site/index.html` و`site/manual.html` و`site/opl-lab.html` يشير إلى الحزمة المنشورة نفسها. لم يُنشأ Release جديد ولم تُغيّر روابط التنزيل؛ artifacts الحديثة أعلاه مؤقتة من CI وليست Release.

**المرحلة التالية:** إكمال دورة حياة candidate/apply وربط sidecars وrender caches عند نقطة آمنة، مع إبقاء المراقبة معطلة حتى اجتياز اختبارات المضيف والهدف. تبقى اختبارات PS2 الفعلية شرطاً لأي ادعاء بتشغيل الألعاب أو تفعيل مصادر العتاد.

## 1. سجل أدلة سابق — 27 نوفمبر 2026 (master: 5d6c189)

### الإصدار المنشور arabic-v1.3.13

- [x] GitHub Release `arabic-v1.3.13` منشور وغير مسودة/تجريبي وفق سجل PR #138. أصل الحزمة `TEa.G3-Complete-Package.zip` حجمه 6,702,711 بايت؛ سجل GitHub Release في PR #138 أبلغ عن SHA-256: `a6b8b47dcc38cd6058e8255166bd3da0a9be84dddaaf96ada6478b6cb2ec1696`. لم يُعد تنزيل أصل Release نفسه في هذا الفحص؛ هذه البصمة من سجل النشر المذكور.
- [x] روابط fallback في `site/index.html` و`site/manual.html` و`site/opl-lab.html` تشير جميعها إلى `arabic-v1.3.13/TEa.G3-Complete-Package.zip`.
- [x] نُشر GitHub Pages بعد دمج PR #138: run [45912445005](https://github.com/TEaG3/newday/actions/runs/45912445005) على master commit `a49ba8aa985b30838d6a977e2608224cd78046b9`. نجحت وظيفتا التحقق والنشر. artifact `15160756772` حجمه 2,616,379 بايت وبصمته `1ac7d8500e100f81a8563f16f9f3bed81fcc915d7ac27aa90e07578c6d834661`; نُزّل وفُك وفُحص، واحتوى الرابط المنشور الصحيح. الصفحات النشطة index/manual/opl-lab لم تحتوِ على NULD أو ORBIT أو danielnuld أو nuld_catalog.
- [x] دُمج PR #139 في master بالالتزام `176e770037d5853bf285259004b4021ad9e31144`. التغيير توثيقي ويحدّث README وملاحظات الإصدار المستقبلية؛ لا يغيّر ELF أو محرك OPL.
- [x] دُمج PR #140 في master بالالتزام `5d6c189ab8661b8f40824193842b322ec6061cb4`: أضاف `items_capacity` و`sidecars_capacity` إلى bundle، ورفض أي نشر يتجاوز السعة قبل تبديل المؤشرات، مع اختبارات للسعة الناقصة في active وcandidate. لم يربط هذا التغيير المسح الحي بالمشغّل.
- [x] تحقق CI مستقل بعد الدمج على master commit `5d6c189ab8661b8f40824193842b322ec6061cb4`: [PS2 launcher build #45919755516](https://github.com/TEaG3/newday/actions/runs/45919755516) نجح في الوظائف الأربع، و[CodeQL #45919755580](https://github.com/TEaG3/newday/actions/runs/45919755580) نجح في C/C++ وPython وJavaScript/TypeScript.
- [x] سجل host tests في run #45912473143: `Ran 77 tests`; نجح ZSO host fixture وZSO EE scanner adapter، واختبارات catalog stage وsidecar migration وselection وconcurrent apply وdownload gates وPS2 semaphore adapter، ومصفوفة OPL-only ذات 12 تركيبة.
- [x] سجل بناء المشغّل في job `140224251719`: `iso selftest ok` و`ul selftest ok`، إضافة إلى اختبارات font وgraphics وengine وnetwork وVMC وruntime installer وcover.
- [x] بناء OPL من المصدر المثبّت نجح في job `140223873935`. فحص التعريب سجل 326/326 سلسلة عربية، 0 غير مترجمة، دون إدخالات إنجليزية فقط، مع حفظ placeholders ووجود الخط العربي؛ البناء استخدم `RTL=1` و`PADEMU=1`.
- [x] Artifacts من تشغيل master بعد الدمج، غير منتهية الصلاحية وقت الفحص:
  - Integration bundle — artifact `15164440995`, 17,763,263 بايت، SHA-256 `5ce139e809ac560ddf5281482bc1349f8e00e69bf08b74e9080508fd74b2af24`.
  - TEa.G3 launcher — artifact `15164281321`, 4,280,329 بايت، SHA-256 `63c7234d6a6a4dfea1a60311f4320611b6a8899fe42ebac7db355c75d56b2737`.
  - OPL runtime — artifact `15163912195`, 2,539,789 بايت، SHA-256 `ddd97bf378da3d515abc5b96fbaea79d5afe89d250d6ba2a586204e36c5d801b`.
  - Build log — artifact `15164281325`, 1,836 بايت، SHA-256 `2c544c591f6ca7fc5b4f73a897850dd91ed565559db27f5406bcbef73ff74cac`.
- [x] نُزّلت artifacts الخاصة بتشغيل master وفُكّت محلياً؛ ZIP integrity ناجح وبصمات الأرشيفات الأربع مطابقة لـ GitHub. يحتوي integration bundle على `TEa.G3.ELF` و`OPL.ELF` بصيغة ELF 32-bit MIPS N32. `SHA256SUMS.txt` الداخلي أكد `TEa.G3-Complete-Package.zip` و`TEa.G3.ELF` (`OK`). بيان runtime يعلن `files=1` وOPL فقط؛ حجم `OPL.ELF` هو 5,792,496 بايت وCRC32 هو `aa490716` مطابق للبيان.
- [x] Neutrino ليس محرك تشغيل في هذه المعمارية: اختبار `tests/test_neutrino_ui_reference.py` يثبت `runtime_linked=false` و`game_engine=OPL` وعدم بناء أو ربط `neutrino.elf`. بقي مرجع الواجهة والأصول العربية فقط.
- [x] PR #29 مدمج بالالتزام `c99ac8695293cf6eaeeb9d2a51247748a0c000db`؛ فرع `safety/io-and-runtime-guards` لم يعد موجوداً.
- [x] بقايا Nuld في المسارات القديمة وملفات الترحيل وأجزاء changelog التاريخية محفوظة للتوافق؛ لم تُحذف. صفحات الموقع النشطة المنشورة خالية من تلك الأسماء.
- [ ] لا يثبت أي من الأدلة أعلاه إقلاع لعبة أو القراءة المستمرة على جهاز PS2. ISO/ZSO/UL اجتازت اختبارات المضيف فقط؛ HDD/APA وMX4SIO وiLink تحتاج اختبار عتاد. SMB/UDPBD/UDPFS وMMCE كمصادر ألعاب تبقى معطّلة وتفشل مغلقاً.

**حدود هذه الأدلة:** نجاح البناء والاختبارات الآلية لا يثبت إقلاع الألعاب أو استمرار قراءة القطاعات على جهاز PlayStation 2 حقيقي. لا توجد في هذا السجل أدلة تشغيل فعلية على جهاز PS2 أو محاكي موثوق.

### سجل تاريخي: arabic-v1.3.12

#### الإصدار المنشور arabic-v1.3.12

- [x] الإصدار منشور وغير مسودة وغير تجريبي منذ 2026-11-26 17:05:32 UTC: [arabic-v1.3.12](https://github.com/TEaG3/newday/releases/tag/arabic-v1.3.12).
- [x] الحزمة `TEa.G3-Complete-Package.zip`: 6,701,973 بايت، وبصمة GitHub SHA-256: `83ad0a3d8b633b534c5d6f107d5c13cf49f088404cbf3ba78faec99cd1a2e986`.
- [x] ملف `SHA256SUMS.txt`: 171 بايت، وبصمة GitHub SHA-256: `69784f8a39cfb03bfc50530fc2c7ef3b0fb4c6ee3c79afbc65526cffe3d836d8`.
- [x] وسم الإصدار يشير إلى commit `4c84e7e8471ab080833086e3909dfab9ffbea314`. مقارنة الوسم مع master الحالي لا تُظهر اختلافات في مصدر المشغّل أو محرك OPL؛ الاختلافات المسجلة تخص ملفات الموقع وسجل الإصدار والترجمة واختباراته.
- [x] دمج PR #127 لترجمة ملاحظات الإصدار العربية في master بالالتزام `5e762e281478e8f062d251f6474647ac0b122cc9`، بعد نجاح فحوص PR.
- [x] اكتمل CI على master للالتزام نفسه: [PS2 launcher build #45868145126](https://github.com/TEaG3/newday/actions/runs/45868145126)، [Website #45868144963](https://github.com/TEaG3/newday/actions/runs/45868144963)، و[CodeQL #45868145039](https://github.com/TEaG3/newday/actions/runs/45868145039) — جميعها `success`.
- [x] بناء PS2 أنشأ artifacts غير منتهية الصلاحية، مع بصمات SHA-256 المسجلة من GitHub:
  - OPL runtime — artifact `15140099701`, الحجم 2,539,789 بايت، digest `af88bb577a8a0b566959b710060aeaef1e86dc3179a76b99877be5f70bb5735d`.
  - TEa.G3 launcher — artifact `15142070275`, الحجم 4,279,459 بايت، digest `b753617ff2b881a0a60d0dce84fbf9a684f1b37cb0f2e2e4bdf7751fbc759e82`.
  - Integration test bundle — artifact `15140379941`, الحجم 13,485,203 بايت، digest `21ddd6bd44bc335aff18014180951dd8914f07c6305c9d9e911b1925426ff52c` (تنتهي صلاحيته في 2026-12-26).
  - Build log — artifact `15142170204`, الحجم 1,836 بايت، digest `467ab4d30bf9b5b6129070427eade22ea3f827ebe60ea4b678a5a755774fefeb`.
- [x] سجل الاختبارات في CI: `Ran 71 tests`؛ نجح ZSO host fixture (raw/compressed/header/bounds/I/O/allocation failures)، ومحول ZSO الخاص بـ EE، واختبارات catalog stage وPS2 semaphore adapter والتزامن، واختبارات ISO وUL واختيار OPL-only، وفحوص اللغة العربية والخط وRTL.
- [x] حزمة الاختبار المجمعة تحققت من وجود `TEa.G3.ELF` و`OPL.ELF` وبيان runtime، وسجلت `package_crc=PASS` و`release package fixture: PASS`.

**حدود هذه الأدلة:** نجاح البناء والاختبارات الآلية لا يثبت إقلاع الألعاب أو استمرار قراءة القطاعات على جهاز PS2 حقيقي. تبقى ISO/ZSO/UL وHDD/APA وغيرها غير معتمدة عتادياً حتى اختبارك الفعلي. SMB/UDPBD/UDPFS وMMCE كمصادر ألعاب تبقى معطّلة وتفشل مغلقاً.

### سجل تاريخي: v1.3.11 وما قبله

- [x] نُشر الإصدار التجريبي `arabic-v1.3.11` بنجاح في run 45840046904 من الالتزام `9f9cec61b33ce78fb768eaab53a068e5374fa63c`، ووسم Git يشير إلى الالتزام نفسه. حجم `TEa.G3-Complete-Package.zip` هو 6,701,894 بايت وبصمة SHA-256 هي `15abea06367f436498452e23baf5ecd08d4e420bb888833abda30e7b209357a9`. ملف `SHA256SUMS.txt` مرفوع وبصمته `sha256:678e77634b05e0827c7d5bcea61521b8e7b9e9f8e1fedba897a9636d6ca974c0`.

- [x] دُمج PR #29، الفرع safety/io-and-runtime-guards، في master بالالتزام c99ac8695293cf6eaeeb9d2a51247748a0c000db. حُذف الفرع بعد الدمج.
- [x] أُغلق PR #118 دون دمج بعد تضمين تحديث سجل القبول الأحدث في PR #120؛ لم تُدمج ملاحظة CI القديمة التي كانت تشير إلى أن اختبار snapshot ما زال بانتظار التشغيل.
- [x] نجح بناء master رقم 45841030508 على الالتزام 507370f0167af8d5a3afe1b087163734bfb44a4d؛ نجحت الوظائف الأربع: 69 اختبار Python، اختبارات ZSO على المضيف، بناء مصدر OPL المثبّت، بناء TEa.G3.ELF بمحرك OPL فقط، وتجميع حزمة الاختبار.
- [x] بعد دمج تحديث الموقع، نجح بناء master [45842046784](https://github.com/TEaG3/newday/actions/runs/45842046784) على commit `279e2fab7f505def2de7ee0bff00fd66467957cb` بجميع الوظائف الأربع؛ رُفعت مجدداً artifacts الخاصة بـ OPL والمشغّل وحزمة الاختبار وسجل البناء.
- [x] نجحت اختبارات ISO وUL المضمنة في make test، واختبار اختيار محرك OPL فقط.
- [x] نجحت اختبارات ZSO الخام والمضغوطة وحدود الكتل وأخطاء القراءة، واختبار محول ZSO الخاص بـ EE على المضيف.
- [x] نجحت اختبارات Python وC لفهرسة snapshot/diff، المفاتيح المستقرة، رفض المسارات غير الآمنة والتكرار، ترحيل sidecar، القراء المتزامنين، إلغاء المسح، بوابة التطبيق، وبوابة التنزيل.
- [x] نجح اختبار محول semaphore الخاص بـ PS2 على المضيف مع حقن فشل WaitSema وSignalSema واختبار القراء المتزامنين.
- [x] بُني OPL من المصدر المثبّت؛ فحص الترجمة العربية سجّل 326/326 سلسلة، دون سلاسل غير مترجمة، مع RTL=1 وخط عربي مضمن. جرى بناء وحدات PADEMU؛ الاقتران الفعلي بوحدة تحكم يحتاج جهازاً.
- [x] تحقق بناء TEa.G3.ELF من أن بيان runtime يحتوي مورداً واحداً فقط هو OPL.ELF، وأن ملف التشغيل وبيان runtime موجودان.
- [x] دُمج PR #120 لتحديث روابط fallback في الصفحة الرئيسية والدليل ومختبر OPL إلى الحزمة المنشورة `arabic-v1.3.11`. نجح فحص الموقع ونشر GitHub Pages في run [45842046763](https://github.com/TEaG3/newday/actions/runs/45842046763) على commit `279e2fab7f505def2de7ee0bff00fd66467957cb`؛ نجحت وظيفتا التحقق والنشر.
- [x] نُشر الإصدار التجريبي arabic-v1.3.10 سابقاً؛ هذه الملاحظة تاريخية، وقد حل محلها الإصدار المنشور arabic-v1.3.12 المسجل أعلاه.

**مطابقة المصدر والإصدار:** أُنشئ وسم arabic-v1.3.11 مباشرةً من الالتزام `9f9cec61b33ce78fb768eaab53a068e5374fa63c`، ثم دُمج PR #119 إلى master عبر الالتزام `507370f0167af8d5a3afe1b087163734bfb44a4d`؛ لذلك يبقى الالتزام المعلّم جزءاً من تاريخ master دون تحريك الوسم أو إعادة كتابة التاريخ. يختار `release.js` أحدث حزمة منشورة تلقائياً، بما فيها الإصدارات التجريبية.

## 1.1 إصلاحات سلامة الملفات والفهرسة

- [x] دُمج PR #115 في master بالالتزام `c4c49cc635fae55036e76b0bee1be4b14dc71445`. يستخدم المثبّت `O_CREAT | O_EXCL` لملفات staging الخاصة بأصل OPL وملف `.bundle-id`، فلا يقتطع أو يحذف ملفات `.tmp` الموجودة مسبقاً.
- [x] نجح اختبار `runtime installer preserves pre-existing staging files` في `make test` ضمن run 45840046904، مع نجاح `iso selftest ok` و`ul selftest ok` وفحص CRC للحزمة.
- [x] دُمج PR #117 في master بالالتزام `b84223d1ac06fd417eb55bbcdb53ff5edbee3687`. أضيفت `catalog_stage_snapshot_publish` لتبديل active/candidate بعد طلب apply، والتحقق من candidate، ومنع القراء الجدد وانتظار reader leases؛ واختُبرت أيضاً عبر محول semaphore الخاص بـ PS2.
- [x] نجحت 69 اختبارات Python، واختبارات catalog stage وPS2 semaphore adapter على المضيف في run 45841030508؛ دالة النشر ما زالت غير مستدعاة من `launcher.c`.
- [x] بُني OPL من المصدر المثبّت `f53e0c5`، وفحص التعريب سجّل 326/326 سلسلة عربية، 0 غير مترجمة، RTL=1 والخط العربي المضمن.

**حدود القبول:** المراقبة الحية ومسح المصادر الفعلي وربط تبديل snapshot بخيط العرض بين إطارين ما زالت غير مفعّلة. لا تعرض SMB/UDPBD/UDPFS أو MMCE كميزات مدعومة. لا يوجد اختبار إقلاع وقراءة مستمرة على جهاز PS2 حقيقي بعد؛ يجب أن يتم ذلك على جهازك قبل إعلان التشغيل ناجحاً.

## 2. محرك التشغيل ومصادر الألعاب

| الصيغة / المصدر | ما تحقق آلياً | حالة التشغيل على PS2 حقيقي |
|---|---|---|
| USB ISO | اختبار محلل ISO وبيانات SYSTEM.CNF، ومسار OPL مدمج | لم يُثبت بعد |
| USB ZSO | اختبارات مفكك ZSO الخام والمضغوط والحدود وأخطاء I/O | لم يُثبت بعد |
| USBExtreme UL | اختبار محلل ul.cfg والتحقق من كل الأجزاء المجزأة ومسار OPL المباشر | لم يُثبت بعد |
| ATA FAT32 HDD ISO/ZSO | محول OPL BDM/ATA مبني | اختبار الجهاز والقراءة المستمرة مطلوب |
| APA / HD Loader | فحص بيانات القسم ومسار OPL HDD mini-launch مبنيان | اختبار الجهاز مطلوب |
| MX4SIO وiLink | محولات OPL مضمّنة في البناء | اختبار الجهاز مطلوب |
| SMB وUDPBD وUDPFS كمصادر ألعاب | غير مفعّلة وتفشل مغلقاً | لا تُعرض كميزات مدعومة |
| MMCE كمصدر ألعاب | تشغيل الألعاب منه معطّل | لا يُعرض كميزة مدعومة |

فحص ملف ISO أو تعداد ملفاته ليس اختبار تشغيل. بعد التسليم يجب على OPL إعادة فتح الجهاز نفسه وقراءة قطاعات اللعبة طوال التحميل واللعب. لا توجد في هذا السجل أدلة إقلاع أو قراءة مستمرة على جهاز PS2؛ لذلك تبقى ISO وZSO وUL وHDD/APA وMX4SIO وiLink غير معتمدة عتادياً.

## 3. الفهرسة المرحلية والمراقبة الحية

**مسار candidate/apply متصل بالمشغّل، لكن المراقبة الدورية لا تزال معطلة.**

- [x] التقاط Snapshot ابتدائي متحقق منه بعد فهرسة الإقلاع، مع مفاتيح ثابتة تعتمد نوع المصدر ووحدته ونوع العنصر والمسار؛ ويستخدم UL بصمة اسم OPL بدلاً من رقم صف ul.cfg.
- [x] حساب Snapshot/Diff للإضافة والحذف والتغيير دون اعتبار اختلاف ترتيب القائمة تغييراً في الهوية.
- [x] بناء candidate catalog في مخزن مستقل عن الجيل النشط، مع إلغاء متعاون وإغلاق مقابض المصدر قبل إسقاط candidate الجزئي.
- [x] ترحيل typed sidecars فقط للعناصر التي بقي مفتاحها ومحتواها دون تغيير؛ وتصفير مؤشرات candidate المستعارة دون تحرير موارد الجيل النشط عند رفض التطبيق.
- [x] ربط نشر الحزمة بخيط العرض: طلب apply يغلق بوابة القراء الجدد، ينتظر leases الحالية، ويبدّل items/sidecars/count/snapshot/selection معاً بين الإطارات.
- [x] بعد نجاح التبديل يعيد إطار العرض بناء `pos/order`، ويبدأ دورة ظهور جديدة لـ `shown_box/wave`؛ ولا تُقرأ حزمة الجيل الجديد قبل اكتمال التبديل.
- [x] فشل retire callback failure-atomic يعيد الحزمة القديمة إلى active قبل فتح القراء؛ إذا فشل قفل semaphore نفسه يتوقف مسار العرض fail-closed.
- [x] بوابة القراء والمسح والتنزيل والنشاط تمنع تداخل المسح/التنزيل مع اللعب أو I/O، وتطلب إلغاء المسح أو التنزيل وانتظار انتهاء العملية قبل بدء النشاط.
- [x] اختبارات مضيف للتزامن وفشل القفل والإلغاء وفشل تحرير semaphore، وللتبديل والرجوع عند فشل retirement.
- [x] اختبار ربط launcher يثبت أن `CATALOG_LIVE_MONITOR_ENABLED` يظل صفراً افتراضياً؛ لا يبدأ خيط المراقبة في النسخة الحالية.

**ما لم يُفعّل بعد:**

- [ ] تفعيل polling دوري محافظ مع mount checks خاصة بكل مصدر، بعد التأكد من سلوك الإلغاء ومقابض I/O.
- [ ] اختبارات USB removal/reinsert وHDD/APA وSMB وMMCE على الجهاز؛ لا تفترض وجود hot-plug event API.
- [ ] اختبارات دورة حياة semaphore وتهيئة الشبكة وTCP connect/send على PS2 حقيقي.
- [ ] إثبات أن المسح والتنزيل يتوقفان أثناء اللعب على العتاد، وأن تطبيق Snapshot لا يتداخل مع IGR أو العودة من OPL.
- [ ] اختبار إقلاع ISO وZSO وUL والقراءة المستمرة لكل مصدر على حدة قبل إعلان الدعم.

لا توجد في هذا السجل أدلة إقلاع لعبة أو قراءة قطاعات مستمرة على جهاز PS2 حقيقي. لذلك تبقى المراقبة الحية معطلة، وتبقى SMB/UDPBD/UDPFS وMMCE كمصادر ألعاب غير مدعومة.

## 4. العربية والهوية والتوافق

- [x] OPL: 326/326 سلسلة عربية مفحوصة، وRTL=1، والخط العربي مضمن في البناء.
- [x] واجهة TEa.G3 الأساسية تخضع لفحص تطابق مفاتيح العربية والإنجليزية وتوليد الخط والثيمات.
- [x] اسم المنتج الحالي TEa.G3 في شاشة البدء والهوية المرئية واختبارات العلامة.
- [x] Neutrino ليس محركاً في النسخة الحالية؛ مرجع الواجهة فقط تحت ui/neutrino-igr.
- [x] الموقع يختار العربية أو الإنجليزية؛ دليل الموقع الحالي يجب أن يعرض ar وen فقط.

تبقى أسماء مثل nuld/ وorbit/ وBOOT/NULD.ELF وBOOT/ORBIT.ELF وVMC/NULD.bin وtools/nuld_catalog.py وnuld_hash ومفتاح التخزين المحلي القديم nuld-site-lang لأغراض ترحيل البيانات والتوافق مع التركيبات السابقة. هذه ليست علامة المنتج المرئية. لا تحذفها ولا تحذف بيانات المستخدم لمجرد إزالة الاسم القديم.

## 5. اختبارات الجهاز المطلوبة قبل إعلان الإصدار نهائياً

- [ ] USB ISO: إقلاع لعبتين على الأقل، ثم قراءة مستمرة وانتقالات مشاهد.
- [ ] USB ZSO: الإقلاع والقراءة المستمرة والتحقق من العنوان والرقم التسلسلي.
- [ ] USBExtreme UL: ul.cfg وكل الأجزاء المطلوبة، ثم إقلاع وقراءة مستمرة.
- [ ] ATA FAT32 HDD وAPA/HD Loader وMX4SIO وiLink، كل مصدر على حدة.
- [ ] الرجوع من OPL إلى TEa.G3 عبر START+SELECT، واختبار R3+L3 لإيقاف الجهاز.
- [ ] اختبار يد التحكم وبطاقات الذاكرة الفعلية والتشغيل المتكرر.
- [ ] اختبار مخارج Composite AV وS-Video وRGB SCART وComponent وHDMI على التوصيلات والشاشات الفعلية؛ لا تفترض أن كل محول HDMI يقبل 480i/576i أو 720p.
- [ ] تسجيل موديل PS2 والمنطقة والوصلة ونظام الملفات واللعبة ورقمها التسلسلي وبصمة الصورة ومسار التشغيل ومدة القراءة وسجل الأخطاء.

**قاعدة القبول:** لا تحول أي اختبار عتادي إلى ناجح اعتماداً على CI أو محاكي أو محاكاة المتصفح. أرفق سجل الجهاز أو صورة/فيديو لكل مصدر وصيغة قبل إزالة حالة الإصدار التجريبي.


## 6. مزامنة حالة القرص الضوئي مع Snapshot (PR #172)

- [x] دُمج PR #172 في `master` بالالتزام `d2f6fb8a94f93377003b3c2a3e7538577daa586b`. عند تطبيق خيط العرض لتغيير القرص، يُعاد التقاط Snapshot النشط حتى تتطابق بصمة سجل `DISC` مع `cv[0]`. إذا فشل الالتقاط، تُلغى جاهزية Snapshot وتظل إعادة الفهرسة الحية معطلة.
- [x] أضيف اختبار مضيف يغيّر بصمة خانة القرص ويتأكد من تصفير موارد Sidecar الخاصة بالقرص وحدها، مع نقل موارد اللعبة التي لم تتغير بأمان.
- [x] PR CI: [PS2 launcher build #46029309904](https://github.com/TEaG3/newday/actions/runs/46029309904) و[CodeQL #46029310049](https://github.com/TEaG3/newday/actions/runs/46029310049) نجحا بالكامل. سجل الاختبار: 99 اختباراً؛ ZSO host/EE scanner وHDL/APA scanner وISO وUL وcatalog stage وPS2 semaphore adapter ومصفوفة OPL-only نجحت.
- [x] بعد الدمج، نجح [PS2 launcher build على master #46029690421](https://github.com/TEaG3/newday/actions/runs/46029690421) و[CodeQL #46029690326](https://github.com/TEaG3/newday/actions/runs/46029690326) على commit الدمج نفسه.
- [x] نُزّلت Artifacts من بناء master بعد الدمج وطوبقت بصمات ZIP مع GitHub: OPL `d206ba64ab11e9b92734309b4199eb64dd9a39d5e6ede1afe2911f88e2a36476`؛ Launcher `b021dce2f8fa97895a33151afce68ac30fd2d8a12816cbc583dae3e7725bb660`؛ Build log `ff6cca3ca1760decaf1321593d1ec54631727fd111fb5400de0d05bb020ca052`.
- [x] فُكّت حزم Launcher وOPL محلياً وفُحصت: `TEa.G3.ELF` ملف ELF 32-bit MIPS N32 حجمه 9,568,564 بايت، و`OPL.ELF` ملف ELF 32-bit MIPS N32 حجمه 5,792,496 بايت. بيان التشغيل يحتوي مورداً واحداً فقط (OPL.ELF)، CRC32 `aa490716` مطابق.
- [ ] لم يُنشأ Release جديد ولم تتغير روابط الموقع؛ لا يوجد دليل تشغيل فعلي على جهاز PS2. المزامنة تصلح Snapshot التمهيدي، لكنها لا تفعّل المسح الحي أو التبديل المرحلي.



## 7. Fail-closed source-root guard (PR #176)

- [x] Added `catalog_source_root_probe.c/.h`. The candidate scanner checks every configured local source root before and after scanning; a missing optional DVD/CD/POPS directory is accepted as empty only when its device root still opens. HDL and the remote UDPFS app catalogue retain their source-specific paths.
- [x] Added a host C test for mounted, missing, non-directory, and overlong roots, plus launcher-wiring regression tests. Updated the existing directory-scan test to require the new root probe rather than the previous ENOENT-only condition.
- [x] PR #176 head `b36f3fae02324bb9388c83b51f25409bb74d1582`, merged to `master` as `5a2bbe6d7c112707973b5120cf4f56e041bdb05d`.
- [x] PR CI: [PS2 launcher build #46042068016](https://github.com/TEaG3/newday/actions/runs/46042068016) and [CodeQL #46042067896](https://github.com/TEaG3/newday/actions/runs/46042067896) succeeded. Host run: 103 Python tests; ZSO raw/compressed and EE scanner, HDL scanner, source-root probe, catalog stage, PS2 semaphore adapter, ISO/UL selftests, and 12-case OPL-only site matrix passed.
- [x] Post-merge master CI: [PS2 launcher build #46042504485](https://github.com/TEaG3/newday/actions/runs/46042504485) and [CodeQL #46042504505](https://github.com/TEaG3/newday/actions/runs/46042504505) succeeded.
- [x] Downloaded and verified post-merge artifacts: launcher artifact `15221581206` (4,286,551 bytes; SHA-256 `b1bf55211876890ffb72d440a40326df1090d1a90474a48db350a4472772ea6a`); OPL artifact `15220023718` (2,539,789 bytes; SHA-256 `bd573f05795956b5e71244d895f8e434ad974b0e5ca0420d9054e63fb7e9fd13`); integration bundle `15220009019` (17,780,614 bytes; SHA-256 `e096f2e1514c4a0add776a299034da060c07ffa0a6ea4934ec1a625b3a2bfb57`); build log `15220976732` (1,864 bytes; SHA-256 `2df8dd49e688de14e796b3e351ebcd798da1e75080e1e99003796266a64460be`).
- [x] ZIP integrity passed. `TEa.G3.ELF` is ELF32 MIPS N32, 9,569,140 bytes; `OPL.ELF` is ELF32 MIPS N32, 5,792,496 bytes. Runtime manifest contains exactly one game engine resource, `OPL.ELF`, with CRC32 `aa490716`. The packaged `TEa.G3-Complete-Package.zip` is 6,707,474 bytes, SHA-256 `81dccb4b9aeebcf0241fea1467529cdfe52038d3fc95ac549e82fc5dfe8b13a0`; its embedded `SHA256SUMS.txt` matches both package and launcher ELF hashes.
- [ ] This probe does not detect a rapid media replacement at the same mount point and does not implement periodic polling. Candidate/apply wiring remains disabled; no new Release or website download-link change was made. Hardware validation is still required before live refresh can be enabled.
