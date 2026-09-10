# Плавание на L_CharacterLab

Пруд расположен в центре (1800, 1700), уровень воды Z=28 см. Берег плавно уходит в чашу примерно 30 × 22 м, глубина в центре около 4,2 м. Форма дна записана в базовый слой Landscape и имеет настоящую коллизию. Остальные участки полигона сохранены.

## Как проверить

В Play откройте панель разработчика: **⌘D на Mac / Ctrl+D на Windows**. В разделе персонажа нажмите **«Вернуться на берег · восстановиться»** и идите к середине.

- WASD — движение; под водой направление вперёд следует взгляду.
- Shift — ускоренный шаг или более частый гребок с расходом сил.
- C — погружение / движение глубже, когда дно глубже 2,3 м.
- Space — всплытие.
- V — переключение камеры первого / третьего лица.

По мере погружения ног растёт сопротивление, замедляется шаг и меняется поза корпуса и рук. При достаточной глубине включается плавание. Брасс разделён на подтягивание рук, вдох, толчок ногами и скольжение. При остановке используются короткие спокойные движения; усталость постепенно переводит персонажа на сокращённый гребок и меньшую скорость. На мелководье персонаж возвращается к ходьбе.

Обычная скорость плавания — 175 см/с, с Shift — 255 см/с, при сильной усталости — около 90 см/с. Порог усталости 15%, восстановление полноценного темпа после 40% предотвращает постоянное переключение состояний. Под водой запас воздуха рассчитан на 40 секунд спокойного движения; ускорение повышает расход на 35%. При остатке менее 25% появляется предупреждение, менее 20% — нарастающее затемнение. Автоматического всплытия нет: Space остаётся действием игрока.

После нуля кислорода истощается отдельный шестисекундный запас до утопления (`DrowningHealth`). Это локальная механика удушья, а не общая система здоровья от любых повреждений. Пока запас не исчерпан, выход дыхательной точки из воды прекращает удушье, восстанавливает воздух и постепенно возвращает видимость. Дыхательная точка принадлежит персонажу; камера третьего лица не даёт воздуха. Используется стабильная высота дыхательной точки с гистерезисом, чтобы покачивание анимации не включало вдох каждый кадр.

При смерти гребки прекращаются, за четыре секунды тело переходит в авторскую расслабленную позу. Движение игрока отключается, тело медленно опускается с проверкой коллизии. Затемнение доходит до чёрного; подсказка и панель разработчика остаются доступны. Всплытие или восстановление кислорода не оживляют умершего персонажа. Для следующего теста кнопка «Вернуться на берег · восстановиться» сбрасывает удушье, смерть, затемнение и состояние движения. Кнопка «Проверить нехватку воздуха · 5%» ускоряет ручную проверку под водой.

## Реализация и границы

Движение использует отдельный режим CharacterMovement с ускорением, сопротивлением и swept collision. Это пользовательский режим, а не встроенный PhysSwimming / Water Plugin. Глубина измеряется лучом до реального дна; переходы имеют разные пороги входа и выхода. Манипуляция предметами и потеря равновесия на время плавания отключаются, чтобы не конкурировать за позу тела.

Камера сохраняет управление взглядом. Подводный эффект включается по положению камеры: цвет, поглощение света по расстоянию, снижение видимости. На поверхности создаются круги на воде. Плавучесть предметов, специальный подводный звук, сетевая синхронизация и полноценный игровой цикл респавна пока не реализованы. На полигоне возвращение после смерти выполняется через панель разработчика.

