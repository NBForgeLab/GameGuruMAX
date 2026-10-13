#include "DBOCompatibility.h"
#include "DBOData.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

// Product globals are host state, not substitute implementations. The real
// metadata destructors inspect the sorting list when an object is destroyed.
int g_iSortedObjectCount = 0;
sObject **g_ppSortedObjectList = nullptr;

static size_t checkCount = 0;
#define CHECK(expression)                                                      \
  do {                                                                         \
    ++checkCount;                                                              \
    if (!(expression))                                                         \
      throw std::runtime_error("Check failed: " #expression);                  \
  } while (false)
static dbo::Matrix identity() {
  return {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
}
static dbo::Model model() {
  dbo::Model m;
  dbo::Frame root;
  root.name = "root";
  root.matrix = identity();
  root.mesh = 0;
  m.frames.push_back(root);
  dbo::Frame child;
  child.name = "bone";
  child.parent = 0;
  child.matrix = identity();
  child.matrix[12] = 7;
  m.frames.push_back(child);
  dbo::Mesh mesh;
  mesh.fvf = 0x002;
  mesh.stride = 12;
  mesh.vertexCount = 3;
  mesh.drawVertexCount = 3;
  mesh.drawPrimitiveCount = 1;
  const float positions[9] = {0, 0, 0, 2, 0, 0, 0, 3, 0};
  mesh.vertexData.resize(sizeof positions);
  std::memcpy(mesh.vertexData.data(), positions, sizeof positions);
  mesh.indices = {0, 1, 2};
  mesh.lodIndices[0] = {2, 1, 0};
  mesh.visible = false;
  mesh.light = false;
  mesh.wireframe = true;
  mesh.material.diffuse = {{.1f, .2f, .3f, .4f}};
  mesh.useMaterial = true;
  dbo::Texture texture;
  texture.name = "relative/albedo.dds";
  texture.addressU = 3;
  texture.channelMask = 8;
  mesh.textures.push_back(texture);
  dbo::Bone bone;
  bone.name = "bone";
  bone.inverseBind = identity();
  bone.vertices = {0, 1};
  bone.weights = {.25f, .75f};
  mesh.bones.push_back(bone);
  m.meshes.push_back(mesh);
  dbo::AnimationSet animation;
  animation.name = "walk";
  animation.length = 40;
  animation.finish = 40;
  dbo::AnimationTrack track;
  track.name = "bone";
  track.positions.push_back({0, {{0, 0, 0}}, {{0, 0, 0}}});
  track.positions.push_back({40, {{10, 0, 0}}, {{0, 0, 0}}});
  track.rotations.push_back({0, {{0, 0, 0, 1}}});
  track.scales.push_back({0, {{1, 2, 3}}, {{0, 0, 0}}});
  track.matrices.push_back({0, identity(), identity()});
  track.boneOffsets = {0};
  track.boneFrameCount = 2;
  track.boneCount = 1;
  track.boneFrames = {identity(), identity()};
  animation.tracks.push_back(track);
  m.animations.push_back(animation);
  m.customData = {1, 2, 3};
  return m;
}
int main() {
  std::string error;
  auto m = model();
#ifdef _DEBUG
  // Warm the CRT before measuring the production allocator/deallocator cycle.
  {
    sObject *warm = nullptr;
    CHECK(dbo::CreateCompatibilityObject(m, &warm, error));
    delete warm;
  }
  _CrtMemState before{}, after{}, difference{};
  _CrtMemCheckpoint(&before);
#endif
  for (int cycle = 0; cycle < 128; ++cycle) {
    sObject *value = nullptr;
    CHECK(dbo::CreateCompatibilityObject(m, &value, error));
    std::unique_ptr<sObject> object(value);
    CHECK(object->iFrameCount == 2 && object->iMeshCount == 1);
    CHECK(object->pFrame->pChild->pParent == object->pFrame);
    CHECK(object->pFrame->pChild->matOriginal._41 == 7);
    const sMesh &mesh = *object->ppMeshList[0];
    CHECK(mesh.dwVertexCount == 3 && mesh.dwIndexCount == 3);
    CHECK(std::memcmp(mesh.pVertexData, m.meshes[0].vertexData.data(), 36) ==
          0);
    CHECK(mesh.pIndices[2] == 2 && mesh.pIndicesLOD1[0] == 2);
    CHECK(!mesh.bVisible && !mesh.bLight && mesh.bWireframe);
    CHECK(mesh.mMaterial.Diffuse.g == .2f &&
          std::strcmp(mesh.pTextures[0].pName, "relative/albedo.dds") == 0);
    CHECK(mesh.pBones[0].pVertices[1] == 1 &&
          mesh.pBones[0].pWeights[1] == .75f);
    CHECK(object->pAnimationSet->ulLength == 40 &&
          object->pAnimationSet->pAnimation->dwNumPositionKeys == 2);
    CHECK(object->pAnimationSet->pAnimation->pPositionKeys[1].vecPos.x == 10);
    CHECK(object->pAnimationSet->pAnimation->pRotateKeys[0].Quaternion.w == 1);
    CHECK(object->pAnimationSet->pAnimation->pScaleKeys[0].vecScale.z == 3);
    CHECK(object->pAnimationSet->pAnimation->ppBoneFrames[1][0]._11 == 1);
    CHECK(object->dwCustomSize == 3 &&
          static_cast<const uint8_t *>(object->pCustomData)[2] == 3);
  }
#ifdef _DEBUG
  _CrtMemCheckpoint(&after);
  CHECK(_CrtMemDifference(&difference, &before, &after) == 0);
  CHECK(_CrtCheckMemory() != 0);
#endif
  // Failed decode and partial metadata construction preserve the caller's
  // output.
  sObject *sentinel = reinterpret_cast<sObject *>(uintptr_t(1));
  const uint8_t invalid[] = {1, 2, 3};
  CHECK(!dbo::DecodeCompatibilityObject(invalid, sizeof invalid, &sentinel,
                                        error));
  CHECK(sentinel == reinterpret_cast<sObject *>(uintptr_t(1)) &&
        !error.empty());
  m.animations[0].tracks[0].name = std::string(256, 'x');
  CHECK(!dbo::CreateCompatibilityObject(m, &sentinel, error));
  CHECK(sentinel == reinterpret_cast<sObject *>(uintptr_t(1)));
  // Mesh-animation subframes own a separate frame tree, never extra object
  // roots.
  m = model();
  auto submesh = m.meshes[0];
  submesh.bones.clear();
  m.meshes.push_back(submesh);
  dbo::Frame subframe;
  subframe.name = "meshframe";
  subframe.matrix = identity();
  subframe.mesh = 1;
  m.frames.push_back(subframe);
  m.meshes[0].subFrames = {2};
  dbo::Frame subchild;
  subchild.name = "subchild";
  subchild.matrix = identity();
  subchild.parent = 2;
  m.frames.push_back(subchild);
  {
    sObject *value = nullptr;
    CHECK(dbo::CreateCompatibilityObject(m, &value, error));
    std::unique_ptr<sObject> object(value);
    CHECK(object->iFrameCount == 2 && object->iMeshCount == 1);
    const auto &frame = object->ppMeshList[0]->pSubFrameList[0];
    CHECK(frame.pMesh->pFrameAttachedTo == &frame &&
          frame.pChild->pParent == &frame);
  }
  // A failure after allocating keys/baked rows must clean up the partial
  // object.
  m = model();
  m.animations[0].tracks[0].boneFrames.clear();
  CHECK(!dbo::CreateCompatibilityObject(m, &sentinel, error));
  CHECK(sentinel == reinterpret_cast<sObject *>(uintptr_t(1)));
  // A valid flat animation list must not overflow the stack during teardown.
  m = model();
  m.animations[0].tracks.clear();
  dbo::AnimationTrack empty;
  empty.name = "bone";
  m.animations[0].tracks.resize(10000, empty);
  {
    sObject *value = nullptr;
    CHECK(dbo::CreateCompatibilityObject(m, &value, error));
    delete value;
  }
  std::cout << "DBOCompatibility: " << checkCount
            << " checks, 128 ownership cycles passed\n";
}
