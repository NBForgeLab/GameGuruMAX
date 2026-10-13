#include "DBOModel.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
namespace dbo {
namespace {
static_assert(sizeof(VertexElement) == 8 && sizeof(VectorKey) == 28 &&
                  sizeof(RotationKey) == 20 && sizeof(MatrixKey) == 132,
              "DBO wire structures must match the legacy writer");
struct Failure {
  Error error;
};
struct Reader {
  const uint8_t *data;
  size_t pos, end;
  const Limits *limits;
  size_t *allocated;
  [[noreturn]] void fail(ErrorCode c, const char *m) const {
    throw Failure{Error{c, pos, m}};
  }
  void need(size_t n) const {
    if (n > end - pos)
      fail(ErrorCode::Truncated, "payload extends beyond enclosing chunk");
  }
  template <class T> T get() {
    need(sizeof(T));
    T v;
    memcpy(&v, data + pos, sizeof(T));
    pos += sizeof(T);
    return v;
  }
  size_t product(size_t n, size_t stride) const {
    if (n > limits->maxElements ||
        (stride && n > std::numeric_limits<size_t>::max() / stride))
      fail(ErrorCode::ResourceLimit, "element count exceeds limit");
    return n * stride;
  }
  void budget(size_t n) {
    if (n > limits->maxDecodedBytes - *allocated)
      fail(ErrorCode::ResourceLimit, "decoded allocation budget exceeded");
    *allocated += n;
  }
  template <class T> std::vector<T> array(size_t n) {
    // Raw byte payloads are bounded by file/allocation budgets, while typed
    // arrays additionally obey the element budget.
    size_t bytes = std::is_same<T, uint8_t>::value ? n : product(n, sizeof(T));
    need(bytes);
    budget(bytes);
    std::vector<T> v(n);
    if (bytes)
      memcpy(v.data(), data + pos, bytes);
    pos += bytes;
    return v;
  }
  std::string string() {
    auto n = get<uint32_t>();
    if (n >= 256)
      fail(ErrorCode::InvalidChunk,
           "legacy fixed-name string exceeds 255 bytes");
    need(n);
    budget(n);
    std::string s(reinterpret_cast<const char *>(data + pos), n);
    pos += n;
    if (s.find('\0') != std::string::npos)
      fail(ErrorCode::InvalidChunk, "embedded null in name");
    return s;
  }
  Reader slice(size_t n) {
    need(n);
    Reader s{data, pos, pos + n, limits, allocated};
    pos += n;
    return s;
  }
  void exact() const {
    if (pos != end)
      fail(ErrorCode::InvalidChunk, "unexpected bytes in chunk");
  }
};
Matrix identity() {
  Matrix m{};
  m[0] = m[5] = m[10] = m[15] = 1;
  return m;
}
void finite(Reader &r, float f) {
  if (!std::isfinite(f))
    r.fail(ErrorCode::InvalidChunk, "non-finite number");
}
template <size_t N> std::array<float, N> floats(Reader &r) {
  std::array<float, N> a;
  for (auto &f : a) {
    f = r.get<float>();
    finite(r, f);
  }
  return a;
}
Material material(Reader &r) {
  Material m;
  m.diffuse = floats<4>(r);
  m.ambient = floats<4>(r);
  m.specular = floats<4>(r);
  m.emissive = floats<4>(r);
  m.power = r.get<float>();
  finite(r, m.power);
  return m;
}
struct Parser {
  Model model;
  const Limits &limits;
  void depth(Reader &r, size_t d) {
    if (d > limits.maxDepth)
      r.fail(ErrorCode::ResourceLimit, "nesting limit exceeded");
  }
  // Writer uses code,size pairs with a mandatory 0,0 terminator in every
  // container.
  template <class F> void chunks(Reader &r, F fn, bool legacyRootEOF = false) {
    std::unordered_set<uint32_t> seen;
    for (;;) {
      if (legacyRootEOF && r.pos == r.end)
        return;
      auto id = r.get<uint32_t>(), size = r.get<uint32_t>();
      if (!id) {
        if (size)
          r.fail(ErrorCode::InvalidChunk, "nonzero terminator size");
        return;
      }
      if (seen.size() >= limits.maxElements)
        r.fail(ErrorCode::ResourceLimit, "chunk count exceeds limit");
      r.budget(32); // Includes the per-container duplicate-detection node.
      if (!seen.insert(id).second)
        r.fail(ErrorCode::InvalidChunk, "duplicate chunk");
      if (!fn(id, size, r)) {
        auto ignored = r.slice(size);
        (void)ignored;
      }
    }
  }
  template <class F> void payload(Reader &r, uint32_t size, F fn) {
    auto s = r.slice(size);
    fn(s);
    s.exact();
  }
  template <class T> void scalar(Reader &r, uint32_t n, T &v) {
    payload(r, n, [&](Reader &s) { v = s.get<T>(); });
  }
  void frame(Reader &r, int parent, size_t d) {
    depth(r, d);
    if (r.end - r.pos >= 8) {
      uint32_t zero;
      memcpy(&zero, r.data + r.pos, 4);
      if (!zero) {
        chunks(r, [](uint32_t, uint32_t, Reader &) { return false; });
        return;
      }
    }
    if (model.frames.size() >= limits.maxFrames)
      r.fail(ErrorCode::ResourceLimit, "frame limit exceeded");
    r.budget(sizeof(Frame));
    size_t ix = model.frames.size();
    model.frames.emplace_back();
    model.frames[ix].parent = parent;
    model.frames[ix].matrix = identity();
    chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
      if (id == 101) {
        payload(src, n, [&](Reader &s) { model.frames[ix].name = s.string(); });
        return true;
      }
      if (id == 102) {
        payload(src, n,
                [&](Reader &s) { model.frames[ix].matrix = floats<16>(s); });
        return true;
      }
      if (id >= 106 && id <= 108) {
        payload(src, n, [&](Reader &s) {
          auto v = floats<3>(s);
          if (id == 106)
            model.frames[ix].offset = v;
          if (id == 107)
            model.frames[ix].rotation = v;
          if (id == 108)
            model.frames[ix].scale = v;
        });
        return true;
      }
      if (id == 103) {
        payload(src, n,
                [&](Reader &s) { model.frames[ix].mesh = mesh(s, d + 1); });
        return true;
      }
      if (id == 104 || id == 105) {
        payload(src, n, [&](Reader &s) {
          frame(s, id == 104 ? int(ix) : parent, d + 1);
        });
        return true;
      }
      return false;
    });
  }
  void bones(Reader &r, uint32_t count, Mesh &m) {
    r.product(count, sizeof(Bone));
    r.budget(count * sizeof(Bone));
    for (uint32_t i = 0; i < count; i++) {
      Bone b;
      b.inverseBind = identity();
      uint32_t influences = 0;
      chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
        if (id == 301) {
          payload(src, n, [&](Reader &s) { b.name = s.string(); });
          return true;
        }
        if (id == 302) {
          scalar(src, n, influences);
          return true;
        }
        if (id == 303) {
          payload(src, n, [&](Reader &s) {
            b.vertices = s.array<uint32_t>(influences);
          });
          return true;
        }
        if (id == 304) {
          payload(src, n,
                  [&](Reader &s) { b.weights = s.array<float>(influences); });
          return true;
        }
        if (id == 305) {
          payload(src, n, [&](Reader &s) { b.inverseBind = floats<16>(s); });
          return true;
        }
        return false;
      });
      if (b.vertices.size() != influences || b.weights.size() != influences)
        r.fail(ErrorCode::InvalidGeometry, "missing bone influences");
      for (size_t k = 0; k < influences; k++) {
        if (b.vertices[k] >= m.vertexCount || !std::isfinite(b.weights[k]) ||
            b.weights[k] < 0)
          r.fail(ErrorCode::InvalidGeometry, "invalid bone influence");
      }
      m.bones.push_back(std::move(b));
    }
    chunks(r, [](uint32_t, uint32_t, Reader &s) -> bool {
      s.fail(ErrorCode::InvalidChunk, "extra bone item");
    });
  }
  void textures(Reader &r, uint32_t count, Mesh &m) {
    r.product(count, sizeof(Texture));
    r.budget(count * sizeof(Texture));
    for (uint32_t i = 0; i < count; i++) {
      Texture t;
      chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
        if (id == 141) {
          payload(src, n, [&](Reader &s) { t.name = s.string(); });
          return true;
        }
        if (id == 154) {
          if (n != 4 && n != 1)
            src.fail(ErrorCode::InvalidChunk, "invalid channel mask size");
          t.channelMask = src.get<uint8_t>();
          return true;
        }
        if (id >= 142 && id <= 153) {
          uint32_t v;
          scalar(src, n, v);
          uint32_t *fields[] = {&t.stage,
                                &t.blendMode,
                                &t.blendArg1,
                                &t.blendArg2,
                                &t.addressU,
                                &t.addressV,
                                &t.mag,
                                &t.min,
                                &t.mip,
                                &t.coordinateMode,
                                &t.primitiveStart,
                                &t.primitiveCount,
                                &t.channelMask};
          *fields[id - 142] = v;
          return true;
        }
        return false;
      });
      m.textures.push_back(std::move(t));
    }
    chunks(r, [](uint32_t, uint32_t, Reader &s) -> bool {
      s.fail(ErrorCode::InvalidChunk, "extra texture item");
    });
  }
  void materials(Reader &r, uint32_t count, Mesh &m) {
    r.product(count, sizeof(MultiMaterial));
    r.budget(count * sizeof(MultiMaterial));
    for (uint32_t i = 0; i < count; i++) {
      MultiMaterial t;
      chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
        if (id == 161) {
          payload(src, n, [&](Reader &s) { t.name = s.string(); });
          return true;
        }
        if (id == 162) {
          payload(src, n, [&](Reader &s) { t.material = material(s); });
          return true;
        }
        if (id >= 163 && id <= 165) {
          uint32_t v;
          scalar(src, n, v);
          if (id == 163)
            t.indexStart = v;
          if (id == 164)
            t.indexCount = v;
          if (id == 165)
            t.polygonCount = v;
          return true;
        }
        return false;
      });
      if (t.indexStart > m.indices.size() ||
          t.indexCount > m.indices.size() - t.indexStart)
        r.fail(ErrorCode::InvalidGeometry,
               "material index range out of bounds");
      m.multiMaterials.push_back(std::move(t));
    }
    chunks(r, [](uint32_t, uint32_t, Reader &s) -> bool {
      s.fail(ErrorCode::InvalidChunk, "extra material item");
    });
  }
  void vertices(Reader &r, Mesh &m, size_t dataOffset) {
    if (!m.vertexCount) {
      if (!m.vertexData.empty())
        r.fail(ErrorCode::InvalidGeometry, "data without vertices");
      return;
    }
    if (!m.stride || m.stride > 4096 ||
        m.vertexData.size() != r.product(m.vertexCount, m.stride))
      r.fail(ErrorCode::InvalidGeometry,
             "vertex count/stride/payload mismatch");
    int po = -1, no = -1, uo = -1;
    uint8_t pt = 2, nt = 2, ut = 1;
    size_t required = 0;
    if (m.fvf) {
      uint32_t pos = m.fvf & 0x00e;
      if (pos == 2) {
        po = 0;
        required = 12;
      } else if (pos >= 6 && pos <= 14) {
        po = 0;
        required = 12 + 4 * ((pos - 4) / 2);
      } else
        r.fail(ErrorCode::InvalidGeometry, "unsupported FVF position");
      if (m.fvf & 0x10) {
        no = int(required);
        required += 12;
      }
      if (m.fvf & 0x20)
        required += 4;
      if (m.fvf & 0x40)
        required += 4;
      if (m.fvf & 0x80)
        required += 4;
      uint32_t tex = (m.fvf >> 8) & 15;
      if (tex > 8)
        r.fail(ErrorCode::InvalidGeometry, "invalid FVF texture count");
      for (uint32_t i = 0; i < tex; i++) {
        uint32_t fmt = (m.fvf >> (16 + i * 2)) & 3;
        size_t components = fmt == 0 ? 2 : fmt == 1 ? 3 : fmt == 2 ? 4 : 1;
        if (i == 0) {
          uo = int(required);
          ut = uint8_t(components - 1);
        }
        required += 4 * components;
      }
    } else {
      bool terminal = false;
      const size_t typeSizes[] = {4, 8, 12, 16, 4, 4, 4, 8, 4,
                                  4, 8, 4,  8,  4, 4, 4, 8};
      for (const auto &e : m.declaration) {
        if (e.stream == 255) {
          terminal = true;
          break;
        }
        if (e.stream || e.method || e.type >= 17)
          r.fail(ErrorCode::InvalidGeometry,
                 "unsupported vertex declaration stream/type/method");
        required = std::max(required, size_t(e.offset) + typeSizes[e.type]);
        if (e.usageIndex == 0) {
          if (e.usage == 0) {
            po = e.offset;
            pt = e.type;
          }
          if (e.usage == 3) {
            no = e.offset;
            nt = e.type;
          }
          if (e.usage == 5) {
            uo = e.offset;
            ut = e.type;
          }
        }
      }
      if (!terminal)
        r.fail(ErrorCode::InvalidGeometry, "unterminated vertex declaration");
    }
    if (po < 0 || pt != 2 || required > m.stride)
      r.fail(ErrorCode::InvalidGeometry,
             "position missing or attributes exceed stride");
    r.budget(r.product(m.vertexCount,
                       sizeof(Vec3) * 2 + sizeof(std::array<float, 2>)));
    m.positions.resize(m.vertexCount);
    if (no >= 0)
      m.normals.resize(m.vertexCount);
    if (uo >= 0)
      m.uv.resize(m.vertexCount);
    auto read = [&](const uint8_t *p, int off, uint8_t type, float *out,
                    size_t components, size_t vertex, const char *label) {
      if (type > 3 || size_t(type + 1) < components)
        r.fail(ErrorCode::InvalidGeometry,
               "unsupported normalized attribute type");
      memcpy(out, p + off, components * 4);
      for (size_t k = 0; k < components; k++) {
        if (!std::isfinite(out[k])) {
          Reader diagnostic = r;
          diagnostic.pos = dataOffset + vertex * m.stride + size_t(off) + k * 4;
          diagnostic.fail(ErrorCode::InvalidGeometry, label);
        }
      }
    };
    for (uint32_t i = 0; i < m.vertexCount; i++) {
      auto p = m.vertexData.data() + size_t(i) * m.stride;
      read(p, po, pt, m.positions[i].data(), 3, i,
           "non-finite vertex position");
      if (no >= 0)
        read(p, no, nt, m.normals[i].data(), 3, i, "non-finite vertex normal");
      if (uo >= 0)
        read(p, uo, ut, m.uv[i].data(), 2, i, "non-finite texture coordinate");
    }
  }
  int mesh(Reader &r, size_t d) {
    depth(r, d);
    if (r.end - r.pos == 8) {
      auto s = r;
      chunks(s, [](uint32_t, uint32_t, Reader &) { return false; });
      r = s;
      return -1;
    }
    Mesh m;
    size_t vertexDataOffset = 0;
    uint32_t indexCount = 0, boneCount = 0, textureCount = 0, matCount = 0;
    std::array<uint32_t, 3> lodCounts{};
    chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
      if (id >= 111 && id <= 114) {
        uint32_t v;
        scalar(src, n, v);
        if (id == 111)
          m.fvf = v;
        if (id == 112)
          m.stride = v;
        if (id == 113)
          m.vertexCount = v;
        if (id == 114)
          indexCount = v;
        return true;
      }
      if (id == 115) {
        payload(src, n, [&](Reader &s) {
          vertexDataOffset = s.pos;
          if (!m.stride || m.stride > 4096)
            s.fail(ErrorCode::InvalidGeometry, "invalid vertex stride");
          size_t bytes = s.product(m.vertexCount, m.stride);
          if (bytes != n)
            s.fail(ErrorCode::InvalidGeometry, "vertex payload count mismatch");
          m.vertexData = s.array<uint8_t>(bytes);
        });
        return true;
      }
      if (id == 116 || (id >= 500 && id <= 502)) {
        payload(src, n, [&](Reader &s) {
          size_t count = id == 116 ? indexCount : lodCounts[id - 500];
          auto v = s.array<uint16_t>(count);
          s.budget(count * sizeof(uint32_t));
          std::vector<uint32_t> out(v.begin(), v.end());
          for (auto x : out)
            if (x >= m.vertexCount)
              s.fail(ErrorCode::InvalidGeometry, "index outside vertex array");
          if (id == 116)
            m.indices = std::move(out);
          else
            m.lodIndices[id - 500] = std::move(out);
        });
        return true;
      }
      if (id >= 503 && id <= 505) {
        scalar(src, n, lodCounts[id - 503]);
        return true;
      }
      if (id == 120) { // ScanMesh writes size=4 followed by the entire
                       // 65-element declaration.
        if (n != 4 && n != 520)
          src.fail(ErrorCode::InvalidChunk, "invalid legacy declaration size");
        auto s = src.slice(520);
        m.declaration = s.array<VertexElement>(65);
        s.exact();
        return true;
      }
      if (id == 117) {
        // Pre-U6 ScanMesh advertised index-buffer length for these DWORDs.
        if (model.version == 1 && n == src.product(indexCount, 2))
          n = 4;
        scalar(src, n, m.primitiveType);
        return true;
      }
      if (id == 118) {
        if (model.version == 1 && n == src.product(indexCount, 2))
          n = 4;
        scalar(src, n, m.drawVertexCount);
        return true;
      }
      if (id == 119) {
        if (model.version == 1 && n == src.product(indexCount, 2))
          n = 4;
        scalar(src, n, m.drawPrimitiveCount);
        return true;
      }
      if (id == 121) {
        scalar(src, n, boneCount);
        return true;
      }
      if (id == 127) {
        scalar(src, n, textureCount);
        return true;
      }
      if (id == 124) {
        scalar(src, n, matCount);
        return true;
      }
      if (id == 122 || id == 128 || id == 139) {
        payload(src, n, [&](Reader &s) {
          if (id == 122)
            bones(s, boneCount, m);
          if (id == 128)
            textures(s, textureCount, m);
          if (id == 139)
            materials(s, matCount, m);
        });
        return true;
      }
      if (id == 125 || id == 123 || id == 140 || (id >= 129 && id <= 135) ||
          id == 137 || id == 156 || id == 159 || id == 160) {
        uint8_t v;
        scalar(src, n, v);
        if (v > 1)
          src.fail(ErrorCode::InvalidChunk, "invalid boolean");
        if (id == 125)
          m.useMaterial = v != 0;
        if (id == 123)
          m.useMultiMaterial = v != 0;
        if (id == 140)
          m.visible = v != 0;
        if (id == 134)
          m.transparency = v != 0;
        if (id == 131)
          m.cull = v != 0;
        if (id == 129)
          m.wireframe = v != 0;
        if (id == 130)
          m.light = v != 0;
        if (id == 132)
          m.fog = v != 0;
        if (id == 133)
          m.ambient = v != 0;
        if (id == 135)
          m.ghost = v != 0;
        if (id == 137)
          m.linked = v != 0;
        if (id == 156)
          m.zBias = v != 0;
        if (id == 159)
          m.zRead = v != 0;
        if (id == 160)
          m.zWrite = v != 0;
        return true;
      }
      if (id == 136) {
        scalar(src, n, m.ghostMode);
        return true;
      }
      if (id == 155) {
        scalar(src, n, m.arbitraryValue);
        return true;
      }
      if (id == 166) {
        scalar(src, n, m.alphaTest);
        return true;
      }
      if (id == 167) {
        scalar(src, n, m.specularOverride);
        finite(src, m.specularOverride);
        return true;
      }
      if (id == 157) {
        scalar(src, n, m.zBiasSlope);
        finite(src, m.zBiasSlope);
        return true;
      }
      if (id == 158) {
        scalar(src, n, m.zBiasDepth);
        finite(src, m.zBiasDepth);
        return true;
      }
      if (id == 126) {
        payload(src, n, [&](Reader &s) { m.material = material(s); });
        return true;
      }
      if (id == 154) {
        payload(src, n, [&](Reader &s) { m.effectName = s.string(); });
        return true;
      }
      if (id == 138) {
        uint32_t count;
        scalar(src, n, count);
        src.product(count, sizeof(Frame));
        src.budget(count * sizeof(size_t));
        for (uint32_t i = 0; i < count; i++) {
          const size_t first = model.frames.size();
          frame(src, -1, d + 1);
          if (model.frames.size() == first)
            src.fail(ErrorCode::InvalidGeometry, "empty counted subframe");
          m.subFrames.push_back(first);
        }
        return true;
      }
      return false;
    });
    vertices(r, m, vertexDataOffset);
    if (m.indices.size() != indexCount || m.bones.size() != boneCount ||
        m.textures.size() != textureCount ||
        m.multiMaterials.size() != matCount)
      r.fail(ErrorCode::InvalidGeometry, "missing counted mesh data");
    for (size_t i = 0; i < 3; i++)
      if (m.lodIndices[i].size() != lodCounts[i])
        r.fail(ErrorCode::InvalidGeometry, "missing LOD indices");
    if (m.drawVertexCount > m.vertexCount)
      r.fail(ErrorCode::InvalidGeometry, "draw vertex count exceeds geometry");
    if (m.primitiveType < 1 || m.primitiveType > 6)
      r.fail(ErrorCode::InvalidGeometry, "invalid primitive type");
    const size_t available = indexCount ? indexCount : m.vertexCount;
    const uint64_t primitives = m.drawPrimitiveCount;
    uint64_t required = 0;
    if (primitives) {
      switch (m.primitiveType) {
      case 1:
        required = primitives;
        break; // point list
      case 2:
        required = primitives * 2;
        break; // line list
      case 3:
        required = primitives + 1;
        break; // line strip
      case 4:
        required = primitives * 3;
        break; // triangle list
      case 5:
      case 6:
        required = primitives + 2;
        break; // strip/fan
      }
    }
    if (required > available)
      r.fail(ErrorCode::InvalidGeometry,
             "draw primitive count exceeds geometry");
    r.budget(sizeof(Mesh));
    int index = int(model.meshes.size());
    model.meshes.push_back(std::move(m));
    return index;
  }
  template <class T>
  void keys(Reader &r, uint32_t n, uint32_t count, std::vector<T> &out) {
    payload(r, n, [&](Reader &s) {
      out = s.array<T>(count);
      uint32_t previous = 0;
      for (auto &key : out) {
        if (key.time < previous)
          s.fail(ErrorCode::InvalidAnimation, "unordered animation keys");
        previous = key.time;
        for (float value : key.value)
          finite(s, value);
      }
      rebuildInterpolation(s, out);
    });
  }
  static void rebuildInterpolation(Reader &, std::vector<RotationKey> &) {}
  template <class T>
  static void rebuildInterpolation(Reader &r, std::vector<T> &out) {
    // These fields are derivative caches, not authored animation data. The
    // legacy UpdatePosition/Scale/MatrixKeys routines never initialized the
    // final cache.
    for (size_t i = 0; i < out.size(); i++) {
      out[i].interpolation = {};
      if (i + 1 == out.size())
        continue;
      uint32_t dt = out[i + 1].time - out[i].time;
      if (!dt)
        continue;
      for (size_t k = 0; k < out[i].value.size(); k++) {
        double derivative =
            (double(out[i + 1].value[k]) - double(out[i].value[k])) /
            double(dt);
        if (std::abs(derivative) > std::numeric_limits<float>::max())
          r.fail(ErrorCode::InvalidAnimation, "animation derivative overflow");
        out[i].interpolation[k] = float(derivative);
      }
    }
  }
  void track(Reader &r, AnimationSet &set, size_t d) {
    depth(r, d);
    if (r.end - r.pos == 8) {
      chunks(r, [](uint32_t, uint32_t, Reader &) { return false; });
      return;
    }
    AnimationTrack t;
    std::array<uint32_t, 4> counts{};
    uint32_t offsets = 0;
    bool stored = false;
    auto validate = [&] {
      if (t.positions.size() != counts[0] || t.rotations.size() != counts[1] ||
          t.scales.size() != counts[2] || t.matrices.size() != counts[3] ||
          t.boneOffsets.size() != offsets ||
          t.boneFrames.size() != r.product(t.boneFrameCount, t.boneCount))
        r.fail(ErrorCode::InvalidAnimation, "missing animation data");
    };
    chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
      if (stored)
        src.fail(ErrorCode::InvalidAnimation, "track fields after NEXT");
      if (id == 211) {
        payload(src, n, [&](Reader &s) { t.name = s.string(); });
        return true;
      }
      if (id == 212 || id == 214 || id == 216 || id == 218) {
        scalar(src, n, counts[(id - 212) / 2]);
        return true;
      }
      if (id == 213) {
        keys(src, n, counts[0], t.positions);
        return true;
      }
      if (id == 215) {
        keys(src, n, counts[1], t.rotations);
        return true;
      }
      if (id == 217) {
        keys(src, n, counts[2], t.scales);
        return true;
      }
      if (id == 219) {
        keys(src, n, counts[3], t.matrices);
        return true;
      }
      if (id == 400) {
        uint32_t v;
        scalar(src, n, v);
        if (v > 1)
          src.fail(ErrorCode::InvalidAnimation, "invalid bone type");
        t.boneType = v != 0;
        return true;
      }
      if (id == 402) {
        scalar(src, n, offsets);
        return true;
      }
      if (id == 401) {
        payload(src, n,
                [&](Reader &s) { t.boneOffsets = s.array<int32_t>(offsets); });
        return true;
      }
      if (id == 403) {
        scalar(src, n, t.boneFrameCount);
        return true;
      }
      if (id == 404) {
        scalar(src, n, t.boneCount);
        return true;
      }
      if (id == 405) {
        payload(src, n, [&](Reader &s) {
          size_t count = s.product(t.boneFrameCount, t.boneCount);
          t.boneFrames = s.array<Matrix>(count);
          for (auto &m : t.boneFrames)
            for (auto f : m)
              finite(s, f);
        });
        return true;
      }
      if (id == 220) {
        validate();
        r.budget(sizeof(AnimationTrack));
        set.tracks.push_back(std::move(t));
        stored = true;
        payload(src, n, [&](Reader &s) { track(s, set, d + 1); });
        return true;
      }
      return false;
    });
    // NEXT follows the complete track; the moved track has already been stored.
    if (!stored) {
      validate();
      r.budget(sizeof(AnimationTrack));
      set.tracks.push_back(std::move(t));
    }
  }
  void animset(Reader &r, size_t d) {
    depth(r, d);
    if (r.end - r.pos == 8) {
      chunks(r, [](uint32_t, uint32_t, Reader &) { return false; });
      return;
    }
    AnimationSet a;
    bool next = false;
    chunks(r, [&](uint32_t id, uint32_t n, Reader &src) {
      if (next)
        src.fail(ErrorCode::InvalidAnimation, "set fields after NEXT");
      if (id == 201) {
        payload(src, n, [&](Reader &s) { a.name = s.string(); });
        return true;
      }
      if (id == 202 || id == 204) {
        uint32_t v;
        scalar(src, n, v);
        if (id == 202)
          a.length = v;
        if (id == 204)
          a.type = v;
        return true;
      }
      if (id == 206 || id == 207) {
        float value;
        scalar(src, n, value);
        finite(src, value);
        if (id == 206)
          a.start = value;
        else
          a.finish = value;
        return true;
      }
      if (id >= 208 && id <= 210) {
        scalar(src, n, a.steps[id - 208]);
        finite(src, a.steps[id - 208]);
        return true;
      }
      if (id == 203) {
        payload(src, n, [&](Reader &s) { track(s, a, d + 1); });
        return true;
      }
      if (id == 205) {
        r.budget(sizeof(AnimationSet));
        model.animations.push_back(std::move(a));
        next = true;
        payload(src, n, [&](Reader &s) { animset(s, d + 1); });
        return true;
      }
      return false;
    });
    if (!next) {
      r.budget(sizeof(AnimationSet));
      model.animations.push_back(std::move(a));
    }
  }
  void parse(Reader &r) {
    if (r.string() != "MAGICDBO")
      r.fail(ErrorCode::InvalidHeader, "invalid DBO magic");
    model.version = r.get<uint32_t>();
    if (model.version != 1 && model.version != 1060 && model.version != 1062)
      r.fail(ErrorCode::UnsupportedVersion, "unsupported DBO version");
    r.get<uint32_t>();
    r.get<uint32_t>();
    bool root = false, animationRoot = false;
    chunks(
        r,
        [&](uint32_t id, uint32_t n, Reader &src) {
          if (id == 1) {
            root = true;
            payload(src, n, [&](Reader &s) { frame(s, -1, 0); });
            return true;
          }
          if (id == 2) {
            animationRoot = true;
            payload(src, n, [&](Reader &s) { animset(s, 0); });
            return true;
          }
          if (id == 406) {
            payload(src, n, [&](Reader &s) {
              auto count = model.version == 1 ? n : s.get<uint32_t>();
              model.customData = s.array<uint8_t>(count);
            });
            return true;
          }
          return false;
        },
        model.version == 1);
    r.exact();
    if (!root || !animationRoot || model.frames.empty())
      r.fail(ErrorCode::InvalidGeometry, "missing root frame");
  }
};
} // namespace
bool DecodeFromMemory(const void *bytes, size_t size, Model &output,
                      Error &error, const Limits &limits) {
  error = {};
  try {
    if (!bytes || size > limits.maxFileBytes) {
      error = {ErrorCode::ResourceLimit, 0,
               "null input or file size exceeds limit"};
      return false;
    }
    size_t allocated = 0;
    Reader r{static_cast<const uint8_t *>(bytes), 0, size, &limits, &allocated};
    Parser p{{}, limits};
    p.parse(r);
    output = std::move(p.model);
    return true;
  } catch (const Failure &e) {
    error = e.error;
  } catch (const std::bad_alloc &) {
    // Reporting allocation failure must itself avoid an allocation.
    error.code = ErrorCode::AllocationFailure;
    error.offset = 0;
    error.message.clear();
  } catch (const std::length_error &) {
    error = {ErrorCode::ResourceLimit, 0, "allocation length exceeded"};
  }
  return false;
}
bool DecodeFromFile(const std::string &filename, Model &output, Error &error,
                    const Limits &limits) {
  error = {};
  try {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
      error = {ErrorCode::FileIO, 0, "cannot open file"};
      return false;
    }
    auto length = file.tellg();
    if (length < 0 || uint64_t(length) > limits.maxFileBytes) {
      error = {ErrorCode::ResourceLimit, 0, "file exceeds size budget"};
      return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char *>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()))) {
      error = {ErrorCode::FileIO, 0, "cannot read file"};
      return false;
    }
    return DecodeFromMemory(bytes.data(), bytes.size(), output, error, limits);
  } catch (const std::bad_alloc &) {
    error.code = ErrorCode::AllocationFailure;
    error.offset = 0;
    error.message.clear();
    return false;
  } catch (const std::exception &) {
    error = {ErrorCode::FileIO, 0, "file read failed"};
    return false;
  }
}
} // namespace dbo
