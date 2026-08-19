# Техническое задание v2: World Simulation + Playable Systemic RPG

## Local Engineering, Architecture & Modeling Contract

**Канонический путь:** [`docs/specs/TZ.md`](TZ.md)  
**Карта документов:** [`docs/INDEX.md`](../INDEX.md)  
**Стек:** C++23 Simulation Core + Godot 4 reference client.  
**Статус:** рабочий проектный контракт для локальной разработки с нуля.

**Главная цель:** строить не «симулятор когда-нибудь», а постоянно играбельную systemic RPG, в которой симуляция мира, RPG-слой, экономика, социальные институты, политика, торговля и магия развиваются одновременно через маленькие причинные вертикальные срезы.

**Управление работой агента:** AI Layer. Этот документ не является вторым протоколом Task/Work/Epic и не переопределяет `project_status` / `work_*` / `task_*` / `epic_*`.

Смежные документы: [`AGENTS.md`](../../AGENTS.md) · [`.cursor/rules/`](../../.cursor/rules/) · [ADR 0001](../decisions/0001-cpp-godot-gdextension.md)

---

# 0. Назначение документа

Это ТЗ является одновременно:

1. продуктовым контрактом;
2. архитектурным контрактом;
3. контрактом локального toolchain;
4. политикой моделирования;
5. политикой тестирования и playtest;
6. инженерными инвариантами для людей и агентов;
7. защитой от scope drift, переусложнения и «вечной разработки симуляции без игры».

Это ТЗ **не является**:

- control-plane AI Layer;
- инструкцией перепечатывать bootstrap AI Layer;
- протоколом `продолжай` / STOP / «следующий Task»;
- источником истины по текущему коду — текущий репозиторий авторитетнее устаревших абзацев.

Проект **greenfield**. Удалённые наработки (Fenster, TypeScript/Canvas, WASM/Emscripten, Playwright/Chromium) не авторитетны и не восстанавливаются.

Цель — построить живой средневековый магический мир, где:

- мир существует независимо от игрока;
- NPC, households, организации и институты принимают решения в рамках доступной им информации;
- экономика имеет физическую и институциональную причинность;
- политика следует из реальной власти, прав, обязанностей, собственности, насилия, информации и коалиций;
- торговля следует из запасов, транспорта, риска, доступа и отношений;
- магия является явным контрфактическим законом мира;
- игрок является обычным участником той же системы;
- игрок потенциально может стать **кем угодно**, если это достижимо через состояние мира;
- игра остаётся реально запускаемой и играбельной на всём протяжении разработки.

Главная продуктовая формула:

```text
world cause
  -> authoritative simulation consequence
  -> player/NPC opportunity
  -> visible game feedback
  -> player choice
  -> persistent world change
```

---

# 0.1. Граница с AI Layer

| Владелец | Что принадлежит |
| --- | --- |
| AI Layer | Work/Task/Epic lifecycle, continuation, Project Map, Knowledge, Decisions storage, managed vs ordinary routing |
| Это ТЗ + `AGENTS.md` + `.cursor/rules/` | продукт, стек, dependency direction, modeling, playable invariants, какой evidence нужен для gameplay claim |
| Текущий исходный код | фактическое поведение |

Правила:

- Не копировать процедуру AI Layer в docs/rules.
- Не вводить параллельный agent protocol (`продолжай`, STOP как оркестрация, собственные Task state-machines).
- «Bounded slice» здесь — **инженерный** запрет расползаться в соседние подсистемы, а не замена `work_wait` / `task_next`.
- Continuation пользователя («продолжай», «дальше») интерпретирует AI Layer (`work.continuation` / `work_resume`), а не это ТЗ.
- Короткий отчёт хоста (что сделано, файлы, какие проверки реально ran, blocker) важнее длинного шаблона секции 43.

---

# 1. Неподвижные инварианты проекта

Эти правила выше локальной красоты архитектуры и выше желания «сначала доделать subsystem».

## 1.1. Playable Main Invariant

`main` должен оставаться запускаемым и играбельным.

Запрещено сознательно оставлять `main` в состоянии:

- «simulation works, client later»;
- «protocol will be wired later»;
- «UI сломан, зато core красивый»;
- «несколько недель делаем только экономическую модель»;
- «игра пока не запускается, потому что идёт большой refactor».

Если изменение требует временно сломать playable path, оно должно быть настолько маленьким, чтобы playable path был восстановлен **в рамках той же bounded change**.

## 1.2. Vertical Capability Invariant

Обычная единица разработки — не «слой», а **одна игровая capability**, проходящая через необходимые слои.

Типичный срез:

```text
minimal world rule in C++
  -> protocol command/projection
  -> Godot affordance/feedback
  -> targeted deterministic test
  -> actual keyboard/mouse playtest
```

Это считается **одной coherent change**, а не пятью независимыми задачами «по слоям».

## 1.3. No Simulation Waiting Room

Нельзя сначала строить «достаточно полную симуляцию», а потом начинать RPG.

Любая новая большая система сначала появляется в минимальной причинно корректной форме, которая уже доступна игроку или заметна ему.

Пример:

```text
НЕ:
полная экономика зерна -> логистика -> рынки -> налоги -> потом торговля игрока

ДА:
минимальный урожай -> один дефицит -> одна сделка игрока -> видимое последствие
  -> затем углубление производства/рынка/институтов
```

## 1.4. Fidelity Ladder

Каждая subsystem развивается ступенями:

```text
F0 absent
F1 minimal causal & playable
F2 richer constraints and consequences
F3 institutional/social feedback
F4 long-horizon feedback
F5 optimization/scale only if measured
```

Запрещено переводить subsystem на F2/F3, если её F1 ещё невозможно нормально увидеть/использовать в игре.

Запрещено держать simulation fidelity более чем на одну значимую ступень впереди player-facing exposure.

## 1.5. One Authoritative World

Существует только одна authoritative реализация законов мира — C++ Simulation Core.

Запрещено иметь:

