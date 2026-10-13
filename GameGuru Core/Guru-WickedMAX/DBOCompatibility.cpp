#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "DBOCompatibility.h"
#include "..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\DBOFormat\DBOData.h"
#include "..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\DBOFormat\DBOFormat.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_set>

namespace {
template <size_t N>
void Name(char (&destination)[N], const std::string &source) {
  if (source.size() >= N || source.find('\0') != std::string::npos)
    throw std::runtime_error("DBO name exceeds compatibility field capacity");
  std::memcpy(destination, source.c_str(), source.size() + 1);
}
void Matrix(GGMATRIX &destination, const dbo::Matrix &source) {
  static_assert(sizeof(GGMATRIX) == 16 * sizeof(float), "matrix layout");
  std::memcpy(&destination, source.data(), sizeof(destination));
}
GGVECTOR3 Vector(const dbo::Vec3 &source) {
  return GGVECTOR3(source[0], source[1], source[2]);
}
void Material(D3DMATERIAL9PRETEND &target, const dbo::Material &source) {
  std::memcpy(&target.Diffuse, source.diffuse.data(), 16);
  std::memcpy(&target.Ambient, source.ambient.data(), 16);
  std::memcpy(&target.Specular, source.specular.data(), 16);
  std::memcpy(&target.Emissive, source.emissive.data(), 16);
  target.Power = source.power;
}
template <class T> T *Copy(const std::vector<T> &values) {
  if (values.empty())
    return nullptr;
  auto data = std::unique_ptr<T[]>(new T[values.size()]);
  std::copy(values.begin(), values.end(), data.get());
  return data.release();
}
WORD *Indices(const std::vector<uint32_t> &values) {
  if (values.empty())
    return nullptr;
  auto data = std::unique_ptr<WORD[]>(new WORD[values.size()]);
  for (size_t i = 0; i < values.size(); ++i) {
    if (values[i] > (std::numeric_limits<WORD>::max)())
      throw std::runtime_error("DBO index exceeds compatibility WORD capacity");
    data[i] = static_cast<WORD>(values[i]);
  }
  return data.release();
}
class Builder {
  const dbo::Model &model;
  std::vector<std::vector<size_t>> children;
  std::unordered_set<size_t> subroots;
  std::unordered_set<size_t> active;

public:
  explicit Builder(const dbo::Model &value)
      : model(value), children(value.frames.size()) {
    for (size_t i = 0; i < model.frames.size(); ++i) {
      const int p = model.frames[i].parent;
      if (p >= 0) {
        if (static_cast<size_t>(p) >= model.frames.size())
          throw std::runtime_error("Invalid DBO parent");
        children[p].push_back(i);
      }
    }
    for (const auto &mesh : model.meshes)
      for (size_t index : mesh.subFrames) {
        if (index >= model.frames.size())
          throw std::runtime_error("Invalid DBO subframe");
        subroots.insert(index);
      }
  }
  std::unique_ptr<sMesh> Mesh(const dbo::Mesh &source) {
    auto mesh = std::unique_ptr<sMesh>(new sMesh);
    mesh->dwFVF = source.fvf;
    mesh->dwFVFOriginal = source.fvf;
    mesh->dwFVFSize = source.stride;
    mesh->dwVertexCount = source.vertexCount;
    mesh->dwIndexCount = static_cast<DWORD>(source.indices.size());
    mesh->pVertexData = Copy(source.vertexData);
    mesh->pIndices = Indices(source.indices);
    mesh->iPrimitiveType = source.primitiveType;
    mesh->iDrawVertexCount = source.drawVertexCount;
    mesh->iDrawPrimitives = source.drawPrimitiveCount;
    if (source.declaration.size() > MAX_FVF_DECL_SIZE)
      throw std::runtime_error("DBO vertex declaration too large");
    static_assert(sizeof(GGVERTEXELEMENT) == sizeof(dbo::VertexElement),
                  "declaration layout");
    if (!source.declaration.empty())
      std::memcpy(mesh->pVertexDeclaration, source.declaration.data(),
                  source.declaration.size() * sizeof(GGVERTEXELEMENT));
    mesh->bUsesMaterial = source.useMaterial;
    mesh->bUseMultiMaterial = source.useMultiMaterial;
    mesh->bVisible = source.visible;
    mesh->bTransparency = source.transparency;
    mesh->bCull = source.cull;
    mesh->bLight = source.light;
    mesh->bFog = source.fog;
    mesh->bAmbient = source.ambient;
    mesh->bWireframe = source.wireframe;
    mesh->bLinked = source.linked;
    mesh->bProtectZWriteState = source.ghost;
    mesh->bZRead = source.zRead;
    mesh->bZWrite = source.zWrite;
    mesh->bZBiasActive = source.zBias;
    mesh->fZBiasSlopeScale = source.zBiasSlope;
    mesh->fZBiasDepth = source.zBiasDepth;
    mesh->dwAlphaTestValue = source.alphaTest;
    mesh->fSpecularOverride = source.specularOverride;
    mesh->iReservedForFuture = source.ghostMode;
    mesh->Collision.dwArbitaryValue = source.arbitraryValue;
    Material(mesh->mMaterial, source.material);
    Name(mesh->pEffectName, source.effectName);
    // Wicked's established importer expects texture stage zero to exist.
    mesh->dwTextureCount =
        static_cast<DWORD>((std::max)(size_t(1), source.textures.size()));
    mesh->pTextures = new sTexture[mesh->dwTextureCount]{};
    for (size_t i = 0; i < source.textures.size(); ++i) {
      auto &target = mesh->pTextures[i];
      const auto &texture = source.textures[i];
      Name(target.pName, texture.name);
      target.dwStage = texture.stage;
      target.dwBlendMode = texture.blendMode;
      target.dwBlendArg1 = texture.blendArg1;
      target.dwBlendArg2 = texture.blendArg2;
      target.dwAddressU = texture.addressU;
      target.dwAddressV = texture.addressV;
      target.dwMagState = texture.mag;
      target.dwMinState = texture.min;
      target.dwMipState = texture.mip;
      target.dwTexCoordMode = texture.coordinateMode;
      target.iStartIndex = texture.primitiveStart;
      target.iPrimitiveCount = texture.primitiveCount;
      target.channelMask = static_cast<unsigned char>(texture.channelMask);
    }
    mesh->dwMultiMaterialCount =
        static_cast<DWORD>(source.multiMaterials.size());
    if (mesh->dwMultiMaterialCount)
      mesh->pMultiMaterial = new sMultiMaterial[mesh->dwMultiMaterialCount]{};
    for (size_t i = 0; i < source.multiMaterials.size(); ++i) {
      auto &target = mesh->pMultiMaterial[i];
      const auto &material = source.multiMaterials[i];
      Name(target.pName, material.name);
      Material(target.mMaterial, material.material);
      target.dwIndexStart = material.indexStart;
      target.dwIndexCount = material.indexCount;
      target.dwPolyCount = material.polygonCount;
    }
    mesh->dwBoneCount = static_cast<DWORD>(source.bones.size());
    if (mesh->dwBoneCount)
      mesh->pBones = new sBone[mesh->dwBoneCount];
    for (size_t i = 0; i < source.bones.size(); ++i) {
      auto &target = mesh->pBones[i];
      const auto &bone = source.bones[i];
      if (bone.vertices.size() != bone.weights.size())
        throw std::runtime_error("Invalid bone influences");
      Name(target.szName, bone.name);
      Matrix(target.matTranslation, bone.inverseBind);
      target.dwNumInfluences = static_cast<DWORD>(bone.vertices.size());
      target.pVertices = new DWORD[bone.vertices.size()];
      std::copy(bone.vertices.begin(), bone.vertices.end(), target.pVertices);
      target.pWeights = Copy(bone.weights);
    }
    mesh->dwIndexCountLOD1 = static_cast<DWORD>(source.lodIndices[0].size());
    mesh->pIndicesLOD1 = Indices(source.lodIndices[0]);
    mesh->dwIndexCountLOD2 = static_cast<DWORD>(source.lodIndices[1].size());
    mesh->pIndicesLOD2 = Indices(source.lodIndices[1]);
    mesh->dwIndexCountLOD3 = static_cast<DWORD>(source.lodIndices[2].size());
    mesh->pIndicesLOD3 = Indices(source.lodIndices[2]);
    mesh->dwSubMeshListCount = static_cast<DWORD>(source.subFrames.size());
    if (mesh->dwSubMeshListCount)
      mesh->pSubFrameList = new sFrame[mesh->dwSubMeshListCount];
    for (size_t i = 0; i < source.subFrames.size(); ++i) {
      auto frame = Frame(source.subFrames[i]);
      mesh->pSubFrameList[i] = *frame;
      auto &target = mesh->pSubFrameList[i];
      if (target.pMesh)
        target.pMesh->pFrameAttachedTo = &target;
      for (sFrame *child = target.pChild; child; child = child->pSibling)
        child->pParent = &target;
      // Transfer owned pointers into the array element, leaving the shell
      // empty.
      frame->pMesh = nullptr;
      frame->pChild = nullptr;
      frame->pSibling = nullptr;
    }
    return mesh;
  }
  std::unique_ptr<sFrame> Frame(size_t index) {
    if (index >= model.frames.size() || active.size() >= 256 ||
        !active.insert(index).second)
      throw std::runtime_error("Invalid DBO hierarchy");
    const auto &source = model.frames[index];
    auto frame = std::unique_ptr<sFrame>(new sFrame);
    Name(frame->szName, source.name);
    Matrix(frame->matOriginal, source.matrix);
    frame->vecOffset = Vector(source.offset);
    frame->vecRotation = Vector(source.rotation);
    frame->vecScale = Vector(source.scale);
    if (source.mesh >= 0) {
      if (static_cast<size_t>(source.mesh) >= model.meshes.size())
        throw std::runtime_error("Invalid DBO mesh reference");
      frame->pMesh = Mesh(model.meshes[source.mesh]).release();
      frame->pMesh->pFrameAttachedTo = frame.get();
    }
    sFrame **tail = &frame->pChild;
    for (size_t child : children[index]) {
      auto next = Frame(child);
      next->pParent = frame.get();
      *tail = next.release();
      tail = &(*tail)->pSibling;
    }
    active.erase(index);
    return frame;
  }
  void Frames(sObject &object) {
    sFrame **tail = &object.pFrame;
    for (size_t i = 0; i < model.frames.size(); ++i)
      if (model.frames[i].parent < 0 && !subroots.count(i)) {
        *tail = Frame(i).release();
        tail = &(*tail)->pSibling;
      }
    if (!object.pFrame)
      throw std::runtime_error("DBO has no object root frame");
  }
};
void Animations(sObject &object, const dbo::Model &model) {
  sAnimationSet **sets = &object.pAnimationSet;
  for (const auto &source : model.animations) {
    *sets = new sAnimationSet;
    auto &set = **sets;
    Name(set.szName, source.name);
    set.ulLength = source.length;
    set.dwAnimSetType = source.type;
    set.fAnimSetStart = source.start;
    set.fAnimSetFinish = source.finish;
    set.fAnimSetStep1 = source.steps[0];
    set.fAnimSetStep2 = source.steps[1];
    set.fAnimSetStep3 = source.steps[2];
    sAnimation **tracks = &set.pAnimation;
    for (const auto &track : source.tracks) {
      *tracks = new sAnimation;
      auto &animation = **tracks;
      Name(animation.szName, track.name);
      animation.bBoneType = track.boneType;
      animation.dwNumPositionKeys = static_cast<DWORD>(track.positions.size());
      if (animation.dwNumPositionKeys)
        animation.pPositionKeys = new sPositionKey[animation.dwNumPositionKeys];
      for (size_t i = 0; i < track.positions.size(); ++i) {
        const auto &key = track.positions[i];
        animation.pPositionKeys[i] = {key.time, Vector(key.value),
                                      Vector(key.interpolation)};
      }
      animation.dwNumRotateKeys = static_cast<DWORD>(track.rotations.size());
      if (animation.dwNumRotateKeys)
        animation.pRotateKeys = new sRotateKey[animation.dwNumRotateKeys];
      for (size_t i = 0; i < track.rotations.size(); ++i) {
        const auto &key = track.rotations[i];
        animation.pRotateKeys[i].dwTime = key.time;
        std::memcpy(&animation.pRotateKeys[i].Quaternion, key.value.data(), 16);
      }
      animation.dwNumScaleKeys = static_cast<DWORD>(track.scales.size());
      if (animation.dwNumScaleKeys)
        animation.pScaleKeys = new sScaleKey[animation.dwNumScaleKeys];
      for (size_t i = 0; i < track.scales.size(); ++i) {
        const auto &key = track.scales[i];
        animation.pScaleKeys[i] = {key.time, Vector(key.value),
                                   Vector(key.interpolation)};
      }
      animation.dwNumMatrixKeys = static_cast<DWORD>(track.matrices.size());
      if (animation.dwNumMatrixKeys)
        animation.pMatrixKeys = new sMatrixKey[animation.dwNumMatrixKeys];
      for (size_t i = 0; i < track.matrices.size(); ++i) {
        const auto &key = track.matrices[i];
        animation.pMatrixKeys[i].dwTime = key.time;
        Matrix(animation.pMatrixKeys[i].matMatrix, key.value);
        Matrix(animation.pMatrixKeys[i].matInterpolation, key.interpolation);
      }
      animation.iBoneOffsetListCount =
          static_cast<int>(track.boneOffsets.size());
      if (animation.iBoneOffsetListCount) {
        animation.piBoneOffsetList = new int[track.boneOffsets.size()];
        std::copy(track.boneOffsets.begin(), track.boneOffsets.end(),
                  animation.piBoneOffsetList);
      }
      animation.iBoneFrameA = track.boneFrameCount;
      animation.iBoneFrameB = track.boneCount;
      if (track.boneFrameCount)
        animation.ppBoneFrames = new GGMATRIX *[track.boneFrameCount] {};
      for (size_t a = 0; a < track.boneFrameCount; ++a) {
        animation.ppBoneFrames[a] = new GGMATRIX[track.boneCount];
        for (size_t b = 0; b < track.boneCount; ++b) {
          const size_t i = a * track.boneCount + b;
          if (i >= track.boneFrames.size())
            throw std::runtime_error("Invalid baked bone matrix count");
          Matrix(animation.ppBoneFrames[a][b], track.boneFrames[i]);
        }
      }
      tracks = &animation.pNext;
    }
    sets = &set.pNext;
  }
}
} // namespace
namespace dbo {
namespace {
thread_local std::string lastError;
}
const std::string &LastCompatibilityError() { return lastError; }
bool CreateCompatibilityObject(const Model &model, sObject **output,
                               std::string &error) {
  if (!output) {
    error = "Null DBO object output";
    return false;
  }
  try {
    auto object = std::unique_ptr<sObject>(new sObject);
    Builder(model).Frames(*object);
    Animations(*object, model);
    object->dwCustomSize = static_cast<DWORD>(model.customData.size());
    object->pCustomData = Copy(model.customData);
    // Publish metadata lists in the same child-before-sibling order as the
    // existing BuildFrameList/BuildMeshList. Finalization can rebuild these
    // using the established runtime path after object registration.
    std::vector<sFrame *> frames, pending;
    std::vector<sMesh *> meshes;
    pending.push_back(object->pFrame);
    while (!pending.empty()) {
      sFrame *frame = pending.back();
      pending.pop_back();
      frame->iID = static_cast<int>(frames.size());
      frames.push_back(frame);
      if (frame->pMesh)
        meshes.push_back(frame->pMesh);
      if (frame->pSibling)
        pending.push_back(frame->pSibling);
      if (frame->pChild)
        pending.push_back(frame->pChild);
    }
    // Allocate both lists before assigning counts; the destructor must not
    // inspect partially initialized lists if allocation fails.
    auto frameList = std::unique_ptr<sFrame *[]>(new sFrame *[frames.size()]);
    auto meshList = std::unique_ptr<sMesh *[]>(new sMesh *[meshes.size()]);
    std::copy(frames.begin(), frames.end(), frameList.get());
    std::copy(meshes.begin(), meshes.end(), meshList.get());
    object->ppFrameList = frameList.release();
    object->ppMeshList = meshList.release();
    object->iFrameCount = static_cast<int>(frames.size());
    object->iMeshCount = static_cast<int>(meshes.size());
    object->bNoMeshesInObject = (object->iMeshCount == 0);
    for (sAnimationSet *set = object->pAnimationSet; set; set = set->pNext) {
      for (sAnimation *anim = set->pAnimation; anim; anim = anim->pNext) {
        if (!anim->pFrame && anim->szName && object->ppFrameList) {
          for (int f = 0; f < object->iFrameCount; ++f) {
            if (object->ppFrameList[f] && object->ppFrameList[f]->szName &&
                _stricmp(object->ppFrameList[f]->szName, anim->szName) == 0) {
              anim->pFrame = object->ppFrameList[f];
              break;
            }
          }
        }
      }
    }
    *output = object.release();
    error.clear();
    return true;
  } catch (const std::exception &exception) {
    error = exception.what();
    return false;
  }
}
bool DecodeCompatibilityObject(const void *bytes, size_t size, sObject **output,
                               std::string &error) {
  Model model;
  Error failure;
  if (!DecodeFromMemory(bytes, size, model, failure)) {
    error = "DBO decode error " +
            std::to_string(static_cast<int>(failure.code)) + " at byte " +
            std::to_string(failure.offset) + ": " + failure.message;
    lastError = error;
    return false;
  }
  const bool success = CreateCompatibilityObject(model, output, error);
  lastError = error;
  return success;
}
} // namespace dbo
