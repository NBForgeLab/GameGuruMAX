#include "../../Guru-WickedMAX/DBOModel.h"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
using Bytes = std::vector<uint8_t>;
void u32(Bytes &b, uint32_t x) {
  for (int i = 0; i < 4; i++)
    b.push_back(uint8_t(x >> (i * 8)));
}
void append(Bytes &b, const Bytes &p) { b.insert(b.end(), p.begin(), p.end()); }
Bytes scalar(uint32_t x) {
  Bytes b;
  u32(b, x);
  return b;
}
void chunk(Bytes &b, uint32_t id, const Bytes &p) {
  u32(b, id);
  u32(b, uint32_t(p.size()));
  append(b, p);
}
void end(Bytes &b) {
  u32(b, 0);
  u32(b, 0);
}
Bytes str(const char *s) {
  Bytes b;
  u32(b, uint32_t(strlen(s)));
  b.insert(b.end(), s, s + strlen(s));
  return b;
}
Bytes triangle() {
  Bytes mesh;
  chunk(mesh, 111, scalar(0x112));
  chunk(mesh, 112, scalar(32));
  chunk(mesh, 113, scalar(3));
  chunk(mesh, 114, scalar(3));
  Bytes vertices(96);
  float p[3][8] = {{0, 0, 0, 0, 1, 0, 0, 0},
                   {1, 0, 0, 0, 1, 0, 1, 0},
                   {0, 1, 0, 0, 1, 0, 0, 1}};
  memcpy(vertices.data(), p, 96);
  chunk(mesh, 115, vertices);
  chunk(mesh, 116, Bytes{0, 0, 1, 0, 2, 0});
  end(mesh);
  return mesh;
}
Bytes fixture() {
  Bytes frame;
  chunk(frame, 101, str("root"));
  chunk(frame, 103, triangle());
  Bytes child;
  chunk(child, 101, str("child"));
  Bytes sibling;
  chunk(sibling, 101, str("sibling"));
  end(sibling);
  chunk(child, 105, sibling);
  end(child);
  chunk(frame, 104, child);
  end(frame);
  Bytes b = str("MAGICDBO");
  u32(b, 1062);
  u32(b, 0);
  u32(b, 0);
  chunk(b, 1, frame);
  Bytes a;
  end(a);
  chunk(b, 2, a);
  end(b);
  return b;
}
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
Bytes animationFixture() {
  auto b = fixture();
  b.resize(b.size() - 24);
  Bytes track;
  chunk(track, 211, str("child"));
  chunk(track, 216, scalar(1));
  Bytes keys;
  u32(keys, 12);
  for (int i = 0; i < 3; i++)
    u32(keys, 0x3f800000);
  for (int i = 0; i < 3; i++)
    u32(keys, 0x7fc00000);
  chunk(track, 217, keys);
  end(track);
  Bytes a;
  chunk(a, 201, str("walk"));
  chunk(a, 202, scalar(12));
  chunk(a, 203, track);
  end(a);
  chunk(b, 2, a);
  end(b);
  return b;
}
size_t findChunk(const Bytes &b, uint32_t id) {
  const auto pattern = scalar(id);
  for (size_t i = 24; i + 8 <= b.size(); ++i)
    if (memcmp(b.data() + i, pattern.data(), 4) == 0)
      return i;
  throw std::runtime_error("fixture chunk not found");
}
void set32(Bytes &b, size_t offset, uint32_t value) {
  auto p = scalar(value);
  memcpy(b.data() + offset, p.data(), 4);
}
Bytes wrapMesh(const Bytes &mesh) {
  Bytes frame;
  chunk(frame, 101, str("root"));
  chunk(frame, 103, mesh);
  end(frame);
  Bytes b = str("MAGICDBO");
  u32(b, 1062);
  u32(b, 0);
  u32(b, 0);
  chunk(b, 1, frame);
  Bytes animation;
  end(animation);
  chunk(b, 2, animation);
  end(b);
  return b;
}
Bytes floatBytes(float value) {
  uint32_t bits;
  memcpy(&bits, &value, 4);
  return scalar(bits);
}
Bytes matrixBytes() {
  Bytes b;
  for (size_t i = 0; i < 16; i++)
    append(b, floatBytes(i % 5 == 0 ? 1.f : 0.f));
  return b;
}
void completeModelTests() {
  auto mesh = triangle();
  mesh.resize(mesh.size() - 8);
  chunk(mesh, 503, scalar(3));
  chunk(mesh, 500, Bytes{2, 0, 1, 0, 0, 0});
  chunk(mesh, 121, scalar(1));
  Bytes bones;
  chunk(bones, 301, str("child"));
  chunk(bones, 302, scalar(1));
  chunk(bones, 303, scalar(1));
  chunk(bones, 304, floatBytes(0.75f));
  chunk(bones, 305, matrixBytes());
  end(bones);
  end(bones);
  chunk(mesh, 122, bones);
  chunk(mesh, 123, Bytes{1});
  chunk(mesh, 124, scalar(1));
  Bytes materials;
  chunk(materials, 161, str("paint.dds"));
  Bytes material;
  for (int i = 0; i < 17; i++)
    append(material, floatBytes(0.5f));
  chunk(materials, 162, material);
  chunk(materials, 163, scalar(0));
  chunk(materials, 164, scalar(3));
  chunk(materials, 165, scalar(1));
  end(materials);
  end(materials);
  chunk(mesh, 139, materials);
  chunk(mesh, 138, scalar(1));
  Bytes subframe;
  chunk(subframe, 101, str("subframe"));
  end(subframe);
  append(mesh, subframe);
  end(mesh);
  auto b = wrapMesh(mesh);
  b.resize(b.size() - 24);
  Bytes nextTrack;
  chunk(nextTrack, 211, str("second"));
  end(nextTrack);
  Bytes track;
  chunk(track, 211, str("child"));
  chunk(track, 212, scalar(2));
  Bytes keys;
  for (uint32_t time : {0u, 10u}) {
    u32(keys, time);
    for (int i = 0; i < 3; i++)
      append(keys, floatBytes(float(time)));
    for (int i = 0; i < 3; i++)
      u32(keys, 0x7fc00000);
  }
  chunk(track, 213, keys);
  chunk(track, 214, scalar(1));
  Bytes rotation = scalar(0);
  for (int i = 0; i < 4; i++)
    append(rotation, floatBytes(i == 3 ? 1.f : 0.f));
  chunk(track, 215, rotation);
  chunk(track, 218, scalar(1));
  Bytes matrixKey = scalar(0);
  append(matrixKey, matrixBytes());
  for (int i = 0; i < 16; i++)
    u32(matrixKey, 0x7fc00000);
  chunk(track, 219, matrixKey);
  chunk(track, 400, scalar(0));
  chunk(track, 402, scalar(1));
  chunk(track, 401, scalar(0));
  chunk(track, 403, scalar(1));
  chunk(track, 404, scalar(1));
  chunk(track, 405, matrixBytes());
  chunk(track, 220, nextTrack);
  end(track);
  Bytes secondSet;
  chunk(secondSet, 201, str("secondSet"));
  end(secondSet);
  Bytes animations;
  chunk(animations, 201, str("walk"));
  chunk(animations, 202, scalar(10));
  chunk(animations, 206, floatBytes(1.5f));
  chunk(animations, 207, floatBytes(9.5f));
  chunk(animations, 208, floatBytes(0.25f));
  chunk(animations, 203, track);
  chunk(animations, 205, secondSet);
  end(animations);
  chunk(b, 2, animations);
  Bytes custom = scalar(3);
  append(custom, Bytes{5, 6, 7});
  chunk(b, 406, custom);
  end(b);
  dbo::Model m;
  dbo::Error e;
  require(dbo::DecodeFromMemory(b.data(), b.size(), m, e),
          "complete model decoded");
  require(m.frames.size() == 2 && m.meshes[0].subFrames.size() == 1 &&
              m.meshes[0].subFrames[0] == 1,
          "subframe ownership retained");
  require(m.meshes[0].bones[0].name == "child" &&
              m.meshes[0].bones[0].vertices[0] == 1 &&
              m.meshes[0].bones[0].weights[0] == 0.75f,
          "bone influences retained");
  require(m.meshes[0].multiMaterials[0].name == "paint.dds" &&
              m.meshes[0].multiMaterials[0].material.power == 0.5f,
          "multiple materials retained");
  require(m.meshes[0].lodIndices[0][0] == 2 && m.customData == Bytes({5, 6, 7}),
          "LOD and custom data retained");
  require(m.animations.size() == 2 && m.animations[0].tracks.size() == 2 &&
              m.animations[0].tracks[0].positions[0].interpolation[0] == 1,
          "linked animation trees and derivatives retained");
  require(m.animations[0].start == 1.5f && m.animations[0].finish == 9.5f,
          "animation range is floating-point");
  require(m.animations[0].tracks[0].matrices[0].interpolation[0] == 0 &&
              m.animations[0].tracks[0].boneFrames.size() == 1,
          "matrix animation and baked bones retained");
}
void malformedTests() {
  dbo::Model m;
  dbo::Error e;
  auto valid = fixture();
  auto reject = [&](Bytes b, const char *reason) {
    m.version = 42;
    require(!dbo::DecodeFromMemory(b.data(), b.size(), m, e), reason);
    require(m.version == 42, "malformed input preserves output");
    require(e.code != dbo::ErrorCode::None, "malformed input has diagnostic");
  };
  auto bad = valid;
  set32(bad, 12, 9999);
  reject(bad, "unsupported version rejected");
  bad = valid;
  set32(bad, findChunk(bad, 113) + 8, 0xffffffff);
  reject(bad, "overflowing vertex count rejected");
  bad = valid;
  set32(bad, findChunk(bad, 112) + 8, 8);
  reject(bad, "attribute stride mismatch rejected");
  bad = valid;
  bad[findChunk(bad, 116) + 8] = 3;
  reject(bad, "out-of-range index rejected");
  bad = valid;
  set32(bad, findChunk(bad, 115) + 8 + 24, 0x7fc00000);
  reject(bad, "nonfinite UV rejected");
  require(e.code == dbo::ErrorCode::InvalidGeometry &&
              e.message == "non-finite texture coordinate",
          "nonfinite UV diagnostic identifies attribute");
  bad = valid;
  set32(bad, findChunk(bad, 115) + 8, 0x7f800000);
  reject(bad, "nonfinite position rejected");
  bad = valid;
  set32(bad, findChunk(bad, 101) + 8, 256);
  reject(bad, "fixed-name overflow rejected");
  bad = animationFixture();
  set32(bad, findChunk(bad, 216) + 8, 0xffffffff);
  reject(bad, "animation count overflow rejected");
  auto mesh = triangle();
  mesh.resize(mesh.size() - 8);
  chunk(mesh, 119, scalar(0xffffffff));
  end(mesh);
  reject(wrapMesh(mesh), "draw primitive count exceeding geometry rejected");
  bad = valid;
  bad.back() = 1;
  reject(bad, "nonzero terminator size rejected");
  dbo::Limits limits;
  limits.maxDepth = 0;
  require(!dbo::DecodeFromMemory(valid.data(), valid.size(), m, e, limits),
          "depth budget enforced");
  limits = {};
  limits.maxDecodedBytes = 1;
  require(!dbo::DecodeFromMemory(valid.data(), valid.size(), m, e, limits),
          "allocation budget enforced");
  limits = {};
  limits.maxFrames = 1;
  require(!dbo::DecodeFromMemory(valid.data(), valid.size(), m, e, limits),
          "frame budget enforced");
  require(!dbo::DecodeFromFile("missing-model-does-not-exist.dbo", m, e),
          "missing file rejected");
}
void writerQuirkTests() {
  dbo::Model m;
  dbo::Error e;
  // Channel masks advertise a DWORD but serialize a BYTE in the real writer.
  auto mesh = triangle();
  mesh.resize(mesh.size() - 8);
  chunk(mesh, 127, scalar(1));
  Bytes textures;
  chunk(textures, 141, str("diffuse.dds"));
  chunk(textures, 142, scalar(2));
  u32(textures, 154);
  u32(textures, 4);
  textures.push_back(7);
  end(textures);
  end(textures);
  chunk(mesh, 128, textures);
  end(mesh);
  auto b = wrapMesh(mesh);
  require(dbo::DecodeFromMemory(b.data(), b.size(), m, e),
          "real channel-mask framing decoded");
  require(m.meshes[0].textures[0].name == "diffuse.dds" &&
              m.meshes[0].textures[0].channelMask == 7,
          "texture fields preserved");
  // Declaration size lies too; stream255 is the product's end marker.
  mesh = triangle();
  set32(mesh, 8, 0);
  Bytes declaration(520);
  declaration[4] = 2;
  declaration[6] = 0;
  declaration[8 + 2] = 12;
  declaration[8 + 4] = 2;
  declaration[8 + 6] = 3;
  declaration[16 + 2] = 24;
  declaration[16 + 4] = 1;
  declaration[16 + 6] = 5;
  declaration[24] = 255;
  Bytes decl;
  u32(decl, 120);
  u32(decl, 4);
  append(decl, declaration);
  mesh.insert(mesh.begin() + 48, decl.begin(), decl.end());
  b = wrapMesh(mesh);
  require(dbo::DecodeFromMemory(b.data(), b.size(), m, e),
          "real declaration framing decoded");
  require(m.meshes[0].uv[2][1] == 1, "declaration geometry decoded");
  // Version1 omitted the object terminator and used index byte lengths for
  // scalar headers.
  mesh = triangle();
  mesh.resize(mesh.size() - 8);
  for (uint32_t id = 117; id <= 119; id++) {
    u32(mesh, id);
    u32(mesh, 6);
    u32(mesh, id == 117 ? 4 : id == 118 ? 3 : 1);
  }
  end(mesh);
  b = wrapMesh(mesh);
  set32(b, 12, 1);
  b.resize(b.size() - 8);
  require(dbo::DecodeFromMemory(b.data(), b.size(), m, e),
          "pre-U6 writer framing decoded");
}
void mutationTests() {
  std::mt19937 rng(1062);
  const auto base = animationFixture();
  dbo::Limits limits;
  limits.maxDecodedBytes = 4 * 1024 * 1024;
  limits.maxElements = 10000;
  for (size_t iteration = 0; iteration < 10000; iteration++) {
    auto b = base;
    for (size_t change = 0; change < 1 + iteration % 4; change++)
      b[rng() % b.size()] = uint8_t(rng());
    dbo::Model m;
    m.version = 42;
    dbo::Error e;
    if (dbo::DecodeFromMemory(b.data(), b.size(), m, e, limits)) {
      for (const auto &mesh : m.meshes) {
        for (const auto &p : mesh.positions)
          for (float v : p)
            require(std::isfinite(v), "mutation success has finite positions");
        for (auto index : mesh.indices)
          require(index < mesh.vertexCount,
                  "mutation success has valid indices");
      }
    } else {
      require(m.version == 42 && m.frames.empty(),
              "mutation failure is atomic");
      require(e.code != dbo::ErrorCode::None, "mutation failure has error");
    }
  }
}
int main(int argc, char **argv) {
  try {
    dbo::Model m;
    dbo::Error e;
    auto b = fixture();
    require(dbo::DecodeFromMemory(b.data(), b.size(), m, e),
            "valid triangle must decode");
    require(m.frames.size() == 3 && m.frames[1].parent == 0 &&
                m.frames[2].parent == 0,
            "siblings preserve parent");
    require(m.meshes[0].positions[1][0] == 1 && m.meshes[0].indices[2] == 2,
            "geometry is decoded");
    require(m.meshes[0].material.diffuse[0] == 1 &&
                m.meshes[0].material.ambient[0] == 1 &&
                m.meshes[0].specularOverride == 1,
            "absent optional fields preserve legacy white material defaults");
    Bytes unaligned(1, 0);
    append(unaligned, b);
    dbo::Model copied;
    require(dbo::DecodeFromMemory(unaligned.data() + 1, b.size(), copied, e),
            "unaligned caller buffers decoded with checked scalar copies");
    std::fill(unaligned.begin(), unaligned.end(), uint8_t(0));
    require(copied.meshes[0].positions[1][0] == 1 &&
                copied.frames[0].name == "root",
            "decoded model owns input bytes and strings");
    for (size_t i = 0; i < b.size(); i++) {
      dbo::Model sentinel;
      sentinel.version = 42;
      require(!dbo::DecodeFromMemory(b.data(), i, sentinel, e),
              "truncated prefix rejected");
      require(sentinel.version == 42 && sentinel.frames.empty(),
              "failure is atomic");
    }
    auto bad = b;
    bad[28] = 255;
    bad[29] = 255;
    bad[30] = 255;
    bad[31] = 127;
    require(!dbo::DecodeFromMemory(bad.data(), bad.size(), m, e),
            "oversized parent chunk rejected");
    auto animation = animationFixture();
    require(dbo::DecodeFromMemory(animation.data(), animation.size(), m, e),
            "authored animation survives uninitialized interpolation cache");
    require(m.animations[0].tracks[0].scales[0].interpolation[0] == 0,
            "last-key interpolation cache rebuilt");
    malformedTests();
    writerQuirkTests();
    completeModelTests();
    mutationTests();
    size_t assets = 0, animated = 0, bones = 0, textured = 0, rejected = 0,
           invalidUV = 0;
    if (argc > 1)
      for (const auto &p :
           std::filesystem::recursive_directory_iterator(argv[1]))
        if (p.path().extension() == ".dbo") {
          dbo::Model model;
          if (!dbo::DecodeFromFile(p.path().string(), model, e)) {
            std::cerr << p.path() << " offset " << e.offset << ": " << e.message
                      << "\n";
            if (e.code == dbo::ErrorCode::InvalidGeometry &&
                e.message == "non-finite texture coordinate")
              ++invalidUV;
            else
              ++rejected;
            continue;
          }
          ++assets;
          animated += !model.animations.empty();
          for (auto &mesh : model.meshes) {
            bones += !mesh.bones.empty();
            textured += !mesh.textures.empty();
          }
        }
    std::cout << "PASS fixtures/truncated prefixes; corpus=" << assets
              << " animated=" << animated << " skinned meshes=" << bones
              << " textured meshes=" << textured
              << " expected-invalid-UV=" << invalidUV
              << " unexpected-rejected=" << rejected << " mutations=10000\n";
    const bool strictCorpus =
        argc > 2 && std::string(argv[2]) == "--strict-corpus";
    return rejected || (strictCorpus && invalidUV) ? 2 : 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << "\n";
    return 1;
  }
}

