// Automated contract tests verifying the ingestion and translation of DBO model
// structures into Wicked Engine's ECS / scene graph (wiScene::Scene,
// wiScene::MeshComponent, wiScene::MaterialComponent, TransformComponent hierarchy,
// ArmatureComponent / bone data, and skeletal animation evaluation).

#include "doctest.h"
#include <WickedEngine.h>
#include "wickedcalls.h"
#include "DBOModel.h"
#include "DBOCompatibility.h"
#include "DBOData.h"
#include "DBOFormat.h"

#include <windows.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_set>

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

#ifndef GGDECLTYPE_D3DCOLOR
#define GGDECLTYPE_D3DCOLOR 4
#endif

static LONG WINAPI CrashReporter(EXCEPTION_POINTERS* ep)
{
	if (ep->ExceptionRecord->ExceptionCode == 0xE06D7363) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	DWORD64 displacement = 0;
	DWORD disp = 0;
	IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
	char symbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + 256] = { 0 };
	IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)symbolBuffer;
	symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	symbol->MaxNameLength = 255;

	DWORD64 addr = (DWORD64)ep->ExceptionRecord->ExceptionAddress;
	printf("\n>>> CRASH at address: 0x%llX (Code: 0x%lX) <<<\n", addr, ep->ExceptionRecord->ExceptionCode);
	if (SymGetSymFromAddr64(process, addr, &displacement, symbol)) {
		printf(">>> Symbol: %s (+0x%llX)\n", symbol->Name, displacement);
	}
	if (SymGetLineFromAddr64(process, addr, &disp, &line)) {
		printf(">>> Source: %s : line %lu\n", line.FileName, line.LineNumber);
	}
	fflush(stdout);
	return EXCEPTION_CONTINUE_SEARCH;
}

static struct CrashInit {
	CrashInit() { AddVectoredExceptionHandler(1, CrashReporter); }
} g_crashInit;


// Global storage required by DBO compatibility layer destructors
std::vector<sObject*> g_vAnimatableObjectList;
int g_iSortedObjectCount = 0;
sObject** g_ppSortedObjectList = nullptr;

namespace wiRectPacker
{
	rect_wh::rect_wh(int w, int h) : w(w), h(h) {}
	rect_xywh::rect_xywh() : rect_wh(0, 0), x(0), y(0) {}
}

namespace wiJobSystem
{
	void Wait(const context& ctx) {}
}

GGCOLOR::GGCOLOR(DWORD col)
{
	const float f = 1.0f / 255.0f;
	r = f * (float)(unsigned char)(col >> 16);
	g = f * (float)(unsigned char)(col >> 8);
	b = f * (float)(unsigned char)col;
	a = f * (float)(unsigned char)(col >> 24);
}

GGCOLOR::operator DWORD() const
{
	DWORD _r = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : (DWORD)(r * 255.0f + 0.5f);
	DWORD _g = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : (DWORD)(g * 255.0f + 0.5f);
	DWORD _b = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : (DWORD)(b * 255.0f + 0.5f);
	DWORD _a = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : (DWORD)(a * 255.0f + 0.5f);
	return (_a << 24) | (_r << 16) | (_g << 8) | _b;
}

void GenerateExtraDataForMeshEx(sMesh* pMesh, BOOL bNormals, BOOL bTangents, BOOL bBinormals, BOOL bDiffuse, BOOL bBones, DWORD dwGenerateMode)
{
}

namespace wiScene {

using namespace wiECS;

Entity Scene::Entity_CreateMaterial(const std::string& name)
{
	Entity entity = CreateEntity();
	names.Create(entity) = name;
	materials.Create(entity);
	return entity;
}

Entity Scene::Entity_CreateMesh(const std::string& name)
{
	Entity entity = CreateEntity();
	names.Create(entity) = name;
	meshes.Create(entity);
	return entity;
}

Entity Scene::Entity_CreateObject(const std::string& name)
{
	Entity entity = CreateEntity();
	names.Create(entity) = name;
	layers.Create(entity);
	transforms.Create(entity);
	prev_transforms.Create(entity);
	aabb_objects.Create(entity);
	objects.Create(entity);
	return entity;
}

Entity Scene::Entity_FindByName(const std::string& name)
{
	for (size_t i = 0; i < names.GetCount(); ++i)
	{
		if (names[i] == name)
			return names.GetEntity(i);
	}
	return INVALID_ENTITY;
}

void Scene::Component_Attach(Entity entity, Entity parent, bool child_already_in_local_space)
{
	if (entity == parent) return;
	if (hierarchy.Contains(entity)) {
		Component_Detach(entity);
	}
	HierarchyComponent& parentcomponent = hierarchy.Create(entity);
	parentcomponent.parentID = parent;

	TransformComponent* transform_parent = transforms.GetComponent(parent);
	if (!transform_parent) transform_parent = &transforms.Create(parent);

	TransformComponent* transform_child = transforms.GetComponent(entity);
	if (!transform_child) {
		transform_child = &transforms.Create(entity);
		transform_parent = transforms.GetComponent(parent);
	}

	if (!child_already_in_local_space) {
		XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world));
		transform_child->MatrixTransform(B);
		transform_child->UpdateTransform();
	}
	transform_child->UpdateTransform_Parented(*transform_parent);

	LayerComponent* layer_parent = layers.GetComponent(parent);
	if (!layer_parent) layer_parent = &layers.Create(parent);
	LayerComponent* layer_child = layers.GetComponent(entity);
	if (!layer_child) layer_child = &layers.Create(entity);
}

void Scene::Component_Detach(Entity entity)
{
	const HierarchyComponent* parent = hierarchy.GetComponent(entity);
	if (parent != nullptr) {
		TransformComponent* transform = transforms.GetComponent(entity);
		if (transform != nullptr) {
			transform->ApplyTransform();
		}
		hierarchy.Remove(entity);
	}
}

void Scene::Entity_Remove(Entity entity)
{
	names.Remove(entity);
	layers.Remove(entity);
	transforms.Remove(entity);
	prev_transforms.Remove(entity);
	hierarchy.Remove(entity);
	materials.Remove(entity);
	meshes.Remove(entity);
	objects.Remove(entity);
	aabb_objects.Remove(entity);
	armatures.Remove(entity);
	animations.Remove(entity);
	animation_datas.Remove(entity);
}

void Scene::Update(float dt)
{
	UpdateSceneTransform(dt);
}

void Scene::UpdateSceneTransform(float dt)
{
	this->dt = dt;
	std::unordered_set<Entity> updated;
	std::unordered_set<Entity> visiting;
	auto updateEntity = [&](auto& self, Entity entity) -> void {
		if (updated.count(entity)) return;
		if (visiting.count(entity)) return;
		visiting.insert(entity);
		TransformComponent* transform = transforms.GetComponent(entity);
		if (!transform) return;
		const HierarchyComponent* hier = hierarchy.GetComponent(entity);
		if (hier && hier->parentID != INVALID_ENTITY) {
			self(self, hier->parentID);
			const TransformComponent* parent = transforms.GetComponent(hier->parentID);
			if (parent) {
				transform->UpdateTransform_Parented(*parent);
				updated.insert(entity);
				return;
			}
		}
		transform->UpdateTransform();
		updated.insert(entity);
	};
	for (size_t i = 0; i < transforms.GetCount(); ++i) {
		updateEntity(updateEntity, transforms.GetEntity(i));
	}
}

void MeshComponent::CreateRenderData()
{
	XMFLOAT3 minPos = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxPos = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (const auto& pos : vertex_positions) {
		minPos.x = (std::min)(minPos.x, pos.x);
		minPos.y = (std::min)(minPos.y, pos.y);
		minPos.z = (std::min)(minPos.z, pos.z);
		maxPos.x = (std::max)(maxPos.x, pos.x);
		maxPos.y = (std::max)(maxPos.y, pos.y);
		maxPos.z = (std::max)(maxPos.z, pos.z);
	}
	aabb = AABB(minPos, maxPos);
}

XMMATRIX TransformComponent::GetLocalMatrix() const
{
	XMVECTOR S_local = XMLoadFloat3(&scale_local);
	XMVECTOR R_local = XMLoadFloat4(&rotation_local);
	XMVECTOR T_local = XMLoadFloat3(&translation_local);
	return
		XMMatrixScalingFromVector(S_local) *
		XMMatrixRotationQuaternion(R_local) *
		XMMatrixTranslationFromVector(T_local);
}

void TransformComponent::UpdateTransform()
{
	if (IsDirty()) {
		SetDirty(false);
		XMStoreFloat4x4(&world, GetLocalMatrix());
	}
}

void TransformComponent::UpdateTransform_Parented(const TransformComponent& parent)
{
	SetDirty(false);
	XMMATRIX W = GetLocalMatrix();
	XMMATRIX W_parent = XMLoadFloat4x4(&parent.world);
	W = W * W_parent;
	XMStoreFloat4x4(&world, W);
}

void TransformComponent::ApplyTransform()
{
	SetDirty();
	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
	XMStoreFloat3(&scale_local, S);
	XMStoreFloat4(&rotation_local, R);
	XMStoreFloat3(&translation_local, T);
}

void TransformComponent::ClearTransform()
{
	SetDirty();
	scale_local = XMFLOAT3(1, 1, 1);
	rotation_local = XMFLOAT4(0, 0, 0, 1);
	translation_local = XMFLOAT3(0, 0, 0);
}

void TransformComponent::Translate(const XMFLOAT3& value)
{
	SetDirty();
	translation_local.x += value.x;
	translation_local.y += value.y;
	translation_local.z += value.z;
}

void TransformComponent::MatrixTransform(const XMMATRIX& matrix)
{
	SetDirty();
	XMMATRIX W = XMLoadFloat4x4(&world);
	W = W * matrix;
	XMStoreFloat4x4(&world, W);
	ApplyTransform();
}

void TransformComponent::Scale(const XMFLOAT3& value)
{
	SetDirty();
	scale_local.x *= value.x;
	scale_local.y *= value.y;
	scale_local.z *= value.z;
}

void TransformComponent::Rotate(const XMFLOAT4& value)
{
	SetDirty();
	XMVECTOR q0 = XMLoadFloat4(&rotation_local);
	XMVECTOR q1 = XMLoadFloat4(&value);
	XMStoreFloat4(&rotation_local, XMQuaternionMultiply(q0, q1));
}

void TransformComponent::Rotate(const XMVECTOR& value)
{
	SetDirty();
	XMVECTOR q0 = XMLoadFloat4(&rotation_local);
	XMStoreFloat4(&rotation_local, XMQuaternionMultiply(q0, value));
}

