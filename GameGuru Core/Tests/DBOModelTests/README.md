# Production DBO decoder regressions

This executable links `Guru-WickedMAX/DBOModel.cpp` directly. It does not reimplement
the decoder and does not need a graphics device. Fixtures construct wire bytes
independently using the production `Shared/DBOFormat/DBOBlock.cpp` writer as the
format reference.

From **GameGuru Core**, with Visual Studio 18 Community's MSBuild:

```powershell
$msbuild = 'C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe'
& $msbuild Tests/DBOModelTests/DBOModelTests.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64
& Tests/DBOModelTests/bin/Debug/DBOModelTests.exe
& $msbuild Tests/DBOModelTests/DBOModelTests.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64
& Tests/DBOModelTests/bin/Release/DBOModelTests.exe
```

Each invocation checks geometry, frame sibling/parent relationships, material
defaults, bones, texture framing, declarations, LODs, subframes, custom bytes,
linked animation tracks/sets, quaternion/matrix/baked animation and interpolation
cache rebuilding. It rejects truncated prefixes, oversized counts, invalid
indices/strides/names/draw ranges, non-finite authored attributes, malformed
terminators and exhausted depth/allocation budgets. It also runs 10,000
deterministic mutated-fixture decodes and checks output atomicity and successful
geometry invariants.

Pass an asset directory to additionally walk a real corpus:

```powershell
& Tests/DBOModelTests/bin/Release/DBOModelTests.exe '..'
& Tests/DBOModelTests/bin/Release/DBOModelTests.exe '..' --strict-corpus
```

The default corpus protocol reports authored non-finite texture coordinates as
expected invalid input, using the error code and exact diagnostic, without a
filename exception. Any other rejection fails the run. `--strict-corpus` fails
on **every** rejected file, including this expected invalid-data class; it is an
asset audit rather than a decoder acceptance gate. The current 102-file corpus
has 101 accepted files and one rejected file, `gasmask.dbo`, whose first mesh has
NaN UVs (first invalid byte: 18857). Rejected data is not silently repaired.

For MSVC AddressSanitizer:

```powershell
& $msbuild Tests/DBOModelTests/DBOModelTests.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:Sanitize=true
& Tests/DBOModelTests/bin/Release-ASAN/DBOModelTests.exe '..'
```

Debug uses `/MTd /Od`; Release uses `/MT /O2`; both enable `/W4 /WX` and symbols.
ASAN uses separate output/intermediate directories. These tests establish binary
decoding and owned-data correctness, not rendering or full application behavior.

## Legacy writer quirks exercised

- Vertex declaration chunk 120 advertises size 4 but writes 65 eight-byte elements;
  the shipped MAX declaration terminator is stream 255 (type may be zero).
- Texture channel mask chunk 154 advertises size 4 but writes one byte.
- Version 1 mesh scalar chunks 117–119 advertised index-buffer byte lengths;
  version 1 object streams can end after the animation container without an
  additional root terminator. Newer versions require the root terminator.
- Position/scale/matrix interpolation fields are derived caches, with the final
  cache historically uninitialized. The decoder validates authored keys and
  rebuilds all derivative caches from adjacent time/value pairs.
- Animation-set start/finish chunks contain float bit patterns, not integers.
