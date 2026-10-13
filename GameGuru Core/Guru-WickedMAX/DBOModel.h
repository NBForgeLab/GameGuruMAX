#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Graphics-device independent representation of the legacy DBO writer's data.
namespace dbo {
using Matrix = std::array<float, 16>;
using Vec3 = std::array<float, 3>;
struct Material {
  std::array<float, 4> diffuse{{1, 1, 1, 1}}, ambient{{1, 1, 1, 1}}, specular{},
      emissive{};
  float power = 0;
};
struct Texture {
  std::string name;
  uint32_t stage = 0, blendMode = 0, blendArg1 = 0, blendArg2 = 0, addressU = 0,
           addressV = 0, mag = 0, min = 0, mip = 0, coordinateMode = 0,
           primitiveStart = 0, primitiveCount = 0, channelMask = 0;
};
struct MultiMaterial {
  std::string name;
  Material material;
  uint32_t indexStart = 0, indexCount = 0, polygonCount = 0;
};
struct Bone {
  std::string name;
  Matrix inverseBind{};
  std::vector<uint32_t> vertices;
  std::vector<float> weights;
};
struct VertexElement {
  uint16_t stream = 0, offset = 0;
  uint8_t type = 0, method = 0, usage = 0, usageIndex = 0;
};
struct Mesh {
  uint32_t fvf = 0, stride = 0, vertexCount = 0, primitiveType = 4,
           drawVertexCount = 0, drawPrimitiveCount = 0;
  std::vector<uint8_t> vertexData;
  std::vector<VertexElement> declaration;
  std::vector<Vec3> positions, normals;
  std::vector<std::array<float, 2>> uv;
  std::vector<uint32_t> indices;
  std::array<std::vector<uint32_t>, 3> lodIndices;
  std::vector<Bone> bones;
  Material material;
  std::vector<Texture> textures;
  std::vector<MultiMaterial> multiMaterials;
  std::vector<size_t> subFrames;
  bool useMaterial = false, useMultiMaterial = false, visible = true,
       transparency = false, cull = true;
  bool light = true, fog = true, ambient = true, wireframe = false,
       linked = false, ghost = false, zRead = true, zWrite = true,
       zBias = false;
  uint32_t ghostMode = 0, arbitraryValue = 0, alphaTest = 0;
  float specularOverride = 1, zBiasSlope = 0, zBiasDepth = 0;
  std::string effectName;
};
struct Frame {
  std::string name;
  Matrix matrix{};
  Vec3 offset{}, rotation{}, scale{{1, 1, 1}};
  int parent = -1, mesh = -1;
};
struct VectorKey {
  uint32_t time = 0;
  Vec3 value{}, interpolation{};
};
struct RotationKey {
  uint32_t time = 0;
  std::array<float, 4> value{};
};
struct MatrixKey {
  uint32_t time = 0;
  Matrix value{}, interpolation{};
};
struct AnimationTrack {
  std::string name;
  std::vector<VectorKey> positions, scales;
  std::vector<RotationKey> rotations;
  std::vector<MatrixKey> matrices;
  bool boneType = true;
  uint32_t boneFrameCount = 0, boneCount = 0;
  std::vector<int32_t> boneOffsets;
  std::vector<Matrix> boneFrames;
};
struct AnimationSet {
  std::string name;
  uint32_t length = 0, type = 0;
  float start = 0, finish = 0;
  std::array<float, 3> steps{};
  std::vector<AnimationTrack> tracks;
};
struct Model {
  uint32_t version = 0;
  std::vector<Frame> frames;
  std::vector<Mesh> meshes;
  std::vector<AnimationSet> animations;
  std::vector<uint8_t> customData;
};
enum class ErrorCode {
  None,
  InvalidHeader,
  UnsupportedVersion,
  Truncated,
  InvalidChunk,
  InvalidGeometry,
  InvalidAnimation,
  ResourceLimit,
  FileIO,
  AllocationFailure
};
struct Error {
  ErrorCode code = ErrorCode::None;
  size_t offset = 0;
  std::string message;
};
struct Limits {
  size_t maxFileBytes = 512u * 1024u * 1024u,
         maxDecodedBytes = 1024u * 1024u * 1024u, maxElements = 16000000,
         maxFrames = 65536, maxDepth = 256;
};
// On failure output is unchanged. All input bytes are copied/decoded before
// return.
bool DecodeFromMemory(const void *bytes, size_t size, Model &output,
                      Error &error, const Limits &limits = Limits{});
bool DecodeFromFile(const std::string &filename, Model &output, Error &error,
                    const Limits &limits = Limits{});
} // namespace dbo
