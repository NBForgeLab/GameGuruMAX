# DBO compatibility integration tests

This executable links the production bounded decoder and metadata adapter with
the real `DBOData.cpp` constructors/destructors and KMaths implementations. It
contains no fake renderer, importer, symbol assembly, or Basic3D library.

Always-on checks exercise copied raw vertices and indices, hierarchy/local
matrix, material and texture names, bone influences, LOD, TRS/matrix/baked bone
animation data, custom data, subframe ownership, failure rollback, and long-list
teardown. Debug additionally checks CRT heap snapshots across 128 ownership
cycles. Neither configuration runs GPU/native scene registration or playback;
those remain product/engine integration checks.

Build and run from a Visual Studio developer PowerShell:

```powershell
msbuild DBOCompatibilityTests.vcxproj /p:Configuration=Release /p:Platform=x64
.\bin\Release\DBOCompatibilityTests.exe
msbuild DBOCompatibilityTests.vcxproj /p:Configuration=Debug /p:Platform=x64
.\bin\Debug\DBOCompatibilityTests.exe
```

Both configurations deliberately retain whole program optimization to remove
unreachable legacy shader code from this headless executable. Debug uses the
Debug CRT and PDBs; checks remain enabled in both configurations.

For AddressSanitizer, use the installed ClangCL toolset, lld, and that toolset's
actual ASan runtime directory. For example, with Visual Studio 18's LLVM 22:

```powershell
$asanRuntime = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\lib\clang\22\lib\windows'
msbuild DBOCompatibilityTests.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=ClangCL /p:UseLldLink=true /p:Sanitize=true "/p:AsanRuntimeDir=$asanRuntime" /p:OutDir=bin\Asan\ /p:IntDir=obj\Asan\
Copy-Item "$asanRuntime\clang_rt.asan_dynamic-x86_64.dll" .\bin\Asan\
.\bin\Asan\DBOCompatibilityTests.exe
```

The production loading path is `LoadObject -> LoadDBO ->
DBOConvertBlockToObject -> dbo::DecodeCompatibilityObject -> owned sObject`.
The established object finalization, Wicked scene registration, textures,
animation playback, transforms, cloning and deletion remain in the explicitly
compiled compatibility source units. Non-DBO assets use the installed Assimp
importer. The unused separate StandaloneDBOReader parser/scene builder was
removed, so no alternate unsafe decoding or empty-success importer remains.