- «упрощённую игровую экономику» в GDScript отдельно от C++;
- client-side inventory truth;
- client-side money truth;
- отдельную логику отношений для UI;
- fake client simulation, которая постепенно становится настоящей;
- C# gameplay rules (даже если установлен Mono-editor Godot).

Godot может иметь presentation prediction/interpolation, но не может создавать authoritative outcome.

## 1.6. Scope Invariant

Одна ordinary change затрагивает только файлы/подсистемы, нужные текущей capability.

Запрещено:

- самим выбирать следующий большой subsystem;
- начинать unrelated refactor;
- «раз уж здесь, ещё почистить»;
- строить framework ради будущего;
- переписывать соседние подсистемы без необходимости текущей задачи.

Это инженерный scope guard. Оркестрацию проходов задаёт AI Layer, не это ТЗ.

---

# 2. Проверенный локальный toolchain и выбранный стек

## 2.1. Фактически доступная среда на 2026-08-19

Проверено локально:

```text
g++                  13.3.0     C++23, std::expected компилируется
Python               3.12.3
Godot                4.7.1.stable.mono.official
                     ~/.local/bin/godot  -> тот же binary
                     ~/.local/bin/godot4 -> тот же binary
gdb                  present
Node.js              22.19.0    НЕ часть стека проекта
```

На момент проверки **не обнаружены**:

```text
cmake
ninja
clang++
clang-format
clang-tidy
valgrind
emcc / Emscripten
```

Нельзя в будущих отчётах писать, что отсутствующие инструменты доступны, пока это не проверено повторно.

