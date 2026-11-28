# TEa.G3 v1.3.14 — ملاحظات الإصدار

## نطاق الإصدار

- ملف المستخدم الوحيد هو `TEa.G3.ELF`، ومحرك ألعاب PS2 الوحيد المضمّن هو OPL.
- Neutrino ليس محرك تشغيل في هذه النسخة؛ بقيت أصول واجهته مرجعاً بصرياً فقط.
- USB ISO/ZSO وUSBExtreme UL لها مسارات OPL واختبارات آلية. HDD/APA وMX4SIO وiLink تحتاج اختباراً على العتاد.
- SMB وUDPBD وUDPFS لبث الألعاب وMMCE كمصدر ألعاب معطلة وتفشل مغلقاً.

## تحسينات الفهرسة والسلامة

- فهرسة candidate مستقلة عن القائمة النشطة، مع Snapshot/Diff ومفاتيح ثابتة.
- ترحيل sidecars typed بحسب هوية اللعبة ومحتواها، مع إعادة ربط التحديد ومؤشرات العمل.
- تطبيق الجيل الجديد بين الإطارات بعد تصريف القراء، وتحرير موارد الجيل القديم داخل بوابة التطبيق.
- التراجع إلى الجيل القديم عند فشل تحرير الموارد، وإيقاف المراقبة عند تعذر معرفة حالة القفل.
- إلغاء مسح المصادر والتنزيلات عند بدء اللعب أو نشاط التخزين، وإغلاق مقابض I/O والتخلص من candidate غير المكتمل.
- المراقبة الدورية تبقى معطلة افتراضياً حتى اختبار semaphore وUSB removal/reinsert وIGR والتزامن على جهاز PS2 حقيقي.

## التحقق البرمجي

بناء master الأساسي قبل رفع رقم الإصدار:
- [PS2 launcher build #46089647468](https://github.com/TEaG3/newday/actions/runs/46089647468) — نجحت الوظائف الأربع: بناء OPL المثبّت، ZSO host fixture، بناء TEa.G3.ELF، وتجميع حزمة التكامل.
- [CodeQL #46089647459](https://github.com/TEaG3/newday/actions/runs/46089647459) — نجحت تحليلات C/C++ وPython وJavaScript/TypeScript.
- 114 اختبار مضيف نجحت، مع اختبارات ISO وUL وZSO وHDL/APA وsource-root وSnapshot/Diff وPS2 semaphore concurrency.
- تعريب OPL: 326/326 سلسلة، RTL=1، والخط العربي مضمّن؛ اختبار الخط 509 glyphs.

نجح مسار نشر الإصدار نفسه: [run #46096405172](https://github.com/TEaG3/newday/actions/runs/46096405172) و[run #46096461979](https://github.com/TEaG3/newday/actions/runs/46096461979). نُشر الإصدار المستقر [arabic-v1.3.14](https://github.com/TEaG3/newday/releases/tag/arabic-v1.3.14) دون prerelease.

- حزمة المستخدم: 6,711,484 بايت، SHA-256 `e81374d9f8360af935e6ca54fda8c44c8c788fcdcc616efdb96ea06437189b55`.
- ملف `TEa.G3.ELF`: SHA-256 `a07cd374fadd6098274af41437e4a8b56deb08333418858f94289f1685c951b7`.
- أصل `SHA256SUMS.txt`: SHA-256 من GitHub `786c3c2ec76fa56eb5b60be7217199a8ebf89a20fe404c402c8e5043783492b0`؛ سجل packager أكد `package_crc=PASS`.

## حدود القبول العتادي

نجاح CI لا يثبت إقلاع الألعاب أو استمرار قراءة القطاعات على جهاز PS2. لم تُعتمد ISO/ZSO/UL أو HDD/APA عتادياً في هذه الملاحظات، ولم تُفعّل المراقبة الحية. يلزم تسجيل اختبارات فعلية لكل مصدر وصيغة، والعودة من OPL إلى المشغّل، قبل ادعاء دعمها.
