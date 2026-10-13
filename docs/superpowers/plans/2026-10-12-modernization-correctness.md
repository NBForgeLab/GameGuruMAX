# Modernization correctness implementation plan

> Execute with subagent-driven-development and independent-domain parallel investigation. Preserve the uncommitted work; no commit, push, or reset is part of this task.

**Goal:** Resolve all seven reviewed defects with production implementations and evidence.

**Architecture:** A GPU-independent validated DBO model decoder feeds real scene construction. A functioning compatibility adapter preserves existing callers while native engine state owns geometry and animation. Production APIs, rather than independent simulations, are the test subjects.

**Tech stack:** Installed MSVC, C++17, doctest, Lua 5.2, existing Wicked Engine and Assimp integrations.

## 1. Build interfaces (controller)
- [ ] Reproduce Release compilation failures using MSBuild on GameGuruWickedMAX.sln, recording output outside the source tree.
- [ ] Remove engine-header coupling from shared wickedcalls.h; retain correctly typed engine-facing declarations where definitions have engine includes.
- [ ] Apply C++17 to Text and other production consumers of mutable string data/string_view; build affected projects with SolutionDir supplied.
- [ ] Correct further production build integration failures revealed by the real build, without adding blanket unresolved-symbol stubs.

## 2. DBO decoding (decoder implementer)
- [ ] Derive framing, chunk values, vertex declarations, hierarchy and animation layouts from Shared/DBOFormat's actual writer/reader and shipped assets.
- [ ] Add independent valid/corrupt fixture tests that fail against the unsafe implementation or demonstrate the missing production decoder contract.
- [ ] Add focused DBOModel.h/.cpp exposing owned decoded geometry, frames, bones, materials and animation; no GPU dependency.
- [ ] Validate enclosure lengths, count/stride arithmetic, attribute/index bounds, recursion and allocation budgets before allocation. All reads use bounded memcpy.
- [ ] Test shipped files, every truncation boundary, invalid counts/strides/indices, and recursion limits. Reject malformed input without partial scene mutation.

## 3. Scene integration and compatibility (integration implementer)
- [ ] Audit the actual object/mesh/limb/animation callers and proven implementations.
- [ ] Replace StandaloneDBOReader's empty importer branches and blanket no-op compatibility implementations with real behavior using the installed engine and existing importers.
- [ ] Preserve required compatibility metadata and lifetime invariants; do not use placeholder geometry or empty success objects.
- [ ] Use the validated decoder model for scene construction, local transforms, textures, skeleton bindings, animation tracks and LODs.
- [ ] Implement real play/loop/frame/speed/frame-count semantics and safe clone/delete behavior, verified on actual components.

## 4. Production string/Lua tests (API test implementer)
- [ ] Compile real cStr and real safe Lua/string message code in a headless production test target; extract reusable production code as needed.
- [ ] Replace ModernString/SafeLua/message simulation assertions with production API calls.
- [ ] Add regressions for bounded copies, absent/nil values, embedded NULs, long strings/messages, mutable/empty string storage and relevant cStr compatibility.
- [ ] Run focused tests in Release and Debug and report exact output and remaining dependencies.

## 5. Decals and gate integration (controller)
- [ ] Make decal creation a single registered operation, using the intended dimensions, atlas UVs, material and object-ID lifetime.
- [ ] Integrate decoder and scene tests into the master test gate; retain configuration audits as audits.
- [ ] Exclude generated build outputs from source packaging using narrowly scoped ignore rules, without deleting user files.
- [ ] Build the production solution in Release and Debug; run production tests and parser adversarial tests, with AddressSanitizer where supported.
- [ ] Review the resulting implementation and fix findings; repeat only affected checks after corrections.
- [ ] Record what is proven by automated tests and what still requires an application rendering check. Do not overstate merge readiness.