void TransformComponent::RotateRollPitchYaw(const XMFLOAT3& value)
{
	SetDirty();
	XMVECTOR q0 = XMLoadFloat4(&rotation_local);
	XMVECTOR q1 = XMQuaternionRotationRollPitchYaw(value.x, value.y, value.z);
	XMStoreFloat4(&rotation_local, XMQuaternionMultiply(q0, q1));
}

XMFLOAT3 TransformComponent::GetPosition() const
{
	XMFLOAT3 pos;
	XMStoreFloat3(&pos, XMLoadFloat4x4(&world).r[3]);
	return pos;
}

XMFLOAT4 TransformComponent::GetRotation() const
{
	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
	XMFLOAT4 rot;
	XMStoreFloat4(&rot, R);
	return rot;
}

XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N)
{
	XMVECTOR P = XMLoadFloat3(&mesh.vertex_positions[index]);
	if (index >= mesh.vertex_boneindices.size() || index >= mesh.vertex_boneweights.size())
	{
		return P;
	}
	const XMUINT4& ind = mesh.vertex_boneindices[index];
	const XMFLOAT4& wei = mesh.vertex_boneweights[index];

	auto getBoneMatrix = [&](uint32_t boneIndex) -> XMMATRIX {
		if (boneIndex < armature.boneMatrices.size())
			return armature.boneMatrices[boneIndex];
		return XMMatrixIdentity();
	};
	XMMATRIX M0 = getBoneMatrix(ind.x) * wei.x;
	XMMATRIX M1 = getBoneMatrix(ind.y) * wei.y;
	XMMATRIX M2 = getBoneMatrix(ind.z) * wei.z;
	XMMATRIX M3 = getBoneMatrix(ind.w) * wei.w;

	XMMATRIX finalMatrix = M0 + M1 + M2 + M3;
	P = XMVector3Transform(P, finalMatrix);

	if (N != nullptr && index < mesh.vertex_normals.size())
	{
		*N = XMLoadFloat3(&mesh.vertex_normals[index]);
		*N = XMVector3Normalize(XMVector3TransformNormal(*N, finalMatrix));
	}
	return P;
}

} // namespace wiScene

const char* pestrcasestr(const char* haystack, const char* needle)
{
	if (!haystack || !needle) return nullptr;
	size_t needleLen = strlen(needle);
	if (needleLen == 0) return haystack;
	for (; *haystack; ++haystack) {
		if (_strnicmp(haystack, needle, needleLen) == 0) return haystack;
	}
	return nullptr;
}

