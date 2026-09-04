; AUTO-GENERATED symbol stubs for the Wicked contract tests.
; The contract suite compiles the whole wickedcalls.cpp translation layer,
; and the MSVC linker requires every reference in a direct .obj to resolve
; even for COMDATs that /OPT:REF later discards. Each product-wide symbol
; below gets raw storage; the tested COMDATs never execute any of it.
; Regenerate from the LNK2001 lines if the link reports new symbols.
.code
PUBLIC GG_CreateFile
PUBLIC GG_GetRealPath
PUBLIC GG_GetWritePath
PUBLIC GG_SetWritablesToRoot
PUBLIC ??0GGCOLOR@@QEAA@K@Z
PUBLIC ??0Matrix@KMaths@@QEAA@AEBU01@@Z
PUBLIC ??0Vector3@KMaths@@QEAA@MMM@Z
PUBLIC ??0rect_xywh@wiRectPacker@@QEAA@XZ
PUBLIC ??0wiArchive@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@_N@Z
PUBLIC ??1cStr@@QEAA@XZ
PUBLIC ??BGGCOLOR@@QEBAKXZ
PUBLIC ??DMatrix@KMaths@@QEBA?AU01@AEBU01@@Z
PUBLIC ??XMatrix@KMaths@@QEAAAEAU01@AEBU01@@Z
PUBLIC ?ApplyTransform@TransformComponent@wiScene@@QEAAXXZ
PUBLIC ?BT_GetGroundHeight@@YAMKMM@Z
PUBLIC ?Burst@wiEmittedParticle@wiScene@@QEAAXH@Z
PUBLIC ?CameraPositionX@@YAMH@Z
PUBLIC ?CameraPositionX@@YAMXZ
PUBLIC ?CameraPositionY@@YAMH@Z
PUBLIC ?CameraPositionY@@YAMXZ
PUBLIC ?CameraPositionZ@@YAMH@Z
PUBLIC ?CameraPositionZ@@YAMXZ
PUBLIC ?CheckForWorkshopFile@@YA_NPEAD@Z
PUBLIC ?ClearTransform@TransformComponent@wiScene@@QEAAXXZ
PUBLIC ?Close@wiArchive@@QEAAXXZ
PUBLIC ?Component_Attach@Scene@wiScene@@QEAAXII_N@Z
PUBLIC ?Component_Detach@Scene@wiScene@@QEAAXI@Z
PUBLIC ?Component_DetachChildren@Scene@wiScene@@QEAAXI@Z
PUBLIC ?CreatePerspective@CameraComponent@wiScene@@QEAAXMMMMM@Z
PUBLIC ?CreateRenderData@MeshComponent@wiScene@@QEAAXXZ
PUBLIC ?DrawBox@wiRenderer@@YAXAEBUXMFLOAT4X4@DirectX@@AEBUXMFLOAT4@3@@Z
PUBLIC ?DrawCapsule@wiRenderer@@YAXAEBUCAPSULE@@AEBUXMFLOAT4@DirectX@@@Z
PUBLIC ?DrawRubberBand@@YAXMMMM@Z
PUBLIC ?EnsureTextureStageValid@@YA_NPEAUsMesh@@H@Z
PUBLIC ?Entity_CreateLight@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBUXMFLOAT3@DirectX@@1MMW4LightType@LightComponent@2@@Z
PUBLIC ?Entity_CreateMaterial@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z
PUBLIC ?Entity_CreateMesh@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z
PUBLIC ?Entity_CreateObject@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z
PUBLIC ?Entity_FindByName@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z
PUBLIC ?Entity_Remove@Scene@wiScene@@QEAAXI@Z
PUBLIC ?FileExist@@YAHPEAD@Z
PUBLIC ?FileExistPrefDDS@@YAHPEAD@Z
PUBLIC ?FileRead@wiHelper@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAV?$vector@EV?$allocator@E@std@@@3@@Z
PUBLIC ?FindObjectFromWickedObjectEntityID@CObjectManager@@QEAAPEAUsObject@@_K@Z
PUBLIC ?FreeResource@wiResourceManager@@YAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z
PUBLIC ?GGTerrain_RayCast@GGTerrain@@YAHURAY@@PEAM11111PEAIH@Z
PUBLIC ?GenerateExtraDataForMeshEx@@YAXPEAUsMesh@@HHHHHK@Z
PUBLIC ?Get@cStr@@QEAAPEADXZ
PUBLIC ?GetDevice@wiRenderer@@YAPEAVGraphicsDevice@wiGraphics@@XZ
PUBLIC ?GetDirectoryFromPath@wiHelper@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@@Z
PUBLIC ?GetErrorCode@wiResourceManager@@YAHXZ
PUBLIC ?GetFVFOffsetMapFixedForBones@@YA_NPEAUsMesh@@PEAUsOffsetMap@@@Z
PUBLIC ?GetFileNameFromPath@wiHelper@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@@Z
PUBLIC ?GetLensFlareState@@YA_NXZ
PUBLIC ?GetLightShaftState@@YA_NXZ
PUBLIC ?GetObjectData@@YAPEAUsObject@@H@Z
PUBLIC ?GetObjectsInternalData@@YAPEAXH@Z
PUBLIC ?GetPickRay@wiRenderer@@YA?AURAY@@JJAEBUwiCanvas@@AEBUCameraComponent@wiScene@@@Z
PUBLIC ?GetPointer@wiInput@@YA?AUXMFLOAT4@DirectX@@XZ
PUBLIC ?GetPosition@TransformComponent@wiScene@@QEBA?AUXMFLOAT3@DirectX@@XZ
PUBLIC ?GetResource@wiResourceManager@@YA?AV?$shared_ptr@UwiResource@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@HPEAH@Z
PUBLIC ?GetRotation@TransformComponent@wiScene@@QEBA?AUXMFLOAT4@DirectX@@XZ
PUBLIC ?ImGuiHook_RenderCall_Direct@@YAXPEAX0@Z
PUBLIC ?ImageCreateSurfaceTexture@@YAXPEAD000@Z
PUBLIC ?ImageHeight@@YAHH@Z
PUBLIC ?ImageWidth@@YAHH@Z
PUBLIC ?IsOpen@wiArchive@@QEAA_NXZ
PUBLIC ?IsWickedMaterialActive@@YA_NPEAX@Z
PUBLIC ?Load@wiResourceManager@@YA?AV?$shared_ptr@UwiResource@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@IPEBE_K@Z
PUBLIC ?MatrixIdentity@KMaths@@YAPEAUMatrix@1@PEAU21@@Z
PUBLIC ?MatrixInverse@KMaths@@YAPEAUMatrix@1@PEAU21@PEAMPEBU21@@Z
PUBLIC ?MatrixRotationX@KMaths@@YAPEAUMatrix@1@PEAU21@M@Z
PUBLIC ?MatrixRotationY@KMaths@@YAPEAUMatrix@1@PEAU21@M@Z
PUBLIC ?MatrixRotationZ@KMaths@@YAPEAUMatrix@1@PEAU21@M@Z
PUBLIC ?MatrixTransform@TransformComponent@wiScene@@QEAAXAEBUXMMATRIX@DirectX@@@Z
PUBLIC ?Merge@AABB@@SA?AU1@AEBU1@0@Z
PUBLIC ?Merge@Scene@wiScene@@QEAAXAEAU12@@Z
PUBLIC ?ObjectExist@@YAHH@Z
PUBLIC ?ObjectIsEntity@@YA_NPEAX@Z
PUBLIC ?Pick@wiScene@@YA?AUPickResult@1@AEBURAY@@IIAEBUScene@1@@Z
PUBLIC ?PickThread@wiScene@@YA?AUPickResult@1@AEBURAY@@IIAEBUScene@1@@Z
PUBLIC ?Pick_OLD@wiScene@@YA?AUPickResult@1@AEBURAY@@IIAEBUScene@1@@Z
PUBLIC ?Restart@wiEmittedParticle@wiScene@@QEAAXXZ
PUBLIC ?Rotate@TransformComponent@wiScene@@QEAAXAEBT__m128@@@Z
PUBLIC ?Rotate@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT4@DirectX@@@Z
PUBLIC ?RotateRollPitchYaw@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT3@DirectX@@@Z
PUBLIC ?SMEMAvailable@@YAHH@Z
PUBLIC ?Scale@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT3@DirectX@@@Z
PUBLIC ?Serialize@AABB@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@AnimationComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@AnimationDataComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@ArmatureComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@CameraComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@DecalComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@EnvironmentProbeComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@ForceFieldComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@HierarchyComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@ImpostorComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@InverseKinematicsComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@LayerComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@LightComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@MaterialComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@MeshComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@NameComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@ObjectComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@PreviousFrameTransformComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@RigidBodyPhysicsComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@SoftBodyPhysicsComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@SoundComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@SpringComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@TransformComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@WeatherComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@wiEmittedParticle@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@wiHairParticle@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z
PUBLIC ?Serialize@wiResourceManager@@YAXAEAVwiArchive@@AEAUResourceSerializer@1@@Z
PUBLIC ?SetErrorCode@wiResourceManager@@YAXH@Z
PUBLIC ?SetLDSSkinningEnabled@wiRenderer@@YAX_N@Z
PUBLIC ?SetLensFlareState@@YAX_N@Z
PUBLIC ?SetLightShaftState@@YAX_N@Z
PUBLIC ?Shooter_Tools_Window@@3_NA
PUBLIC ?TransformCoord@KMaths@@YAPEAVVector3@1@PEAV21@PEBV21@PEBUMatrix@1@@Z
PUBLIC ?TransformNormal@KMaths@@YAPEAVVector3@1@PEAV21@PEBV21@PEBUMatrix@1@@Z
PUBLIC ?Translate@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT3@DirectX@@@Z
PUBLIC ?Update@Scene@wiScene@@QEAAXM@Z
PUBLIC ?UpdateSceneTransform@Scene@wiScene@@QEAAXM@Z
PUBLIC ?UpdateTransform@TransformComponent@wiScene@@QEAAXXZ
PUBLIC ?UpdateTransform_Parented@TransformComponent@wiScene@@QEAAXAEBU12@@Z
PUBLIC ?Wait@wiJobSystem@@YAXAEBUcontext@1@@Z
PUBLIC ?WickedCustomShaderID@@YAHXZ
PUBLIC ?WickedCustomShaderParam1@@YAMXZ
PUBLIC ?WickedCustomShaderParam2@@YAMXZ
PUBLIC ?WickedCustomShaderParam3@@YAMXZ
PUBLIC ?WickedCustomShaderParam4@@YAMXZ
PUBLIC ?WickedCustomShaderParam5@@YAMXZ
PUBLIC ?WickedCustomShaderParam6@@YAMXZ
PUBLIC ?WickedCustomShaderParam7@@YAMXZ
PUBLIC ?WickedDoubleSided@@YA_NXZ
PUBLIC ?WickedGetAlphaRef@@YAMXZ
PUBLIC ?WickedGetBaseColor@@YAKXZ
PUBLIC ?WickedGetBaseColorName@@YA?AVcStr@@XZ
PUBLIC ?WickedGetCastShadows@@YA_NXZ
PUBLIC ?WickedGetDisplacementName@@YA?AVcStr@@XZ
PUBLIC ?WickedGetEmissiveName@@YA?AVcStr@@XZ
PUBLIC ?WickedGetEmissiveStrength@@YAMXZ
PUBLIC ?WickedGetEmmisiveColor@@YAKXZ
PUBLIC ?WickedGetMetallnessStrength@@YAMXZ
PUBLIC ?WickedGetNormalName@@YA?AVcStr@@XZ
PUBLIC ?WickedGetNormalStrength@@YAMXZ
PUBLIC ?WickedGetReflectance@@YAMXZ
PUBLIC ?WickedGetRoughnessStrength@@YAMXZ
PUBLIC ?WickedGetSurfaceName@@YA?AVcStr@@XZ
PUBLIC ?WickedGetTransparent@@YA_NXZ
PUBLIC ?WickedGetTreeAlphaRef@@YAMXZ
PUBLIC ?WickedPlanerReflection@@YA_NXZ
PUBLIC ?WickedRenderOrderBias@@YAMXZ
PUBLIC ?WickedSetMeshNumber@@YAXH@Z
PUBLIC ?Wicked_Highlight_AllLogicObjects@@YAXXZ
PUBLIC ?Wicked_Highlight_ClearAllObjects@@YAXXZ
PUBLIC ?Wicked_Highlight_LockedList@@YAXXZ
PUBLIC ?Wicked_Highlight_Rubberband@@YAXXZ
PUBLIC ?Wicked_Highlight_Selection@@YAXXZ
PUBLIC ?Wicked_Update_Shadows@@YAXPEAX@Z
PUBLIC ?bImGuiGotFocus@@3_NA
PUBLIC ?bImGuiInTestGame@@3_NA
PUBLIC ?bProceduralLevel@@3_NA
PUBLIC ?bUseEditorOutlineSelection@@YA_NXZ
PUBLIC ?clear_highlighted_tree@@YAXXZ
PUBLIC ?g@@3USglobals@@A
PUBLIC ?g_StandaloneObjectHighlightList@@3V?$vector@HV?$allocator@H@std@@@std@@A
PUBLIC ?g_bLightProbeInstantChange@@3_NA
PUBLIC ?g_entityCameraLight@@3IA
PUBLIC ?g_entitySunLight@@3IA
PUBLIC ?g_entityThumbLight2@@3IA
PUBLIC ?g_entityThumbLight@@3IA
PUBLIC ?g_pGlob@@3PEAUGlobStruct@@EA
PUBLIC ?g_weatherEntityID@@3IA
PUBLIC ?getAsBoxMatrix@AABB@@QEBA?AUXMMATRIX@DirectX@@XZ
PUBLIC ?getBitsPerPixel@@YAHH@Z
PUBLIC ?getHalfWidth@AABB@@QEBA?AUXMFLOAT3@DirectX@@XZ
PUBLIC ?getImageformat@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@H@Z
PUBLIC ?get_terrain_sculpt_mode@@YAHXZ
PUBLIC ?ggterrain_extra_params@GGTerrain@@3UGGTerrainExtraParams@1@A
PUBLIC ?iGetgrideditselect@@YAHXZ
PUBLIC ?iLastTerrainSculptMode@@3HA
PUBLIC ?m_ObjectManager@@3VCObjectManager@@A
PUBLIC ?m_pD3D@@3PEAUID3D11Device@@EA
PUBLIC ?m_pImmediateContext@@3PEAUID3D11DeviceContext@@EA
PUBLIC ?master@@3VMaster@@A
PUBLIC ?pestrcasestr@@YAPEBDPEBD0@Z
PUBLIC ?set_terrain_edit_mode@@YAXH@Z
PUBLIC ?set_terrain_sculpt_mode@@YAXH@Z
PUBLIC ?t@@3UStemps@@A
PUBLIC ?timestampactivity@@YAXHPEAD@Z
_DATA SEGMENT
GG_CreateFile DB 16 DUP (?)
GG_GetRealPath DB 16 DUP (?)
GG_GetWritePath DB 16 DUP (?)
GG_SetWritablesToRoot DB 16 DUP (?)
??0GGCOLOR@@QEAA@K@Z DB 16 DUP (?)
??0Matrix@KMaths@@QEAA@AEBU01@@Z DB 16 DUP (?)
??0Vector3@KMaths@@QEAA@MMM@Z DB 16 DUP (?)
??0rect_xywh@wiRectPacker@@QEAA@XZ DB 16 DUP (?)
??0wiArchive@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@_N@Z DB 16 DUP (?)
??1cStr@@QEAA@XZ DB 16 DUP (?)
??BGGCOLOR@@QEBAKXZ DB 16 DUP (?)
??DMatrix@KMaths@@QEBA?AU01@AEBU01@@Z DB 16 DUP (?)
??XMatrix@KMaths@@QEAAAEAU01@AEBU01@@Z DB 16 DUP (?)
?ApplyTransform@TransformComponent@wiScene@@QEAAXXZ DB 16 DUP (?)
?BT_GetGroundHeight@@YAMKMM@Z DB 16 DUP (?)
?Burst@wiEmittedParticle@wiScene@@QEAAXH@Z DB 16 DUP (?)
?CameraPositionX@@YAMH@Z DB 16 DUP (?)
?CameraPositionX@@YAMXZ DB 16 DUP (?)
?CameraPositionY@@YAMH@Z DB 16 DUP (?)
?CameraPositionY@@YAMXZ DB 16 DUP (?)
?CameraPositionZ@@YAMH@Z DB 16 DUP (?)
?CameraPositionZ@@YAMXZ DB 16 DUP (?)
?CheckForWorkshopFile@@YA_NPEAD@Z DB 16 DUP (?)
?ClearTransform@TransformComponent@wiScene@@QEAAXXZ DB 16 DUP (?)
?Close@wiArchive@@QEAAXXZ DB 16 DUP (?)
?Component_Attach@Scene@wiScene@@QEAAXII_N@Z DB 16 DUP (?)
?Component_Detach@Scene@wiScene@@QEAAXI@Z DB 16 DUP (?)
?Component_DetachChildren@Scene@wiScene@@QEAAXI@Z DB 16 DUP (?)
?CreatePerspective@CameraComponent@wiScene@@QEAAXMMMMM@Z DB 16 DUP (?)
?CreateRenderData@MeshComponent@wiScene@@QEAAXXZ DB 16 DUP (?)
?DrawBox@wiRenderer@@YAXAEBUXMFLOAT4X4@DirectX@@AEBUXMFLOAT4@3@@Z DB 16 DUP (?)
?DrawCapsule@wiRenderer@@YAXAEBUCAPSULE@@AEBUXMFLOAT4@DirectX@@@Z DB 16 DUP (?)
?DrawRubberBand@@YAXMMMM@Z DB 16 DUP (?)
?EnsureTextureStageValid@@YA_NPEAUsMesh@@H@Z DB 16 DUP (?)
?Entity_CreateLight@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBUXMFLOAT3@DirectX@@1MMW4LightType@LightComponent@2@@Z DB 16 DUP (?)
?Entity_CreateMaterial@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z DB 16 DUP (?)
?Entity_CreateMesh@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z DB 16 DUP (?)
?Entity_CreateObject@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z DB 16 DUP (?)
?Entity_FindByName@Scene@wiScene@@QEAAIAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z DB 16 DUP (?)
?Entity_Remove@Scene@wiScene@@QEAAXI@Z DB 16 DUP (?)
?FileExist@@YAHPEAD@Z DB 16 DUP (?)
?FileExistPrefDDS@@YAHPEAD@Z DB 16 DUP (?)
?FileRead@wiHelper@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAV?$vector@EV?$allocator@E@std@@@3@@Z DB 16 DUP (?)
?FindObjectFromWickedObjectEntityID@CObjectManager@@QEAAPEAUsObject@@_K@Z DB 16 DUP (?)
?FreeResource@wiResourceManager@@YAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z DB 16 DUP (?)
?GGTerrain_RayCast@GGTerrain@@YAHURAY@@PEAM11111PEAIH@Z DB 16 DUP (?)
?GenerateExtraDataForMeshEx@@YAXPEAUsMesh@@HHHHHK@Z DB 16 DUP (?)
?Get@cStr@@QEAAPEADXZ DB 16 DUP (?)
?GetDevice@wiRenderer@@YAPEAVGraphicsDevice@wiGraphics@@XZ DB 16 DUP (?)
?GetDirectoryFromPath@wiHelper@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@@Z DB 16 DUP (?)
?GetErrorCode@wiResourceManager@@YAHXZ DB 16 DUP (?)
?GetFVFOffsetMapFixedForBones@@YA_NPEAUsMesh@@PEAUsOffsetMap@@@Z DB 16 DUP (?)
?GetFileNameFromPath@wiHelper@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@@Z DB 16 DUP (?)
?GetLensFlareState@@YA_NXZ DB 16 DUP (?)
?GetLightShaftState@@YA_NXZ DB 16 DUP (?)
?GetObjectData@@YAPEAUsObject@@H@Z DB 16 DUP (?)
?GetObjectsInternalData@@YAPEAXH@Z DB 16 DUP (?)
?GetPickRay@wiRenderer@@YA?AURAY@@JJAEBUwiCanvas@@AEBUCameraComponent@wiScene@@@Z DB 16 DUP (?)
?GetPointer@wiInput@@YA?AUXMFLOAT4@DirectX@@XZ DB 16 DUP (?)
?GetPosition@TransformComponent@wiScene@@QEBA?AUXMFLOAT3@DirectX@@XZ DB 16 DUP (?)
?GetResource@wiResourceManager@@YA?AV?$shared_ptr@UwiResource@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@HPEAH@Z DB 16 DUP (?)
?GetRotation@TransformComponent@wiScene@@QEBA?AUXMFLOAT4@DirectX@@XZ DB 16 DUP (?)
?ImGuiHook_RenderCall_Direct@@YAXPEAX0@Z DB 16 DUP (?)
?ImageCreateSurfaceTexture@@YAXPEAD000@Z DB 16 DUP (?)
?ImageHeight@@YAHH@Z DB 16 DUP (?)
?ImageWidth@@YAHH@Z DB 16 DUP (?)
?IsOpen@wiArchive@@QEAA_NXZ DB 16 DUP (?)
?IsWickedMaterialActive@@YA_NPEAX@Z DB 16 DUP (?)
?Load@wiResourceManager@@YA?AV?$shared_ptr@UwiResource@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@IPEBE_K@Z DB 16 DUP (?)
?MatrixIdentity@KMaths@@YAPEAUMatrix@1@PEAU21@@Z DB 16 DUP (?)
?MatrixInverse@KMaths@@YAPEAUMatrix@1@PEAU21@PEAMPEBU21@@Z DB 16 DUP (?)
?MatrixRotationX@KMaths@@YAPEAUMatrix@1@PEAU21@M@Z DB 16 DUP (?)
?MatrixRotationY@KMaths@@YAPEAUMatrix@1@PEAU21@M@Z DB 16 DUP (?)
?MatrixRotationZ@KMaths@@YAPEAUMatrix@1@PEAU21@M@Z DB 16 DUP (?)
?MatrixTransform@TransformComponent@wiScene@@QEAAXAEBUXMMATRIX@DirectX@@@Z DB 16 DUP (?)
?Merge@AABB@@SA?AU1@AEBU1@0@Z DB 16 DUP (?)
?Merge@Scene@wiScene@@QEAAXAEAU12@@Z DB 16 DUP (?)
?ObjectExist@@YAHH@Z DB 16 DUP (?)
?ObjectIsEntity@@YA_NPEAX@Z DB 16 DUP (?)
?Pick@wiScene@@YA?AUPickResult@1@AEBURAY@@IIAEBUScene@1@@Z DB 16 DUP (?)
?PickThread@wiScene@@YA?AUPickResult@1@AEBURAY@@IIAEBUScene@1@@Z DB 16 DUP (?)
?Pick_OLD@wiScene@@YA?AUPickResult@1@AEBURAY@@IIAEBUScene@1@@Z DB 16 DUP (?)
?Restart@wiEmittedParticle@wiScene@@QEAAXXZ DB 16 DUP (?)
?Rotate@TransformComponent@wiScene@@QEAAXAEBT__m128@@@Z DB 16 DUP (?)
?Rotate@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT4@DirectX@@@Z DB 16 DUP (?)
?RotateRollPitchYaw@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT3@DirectX@@@Z DB 16 DUP (?)
?SMEMAvailable@@YAHH@Z DB 16 DUP (?)
?Scale@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT3@DirectX@@@Z DB 16 DUP (?)
?Serialize@AABB@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@AnimationComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@AnimationDataComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@ArmatureComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@CameraComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@DecalComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@EnvironmentProbeComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@ForceFieldComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@HierarchyComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@ImpostorComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@InverseKinematicsComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@LayerComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@LightComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@MaterialComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@MeshComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@NameComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@ObjectComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@PreviousFrameTransformComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@RigidBodyPhysicsComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@SoftBodyPhysicsComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@SoundComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@SpringComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@TransformComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@WeatherComponent@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@wiEmittedParticle@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@wiHairParticle@wiScene@@QEAAXAEAVwiArchive@@AEAUEntitySerializer@wiECS@@@Z DB 16 DUP (?)
?Serialize@wiResourceManager@@YAXAEAVwiArchive@@AEAUResourceSerializer@1@@Z DB 16 DUP (?)
?SetErrorCode@wiResourceManager@@YAXH@Z DB 16 DUP (?)
?SetLDSSkinningEnabled@wiRenderer@@YAX_N@Z DB 16 DUP (?)
?SetLensFlareState@@YAX_N@Z DB 16 DUP (?)
?SetLightShaftState@@YAX_N@Z DB 16 DUP (?)
?Shooter_Tools_Window@@3_NA DB 16 DUP (?)
?TransformCoord@KMaths@@YAPEAVVector3@1@PEAV21@PEBV21@PEBUMatrix@1@@Z DB 16 DUP (?)
?TransformNormal@KMaths@@YAPEAVVector3@1@PEAV21@PEBV21@PEBUMatrix@1@@Z DB 16 DUP (?)
?Translate@TransformComponent@wiScene@@QEAAXAEBUXMFLOAT3@DirectX@@@Z DB 16 DUP (?)
?Update@Scene@wiScene@@QEAAXM@Z DB 16 DUP (?)
?UpdateSceneTransform@Scene@wiScene@@QEAAXM@Z DB 16 DUP (?)
?UpdateTransform@TransformComponent@wiScene@@QEAAXXZ DB 16 DUP (?)
?UpdateTransform_Parented@TransformComponent@wiScene@@QEAAXAEBU12@@Z DB 16 DUP (?)
?Wait@wiJobSystem@@YAXAEBUcontext@1@@Z DB 16 DUP (?)
?WickedCustomShaderID@@YAHXZ DB 16 DUP (?)
?WickedCustomShaderParam1@@YAMXZ DB 16 DUP (?)
?WickedCustomShaderParam2@@YAMXZ DB 16 DUP (?)
?WickedCustomShaderParam3@@YAMXZ DB 16 DUP (?)
?WickedCustomShaderParam4@@YAMXZ DB 16 DUP (?)
?WickedCustomShaderParam5@@YAMXZ DB 16 DUP (?)
?WickedCustomShaderParam6@@YAMXZ DB 16 DUP (?)
?WickedCustomShaderParam7@@YAMXZ DB 16 DUP (?)
?WickedDoubleSided@@YA_NXZ DB 16 DUP (?)
?WickedGetAlphaRef@@YAMXZ DB 16 DUP (?)
?WickedGetBaseColor@@YAKXZ DB 16 DUP (?)
?WickedGetBaseColorName@@YA?AVcStr@@XZ DB 16 DUP (?)
?WickedGetCastShadows@@YA_NXZ DB 16 DUP (?)
?WickedGetDisplacementName@@YA?AVcStr@@XZ DB 16 DUP (?)
?WickedGetEmissiveName@@YA?AVcStr@@XZ DB 16 DUP (?)
?WickedGetEmissiveStrength@@YAMXZ DB 16 DUP (?)
?WickedGetEmmisiveColor@@YAKXZ DB 16 DUP (?)
?WickedGetMetallnessStrength@@YAMXZ DB 16 DUP (?)
?WickedGetNormalName@@YA?AVcStr@@XZ DB 16 DUP (?)
?WickedGetNormalStrength@@YAMXZ DB 16 DUP (?)
?WickedGetReflectance@@YAMXZ DB 16 DUP (?)
?WickedGetRoughnessStrength@@YAMXZ DB 16 DUP (?)
?WickedGetSurfaceName@@YA?AVcStr@@XZ DB 16 DUP (?)
?WickedGetTransparent@@YA_NXZ DB 16 DUP (?)
?WickedGetTreeAlphaRef@@YAMXZ DB 16 DUP (?)
?WickedPlanerReflection@@YA_NXZ DB 16 DUP (?)
?WickedRenderOrderBias@@YAMXZ DB 16 DUP (?)
?WickedSetMeshNumber@@YAXH@Z DB 16 DUP (?)
?Wicked_Highlight_AllLogicObjects@@YAXXZ DB 16 DUP (?)
?Wicked_Highlight_ClearAllObjects@@YAXXZ DB 16 DUP (?)
?Wicked_Highlight_LockedList@@YAXXZ DB 16 DUP (?)
?Wicked_Highlight_Rubberband@@YAXXZ DB 16 DUP (?)
?Wicked_Highlight_Selection@@YAXXZ DB 16 DUP (?)
?Wicked_Update_Shadows@@YAXPEAX@Z DB 16 DUP (?)
?bImGuiGotFocus@@3_NA DB 16 DUP (?)
?bImGuiInTestGame@@3_NA DB 16 DUP (?)
?bProceduralLevel@@3_NA DB 16 DUP (?)
?bUseEditorOutlineSelection@@YA_NXZ DB 16 DUP (?)
?clear_highlighted_tree@@YAXXZ DB 16 DUP (?)
?g@@3USglobals@@A DB 16 DUP (?)
?g_StandaloneObjectHighlightList@@3V?$vector@HV?$allocator@H@std@@@std@@A DB 16 DUP (?)
?g_bLightProbeInstantChange@@3_NA DB 16 DUP (?)
?g_entityCameraLight@@3IA DB 16 DUP (?)
?g_entitySunLight@@3IA DB 16 DUP (?)
?g_entityThumbLight2@@3IA DB 16 DUP (?)
?g_entityThumbLight@@3IA DB 16 DUP (?)
?g_pGlob@@3PEAUGlobStruct@@EA DB 16 DUP (?)
?g_weatherEntityID@@3IA DB 16 DUP (?)
?getAsBoxMatrix@AABB@@QEBA?AUXMMATRIX@DirectX@@XZ DB 16 DUP (?)
?getBitsPerPixel@@YAHH@Z DB 16 DUP (?)
?getHalfWidth@AABB@@QEBA?AUXMFLOAT3@DirectX@@XZ DB 16 DUP (?)
?getImageformat@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@H@Z DB 16 DUP (?)
?get_terrain_sculpt_mode@@YAHXZ DB 16 DUP (?)
?ggterrain_extra_params@GGTerrain@@3UGGTerrainExtraParams@1@A DB 16 DUP (?)
?iGetgrideditselect@@YAHXZ DB 16 DUP (?)
?iLastTerrainSculptMode@@3HA DB 16 DUP (?)
?m_ObjectManager@@3VCObjectManager@@A DB 16 DUP (?)
?m_pD3D@@3PEAUID3D11Device@@EA DB 16 DUP (?)
?m_pImmediateContext@@3PEAUID3D11DeviceContext@@EA DB 16 DUP (?)
?master@@3VMaster@@A DB 16 DUP (?)
?pestrcasestr@@YAPEBDPEBD0@Z DB 16 DUP (?)
?set_terrain_edit_mode@@YAXH@Z DB 16 DUP (?)
?set_terrain_sculpt_mode@@YAXH@Z DB 16 DUP (?)
?t@@3UStemps@@A DB 16 DUP (?)
?timestampactivity@@YAXHPEAD@Z DB 16 DUP (?)
_DATA ENDS
END
