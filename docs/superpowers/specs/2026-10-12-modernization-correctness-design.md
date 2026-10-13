# GameGuru MAX modernization: correctness and integration design

## Objective

Resolve the seven review findings without placeholder implementations, preserve the product's existing model and animation behavior, and verify the production code before declaring the changes ready for integration. This design covers the currently uncommitted changes in GameGuruMAX, using the sibling WickedRepo as the engine reference.

## Constraints and baseline

- Preserve existing user changes, including changes made during the review. Do not reset the working tree, merge, publish, or commit unrelated files.
- Preserve the stated removal of Basic3D.lib and the obsolete dependency list in PROJECT.md. Restoring that library is an alternative migration strategy, not the recommended implementation under the existing requirements.
- Use the installed engine version and toolchain; do not upgrade the engine or third-party dependencies as part of this repair.
- The previous Release test gate passed 120 test cases, but the production solution build exited with code 1. The new E2E string, DBO, and Lua tests currently exercise independent simulations. These are not evidence of production correctness.
- Build artifacts and logs under the untracked DarkLUA/DarkLua directory are generated outputs, not implementation source.

## Approaches considered

1. **Complete the native Wicked integration with an explicit compatibility adapter (recommended).** Keep callers working while replacing each required legacy operation with a real implementation and making native scene ownership explicit. This meets the dependency-removal requirement and preserves behavior. It requires more work than fixing compilation alone.
2. **Retain the proven legacy object subsystem until the migration is complete.** Remove the replacement stubs and restore the existing subsystem. This preserves behavior quickly, but postpones the requested Basic3D removal and is therefore a scope change.
3. **Repair individual stubs and leave the rest in place.** Rejected: this would keep silent functionality loss and misleading success paths in production.

## Architecture

### Build and public interfaces

Apply C++17 consistently to the projects that use mutable std::string storage and string_view. Keep engine-specific types out of shared headers consumed by SDK projects that do not depend on engine headers. Put native entity declarations in a dedicated engine-facing header, or use the engine's actual underlying entity representation at the shared ABI boundary after verifying it in wiECS.h. Do not solve missing includes by adding unnecessary engine dependencies to every SDK project.

### DBO parsing

Separate the binary decoder from scene construction. The decoder must not require a graphics device. Its output is an owned intermediate model containing frames, geometry, materials, skeletons, animation tracks, and LOD descriptions.

Use a bounded byte reader with checked integer arithmetic and memcpy-based scalar reads. Validate each chunk against its enclosing range, file length, element counts, stride, attribute layout, index range, and allocation budget before allocating or reading. Handle the actual DBO framing and terminator rules using the existing DBOBlock/DBOFormat writer as the format oracle. Enforce recursion and resource limits. Missing required vertex data, truncated chunks, unsupported versions, invalid indices, and malformed nesting return structured failures.

Parsing a rejected asset must leave the destination scene unchanged. Allocation or decoding failure must not escape as an uncaught exception through the asset-loading API.

### Scene construction and asset import

Build geometry, materials, textures, hierarchy, armatures, and animation from the validated intermediate model. Apply frame transforms to local transform state, preserving parent/sibling relationships. Fill the engine's complete armature mapping rather than assigning inverse bind matrices alone. Respect texture-loading options and asset-relative paths.

Route supported modern formats through the real import implementation available in this repository. Verify support per format instead of assuming that the installed engine provides glTF or FBX importers. Reuse the product's existing Assimp path where it is the authoritative importer. A load succeeds only when actual required geometry and components have been created. Unsupported formats and missing files return failure; they must not create empty success objects.

### Compatibility and animation

Inventory actual callers of the replaced object, limb, mesh, animation, bounds, transform, and visibility APIs. Move their real implementations into a focused adapter over native entities. The adapter must maintain stable object-ID lookup and all compatibility metadata still accessed by wickedcalls.cpp and gameplay code. Remove the blanket no-op replacement section.

Implement animation playback, looping, frame selection, speed, frame-count queries, bone attachments, and bounds using real engine state. Preserve the legacy caller's frame units and timing semantics. Verify cloning and instancing against ownership rules; do not alias destructible metadata or silently substitute unrelated geometry.

### Object ownership and decals

Track the entities and resources owned by a loaded model so reload, failure, clone, and deletion have explicit lifetime behavior. Decal creation must produce one object registered under its intended object ID, with its atlas UVs, material, dimensions, transform, and cleanup path. Remove the unused first creation in M-Entity.cpp and the second competing construction path. Do not leave orphan scene entities behind.

### Production tests

Replace the simulation-based correctness assertions with tests linked to the actual decoder, string implementation, and safe Lua APIs. Keep binary fixture generation independent from the decoder. Isolate reusable production code where necessary to enable headless tests; do not create test-only implementations of production algorithms.

Test valid shipped DBO assets and independent small fixtures; corrupt and truncated data; extreme counts; malformed attribute strides; hierarchy; materials; animation; modern-format import; cloning; deletion; and decal resource counts. Exercise the actual cStr contracts and actual Lua message/bounded-string paths. Keep useful project dependency audit tests, but distinguish them from runtime tests.

## Verification and acceptance

1. Reproduce each failure before changing the relevant implementation and add a regression test where behavior can be automated.
2. Build the production solution in Release and Debug for x64, with fresh logs and nonzero exit codes treated as failures.
3. Run the real product tests and integration tests in both configurations. An assertion failure or genuine runtime contract failure fails the gate; diagnostic script sweeps must not be described as successful runtime integration tests.
4. Run malformed-input and ownership tests under MSVC AddressSanitizer where the installed dependencies support instrumentation. State any instrumentation limitation explicitly.
5. Verify real model loading and animation in the application when an executable and suitable assets are available. Headless parser tests alone do not establish rendering correctness.
6. Review the complete resulting diff for scope, remaining no-ops, generated files, dependency consistency, and lifetime bugs. Rerun affected checks after review corrections.
7. Mark each of the seven original findings resolved only with the corresponding evidence. Do not declare merge readiness if production builds or required integration tests remain unverified.

## Reference guidance

- Microsoft standard-library documentation: https://learn.microsoft.com/en-us/cpp/standard-library/basic-string-class
- MSVC AddressSanitizer: https://learn.microsoft.com/en-us/cpp/sanitizers/asan
- The repository's existing DBO writer, object implementation, Assimp integration, and installed Wicked scene/animation APIs are the implementation-specific references.