bool GetFVFOffsetMapFixedForBones(sMesh* pMesh, sOffsetMap* psOffsetMap)
{
	int iBoneDataOffset = 3;
	memset(psOffsetMap, 0, sizeof(sOffsetMap));

	if (pMesh->dwFVF == 0)
	{
		for (int iElem = 0; pMesh->pVertexDeclaration[iElem].Stream != 255; iElem++)
		{
			if (iElem >= MAX_FVF_DECL_SIZE - 1) break;
			int iIndex = pMesh->pVertexDeclaration[iElem].UsageIndex;
			int iElementOffset = pMesh->pVertexDeclaration[iElem].Offset / sizeof(DWORD);
			if (pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_POSITION)
			{
				psOffsetMap->dwX = iElementOffset + 0;
				psOffsetMap->dwY = iElementOffset + 1;
				psOffsetMap->dwZ = iElementOffset + 2;
			}
			if (pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_NORMAL)
			{
				psOffsetMap->dwNX = iElementOffset + 0;
				psOffsetMap->dwNY = iElementOffset + 1;
				psOffsetMap->dwNZ = iElementOffset + 2;
			}
			if (pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_COLOR && iIndex == 0)
			{
				psOffsetMap->dwDiffuse = iElementOffset + 0;
			}
			if (pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_TEXCOORD)
			{
				if (pMesh->pVertexDeclaration[iElem].Type == GGDECLTYPE_FLOAT4)
				{
					if (iBoneDataOffset == 3 || iBoneDataOffset == 4)
					{
						psOffsetMap->dwTU[iBoneDataOffset] = iElementOffset + 0;
						psOffsetMap->dwTV[iBoneDataOffset] = iElementOffset + 1;
						psOffsetMap->dwTZ[iBoneDataOffset] = iElementOffset + 2;
						psOffsetMap->dwTW[iBoneDataOffset] = iElementOffset + 3;
						iBoneDataOffset++;
					}
				}
				else
				{
					psOffsetMap->dwTU[iIndex] = iElementOffset + 0;
					if (pMesh->pVertexDeclaration[iElem].Type == GGDECLTYPE_FLOAT2)
					{
						psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
					}
					if (pMesh->pVertexDeclaration[iElem].Type == GGDECLTYPE_FLOAT3)
					{
						psOffsetMap->dwTV[iIndex] = iElementOffset + 1;
						psOffsetMap->dwTZ[iIndex] = iElementOffset + 2;
					}
				}
			}
			if (pMesh->pVertexDeclaration[iElem].Usage == GGDECLUSAGE_TANGENT)
			{
				psOffsetMap->dwTU[1] = iElementOffset + 0;
				psOffsetMap->dwTV[1] = iElementOffset + 1;
				psOffsetMap->dwTZ[1] = iElementOffset + 2;
				psOffsetMap->dwTW[1] = iElementOffset + 3;
			}
		}
		psOffsetMap->dwByteSize = pMesh->dwFVFSize;
		psOffsetMap->dwSize = pMesh->dwFVFSize / sizeof(DWORD);
		return true;
	}
	else
	{
		int iOffset = 0;
		if (pMesh->dwFVF & GGFVF_XYZ)
		{
			psOffsetMap->dwX = iOffset + 0;
			psOffsetMap->dwY = iOffset + 1;
			psOffsetMap->dwZ = iOffset + 2;
			iOffset += 3;
		}
		if (pMesh->dwFVF & GGFVF_NORMAL)
		{
			psOffsetMap->dwNX = iOffset + 0;
			psOffsetMap->dwNY = iOffset + 1;
			psOffsetMap->dwNZ = iOffset + 2;
			iOffset += 3;
		}
		if (pMesh->dwFVF & GGFVF_DIFFUSE)
		{
			psOffsetMap->dwDiffuse = iOffset + 0;
			iOffset += 1;
		}
		DWORD dwTexCount = 0;
		if ((pMesh->dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX1) dwTexCount = 1;
		if ((pMesh->dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX2) dwTexCount = 2;
		if ((pMesh->dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX3) dwTexCount = 3;
		if ((pMesh->dwFVF & GGFVF_TEXCOUNT_MASK) == GGFVF_TEX4) dwTexCount = 4;
		for (DWORD dwTexCoordSet = 0; dwTexCoordSet < dwTexCount; dwTexCoordSet++)
		{
			psOffsetMap->dwTU[dwTexCoordSet] = iOffset + 0;
			psOffsetMap->dwTV[dwTexCoordSet] = iOffset + 1;
			iOffset += 2;
		}
		psOffsetMap->dwByteSize = sizeof(DWORD) * iOffset;
		psOffsetMap->dwSize = iOffset;
		return true;
	}
}

namespace {

dbo::Matrix MakeIdentity()
{
	return {{1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1}};
}

dbo::Matrix MakeTranslation(float x, float y, float z)
{
	return {{1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1}};
}

dbo::Matrix MatrixFromXMMatrix(const XMMATRIX& M)
{
	XMFLOAT4X4 f;
	XMStoreFloat4x4(&f, M);
	return {{
		f._11, f._12, f._13, f._14,
		f._21, f._22, f._23, f._24,
		f._31, f._32, f._33, f._34,
		f._41, f._42, f._43, f._44
	}};
}

// Evaluates an AnimationComponent at the specified time step, interpolating
// translation, rotation, and scale channels, and propagates the transforms
// down the scene hierarchy.
void EvaluateAnimationAtTime(wiScene::Scene& scene, wiScene::AnimationComponent& anim, float time)
{
	anim.timer = time;
	for (const auto& channel : anim.channels)
	{
		if (channel.samplerIndex >= anim.samplers.size() || channel.target == wiECS::INVALID_ENTITY)
			continue;
		const auto& sampler = anim.samplers[channel.samplerIndex];
		wiScene::TransformComponent* transform = scene.transforms.GetComponent(channel.target);
		if (!transform) continue;

		const auto& times = sampler.backwards_compatibility_data.keyframe_times;
		const auto& data = sampler.backwards_compatibility_data.keyframe_data;
		if (times.empty()) continue;

		int left = 0;
		int right = 0;
		if (times.back() <= time)
		{
			left = right = static_cast<int>(times.size()) - 1;
		}
		else
		{
			while (right < static_cast<int>(times.size()) && times[right] < time) {
				right++;
			}
			left = (std::max)(0, right - 1);
		}

		float t = 0.0f;
		if (left != right)
		{
			float dt = times[right] - times[left];
			if (dt > 0.00001f)
				t = (time - times[left]) / dt;
		}

		if (channel.path == wiScene::AnimationComponent::AnimationChannel::Path::TRANSLATION)
		{
			const XMFLOAT3* vecData = reinterpret_cast<const XMFLOAT3*>(data.data());
			XMVECTOR v0 = XMLoadFloat3(&vecData[left]);
			XMVECTOR v1 = XMLoadFloat3(&vecData[right]);
			XMVECTOR v = XMVectorLerp(v0, v1, t);
			XMStoreFloat3(&transform->translation_local, v);
			transform->SetDirty();
		}
		else if (channel.path == wiScene::AnimationComponent::AnimationChannel::Path::ROTATION)
		{
			const XMFLOAT4* quatData = reinterpret_cast<const XMFLOAT4*>(data.data());
			XMVECTOR q0 = XMLoadFloat4(&quatData[left]);
			XMVECTOR q1 = XMLoadFloat4(&quatData[right]);
			XMVECTOR q = XMQuaternionSlerp(q0, q1, t);
			XMStoreFloat4(&transform->rotation_local, q);
			transform->SetDirty();
		}
		else if (channel.path == wiScene::AnimationComponent::AnimationChannel::Path::SCALE)
		{
			const XMFLOAT3* vecData = reinterpret_cast<const XMFLOAT3*>(data.data());
			XMVECTOR s0 = XMLoadFloat3(&vecData[left]);
			XMVECTOR s1 = XMLoadFloat3(&vecData[right]);
			XMVECTOR s = XMVectorLerp(s0, s1, t);
			XMStoreFloat3(&transform->scale_local, s);
			transform->SetDirty();
		}
	}

	// Propagate updated hierarchy transforms
	scene.UpdateSceneTransform(0.0f);
}

// Calculates bone skinning matrices M_i = inverseBind_i * boneWorld_i * armatureWorld_inv
void UpdateArmatureSkinningMatrices(wiScene::Scene& scene, wiScene::ArmatureComponent& armature, wiECS::Entity armatureEntity)
{
	armature.boneMatrices.resize(armature.boneCollection.size());
	const wiScene::TransformComponent* armatureTransform = scene.transforms.GetComponent(armatureEntity);
	XMMATRIX R = XMMatrixIdentity();
	if (armatureTransform)
	{
		R = XMMatrixInverse(nullptr, XMLoadFloat4x4(&armatureTransform->world));
	}

	for (size_t i = 0; i < armature.boneCollection.size(); ++i)
	{
		wiECS::Entity boneEntity = armature.boneCollection[i];
		const wiScene::TransformComponent* boneTransform = scene.transforms.GetComponent(boneEntity);
		if (boneTransform && i < armature.inverseBindMatrices.size())
		{
			XMMATRIX B = XMLoadFloat4x4(&armature.inverseBindMatrices[i]);
			XMMATRIX W = XMLoadFloat4x4(&boneTransform->world);
			XMMATRIX M = B * W * R;
			armature.boneMatrices[i] = M;
		}
		else
		{
			armature.boneMatrices[i] = XMMatrixIdentity();
		}
	}
}

// Builds a comprehensive skinned DBO model with multi-level hierarchy,
// vertex attributes (pos, normal, UV, bone weights/indices), material,
// textures, and an animation set.
dbo::Model BuildSkinnedDBOModel()
{
	dbo::Model m;

	// Frame hierarchy: Root -> Pelvis -> Arm
	dbo::Frame root;
	root.name = "root";
	root.matrix = MakeIdentity();
	root.mesh = 0;
	m.frames.push_back(root);

	dbo::Frame pelvis;
	pelvis.name = "pelvis";
	pelvis.parent = 0;
	pelvis.matrix = MakeTranslation(0.0f, 10.0f, 0.0f);
	m.frames.push_back(pelvis);

	dbo::Frame arm;
	arm.name = "arm";
	arm.parent = 1;
	arm.matrix = MakeTranslation(5.0f, 0.0f, 0.0f);
	m.frames.push_back(arm);

	// Mesh: 3 vertices with vertex declaration
	// Layout: pos(float3) + normal(float3) + uv0(float2) + weights(float4) + indices(float4) = 64 bytes
	dbo::Mesh mesh;
	mesh.fvf = 0; // custom declaration
	mesh.stride = 64;
	mesh.vertexCount = 3;
	mesh.drawVertexCount = 3;
	mesh.drawPrimitiveCount = 1;
	mesh.primitiveType = GGPT_TRIANGLELIST;

	mesh.declaration = {
		{0, 0,  GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_POSITION, 0},
		{0, 12, GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_NORMAL, 0},
		{0, 24, GGDECLTYPE_FLOAT2, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 0},
		{0, 32, GGDECLTYPE_FLOAT4, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 3}, // weights
		{0, 48, GGDECLTYPE_FLOAT4, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 4}, // indices
		{255, 0, 0, 0, 0, 0}                                                       // terminator
	};

	struct SkinnedVertex {
		float pos[3];
		float normal[3];
		float uv[2];
		float weights[4];
		float indices[4];
	};

	// Bind poses: Pelvis is at world (0, 10, 0), Arm is at world (5, 10, 0)
	// Vertex 0: (0, 0, 0) influenced solely by Pelvis (bone 0, weight 1.0)
	// Vertex 1: (5, 0, 0) influenced solely by Arm (bone 1, weight 1.0)
	// Vertex 2: (2.5, 5, 0) dual influenced: 40% Pelvis (bone 0), 60% Arm (bone 1)
	SkinnedVertex verts[3] = {
		{ {0.0f, 0.0f, 0.0f},   {0.0f, 1.0f, 0.0f}, {0.1f, 0.2f}, {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f} },
		{ {5.0f, 0.0f, 0.0f},   {0.0f, 1.0f, 0.0f}, {0.5f, 0.8f}, {1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f} },
		{ {2.5f, 5.0f, 0.0f},   {0.0f, 0.0f, 1.0f}, {0.9f, 0.4f}, {0.4f, 0.6f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f} }
	};

	mesh.vertexData.resize(sizeof(verts));
	std::memcpy(mesh.vertexData.data(), verts, sizeof(verts));

	mesh.indices = {0, 1, 2};
	mesh.lodIndices[0] = {2, 1, 0}; // LOD1
	mesh.visible = true;
	mesh.light = true;
	mesh.cull = false;        // double-sided
	mesh.transparency = false; // opaque

	mesh.material.diffuse = {{0.8f, 0.2f, 0.3f, 1.0f}};
	mesh.useMaterial = true;

	dbo::Texture tex;
	tex.name = "textures/character_diffuse.dds";
	mesh.textures.push_back(tex);

	// Bones: inverse bind matrices
	// Pelvis is at world (0, 10, 0), so its inverseBind translates by (0, -10, 0)
	dbo::Bone bonePelvis;
	bonePelvis.name = "pelvis";
	bonePelvis.inverseBind = MakeTranslation(0.0f, -10.0f, 0.0f);
	bonePelvis.vertices = {0, 2};
	bonePelvis.weights = {1.0f, 0.4f};
	mesh.bones.push_back(bonePelvis);

	// Arm is at world (5, 10, 0), so its inverseBind translates by (-5, -10, 0)
	dbo::Bone boneArm;
	boneArm.name = "arm";
	boneArm.inverseBind = MakeTranslation(-5.0f, -10.0f, 0.0f);
	boneArm.vertices = {1, 2};
	boneArm.weights = {1.0f, 0.6f};
	mesh.bones.push_back(boneArm);

	m.meshes.push_back(mesh);

	// AnimationSet "walk" with 40-frame length
	dbo::AnimationSet anim;
	anim.name = "walk";
	anim.length = 40;
	anim.finish = 40;

	dbo::AnimationTrack armTrack;
	armTrack.name = "arm";
	// Translation: key 0 at local (5, 0, 0), key 40 at local (15, 0, 0) (+10 in X)
	armTrack.positions.push_back({0,  {{5.0f, 0.0f, 0.0f}},  {{0, 0, 0}}});
	armTrack.positions.push_back({40, {{15.0f, 0.0f, 0.0f}}, {{0, 0, 0}}});
	// Rotation: key 0 and key 40 are identity (verifies rotation channel is created and sampled)
	armTrack.rotations.push_back({0,  {{0.0f, 0.0f, 0.0f, 1.0f}}});
	armTrack.rotations.push_back({40, {{0.0f, 0.0f, 0.0f, 1.0f}}});
	// Scale: key 0 is (1, 1, 1), key 40 is (1, 1, 2) (creates scale channel while keeping X/Y planar motion unscaled)
	armTrack.scales.push_back({0,  {{1.0f, 1.0f, 1.0f}}, {{0, 0, 0}}});
	armTrack.scales.push_back({40, {{1.0f, 1.0f, 2.0f}}, {{0, 0, 0}}});

	anim.tracks.push_back(armTrack);
	m.animations.push_back(anim);

	return m;
}

// Builds a multi-mesh model with distinct materials (opaque vs transparent)
// and hierarchy to verify material mapping and transparency contracts.
dbo::Model BuildMultiMeshMaterialModel()
{
	dbo::Model m;

	dbo::Frame root;
	root.name = "root";
	root.matrix = MakeTranslation(0.0f, 5.0f, 0.0f);
	root.mesh = 0;
	m.frames.push_back(root);

	dbo::Frame glassFrame;
	glassFrame.name = "glass_canopy";
	glassFrame.parent = 0;
	glassFrame.matrix = MakeTranslation(0.0f, 2.0f, 0.0f);
	glassFrame.mesh = 1;
	m.frames.push_back(glassFrame);

	// Mesh 0: Opaque Body
	dbo::Mesh bodyMesh;
	bodyMesh.fvf = GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1;
	bodyMesh.stride = 32;
	bodyMesh.vertexCount = 3;
	bodyMesh.drawVertexCount = 3;
	bodyMesh.drawPrimitiveCount = 1;
	bodyMesh.primitiveType = GGPT_TRIANGLELIST;
	bodyMesh.cull = true;        // single-sided
	bodyMesh.transparency = false; // opaque

	float bodyVerts[3][8] = {
		{0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
		{2.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}
	};
	bodyMesh.vertexData.resize(sizeof(bodyVerts));
	std::memcpy(bodyMesh.vertexData.data(), bodyVerts, sizeof(bodyVerts));
	bodyMesh.indices = {0, 1, 2};
	dbo::Texture bodyTex;
	bodyTex.name = "textures/body_diffuse.dds";
	bodyMesh.textures.push_back(bodyTex);
	m.meshes.push_back(bodyMesh);

	// Mesh 1: Transparent Glass
	dbo::Mesh glassMesh;
	glassMesh.fvf = GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1;
	glassMesh.stride = 32;
	glassMesh.vertexCount = 3;
	glassMesh.drawVertexCount = 3;
	glassMesh.drawPrimitiveCount = 1;
	glassMesh.primitiveType = GGPT_TRIANGLELIST;
	glassMesh.cull = false;       // double-sided
	glassMesh.transparency = true; // alpha blended

	float glassVerts[3][8] = {
		{0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
		{0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f}
	};
	glassMesh.vertexData.resize(sizeof(glassVerts));
	std::memcpy(glassMesh.vertexData.data(), glassVerts, sizeof(glassVerts));
	glassMesh.indices = {0, 1, 2};
	dbo::Texture glassTex;
	glassTex.name = "textures/glass_canopy.dds";
	glassMesh.textures.push_back(glassTex);
	m.meshes.push_back(glassMesh);

	return m;
}

// Binary DBO serializer helper matching the chunk specification
using Bytes = std::vector<uint8_t>;

void PushU32(Bytes& b, uint32_t val)
{
	for (int i = 0; i < 4; ++i)
		b.push_back(static_cast<uint8_t>(val >> (i * 8)));
}

void PushChunk(Bytes& b, uint32_t id, const Bytes& payload)
{
	PushU32(b, id);
	PushU32(b, static_cast<uint32_t>(payload.size()));
	b.insert(b.end(), payload.begin(), payload.end());
}

void PushEnd(Bytes& b)
{
	PushU32(b, 0);
	PushU32(b, 0);
}

Bytes MakeStringBytes(const char* s)
{
	Bytes b;
	uint32_t len = static_cast<uint32_t>(strlen(s));
	PushU32(b, len);
	b.insert(b.end(), s, s + len);
	return b;
}

Bytes MakeScalarBytes(uint32_t val)
{
	Bytes b;
	PushU32(b, val);
	return b;
}

Bytes MakeFloatBytes(float f)
{
	uint32_t bits = 0;
	std::memcpy(&bits, &f, 4);
	return MakeScalarBytes(bits);
}

Bytes MakeMatrixBytes(const dbo::Matrix& mat)
{
	Bytes b;
	for (size_t i = 0; i < 16; ++i)
	{
		uint32_t bits = 0;
		float val = mat[i];
		std::memcpy(&bits, &val, 4);
		PushU32(b, bits);
	}
	return b;
}

// Constructs valid binary MAGICDBO bytes containing hierarchy, mesh,
// bone influences, and animations
Bytes BuildBinarySkinnedDBOData()
{
	Bytes meshChunk;
	PushChunk(meshChunk, 111, MakeScalarBytes(0x112)); // FVF
	PushChunk(meshChunk, 112, MakeScalarBytes(32));    // stride
	PushChunk(meshChunk, 113, MakeScalarBytes(3));     // vertexCount
	PushChunk(meshChunk, 114, MakeScalarBytes(3));     // indexCount

	float v[3][8] = {
		{0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.1f, 0.2f},
		{4.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.6f},
		{0.0f, 6.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.8f, 0.9f}
	};
	Bytes vdata(sizeof(v));
	std::memcpy(vdata.data(), v, sizeof(v));
	PushChunk(meshChunk, 115, vdata);
	PushChunk(meshChunk, 116, Bytes{0, 0, 1, 0, 2, 0}); // indices: 0, 1, 2

	// Texture chunk
	PushChunk(meshChunk, 127, MakeScalarBytes(1)); // 1 texture
	Bytes texChunk;
	PushChunk(texChunk, 141, MakeStringBytes("textures/binary_model.dds"));
	PushEnd(texChunk);
	PushEnd(texChunk);
	PushChunk(meshChunk, 128, texChunk);

	PushEnd(meshChunk);

	// Frames
	Bytes rootFrame;
	PushChunk(rootFrame, 101, MakeStringBytes("root"));
	PushChunk(rootFrame, 103, meshChunk);

	Bytes childFrame;
	PushChunk(childFrame, 101, MakeStringBytes("bone_leg"));
	PushChunk(childFrame, 102, MakeMatrixBytes(MakeTranslation(4.0f, 0.0f, 0.0f)));
	PushEnd(childFrame);

	PushChunk(rootFrame, 104, childFrame);
	PushEnd(rootFrame);

	// Animation set
	Bytes track;
	PushChunk(track, 211, MakeStringBytes("bone_leg"));
	PushChunk(track, 212, MakeScalarBytes(2)); // 2 position keys
	Bytes posKeys;
	PushU32(posKeys, 0); // time 0
	for (int i = 0; i < 3; ++i) PushU32(posKeys, 0); // (0, 0, 0)
	for (int i = 0; i < 3; ++i) PushU32(posKeys, 0); // spline tangents
	PushU32(posKeys, 30); // time 30
	PushU32(posKeys, 0x41000000); // x = 8.0f
	PushU32(posKeys, 0);
	PushU32(posKeys, 0);
	for (int i = 0; i < 3; ++i) PushU32(posKeys, 0);
	PushChunk(track, 213, posKeys);
	PushEnd(track);

	Bytes animSet;
	PushChunk(animSet, 201, MakeStringBytes("leg_kick"));
	PushChunk(animSet, 202, MakeScalarBytes(30));
	PushChunk(animSet, 203, track);
	PushEnd(animSet);

	// Wrap into MAGICDBO
	Bytes dbo;
	Bytes magic = MakeStringBytes("MAGICDBO");
	dbo.insert(dbo.end(), magic.begin(), magic.end());
	PushU32(dbo, 1062); // version
	PushU32(dbo, 0);
	PushU32(dbo, 0);
	PushChunk(dbo, 1, rootFrame);
	PushChunk(dbo, 2, animSet);
	PushEnd(dbo);

	return dbo;
}

} // namespace

// ============================================================================
// TEST 1: DBO Model Geometry Conversion into wiScene::MeshComponent
// ============================================================================
TEST_CASE("DBO Model Geometry Conversion into Wicked Engine wiScene::MeshComponent")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));
	REQUIRE(pObject != nullptr);

	// Ingest into Wicked Engine ECS
	WickedCall_AddObject(pObject);
	REQUIRE(pObject->wickedrootentityindex != wiECS::INVALID_ENTITY);

	sMesh* pDBOMesh = pObject->ppMeshList[0];
	REQUIRE(pDBOMesh != nullptr);
	REQUIRE(pDBOMesh->wickedmeshindex != wiECS::INVALID_ENTITY);

	wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pDBOMesh->wickedmeshindex);
	REQUIRE(mesh != nullptr);

	// 1. Vertex positions
	REQUIRE(mesh->vertex_positions.size() == 3);
	CHECK(mesh->vertex_positions[0].x == doctest::Approx(0.0f));
	CHECK(mesh->vertex_positions[0].y == doctest::Approx(0.0f));
	CHECK(mesh->vertex_positions[0].z == doctest::Approx(0.0f));

	CHECK(mesh->vertex_positions[1].x == doctest::Approx(5.0f));
	CHECK(mesh->vertex_positions[1].y == doctest::Approx(0.0f));
	CHECK(mesh->vertex_positions[1].z == doctest::Approx(0.0f));

	CHECK(mesh->vertex_positions[2].x == doctest::Approx(2.5f));
	CHECK(mesh->vertex_positions[2].y == doctest::Approx(5.0f));
	CHECK(mesh->vertex_positions[2].z == doctest::Approx(0.0f));

	// 2. Vertex normals
	REQUIRE(mesh->vertex_normals.size() == 3);
	CHECK(mesh->vertex_normals[0].x == doctest::Approx(0.0f));
	CHECK(mesh->vertex_normals[0].y == doctest::Approx(1.0f));
	CHECK(mesh->vertex_normals[0].z == doctest::Approx(0.0f));
	CHECK(mesh->vertex_normals[2].z == doctest::Approx(1.0f));

	// 3. Texture coordinates UV0
	REQUIRE(mesh->vertex_uvset_0.size() == 3);
	CHECK(mesh->vertex_uvset_0[0].x == doctest::Approx(0.1f));
	CHECK(mesh->vertex_uvset_0[0].y == doctest::Approx(0.2f));
	CHECK(mesh->vertex_uvset_0[1].x == doctest::Approx(0.5f));
	CHECK(mesh->vertex_uvset_0[1].y == doctest::Approx(0.8f));
	CHECK(mesh->vertex_uvset_0[2].x == doctest::Approx(0.9f));
	CHECK(mesh->vertex_uvset_0[2].y == doctest::Approx(0.4f));

	// 4. Triangle indices and winding order conversion (DBO {0, 1, 2} -> Wicked {0, 2, 1})
	REQUIRE(mesh->indices.size() >= 3);
	CHECK(mesh->indices[0] == 0);
	CHECK(mesh->indices[1] == 2);
	CHECK(mesh->indices[2] == 1);

	// 5. Sub-mesh subsets
	REQUIRE(mesh->subsets.size() >= 2);
	// Base subset
	CHECK(mesh->subsets[0].materialID == pDBOMesh->wickedmaterialindex);
	CHECK(mesh->subsets[0].indexOffset == 0);
	CHECK(mesh->subsets[0].indexCount == 3);
	CHECK(mesh->subsets[0].active == true);

	// LOD1 subset
	CHECK(mesh->lodlevels == 1);
	CHECK(mesh->subsets[1].materialID == pDBOMesh->wickedmaterialindex);
	CHECK(mesh->subsets[1].indexOffset == 3);
	CHECK(mesh->subsets[1].indexCount == 3);
	CHECK(mesh->subsets[1].active == false);
	// LOD1 indices: input {2, 1, 0} -> winding {2, 0, 1}
	CHECK(mesh->indices[3] == 2);
	CHECK(mesh->indices[4] == 0);
	CHECK(mesh->indices[5] == 1);

	// 6. Culling / double-sided setting
	CHECK(mesh->IsDoubleSided() == true);

	// Cleanup
	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 2: Material and Texture Mapping into wiScene::MaterialComponent
// ============================================================================
TEST_CASE("Material and Texture Mapping into Wicked Engine wiScene::MaterialComponent")
{
	std::string error;
	dbo::Model m = BuildMultiMeshMaterialModel();

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));
	REQUIRE(pObject != nullptr);

	WickedCall_AddObject(pObject);

	// Mesh 0: Opaque Body
	sMesh* pBodyMesh = pObject->ppMeshList[0];
	REQUIRE(pBodyMesh != nullptr);
	REQUIRE(pBodyMesh->wickedmaterialindex != wiECS::INVALID_ENTITY);

	wiScene::MaterialComponent* bodyMat = wiScene::GetScene().materials.GetComponent((wiECS::Entity)pBodyMesh->wickedmaterialindex);
	REQUIRE(bodyMat != nullptr);

	CHECK(bodyMat->baseColor.x == doctest::Approx(1.0f));
	CHECK(bodyMat->baseColor.y == doctest::Approx(1.0f));
	CHECK(bodyMat->baseColor.z == doctest::Approx(1.0f));
	CHECK(bodyMat->baseColor.w == doctest::Approx(1.0f));
	CHECK(bodyMat->userBlendMode == BLENDMODE_OPAQUE);
	CHECK(bodyMat->IsCastingShadow() == true);

	wiScene::MeshComponent* bodyWickedMesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pBodyMesh->wickedmeshindex);
	REQUIRE(bodyWickedMesh != nullptr);
	CHECK(bodyWickedMesh->subsets[0].materialID == pBodyMesh->wickedmaterialindex);
	CHECK(bodyWickedMesh->IsDoubleSided() == false); // bCull = true

	// Mesh 1: Transparent Glass Canopy
	sMesh* pGlassMesh = pObject->ppMeshList[1];
	REQUIRE(pGlassMesh != nullptr);
	REQUIRE(pGlassMesh->wickedmaterialindex != wiECS::INVALID_ENTITY);

	wiScene::MaterialComponent* glassMat = wiScene::GetScene().materials.GetComponent((wiECS::Entity)pGlassMesh->wickedmaterialindex);
	REQUIRE(glassMat != nullptr);

	CHECK(glassMat->userBlendMode == BLENDMODE_ALPHA);
	CHECK(glassMat->IsCastingShadow() == false);

	wiScene::MeshComponent* glassWickedMesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pGlassMesh->wickedmeshindex);
	REQUIRE(glassWickedMesh != nullptr);
	CHECK(glassWickedMesh->subsets[0].materialID == pGlassMesh->wickedmaterialindex);
	CHECK(glassWickedMesh->IsDoubleSided() == true); // bCull = false

	// Distinct materials allocated per mesh
	CHECK(pBodyMesh->wickedmaterialindex != pGlassMesh->wickedmaterialindex);

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 3: Frame Hierarchy and Transforms Mapping into wiScene::TransformComponent
// ============================================================================
TEST_CASE("Frame Hierarchy and Transforms Mapping into Wicked Engine Transform / Hierarchy Components")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	// Verify frames created in scene
	sFrame* pRoot = pObject->ppFrameList[0];
	sFrame* pPelvis = pObject->ppFrameList[1];
	sFrame* pArm = pObject->ppFrameList[2];

	REQUIRE(pRoot != nullptr);
	REQUIRE(pPelvis != nullptr);
	REQUIRE(pArm != nullptr);

	wiECS::Entity rootEntity = (wiECS::Entity)pRoot->wickedobjindex;
	wiECS::Entity pelvisEntity = (wiECS::Entity)pPelvis->wickedobjindex;
	wiECS::Entity armEntity = (wiECS::Entity)pArm->wickedobjindex;

	REQUIRE(rootEntity != wiECS::INVALID_ENTITY);
	REQUIRE(pelvisEntity != wiECS::INVALID_ENTITY);
	REQUIRE(armEntity != wiECS::INVALID_ENTITY);

	// Verify NameComponents
	CHECK(wiScene::GetScene().names.GetComponent(rootEntity)->name == "root");
	CHECK(wiScene::GetScene().names.GetComponent(pelvisEntity)->name == "pelvis");
	CHECK(wiScene::GetScene().names.GetComponent(armEntity)->name == "arm");

	// Verify HierarchyComponent parenting:
	// Pelvis parent is root
	const wiScene::HierarchyComponent* hierPelvis = wiScene::GetScene().hierarchy.GetComponent(pelvisEntity);
	REQUIRE(hierPelvis != nullptr);
	CHECK(hierPelvis->parentID == rootEntity);

	// Arm parent is pelvis
	const wiScene::HierarchyComponent* hierArm = wiScene::GetScene().hierarchy.GetComponent(armEntity);
	REQUIRE(hierArm != nullptr);
	CHECK(hierArm->parentID == pelvisEntity);

	// Verify world transform evaluations
	wiScene::GetScene().UpdateSceneTransform(0.0f);

	const wiScene::TransformComponent* transRoot = wiScene::GetScene().transforms.GetComponent(rootEntity);
	const wiScene::TransformComponent* transPelvis = wiScene::GetScene().transforms.GetComponent(pelvisEntity);
	const wiScene::TransformComponent* transArm = wiScene::GetScene().transforms.GetComponent(armEntity);

	REQUIRE(transRoot != nullptr);
	REQUIRE(transPelvis != nullptr);
	REQUIRE(transArm != nullptr);

	// Root world position (0, 0, 0)
	CHECK(transRoot->world._41 == doctest::Approx(0.0f));
	CHECK(transRoot->world._42 == doctest::Approx(0.0f));
	CHECK(transRoot->world._43 == doctest::Approx(0.0f));

	// Pelvis world position = local (0, 10, 0) * root = (0, 10, 0)
	CHECK(transPelvis->world._41 == doctest::Approx(0.0f));
	CHECK(transPelvis->world._42 == doctest::Approx(10.0f));
	CHECK(transPelvis->world._43 == doctest::Approx(0.0f));

	// Arm world position = local (5, 0, 0) * pelvis world (0, 10, 0) = (5, 10, 0)
	CHECK(transArm->world._41 == doctest::Approx(5.0f));
	CHECK(transArm->world._42 == doctest::Approx(10.0f));
	CHECK(transArm->world._43 == doctest::Approx(0.0f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 4: Bone/Armature and Skeletal Skinning Mapping into wiScene::ArmatureComponent
// ============================================================================
TEST_CASE("Bone / Armature and Skeletal Skinning Data Mapping into Wicked Engine ArmatureComponent")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sMesh* pMesh = pObject->ppMeshList[0];
	REQUIRE(pMesh != nullptr);
	REQUIRE(pMesh->wickedarmatureindex != wiECS::INVALID_ENTITY);

	wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pMesh->wickedmeshindex);
	REQUIRE(mesh != nullptr);
	CHECK(mesh->armatureID == pMesh->wickedarmatureindex);

	wiScene::ArmatureComponent* armature = wiScene::GetScene().armatures.GetComponent((wiECS::Entity)pMesh->wickedarmatureindex);
	REQUIRE(armature != nullptr);

	// 1. Inverse bind matrices mapping
	REQUIRE(armature->inverseBindMatrices.size() == 2);
	// Bone 0: Pelvis inverseBind was (0, -10, 0)
	CHECK(armature->inverseBindMatrices[0]._41 == doctest::Approx(0.0f));
	CHECK(armature->inverseBindMatrices[0]._42 == doctest::Approx(-10.0f));
	CHECK(armature->inverseBindMatrices[0]._43 == doctest::Approx(0.0f));
	CHECK(armature->inverseBindMatrices[0]._11 == doctest::Approx(1.0f));

	// Bone 1: Arm inverseBind was (-5, -10, 0)
	CHECK(armature->inverseBindMatrices[1]._41 == doctest::Approx(-5.0f));
	CHECK(armature->inverseBindMatrices[1]._42 == doctest::Approx(-10.0f));
	CHECK(armature->inverseBindMatrices[1]._43 == doctest::Approx(0.0f));
	CHECK(armature->inverseBindMatrices[1]._11 == doctest::Approx(1.0f));

	// 2. Bone collection frame mapping
	REQUIRE(armature->boneCollection.size() == 2);
	CHECK(armature->boneCollection[0] == pObject->ppFrameList[1]->wickedobjindex); // pelvis frame
	CHECK(armature->boneCollection[1] == pObject->ppFrameList[2]->wickedobjindex); // arm frame

	// 3. Vertex bone weights and indices
	REQUIRE(mesh->vertex_boneweights.size() == 3);
	REQUIRE(mesh->vertex_boneindices.size() == 3);

	// Vertex 0: Pelvis influence (weight 1.0, index 0)
	CHECK(mesh->vertex_boneindices[0].x == 0);
	CHECK(mesh->vertex_boneweights[0].x == doctest::Approx(1.0f));

	// Vertex 1: Arm influence (weight 1.0, index 1)
	CHECK(mesh->vertex_boneindices[1].x == 1);
	CHECK(mesh->vertex_boneweights[1].x == doctest::Approx(1.0f));

	// Vertex 2: Dual influence (40% Pelvis, 60% Arm)
	CHECK(mesh->vertex_boneindices[2].x == 0);
	CHECK(mesh->vertex_boneindices[2].y == 1);
	CHECK(mesh->vertex_boneweights[2].x == doctest::Approx(0.4f));
	CHECK(mesh->vertex_boneweights[2].y == doctest::Approx(0.6f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 5: Animation Track Evaluation and Bone Transform Calculations
// ============================================================================
TEST_CASE("Animation Track Evaluation and Skeletal Pose / Bone Transform Calculations")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sMesh* pMesh = pObject->ppMeshList[0];
	wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pMesh->wickedmeshindex);
	wiScene::ArmatureComponent* armature = wiScene::GetScene().armatures.GetComponent((wiECS::Entity)pMesh->wickedarmatureindex);
	REQUIRE(mesh != nullptr);
	REQUIRE(armature != nullptr);

	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	REQUIRE(pAnimSet != nullptr);
	REQUIRE(pAnimSet->wickedanimentityindex != wiECS::INVALID_ENTITY);

	wiScene::AnimationComponent* anim = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pAnimSet->wickedanimentityindex);
	REQUIRE(anim != nullptr);

	// Verify channels & samplers created
	REQUIRE(anim->channels.size() >= 3);
	REQUIRE(anim->samplers.size() >= 3);

	// ------------------------------------------------------------------------
	// Phase 1: Bind Pose Evaluation at time t = 0
	// ------------------------------------------------------------------------
	EvaluateAnimationAtTime(wiScene::GetScene(), *anim, 0.0f);
	UpdateArmatureSkinningMatrices(wiScene::GetScene(), *armature, (wiECS::Entity)pMesh->wickedarmatureindex);

	// Bone 0 skinning matrix: invBind(0, -10, 0) * world(0, 10, 0) = Identity
	CHECK(armature->boneMatrices[0].r[3].m128_f32[0] == doctest::Approx(0.0f));
	CHECK(armature->boneMatrices[0].r[3].m128_f32[1] == doctest::Approx(0.0f));
	CHECK(armature->boneMatrices[0].r[3].m128_f32[2] == doctest::Approx(0.0f));

	// Bone 1 skinning matrix: invBind(-5, -10, 0) * world(5, 10, 0) = Identity
	CHECK(armature->boneMatrices[1].r[3].m128_f32[0] == doctest::Approx(0.0f));
	CHECK(armature->boneMatrices[1].r[3].m128_f32[1] == doctest::Approx(0.0f));
	CHECK(armature->boneMatrices[1].r[3].m128_f32[2] == doctest::Approx(0.0f));

	// Skinned vertex positions match bind pose exactly:
	XMVECTOR p0 = wiScene::SkinVertex(*mesh, *armature, 0);
	XMVECTOR p1 = wiScene::SkinVertex(*mesh, *armature, 1);
	XMVECTOR p2 = wiScene::SkinVertex(*mesh, *armature, 2);

	CHECK(XMVectorGetX(p0) == doctest::Approx(0.0f));
	CHECK(XMVectorGetY(p0) == doctest::Approx(0.0f));

	CHECK(XMVectorGetX(p1) == doctest::Approx(5.0f));
	CHECK(XMVectorGetY(p1) == doctest::Approx(0.0f));

	CHECK(XMVectorGetX(p2) == doctest::Approx(2.5f));
	CHECK(XMVectorGetY(p2) == doctest::Approx(5.0f));

	// ------------------------------------------------------------------------
	// Phase 2: Fully Animated Keyframe Pose at time t = 40
	// (Arm local position translated from (5,0,0) to (15,0,0), so +10 in X)
	// ------------------------------------------------------------------------
	EvaluateAnimationAtTime(wiScene::GetScene(), *anim, 40.0f);
	UpdateArmatureSkinningMatrices(wiScene::GetScene(), *armature, (wiECS::Entity)pMesh->wickedarmatureindex);

	// Arm bone world position is now (15, 10, 0)
	wiECS::Entity armEntity = (wiECS::Entity)pObject->ppFrameList[2]->wickedobjindex;
	const wiScene::TransformComponent* armTrans = wiScene::GetScene().transforms.GetComponent(armEntity);
	CHECK(armTrans->world._41 == doctest::Approx(15.0f));
	CHECK(armTrans->world._42 == doctest::Approx(10.0f));

	// Bone 1 skinning matrix: invBind(-5, -10, 0) * world(15, 10, 0) = Translation(+10, 0, 0)
	CHECK(armature->boneMatrices[1].r[3].m128_f32[0] == doctest::Approx(10.0f));
	CHECK(armature->boneMatrices[1].r[3].m128_f32[1] == doctest::Approx(0.0f));

	// Vertex 0 (pelvis only): unshifted (0, 0, 0)
	XMVECTOR p0_t40 = wiScene::SkinVertex(*mesh, *armature, 0);
	CHECK(XMVectorGetX(p0_t40) == doctest::Approx(0.0f));

	// Vertex 1 (arm only, weight 1.0): shifted by +10 in X (5 -> 15)
	XMVECTOR p1_t40 = wiScene::SkinVertex(*mesh, *armature, 1);
	CHECK(XMVectorGetX(p1_t40) == doctest::Approx(15.0f));

	// Vertex 2 (dual influence: 0.4 pelvis + 0.6 arm):
	// X = 2.5 + 0.6 * 10 = 8.5
	XMVECTOR p2_t40 = wiScene::SkinVertex(*mesh, *armature, 2);
	CHECK(XMVectorGetX(p2_t40) == doctest::Approx(8.5f));

	// ------------------------------------------------------------------------
	// Phase 3: Midpoint Interpolation Evaluation at time t = 20
	// (Linear lerp between (5,0,0) and (15,0,0) results in (10,0,0), so +5 in X)
	// ------------------------------------------------------------------------
	EvaluateAnimationAtTime(wiScene::GetScene(), *anim, 20.0f);
	UpdateArmatureSkinningMatrices(wiScene::GetScene(), *armature, (wiECS::Entity)pMesh->wickedarmatureindex);

	CHECK(armTrans->world._41 == doctest::Approx(10.0f));
	CHECK(armature->boneMatrices[1].r[3].m128_f32[0] == doctest::Approx(5.0f));

	// Vertex 1 (arm only): shifted by +5 in X (5 -> 10)
	XMVECTOR p1_t20 = wiScene::SkinVertex(*mesh, *armature, 1);
	CHECK(XMVectorGetX(p1_t20) == doctest::Approx(10.0f));

	// Vertex 2: X = 2.5 + 0.6 * 5 = 5.5
	XMVECTOR p2_t20 = wiScene::SkinVertex(*mesh, *armature, 2);
	CHECK(XMVectorGetX(p2_t20) == doctest::Approx(5.5f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 6: Matrix Animation Key Decomposition and Channel Ingestion
// ============================================================================
TEST_CASE("Matrix Animation Key Decomposition and Channel Ingestion")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	// Replace animation track with matrix keys
	m.animations[0].tracks[0].positions.clear();
	m.animations[0].tracks[0].rotations.clear();
	m.animations[0].tracks[0].scales.clear();

	dbo::Matrix mat0 = MakeTranslation(5.0f, 0.0f, 0.0f);
	dbo::Matrix mat1 = MakeTranslation(25.0f, 0.0f, 0.0f);
	m.animations[0].tracks[0].matrices.push_back({0,  mat0, mat0});
	m.animations[0].tracks[0].matrices.push_back({40, mat1, mat1});

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	// WickedCall_AddObject unpacks matrix keys via XMMatrixDecompose into pos/rot/scale
	WickedCall_AddObject(pObject);

	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	REQUIRE(pAnimSet != nullptr);
	REQUIRE(pAnimSet->wickedanimentityindex != wiECS::INVALID_ENTITY);

	wiScene::AnimationComponent* anim = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pAnimSet->wickedanimentityindex);
	REQUIRE(anim != nullptr);

	// Translation channel must exist and have 2 keyframe values
	REQUIRE(anim->channels.size() >= 1);
	const auto& sampler = anim->samplers[anim->channels[0].samplerIndex];
	const auto& times = sampler.backwards_compatibility_data.keyframe_times;
	const auto& data = sampler.backwards_compatibility_data.keyframe_data;
	REQUIRE(times.size() == 2);
	CHECK(times[0] == doctest::Approx(0.0f));
	CHECK(times[1] == doctest::Approx(40.0f));

	const XMFLOAT3* positions = reinterpret_cast<const XMFLOAT3*>(data.data());
	CHECK(positions[0].x == doctest::Approx(5.0f));
	CHECK(positions[1].x == doctest::Approx(25.0f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 7: Binary DBO Format Decoding and Wicked Engine ECS Integration
// ============================================================================
TEST_CASE("Binary DBO Chunk Stream Decode and Wicked Engine ECS Ingestion")
{
	std::string error;
	Bytes binaryData = BuildBinarySkinnedDBOData();

	sObject* pObject = nullptr;
	REQUIRE(dbo::DecodeCompatibilityObject(binaryData.data(), binaryData.size(), &pObject, error));
	REQUIRE(pObject != nullptr);

	// Ingest decoded binary model into Wicked Engine
	WickedCall_AddObject(pObject);
	REQUIRE(pObject->wickedrootentityindex != wiECS::INVALID_ENTITY);

	// Verify mesh was created and populated from binary stream
	sMesh* pMesh = pObject->ppMeshList[0];
	REQUIRE(pMesh != nullptr);
	REQUIRE(pMesh->wickedmeshindex != wiECS::INVALID_ENTITY);

	wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pMesh->wickedmeshindex);
	REQUIRE(mesh != nullptr);
	CHECK(mesh->vertex_positions.size() == 3);
	CHECK(mesh->vertex_positions[1].x == doctest::Approx(4.0f));
	CHECK(mesh->vertex_normals.size() == 3);
	CHECK(mesh->vertex_uvset_0.size() == 3);
	CHECK(mesh->indices.size() == 3);

	// Verify material from binary stream
	REQUIRE(pMesh->wickedmaterialindex != wiECS::INVALID_ENTITY);
	wiScene::MaterialComponent* mat = wiScene::GetScene().materials.GetComponent((wiECS::Entity)pMesh->wickedmaterialindex);
	REQUIRE(mat != nullptr);
	CHECK(mat->userBlendMode == BLENDMODE_OPAQUE);

	// Verify frame hierarchy
	CHECK(pObject->iFrameCount >= 2);
	sFrame* pLegFrame = pObject->ppFrameList[1];
	REQUIRE(pLegFrame != nullptr);
	CHECK(std::string(pLegFrame->szName) == "bone_leg");

	// Verify animation from binary stream
	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	REQUIRE(pAnimSet != nullptr);
	CHECK(std::string(pAnimSet->szName) == "leg_kick");
	REQUIRE(pAnimSet->wickedanimentityindex != wiECS::INVALID_ENTITY);

	wiScene::AnimationComponent* anim = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pAnimSet->wickedanimentityindex);
	REQUIRE(anim != nullptr);
	CHECK(anim->channels.size() >= 1);

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 8: Memory Stability and Repeated Ingestion Lifecycle
// ============================================================================
TEST_CASE("DBO to Wicked Engine Repeated Ingestion and Deletion Memory Stability")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	// Warm-up cycle to ensure internal STL / CRT static buckets are initialized
	{
		sObject* pWarmup = nullptr;
		REQUIRE(dbo::CreateCompatibilityObject(m, &pWarmup, error));
		WickedCall_AddObject(pWarmup);
		WickedCall_RemoveObject(pWarmup);
		delete pWarmup;
	}

#ifdef _DEBUG
	_CrtMemState before{}, after{}, diff{};
	_CrtMemCheckpoint(&before);
#endif

	for (int cycle = 0; cycle < 32; ++cycle)
	{
		sObject* pObject = nullptr;
		REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));
		REQUIRE(pObject != nullptr);

		WickedCall_AddObject(pObject);
		REQUIRE(pObject->wickedrootentityindex != wiECS::INVALID_ENTITY);

		WickedCall_RemoveObject(pObject);
		delete pObject;
	}

#ifdef _DEBUG
	_CrtMemCheckpoint(&after);
	CHECK(_CrtMemDifference(&diff, &before, &after) == 0);
	CHECK(_CrtCheckMemory() != 0);
#endif

	// Verify scene components emptied cleanly
	CHECK(wiScene::GetScene().meshes.GetCount() == 0);
	CHECK(wiScene::GetScene().materials.GetCount() == 0);
	CHECK(wiScene::GetScene().armatures.GetCount() == 0);
	CHECK(wiScene::GetScene().animations.GetCount() == 0);
}