Анимации созданы для Manny с ограниченными целями конечностей и положительным сгибом коленей / локтей. Это авторские тестовые клипы, не подводный motion capture. Референсы: [Swim England — техника брасса](https://www.swimming.org/masters/improving-your-breaststroke-technique/), [CMU Graphics Lab, subject 125](https://mocap.cs.cmu.edu/search.php?subjectnumber=125). Исходники и атрибуция — SourceArt/Motion/CMU125. Прямой перенос записи CMU отклонён из-за резких переходов поз.

## Проверки

- `Scripts/MCP/review_swim_motion.py`: сгибы коленей / локтей, шаг поворота костей между кадрами, изображения поз.
- `Scripts/Editor/review_swimming.py` в PIE: брод, плавание, Shift, усталость, погружение, всплытие, выход на берег.
- `Scripts/Editor/review_drowning.py` в PIE: расход воздуха, удушье, спасение, смерть, блокировка управления и повторный тест.
- `Scripts/MCP/analyze_swimming.py drowning`: анализ поз в сценарии утопления.
- `Scripts/MCP/analyze_swimming.py`: конечность координат и направления сгибов по фактическим костям из PIE.
- `Scripts/Editor/view_swimming.py`: ограниченный по времени визуальный просмотр; `restore_swimming_view.py` возвращает обычную скорость и ввод.

Отчёты находятся в Saved/swimming_review.json и Saved/swimming_analysis.json. Проверка углов исключает обратный сгиб в записанных кадрах, но не заменяет художественную оценку всей пластики.

10 сентября 2026: сборка AltaiEditor Mac Development успешна; восемь проверок PIE прошли, включая направленное движение под водой. Анализ 673 кадров: колени 14,2–113,3°, локти 8,5–134,4°, обратных сгибов нет. Визуально просмотрены фаза скольжения, сокращённый гребок и подводная камера первого лица. Повторная проверка инвентаря и исключительности панелей также прошла. Windows в этой сессии не запускался. На карте остаётся предупреждение о непересобранном освещении (56 объектов); запекание освещения не выполнялось.

Проверка утопления 10 сентября 2026: финальная сборка Mac успешна; шесть сценариев прошли. В 472 кадрах перехода к смерти колени 21,3–110,5°, локти 8,5–128,8°, обратных сгибов нет. Погружение после смерти сохраняет коллизию и игнорирует ввод; восстановление кислорода не снимает состояние смерти. Затемнение рисуется после обработки изображения мира и до панелей Slate.

## Swimming motion polish — 2026-09-10

Reauthored the breaststroke catch/insweep/recovery and delayed leg kick. Periodic
cubic curves share tangents between stroke phases instead of stopping at every
key. Thoracic extension is distributed over four spine joints for the breath;
leg recovery is narrower. These are authored Manny animations, not motion capture.

Added A_SwimTread: small sculling and alternating leg support when stationary.
Forward swimming blends in with speed; fatigue remains a separate stroke blend.
Shore preparation starts at 85–125 cm immersion. Forward extension fades over
200–125 cm bottom depth before the character resumes stepping. The upright pose
blends back into locomotion with a bounded smooth transition. Shift cadence is
filtered rather than changing instantly; animation phase remains continuous.

Human references: [Speedo arms](https://www.youtube.com/watch?v=mFFxTuaMpDQ),
[Speedo kick](https://www.youtube.com/watch?v=BJj9z4n0STk),
[Swim England technique](https://www.swimming.org/masters/improving-your-breaststroke-technique/).
Subnautica remains the underwater-control reference; its first-person footage
is not evidence for third-person joint anatomy. No external animation assets copied.

Validation: Mac Editor build; four clip hinge/rotation/loop checks; eight PIE
swimming scenarios on the saved pond; third-person side-view shore traversal.
`view_swim_shore.py` provides a bounded repeatable visual pass. Numerical checks
verify continuity and joint direction, not artistic realism or every terrain shape.

Visual follow-up lowered the treading pelvis 32 cm and tucked the legs so the
waterline reaches the neck rather than the waist. Four final clips retain positive
hinges (tread knees 88–115 degrees) and matching loop endpoints. Six breathing,
suffocation, rescue, death and reset scenarios also passed after the runtime change.

The final integration pass uses simulation time, with a 120-second wall-clock
watchdog, so background throttling cannot truncate the route. It exposed a
146-degree knee inherited from the outgoing pose at very low swim blend; the
swimming node now applies calibrated hinge bounds after blending as well.