Установленный Godot — **Mono** (C#-capable) editor. Язык клиента проекта — **GDScript**, не C#.

## 2.2. Основной язык Simulation Core

**C++23**.

Проектный режим:

```text
CMAKE_CXX_STANDARD = 23
CMAKE_CXX_STANDARD_REQUIRED = ON
CMAKE_CXX_EXTENSIONS = OFF
```

Базовый native compiler текущей среды:

- GCC 13.3 — основной локальный compiler path.

Clang — второй compiler только после явной установки. Не использовать compiler-specific extensions внутри Simulation Core без отдельного решения.

## 2.3. Build system

Использовать:

- CMake;
- Ninja;
- CMake Presets;
- CTest;
- GoogleTest 1.17.0 как **test-only dependency**, pinned exact revision/version;
- nlohmann/json только на serialization/adapter/persistence boundaries, не как domain data model;
- godot-cpp, pinned to Godot **4.7.x**, **только** в `src/adapters/gdextension`.

Правила:

- project-wide настройки — `CMakePresets.json`;
- локальные machine-specific overrides — `CMakeUserPresets.json`, не коммитить;
- target-based CMake;
- никаких глобальных `include_directories()`/`add_definitions()` для project code;
- compile options задаются target scope;
- warnings не должны случайно наследоваться third-party dependencies.

Первый bootstrap обязан **явно** установить cmake и ninja. Не симулировать успешный C++ build без них.

## 2.4. C++ dependency policy

Разрешённые стартовые third-party C++ dependencies:

```text
GoogleTest 1.17.0     tests only
nlohmann/json         protocol/persistence/adapters only
godot-cpp 4.7.x       GDExtension adapter only
```

Правила:

- exact version/commit + integrity hash;
- объявление в одном `cmake/Dependencies.cmake`;
- никакого floating `main`, `master`, `latest`;
- dependency не может проникать в domain API без отдельного architecture decision;
- ordinary gameplay change не обновляет dependencies;
- network fetch не должен происходить неожиданно во время test/playtest; initial dependency acquisition — explicit bootstrap;
- third-party warnings не повышаются до project `-Werror`;
- dependency upgrade — отдельная bounded change.

`nlohmann::json` — boundary representation. Domain code не хранит `json` objects вместо typed state.

## 2.5. Godot client

Использовать:

- Godot **4.7.x** (локально проверено 4.7.1.stable.mono);
- 2D (`Node2D` / `CanvasLayer`) как F1 presentation;
- typed GDScript;
- InputMap semantic actions;
- scenes как reusable composed objects;
- GDExtension для вызова C++ protocol.

На старте не использовать:

- C# / .NET gameplay;
- Godot 3.x API;
- 3D world как default presentation;
- React/TypeScript/Canvas web client;
- WASM/Emscripten;
- Playwright/Chromium;
- Phaser/Pixi/Three/Electron;
- ECS-addon как замену Simulation Core.

Они могут появиться только после конкретного измеренного недостатка и ADR.

## 2.6. Agent/human playtest stack

Использовать:

- Python 3 для developer tooling;
- единственный Godot playtest entry `python tools/play.py --scenario <name>` (когда инструмент создан);
- Godot `--path godot` windowed или `--headless` по scenario;
- screenshots / debug projection dump;
- read-only debug surface.

System Godot binary — тот, что в toolchain lock / `PATH` (`godot` / `godot4`). Не запускать «любой» editor из Downloads в ordinary playtest.

## 2.7. Python tooling policy

Python используется только для developer tooling/playtest orchestration, не для Simulation Core.

Repo создаёт `.venv` и pin-ит Python tooling. Минимальный набор:

```text
ruff == project-pinned version
```

Playwright **не** входит в стек.

Правила Python:

- type hints для public tool functions, где это повышает ясность;
- `pathlib`;
- `subprocess` только с explicit argv, cwd, environment и timeout/lifecycle;
- `shell=True` запрещён без отдельной причины;
- cleanup через `try/finally` / context managers;
- никакого бесконечного polling;
- lint/format через Ruff;
- Godot process lifecycle реализуется в одном месте.

## 2.8. Tool version policy

### Project semantic contract

Жёстко задано:

- C++23 Simulation Core;
- Godot 4.7.x 2D client;
- GDExtension adapter;
- deterministic native behavior на поддерживаемых сценариях.

### Concrete tool versions

Пишутся в:

```text
tools/toolchain.lock
third_party/dependency lock
godot/project.godot   (config_version / features)
```

Upgrade toolchain — отдельная bounded change с build/test/playtest.

---

# 3. Архитектура верхнего уровня

Проект имеет направленный граф зависимостей.

```text
                   +--------------------+
                   |    content data    |
                   +----------+---------+
                              |
                              v
+----------------+    +-------+--------+    +------------------+
| native tools   | -> |  Simulation    | <- | deterministic    |
| scenarios CLI  |    |  Core C++23    |    | tests            |
+----------------+    +-------+--------+    +------------------+
                              |
                              v
                     +--------+---------+
                     | Game/Application |
                     | Protocol C++     |
                     +--------+---------+
                              |
                              v
                     +--------+---------+
                     | GDExtension      |
                     | adapter          |
                     +--------+---------+
                              |
                              v
                     +--------+---------+
                     | Godot 4 client   |
                     | scenes/GDScript  |
                     | 2D / UI / Audio  |
                     +--------+---------+
                              |
                              v
                    save / inspect / replay
```

Правило зависимости:

```text
presentation -> adapter -> protocol -> simulation
simulation !-> Godot
protocol   !-> Godot
godot-cpp  !-> src/sim
```

ADR: [`docs/decisions/0001-cpp-godot-gdextension.md`](../decisions/0001-cpp-godot-gdextension.md).

---

# 4. Внутренние границы

Рекомендуемая структура:

```text
src/
  sim/
    domain/
    application/
    content/
    persistence/
    random/
    diagnostics/
  protocol/
    command.hpp
    result.hpp
    events.hpp
    projection.hpp
    version.hpp
    serialization/
  adapters/
    native_cli/
    gdextension/

godot/
  project.godot
  scenes/
  scripts/
  resources/
  ui/

content/
  village/
  items/
  occupations/
  institutions/
  magic/

tests/
  sim/
  protocol/
  scenarios/
  determinism/

tools/
  play.py
  dev.py

docs/
  INDEX.md                      # documentation map
  specs/TZ.md                   # canonical contract (this file)
  engineering/                  # stack how/how-not (C++, Godot, GDExtension, CMake/Python)
  GAME.md                       # planned: current playable facts
  ARCHITECTURE.md               # planned: runtime dependency direction
  MODELING_POLICY.md            # planned: extract from this TZ when needed
  models/
  research/
  decisions/

AGENTS.md                       # pointer; no AI Layer reprint
.cursor/rules/                  # short splits; details in docs/engineering/
```

Не создавать отдельные packages ради папок. Physical target boundary появляется, только если она реально enforce-ит dependency direction.

`src/sim` собирается и тестируется **без** Godot.

---

# 5. Simulation Core: что ему запрещено знать

Simulation Core не импортирует и не использует:

- Godot headers, godot-cpp, GDExtension macros;
- scenes, nodes, Resources, Autoloads;
- InputMap / keyboard / mouse;
- camera / sprite / canvas coordinates;
- FPS / frame delta;
- реальное wall-clock время как gameplay input;
- filesystem как часть domain logic;
- network;
- UI state;
- system locale;
- process environment как источник игровых правил;
- DOM, Canvas, Web APIs, Playwright.

Simulation Core должен запускаться:

1. как native headless library/executable/test **без Godot**;
2. через GDExtension в Godot client;
3. с одинаковым command/state contract.

---

# 6. C++ coding policy

## 6.1. Ownership

Следовать RAII.

Правила:

- value semantics по умолчанию;
- `std::unique_ptr` — если действительно нужна heap ownership;
- `std::shared_ptr` — только при доказанной shared ownership;
- raw pointer/reference — non-owning view;
- owning raw pointers запрещены;
- ручные `new/delete` в project code запрещены, кроме узких низкоуровневых случаев с обоснованием;
- resource lifetime должен выражаться типом.

## 6.2. Error model

Ожидаемые domain failures — данные, а не исключения.

```cpp
std::expected<TradeResult, TradeError> execute_trade(...);
```

Типичные domain failures: insufficient funds, unavailable stock, permission denied, target not reachable, magic access missing, requirement not satisfied.

Исключения допустимы на adapter/tool boundary для exceptional failures, но:

- исключения не пересекают GDExtension/C ABI boundary как gameplay outcome;
- исключение не кодирует обычный gameplay result;
- catch-all без диагностики запрещён.

## 6.3. Types

Использовать strong domain types там, где перепутать значения опасно: Money, SimulationTick, PersonId, HouseholdId, PlaceId, ItemCount.

Не превращать каждый primitive в class без пользы.

## 6.4. Global state

Запрещено: mutable global state, service locator, global singleton world, hidden static RNG, global registries, которые меняются во время игры, magic initialization order.

Read-only compile-time/constants допустимы.

## 6.5. Interfaces

Не создавать abstract base class до второго реального implementation, кроме границы, которая принципиально требует adapter (GDExtension/native serialization surface).

Prefer concrete types.

## 6.6. Templates

Templates — для реальной type-safety/reuse. Запрещено превращать Simulation Core в meta-programming framework.

## 6.7. Warnings

Project code:

```text
-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Wshadow -Wformat=2 -Wundef -Wnon-virtual-dtor -Wold-style-cast
```

`-Werror` разрешён в controlled verification preset для project code, не для third-party.

## 6.8. Formatting / static analysis

После установки:

- `clang-format` — единственный formatter C++;
- `clang-tidy` — targeted static analysis.

Не делать массовый format unrelated файлов в gameplay change. Не утверждать, что они доступны, пока не установлены.

---

# 7. Детерминизм

Одинаковые seed, initial state, content version, protocol version, command sequence, simulation step sequence должны давать одинаковый authoritative result в пределах заявленного contract.

## 7.1. Запрещено в authoritative logic

- `std::random_device`;
- скрытый system RNG;
- wall clock;
- `std::chrono::system_clock::now()` как gameplay input;
- dependence on frame rate;
- dependence on thread scheduling;
- nondeterministic iteration order, если порядок меняет результат;
- address-based ordering;
- locale-dependent parsing;
- unspecified filesystem enumeration order.

## 7.2. RNG

Один explicit seeded PRNG state является частью world state или deterministic subsystem state. Не создавать новый RNG «для удобства» внутри NPC/system.

## 7.3. Containers

`std::unordered_map` — lookup only. Authoritative outcome не зависит от порядка обхода.

## 7.4. Floating point

Authoritative load-bearing systems по умолчанию — integers/fixed-point/scaled integers (деньги, item quantity, time, thresholds, charges/costs).

Float разрешён в rendering/presentation и там, где bit-identical result не является контрактом.

Если float влияет на authoritative branch — отдельное решение и native determinism tests.

## 7.5. Threading

Simulation Core на первом этапе **single-threaded**. Parallel execution не должно менять simulation result.

---

# 8. Игровое время

Simulation time — integer domain value (`SimulationTick` или integer milliseconds, если единица нужна механикам).

Godot frame time не является simulation time.

Разрешены pause, x1/xN speed, discrete fast-forward, headless long steps. Все изменения мира — только через explicit simulation advancement.

После load не выполняется скрытый extra tick.

---

# 9. Protocol boundary

Protocol — небольшой application contract, не копия внутреннего WorldState.

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

Примеры intents: MoveIntent, InteractIntent, TalkIntent, TradeIntent, GiveIntent, TakeWorkIntent, UseItemIntent, CastMagicIntent, SleepIntent, TravelIntent, JoinOrganizationIntent, PetitionIntent, Vote/SupportIntent, AdvanceTimeIntent.

## 9.1. Client не отправляет desired state

Плохо: `SetMoney(500)`, `SetRelationship(+10)`, `SetJob("blacksmith")`.

Хорошо: `OfferTrade(...)`, `GiveGift(...)`, `AskForApprenticeship(...)`, `ApplyForOffice(...)`.

## 9.2. Protocol version

Explicit version. Breaking change обновляет version и native + Godot-facing tests одновременно. Не поддерживать старые protocol versions заранее.

---

# 10. C++ -> Godot boundary (GDExtension)

Внутренние C++ классы **не являются Godot API**.

GDExtension adapter — отдельный тонкий слой.

Предпочтительный ранний контракт:

```text
small exported facade
  + typed protocol structs
  + optional UTF-8 JSON envelopes for debug/save/replay
```

Причины:

- нет lifetime coupling GDScript <-> C++ object graphs мира;
- native test использует тот же protocol без Godot;
- простой стабильный boundary важнее ранней micro-optimization.

Запрещено:

- экспортировать в Godot mutable pointer на WorldState как gameplay API;
- позволять GDScript напрямую менять C++ memory мира;
- класть domain rules в adapter «на минутку»;
- делать Autoload, который сам симулирует мир.

Godot получает projections/events и отправляет intents.

---

# 11. Reference client

Reference client — **настоящая игра**, а не debug visualizer.

Он обязан позволять игроку: ходить, видеть NPC и мир, взаимодействовать, разговаривать, торговать, использовать/передавать предметы, работать, путешествовать в пределах реализованного мира, применять магию, наблюдать социальные и экономические последствия, сохранять/загружать, понимать локальные проблемы без чтения debug console.

## 11.1. Ответственность client

Client владеет: input mapping, camera, rendering, animation, interpolation, UI panels, audio, selection/hover, accessibility/presentation state, local menu state.

Client **не владеет**: inventory truth, money truth, relationships truth, status truth, skills truth, authoritative semantic location, ownership truth, institution membership truth, spell outcome, trade outcome.

---

# 12. Принцип «игра развивается параллельно симуляции»

## 12.1. Capability Quartet

Каждая новая gameplay capability должна иметь четыре части:

```text
RULE        authoritative causal rule
CONTRACT    command/result/projection/events
EXPERIENCE  что игрок реально делает и видит
PROOF       тест + playtest
```

Capability не считается законченной только потому, что `RULE` готов.

## 12.2. Default vertical change

По умолчанию:

> Игрок теперь может/видит X, потому что simulation реально делает Y, и это доказано Z.

Не так: «Реализовать новую architecture subsystem X».

## 12.3. Допустимые sim-only changes

Только если:

- исправляет конкретный defect уже playable capability;
- является performance fix после измерения;
- является deterministic/persistence correctness fix;
- является маленьким prerequisite следующего player-visible изменения и пользователь явно разрешил именно его.

## 12.4. Запрет subsystem tunnel vision

Нельзя делать подряд серию задач `economy internals 1..N`, если игрок не получает нового читаемого interaction/consequence.

## 12.5. Minimal causal first

Если полная simulation feature дорогая, сделать не fake, а **самую маленькую настоящую модель**.

Политическая власть на F1 может быть: one office, one rule granting a real permission, one obligation, one selection/replacement mechanism, one visible consequence.

---

# 13. «Игрок может стать кем угодно» — системный контракт

Фраза не означает бесконечный список hardcoded classes.

Она означает, что роль игрока возникает из compositional world state.

## 13.1. Не существует authoritative `PlayerClass`

Не делать core enum `FARMER / MAGE / MERCHANT / NOBLE / CRIMINAL / PRIEST / POLITICIAN` как главный источник прав.

Роль определяется комбинацией: навыков, знаний, имущества, капитала/запасов, инструментов, лицензий/прав, legal status, household ties, организации, должности, репутации, отношений, долгов и обязанностей, территории, доступа к магии, фактических действий и истории.

UI может показать label «merchant» или «village reeve», но label — **projection**, не source of truth.

## 13.2. Player/NPC symmetry

Если игрок может купить землю, владеть мастерской, нанять человека, вступить в организацию, стать учеником мага, получить должность, дать взятку, нарушить закон, заняться торговлей, создать долг — это следует из правил мира, а не `if (actor.is_player)`.

NPC не обязаны иметь тот же decision algorithm, но должны подчиняться тем же constraints/consequences.

## 13.3. Opportunity-driven RPG

```text
world state + actor state + institution rules
  -> available actions/opportunities
```

Не вокруг глобального skill-tree, который абстрактно unlock-ит мир независимо от институтов.

---

# 14. NPC и bounded rationality

NPC знают только доступную им информацию.

Knowledge может приходить из: личного наблюдения, памяти, разговора, слуха, письма, официального объявления, рынка, организации, магического средства информации, если оно существует.

Запрещено принимать решение на основе hidden global truth без явного design reason.

Минимальное состояние NPC: identity, household, semantic location, carried/owned resources, health/energy/hunger на нужной fidelity, active intention/task, relevant skills, relationships/obligations, known facts, institutional memberships/statuses.

Не строить заранее full psychology/personality ontology.

---

# 15. Task/action model NPC

```text
need / obligation / opportunity / threat / goal
  -> perceived options
  -> feasible option evaluation
  -> intention
  -> required place/resources/people
  -> travel/access
  -> process over simulation time
  -> consume/produce/transfer
  -> events/consequences
  -> new world state
```

Расписание может ограничивать возможность, но не должно быть главным hidden script `08:00 -> FarmerWork()`.

---

# 16. Households

Household — один из базовых социально-экономических aggregates, но не универсальный ответ на всё.

Может иметь: housing rights, shared stores, shared tools, land access, dependents, obligations, debts, internal resource access rules.

Не моделировать family genetics до gameplay need.

---

# 17. Экономика

Экономика обязана иметь physical + institutional causality.

Для production chain разделять: resources, labor/time, tools/skills, access rights, ownership/control, transport, storage/loss, obligations/extraction, exchange если он реально происходит.

## 17.1. Ранний economic slice

```text
access to field
  -> labor
  -> grain
  -> storage
  -> food processing
  -> household consumption
  -> shortage/work-capacity consequence
  -> player intervention
```

## 17.2. Trade

Trade — конкретная transaction, а не магический глобальный price API.

На старте: конкретные actors/market place, stock, money/resources, simple asking/offer rule, transaction history, transport/access constraints.

Различать: sale, barter, gift, debt, rent, tax/tribute, household sharing, wage, labor obligation, institutional allocation.

---

# 18. Политика

Политика не начинается с «politics subsystem».

Она появляется, когда существуют реальные права, ограничения, должности, ресурсы власти, способы назначения/смещения, наказания, обязательства, коалиции/поддержка, информационные ограничения.

Минимальный political slice должен позволять игроку заметить:

```text
кто может принять решение
почему он имеет право
кто выигрывает/проигрывает
как это решение можно изменить/оспорить/обойти
```

Не строить государственный simulator до локального playable institution.

---

# 19. Социальный слой

Relationships не должны быть одной универсальной шкалой `-100..100` для всего.

На ранней стадии можно иметь несколько осмысленных dimensions только там, где они меняют действие: trust, obligation/debt, familiarity, reputation in group, hostility/fear, authority/standing.

Не вводить dimension без gameplay consequence.

Social event должен менять future opportunity, behavior или cost, иначе это декоративный number.

---

# 20. Исторический baseline

История определяет немагический baseline.

Для каждого reference scenario указать: region, period, settlement type, climate/geography, political/institutional assumptions, trade context, technology baseline.

Не смешивать автоматически практики разных веков и регионов.

Load-bearing historical assumptions требуют источников.

Приоритет: academic monographs; peer-reviewed research; critical editions / specialist datasets; university/museum scholarly material; primary sources с контекстом.

Research прекращается, когда для gameplay уже понятно: causal baseline, plausible range, uncertainty, simplification, consequences of simplification.

---

# 21. Magic counterfactual policy

Магия — explicit altered law.

Для каждой magic capability определить: Capability, Access, Acquisition, Cost, Range, Duration, Reliability, Failure modes, Observability, Countermeasures, Institutional control, Economic / Political / Social / Long-term consequences.

Магия не может быть универсальным объяснением дыр. Если магия снимает load-bearing constraint, downstream baseline пересматривается.

Примеры: teleportation -> transport/trade/borders; healing -> mortality/labor/care; weather control -> agriculture/storage/power; prophecy -> information/crime/markets; food creation -> scarcity and land/labor value.

---

# 22. Model Contract

Для серьёзной mechanics создавать `docs/models/<mechanic>.md`.

Шаблон:

```markdown
# Model: <name>

Status: DRAFT | ACCEPTED | REVISE

## Gameplay purpose
## Causal model
## Historical baseline
## Magic deviations
## Inputs
## State
## Transitions
## Outputs / consequences
## Player-facing exposure
## Uncertainty
## Simplifications
## Deliberately not simulated
## Sources
## Falsifiers
```

Model Contract не создаётся для каждой мелкой функции.

---

# 23. Content отдельно от laws

Rules определяют, что возможно.

Content определяет конкретные people, places, households, items, institutions, occupations, recipes, magic capabilities, scenario initial conditions.

Не hardcode конкретного NPC внутри общего domain algorithm.

Не строить universal DSL на старте. Использовать простые typed definitions + validation.

---

# 24. Save / Load

Snapshot versioned с первого дня.

Пример:

```json
{
  "schemaVersion": 1,
  "protocolVersion": 1,
  "contentVersion": "village-v1",
  "seed": "...",
  "time": 123456,
  "world": {}
}
```

Save содержит authoritative state.

Не сохранять как authoritative: camera, animation frame, hovered UI element, open menu.

Load не создаёт hidden tick. После load одинаковые subsequent commands должны давать тот же result.

Не писать migration framework для несуществующих версий.

---

# 25. Domain Events

Events нужны для player feedback, debugging, scenarios/tests, projections, causal explanation.

Примеры: TaskSelected, TravelStarted, Arrived, WorkCompleted, ItemTransferred, TradeCompleted, DebtCreated, ObligationFulfilled, OfficeGranted, OfficeRevoked, LawViolated, MagicCast, MagicFailed, HouseholdShortageStarted, RelationshipChanged.

Не превращать всё persistence в event sourcing без причины.

---

# 26. Debug surface

Dev client публикует read-only debug overlay / dump (не `window.__GAME_DEBUG__`).

Минимум: ready state, build id, protocol version, player presentation position, player semantic location, world time, nearby interactables, visible NPC ids, last authoritative events, current seed, current scenario, last command result, current screen/dialog state.

Debug surface не имеет methods для mutation authoritative state.

Для scenario setup использовать explicit test/scenario initialization до запуска мира, а не cheat functions из Godot debugger.

---

# 27. Godot playtest: anti-hang contract

Это критическое правило проекта. Цель — не зависнуть в editor/play и не убивать чужие процессы.

## 27.1. Единственная точка входа

Когда `tools/play.py` существует, запрещено для ordinary playtest напрямую вызывать произвольные `godot` / editor GUI / ручной `--script` вне этого supervisor.

Единственный разрешённый entry:

```bash
python tools/play.py --scenario <name>
```

До появления инструмента — не изобретать второй runner.

## 27.2. Singleton lock

`tools/play.py` берёт non-blocking OS file lock `.cache/play/godot.lock`.

Если lock занят: не ждать, не retry loop, не запускать второй Godot playtest, вывести `PLAYTEST BUSY`, non-zero exit, сообщить blocker.

## 27.3. One Godot process

В обычном scenario run — ровно один Godot process, один project path `godot/`.

Запрещены parallel Godot play workers и persistent user-data, кроме отдельно утверждённого test.

## 27.4. Timeouts mandatory

Никогда `timeout=0`. Базовые пределы (уточняются в scenario):

```text
Godot start           15 s
action/default         5 s
state wait             5 s
single scripted run   45 s hard wall-clock
```

## 27.5. No arbitrary sleeps

Ждать condition: debug ready, projection state, explicit event, screenshot checkpoint. Короткая задержка — только для intentional animation observation.

## 27.6. Outer watchdog

`tools/play.py` — supervisor со своей process group. Hard deadline, graceful terminate своей group, затем kill только своей group, diagnostics, освобождение lock.

Запрещено: `pkill godot`, `killall godot`, убивать чужие editor processes, чистить весь user Godot state.

## 27.7. Artifacts

```text
.cache/play/<run-id>/
  run.json
  stdout.log
  stderr.log
  final.png
  debug.json
```

Retention: success — последние N runs; failure — до явной cleanup command.

---

# 28. Test architecture

```text
many fast deterministic C++ tests
some protocol/native scenario tests
few Godot playtest scenarios
manual/exploratory playtest
```

Godot tests не заменяют Simulation Core tests. Simulation tests не заменяют игру.

## 28.2. GoogleTest policy

Independent, repeatable, deterministic, named по behavior, маленькие, без order dependence. Mocks редко. Core domain лучше тестировать реальными value objects/state.

## 28.3. CTest labels

`unit`, `sim`, `protocol`, `determinism`, `scenario`, `slow`.

Godot playtest не обязан быть CTest test, чтобы случайно не запускаться через общий `ctest`.

## 28.4. Sanitizers

Отдельный preset ASan+UBSan: при memory/UB-sensitive изменениях, перед milestone, при crash/corruption. Не после каждого UI tweak.

## 28.5. Headless core vs Godot

Load-bearing deterministic scenarios проверяются native C++ hash/result. Godot playtest доказывает, что тот же protocol виден игроку. Расхождение — blocker capability.

---

# 29. Локальные команды разработки

Единый front door (после создания tools):

```bash
python tools/dev.py configure
python tools/dev.py build
python tools/dev.py test --target sim
python tools/dev.py test --target protocol
python tools/dev.py test --target determinism
python tools/dev.py godot-import
python tools/dev.py check
python tools/play.py --scenario smoke
```

Thin orchestration над CMake/Godot, не скрытый custom build system. README показывает канонические команды.

Пока cmake/ninja не установлены, единственный честный статус bootstrap — «toolchain incomplete».

---

# 30. Проверка по риску

Минимально достаточная проверка текущей change.

### C++ rule change

```text
build affected target
-> focused gtest filter
-> deterministic scenario if relevant
-> affected Godot playtest if player-visible
```

### Godot-only presentation change

```text
open/import Godot project
-> affected playtest screenshot
```

### GDExtension / protocol boundary change

```text
native protocol tests
-> adapter build
-> Godot load of extension
-> playtest round-trip
```

### Persistence change

```text
save/load roundtrip
-> replay determinism
-> one player-facing save/load scenario
```

Не утверждать, что проверка passed, если она не запускалась. Отсутствующий cmake — не pass.

---

# 31. CI policy

CI — страховка, не управляющая машина разработки.

Когда нужен CI, ранний workflow один и bounded:

```text
checkout
-> install pinned toolchain/deps
-> configure
-> native build
-> fast unit/protocol/determinism tests
```

Не запускать full Godot playtest в CI, пока команда не доказала необходимость.

Не делать CI state-machine, validation branches, polling workflows, десятки matrix jobs раннего проекта.

Проверка CI — по verification skill хоста / AI Layer, не по отдельному STOP-протоколу этого ТЗ.

---

# 32. Инженерный workflow (не control-plane)

AI Layer задаёт, *когда* work/task живёт и как continue. Это ТЗ задаёт, *как* менять игру.

Каждый implementation pass:

```text
inspect relevant code/tests/docs
-> minimal coherent vertical slice
-> self-review diff
-> targeted tests that actually ran
-> if gameplay affected: one bounded playtest
-> commit/push only when user permitted
-> short evidence in the host response
```

## 32.1. Scope declaration

Соблюдать, даже если не печатать длинный план:

```text
IN SCOPE   — files/subsystems needed for current capability
OUT OF SCOPE — adjacent tempting work
```

## 32.2. No speculative architecture

Перед новой abstraction: какой current problem; есть ли второй use case; можно ли проще; не растёт ли cognitive load без payoff.

## 32.3. No silent contract invention

Читать существующий header/schema/test перед использованием API. Не выдумывать function names, event schemas, save fields, tool flags, content contracts.

## 32.4. Two-attempt diagnose rule

После двух осмысленных неудач той же гипотезы — не random retry. Новые факты или смена гипотезы; после третьей эквивалентной неудачи — blocker и диагностика. Это совпадает с AI Layer bootstrap, не заменяет его.

---

# 33. Документация

Карта: [`docs/INDEX.md`](../INDEX.md). Один канонический источник на факт; остальные файлы ссылаются, а не копируют версии toolchain. How/how-not стека: [`docs/engineering/STACK.md`](../engineering/STACK.md).

| Путь | Роль | Статус |
| --- | --- | --- |
| [`docs/specs/TZ.md`](TZ.md) | продуктовый/архитектурный/инженерный контракт | есть |
| [`docs/INDEX.md`](../INDEX.md) | карта документов | есть |
| [`docs/engineering/STACK.md`](../engineering/STACK.md) | how/how-not по C++ / Godot / GDExtension / CMake / Python | есть |
| [`docs/decisions/`](../decisions/) | дорогие ADR | есть 0001 |
| [`AGENTS.md`](../../AGENTS.md) | pointer агентам, без AI Layer procedure | есть |
| [`README.md`](../../README.md) | вход и честный toolchain | есть |
| `docs/GAME.md` | только актуальное playable | создать, когда появится игра |
| `docs/ARCHITECTURE.md` | dependency direction / protocol / GDExtension | создать вместе с кодом |
| `docs/MODELING_POLICY.md` | historical baseline, magic, fidelity ladder | извлекать из этого ТЗ по мере нужды |
| `docs/models/<mechanic>.md` | Model Contract | по серьёзной mechanics |

Не создавать `docs/AGENT_RULES.md` — это конкурировало бы с AI Layer.

---

# 34. Performance policy

Не оптимизировать воображаемый масштаб.

На старте измерять: simulation step latency, NPC decisions per step, allocations, projection serialization, GDExtension command round-trip, save/load time, render frame time, active actors.

Не вводить без профиля: ECS framework, multithread job system, regional LOD, distributed simulation, database sharding, networking architecture.

---

# 35. Long-horizon simulation

Long-horizon tests нужны только для systems с feedback loops: demographics, fertility/mortality, resource depletion, debt, wealth concentration, market/production feedback, political power concentration, magical accumulation, institutional evolution.

Long run обязан объяснять trajectory через events/metrics. «Не упало за 100 лет» — не достаточный test.

---

# 36. Anti-overmodeling checklist

Перед новой subsystem:

1. Что игрок увидит или сможет сделать?
2. Какой observable result сейчас неверен без subsystem?
3. Можно ли решить F1-моделью из 20% сложности?
4. Как проверить её сегодня в playable build?
5. Какие данные мы намеренно не моделируем?
6. Не создаём ли мы два источника истины?
7. Не требует ли она будущих abstractions, которых пока нет use case?

Если ответы слабые — subsystem не добавлять.

---

# 37. Первый playable vertical slice

Первый slice — маленькая игра вокруг одного дефицита и одного магического counterfactual.

## Мир

небольшая деревня; несколько households; несколько NPC; дома; одно production place; одно exchange/social place; одна short resource chain; один локальный институт/authority; player character.

## Игрок может

ходить; разговаривать; видеть локальную проблему; носить предметы; работать/помогать; совершить простую trade/transfer; использовать одну magic capability; получить persistent consequence; увидеть хотя бы одну social/institutional реакцию.

## NPC могут

иметь need/obligation; выбирать feasible task; перемещаться; работать; переносить/потреблять/производить resource; реагировать на shortage; отказываться от невозможного.

## Критерий

Через 10–20 минут игрок понимает: кто здесь живёт; что происходит без него; где проблема; что он может предпринять; почему варианты отличаются; что изменилось после выбора.

---

# 38. Roadmap: слои развиваются одновременно

Каждый milestone должен быть playable.

## Milestone 0 — Toolchain & Playable Spine

- установить cmake + ninja;
- C++23 native `sim_core`;
- CMake/Ninja presets;
- Godot 4.7 2D project;
- pinned godot-cpp + GDExtension loads;
- protocol round-trip;
- WASD через InputMap → authoritative command;
- singleton `tools/play.py`;
- screenshot / debug projection;
- one deterministic gtest.

Критерий:

> Игру можно открыть в Godot, походить и доказать, что движение/состояние пришли из C++ core, а не из GDScript.

## Milestone 1 — Living Need

one NPC need; one causal task; travel/action; visible result; player can interfere/help.

> NPC делает что-то по причине состояния мира, и игрок может изменить outcome.

## Milestone 2 — Household Resource Loop

household stock; production; consumption; shortage; player trade/gift/work response.

> Деревня может попасть в проблему без игрока; игрок может решить её разными путями.

## Milestone 3 — Social Consequence

trust/obligation or reputation dimension; interaction changes future opportunity; NPC remembers relevant event.

> Одно и то же действие завтра доступно/недоступно из-за вчерашнего поведения.

## Milestone 4 — First Institution / Politics

one office/authority; one real permission/obligation; one way to gain/lose/influence it; visible distributional consequence.

> Игрок может участвовать во власти не через quest flag, а через world state.

## Milestone 5 — First Magic Counterfactual

one magic capability; explicit access/cost; non-magic alternative; economic/social/institutional downstream effect.

> Магия меняет system trajectory, а не только VFX.

## Milestone 6 — Emergent Role

enough compositional state that player can become worker/trader/apprentice/office-holder through different paths; UI reflects role as projection.

> По крайней мере две разные «карьеры» возникают из world rules без hardcoded class.

## Milestone 7 — Persistence & Repeated Play

save/load; replay; 30–60 minute session; gameplay fixes from actual play.

> Игру хочется продолжить, а мир сохраняет понятные последствия.

---

# 39. Definition of Done для capability

Capability готова только если:

- есть authoritative C++ implementation;
- Godot не содержит bypass truth;
- protocol явно выражает input/output;
- deterministic behavior проверен там, где требуется;
- targeted regression test существует для важной причинности;
- player-facing feedback существует;
- capability реально вызвана в игре;
- screenshot/debug state подтверждают ожидаемый outcome;
- не добавлен unrelated refactor;
- docs/model обновлены только если изменился реальный contract.

Compile green сам по себе не является DoD. Unit tests green сами по себе не являются DoD для gameplay.

---

# 40. Правила playtest

После player-facing изменения нужен короткий bounded playtest на конкретный вопрос текущей capability.

Плохо: запустить игру и оставить на минуту.

Хорошо: spawn shortage scenario → walk to miller → open interaction → trade grain → observe household shortage change → capture screenshot → inspect last authoritative events.

Не исследовать соседние mechanics во время этого run, если они не нужны текущей задаче.

---

# 41. Что не делать в начале

Не начинать с:

- combat framework;
- 3D open world;
- giant procedural world;
- 100 professions;
- universal skill tree;
- MMO networking;
- LLM NPC;
- universal GOAP planner;
- detailed genetics;
- full body metabolism;
- global political simulator;
- full religion simulator;
- universal magic ontology;
- weather physics;
- microservices;
- database cluster;
- custom scripting language;
- generic plugin ecosystem;
- ECS framework;
- full event sourcing;
- TypeScript/WASM revival;
- C# gameplay layer.

Любая из них может появиться после доказанного player-facing need и ADR.

---

# 42. Git workflow

Trunk-first.

- `main` playable;
- coherent bounded capability → one meaningful commit;
- feature branch только для реально опасного/длинного изменения;
- commit только по просьбе пользователя;
- commit message описывает новое behavior.

Хорошо: `Let villagers seek reachable food when household stores run low`.

Плохо: `Refactor`, `Update files`, `Phase 2`, `Fix tests`.

---

# 43. Evidence, не второй report protocol

Хост уже обязан быть кратким. Не печатать ритуальный баннер.

Нужно явно разделить:

```text
VERIFIED     — команда/проверка, которая реально ran, и результат
NOT VERIFIED — что не запускалось (например cmake отсутствует)
BLOCKERS     — только реальные blockers
```

Для gameplay: scenario, input, что было видно (screenshot/debug dump).

Не писать «полностью готово», если проверена только часть.

---

# 44. Первый обязательный development step

Не строить сначала архитектурный skeleton без игры.

Первый step — **Playable Spine**:

1. установить cmake + ninja;
2. минимальный CMake project;
3. C++23 `sim_core`;
4. explicit seeded deterministic state;
5. один `MoveIntent`;
6. native test через GoogleTest/CTest;
7. Godot 4.7 2D project;
8. pinned godot-cpp + GDExtension;
9. минимальный protocol adapter;
10. InputMap WASD → command;
11. read-only debug overlay/dump;
12. `tools/play.py` singleton/watchdog;
13. реальный Godot run;
14. screenshot после движения;
15. доказательство, что movement/state пришли из authoritative core.

Step не завершён, пока playtest не доказал round-trip **или** честно зафиксирован blocker (нет cmake, extension не грузится, и т.д.).

Не восстанавливать удалённый Fenster/web стек как промежуточный клиент.

---

# 45. Как читать этот документ в новом чате

Роль: ведущий инженер и systemic game designer. Оптимизировать скорость получения причинно корректной **играбельной** RPG, не количество кода.

Приоритеты: playable main; causal correctness; one authoritative C++ simulation; player/NPC rule symmetry; visible consequences; vertical slices; historical baseline; coherent magic counterfactuals; determinism; maintainable architecture; performance only after measurement.

Обязательные правила:

- C++23 — authoritative Simulation Core.
- Godot 4 — presentation/client, не second simulation.
- GDExtension — единственный Godot↔sim seam.
- Каждая gameplay capability проходит rule → contract → experience → proof.
- `main` всегда playable.
- Не уходи в simulation-only tunnel.
- Не строй subsystem глубже, чем её можно проверить в текущей игре.
- Не создавай hardcoded PlayerClass как источник прав.
- История задаёт non-magic baseline; магия явно изменяет baseline law.
- Не моделируй то, что пока не меняет observable gameplay.
- Не выдумывай API: читай текущий contract/tests.
- Godot playtest — через `tools/play.py`; никогда `pkill godot`.
- Workflow/continuation — AI Layer, не секция «продолжай» этого файла.

---

# 46. Финальный критерий архитектуры

Архитектура считается хорошей не если она максимально универсальна, а если новый gameplay slice можно провести через неё без обхода правил.

Хороший вопрос:

> Насколько легко добавить одну новую причинную возможность игрока и увидеть её настоящее системное последствие?

Плохой вопрос:

> Насколько много будущих неизвестных use cases наш framework якобы уже умеет абстрагировать?

Если архитектура делает Simulation Core красивее, но замедляет получение playable consequence — она подозрительна.

Если client ускоряет разработку, но создаёт второй authoritative state — он неправильный.

Если модель исторически богата, но игрок не может понять или затронуть её — её fidelity опережает игру.

---

# 47. Definition of Success проекта

Проект движется правильно, если через последовательность маленьких bounded slices игрок всё больше может жить, работать, торговать, владеть, учиться, колдовать, договариваться, обманывать, помогать, конфликтовать, вступать в организации, получать и терять статус, влиять на институты, менять экономические потоки, становиться человеком с реальной ролью в мире,

и всё это является следствием одного общего authoritative world state, а не набора специальных игровых исключений.

**Главный принцип проекта:**

> Симуляция не предшествует игре. Игра является способом непрерывно строить, проверять и углублять симуляцию.