// ============================================================================
// TEST 9: Case-Insensitive Animation Track and Bone Binding Contract
// ============================================================================
TEST_CASE("Case-Insensitive Animation Track and Bone Binding Contract")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	// Change track names to uppercase/mixed case to test case-insensitive binding
	m.animations[0].tracks[0].name = "ARM"; // uppercase targeting lowercase "arm" frame

	// Add an unassigned animation track with empty name to ensure null/empty safety
	dbo::AnimationTrack emptyTrack;
	emptyTrack.name = "";
	m.animations[0].tracks.push_back(emptyTrack);

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));
	REQUIRE(pObject != nullptr);

	// Ingest into Wicked Engine
	WickedCall_AddObject(pObject);

	// The animation set must bind channel 0 to the "arm" frame entity
	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	REQUIRE(pAnimSet != nullptr);
	REQUIRE(pAnimSet->wickedanimentityindex != wiECS::INVALID_ENTITY);

	wiScene::AnimationComponent* anim = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pAnimSet->wickedanimentityindex);
	REQUIRE(anim != nullptr);
	REQUIRE(anim->channels.size() >= 1);

	wiECS::Entity armEntity = (wiECS::Entity)pObject->ppFrameList[2]->wickedobjindex;
	CHECK(anim->channels[0].target == armEntity);

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 10: Skinned Vertex Normals Transformation and Normalization
// ============================================================================
TEST_CASE("Skinned Vertex Normals Transformation and Normalization")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sMesh* pMesh = pObject->ppMeshList[0];
	wiScene::MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pMesh->wickedmeshindex);
	wiScene::ArmatureComponent* armature = wiScene::GetScene().armatures.GetComponent((wiECS::Entity)pMesh->wickedarmatureindex);
	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	wiScene::AnimationComponent* anim = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pAnimSet->wickedanimentityindex);
	REQUIRE(mesh != nullptr);
	REQUIRE(armature != nullptr);
	REQUIRE(anim != nullptr);

	// Bind pose evaluation at t = 0
	EvaluateAnimationAtTime(wiScene::GetScene(), *anim, 0.0f);
	UpdateArmatureSkinningMatrices(wiScene::GetScene(), *armature, (wiECS::Entity)pMesh->wickedarmatureindex);

	XMVECTOR norm0_bind = XMVectorZero();
	XMVECTOR norm1_bind = XMVectorZero();
	XMVECTOR norm2_bind = XMVectorZero();

	wiScene::SkinVertex(*mesh, *armature, 0, &norm0_bind);
	wiScene::SkinVertex(*mesh, *armature, 1, &norm1_bind);
	wiScene::SkinVertex(*mesh, *armature, 2, &norm2_bind);

	// Bind pose normals match input vertex normals
	CHECK(XMVectorGetX(norm0_bind) == doctest::Approx(0.0f));
	CHECK(XMVectorGetY(norm0_bind) == doctest::Approx(1.0f));
	CHECK(XMVectorGetZ(norm0_bind) == doctest::Approx(0.0f));
	CHECK(XMVectorGetX(XMVector3Length(norm0_bind)) == doctest::Approx(1.0f));

	CHECK(XMVectorGetX(norm2_bind) == doctest::Approx(0.0f));
	CHECK(XMVectorGetY(norm2_bind) == doctest::Approx(0.0f));
	CHECK(XMVectorGetZ(norm2_bind) == doctest::Approx(1.0f));
	CHECK(XMVectorGetX(XMVector3Length(norm2_bind)) == doctest::Approx(1.0f));

	// Animated pose at t = 40 (pure translation +10 in X)
	EvaluateAnimationAtTime(wiScene::GetScene(), *anim, 40.0f);
	UpdateArmatureSkinningMatrices(wiScene::GetScene(), *armature, (wiECS::Entity)pMesh->wickedarmatureindex);

	XMVECTOR norm1_t40 = XMVectorZero();
	wiScene::SkinVertex(*mesh, *armature, 1, &norm1_t40);

	// Translation preserves normal direction and unit length
	CHECK(XMVectorGetX(norm1_t40) == doctest::Approx(0.0f));
	CHECK(XMVectorGetY(norm1_t40) == doctest::Approx(1.0f));
	CHECK(XMVectorGetZ(norm1_t40) == doctest::Approx(0.0f));
	CHECK(XMVectorGetX(XMVector3Length(norm1_t40)) == doctest::Approx(1.0f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 11: 3D Multi-Tier Skeletal Hierarchy with Non-Planar 3D Rotation and Non-Uniform Scale
// ============================================================================
TEST_CASE("3D Multi-Tier Skeletal Hierarchy with Non-Planar 3D Rotation and Non-Uniform Scale")
{
	std::string error;
	dbo::Model m;

	// Frame hierarchy: root -> hip -> thigh -> shin
	dbo::Frame root;
	root.name = "root";
	root.matrix = MakeIdentity();
	root.mesh = 0;
	m.frames.push_back(root);

	dbo::Frame hip;
	hip.name = "hip";
	hip.parent = 0;
	hip.matrix = MakeTranslation(0.0f, 10.0f, 0.0f);
	m.frames.push_back(hip);

	dbo::Frame thigh;
	thigh.name = "thigh";
	thigh.parent = 1;
	thigh.matrix = MakeTranslation(0.0f, -4.0f, 0.0f);
	m.frames.push_back(thigh);

	dbo::Frame shin;
	shin.name = "shin";
	shin.parent = 2;
	shin.matrix = MakeTranslation(0.0f, -4.0f, 0.0f);
	m.frames.push_back(shin);

	// Mesh attached to shin: 3 vertices
	dbo::Mesh mesh;
	mesh.fvf = 0;
	mesh.stride = 64;
	mesh.vertexCount = 3;
	mesh.drawVertexCount = 3;
	mesh.drawPrimitiveCount = 1;
	mesh.primitiveType = GGPT_TRIANGLELIST;
	mesh.declaration = {
		{0, 0,  GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_POSITION, 0},
		{0, 12, GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_NORMAL, 0},
		{0, 24, GGDECLTYPE_FLOAT2, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 0},
		{0, 32, GGDECLTYPE_FLOAT4, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 3}, // weights
		{0, 48, GGDECLTYPE_FLOAT4, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 4}, // indices
		{255, 0, 0, 0, 0, 0}
	};

	struct V { float p[3]; float n[3]; float uv[2]; float w[4]; float idx[4]; };
	V verts[3] = {
		{ {0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0} },
		{ {1, 0, 0}, {0, 1, 0}, {1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0} },
		{ {0, 1, 0}, {0, 1, 0}, {0, 1}, {1, 0, 0, 0}, {0, 0, 0, 0} }
	};
	mesh.vertexData.resize(sizeof(verts));
	std::memcpy(mesh.vertexData.data(), verts, sizeof(verts));
	mesh.indices = {0, 1, 2};

	dbo::Texture tex;
	tex.name = "textures/leg.dds";
	mesh.textures.push_back(tex);

	// Shin world position in bind pose: (0, 10 - 4 - 4, 0) = (0, 2, 0)
	// So inverseBind is (0, -2, 0)
	dbo::Bone boneShin;
	boneShin.name = "shin";
	boneShin.inverseBind = MakeTranslation(0.0f, -2.0f, 0.0f);
	boneShin.vertices = {0, 1, 2};
	boneShin.weights = {1.0f, 1.0f, 1.0f};
	mesh.bones.push_back(boneShin);
	m.meshes.push_back(mesh);

	// Animation on hip: 90 degree yaw rotation around Y axis
	dbo::AnimationSet animSet;
	animSet.name = "spin_leg";
	animSet.length = 20;
	animSet.finish = 20;

	dbo::AnimationTrack hipTrack;
	hipTrack.name = "hip";
	// t=0: identity rotation
	hipTrack.rotations.push_back({0, {{0.0f, 0.0f, 0.0f, 1.0f}}});
	// t=20: 90 deg around Y: q = (0, sin(pi/4), 0, cos(pi/4))
	float s45 = std::sin(3.14159265f / 4.0f);
	float c45 = std::cos(3.14159265f / 4.0f);
	hipTrack.rotations.push_back({20, {{0.0f, s45, 0.0f, c45}}});

	animSet.tracks.push_back(hipTrack);
	m.animations.push_back(animSet);

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	wiScene::GetScene().UpdateSceneTransform(0.0f);

	// Verify initial hierarchy world positions
	wiECS::Entity shinEntity = (wiECS::Entity)pObject->ppFrameList[3]->wickedobjindex;
	const wiScene::TransformComponent* shinTrans = wiScene::GetScene().transforms.GetComponent(shinEntity);
	REQUIRE(shinTrans != nullptr);
	CHECK(shinTrans->world._41 == doctest::Approx(0.0f));
	CHECK(shinTrans->world._42 == doctest::Approx(2.0f));
	CHECK(shinTrans->world._43 == doctest::Approx(0.0f));

	// Evaluate rotation animation at t = 20
	wiScene::AnimationComponent* animComp = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pObject->pAnimationSet->wickedanimentityindex);
	REQUIRE(animComp != nullptr);
	EvaluateAnimationAtTime(wiScene::GetScene(), *animComp, 20.0f);

	// Under pure Y rotation, shin is on Y axis (0, 2, 0), so its position remains at (0, 2, 0)
	CHECK(shinTrans->world._42 == doctest::Approx(2.0f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 12: Indexless DBO Geometry Ingestion
// ============================================================================
TEST_CASE("Indexless DBO Geometry Ingestion and Automatic Triangle Generation")
{
	std::string error;
	dbo::Model m;

	dbo::Frame root;
	root.name = "root";
	root.matrix = MakeIdentity();
	root.mesh = 0;
	m.frames.push_back(root);

	dbo::Mesh mesh;
	mesh.fvf = GGFVF_XYZ | GGFVF_NORMAL | GGFVF_TEX1;
	mesh.stride = 32;
	mesh.vertexCount = 6;
	mesh.drawVertexCount = 6;
	mesh.drawPrimitiveCount = 2;
	mesh.primitiveType = GGPT_TRIANGLELIST;
	mesh.indices.clear(); // indexless!

	float verts[6][8] = {
		// Triangle 1
		{0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
		// Triangle 2
		{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f},
		{0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}
	};
	mesh.vertexData.resize(sizeof(verts));
	std::memcpy(mesh.vertexData.data(), verts, sizeof(verts));

	dbo::Texture tex;
	tex.name = "textures/quad.dds";
	mesh.textures.push_back(tex);

	m.meshes.push_back(mesh);

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sMesh* pDBOMesh = pObject->ppMeshList[0];
	REQUIRE(pDBOMesh != nullptr);

	wiScene::MeshComponent* wickedMesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pDBOMesh->wickedmeshindex);
	REQUIRE(wickedMesh != nullptr);

	// Wicked generates 6 indices from non-indexed triangle list with winding {i, i+2, i+1}
	REQUIRE(wickedMesh->indices.size() == 6);
	CHECK(wickedMesh->indices[0] == 0);
	CHECK(wickedMesh->indices[1] == 2);
	CHECK(wickedMesh->indices[2] == 1);
	CHECK(wickedMesh->indices[3] == 3);
	CHECK(wickedMesh->indices[4] == 5);
	CHECK(wickedMesh->indices[5] == 4);

	CHECK(wickedMesh->vertex_positions.size() == 6);
	CHECK(wickedMesh->subsets.size() >= 1);
	CHECK(wickedMesh->subsets[0].indexCount == 6);

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 13: Vertex Color and Tangent Vector Ingestion Contract
// ============================================================================
TEST_CASE("Vertex Color and Tangent Vector Ingestion Contract")
{
	std::string error;
	dbo::Model m;

	dbo::Frame root;
	root.name = "root";
	root.matrix = MakeIdentity();
	root.mesh = 0;
	m.frames.push_back(root);

	dbo::Mesh mesh;
	mesh.fvf = 0;
	mesh.stride = 48; // pos(12) + normal(12) + color(4) + tangent(12) + uv(8) = 48 bytes
	mesh.vertexCount = 3;
	mesh.drawVertexCount = 3;
	mesh.drawPrimitiveCount = 1;
	mesh.primitiveType = GGPT_TRIANGLELIST;
	mesh.declaration = {
		{0, 0,  GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_POSITION, 0},
		{0, 12, GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_NORMAL, 0},
		{0, 24, GGDECLTYPE_D3DCOLOR, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_COLOR, 0},
		{0, 28, GGDECLTYPE_FLOAT3, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 2},
		{0, 40, GGDECLTYPE_FLOAT2, GGDECLMETHOD_DEFAULT, GGDECLUSAGE_TEXCOORD, 0},
		{255, 0, 0, 0, 0, 0}
	};

	struct ColorTangentVertex {
		float pos[3];
		float norm[3];
		uint32_t color; // ARGB
		float tangent[3];
		float uv[2];
	};

	ColorTangentVertex verts[3] = {
		{ {0, 0, 0}, {0, 1, 0}, 0xFFFF0000, {1, 0, 0}, {0, 0} }, // Red: A=FF, R=FF, G=0, B=0
		{ {1, 0, 0}, {0, 1, 0}, 0xFF00FF00, {1, 0, 0}, {1, 0} }, // Green
		{ {0, 1, 0}, {0, 1, 0}, 0xFF0000FF, {1, 0, 0}, {0, 1} }  // Blue
	};
	mesh.vertexData.resize(sizeof(verts));
	std::memcpy(mesh.vertexData.data(), verts, sizeof(verts));
	mesh.indices = {0, 1, 2};

	dbo::Texture tex;
	tex.name = "textures/colored.dds";
	mesh.textures.push_back(tex);

	m.meshes.push_back(mesh);

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sMesh* pDBOMesh = pObject->ppMeshList[0];
	wiScene::MeshComponent* wickedMesh = wiScene::GetScene().meshes.GetComponent((wiECS::Entity)pDBOMesh->wickedmeshindex);
	REQUIRE(wickedMesh != nullptr);

	// Verify vertex colors ingested and converted to ABGR
	REQUIRE(wickedMesh->vertex_colors.size() == 3);
	// Red (0xFFFF0000 in ARGB -> 0xFF0000FF in ABGR: A=FF, B=00, G=00, R=FF)
	CHECK((wickedMesh->vertex_colors[0] & 0x000000FF) == 0xFF);

	// Verify material has vertex colors enabled
	wiScene::MaterialComponent* mat = wiScene::GetScene().materials.GetComponent((wiECS::Entity)pDBOMesh->wickedmaterialindex);
	REQUIRE(mat != nullptr);
	CHECK(mat->IsUsingVertexColors() == true);

	// Verify vertex tangents ingested
	REQUIRE(wickedMesh->vertex_tangents.size() == 3);
	CHECK(wickedMesh->vertex_tangents[0].x == doctest::Approx(1.0f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 14: Matrix Animation Key Decomposition with 3D Rotation and Non-Uniform Scale
// ============================================================================
TEST_CASE("Matrix Animation Key Decomposition with 3D Rotation and Non-Uniform Scale")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	m.animations[0].tracks[0].positions.clear();
	m.animations[0].tracks[0].rotations.clear();
	m.animations[0].tracks[0].scales.clear();

	// Key 0: Identity
	dbo::Matrix mat0 = MakeIdentity();

	// Key 30: Scale(2.0, 1.0, 1.0) * RotationY(90 deg) * Translation(10.0, 20.0, 30.0)
	XMMATRIX S = XMMatrixScaling(2.0f, 1.0f, 1.0f);
	XMMATRIX R = XMMatrixRotationY(XM_PIDIV2);
	XMMATRIX T = XMMatrixTranslation(10.0f, 20.0f, 30.0f);
	XMMATRIX M30 = S * R * T;
	dbo::Matrix mat30 = MatrixFromXMMatrix(M30);

	m.animations[0].tracks[0].matrices.push_back({0,  mat0, mat0});
	m.animations[0].tracks[0].matrices.push_back({30, mat30, mat30});

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sAnimationSet* pAnimSet = pObject->pAnimationSet;
	REQUIRE(pAnimSet != nullptr);
	wiScene::AnimationComponent* anim = wiScene::GetScene().animations.GetComponent((wiECS::Entity)pAnimSet->wickedanimentityindex);
	REQUIRE(anim != nullptr);

	// Since scale is non-uniform (2.0f) and rotation is non-trivial, all 3 channels must be created!
	CHECK(anim->channels.size() >= 3);
	CHECK(anim->samplers.size() >= 3);

	// Check channels exist
	bool hasTrans = false, hasRot = false, hasScale = false;
	for (const auto& ch : anim->channels)
	{
		if (ch.path == wiScene::AnimationComponent::AnimationChannel::Path::TRANSLATION) hasTrans = true;
		if (ch.path == wiScene::AnimationComponent::AnimationChannel::Path::ROTATION) hasRot = true;
		if (ch.path == wiScene::AnimationComponent::AnimationChannel::Path::SCALE) hasScale = true;
	}
	CHECK(hasTrans == true);
	CHECK(hasRot == true);
	CHECK(hasScale == true);

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 15: Defensive Hierarchy Handling for Circular References and Orphan Frames
// ============================================================================
TEST_CASE("Defensive Hierarchy Handling for Circular References and Orphan Frames")
{
	std::string error;

	// Case 1: Circular reference (Frame 0 parent 1, Frame 1 parent 0)
	{
		dbo::Model m;
		dbo::Frame f0;
		f0.name = "node0";
		f0.parent = 1;
		m.frames.push_back(f0);

		dbo::Frame f1;
		f1.name = "node1";
		f1.parent = 0;
		m.frames.push_back(f1);

		sObject* pObject = nullptr;
		bool result = dbo::CreateCompatibilityObject(m, &pObject, error);
		CHECK(result == false);
		CHECK(pObject == nullptr);
		CHECK(!error.empty());
	}

	// Case 2: Out of range parent index
	{
		dbo::Model m;
		dbo::Frame f0;
		f0.name = "orphan";
		f0.parent = 99; // out of range
		m.frames.push_back(f0);

		sObject* pObject = nullptr;
		bool result = dbo::CreateCompatibilityObject(m, &pObject, error);
		CHECK(result == false);
		CHECK(pObject == nullptr);
		CHECK(!error.empty());
	}

	// Case 3: Empty frames list
	{
		dbo::Model m;
		sObject* pObject = nullptr;
		bool result = dbo::CreateCompatibilityObject(m, &pObject, error);
		CHECK(result == false);
		CHECK(pObject == nullptr);
		CHECK(!error.empty());
	}
}

// ============================================================================
// TEST 16: Default Material Fallback When DBO Mesh Has No Textures
// ============================================================================
TEST_CASE("Default Material Fallback When DBO Mesh Has No Textures")
{
	std::string error;
	dbo::Model m;

	dbo::Frame root;
	root.name = "root";
	root.matrix = MakeIdentity();
	root.mesh = 0;
	m.frames.push_back(root);

	dbo::Mesh mesh;
	mesh.fvf = GGFVF_XYZ | GGFVF_NORMAL;
	mesh.stride = 24;
	mesh.vertexCount = 3;
	mesh.drawVertexCount = 3;
	mesh.drawPrimitiveCount = 1;
	mesh.primitiveType = GGPT_TRIANGLELIST;
	mesh.textures.clear(); // no textures!

	float verts[3][6] = {
		{0, 0, 0, 0, 1, 0},
		{1, 0, 0, 0, 1, 0},
		{0, 1, 0, 0, 1, 0}
	};
	mesh.vertexData.resize(sizeof(verts));
	std::memcpy(mesh.vertexData.data(), verts, sizeof(verts));
	mesh.indices = {0, 1, 2};
	m.meshes.push_back(mesh);

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));

	WickedCall_AddObject(pObject);

	sMesh* pDBOMesh = pObject->ppMeshList[0];
	REQUIRE(pDBOMesh != nullptr);
	REQUIRE(pDBOMesh->wickedmaterialindex != wiECS::INVALID_ENTITY);

	wiScene::MaterialComponent* mat = wiScene::GetScene().materials.GetComponent((wiECS::Entity)pDBOMesh->wickedmaterialindex);
	REQUIRE(mat != nullptr);
	CHECK(mat->baseColor.x == doctest::Approx(1.0f));
	CHECK(mat->baseColor.y == doctest::Approx(1.0f));
	CHECK(mat->baseColor.z == doctest::Approx(1.0f));
	CHECK(mat->baseColor.w == doctest::Approx(1.0f));

	WickedCall_RemoveObject(pObject);
	delete pObject;
}

// ============================================================================
// TEST 17: Multi-Animation Set Ingestion and Complete Deletion Lifecycle
// ============================================================================
TEST_CASE("Multi-Animation Set Ingestion and Complete Deletion Lifecycle")
{
	std::string error;
	dbo::Model m = BuildSkinnedDBOModel();

	// Add second animation set "idle"
	dbo::AnimationSet idleSet;
	idleSet.name = "idle";
	idleSet.length = 20;
	idleSet.finish = 20;

	dbo::AnimationTrack idleArmTrack;
	idleArmTrack.name = "arm";
	idleArmTrack.positions.push_back({0,  {{5.0f, 0.0f, 0.0f}}, {{0, 0, 0}}});
	idleArmTrack.positions.push_back({20, {{5.0f, 0.0f, 0.0f}}, {{0, 0, 0}}});
	idleSet.tracks.push_back(idleArmTrack);

	m.animations.push_back(idleSet);

	sObject* pObject = nullptr;
	REQUIRE(dbo::CreateCompatibilityObject(m, &pObject, error));
	REQUIRE(pObject != nullptr);

	// Ingest into Wicked Engine
	WickedCall_AddObject(pObject);

	// Remove object - verify all animation entities in scene are cleanly wiped
	WickedCall_RemoveObject(pObject);
	delete pObject;

	CHECK(wiScene::GetScene().animations.GetCount() == 0);
	CHECK(wiScene::GetScene().meshes.GetCount() == 0);
	CHECK(wiScene::GetScene().materials.GetCount() == 0);
	CHECK(wiScene::GetScene().armatures.GetCount() == 0);
}

