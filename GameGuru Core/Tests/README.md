# GameGuru MAX — Product Unit Tests

Unit tests for the product side of GameGuru MAX. This layer did not exist
before: there were zero test projects, zero frameworks and zero automated
checks between a successful build and a Steam upload. This project is the
foundation of that gate.

## What is tested today

The first suite targets the **product's own Lua gameplay scripts** — the
hundreds of behavior scripts shipped to every customer — running on the exact
Lua 5.2 runtime the game uses (compiled directly from the vendored
`DarkLUA\lua` sources, so there are no new third-party dependencies).

* **Script behavior** (`lua_script_tests.cpp`): `utillib.lua` is loaded into a
  stubbed Lua host and its functions are exercised: 3D point rotation,
  distance checks, the collectables/foraging API (`ChangeAmount`,
  `SetAmount`, `HaveAmount`, `HaveEnough`), player-proximity helpers,
  entity proximity queries, closest-entity searches from the player, sorted
  iteration, the player-eyeline raycast helpers (with recorded stub calls
  asserting eye height while standing vs ducked) and the bearing/FOV checks
  of `PlayerLookingNear` including their wrap-around cases. Engine entry
  points referenced by the scripts are small faithful stubs registered in
  `LuaHost.cpp`, and the stub records its calls so tests can assert *how* a
  script uses the engine.
* **Master interpreter** (`lua_masterinterpreter_tests.cpp`): the product's
  visual behavior engine (`masterinterpreter.lua`). Two contracts are pinned:
  the **behavior bytecode loader** — the compiled `.byc` file format
  (magic number, versions 101/102, states with interrupt flags, instruction
  fields with their string/number split and -1 offsets) that the editor
  exports for every shipped behavior — and the **condition evaluator**, the
  decision brain mapping condition ids to results from behavior state:
  parameter resolution (`"50"` vs `"=variablename"`), damage/health
  thresholds, once-per-threshold timer semantics, start-position distance
  checks (within/beyond) and the always/isvaluezero/random edge cases. The
  condition id constants are asserted against the shipped numbering because
  compiled behaviors embed them.
* **Module behavior** (`lua_module_misclib_tests.cpp`): `module_misclib.lua`
  is loaded through the real `require()` machinery (the Lua host opens the
  package library and points it at the product's script banks), and its
  `pinpoint()` selection flow is exercised against recorded engine stubs:
  outline toggling, emissive highlight and restore, the icon mode and the
  near-miss dot pointer.
* **Script syntax net** (`lua_syntax_tests.cpp`): every `.lua` file shipped
  under `Scripts\` (scriptbank, titlesbank and any future bank) must compile
  (load-only, nothing is executed). This is the cheapest possible regression
  net against corrupted or half-saved scripts reaching customers. UTF-8 BOMs
  are tolerated, matching the game's own loader.

## Bugs this suite has already caught

* `utillib.lua` `HaveEnough()` indexed the nil module list and raised a Lua
  error when called before `SetList()` — fixed (guard added, consistent with
  `HaveAmount`), with a permanent regression test.
* `utillib.lua` `RandomOffsetPos()` passed its arguments to `RandomPos()`
  swapped: the interpolated center position became the random radius and the
  radius became a center coordinate, scattering spawn points far outside the
  intended area. The function had no callers, so the fix (restoring the
  documented `RandomPos(dist, x, z)` argument order) is safe, and a
  regression test pins the documented behavior.

* **Cross-repo integration contract** (`WickedContractTests/`, separate
  project registered in the solution): compiles the **real**
  `Guru-WickedMAX/wickedcalls.cpp` translation layer (function-level linking
  via `/Gy` keeps only the code paths under test) together with the sibling
  `WickedRepo` engine headers — exactly as the shipped build does, including
  the `GGREDUCED` define. Pinned today: image path resolution relative to
  the product's `Files\` root (`WickedCall_GetRelativeAfterRoot`), the
  image-list bookkeeping (`WickedCall_AddImageToList`,
  `WickedCall_FindImageIndexInList` with the master-object 50000..70000
  convention), `WickedCall_InitImageManagement` reset semantics and the
  engine header-inline API GameGuru calls. If the sibling layout drifts or
  the translation layer changes behavior, this suite fails in the gate.

* **Lua performance and memory** (`lua_performance_tests.cpp`): the Lua
  heap is measured with `collectgarbage("count")` (after forced full
  collections) around steady-state cycles — 20k condition evaluations,
  30x behavior bytecode loads, 20k foraging calls — and must stay flat
  (<64 KB). Hot entry points carry generous time budgets: proximity
  checks, rotation math, condition evaluation and 500-instruction
  behavior loads.

## Framework

[doctest](https://github.com/doctest/doctest) **v2.4.12**, vendored under
`Tests\third_party\doctest\` (pinned, MIT licensed). Single header, no
dependencies, fastest compile times in class.

## Layout

```
GameGuru Core/Tests/
├── README.md                        this file
├── run_unit_tests.bat               build + run + exit-code gate
│                                    (Release: all suites + speed budgets;
│                                     Debug: additionally the engine CRT
│                                     leak layer from the sibling repo)
├── third_party/doctest/             pinned framework + license
├── WickedContractTests/             cross-repo integration contract tests
│                                    (compiles the real wickedcalls.cpp)
└── GameGuruUnitTests/
    ├── GameGuruUnitTests.vcxproj    console project, registered in
    │                                GameGuruWickedMAX.sln
    ├── main.cpp                     doctest entry point
    ├── LuaHost.h/.cpp               headless Lua 5.2 host: opens only the
    │                                pure standard libs, stubs the engine
    │                                entry points, finds Scripts\ root
    ├── lua_script_tests.cpp         utillib.lua behavior
    └── lua_syntax_tests.cpp         compile-check of all product scripts
```

## Running the tests

```
:: from GameGuru Core\
run_unit_tests.bat            (build + run, Release; non-zero exit on failure)
run_unit_tests.bat Debug
```

Or inside Visual Studio: build and run the `GameGuruUnitTests` project from
`GameGuruWickedMAX.sln`. Scripts are located automatically by walking up from
the executable; set `GGMAX_SCRIPTS_ROOT` to override.

## Deployment gate

`AI-CompileBuildDeploy.bat` calls `run_unit_tests.bat` after the
`ReleaseForSteam` build and **aborts the deploy with a non-zero exit code if
any test fails** — no more path between a successful compile and a customer
build that skips verification entirely.

## Conventions for new tests

1. One file per area, named `<area>_tests.cpp`; add it to the `.vcxproj`.
2. Test names describe behavior, e.g. `"utillib collectables: HaveEnough must
   not crash before any list is set"`.
3. When a script needs engine state, prefer stubbing at the Lua-C boundary
   (register a stub global) — never execute engine code headless.
4. New product scripts with pure, testable logic are excellent targets: load
   with `LoadScriptAs`, call with `PushModuleFunction`, assert on the stack.
5. The syntax net grows automatically with the script bank; no maintenance
   needed when scripts are added.
