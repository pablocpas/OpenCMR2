#include "CockpitExtension.h"
// Declared before RallyData.h, as in the original: in an expression that calls
// both, MSVC calls RallyDataState first.
short Car_GetOrderCount(void);
#include "SoundCurve.h"
#include "TrailHistory.h"
#include "FloatToInt.h"
#include "port/disk.h"
#include "port/disk_layouts.h"
#include "port/replay_disk.h"
#include "port/sys.h"
#include "ReplayStream.h"
#include "AINetwork.h"
#include "FinRecord.h"
#include "StageObjectCount.h"
#include <stdlib.h>
#include "StageBlock.h"
#include <string.h>
#include <stddef.h>
#include "RallyData.h"
#include "GraphicsOptions.h"
#include "SceneNode.h"
#include "Frontend.h"
#include "InstallInfo.h"
#include "Texture.h"
#include "StageTiming.h"
#include "StageWeather.h"
#include "AIHelper.h"
#include "RegKey.h"
#include <stdio.h>
#include <math.h>
#include "FixedPoint.h"
#include "MovingObject.h"
#include "MeshCollisionRecord.h"
#include "Car.h"
#include "CarResources.h"
#include "CarPhysics.h"
#include "WheelTrail.h"
#include "GameInfo.h"
#include "Input.h"
#include "main.h"
#include "Game.h"
#include "GenericFileLoader.h"
#include "FileBuffer.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "CarParts.h"
#include "CarLightState.h"
#include "LightField.h"
#include "FogBeamProjection.h"
#include "SnowGlint.h"
#include "SnowMaterial.h"
#include "Graphics.h"
#include "Font.h"
#include "Sprite.h"
#include "PrecipitationQuad.h"
#include "WeatherSidecar.h"
#include "Collision2D.h"
#include "PauseMenus.h"
#include "Menu.h"
#include "Glow.h"
#include "CarExhaust.h"
#include "LayoutChecks.h"
// --- module prototypes (address order; see tools/fastcmp/tuproto.py) ---
void StageObject_DrawViewPrecipitationAndObjects(int *pViewRect, int view);
void StageWeather_DrawViewPrecipitation(int index, int view);
void StageObject_DeriveLoadedScaleVector(FixVector *pOut);
void StageObject_AdvanceAnimatedRecordState(ViewWeatherState *p, int unused);
BYTE StageObject_GetViewWeatherStateByte(int index);
int StageObject_GetViewWeatherField54(int index);
void StageObject_SetScaledValue(int value, int index);
int StageObject_GetCarWeatherRampValue(Car *pCar);
int CarDamage_EmitBodySparkBillboards(int amount, Car *pCar);
void StageObject_SetLighting(const StageLightPreset *pPrimary, const StageLightPreset *pSecondary);
int StageObject_BlendClampedByteValues(BYTE a, BYTE b, int t);
void StageLights_InterpolatePreset(BYTE *out, BYTE *from, BYTE *to, int t);
BYTE *StageLights_BlendTimeOfDayPreset(unsigned short timeOfDay, int slot, BYTE **records);
void StageLights_BlendPresetPair(unsigned short timePrimary, unsigned short timeSecondary, BYTE **records, StageLightPreset **pPrimary, StageLightPreset **pSecondary);
void StageLights_SetPairFogDistances(StageLightPreset *pA, StageLightPreset *pB);
void StageLights_SetWeatherFogDistance(StageLightPreset *pPreset, int type);
void StageObject_InterpolateSecondaryRampRecords(int t);
void StageObject_RebuildViewWeatherLighting(int index);
void StageObject_AverageWheelGroundLighting(unsigned int param_1, int param_2);
void StageObject_UpdateSceneAmbientColour(BYTE *pColour);
void StageObject_UpdateSunVisibility(short *pRect);
void StageObject_DrawProjectedViewIcon(int *pViewRect, int param_2);
void StageLights_UpdateDirection(void);
int *StageObject_GetViewWeatherSlot(int i, int j);
void StageLights_SetTransform(FixVector *pAxes);
void StageLights_LoadTextures(void);
void StageLights_Create(void);
void StageLights_Update(void);
void StageObject_SetBoundedWeatherKind(int value);
void StageLights_Off(void);
void CarLights_LoadTextures(void);
void CarLight_BuildPointGlows(Car *pCar);
void CarLight_UpdateFlagsAndGlows(Car *pCar);
int StageObject_GetSceneLightBrightness(void);
void StageObject_PositionRearViewLightNodes(unsigned int param_1);
BYTE *StageObject_GetViewRectangleIndex(int index);
BYTE *StageObject_GetPlayerViewRectangle(int view);
void StageObject_InitScreenRectangles(void);
void StageObject_AdvanceFlaggedCarSlotCounters(int car);
void StageObject_UpdateSkidTrails(int carIndex);
void CarSkid_ClearWheelTrails(void);
void StageObject_LoadSkidTrailTextures(void);
void StageObject_UpdateRaceSkidTrails(int param_1);
void StageObject_RebuildCarExhaustPoints(int car);
void CarEffects_DrawTyreMarks(int index);
int StageObject_GetWheelSlip(int carIndex, int wheelIndex);
int StageObject_GetNormalizedWheelSlip(int car, int wheel);
int StageObject_ClampByteRange(int value);
void Replay_ReplaceActiveBuffer(int frames, int samples);
void Replay_ResetSelectionIndices(void);
void Replay_LoadPathOrDefaultBuffer(char *path);
void CarEffects_MakeGhost(Car *pCar);
void Replay_RestartGhostCar(int a, int b);
void Replay_ResetActiveBufferState(void);
int Replay_GetActiveBufferState(void);
void Replay_SwapPendingSlotValue(int **pValue, int slot, int flag);
void Replay_SetControlStateByte(BYTE value);
int Replay_GetSelectionStateByte(void);
void StageObject_UpdateProjectedDistanceFade(int param_1);
void StageObjects_Init(void);
void StageObject_UpdateDeviceEffectSupportFlag(void);
void StageObject_UpdateListedCarPhysicsAndWeather(short *pOrder, short count);
void StageObjects_Update(void);
void CarDamage_UpdateOrderedCarsOffRoadState(short *param_1, short param_2, int *pViewRect, int param_4);
void StageObject_SetFrameRate(int value);
void CarDamage_BuildRelativeVelocityHull(Car *pCar, CarPartSet *pParts);
void CarDamage_DecodeImpactPayload(CarImpactPayload *p);
void StageObject_ApplyWeightedContactDamage(Car *pCar, int amount);
void StageObject_RebuildDamagePartValues(Car *pCar);
int CarDamage_AverageVertexDisplacement(Car *pCar, CarPartSet *set);
void StageObject_SetModelSubmeshVisibility(Car *pCar, int param_2, char param_3);
void StageObject_ResetCarObjectState(Car *pCar);
void CarPart_MarkHidden(Car *pCar, int slot);
int CarDamage_PickRandomTriangleEdgePoint(FixVector *pOut, CarPartSet *pParts, int index);
int StageObject_IsEligibleType(short type, int mode, int category);
void StageObject_UpdateEnabledCornerContactFrames(Car *param_1, short *param_2, short param_3);
void CarDamage_UpdateSuspensionImpactContacts(Car *pCar);
void CarDamage_RebuildPartBounds(int param_1, SceneNode *param_2, CarPartSet *param_3);
void CarDamage_BuildPartVertexBuffer(int param_1, SceneNode *param_2, CarPartSet *set);
void CarDamage_SetModelEnabled(int enabled, int carIndex);
void CarDamage_ClearModelEnabled(void);
void Mesh_ReadVertexFixed(Mesh **ppMeshes, int mesh, int vertex, FixVector *pOut);
int CarDamage_GetModelEnabled(Car *pCar);
void CarEffects_UpdateBrokenLightFlicker(Car *pCar);
void CarPart_ApplyHiddenStates(Car *pCar);
void StageObject_ClearNodeValueBelowThreshold(SceneNode *pNode, BYTE threshold);
void StageObject_ClearNodeTreeValuesBelowThreshold(SceneNode *pNode, BYTE threshold);
void StageObject_ResetPairedCarValues(void);
void StageObject_SetPairedCarValue(int i, int value, int j);
void StageObject_SetCarValuePendingFlag(int index, int reset);
void StageObject_SetCarVisibilityBits(int type, int car, int index);
void StageObject_ResetCarNodeViewFlags(Car *pCar);
void StageObject_RebuildOrderedWheelVisibility(void);
int StageObject_GetPairedCarValue(int i, int j);
BYTE StageObject_GetCarRecordState(int index);
int StageObject_UsesExtendedMode(void);
void Replay_EncodeControlPacket(BYTE *pIn, BYTE *pOut, int active, int handbrake, int lightA, int lightB);
int Replay_DecodeInputPacket(ReplayStream *pState, BYTE *pIn, BYTE *pOut, BYTE *pCounter, Car *pCar);
void Replay_CaptureCarSnapshot(ReplayInputLane *pDst, BYTE car);
// Oriented box of a stage object (same layout as in Collision2D.cpp).

// One firework rocket of the pool at g_unk0x00590af8 (0x938 bytes). Each
// simulated array has a previous-frame copy and an interpolated copy that is
// what gets drawn.
struct FireworkRocket {
    FixVector pos;               // 0x000
    FixVector vel;               // 0x00c
    FixVector trail[20];         // 0x018 spark trail behind the rising rocket
    FixVector sparks[3][6];      // 0x108 burst sparks (columns 3..5 mirror 0..2)
    FixVector prevPos;           // 0x1e0 last frame's copies, for interpolation
    FixVector drawPos;           // 0x1ec interpolated copies that are drawn
    FixVector prevTrail[20];     // 0x1f8
    FixVector drawTrail[20];     // 0x2e8
    FixVector prevSparks[3][6];  // 0x3d8
    FixVector drawSparks[3][6];  // 0x4b0
    FixVector sparkVel[3][6];    // 0x588
    int sparkSpeed;              // 0x660
    int fuse;                    // 0x664
    int burstTime;               // 0x668
    int rise;                    // 0x66c gravity on the rocket
    int sparkGravity;            // 0x670
    int blinkTime;               // 0x674 burst time when the blink started
    BYTE rocketColour[4];        // 0x678
    BYTE trailColour[4];         // 0x67c
    BYTE colourA[2][4];          // 0x680 blend from/to for the first spark set
    BYTE colourB[2][4];          // 0x688 and for the second one
    char trailLen;               // 0x690
    char trailHead;              // 0x691
    BYTE colour;                 // 0x692
    char sound;                  // 0x693
    int state;                   // 0x694 1 rising, 2 burst
    int type;                    // 0x698
    int trailFlag[20];           // 0x69c
    int sparkOn[3][6][4];        // 0x6ec per spark and mirrored quadrant
    int sparkAlt[3][6][4];       // 0x80c second colour
    int burst;                   // 0x92c sparks (else debris)
    int blinking;                // 0x930
    int blink;                   // 0x934
};
CMR2_LAYOUT_CHECK(FireworkRocketSize, sizeof(FireworkRocket) == 0x938);

void Replay_InitObjectListCarState(ReplayInputLane *pLane, BYTE param_2);
void Replay_SaveCarTorqueAndRecordState(int *pState, BYTE car);
void Replay_RestoreCarTorqueAndRecordState(int *pState, BYTE car);
void Replay_InitMeshListCarState(ReplayStateLane *pLane, BYTE param_2);
void Replay_EncodeCarControls(BYTE *pOut, BYTE car);
int Replay_DecodeCarControls(ReplayStream *pState, BYTE *pIn, BYTE car, BYTE *pCounter);
void Replay_InitSlots(void);
BYTE *Replay_AllocateStreamBuffer(short frames, short samples, int type);
int Replay_FreeBuffers(void);
int Replay_InitCarStreamState(ReplayStream *p, int unused, BYTE car);
void Replay_AdvanceLiveRecordingStreams(void);
int Replay_CanExtendControlPacketRun(BYTE *packet, BYTE car);
int Replay_StopRecording(BYTE *pBuffer);
int Replay_SelectAndInitializeLane(ReplayStream *p, short lane, short start, BYTE car);
void Replay_PlayStreamFrame(ReplayStream *pState);
void Replay_PlaybackAllStreamFrames(void);
int Replay_ResetBufferIfActive(ReplayStream *p);
BYTE *Replay_LoadValidatedBuffer(char *path);
int Replay_Save(BYTE *pBuffer, char *pName);
void Replay_SetupPointers(ReplayStream *p, int unused);
BYTE *Replay_GetStreamFrameState(void);
void Replay_RecordPeriodicCarSamples(void);
void Replay_RecordAllStreamFrames(void);

void Replay_InterpolatePoseStream(ReplayStream *p);
void Replay_EncodeCarPoseSample(Car *pCar, ReplaySample *pSample);
void Replay_DecodeCarPoseSample(unsigned int *pFlag24, unsigned int *pFlag27, unsigned int *pSteering, int *pLevel,
                  FixMatrix *pMatrix, int *pBits21, unsigned int *pFlag26, ReplaySample *pSample);
void Replay_ComputePoseVelocityCorrection(ReplayPose *pPose, Car *pCar);
void Replay_HideFinishedGhostCarNodes(void);
void Events_Reset(void);
void Events_Init(char *unused, int slot, char animate);
void Events_ComputeSteps(void);
void StageObject_StampPendingEventDraws(int unused);
void StageObject_SpawnCarWheelSkidEffects(int param_1);
void CarSkid_EmitSlippingWheelEffects(int param_1, int param_2);
void StageObject_QueueTimedEventDraw(int index, int x, int y);
BYTE Events_Tick(int index);
void CarSkid_SpawnWheelDustPuff(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6, int param_7, int param_8);
BYTE StageObject_IsWheelOnActiveEffectSurface(int index, int wheel);
void Events_Flush(void);
void StageObject_BuildCloudTexturePath(void);
BYTE StageObject_ReleaseLoadedObjectFile(void);
void StageObjects_LoadSkyAndGround(void);
void StageObject_PositionSplitViewNodes(int param_1);
void StageObject_GetCurrentObjectPosition(FixVector *pOut);
void StageObject_GetSkyNode(SceneNode **pOut);
void StageObject_GetGroundNode(SceneNode **pOut);
void StageObject_GetCloudNodes(SceneNode **pOut1, SceneNode **pOut2);
int StageObjects_ReleaseObjectFiles(void);
void StageObject_LoadAndClassifyMeshes(void);
void StageObject_ResetMovingObjectCountsAndPhases(void);
struct MovingObject;
void StageObject_InitMovingObject(MovingObject *pState, int carIndex);
void StageObject_QueueOrEvictMovingObject(SectorObjectEntry *param_1, int param_2, int param_3);
void StageObject_InterpolateAllObjectMatrices(int t);
void StageObject_ApplyInterpolatedNodeMatrices(int param_1);
void StageObject_LoadModelVariantsAndSaveCounts(void);
unsigned int StageObject_GetLoadedVariantDataAndState(SectorObjectEntry **pOut);
void StageObject_LoadTempObjectModel(void);
void StageObject_LoadTempShadowModel(void);
void Knockout_SetRoundActiveFlag(void);
void Knockout_ClearRoundActiveFlag(void);
int Knockout_GetRoundActiveFlag(void);
int Knockout_HasPendingOrUnknownRoundMatch(void);
int Knockout_HasHumanLostMatch(KnockoutMatch *pMatch);
BYTE Knockout_ClearChampionshipPendingFlag(void);
void Knockout_RecordCurrentMatchResults(void);
void Knockout_ResetRaceStateMachine(void);
void Knockout_ApplyCarGroundMaterials(void);
void Knockout_UpdateRaceStateAndFades(CallbackStateMachine *param_1, unsigned int param_2);
int Knockout_HasUndecidedRoundMatch(void);
int Knockout_AreCurrentDriversKnown(void);
unsigned int Knockout_GetCurrentDriverField(unsigned int *pHigh);
int Knockout_HasUnknownMatchDriver(unsigned int *p);
char *Knockout_GetDriverNameForSide(KnockoutMatch *pMatch, int side);
int Knockout_IsHumanMatchSide(KnockoutMatch *pMatch, int side);
int Knockout_SelectDisplaySide(KnockoutMatch *pMatch, int param2);
char *Knockout_GetCarNameForSide(KnockoutMatch *pMatch, int side);
void StageObject_DrawPauseStageDataPanel(Menu *pMenu);
void StageObject_DrawInRacePauseMenu(Menu *pMenu);
void StageObject_DrawTypingTextFraction(char *text, int fraction, unsigned int font, int x, unsigned int y, int *pColour, unsigned int flags);
void Knockout_DrawCurrentRoundBracket(void);
void Knockout_DrawAnimatedMatchHeader(int param1, int param2, int param3, int param4, int param5, KnockoutMatch *param6);
char *StageObject_FormatAnimatedLapTime(int *p, int index, int mode);
void StageObject_DrawAnimatedTextPair(int param1, KnockoutMatch *param2, short *param3, BYTE *param4, BYTE *param5, int param6, int param7, int *param8, int param9);
void StageObject_DrawOutlinedStageBox(short *pRect, BYTE *pColour, BYTE *pEdgeColour, int drawTexture);
int StageObject_FillWidthScaledRectangle(int scale, BYTE *unused, short *pRect, BYTE *pColour, int layer);
void StageObject_ClearAndDrawSplitPositions(int unused1, int unused2);
void StageObject_BuildInRaceActionMenu(void);
Menu *StageObject_GetInRaceActionMenu(void);
void StageObject_LoadAndAttachCarInterior(BYTE record, BYTE car);
void StageObject_SyncStateAndSceneMatrix(CameraRecord *p, int *src, int unused, BYTE value);
void CarInterior_ResetDriverPoseOnNextUpdate(CameraRecord *pRecord);
int CarInterior_IsLoaded(BYTE index);
void CarInterior_CacheProfilePointers(int index);
void StageObject_DispatchActiveCarObjectUpdate(CameraRecord *pObj, FixMatrix *pRef, int b);
void StageObject_BlendCarMountTransform(int car);
int CarInterior_UpdateSteeringBlendFraction(int car, Car *pCar);
void CarInterior_AnimateWipers(int index);
void CarInterior_UpdateWiperSweep(int index);
void CarInterior_IntegrateDriverPose(CameraRecord *param_1, FixMatrix *param_2, int unused);
void StageObject_FindPlayerRevTextures(int player);
void StageObject_UpdateRevCounterTextures(int index);
void CarInterior_UpdateGearDigitTexture(Texture *pTexture, int state, int cacheBase, int index);
void StageObject_BuildCarNodeOrientation(CameraRecord *object, int *src);
void StageObject_BuildCarMountWorldMatrix(CameraRecord *object, void *unused);
int View_CurrentWidening(unsigned int view);
int View_CameraMoveStep(int cameraType, int view);
#include "CockpitEyeShift.h"
void StageObject_ResetBodyTextureCaches(void);
void CarLight_ResetTextureState(int index);
void CarLight_CacheBodyTextures(int car, int unused1, int unused2, BYTE flag);
void CarLight_SetChannelMasks(int index, char set0, char set1, BYTE mask);
void CarLight_GetScaledChannelLevels(int lane, int *pA, int *pB, int slot);
void StageObject_AnimateCarLightLevels(int car);
void CarLight_InvalidateAppliedLevels(void);
void StageObject_StartCarSoundTimer(int index);
void StageObject_UpdateCarSoundElapsedTime(int index);
void StageObject_ResetCarSoundElapsedTime(int index);
void StageObject_AddCarSoundTimeSeconds(int index, int seconds);
int StageObject_GetCarSoundElapsedTime(int index);
void StageObject_CopyCarSurfaceNoiseTarget(Car *pCar);
BYTE StageObject_LoadStageMenuSounds(void);
void StageObject_SetSoundStateValue(int value);
void SurfaceSound_UpdateNearestLocalCarEngines(void);
void SurfaceSound_UpdateNearestNetworkCarEngines(void);
void CarInput_UpdateRaceOrderSlot(int slot);
void StageObject_UpdatePlayerControlIndicators(int player, int device);
void StageObject_DriveCPUOrderSlot(int slot);
void CarInput_UpdateWaitingStartSlot(int slot);
void StageObject_BrakeFinishedCarSlot(int slot);
void StageObject_ResetCarStartControls(int index);
void View_UpdateDriverCameraCycle(unsigned int param_1);
void StageObject_CycleDriverCameraSelection(unsigned int param_1, unsigned int param_2);
void StageObject_ReadCarControllerMapping(int slot);
void StageObject_StartFreshSession(void);
void StageObject_StartReplaySession(char restart);
void AI_UpdateRouteDrivingControls(Car *pCar, int car, int preview);
void StageObject_ClearUnusedCarSlotValues(void);
void StageObject_SeedRaceRandomAndCPURevs(char replay, char restart);
BYTE *AI_LoadStageRouteTables(void);
int StageObject_GetCarSlotStateValue(int index);
void StageObject_ResetObjectValuesToUnity(void);
unsigned int AI_SelectWallCollisionResponse(int *pState);
void StageObject_BuildTypeTableRowOutput(int param_1, int *pControls, int *param_3);
void StageObject_ProjectRotationTableRows(AINetwork *param_1);
void StageObject_LookupKeyedValuePair(int unused, int table, int entry, int *pOut1, int *pOut2);
void StageObject_SetBaseCarCountAndState(void);
void StageObject_UpdateApproachingCar(int carIndex, int time);
int StageObject_GetTypeTableIndexValue(int index);
int AI_FindClosestCarInAngleWindow(int param_1, int *param_2, int param_3, int *param_4);
unsigned int AI_SelectCarsWithinOvertakeWindow(int index, BYTE *pOut, int *pCount, BYTE *pFlag);
int AI_UpdateAttractDrivingControls(int car, int preview);
void StageObject_CreateRecordGlows(void);
void StageObject_SpawnCarHeadlightGlow(BYTE car);
void StageObject_TestHeadlightGlowsAgainstCarBox(Car *pCar, CollisionBox *pBox);
void CarDamage_UpdateFlyingDebris(void);
void StageObject_InterpolateHeadlightGlows(int t);
void StageObject_SetVehicleEffectColour(BYTE *pColour);
void Fireworks_Init(BYTE count);
BYTE StageObject_ReleaseVehicleModelFiles(void);
void Fireworks_Update(void);
void Firework_InterpolateRocketColours(FireworkRocket *p, BYTE *pOut, BYTE *pFrom, BYTE *pTo);
void Fireworks_Draw(void);
void StageObject_SpawnDebris(const FixVector *pPosition, const FixVector *pVelocity, unsigned int variant);
void StageObject_FadeAmbientColourToBase(void);
void StageObject_SpawnRandomDebrisBurst(void);
void StageObject_SetVehicleEffectState(int value);
void StageObject_UpdateDebris(int scale);
void StageObject_ClearPartTuningState(void);
void CarPart_ResetDamageThresholds(void);
void CarPart_HideNode(Car *pCar, int slot, int hidden);
void StageObject_AssignMatchingCarClassSlots(Car *pCar, SceneNode *pObject, BYTE flag);
void CarPart_ResetAttachedNodes(Car *pCar);
void CarPart_IntegrateDetachedMotion(void);
unsigned int CarPart_GetModelIndex(int slot, int carIndex);
SceneNode *CarPart_GetNode(Car *pCar, int slot);
void CarDamage_StepClimbingCarRecords(short *param_1, short count);
void StageObject_ApplyRandomizedBodyImpulse(FixVector *pImpulse);
void StageObject_IntegrateCarMotionRecord(int index);
void StageObject_DrawListedCarLightBeams(short *pOrder, short count, int view);
void StageObject_DrawObjectDustTrail(unsigned int index, int *pTarget, int flag);
int StageObject_DistanceFade(FixVector *delta);
void StageObject_InterpolateOrderedMotionRecords(int scale);
void StageObject_ResetVectorListRecord(int list, int index, int value);
void StageObject_DestroyStageKindCarNodes(void);
void CarInfo_CacheCameraOffsets(void);
void StageObject_SelectAndCopyCarNodePayload(CameraRecord *pObj, int *pSrc, BYTE index, BYTE value);
void CarCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther);
void CarCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther);
void CarCamera_SetFullCarLift(CameraRecord *pRecord, CameraRecord *pOther);
void CarCamera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther);
void Collision_BuildCarBox(CollisionBox *pBox, int *pBodyAxes, FixVector *pPosition, FixVector *pCorners);
void Collision_BuildBoxFootprint(CollisionBox *pBox, FixVector *pPosition);
int StageObject_GetCollisionRecordState(void);
void Collision_BuildOrientedBoxWorldCorners(FixVector *pCorners, FixVector *pCenter, FixMatrix *pRot, FixVector *pHalf);
void Collision_BuildObjectPointBounds(FixVector *pBasis, FixVector *pCorners, SectorObjectBounds *pBounds);
void Collision_UpdateOrderedCars(Car *pCars, short *pOrder, short count);
int Collision_TestCarAgainstSectorObjects(Car *pCar);
int Collision_DoSpheresOverlap(int r1, int r2, int *pA, int *pB);
void Collision_BuildObjectBox(CollisionBox *pBox, SectorObjectEntry *pEntry, FixVector *pPosition);
void Collision_SetObjectSphere(FixVector *pPosition, SectorObjectEntry *pEntry);
void Collision_ApplyObjectOverlapDamping(CollisionBox *pBox, Car *pCar);
int Collision_ResolveCarTurnedObjectContact(Car *pCar, StageObject **pEntry, CollisionBox *pBox, CollisionBox *pObject);
int Collision_TestOrientedBoxCornerOverlap(CollisionBox *pBoxA, CollisionBox *pBoxB, FixVector *pOffset, int scale);
int Collision_FindQuadEdgeOverlap(FixVector *pVertsA, FixVector *pVertsB, FixVector *pDir, int *pDistance);
void StageObject_ApplyRecursiveFrameDelta(BYTE index, char other, int *pDelta, int flag);
void StageObject_MoveRecursiveBoxCorners(BYTE index);
CollisionBox *Collision_GetCarBox(int index);
void StageObject_StartAndRegisterFrameCallback(void);
int StageObject_GetSelectedRecordListState(void);
int StageObject_SelectNonemptyRecordList(int *pList);
void View_PlaceTracksideCameraAtSpot(CameraRecord *pRecord, FixMatrix *pRef, int spot);
void View_UpdateTracksideZoomAndShake(CameraRecord *pRecord, FixMatrix *pRef);
void View_RestartTracksideCameraDolly(CameraRecord *pRecord, FixMatrix *pRef);
void View_BuildTracksideCameraMatrix(CameraRecord *pRecord, FixMatrix *pRef);
void TracksideCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther);
void TracksideCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther);
void TracksideCamera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther);
unsigned int StageObject_FindNearestCameraSpot(FixVector *pPos);
int StageObject_GetCarCameraSpotIndex(CameraRecord *pRecord);
void View_UpdateTracksideAimTarget(CameraRecord *pRecord, FixMatrix *pMatrix);
void View_StepTracksideAim(CameraRecord *pRecord, int step);
void View_StepTracksideZoom(CameraRecord *pRecord, int step);
void View_AdvanceTracksideDolly(CameraRecord *pRecord, int amount);
void View_BuildTracksideAimLead(FixVector *pOut, CameraRecord *pRecord, Car *pCar, FixMatrix *pMatrix);
int StageObject_IsNegativeRightAngleCameraSpot(CameraRecord *pRecord);
void CarPhysics_UpdateWheelSlipAndVelocityDamping(Car *param_1);
void Collision_TestCarAgainstSectorEdges(Car *pCar, int param);
int Collision_TestSectorEdgeEndpoints(char type);
int Collision_ResolveSectorFaceContact(int *param_1, int *param_2, int param_3, char param_4);
int StageObject_IsCarOutsideCollisionHeightInterval(void);
void StageObject_ColourGroundMeshReferencePoint(DWORD *pColour, DWORD *pReference);
void StageObject_ColourObjectMeshReferencePoint(DWORD *pColour, DWORD *pReference);
void StageObject_YawMainAndSunNodes(int angle, int unused, int sunAngle);
void StageObject_GetRotatedStageLightVector(FixVector *pOut);
void StageObject_GetStageLightValues(int *pOut1, int *pOut2, int *pOut3);
void StageObject_GetStageLightState(int *pOut);
void StageObject_SetStageLightState(int value);
int StageObject_FindClosestMeshVertex(void);
void StageObject_RandomizeStageTriangleTextures(void);
void StageObject_GetGroundReferenceColour(int *pOut);
void StageObject_OrientLightNodesToView(int view);
void StageObject_SetUnboostedStageLight(FixVector *pLight);
void StageObject_SetForwardedFlareState(int value);
int Track_GetGroundHeight5(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass, int defaultY);
void StageObject_UpdateCarCornerGroundHeights(Car *pCar, int count);
CarContact *StageObject_GetMotionRecord(int index);
int StageObject_Atan2Degrees(int y, int x);
int StageObject_WrapFixedDegreeAngle(int angle);
unsigned int StageObject_GetAnyDeviceHeldButtons(void);
int StageObject_RunNodeActionAndSoundCallback(void *pNode, int value);
// --- end module prototypes ---

struct GlowLight;
void *CarInfo_GetSection(Car *pCar, int section);
struct KnockoutMatch;
int Knockout_HasHumanLostMatch(KnockoutMatch *pMatch);
int Car_UsesNarrowWheels(Car *pCar, int variant);
int StageObject_GetWheelSlip(int carIndex, int wheelIndex);
BYTE StageObject_GetViewWeatherStateByte(int index);
int StageObject_GetViewWeatherField54(int index);
int *StageObject_GetViewWeatherSlot(int carIndex, int wheelIndex);
unsigned int StageObject_GetLoadedVariantDataAndState(SectorObjectEntry **pOut);
void RallyData_CopyRaisedElementVector(int *pDest, SectorObjectEntry **pEntry);
int Track_GetGroundHeight(FixVector *pPoint, FixVector *pNormal, short *pTri,
                          unsigned short *pSurface, int defaultY);
void Sector_RemoveNode(SceneNode *pNode);
void Sector_InsertNodeByPosition(SceneNode *pNode);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
void Stage_InitLightMeshes(void);
int *RallyData_GetDriverSettingPair(int index);
void Track_ShiftMeshAndAmbientHeights(int oldHeight, int newHeight, int mode);
void StageObject_SetStageLightState(int value);
void StageObject_SetForwardedFlareState(int value);
void StageObject_BuildCarNodeOrientation(CameraRecord *object, int *src);
void CarPhysics_UpdateWheelSlipAndVelocityDamping(Car *param_1);
short *Car_GetOrder(void);
short Car_GetOrderCount(void);
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile);
void StageObject_MoveRecursiveBoxCorners(BYTE index);
char *StageObject_FormatAnimatedLapTime(int *p, int index, int mode);
void RallyData_SetSelectionBits14To15(BYTE param1);
unsigned short Input_GetControllerSlotMapping(unsigned short slot);
void Car_SetDrawnFlag(int index, char value);
int CarPart_GetHiddenState(Car *pCar, int slot);
void CarInterior_UpdateWiperSweep(int index);
Texture *InRaceMenu_GetRoundBoxTexture(void);
int CarInterior_UpdateSteeringBlendFraction(int car, Car *pCar);

// Global fixed-point lighting parameters for both stage conditions.
// GLOBAL: CMR2 0x00547950
StageLightingState g_stageLighting;

// Default intensity by stage weather index.
// GLOBAL: CMR2 0x0051b0b8
int g_stageWeatherIntensity[9] = {
    0x10000, 0x10000, 0x1999, 0x1999, 0x1999, 0x1999, 0x1999, 0x1999, 0x1999
};

struct StageSurfaceInfo {
    int flags;
    BYTE red, green, blue, alpha;
};
// GLOBAL: CMR2 0x0051bc68
StageSurfaceInfo g_stageSurfaceInfo[9] = {
    {0, 0, 0, 0, 0}, {1, 2, 2, 2, 0x18},
    {9, 0x48, 0x32, 0x27, 0x12}, {9, 0x14, 0x0d, 0, 0x12},
    {9, 0x0e, 0x0c, 0x06, 0x12}, {0x16, 0xad, 0xbd, 0xc6, 0x12},
    {0x16, 0xf4, 0xf4, 0xf4, 0x12}, {9, 0x48, 0x32, 0x27, 0x12},
    {4, 0x64, 0x64, 0x64, 0xff}
};
// GLOBAL: CMR2 0x0051bcb0
BYTE g_stageSurfaceMap[48] = {
    0, 7, 0, 0, 4, 4, 7, 7, 7, 0, 7, 7, 2, 2, 0, 0,
    0, 6, 5, 0, 0, 0, 6, 6, 0, 1, 2, 7, 7, 0, 6, 0,
    0, 0, 0, 3, 3, 3, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0
};
// GLOBAL: CMR2 0x00588620
BYTE g_trailColor[8][4];
// GLOBAL: CMR2 0x00588750
int g_trailFrame;
// OpenCMR2: the skid-mark ring holds up to TRAIL_MAX points per wheel. The original size, 200, stays the default; video.tire_track_length (0 = original 200, else 200..TRAIL_MAX)
// lengthens the ring of every car slot (points are only laid for the viewed car of each view, so one car per view), so the geometry drawn per frame stays bounded: 4 wheels x 1000 points x 2 triangles = 8000 per view, 2 views fit the 16384-entry skid layer. Time-based expiry never runs in this port
// (a point's age is only ever set to 0), so the ring size is what decides how long marks stay.
#define TRAIL_MAX 1000
extern BYTE g_trailPointUsed[8][4][TRAIL_MAX];
int StageObject_TrailRing(int car);
// One point of a wheel's skid mark ring (200 per wheel and car).
struct TrailPoint {
    int age;                // 0x00 frames since it was laid
    int frame;              // 0x04 g_trailFrame when it was laid
    FixVector left;         // 0x08 the two edges of the mark
    FixVector right;        // 0x14
    BYTE colour[4];         // 0x20
    BYTE alphaLeft;         // 0x24
    BYTE alphaRight;        // 0x25
    BYTE material : 4;      // 0x26 surface material
    BYTE flags : 4;
    BYTE pad;
};
extern TrailPoint g_trailPoints[8][4][TRAIL_MAX];
extern int g_unk0x00549b20[8][4];
extern FixVector g_carTyreTrailCenters[8][4];

// Accessors of the stage object tables (0x460bf0-0x4789b0)

extern ViewWeatherState *g_viewWeather;
extern BYTE g_weatherViewCount;
extern FixVector g_weatherWindDirection;
extern int g_weatherWindStrength;
extern BYTE *g_movingObjectMeshIndex;
extern Mesh **g_movingObjectMeshes;
extern int *g_movingObjectTypes;
extern CarSurfaceRamp *g_carSurfaceRamps;
extern int g_knockoutRaceState;
extern SectorObjectEntry *g_movingSectorEntries;
extern unsigned int g_unk0x0058ca6c;

int Replay_ResetBufferIfActive(ReplayStream *p);
int RallyData_GetActiveCarRaceRecordField0(Car *p);
int RallyData_GetRouteAvailabilityState(void);
unsigned int RallyData_GetSelectionFlag27(void);
unsigned char RallyData_GetSelectionFlag28(void);
float StageTiming_GetSelectedStartTableFloat(void);

// Chooses the stage object path for the current game mode and rally state.
// FUNCTION: CMR2 0x0046bd50
int StageObject_UsesExtendedMode(void)
{
    if (CGameInfo::GetConfiguredGameMode() == 8 ||
        CGameInfo::GetConfiguredGameMode() == 9 ||
        CGameInfo::GetConfiguredGameMode() == 10)
        return false;
    if (CGameInfo::GetConfiguredGameMode() == 11 ||
        CGameInfo::GetConfiguredGameMode() == 12)
        return true;
    if (CGameInfo::GetConfiguredGameMode() != 0 &&
        CGameInfo::GetConfiguredGameMode() != 1 &&
        CGameInfo::GetConfiguredGameMode() != 2 &&
        CGameInfo::GetConfiguredGameMode() != 3)
        return true;
    return (BYTE)RallyData_GetSelectionFlag27() != 0;
}

void Game_SetTriangleField2CByGroup(Mesh *pMesh, int mask, int value);
void Game_SetTriangleField30ByGroup(Mesh *pMesh, int mask, int value);

// Enables/disables the sub-meshes of a stage object's model according to the
// object type and the current game mode.
// FUNCTION: CMR2 0x004694a0
void StageObject_SetModelSubmeshVisibility(Car *pCar, int param_2, char param_3)
{
    CarPartSet *pParts;
    CarSceneRecord *pType;
    int i;
    int uVar4;
    int uVar3;

    pParts = CarPart_GetSet(pCar->index);
    pType = StageTiming_GetStartTableRecord(pCar->index);
    if ((char)pType->detailCode == 'C' ||
        (pType = StageTiming_GetStartTableRecord(pCar->index), (char)pType->detailCode == 'A')) {
        switch (param_2) {
        case 0:
            uVar4 = 0;
            uVar3 = 1;
            break;
        case 1:
            uVar4 = 3;
            uVar3 = 4;
            break;
        default:
            if ((BYTE)param_3 == 4 || (BYTE)param_3 == 5) {
                uVar4 = 3;
                uVar3 = 4;
            } else {
                uVar4 = 5;
                uVar3 = 7;
            }
            break;
        }
        switch ((BYTE)param_3) {
        case 0:
            i = StageTiming_FindModelPartByNodeType(7, pParts);
            if (i >= 0) {
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 0x100, uVar4);
                Game_SetTriangleField30ByGroup(pParts->geometry.meshes[i], 0x100, uVar3);
                return;
            }
            break;
        case 1:
            i = StageTiming_FindModelPartByNodeType(0xc, pParts);
            if (i >= 0) {
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 0x20, uVar4);
                Game_SetTriangleField30ByGroup(pParts->geometry.meshes[i], 0x20, uVar3);
            }
            i = StageTiming_FindModelPartByNodeType(7, pParts);
            if (i >= 0) {
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 0x20, uVar4);
                Game_SetTriangleField30ByGroup(pParts->geometry.meshes[i], 0x20, uVar3);
                return;
            }
            break;
        case 2:
            i = StageTiming_FindModelPartByNodeType(7, pParts);
            if (i >= 0) {
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 0x40, uVar4);
                Game_SetTriangleField30ByGroup(pParts->geometry.meshes[i], 0x40, uVar3);
                return;
            }
            break;
        case 3:
            i = StageTiming_FindModelPartByNodeType(7, pParts);
            if (i >= 0) {
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 0x80, uVar4);
                Game_SetTriangleField30ByGroup(pParts->geometry.meshes[i], 0x80, uVar3);
                return;
            }
            break;
        case 4:
            i = StageTiming_FindModelPartByNodeType(0xe, pParts);
            if (i >= 0) {
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 4, uVar4);
                return;
            }
            break;
        case 5:
            i = StageTiming_FindModelPartByNodeType(0xe, pParts);
            if (i >= 0)
                Game_SetTriangleField2CByGroup(pParts->geometry.meshes[i], 8, uVar4);
            break;
        }
    }
}

// FUNCTION: CMR2 0x00469de0
int StageObject_IsEligibleType(short type, int mode, int category)
{
    int result = 0;
    if (mode == 0 || category == 6) {
        switch (type) {
        case 1:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 16:
        case 24:
        case 25:
        case 27:
        case 28:
            result = 1;
        }
    }
    return result;
}

// Full strength within ten fixed-point units, fading to zero at fifty.
// FUNCTION: CMR2 0x004863d0
int StageObject_DistanceFade(FixVector *delta)
{
    int component;
    int length;
    int fade;

    component = delta->x;
    if (component < 0) component = -component;
    if (component <= 0x320000) {
        component = delta->y;
        if (component < 0) component = -component;
        if (component <= 0x320000) {
            component = delta->z;
            if (component < 0) component = -component;
            if (component <= 0x320000) {
                length = FixVecLength(delta);
                if (length <= 0x320000) {
                    length -= 0xa0000;
                    if (length <= 0) return 0x10000;
                    fade = FixMul(length, 0x666);
                    if (fade > 0x10000) fade = 0x10000;
                    return 0x10000 - fade;
                }
            }
        }
    }
    return 0;
}

// GLOBAL: CMR2 0x00543f28
int g_unk0x00543f28[8 * 4];
// GLOBAL: CMR2 0x00547b80
BYTE g_unk0x00547b80;
// GLOBAL: CMR2 0x00548110
BYTE g_unk0x00548110[8][8];
// GLOBAL: CMR2 0x00588758
ReplayStream *g_activeReplayStream;
// GLOBAL: CMR2 0x00588760
char g_unk0x00588760;
// GLOBAL: CMR2 0x00588761
signed char g_unk0x00588761;
// GLOBAL: CMR2 0x00588864
int g_unk0x00588864;
// GLOBAL: CMR2 0x00588868
int g_unk0x00588868;
// GLOBAL: CMR2 0x00588970
int g_carDamageModelEnabled[8];
// GLOBAL: CMR2 0x00588ba4
BYTE g_unk0x00588ba4[16];
// GLOBAL: CMR2 0x00588bb4
int g_carPairValuePending[16];
// GLOBAL: CMR2 0x00588cd4
ReplayLevelState g_replayLevelState;
// GLOBAL: CMR2 0x00588d38
BYTE *g_unk0x00588d38;
// GLOBAL: CMR2 0x00589438
SceneNode *g_stageSkyNode;
// GLOBAL: CMR2 0x0058943c
SceneNode *g_stageGroundNode;
// GLOBAL: CMR2 0x00589440
SceneNode *g_stageCloudNode;
// GLOBAL: CMR2 0x00589444
SceneNode *g_stageCloudTopNode;
// GLOBAL: CMR2 0x00589448
GenericFile g_stageCloudArchive;
// GLOBAL: CMR2 0x0058cf68
int g_knockoutRoundActive;
// GLOBAL: CMR2 0x0058cf6c
int g_unk0x0058cf6c;
// GLOBAL: CMR2 0x0058cf80
Menu g_inRaceActionMenu;
// Scratch rotation matrix for the stage-object nodes.
// GLOBAL: CMR2 0x0058d260
FixMatrix g_carWiperRotationMatrix;
StageBlock g_stageBlock;
// GLOBAL: CMR2 0x0058da30
int g_carSoundElapsed[8];
// GLOBAL: CMR2 0x0058da10
int g_carSoundTimerStart[8];
// GLOBAL: CMR2 0x0058dda8
int g_unk0x0058dda8;
// GLOBAL: CMR2 0x0058e230
int g_unk0x0058e230[15];
// Random seed of the stage (kept for replays).
// GLOBAL: CMR2 0x0058e26c
int g_stageRandomSeed;
// GLOBAL: CMR2 0x0058e270
char g_unk0x0058e270[16];
// GLOBAL: CMR2 0x0058e0b0
BYTE g_unk0x0058e0b0[8];
struct StageObjectValue { int value; BYTE rest[0x2c]; };
// GLOBAL: CMR2 0x0058e0b8
StageObjectValue g_unk0x0058e0b8[4];
// Driving state of the CPU car being updated (0xb8 bytes; AI_UpdateRouteDrivingControls and the
// helpers it calls read and write its fields by offset).
// GLOBAL: CMR2 0x0058e178
int g_aiDriveState[0x2e];

// GLOBAL: CMR2 0x005113f8
double g_radiansToDegrees = 57.295827908797776;

// The larger of the longitudinal and lateral wheel slip, after each dead zone.
// FUNCTION: CMR2 0x00465d70
int StageObject_GetWheelSlip(int carIndex, int wheelIndex)
{
    if (carIndex < 8) {
        int slip = Car_Get(carIndex)->wheelSlipLateral[wheelIndex];
        if (slip < 0)
            slip = -Car_Get(carIndex)->wheelSlipLateral[wheelIndex];
        else
            slip = Car_Get(carIndex)->wheelSlipLateral[wheelIndex];
        slip -= 0xccc;
        if (slip < 0) slip = 0;

        int lateral = (Car_Get(carIndex)->wheelSlip[wheelIndex] < 0
                           ? -Car_Get(carIndex)->wheelSlip[wheelIndex]
                           : Car_Get(carIndex)->wheelSlip[wheelIndex]) - 0x2666;
        if (lateral < 0) lateral = 0;
        if (lateral > slip) slip = lateral;
        if (slip < 0) slip = -slip;
        if (slip > 0) {
            slip = FixDiv(slip, 0x10000);
            if (slip < 0x10000) return slip;
            return 0x10000;
        }
    }
    return 0;
}

// Returns a 16.16 angle in degrees from the two vector components.
// FUNCTION: CMR2 0x00498d80
int StageObject_Atan2Degrees(int y, int x)
{
    double degrees = X87S_MUL(atan2((double)y, (double)x), g_radiansToDegrees);
    return (int)FTOI(__int64, X87S_MUL(degrees, CGraphics::m_65536));
}

int Race_GetScaledState37F24(void);
char Race_GetBaseCarCount(void);

// FUNCTION: CMR2 0x00478130
void StageObject_StartCarSoundTimer(int index)
{
    g_carSoundTimerStart[index] = Race_GetScaledState37F24();
}

// FUNCTION: CMR2 0x00478150
void StageObject_UpdateCarSoundElapsedTime(int index)
{
    g_carSoundElapsed[index] = Race_GetScaledState37F24() - g_carSoundTimerStart[index];
}

// FUNCTION: CMR2 0x0047aa60
void StageObject_SetSoundStateValue(int value)
{
    g_unk0x0058dda8 = value;
}

// FUNCTION: CMR2 0x0047c5b0
int StageObject_GetCarSlotStateValue(int index)
{
    return g_unk0x0058e230[index];
}

// FUNCTION: CMR2 0x0047c5c0
void StageObject_ResetObjectValuesToUnity(void)
{
    int i;

    for (i = 0; i < 4; i++)
        g_unk0x0058e0b8[i].value = 0x10000;
}

// The stage's AI tables, set by AI_LoadStageRouteTables from ai0/ai1/ai2.dat.
// Each table is numbered from 1, as the file references them; entry 0 is unused.
struct AIRouteTables {
    BYTE *routes[6];          // 0x000 [1..counts[1]] 0x88-byte route records
    char *descriptors[40];    // 0x018 [1..counts[2]] 0x14-byte input descriptors
    AINetwork *networks[10];  // 0x0b8 [1..counts[3]] driving networks
    BYTE *points[10];         // 0x0e0 [1..counts[0]] 0x10-byte route points
    signed char counts[4];    // 0x108 entries of points, routes, descriptors, networks
    int *pActivation;         // 0x10c activation table of the networks (64 entries)
};
CMR2_LAYOUT_CHECK(AIRouteTablesSize, sizeof(AIRouteTables) == 0x110);

// GLOBAL: CMR2 0x0058e394
AIRouteTables g_aiRouteTables;
// ON-DISK: one record per route node (370 of them) in the AI archive, after
// the networks' activation buffer and before the reference routes.
struct AIRouteNodeInfo {
    BYTE field_0x0;
    BYTE pad_0x1[3];
    int key;                // 0x4 matched against the route tables' entries
    signed char wall[8];    // 0x8 clearance to the walls: 9, 10 on one side,
                            //     0xc..0xe on the other (StageObject offsets)
};
// GLOBAL: CMR2 0x0058e4a4
AIRouteNodeInfo *g_aiRouteNodes;

// Looks up the pair of values that table `table` gives for the key of entry `entry`.
// FUNCTION: CMR2 0x0047cbc0
void StageObject_LookupKeyedValuePair(int unused, int table, int entry, int *pOut1, int *pOut2)
{
    int key = g_aiRouteNodes[entry].key;
    BYTE *p = g_aiRouteTables.routes[table];
    int i;

    for (i = 0; i < *(int *)(p + 0x84); i++) {
        if (key == (char)p[4 + i * 8]) {
            *pOut1 = (char)p[7 + i * 8];
            *pOut2 = (char)g_aiRouteTables.routes[table][6 + i * 8];
            return;
        }
    }
}

// FUNCTION: CMR2 0x0047cc30
void StageObject_SetBaseCarCountAndState(void)
{
    g_unk0x0058e0b0[0] = Race_GetBaseCarCount();
    g_unk0x0058e0b0[1] = 1;
}

// FUNCTION: CMR2 0x0047cd00
int StageObject_GetTypeTableIndexValue(int index)
{
    return g_unk0x0058e270[index];
}

struct Block0x309 { int data[0x309]; };
struct Block0x134 { int data[0x134]; };
struct Block6 { int data[6]; };

struct StageTableEntry { int flag; short a; short b; };
// GLOBAL: CMR2 0x0051b9f0
StageTableEntry g_unk0x0051b9f0[5] = {
    {0, 2, 2}, {0, 2, 1}, {0x10000, 2, 1}, {0, 1, 2}, {1, 1, 2}
};

// FUNCTION: CMR2 0x00464b00
StageTableEntry *StageObject_GetScreenRectangleEntry(int index)
{
    return &g_unk0x0051b9f0[index];
}

// FUNCTION: CMR2 0x0046b6b0
void StageObject_ClearNodeValueBelowThreshold(SceneNode *pNode, BYTE threshold)
{
    if (pNode->type == 0 && pNode->pObject != NULL &&
        ((Mesh *)pNode->pObject)->pTriangles->colour[0][3] <= threshold)
        pNode->viewMask = 0;
}

// FUNCTION: CMR2 0x0046b6e0
void StageObject_ClearNodeTreeValuesBelowThreshold(SceneNode *pNode, BYTE threshold)
{
    for (; pNode != NULL; pNode = pNode->pNext) {
        StageObject_ClearNodeValueBelowThreshold(pNode, threshold);
        if (pNode->pFirstChild != NULL)
            StageObject_ClearNodeTreeValuesBelowThreshold(pNode->pFirstChild, threshold);
    }
}


// Moving stage objects (debris and the like): one object in the original, so a
// store into entries[] makes MSVC re-read count.
// GLOBAL: CMR2 0x005894b8
BYTE g_farVotes[40];    // per entry: cars for which it is the farthest

// GLOBAL: CMR2 0x005894e0
MovingObjects g_movingObjects;

// FUNCTION: CMR2 0x0046f7e0
void StageObject_ResetMovingObjectCountsAndPhases(void)
{
    int i;

    g_movingObjects.count = 0;
    for (i = 0; i < 40; i++) {
        if (g_movingObjects.entries[i].pObject != NULL)
            g_movingObjects.entries[i].pObject->current.position.y += 0xd8f00000U;
    }
}

// Initializes a moving stage object from its route entry and the car's motion.
// FUNCTION: CMR2 0x0046f810
void StageObject_InitMovingObject(MovingObject *pState, int carIndex)
{
    int triangle = -1;
    int surfaceIndex = 0;
    Car *pCar = Car_Get(carIndex);
    SectorObjectEntry *pEntries;
    int count = StageObject_GetLoadedVariantDataAndState(&pEntries);
    int entryIndex = -1;
    int i;
    int ground;
    int airborne;
    FixVector position;
    FixVector velocity;
    FixVector local;
    FixVector axis;

    for (i = 0; i < count; i++) {
        if (pEntries + i == pState->pEntry) {
            entryIndex = i;
            i = count;
        }
    }
    if (entryIndex >= 0) {
        pState->objectType = g_movingObjectTypes[g_movingObjectMeshIndex[entryIndex]];
        pState->pObject->type = SCENE_NODE_MESH;
        pState->pObject->pObject = g_movingObjectMeshes[g_movingObjectMeshIndex[entryIndex]];
        pState->pObject->viewMask = (BYTE)(1 << carIndex);
        pState->actionState = -0x10000;

        RallyData_CopyRaisedElementVector((int *)&position, &pState->pEntry);
        FixMatrix_SetPosition(&position, &pState->pObject->current);
        FixMatrix_GetRight(&local, &pState->pEntry->pObject->matrix);
        FixMatrix_SetRight(&local, &pState->pObject->current);
        FixMatrix_GetUp(&local, &pState->pEntry->pObject->matrix);
        FixMatrix_SetUp(&local, &pState->pObject->current);
        FixMatrix_GetForward(&local, &pState->pEntry->pObject->matrix);
        FixMatrix_SetForward(&local, &pState->pObject->current);

        pState->pMatrix = &pState->keyB;
        pState->keyB = pState->pObject->current;
        pState->current = pState->pObject->current;
        pState->keyA = pState->pObject->current;

        airborne = 0;
        ground = 0;
        if (pState->objectType == 1) {
            ground = 1;
        } else if (pState->objectType == 0) {
            if (pCar->speed > 0xc000)
                airborne = 1;
            else
                ground = 1;
        }
        pState->groundHeight = 0;
        pState->groundTriangle = -1;

        if (ground) {
            Track_GetGroundHeight(&position, &pState->groundNormal, (short *)&triangle, (unsigned short *)&surfaceIndex, 0);
            velocity = pCar->velocity;
            if (pCar->speed > 0x8000) {
                FixVecScaleRecip(&velocity, &velocity, pCar->speed);
                FixVecScale(&velocity, &velocity, 0x8000);
            }
            FixMatrix_InverseRotateVector(&local, &velocity, pState->pMatrix);
            axis.x = FixMul(local.z, 0x10000);
            axis.y = 0;
            axis.z = -FixMul(local.x, 0x10000);
            if (pState->objectType == 0)
                axis.z = 0;
            pState->probe.x = FixMul(0x8000, pState->pEntry->pObject->pMesh->pCollisionRecords->width);
            pState->probe.y = FixMul(0x8000, pState->pEntry->pObject->pMesh->pCollisionRecords->height);
            pState->probe.z = FixMul(0x8000, pState->pEntry->pObject->pMesh->pCollisionRecords->length);
            pState->field_0xf8.x = 0;
            pState->field_0xf8.y = pState->probe.y;
            pState->field_0xf8.z = 0;
            FixVecScale(&pState->spin, &axis, g_physicsTimeStep);
            pState->mode = 1;
        } else if (airborne) {
            velocity = pCar->velocity;
            FixMatrix_InverseRotateVector(&local, &velocity, &pState->keyB);
            axis.x = -FixMul(local.z, 0x6666);
            axis.y = 0;
            axis.z = FixMul(local.x, 0x6666);
            FixVecScale(&pState->spin, &axis, g_physicsTimeStep);
            FixVecScale(&pState->velocity, &velocity, 0xcccc);
            pState->velocity.y = FixVecLength(&velocity);
            pState->velocity.y = FixMul(pState->velocity.y, 0x4ccc);
            pState->probe.x = FixMul(0x8000, pState->pEntry->pObject->pMesh->pCollisionRecords->width);
            pState->probe.y = FixMul(0x8000, pState->pEntry->pObject->pMesh->pCollisionRecords->height);
            pState->probe.z = FixMul(0x8000, pState->pEntry->pObject->pMesh->pCollisionRecords->length);
            pState->field_0xf8.y = pState->probe.y;
            pState->field_0xf8.x = 0;
            pState->field_0xf8.z = 0;
            pState->field_0x120 = 0;
            pState->mode = 2;
            pState->moved = 1;
        }

        pState->pObject->world = pState->pObject->current;
        if (pState->pObject->sector != -1)
            Sector_RemoveNode(pState->pObject);
        if (pState->pObject->sector == -1)
            Sector_InsertNodeByPosition(pState->pObject);
    } else {
        pState->pObject->type = SCENE_NODE_EMPTY;
    }
}

// Derives the stage's object scale from the loaded records: the average of
// their +0x54 fields (the single record in created-flag mode 1), floored at
// 0x4ccc, and scales the global stage vector by it, quadrupled for a type 2.
// match 84%: MSVC picks EDX for the record count and ESI for the loop counter
// (the original has them swapped); the code itself is identical.
// match 83%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00460a30
void StageObject_DeriveLoadedScaleVector(FixVector *pOut)
{
    DWORD flags;
    int value;
    int typeTwo;
    int count;
    int i;
    int *p;

    // The original loads the whole dword at the flag byte (the variable lived
    // in that translation unit; see CONOCIMIENTO 4.y) and only uses its low byte.
    flags = *(DWORD *)&g_weatherViewCount;
    value = 0;
    typeTwo = 0;
    if ((char)flags == 1) {
        value = g_viewWeather->intensity;
        if (g_viewWeather->kind == 2)
            typeTwo = 1;
    } else {
        count = (BYTE)flags;
        for (i = 0; i < count; i++) {
            value += g_viewWeather[i].intensity;
            if (g_viewWeather[i].kind == 2)
                typeTwo = 1;
        }
        if ((char)flags != 0)
            value = FixDiv(value, count << 16);
        else
            value = 0x4ccc;
    }
    if (value < 0x4ccc)
        value = 0x4ccc;
    if (typeTwo)
        FixVecScale(pOut, &g_weatherWindDirection, FixMul(0x20000, FixMul(value, g_weatherWindStrength)));
    else
        FixVecScale(pOut, &g_weatherWindDirection, FixMul(value, g_weatherWindStrength));
}

// FUNCTION: CMR2 0x00460bf0
BYTE StageObject_GetViewWeatherStateByte(int index)
{
    return *(BYTE *)&g_viewWeather[index].kind;
}

// FUNCTION: CMR2 0x00460c10
int StageObject_GetViewWeatherField54(int index)
{
    return g_viewWeather[index].intensity;
}

// GLOBAL: CMR2 0x00543d50
int g_weatherParticleQualityScale;

// Sets an object's scalar and derives its fixed-point scaled component.
// FUNCTION: CMR2 0x00460c30
void StageObject_SetScaledValue(int value, int index)
{
    ViewWeatherState *entry = &g_viewWeather[index];
    index = value;
    entry->intensity = value;
    value = FixMul(g_weatherParticleQualityScale, index);
    index = (int)entry->particleCapacity << 16;
    entry->targetParticleCount = FixMul(value, index);
}

// FUNCTION: CMR2 0x00460c80
int StageObject_GetCarWeatherRampValue(Car *pCar)
{
    return g_carSurfaceRamps[(signed char)pCar->index].blend;
}

// FUNCTION: CMR2 0x00463270
int *StageObject_GetViewWeatherSlot(int i, int j)
{
    return &g_unk0x00543f28[j + i * 4];
}

// FUNCTION: CMR2 0x00463ce0
void StageObject_SetBoundedWeatherKind(int value)
{
    g_unk0x00547b80 = value;
    if (g_unk0x00547b80 > 7)
        g_unk0x00547b80 = 6;
}

// FUNCTION: CMR2 0x00464af0
BYTE *StageObject_GetViewRectangleIndex(int index)
{
    return g_unk0x00548110[index];
}


extern Texture *g_skidTrailTexture;
extern Texture *g_skidMarkTexture;
extern Texture *g_skidBlankTexture;
extern int g_unk0x00549ba0[8][4];
int StageObject_ClampByteRange(int value);
#define TRAIL_POINT(i) (&g_trailPoints[0][0][0] + (i))
#define TRAIL_FADE(p, f) ((p)->f - (p)->f * (p)->age * 0x7c / 0x4d8)

// Queues the quad (two triangles) joining pCur to pPrev. Returns false when nothing was queued.
static bool DrawTrailSegment(const TrailPoint *pCur, const TrailPoint *pPrev)
{
    BYTE curColour[4] = { 0, 0, 0, 25 };
    BYTE prevColour[4] = { 0, 0, 0, 25 };
    Quad2DInputVertex a, b, c, d;
    int alpha0, alpha1, alpha2, alpha3;
    Texture *pTexture;

    a.u = 0;
    a.v = 0;
    b.u = 0x10000;
    b.v = 0;
    d.u = 0x10000;
    d.v = 0x10000;
    c.u = 0;
    c.v = 0x10000;
    if (pPrev->colour[0] <= 200 && pPrev->age <= 0x4d8) {
        alpha0 = pCur->alphaLeft;
        alpha3 = pPrev->alphaRight;
        alpha2 = pPrev->alphaLeft;
        alpha1 = pCur->alphaRight;
    } else {
        alpha2 = TRAIL_FADE(pPrev, alphaLeft);
        alpha3 = TRAIL_FADE(pPrev, alphaRight);
        alpha0 = TRAIL_FADE(pCur, alphaLeft);
        alpha1 = TRAIL_FADE(pCur, alphaRight);
    }
    alpha0 = StageObject_ClampByteRange(alpha0);
    alpha1 = StageObject_ClampByteRange(alpha1);
    alpha2 = StageObject_ClampByteRange(alpha2);
    alpha3 = StageObject_ClampByteRange(alpha3);
    if (alpha0 <= 1 && alpha1 <= 1 && alpha2 <= 1 && alpha3 <= 1)
        return false;
    curColour[0] = pCur->colour[0];
    curColour[1] = pCur->colour[1];
    curColour[2] = pCur->colour[2];
    prevColour[0] = pPrev->colour[0];
    prevColour[1] = pPrev->colour[1];
    prevColour[2] = pPrev->colour[2];
    a.x = pCur->left.x;
    a.y = pCur->left.y;
    a.z = pCur->left.z;
    *(DWORD *)a.colour = *(DWORD *)curColour;
    a.colour[3] = (BYTE)alpha0;
    b.x = pCur->right.x;
    b.y = pCur->right.y;
    b.z = pCur->right.z;
    *(DWORD *)b.colour = *(DWORD *)curColour;
    b.colour[3] = (BYTE)alpha1;
    c.x = pPrev->left.x;
    c.y = pPrev->left.y;
    c.z = pPrev->left.z;
    *(DWORD *)c.colour = *(DWORD *)prevColour;
    c.colour[3] = (BYTE)alpha2;
    d.x = pPrev->right.x;
    d.y = pPrev->right.y;
    d.z = pPrev->right.z;
    *(DWORD *)d.colour = *(DWORD *)prevColour;
    d.colour[3] = (BYTE)alpha3;
    if (pPrev->left.x == pPrev->right.x && pPrev->left.y == pPrev->right.y)
        return false;
    switch ((g_stageSurfaceInfo[pCur->material & 0xf].flags >> 2) & 3) {
    case 1:
        pTexture = g_skidMarkTexture;
        break;
    case 2:
        a.colour[0] = 0;
        a.colour[1] = 0;
        a.colour[2] = 0;
        a.colour[3] = (BYTE)(alpha0 / 3);
        *(DWORD *)c.colour = *(DWORD *)a.colour;
        b.colour[0] = 0;
        b.colour[1] = 0;
        b.colour[2] = 0;
        b.colour[3] = (BYTE)(alpha1 / 3);
        *(DWORD *)d.colour = *(DWORD *)b.colour;
        pTexture = g_skidBlankTexture;
        break;
    default:
        pTexture = g_skidTrailTexture;
        break;
    }
    Quad2D_QueueFixedTriangle(0, &b, &a, &c, pTexture, 0x26);
    Quad2D_QueueFixedTriangle(0, &d, &b, &c, pTexture, 0x26);
    return true;
}

static TrailHistory<TrailPoint> g_trailHist[8][4];
// Reference radius for skipping far history, in fixed-point world coordinates (16.16). The game's world unit scale is not
// verified natively.
static const long long kTrailHistRadius = 150LL << 16;
static const int kTrailHistSegmentsPerWheel = 1000;
// Points examined in near chunks per wheel per frame. Worst case per wheel per view: 1024 chunk tests (64k point cap / 64) plus 4096 point visits.
static const int kTrailHistVisitsPerWheel = 4096;
void View_GetCameraXZ(unsigned view, int *x, int *z);
static int g_trailDrawView;
static int StageObject_TrailHistoryOption(void)
{
    int n = Sys_GetOption("video.tire_track_history", 0);
    return n < 0 ? 0 : n > 64 ? 64 : n;
}
static void StageObject_TrailHistoryPush(int car, int wheel, const TrailPoint &p)
{
    TrailHistory<TrailPoint> &h = g_trailHist[car][wheel];
    h.Init(StageObject_TrailHistoryOption() * 1024);
    h.Push(p, (int)(((long long)p.left.x + p.right.x) / 2), (int)(((long long)p.left.z + p.right.z) / 2));
}
// Draws the stored history of one car instead of its 200-point ring. Returns false when the option is off.
static bool StageObject_DrawTrailHistory(int index)
{
    int camX, camZ;
    View_GetCameraXZ((unsigned)g_trailDrawView, &camX, &camZ);
    if (StageObject_TrailHistoryOption() == 0)
        return false;
    const int hstep = g_stageSurfaceInfo[8].flags > 0 ? g_stageSurfaceInfo[8].flags : 1;
    for (int wheel = 0; wheel < 4; wheel++) {
        const TrailHistory<TrailPoint> &h = g_trailHist[index][wheel];
        if (!h.Active() || h.count < 2)
            continue;
        h.Walk(camX, camZ, kTrailHistRadius, kTrailHistSegmentsPerWheel, kTrailHistVisitsPerWheel, [&](const TrailPoint &cur, const TrailPoint &prev) {
            if (cur.frame != prev.frame + hstep || (prev.left.x == prev.right.x && prev.left.y == prev.right.y))
                return false;
            return DrawTrailSegment(&cur, &prev);
        });
    }
    return true;
}

// Draws the tyre marks of one car: for every wheel it walks its ring of 200
// trail points backwards in steps of g_stageSurfaceInfo[8].flags, joins each
// point to the previous consecutive one with a textured quad (two triangles)
// whose alpha fades with the points' age, and picks the mark texture from the
// surface the point was laid on.
// FUNCTION: CMR2 0x004658e0
void CarEffects_DrawTyreMarks(int index)
{
    int *pHead;
    int base;
    int wheel;
    int j;
    int step;
    int cur;
    int k;
    int found;
    TrailPoint *pCur;
    TrailPoint *pPrev;

    if (index >= 8 || CGameInfo::GetGraphicsOptionBits25To26() == 2)
        return;
    if (StageObject_DrawTrailHistory(index))
        return;
    base = index * 4 * TRAIL_MAX;
    const int ring = StageObject_TrailRing(index);
    pHead = g_unk0x00549ba0[index];
    for (wheel = 4; wheel != 0; wheel--, pHead++, base += TRAIL_MAX) {
        j = 0;
        do {
            found = 0;
            cur = ((*pHead % ring) - j + ring) % ring;
            step = g_stageSurfaceInfo[8].flags;
            if (step > 1) {
                k = cur - step + ring;
                do {
                    if (found)
                        break;
                    step--;
                    k++;
                    if ((&g_trailPointUsed[0][0][0])[base + cur] == 0 ||
                        (&g_trailPointUsed[0][0][0])[base + k % ring] == 0)
                        continue;
                    pPrev = TRAIL_POINT(k % ring + base);
                    pCur = TRAIL_POINT(base + cur);
                    found = pCur->frame == pPrev->frame + step;
                    if (found != 1)
                        continue;
                    DrawTrailSegment(pCur, pPrev);
                } while (step > 1);
            }
            j += step;
        } while (j <= ring - 1);
    }
}
#undef TRAIL_POINT
#undef TRAIL_FADE

// FUNCTION: CMR2 0x00465ea0
int StageObject_ClampByteRange(int value)
{
    if (value < 0)
        return 0;
    if (value > 0xff)
        value = 0xff;
    return value;
}

// FUNCTION: CMR2 0x00465f80
void Replay_ResetSelectionIndices(void)
{
    g_unk0x00588760 = 0xff;
    g_unk0x00588864 = -1;
}

// GLOBAL: CMR2 0x0058875c
Car *g_ghostCar;

void StageTiming_SetNodeMeshAlpha(SceneNode *pNode, int alpha, BYTE checkFlag);
void StageTiming_SetNodeTreeMeshAlpha(SceneNode *pNode, int alpha, BYTE checkFlag);

// Makes a car the ghost car: flags it and fades its body nodes in.
// FUNCTION: CMR2 0x00465fc0
void CarEffects_MakeGhost(Car *pCar)
{
    g_ghostCar = pCar;
    pCar->isGhost = 1;
    g_unk0x00588761 = -1;
    g_unk0x00588864 = -1;
    StageTiming_SetNodeMeshAlpha(pCar->pSceneRoot, 100, 1);
    StageTiming_SetNodeMeshAlpha(pCar->pBodyNode, 100, 1);
    StageTiming_SetNodeTreeMeshAlpha(pCar->pSceneRoot->pFirstChild, 100, 1);
    StageTiming_SetNodeTreeMeshAlpha(pCar->pBodyNode->pFirstChild, 100, 1);
}

// FUNCTION: CMR2 0x00466080
void Replay_ResetActiveBufferState(void)
{
    Replay_ResetBufferIfActive(g_activeReplayStream);
}

// FUNCTION: CMR2 0x00466090
int Replay_GetActiveBufferState(void)
{
    if (g_activeReplayStream != NULL)
        return g_activeReplayStream->playing;
    return 0;
}

// FUNCTION: CMR2 0x004660e0
void Replay_SetControlStateByte(BYTE value)
{
    g_unk0x00588761 = value;
}

// FUNCTION: CMR2 0x004660f0
int Replay_GetSelectionStateByte(void)
{
    return g_unk0x00588760;
}

// FUNCTION: CMR2 0x0046b400
void CarDamage_SetModelEnabled(int enabled, int carIndex)
{
    g_carDamageModelEnabled[carIndex] = enabled;
}

// FUNCTION: CMR2 0x0046b420
void CarDamage_ClearModelEnabled(void)
{
    memset(g_carDamageModelEnabled, 0, 8 * sizeof(int));
}

// Reads vertex `vertex` of mesh `mesh` (float source data) as a 16.16 vector.
// FUNCTION: CMR2 0x0046b440
void Mesh_ReadVertexFixed(Mesh **ppMeshes, int mesh, int vertex, FixVector *pOut)
{
    float f;

    f = ((MeshVertexF *)ppMeshes[mesh]->pVertexData)[vertex].x;
    pOut->x = (int)FTOI(__int64, X87S_MUL(f, CGraphics::m_65536));
    f = ((MeshVertexF *)ppMeshes[mesh]->pVertexData)[vertex].y;
    pOut->y = (int)FTOI(__int64, X87S_MUL(f, CGraphics::m_65536));
    f = ((MeshVertexF *)ppMeshes[mesh]->pVertexData)[vertex].z;
    pOut->z = (int)FTOI(__int64, X87S_MUL(f, CGraphics::m_65536));
}


extern float g_oneOverRandMax;

// Flicker timer of a car's broken light: once the damage (+0x29c) passes
// half, the light alternates on and off (+0x4cc) for random spans, the off
// spans getting shorter and the on spans longer the heavier the damage.
// FUNCTION: CMR2 0x0046b4e0
void CarEffects_UpdateBrokenLightFlicker(Car *pCar)
{
    CarPartSet *pParts;
    int damageRate;

    pParts = CarPart_GetSet(pCar->index);
    if (pParts->damageValues[23] > 0x8000) {
        damageRate = FixMul(pParts->damageValues[23] - 0x8000, 0x20000);
        if (damageRate < 0)
            damageRate = 0;
        else if (damageRate > 0x10000)
            damageRate = 0x10000;
        damageRate = FixMul(damageRate, 0xcccc);
        if (pParts->brokenLightFlickerTimer <= 0) {
            if (pParts->brokenLightFlickerPhase != 0) {
                pParts->brokenLightFlickerPhase = 0;
                pParts->brokenLightFlickerTimer = FixMul(0x10000 - damageRate, FixMul((int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536)), 0xfa0000));
                pParts->brokenLightFlickerTimer += FixMul(0x10000 - damageRate, 0x320000);
            } else {
                pParts->brokenLightFlickerPhase = 1;
                pParts->brokenLightFlickerTimer = FixMul(damageRate, FixMul((int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536)), 0xf0000));
                pParts->brokenLightFlickerTimer += FixMul(damageRate, 0);
            }
        } else {
            pParts->brokenLightFlickerTimer -= 0x10000;
        }
    }
}

// FUNCTION: CMR2 0x0046b4c0
int CarDamage_GetModelEnabled(Car *pCar)
{
    return g_carDamageModelEnabled[pCar->index];
}

// FUNCTION: CMR2 0x0046b710
void StageObject_ResetPairedCarValues(void)
{
    int *p;
    p = &g_replayLevelState.levels[1];
    memset(g_carPairValuePending, 0, 8 * sizeof(int));
    do {
        p[-1] = 0;
        *p = 0;
        p += 2;
    } while ((INT_PTR)p < (INT_PTR)g_replayLevelState.pending);
}

// FUNCTION: CMR2 0x0046b740
void StageObject_SetPairedCarValue(int i, int value, int j)
{
    g_replayLevelState.levels[j + i * 2] = value;
}

// FUNCTION: CMR2 0x0046b760
void StageObject_SetCarValuePendingFlag(int index, int reset)
{
    if (reset != 0) {
        g_carPairValuePending[index] = 0;
        return;
    }
    g_carPairValuePending[index] = 1;
}

// FUNCTION: CMR2 0x0046bd20
int StageObject_GetPairedCarValue(int i, int j)
{
    return g_replayLevelState.levels[j + i * 2];
}

// FUNCTION: CMR2 0x0046bd40
BYTE StageObject_GetCarRecordState(int index)
{
    return g_unk0x00588ba4[index];
}

// Decodes one 4-byte replay input packet into the control record pOut and the
// car's handbrake/light switches; returns 1 when the packet's repeat count is
// used up.
// FUNCTION: CMR2 0x0046bec0
int Replay_DecodeInputPacket(ReplayStream *pState, BYTE *pIn, BYTE *pOut, BYTE *pCounter, Car *pCar)
{
    BYTE steer = pIn[3] & 0x3f;

    if ((pIn[3] & 0x40) != 0) {
        pOut[0] = steer;
        pOut[1] = 0;
    } else {
        pOut[0] = 0;
        pOut[1] = steer;
    }
    pOut[2] = pIn[0] >> 2;
    pOut[3] = pIn[1] >> 2;
    *(unsigned int *)(pOut + 8) = pIn[2] >> 7;
    pOut[4] = (pIn[1] & 3) - 1;
    *(unsigned int *)(pOut + 0xc) = (pIn[2] & 0x40) >> 6;
    if ((pIn[0] & 1) != 0)
        pCar->analogThrottle = 1;
    else
        pCar->analogThrottle = 0;
    if ((pIn[0] & 2) != 0)
        pCar->analogBrake = 1;
    else
        pCar->analogBrake = 0;
    if ((pIn[3] & 0x80) != 0) {
        if (pState->steeringModeOption != 0)
            pCar->steeringInputMode = 1;
        else
            pCar->steeringInputMode = 2;
    } else
        pCar->steeringInputMode = 0;
    if ((BYTE)++*pCounter < (pIn[2] & 0x3f))
        return 0;
    *pCounter = 0;
    return 1;
}

// FUNCTION: CMR2 0x0046bfb0
void Replay_CopyBlock309(Block0x309 *pSrc, Block0x309 *pDst)
{
    *pDst = *pSrc;
}

// Restores a car from a saved state record, keeping the destination's scene
// node bindings, its wheel emitter vectors and its timing index.
// FUNCTION: CMR2 0x0046bfd0
void Replay_RestoreCarState(Block0x309 *pSrc, Car *pDst)
{
    SceneNode *pSceneRoot = pDst->pSceneRoot;
    SceneNode *pBodyNode = pDst->pBodyNode;
    SceneNode *pAlternateBodyNode = pDst->pAlternateBodyNode;
    SceneNode *pExtraNodes[4];
    SceneNode *pWheelNodes[4];
    SceneNode *pViewNodeNear = pDst->pViewNodeNear;
    SceneNode *pViewNodeFar = pDst->pViewNodeFar;
    FixMatrix *pWorld = pDst->pWorld;
    FixMatrix *pBodyMatrix = pDst->pBodyMatrix;
    FixVector wheelEmitter[4];
    char carIndex = pDst->index;
    int i;

    for (i = 0; i < 4; i++) {
        pExtraNodes[i] = pDst->pExtraNodes[i];
        pWheelNodes[i] = pDst->pWheelNodes[i];
    }
    memcpy(wheelEmitter, pDst->wheelEmitter, sizeof(wheelEmitter));

    if (sizeof(Car) == g_diskLayout_Car.diskSize) {
        *(Block0x309 *)pDst = *pSrc;
    } else {
        for (int run = 0; run < g_diskLayout_Car.runCount; ++run) {
            const DiskRun &r = g_diskLayout_Car.runs[run];
            memcpy((BYTE *)pDst + r.native, (BYTE *)pSrc + r.disk, r.size);
        }
    }

    pDst->pSceneRoot = pSceneRoot;
    pDst->pBodyNode = pBodyNode;
    pDst->pAlternateBodyNode = pAlternateBodyNode;
    pDst->pViewNodeNear = pViewNodeNear;
    pDst->pViewNodeFar = pViewNodeFar;
    pDst->pWorld = pWorld;
    pDst->pBodyMatrix = pBodyMatrix;
    for (i = 0; i < 4; i++) {
        pDst->pExtraNodes[i] = pExtraNodes[i];
        pDst->pWheelNodes[i] = pWheelNodes[i];
        pDst->wheelEmitter[i] = wheelEmitter[i];
    }
    pDst->index = carIndex;

    if ((BYTE)RallyData_GetSelectionFlag28() != 0)
        pDst->simulationRateHz = 25.0f;
    if (pDst->index > 0 && (BYTE)RallyData_GetSelectionFlag27() != 0 &&
        (BYTE)CGameInfo::GetGameModeOptionBit19() == 0 && (BYTE)RallyDataState() == 1)
        pDst->simulationRateHz = StageTiming_GetSelectedStartTableFloat();
}

// FUNCTION: CMR2 0x0046c180
void Replay_CopyAuxState(Block0x134 *pSrc, Block0x134 *pDst)
{
    *pDst = *pSrc;
}

// Copies a 0x134-int car state record, keeping the destination's first three
// 15-int blocks.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046c1a0
void Replay_RestoreAuxState(Block0x134 *pSrc, Block0x134 *pDst)
{
    if (sizeof(CarPartSet) != g_diskLayout_CarPartSet.diskSize) {
        for (int run = 0; run < g_diskLayout_CarPartSet.runCount; ++run) {
            const DiskRun &r = g_diskLayout_CarPartSet.runs[run];
            memcpy((BYTE *)pDst + r.native, (BYTE *)pSrc + r.disk, r.size);
        }
        return;
    }

    int keepC[15];
    int keepB[15];
    int keepA[15];
    int i;

    memcpy(keepC, pDst->data + 30, sizeof(keepC));
    memcpy(keepB, pDst->data + 15, sizeof(keepB));
    memcpy(keepA, pDst->data, sizeof(keepA));
    *pDst = *pSrc;
    for (i = 0; i < 15; i++) {
        pDst->data[i] = keepA[i];
        pDst->data[15 + i] = keepB[i];
        pDst->data[30 + i] = keepC[i];
    }
}

// FUNCTION: CMR2 0x0046c220
void Replay_CopyBlock6(Block6 *pSrc, Block6 *pDst)
{
    *pDst = *pSrc;
}

CarPartSet *CarPart_GetSet(int carIndex);
struct RaceRecord;
RaceRecord *RallyData_GetCarRaceRecord(int index);
void RallyData_ResetRaceRecordAndRouteProbe(int index);

// Snapshots a car's state into the 0x1100-byte record at pDst.
// FUNCTION: CMR2 0x0046c240
void Replay_CaptureCarSnapshot(ReplayInputLane *pDst, BYTE car)
{
    Car *pCar = Car_Get(car);

    Disk_Pack(pDst->partState, CarPart_GetSet(car), &g_diskLayout_CarPartSet);
    Disk_Pack(pDst->carState, pCar, &g_diskLayout_Car);
    Replay_CopyBlock6((Block6 *)RallyData_GetCarRaceRecord(car), (Block6 *)pDst->raceRecord);
    RallyData_ResetRaceRecordAndRouteProbe(car);
}

// FUNCTION: CMR2 0x0046d2a0
int Replay_ResetBufferIfActive(ReplayStream *p)
{
    ReplayStream *stream = p;

    if (stream != NULL && stream->playing != 0) {
        stream->playing = 0;
        stream->playStarted = 0;
        stream->field_0x14 = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: CMR2 0x0046d500
BYTE *Replay_GetStreamFrameState(void)
{
    return g_unk0x00588d38;
}

// FUNCTION: CMR2 0x0046f4c0
void StageObject_GetSkyNode(SceneNode **pOut)
{
    *pOut = g_stageSkyNode;
}

// FUNCTION: CMR2 0x0046f4d0
void StageObject_GetGroundNode(SceneNode **pOut)
{
    *pOut = g_stageGroundNode;
}

// Per variant entry, index of its cloned mesh in g_movingObjectMeshes (0xff none).
// GLOBAL: CMR2 0x0058c928
BYTE *g_movingObjectMeshIndex;
// Meshes cloned for the moving objects (StageObject_LoadAndClassifyMeshes).
// GLOBAL: CMR2 0x0058c92c
Mesh **g_movingObjectMeshes;
// GLOBAL: CMR2 0x0058c930
int *g_movingObjectTypes;

// Releases the three files loaded by 0x46f550 (registered callback).
// FUNCTION: CMR2 0x0046f500
int StageObjects_ReleaseObjectFiles(void)
{
    if (g_movingObjectMeshIndex != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_movingObjectMeshIndex);
        g_movingObjectMeshIndex = NULL;
    }
    if (g_movingObjectTypes != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_movingObjectTypes);
        g_movingObjectTypes = NULL;
    }
    if (g_movingObjectMeshes != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_movingObjectMeshes);
        g_movingObjectMeshes = NULL;
    }
    g_movingObjects.meshCount = 0;
    return 1;
}

// FUNCTION: CMR2 0x0046f4e0
void StageObject_GetCloudNodes(SceneNode **pOut1, SceneNode **pOut2)
{
    *pOut1 = g_stageCloudNode;
    *pOut2 = g_stageCloudTopNode;
}

// GLOBAL: CMR2 0x0051c6d0
char g_strTempGro[] = "TEMP.GRO";
// GLOBAL: CMR2 0x0051c6dc
char g_strTopC3d[] = "top.c3d";
// GLOBAL: CMR2 0x0051c6e4
char g_strC3dExt[] = ".c3d";
// GLOBAL: CMR2 0x0051c6ec
char g_strBflExt[] = ".bfl";
// GLOBAL: CMR2 0x0051c6f4
char g_strTempSky[] = "TEMP.SKY";

void Graphics_SetRecordField2C(Mesh *p, int value);
void StageObject_BuildCloudTexturePath(void);
struct SceneNode *Sector_BuildC3DModelScene(void *data, struct SceneNode *parent, struct GenericFile *archive);
GenericFile *Race_GetLoadedStageFile(void);
SceneNode *RallyData_GetChallengeRenderState(void);
BYTE StageObject_ReleaseLoadedObjectFile(void);

// Loads the stage's sky and ground objects: the TEMP.SKY archive (also opened
// as .bfl, .c3d and top.c3d) and TEMP.GRO, releasing each one's meshes first.
// FUNCTION: CMR2 0x0046f060
void StageObjects_LoadSkyAndGround(void)
{
    char buffer[MAX_PATH];
    void *pFile;
    void *pC3d;
    SceneNode *node;
    Mesh *pMesh;

    pFile = CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), g_strTempSky, NULL, NULL, 0);
    if (pFile != NULL) {
        g_stageSkyNode = Sector_BuildC3DModelScene(pFile, RallyData_GetChallengeRenderState(),
                                           Race_GetLoadedStageFile());
        if (g_stageSkyNode != 0) {
            Graphics_SetRecordField2C((Mesh *)g_stageSkyNode->pObject, 0);
            g_stageSkyNode->visible = 0;
            node = g_stageSkyNode->pFirstChild;
            if (node != 0) {
                pMesh = (Mesh *)node->pObject;
                node->visible = 0;
                Graphics_SetRecordField2C(pMesh, 0);
                node = g_stageSkyNode->pFirstChild->pNext;
                if (node != 0) {
                    pMesh = (Mesh *)node->pObject;
                    node->visible = 0;
                    Graphics_SetRecordField2C(pMesh, 0);
                    node = g_stageSkyNode->pFirstChild->pNext->pNext;
                    if (node != 0) {
                        pMesh = (Mesh *)node->pObject;
                        node->visible = 0;
                        Graphics_SetRecordField2C(pMesh, 0);
                    }
                }
            }
        }
    }
    StageObject_BuildCloudTexturePath();
    strcpy(buffer, CFrontend::m_stringDest);
    strcpy(CFrontend::m_stringDest, buffer);
    strcat(CFrontend::m_stringDest, g_strBflExt);
    CGenericFileLoader::LoadIntoFileRecord(&g_stageCloudArchive, CFrontend::m_stringDest);
    strcpy(CFrontend::m_stringDest, buffer);
    strcat(CFrontend::m_stringDest, g_strC3dExt);
    pC3d = CGenericFileLoader::FindFile(&g_stageCloudArchive, CFrontend::m_stringDest, NULL, NULL, 0);
    strcpy(CFrontend::m_stringDest, buffer);
    strcat(CFrontend::m_stringDest, g_strTopC3d);
    pFile = CGenericFileLoader::FindFile(&g_stageCloudArchive, CFrontend::m_stringDest, NULL, NULL, 0);
    if (pFile != NULL) {
        g_stageCloudTopNode = Sector_BuildC3DModelScene(pFile, RallyData_GetChallengeRenderState(),
                                           &g_stageCloudArchive);
        if (g_stageCloudTopNode != 0) {
            pMesh = (Mesh *)g_stageCloudTopNode->pObject;
            g_stageCloudTopNode->visible = 0;
            Graphics_SetRecordField2C(pMesh, 0);
            g_stageCloudTopNode->viewMask = 0;
        }
    }
    if (pC3d != NULL) {
        g_stageCloudNode = Sector_BuildC3DModelScene(pC3d, RallyData_GetChallengeRenderState(),
                                           &g_stageCloudArchive);
        if (g_stageCloudNode != 0) {
            pMesh = (Mesh *)g_stageCloudNode->pObject;
            g_stageCloudNode->visible = 0;
            Graphics_SetRecordField2C(pMesh, 0);
        }
    }
    pFile = CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), g_strTempGro, NULL, NULL, 0);
    if (pFile != NULL) {
        g_stageGroundNode = Sector_BuildC3DModelScene(pFile, RallyData_GetChallengeRenderState(),
                                           Race_GetLoadedStageFile());
        if (g_stageGroundNode != 0) {
            pMesh = (Mesh *)g_stageGroundNode->pObject;
            g_stageGroundNode->visible = 0;
            Graphics_SetRecordField2C(pMesh, 0);
        }
    }
    CGame::RegisterCallback((void *)StageObject_ReleaseLoadedObjectFile, NULL);
}

SceneNode *RallyData_GetChallengeRenderState(void);
void Mesh_ResetCloneCount(void);
Mesh *Mesh_CloneInto(Mesh *pSrc, StageObject *pSource);
extern double g_minus65536;

// Loads the stage object list, creates a scene node and clones the mesh of
// every object, then classifies each bounding box as ground or wall.
// match 49%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The three bounding-box accumulators land in ESI/EDX/ECX instead of the
// original's EDI/ESI/EDX and MSVC merges the "if (v < 0)" phi without the
// original's extra jmp; the code itself is identical.
// FUNCTION: CMR2 0x0046f550
void StageObject_LoadAndClassifyMeshes(void)
{
    SectorObjectEntry *pEntries;
    int i;
    int count;
    int maxX;
    int maxY;
    int maxZ;
    Mesh *mesh;
    float *pFloats;

    Mesh_ResetCloneCount();
    for (i = 0; i < 40; i++) {
        SceneNode *pNode = SceneNode_Create(RallyData_GetChallengeRenderState());
        g_movingObjects.entries[i].pObject = pNode;
        pNode->type = SCENE_NODE_EMPTY;
        g_movingObjects.entries[i].actionState = -0x10000;
    }

    count = StageObject_GetLoadedVariantDataAndState(&pEntries);
    g_movingObjectMeshes = (Mesh **)CFileBuffer::AllocateLockedBuffer(count * sizeof(*g_movingObjectMeshes));
    i = 0;
    if (count > 0) {
        do {
            i++;
            g_movingObjectMeshes[i - 1] = 0;
        } while (i < count);
    }
    g_movingObjectMeshIndex = 0;
    if (count >= 1) {
        g_movingObjectMeshIndex = (BYTE *)CFileBuffer::AllocateLockedBuffer(count);
        i = 0;
        g_movingObjects.meshCount = 0;
        if (count > 0) {
            do {
                mesh = pEntries[i].pObject->pMesh;
                if ((int)g_movingObjects.meshCount < count) {
                    g_movingObjectMeshIndex[i] = (BYTE)g_movingObjects.meshCount;
                    mesh = Mesh_CloneInto(mesh, pEntries[i].pObject);
                    g_movingObjectMeshes[(BYTE)g_movingObjects.meshCount] = mesh;
                    if (g_movingObjectMeshes[(BYTE)g_movingObjects.meshCount] == 0)
                        g_movingObjectMeshIndex[i] = 0;
                    else
                        g_movingObjects.meshCount++;
                } else {
                    g_movingObjectMeshIndex[i] = 0xff;
                }
                i++;
            } while (i < count);
        }
    }
    g_movingObjectTypes = 0;
    if (g_movingObjects.meshCount > 0)
        g_movingObjectTypes = (int *)CFileBuffer::AllocateLockedBuffer((g_movingObjects.meshCount & 0xff) * sizeof(*g_movingObjectTypes));
    i = 0;
    if (g_movingObjects.meshCount > 0) {
        do {
            Mesh *pObject = g_movingObjectMeshes[i];
            int n;
            int x;
            int y;
            int z;
            maxY = 0;
            maxX = 0;
            n = pObject->field_0x10;
            maxZ = 0;
            if (n >= 1) {
                pFloats = (float *)pObject->pVertexData;
                do {
                    x = (int)FTOI(__int64, X87S_MUL(pFloats[0], CGraphics::m_65536));
                    if (x < 0)
                        x = (int)FTOI(__int64, X87S_MUL(pFloats[0], g_minus65536));
                    y = (int)FTOI(__int64, X87S_MUL(pFloats[1], CGraphics::m_65536));
                    if (y < 0)
                        y = (int)FTOI(__int64, X87S_MUL(pFloats[1], g_minus65536));
                    z = (int)FTOI(__int64, X87S_MUL(pFloats[2], CGraphics::m_65536));
                    if (z < 0)
                        z = (int)FTOI(__int64, X87S_MUL(pFloats[2], g_minus65536));
                    if (x > maxX)
                        maxX = x;
                    if (y > maxY)
                        maxY = y;
                    if (z > maxZ)
                        maxZ = z;
                    pFloats += 0xc;
                    n--;
                } while (n != 0);
            }
            if (FixDiv(maxZ, maxX) < 0x4ccc)
                g_movingObjectTypes[i] = 0;
            else if (FixDiv(maxX, maxY) < 0x4ccc)
                g_movingObjectTypes[i] = 1;
            else
                g_movingObjectTypes[i] = 0;
            i++;
        } while (i < (int)(g_movingObjects.meshCount & 0xff));
    }
    CGame::RegisterCallback((void *)StageObjects_ReleaseObjectFiles, NULL);
}

// FUNCTION: CMR2 0x00471bd0
unsigned int StageObject_GetLoadedVariantDataAndState(SectorObjectEntry **pOut)
{
    *pOut = g_movingSectorEntries;
    return g_unk0x0058ca6c;
}

// FUNCTION: CMR2 0x004728b0
void Knockout_SetRoundActiveFlag(void)
{
    g_knockoutRoundActive = 1;
}

// FUNCTION: CMR2 0x004728c0
void Knockout_ClearRoundActiveFlag(void)
{
    g_knockoutRoundActive = 0;
}

// FUNCTION: CMR2 0x004728d0
int Knockout_GetRoundActiveFlag(void)
{
    return g_knockoutRoundActive;
}

// Returns 1 when a match of the current knockout round is undecided or one of
// its drivers is not in the table.
// FUNCTION: CMR2 0x004728e0
int Knockout_HasPendingOrUnknownRoundMatch(void)
{
    unsigned int *pState;
    KnockoutMatch *pMatch = NULL;
    int count = 0;
    int i;

    pState = RallyData_GetChampionshipState();
    switch ((*pState >> 3) & 7) {
    case 1:
        count = 8;
        pMatch = (KnockoutMatch *)(pState + 0x16);
        break;
    case 2:
        count = 4;
        pMatch = (KnockoutMatch *)(pState + 10);
        break;
    case 3:
        count = 2;
        pMatch = (KnockoutMatch *)(pState + 4);
        break;
    case 4:
        count = 1;
        pMatch = (KnockoutMatch *)(pState + 1);
    }
    for (i = 0; i < count; i++, pMatch++) {
        if ((RallyData_GetUsableRecordCategory((BYTE)(pMatch->flags & 0x1f)) == -1 && (pMatch->flags & 0x400) == 0) ||
            (RallyData_GetUsableRecordCategory((BYTE)((pMatch->flags >> 5) & 0x1f)) == -1 && (pMatch->flags & 0x400) == 0))
            return 1;
        if (Knockout_HasHumanLostMatch(pMatch))
            return 1;
    }
    return 0;
}

// Whether the human player lost the knockout match (the winner is not a human
// driver): bit 11 means the second driver won, bit 12 the first one.
// FUNCTION: CMR2 0x00472990
int Knockout_HasHumanLostMatch(KnockoutMatch *pMatch)
{
    if (RallyData_GetUsableRecordCategory(pMatch->flags & 0x1f) == -1 && (pMatch->flags & 0x1800) == 0x800)
        return 1;
    if (RallyData_GetUsableRecordCategory((pMatch->flags >> 5) & 0x1f) == -1 && (pMatch->flags & 0x1800) == 0x1000)
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x00472ca0
void Knockout_ResetRaceStateMachine(void)
{
    g_knockoutRaceState = 0;
}

// Sets the ground material of the two cars from the current championship
// state (the knockout bracket entry selected by the state flags).
// FUNCTION: CMR2 0x00472cb0
void Knockout_ApplyCarGroundMaterials(void)
{
    unsigned int *pState = RallyData_GetChampionshipState();
    BYTE value;
    int i;

    switch ((*pState >> 3) & 7) {
    case 1:
        if (RallyData_GetUsableRecordCategory((BYTE)pState[((*pState >> 0xc) & 0xf) * 3 + 0x16] & 0x1f) == -1 &&
            RallyData_GetUsableRecordCategory((BYTE)(pState[((*pState >> 0xc) & 0xf) * 3 + 0x16] >> 5) & 0x1f) == -1)
            goto fail;
        goto ok;
    case 2:
        if (RallyData_GetUsableRecordCategory((BYTE)pState[((*pState >> 0xc) & 0xf) * 3 + 10] & 0x1f) == -1 &&
            RallyData_GetUsableRecordCategory((BYTE)(pState[((*pState >> 0xc) & 0xf) * 3 + 10] >> 5) & 0x1f) == -1)
            goto fail;
    ok:
        value = 1;
        break;
    case 3:
        if (RallyData_GetUsableRecordCategory((BYTE)pState[((*pState >> 0xc) & 0xf) * 3 + 4] & 0x1f) == -1 &&
            RallyData_GetUsableRecordCategory((BYTE)(pState[((*pState >> 0xc) & 0xf) * 3 + 4] >> 5) & 0x1f) == -1)
            goto fail;
        goto ok;
    case 4:
        if (RallyData_GetUsableRecordCategory((BYTE)pState[1] & 0x1f) == -1 &&
            RallyData_GetUsableRecordCategory((BYTE)(pState[1] >> 5) & 0x1f) == -1)
            goto fail;
        goto ok;
    default:
    fail:
        value = 2;
        break;
    }
    RallyData_SetSelectionBits14To15(value);
    for (i = 0; i < 2; i++) {
        Car_SetDrawnFlag(i, (BYTE)Input_GetControllerSlotMapping((unsigned short)i));
        if (i >= (int)(RallyDataState() & 0xff))
            Car_SetDrawnFlag(i, -1);
    }
}

// Stage fade timers and race-state flags of the in-race state machine
// (0x472e00). Each "*" object is registered with the timer manager and its
// first byte is the timer slot.
// Magic value handed to NetRace_FadeOutPlayerScreen as opaque data, not an address.
// GLOBAL: CMR2 0x0051c9a8
int g_unk0x0051c9a8 = 0xacb49c;
// GLOBAL: CMR2 0x0051c9ac
char g_strVs0x0051c9ac[] = "vs.";
// GLOBAL: CMR2 0x0058ca80
BYTE g_unk0x0058ca80;
// GLOBAL: CMR2 0x0058ca84
int g_unk0x0058ca84;
// The five fade-timer objects below live inside the 0x58ca90 block declared in
// StageUI.cpp, so they are addressed as offsets of it (a separate declaration
// would overlap it).
extern PauseMenus g_pauseMenus;

extern CallbackStateMachine *g_unk0x0058ca88;
extern BYTE g_unk0x0058ca8c[4];
extern int g_unk0x0058cf64;
extern int g_knockoutRaceState;

typedef void (*FadeCallback)(BYTE index);

extern unsigned int g_unk0x0058cf74;
extern unsigned int g_unk0x0058cf78;
extern int g_unk0x0058cf70;

void Surface_StopAndFreeSounds(void);
void Race_StopAllStageSounds(void);
void Race_FreePerCarSounds(void);
void NetRace_UpdatePlayerFlashTimer(unsigned int player, int check);
void NetRace_SetPlayerFlashIntensity(unsigned int player, int t, int check);
void StageUI_UpdatePauseMenu(void);
BYTE Graphics_IsRegisteredTimerRunning(BYTE *p);
int Timer_GetValue(BYTE index);
void Graphics_StartShapedInterpolationTimer(BYTE *p, int, int, int, int, int, BYTE);
void Graphics_ResetInterpolationTimers(void);
void Graphics_StopRegisteredTimer(BYTE *p);
unsigned int *RallyData_GetRoundEntry(void);
int NetRace_IsPlayerFadeActive(BYTE index);
int Knockout_HasUnknownMatchDriver(unsigned int *p);
int Knockout_AreCurrentDriversKnown(void);
int Knockout_HasUndecidedRoundMatch(void);
void StageUI_SetRaceEndPending(void);
int StageUI_GetRaceEndState(void);
void StageTiming_InitRaceDriverRecords(char);
void Knockout_RecordCurrentMatchResults(void);
BOOL Replay_ReleaseStageBuffers(void);
void Replay_InitRaceSlots(void);
void NetRace_FadeOutPlayerScreen(BYTE index, FadeCallback pfnDone, int param3, int param4, int param5, char force);
void Race_SetFlag3810D(void);
void RallyData_SelectKnockoutCountryStage(void);
void StageUI_BuildPauseMenus(void);
struct Menu;
void StageUI_RetireFromRace(Menu *pMenu);
void StageUI_ReleaseSceneAndFadeStage(BYTE index);

// Per-frame step of the in-race state machine: waits for the stage objects of
// the round, runs the state transitions and drives the fade in/out of the
// scene.
// FUNCTION: CMR2 0x00472e00
void Knockout_UpdateRaceStateAndFades(CallbackStateMachine *param_1, unsigned int param_2)
{
    unsigned int *pState;
    unsigned int *pEntry;
    unsigned int state;
    int i;
    pState = RallyData_GetChampionshipState();
    Surface_StopAndFreeSounds();
    Race_StopAllStageSounds();
    Race_FreePerCarSounds();
    g_unk0x0058ca88 = param_1;
    g_unk0x0058ca84 = param_2 & 0xff;
    for (i = 0; i < 2; i++) {
        NetRace_UpdatePlayerFlashTimer(i, 0);
        NetRace_SetPlayerFlashIntensity(i, 0x10000, 0);
    }
    if (NetRace_IsPlayerFadeActive(0) != 0)
        return;
    if (g_unk0x0058cf6c != 0)
        StageUI_UpdatePauseMenu();
    if (Graphics_IsRegisteredTimerRunning(&g_unk0x0058ca80))
        g_pauseMenus.bracketFade = Timer_GetValue(g_unk0x0058ca80);
    if (Graphics_IsRegisteredTimerRunning(&g_pauseMenus.textTimer))
        g_pauseMenus.textFade = Timer_GetValue(g_pauseMenus.textTimer);
    else
        g_pauseMenus.textFade = 0;

    switch (g_knockoutRaceState) {
    case 0:
        Graphics_ResetInterpolationTimers();
        g_pauseMenus.bracketFade = 0;
        if ((*pState & 0x38) == 8) {
            Graphics_StartShapedInterpolationTimer(&g_unk0x0058ca80, 2, 0xd, 0, 0, 0x10000, 0);
            Graphics_StartShapedInterpolationTimer(&g_pauseMenus.textTimer, 2, 7, 0, 0, 0x10000, 0);
        }
        *pState = *pState & 0xff1fffff;
        StageUI_BuildPauseMenus();
        g_knockoutRaceState = 1;
        for (i = 0; i < 2; i++) {
            if (i >= (int)(RallyDataState() & 0xff))
                Car_SetDrawnFlag(i, -1);
        }
        g_unk0x0058cf64 = 0;
        return;
    case 1:
        if (g_unk0x0058cf64 != 0) {
            StageTiming_InitRaceDriverRecords(1);
            g_knockoutRaceState = 2;
            g_unk0x0058cf64 = 0;
            return;
        }
        break;
    case 2:
        pEntry = RallyData_GetRoundEntry();
        if (Knockout_HasUnknownMatchDriver(pEntry) != 0) {
            g_knockoutRaceState = 5;
            Knockout_RecordCurrentMatchResults();
            g_unk0x0058cf78 = 0;
            g_unk0x0058cf74 = 0;
        }
        if (g_unk0x0058cf64 != 0) {
            Knockout_ApplyCarGroundMaterials();
            Replay_ReleaseStageBuffers();
            Replay_InitRaceSlots();
            if (Knockout_AreCurrentDriversKnown() != 0) {
                g_knockoutRaceState = 3;
                Knockout_RecordCurrentMatchResults();
                g_unk0x0058cf78 = 0;
                g_unk0x0058cf74 = 0;
                g_unk0x0058cf64 = 0;
                return;
            } else {
                g_pauseMenus.roundEntry = RallyData_GetRoundEntry();
                g_unk0x0058cf64 = 0;
                g_knockoutRaceState = 4;
                return;
            }
        }
        break;
    case 3:
        if (g_unk0x0058cf64 != 0) {
            g_knockoutRaceState = 5;
            Graphics_StartShapedInterpolationTimer(&g_pauseMenus.textTimer, 2, 7, 0, 0, 0x10000, 0);
            g_unk0x0058cf64 = 0;
            return;
        }
        break;
    case 4:
        if (StageUI_GetRaceEndState() == 0) {
            StageUI_SetRaceEndPending();
            g_unk0x0058cf64 = 0;
            return;
        }
        NetRace_FadeOutPlayerScreen(0, (FadeCallback)StageUI_ReleaseSceneAndFadeStage, 1, 0, g_unk0x0051c9a8, 0);
        if ((char)RallyDataState() == 2)
            NetRace_FadeOutPlayerScreen(1, NULL, 1, 0, g_unk0x0051c9a8, 0);
        break;
    case 5:
        if ((*pState & 0x400000) == 0) {
            g_unk0x0058cf64 = 0;
            g_knockoutRaceState = 2;
            return;
        }
        if ((*pState & 0x38) == 0x20) {
            if (g_unk0x0058cf64 != 0) {
                g_unk0x0058cf64 = 0;
                g_knockoutRaceState = 7;
                return;
            }
        } else if (g_unk0x0058cf64 != 0) {
            g_pauseMenus.bracketFade = 0x10000;
            Graphics_StartShapedInterpolationTimer(&g_pauseMenus.bracketTimer, 2, 0xd, 0, 0, 0x10000, 0);
            g_unk0x0058cf64 = 0;
            g_knockoutRaceState = 6;
            return;
        }
        break;
    case 6:
        if (Graphics_IsRegisteredTimerRunning(&g_pauseMenus.bracketTimer)) {
            g_pauseMenus.bracketFade = Timer_GetValue(g_pauseMenus.bracketTimer);
            g_unk0x0058cf64 = 0;
            return;
        }
        if (!Graphics_IsRegisteredTimerRunning(g_unk0x0058ca8c)) {
            Graphics_StopRegisteredTimer(&g_pauseMenus.bracketTimer);
            g_pauseMenus.bracketFade = 0;
            Graphics_StartShapedInterpolationTimer(g_unk0x0058ca8c, 2, 0xd, 0, 0, 0x10000, 1);
            g_unk0x0058cf64 = 0;
            return;
        }
        g_pauseMenus.bracketFade = Timer_GetValue(g_unk0x0058ca8c[0]);
        if (!Graphics_IsRegisteredTimerRunning(g_unk0x0058ca8c))
            g_pauseMenus.bracketFade = 0;
        if (g_unk0x0058cf64 != 0 || !Graphics_IsRegisteredTimerRunning(g_unk0x0058ca8c)) {
            Graphics_ResetInterpolationTimers();
            g_pauseMenus.bracketFade = 0;
            g_pauseMenus.textFade = 0;
            g_unk0x0058cf70 = Knockout_HasUndecidedRoundMatch();
            state = *pState;
            *pState = (((state & 0xfffffff8) + 8 ^ state) & 0x38 ^ state) & 0xffbf0fff | 0x200000;
            RallyData_SelectKnockoutCountryStage();
            Race_SetFlag3810D();
            i = 0;
            if (g_unk0x0058ca88->count > 0) {
                do {
                    CGame::PromoteCallbackEntryByRule(g_unk0x0058ca88, i, 1, 2);
                    i++;
                } while (i < (int)g_unk0x0058ca88->count);
            }
            g_knockoutRaceState = 0;
            g_unk0x0058cf64 = 0;
            return;
        }
        break;
    case 7:
        NetRace_FadeOutPlayerScreen(0, (FadeCallback)StageUI_RetireFromRace, 1, 0, g_unk0x0051c9a8, 0);
        if ((char)RallyDataState() == 2)
            NetRace_FadeOutPlayerScreen(1, NULL, 1, 0, g_unk0x0051c9a8, 0);
        break;
    }
    g_unk0x0058cf64 = 0;
}

// FUNCTION: CMR2 0x00473680
int Knockout_HasUnknownMatchDriver(unsigned int *p)
{
    if ((*p & 0x1f) != 0x1f && (*p & 0x3e0) != 0x3e0)
        return 0;
    return 1;
}

int *RallyData_GetDriverNameIndexRecord(int index);

// Name of the driver on the given side (0 first, 1 second) of a knockout
// match: the player's name, the AI name, or "" for an empty slot.
// match 55%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004736b0
char *Knockout_GetDriverNameForSide(KnockoutMatch *pMatch, int side)
{
    if (side == 0) {
        if ((pMatch->flags & 0x1f) == 0x1f)
            return CMain::m_logFileBlankLine;
        if (RallyData_GetUsableRecordCategory(pMatch->flags & 0x1f) != -1) {
            sprintf(CFrontend::m_stringDest, "%s", CAIHelper::GetNameForID(RallyData_GetUsableRecordCategory(pMatch->flags & 0x1f)));
        } else {
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord(pMatch->flags & 0x1f));
        }
    } else if (side == 1) {
        if ((pMatch->flags & 0x3e0) == 0x3e0)
            return CMain::m_logFileBlankLine;
        if (RallyData_GetUsableRecordCategory((pMatch->flags >> 5) & 0x1f) != -1) {
            sprintf(CFrontend::m_stringDest, "%s", CAIHelper::GetNameForID(RallyData_GetUsableRecordCategory((pMatch->flags >> 5) & 0x1f)));
        } else {
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord((pMatch->flags >> 5) & 0x1f));
        }
    }
    return CFrontend::m_stringDest;
}

// Whether the given side of the match is the human player.
// FUNCTION: CMR2 0x00473790
int Knockout_IsHumanMatchSide(KnockoutMatch *pMatch, int side)
{
    unsigned int driver;

    if (side == 0)
        driver = pMatch->flags;
    else
        driver = pMatch->flags >> 5;
    driver &= 0x1f;
    if (driver == 0x1f)
        return 0;
    return RallyData_GetUsableRecordCategory(driver) == -1;
}

// Side of the match to show: the human player's side when it is param2,
// otherwise the other side.
// FUNCTION: CMR2 0x004737d0
int Knockout_SelectDisplaySide(KnockoutMatch *pMatch, int param2)
{
    int slot = pMatch->flags & 0x1f;

    if (slot != 0x1f && RallyData_GetUsableRecordCategory(slot) == -1)
        return param2 != 0;
    return param2 == 0;
}

// Same as Knockout_GetDriverNameForSide with the car names of the AI drivers.
// match 55%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00473810
char *Knockout_GetCarNameForSide(KnockoutMatch *pMatch, int side)
{
    if (side == 0) {
        if ((pMatch->flags & 0x1f) == 0x1f)
            return CMain::m_logFileBlankLine;
        if (RallyData_GetUsableRecordCategory(pMatch->flags & 0x1f) != -1) {
            sprintf(CFrontend::m_stringDest, "%s", (char *)RallyData_GetDriverNameIndexRecord(RallyData_GetUsableRecordCategory(pMatch->flags & 0x1f)));
        } else {
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord(pMatch->flags & 0x1f));
        }
    } else if (side == 1) {
        if ((pMatch->flags & 0x3e0) == 0x3e0)
            return CMain::m_logFileBlankLine;
        if (RallyData_GetUsableRecordCategory((pMatch->flags >> 5) & 0x1f) != -1) {
            sprintf(CFrontend::m_stringDest, "%s", (char *)RallyData_GetDriverNameIndexRecord(RallyData_GetUsableRecordCategory((pMatch->flags >> 5) & 0x1f)));
        } else {
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord((pMatch->flags >> 5) & 0x1f));
        }
    }
    return CFrontend::m_stringDest;
}

extern int g_unk0x0058cf64;
// GLOBAL: CMR2 0x0058cf74
unsigned int g_unk0x0058cf74;
// GLOBAL: CMR2 0x0058cf78
unsigned int g_unk0x0058cf78;

extern char g_minSecMSECFormatString[];

// Formats one of the two lap time fields as "%02d:%02d.%02d", printing the
// identical placeholder while the value eases towards its target.
// match 68%: same logic; MSVC keeps the two eased values in different
// registers and spills one extra
// match 68%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004752f0
char *StageObject_FormatAnimatedLapTime(int *p, int index, int mode)
{
    unsigned int current1;
    unsigned int current2;
    int time1;
    int time2;

    time1 = p[1];
    time2 = p[2];
    if (mode != 0) {
        current1 = g_unk0x0058cf74;
        if (current1 <= (unsigned)(time1 - 0x27))
            current1 += 0x27;
        else
            current1 = time1;
        g_unk0x0058cf74 = current1;
        time2 = p[2];
        current2 = g_unk0x0058cf78;
        if (current2 <= (unsigned)(time2 - 0x27))
            current2 += 0x27;
        else
            current2 = time2;
        g_unk0x0058cf78 = current2;
        if (current1 == (unsigned)p[1] && current2 == (unsigned)p[2])
            g_unk0x0058cf64 = 1;
        time1 = current1;
        time2 = current2;
    }
    if (index == 0) {
        if ((p[0] & 0x1f) == 0x1f)
            return CMain::m_logFileBlankLine;
        RallyData_GetUsableRecordCategory((BYTE)(p[0] & 0x1f));
        sprintf(CFrontend::m_stringDest, g_minSecMSECFormatString, time1 / 6000, (time1 % 6000) / 100,
                time1 % 100);
    } else if (index == 1) {
        if ((p[0] & 0x3e0) == 0x3e0)
            return CMain::m_logFileBlankLine;
        RallyData_GetUsableRecordCategory((BYTE)(((unsigned int)p[0] >> 5) & 0x1f));
        sprintf(CFrontend::m_stringDest, g_minSecMSECFormatString, time2 / 6000, (time2 % 6000) / 100,
                time2 % 100);
    }
    return CFrontend::m_stringDest;
}

// Builds the in-race menu of two items (returned by StageObject_GetInRaceActionMenu); both
// actions are forwarded to CGame (0x49c070 / 0x49c080).
void Game_NoOpInRaceMenuItemEvent(Menu *pMenu, int param);
void Game_SetRaceExitFlags(Menu *pMenu, int param);
void Game_DrawInRaceActionMenu(Menu *pMenu);
// FUNCTION: CMR2 0x00475f00
void StageObject_BuildInRaceActionMenu(void)
{
    Menu_Init(&g_inRaceActionMenu, 0, -1, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_inRaceActionMenu, 0, 0xf4, (MenuItemAction)Game_NoOpInRaceMenuItemEvent, -1);
    Menu_AddItemType4(&g_inRaceActionMenu, 0, 0xf5, (MenuItemAction)Game_SetRaceExitFlags, -1);
    Menu_SetCallbacks(&g_inRaceActionMenu, NULL, NULL, (MenuCallback)Game_DrawInRaceActionMenu, NULL);
    Menu_ValidateCursor(&g_inRaceActionMenu, 0);
}

// FUNCTION: CMR2 0x00475f70
Menu *StageObject_GetInRaceActionMenu(void)
{
    return &g_inRaceActionMenu;
}

// Updates one stage object's state byte and re-syncs its scene node with the
// given source matrix; when the node ends up in another sector it is detached
// and released from the scene again.
// FUNCTION: CMR2 0x00476410
void StageObject_SyncStateAndSceneMatrix(CameraRecord *p, int *src, int unused, BYTE value)
{
    int index;
    FixVector pos;

    g_carInteriorModelClasses[p->index] = value;
    StageObject_BuildCarNodeOrientation(p, src);
    if (g_carInteriorRoots[p->car] != NULL) {
        SceneNode_Unused(g_carInteriorRoots[p->car]);
        pos.x = (g_carInteriorRoots[p->car])->world.position.x;
        pos.y = (g_carInteriorRoots[p->car])->world.position.y;
        pos.z = (g_carInteriorRoots[p->car])->world.position.z;
        index = (short)Sector_FromPosition(&pos);
        if (index != (g_carInteriorRoots[p->car])->sector) {
            Sector_InsertNodeByPosition(g_carInteriorRoots[p->car]);
            SceneNode_Unused(g_carInteriorRoots[p->car]);
        }
    }
}

// FUNCTION: CMR2 0x004764e0
void CarInterior_ResetDriverPoseOnNextUpdate(CameraRecord *pRecord)
{
    g_carDriverPoseNeedsReset[pRecord->car] = 1;
}

// FUNCTION: CMR2 0x00476520
int CarInterior_IsLoaded(BYTE index)
{
    return g_carInteriorLoaded[index];
}

// Caches the CIN cockpit profile and the class-dependent wiper direction.
// FUNCTION: CMR2 0x00476540
void CarInterior_CacheProfilePointers(int index)
{
    Car *pCar = Car_Get(index);
    char type = (BYTE)pCar->type;
    BYTE *p;

    if (type == 8 || type == 7 || type == 9 || type == 13)
        g_carWiperDirectionReversed[index] = 1;
    else
        g_carWiperDirectionReversed[index] = 0;
    // Preserve the original advancing cursor; the member gives its bias.
#define INTERIOR_AT(member) ((CarInteriorProfile *)(p - offsetof(CarInteriorProfile, member)))
    p = (BYTE *)CarInfo_GetSection(pCar, CAR_INFO_INTERIOR);
    g_carInteriorProfilePointers[index].firstWiperRotation = &INTERIOR_AT(firstWiperRotation)->firstWiperRotation;
    p += offsetof(CarInteriorProfile, firstWiperMaxAngle) - offsetof(CarInteriorProfile, firstWiperRotation);
    g_carInteriorProfilePointers[index].firstWiperMaxAngle = &INTERIOR_AT(firstWiperMaxAngle)->firstWiperMaxAngle;
    p += offsetof(CarInteriorProfile, driverPoseOffset) - offsetof(CarInteriorProfile, firstWiperMaxAngle);
    g_carInteriorProfilePointers[index].driverPoseOffset = &INTERIOR_AT(driverPoseOffset)->driverPoseOffset;
    p += offsetof(CarInteriorProfile, driverRestYaw) - offsetof(CarInteriorProfile, driverPoseOffset);
    g_carInteriorProfilePointers[index].driverRestYaw = &INTERIOR_AT(driverRestYaw)->driverRestYaw;
    p += offsetof(CarInteriorProfile, steeringBaseRotation) - offsetof(CarInteriorProfile, driverRestYaw);
    g_carInteriorProfilePointers[index].steeringBaseRotation = &INTERIOR_AT(steeringBaseRotation)->steeringBaseRotation;
    p += offsetof(CarInteriorProfile, secondWiperRotation) - offsetof(CarInteriorProfile, steeringBaseRotation);
    g_carInteriorProfilePointers[index].secondWiperRotation = &INTERIOR_AT(secondWiperRotation)->secondWiperRotation;
    p += offsetof(CarInteriorProfile, secondWiperMaxAngle) - offsetof(CarInteriorProfile, secondWiperRotation);
    g_carInteriorProfilePointers[index].secondWiperMaxAngle = &INTERIOR_AT(secondWiperMaxAngle)->secondWiperMaxAngle;
#undef INTERIOR_AT
}

// Draws a stage box: a filled rectangle, its one pixel outline and, optionally,
// the championship round box texture scaled to the rectangle.
// FUNCTION: CMR2 0x00475740
void StageObject_DrawOutlinedStageBox(short *pRect, BYTE *pColour, BYTE *pEdgeColour, int drawTexture)
{
    short edge[4];
    SpriteRect dest;

    if (pColour != NULL)
        Sprite_FillRect(&g_pGraphics->field309_0x150, pRect, pColour, 2);
    if (pEdgeColour != NULL) {
        edge[0] = pRect[0];
        edge[1] = pRect[1];
        edge[2] = pRect[2];
        edge[3] = 1;
        Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pEdgeColour, 2);
        edge[0] = (short)(pRect[0] + pRect[2]);
        edge[1] = pRect[1];
        edge[2] = 1;
        edge[3] = (short)(pRect[3] + 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pEdgeColour, 2);
        edge[0] = pRect[0];
        edge[1] = (short)(pRect[1] + pRect[3]);
        edge[2] = pRect[2];
        edge[3] = 1;
        Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pEdgeColour, 2);
        edge[0] = pRect[0];
        edge[1] = pRect[1];
        edge[2] = 1;
        edge[3] = pRect[3];
        Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pEdgeColour, 2);
    }
    if (drawTexture != 0) {
        dest.x = pRect[0];
        dest.y = pRect[1];
        // The texture quad is scaled to the rectangle and to the reference
        // resolution (640x480) it was authored for.
        dest.w = (short)(((int)InRaceMenu_GetRoundBoxTexture()->width * (int)pRect[2] * (int)g_pGraphics->resX / 0x280) /
                          ((int)g_pGraphics->resX * 0x55 / 0x280));
        dest.h = (short)(((int)InRaceMenu_GetRoundBoxTexture()->height * (int)pRect[3] * (int)g_pGraphics->resY / 0x1e0) /
                          ((int)g_pGraphics->resY * 0x26 / 0x1e0));
        Sprite_Queue((SpriteRect *)&InRaceMenu_GetRoundBoxTexture()->field_0x11c, &dest, InRaceMenu_GetRoundBoxTexture(), 2, 0, 0, 0,
                     pEdgeColour, 8);
    }
}

// The original callers pass the 12-bit angle in a 16-bit slot without sign
// extension (the definition reads a 32-bit int and masks it to 0xfff).
#define FromAxisAngle16(pOut, pAxis, angle) \
    (((void (__stdcall *)(FixMatrix *, FixVector *, short))FixMatrix_FromAxisAngle)((pOut), (pAxis), (angle)))

// Blends a car's stage object transform: rotates its mount nodes by the car
// heading and interpolates the blended node's matrix between the reference
// node and the rotated mount using the body fade factor.
// FUNCTION: CMR2 0x00476640
void StageObject_BlendCarMountTransform(int car)
{
    // Per car fade curve (16.16), sampled with index = 12 * fade.
    int fadeCurve[13] = {0, 0x51e, 0xccc, 0x1999, 0x3333, 0x6666, 0x9999, 0xcccc,
                         0xe666, 0xf333, 0xfae1, 0x10000, 0x10000};
    Car *pCar = Car_Get(car);
    short angle = -pCar->wheelSteeringAngle;
    FixVector axis;
    FixMatrix rot;
    FixMatrix combined;
    FixMatrix original;
    FixVector position;
    int fade;

    SceneNode_SetRotation(g_carInteriorNodeRows[car].steeringWheelNode, g_carInteriorProfilePointers[car].steeringBaseRotation);
    axis.x = g_carInteriorNodeRows[car].steeringWheelNode->current.right.x;
    axis.y = g_carInteriorNodeRows[car].steeringWheelNode->current.right.y;
    axis.z = g_carInteriorNodeRows[car].steeringWheelNode->current.right.z;
    FromAxisAngle16(&rot, &axis, angle);

    // Rotate the node without touching its translation.
    position.x = g_carInteriorNodeRows[car].steeringWheelNode->current.position.x;
    position.y = g_carInteriorNodeRows[car].steeringWheelNode->current.position.y;
    position.z = g_carInteriorNodeRows[car].steeringWheelNode->current.position.z;
    g_carInteriorNodeRows[car].steeringWheelNode->current.position.x = 0;
    g_carInteriorNodeRows[car].steeringWheelNode->current.position.y = 0;
    g_carInteriorNodeRows[car].steeringWheelNode->current.position.z = 0;
    FixMatrix_Multiply(&g_carInteriorNodeRows[car].steeringWheelNode->current, &g_carInteriorNodeRows[car].steeringWheelNode->current, &rot);
    g_carInteriorNodeRows[car].steeringWheelNode->current.position.x = position.x;
    g_carInteriorNodeRows[car].steeringWheelNode->current.position.y = position.y;
    g_carInteriorNodeRows[car].steeringWheelNode->current.position.z = position.z;

    SceneNode_SetRotation(g_carInteriorNodeRows[car].steeringReferenceNode, g_carInteriorProfilePointers[car].steeringBaseRotation);
    FixMatrix_Multiply(&combined, &g_carInteriorNodeRows[car].steeringReferenceNode->current, &rot);
    combined.position.x += position.x;
    combined.position.y += position.y;
    combined.position.z += position.z;

    original = g_carInteriorNodeRows[car].steeringRestNode->current;

    fade = CarInterior_UpdateSteeringBlendFraction(car, pCar);
    FixMatrix_Interpolate(&g_carInteriorNodeRows[car].steeringBlendNode->current, &combined, &original, 0, 0,
                          fadeCurve[(FixMul(0xC0000, fade) >> 16)], 1);
}
#undef FromAxisAngle16

// FUNCTION: CMR2 0x00477a90
void StageObject_ResetBodyTextureCaches(void)
{
    memset(g_carAppliedDigitTextureAlphas, 0xff, sizeof(g_carAppliedDigitTextureAlphas));
    memset(g_carAppliedRevTextureLevels, 0xff, sizeof(g_carAppliedRevTextureLevels));
}

// FUNCTION: CMR2 0x00478170
void StageObject_ResetCarSoundElapsedTime(int index)
{
    g_carSoundElapsed[index] = 0;
}

// FUNCTION: CMR2 0x00478190
void StageObject_AddCarSoundTimeSeconds(int index, int seconds)
{
    g_carSoundElapsed[index] += seconds * 100;
}

// FUNCTION: CMR2 0x004781c0
int StageObject_GetCarSoundElapsedTime(int index)
{
    return g_carSoundElapsed[index];
}

// FUNCTION: CMR2 0x004789b0
void StageObject_CopyCarSurfaceNoiseTarget(Car *pCar)
{
    pCar->surfaceNoise = pCar->surfaceNoiseTarget;
}

// Second group (0x4805f0-0x49e940)


extern CarContact *g_carContacts;
void StageObject_GetSkyNode(SceneNode **pOut);
void Glow_NoOpEntryCallback(BYTE a, BYTE b, int c, int d);

// GLOBAL: CMR2 0x005909bc
int g_unk0x005909bc;
// GLOBAL: CMR2 0x005909c8
FixVector g_unk0x005909c8[4];
// GLOBAL: CMR2 0x00590b7c
SceneNode *g_carPartNodes[4][8];
// GLOBAL: CMR2 0x00590bfc
BYTE g_unk0x00590bfc;
// GLOBAL: CMR2 0x00590bfd
BYTE g_unk0x00590bfd;
// GLOBAL: CMR2 0x00590c24
BYTE g_carPartModelIndices[4][8];
// Per moving body-part slot damage threshold (CarPart_AttachDamagedSlots).
// GLOBAL: CMR2 0x00590c44
int g_carPartDamageThresholds[4];
// GLOBAL: CMR2 0x00590c60
BYTE g_carPartDamageIndices[4];
// GLOBAL: CMR2 0x00590d70
int g_unk0x00590d70;
// GLOBAL: CMR2 0x00590db0
int g_viewCarLift[64];
// GLOBAL: CMR2 0x00590ed0
CollisionBox g_carCollisionBoxes[8];
// GLOBAL: CMR2 0x00591390
int g_unk0x00591390;
// GLOBAL: CMR2 0x005913d8
int g_unk0x005913d8;
// Eight per-car contact counts. The original clears them with two dword
// stores at 0x5913dc and 0x5913e0; the latter is the array's second half.
// GLOBAL: CMR2 0x005913dc
BYTE g_carContactCounts[8];
// GLOBAL: CMR2 0x005913f8
BYTE g_unk0x005913f8[8][8];
// GLOBAL: CMR2 0x0059146c
int g_unk0x0059146c[8];
// GLOBAL: CMR2 0x005914c8
FixVector g_unk0x005914c8;
// Per view record, the trackside camera's state: the point it turns to (the
// car plus a speed lead) and the aim that steps towards it.
// GLOBAL: CMR2 0x005916a0
FixVector g_tracksideAimTargets[4];
// The aim has reached its target and follows it directly.
// GLOBAL: CMR2 0x005916f0
int g_tracksideAimLocked[4];
// Smoothed lead of the aim ahead of the car (View_BuildTracksideAimLead).
// GLOBAL: CMR2 0x00591868
FixVector g_tracksideAimLeads[4];
// GLOBAL: CMR2 0x00591898
FixVector g_tracksideAims[4];
// GLOBAL: CMR2 0x00591740
int g_viewCameraSpot[4];
// GLOBAL: CMR2 0x00591750
// Trackside camera spot of the stage (0x6c bytes); the stage file lists them
// after a count (StageObject_SelectNonemptyRecordList).
struct CameraSpot {
    short field_0x0;
    short heading;          // 0x02 near +/-0x400: the spot behaves as a chase camera
    FixVector right;        // 0x04 basis of the spot
    FixVector up;           // 0x10
    FixVector forward;      // 0x1c direction of its dolly track
    FixVector position;     // 0x28
    FixVector trackEnd;     // 0x34 end of the dolly track
    int range;              // 0x40 trigger range
    int zoomAtRange;        // 0x44 zoom with the car at the trigger range
    int speedScale;         // 0x48 lead of the aim point per unit of car speed
    int aimStep[2];         // 0x4c step of the aim point (car nearer the spot / the camera)
    int zoom;               // 0x54 zoom with the car at the spot
    int shakeRange;         // 0x58 camera-to-car distance below which the camera shakes
    int shakeAmplitude;     // 0x5c shake at distance 0, per unit of car speed
    int dollyStep;          // 0x60 progress along the dolly track per frame (none when <= 0)
    int zoomStep[2];        // 0x64 step of the zoom, as aimStep
};
// The stage's list of CameraSpot records (NULL without one). Keeps its address
// name: any other name changes register ties in seven functions of this file.
CameraSpot *g_unk0x00591750;
// GLOBAL: CMR2 0x005918c8
int g_unk0x005918c8;
// GLOBAL: CMR2 0x005920f0
Mesh *g_groundReferenceMesh;
// GLOBAL: CMR2 0x00592114
FixVector g_lightMeshReferencePoint;
// GLOBAL: CMR2 0x00592128
int g_unk0x00592128;
// GLOBAL: CMR2 0x0059212c
int g_unk0x0059212c;
// GLOBAL: CMR2 0x00592130
int g_unk0x00592130;
// GLOBAL: CMR2 0x00592134
int g_unk0x00592134;

// GLOBAL: CMR2 0x00590af8
void *g_unk0x00590af8;
// GLOBAL: CMR2 0x00590afc
BYTE g_unk0x00590afc;
// GLOBAL: CMR2 0x00590b00
Texture *g_unk0x00590b00;
// GLOBAL: CMR2 0x00590b04
void *g_unk0x00590b04;
// GLOBAL: CMR2 0x00590b08
void *g_unk0x00590b08;
// GLOBAL: CMR2 0x00590b0c
void **g_unk0x00590b0c;

// GLOBAL: CMR2 0x0051ea20
char g_strMenuSoundNames[5][7] = {"move", "select", "back", "error", "toggle"};

extern char g_strMenuSoundFormat[24];
extern DWORD g_unk0x0058dc58;

// Loads the five menu sounds from the common frontend archive, for the stage
// menus. Returns 0 if any sample failed to load.
// FUNCTION: CMR2 0x00478b80
BYTE StageObject_LoadStageMenuSounds(void)
{
    int i;
    BYTE result;

    result = 1;
    g_unk0x0058dc58 = 0;
    for (i = 0; i < 5; i++) {
        sprintf(CFrontend::m_stringDest, g_strMenuSoundFormat, CInstallInfo::GetGameCDPath(), g_strMenuSoundNames[i]);
        if (Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile0()) == 0)
            result = 0;
    }
    return result;
}

// Releases the vehicle files loaded by 0x47e4d0 (registered callback).
// FUNCTION: CMR2 0x0047ea20
BYTE StageObject_ReleaseVehicleModelFiles(void)
{
    int i;

    if (g_unk0x00590af8 != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_unk0x00590af8);
        g_unk0x00590af8 = NULL;
    }
    if (g_unk0x00590b04 != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_unk0x00590b04);
        g_unk0x00590b04 = NULL;
    }
    if (g_unk0x00590b08 != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_unk0x00590b08);
        g_unk0x00590b08 = NULL;
    }
    if (g_unk0x00590b0c != NULL) {
        for (i = 0; i < 3; i++) {
            if (g_unk0x00590b0c[i] != NULL) {
                CFileBuffer::FreeGenericFileBuffer(g_unk0x00590b0c[i]);
                g_unk0x00590b0c[i] = NULL;
            }
        }
        if (g_unk0x00590b0c != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_unk0x00590b0c);
            g_unk0x00590b0c = NULL;
        }
    }
    g_unk0x00590afc = 0;
    return 1;
}

extern float g_oneOverRandMax;

// Colours picked for the vehicle debris fragments.
// GLOBAL: CMR2 0x0051f4ec
DWORD g_stageDebrisPalette[6] = {
    0xffb34aff, 0xffff5c30, 0xffffff3d,
    0xff4ec4ff, 0xff2368ff, 0xff27ff9e
};

// Starts a vehicle debris effect in the first free slot, including its fragments and sound.
#define DEBRIS_RAND() ((int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536)))
// FUNCTION: CMR2 0x0047fcb0
void StageObject_SpawnDebris(const FixVector *pPosition, const FixVector *pVelocity, unsigned int variant)
{
    FireworkRocket *p;
    int slot;
    int i;
    int row;
    int column;
    int quadrant;
    int enabled;
    int chance;
    int density;
    int roll;

    slot = -1;
    for (i = 0; i < g_unk0x00590afc; i++) {
        if (((FireworkRocket *)g_unk0x00590af8)[i].state == 0) {
            slot = i;
            i = g_unk0x00590afc;
        }
    }
    if (slot < 0)
        return;
    p = &((FireworkRocket *)g_unk0x00590af8)[slot];
    p->state = 1;
    p->pos = *pPosition;
    p->prevPos = *pPosition;
    p->vel = *pVelocity;
    p->trailLen = 0;
    p->trailHead = 0;
    for (i = 0; i < 20; i++) {
        p->trail[i] = p->pos;
        p->prevTrail[i] = p->pos;
        p->trailFlag[i] = 0;
    }
    p->blink = 0;
    if (variant != 0)
        p->fuse = FixMul(DEBRIS_RAND(), 0xa0000) + 0x50000;
    else
        p->fuse = 0x190000;
    p->rise = 0xa3d;
    p->sparkGravity = FixMul(0x106, 0x60000);
    p->colour = rand() % 6;
    *(DWORD *)p->rocketColour = g_stageDebrisPalette[p->colour];
    p->rocketColour[3] = 0xff;
    *(DWORD *)p->trailColour = *(DWORD *)p->rocketColour;
    *(DWORD *)p->colourA[0] = *(DWORD *)p->rocketColour;
    *(DWORD *)p->colourA[1] = g_stageDebrisPalette[rand() % 6];
    p->colourA[1][3] = 0xff;
    *(DWORD *)p->colourB[1] = g_stageDebrisPalette[rand() % 6];
    p->colourB[1][3] = 0xff;
    *(DWORD *)p->colourB[0] = g_stageDebrisPalette[rand() % 6];
    p->colourB[0][3] = 0xff;
    roll = DEBRIS_RAND();
    if (variant != 0) {
        if (roll <= 0x10000)
            p->type = 1;
        else
            p->type = 0;
    } else {
        if (roll < 0x8000)
            p->type = 1;
        else
            p->type = 2;
    }
    if (p->type == 1) {
        chance = FixMul((rand() % 9 + 1) << 16, 0x1999);
        if (DEBRIS_RAND() > 0xfd70) {
            p->blinking = 1;
            chance = FixMul(chance, 0x20000);
        } else {
            p->blinking = 0;
        }
        enabled = 0;
        for (row = 0; row < 3; row++) {
            for (column = 0; column < 6; column++) {
                for (quadrant = 0; quadrant < 4; quadrant++) {
                    if (DEBRIS_RAND() <= chance) {
                        p->sparkOn[row][column][quadrant] = 1;
                        enabled++;
                    } else {
                        p->sparkOn[row][column][quadrant] = 0;
                    }
                    if (DEBRIS_RAND() > 0x8000)
                        p->sparkAlt[row][column][quadrant] = 1;
                    else
                        p->sparkAlt[row][column][quadrant] = 0;
                }
            }
        }
        density = FixDiv(enabled << 16, 0x480000);
        p->burstTime = FixMul(0xf0000, density) + 0xa0000;
        p->sparkSpeed = FixMul(0xccc, 0x10000 - density) + 0x11eb;
        p->burst = 1;
        if (p->blinking != 0) {
            p->blinkTime = p->burstTime;
            p->burstTime = FixMul(p->burstTime, 0x20000);
        }
    } else {
        for (row = 0; row < 3; row++)
            for (column = 0; column < 6; column++)
                for (quadrant = 0; quadrant < 4; quadrant++)
                    p->sparkOn[row][column][quadrant] = 0;
        p->burstTime = 0x50000;
        p->sparkSpeed = 0x11eb;
        p->burst = 0;
    }
    Sound_PlaySampleWithParameters((unsigned short)(g_unk0x005909bc + 10), 0xccc, 0x5622, 0, 0, 0);
    if (DEBRIS_RAND() > 0x1999)
        p->sound = (char)Sound_PlaySampleWithParameters((unsigned short)(g_unk0x005909bc + rand() % 2),
                                      FixMul(0x4000, DEBRIS_RAND()) + 0x4000, 0x5622, 0, 0, 0);
    else
        p->sound = (char)0xff;
}
#undef DEBRIS_RAND

extern double g_fixedDegreesToAngle12;

// Spawns one debris burst at a random entry of the four-way spawn table with a
// random, upward-biased velocity of length 1.5..2.5.
// FUNCTION: CMR2 0x00480380
void StageObject_SpawnRandomDebrisBurst(void)
{
    FixVector velocity;
    FixVector *pPosition;
    unsigned short angle;
    int randomFixed;
    int randomSpread;

    randomFixed = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536));
    angle = (unsigned short)FTOI(__int64, X87S_MUL((double)FixMul(randomFixed, 0x1680000), g_fixedDegreesToAngle12));
    pPosition = &g_unk0x005909c8[rand() % 4];

    velocity.x = g_sinTable[angle & 0xfff];
    velocity.y = 0;
    velocity.z = g_sinTable[(angle + 0x400) & 0xfff];

    FixVecScale(&velocity, &velocity, (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536)));
    velocity.y = 0x40000;

    FIX_NORMALIZE_INTO(velocity, velocity)

    randomSpread = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536));
    FixVecScale(&velocity, &velocity, FixMul(0x10000, randomSpread) + 0x18000);

    StageObject_SpawnDebris(pPosition, &velocity, 0);
}

// FUNCTION: CMR2 0x004805f0
void StageObject_SetVehicleEffectState(int value)
{
    g_unk0x005909bc = value;
}

// Interpolates every active debris slot's derived position arrays one step
// toward their targets by `scale` (16.16 fixed point).
// One debris piece (0x938 bytes): current, previous and interpolated copies
// of its position, its 20 outline points and its 3x6 shard vertices.
struct DebrisSlot {
    FixVector pos;               // 0x000
    BYTE field_0xc[0xc];
    FixVector points[20];        // 0x018
    FixVector shards[3][6];      // 0x108
    FixVector prevPos;           // 0x1e0
    FixVector lerpPos;           // 0x1ec
    FixVector prevPoints[20];    // 0x1f8
    FixVector lerpPoints[20];    // 0x2e8
    FixVector prevShards[3][6];  // 0x3d8
    FixVector lerpShards[3][6];  // 0x4b0
    BYTE field_0x588[0x10c];
    int active;                  // 0x694
    BYTE field_0x698[0x2a0];
};

// FUNCTION: CMR2 0x00480600
void StageObject_UpdateDebris(int scale)
{
    DebrisSlot *pSlot;
    FixVector delta;
    int i;
    int k;
    int r;
    int c;

    for (i = 0; i < (int)(g_unk0x00590afc & 0xff); i++) {
        pSlot = &((DebrisSlot *)g_unk0x00590af8)[i];
        if (pSlot->active != 0) {
            delta.x = pSlot->pos.x - pSlot->prevPos.x;
            delta.y = pSlot->pos.y - pSlot->prevPos.y;
            delta.z = pSlot->pos.z - pSlot->prevPos.z;
            FixVecScale(&delta, &delta, scale);
            pSlot->lerpPos.x = pSlot->prevPos.x + delta.x;
            pSlot->lerpPos.y = pSlot->prevPos.y + delta.y;
            pSlot->lerpPos.z = pSlot->prevPos.z + delta.z;
            for (k = 0; k < 20; k++) {
                delta.x = pSlot->points[k].x - pSlot->prevPoints[k].x;
                delta.y = pSlot->points[k].y - pSlot->prevPoints[k].y;
                delta.z = pSlot->points[k].z - pSlot->prevPoints[k].z;
                FixVecScale(&delta, &delta, scale);
                pSlot->lerpPoints[k].x = pSlot->prevPoints[k].x + delta.x;
                pSlot->lerpPoints[k].y = pSlot->prevPoints[k].y + delta.y;
                pSlot->lerpPoints[k].z = pSlot->prevPoints[k].z + delta.z;
            }
            for (r = 0; r < 3; r++) {
                for (c = 0; c < 6; c++) {
                    delta.x = pSlot->shards[r][c].x - pSlot->prevShards[r][c].x;
                    delta.y = pSlot->shards[r][c].y - pSlot->prevShards[r][c].y;
                    delta.z = pSlot->shards[r][c].z - pSlot->prevShards[r][c].z;
                    FixVecScale(&delta, &delta, scale);
                    pSlot->lerpShards[r][c].x = pSlot->prevShards[r][c].x + delta.x;
                    pSlot->lerpShards[r][c].y = pSlot->prevShards[r][c].y + delta.y;
                    pSlot->lerpShards[r][c].z = pSlot->prevShards[r][c].z + delta.z;
                }
            }
        }
    }
}
// FUNCTION: CMR2 0x00480a50
void StageObject_ClearPartTuningState(void)
{
    g_unk0x00590d70 = 0;
}

// FUNCTION: CMR2 0x00480ac0
void CarPart_HideNode(Car *pCar, int slot, int hidden)
{
    if (g_carPartNodes[slot][pCar->index] != NULL && hidden != 0)
        g_carPartNodes[slot][pCar->index]->viewMask = 0;
}

// FUNCTION: CMR2 0x00484d10
unsigned int CarPart_GetModelIndex(int slot, int carIndex)
{
    return g_carPartModelIndices[slot][carIndex];
}

// FUNCTION: CMR2 0x00484de0
SceneNode *CarPart_GetNode(Car *pCar, int slot)
{
    return g_carPartNodes[slot][pCar->index];
}

extern int *g_carLineCounts[8];
extern CarFlexibleLineState **g_carLineStates;
extern CarFlexibleLineDescriptor *g_carLineDescriptors[8];
extern FixVector g_carLineImpulse;

// Steps one 0x3c-byte record (`index`) of the current car's stage-object list:
// the record's +0x0 vector is copied into +0xc, the +0x30/+0x34 velocities lose
// the part of g_carLineImpulse that lies along the record's axis vector, and then
// a damped spring (k = 0x20000, c = 0x3333, step = g_physicsTimeStep) integrates
// +0x0/+0x2. Records with the +0x38 flag set are skipped.
// FUNCTION: CMR2 0x004854a0
void StageObject_IntegrateCarMotionRecord(int index)
{
    signed char car = g_partCar->index;
    BYTE i = (BYTE)index;
    CarFlexibleLineState *pRecord = g_carLineStates[car] + i;
    FixVector *pAxis = &g_carLineDescriptors[car][i].axis;
    FixVector velocity;
    FixVector projected;
    int dot;

    if (pRecord->grounded != 0)
        return;

    pRecord->previousPosition = pRecord->position;

    velocity = g_carLineImpulse;
    dot = FixVecDot(&velocity, pAxis);
    FixVecScale(&projected, pAxis, dot);
    velocity.x -= projected.x;
    velocity.y -= projected.y;
    velocity.z -= projected.z;

    pRecord->velocityX -= velocity.x;
    pRecord->velocityZ -= velocity.z;

    pRecord->velocityX += FixMul(g_physicsTimeStep,
                           -(FixMul(0x20000, pRecord->position.x) + FixMul(0x3333, pRecord->velocityX)));
    pRecord->velocityZ += FixMul(g_physicsTimeStep,
                           -(FixMul(0x20000, pRecord->position.z) + FixMul(0x3333, pRecord->velocityZ)));
    pRecord->position.x += FixMul(g_physicsTimeStep, pRecord->velocityX);
    pRecord->position.z += FixMul(g_physicsTimeStep, pRecord->velocityZ);
    pRecord->position.y = 0x10000;
}

// Steps every 0x3c-byte record of the ordered cars' lists: the record's +0x18
// vector becomes its +0xc vector plus the (+0x0 - +0xc) difference scaled by
// `scale`.
// match 65%: same logic; register allocation and the inner loop scheduling differ
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00486500
void StageObject_InterpolateOrderedMotionRecords(int scale)
{
    int count;
    int n;
    int offset;
    short *pIndex;
    CarFlexibleLineState *p;
    FixVector v;

    pIndex = Car_GetOrder();
    count = Car_GetOrderCount();
    if (count - 1 >= 0) {
        pIndex += count - 1;
        do {
            g_partCar = (Car *)Car_Get(*pIndex);
            if (g_partCar->isGhost == 0) {
                n = *g_carLineCounts[g_partCar->index] - 1;
                if (n >= 0) {
                    offset = n * sizeof(CarFlexibleLineState);
                    n++;
                    do {
                        p = (CarFlexibleLineState *)((BYTE *)g_carLineStates[g_partCar->index] +
                                    offset);
                        v.x = p->position.x - p->previousPosition.x;
                        v.y = p->position.y - p->previousPosition.y;
                        v.z = p->position.z - p->previousPosition.z;
                        FixVecScale(&v, &v, scale);
                        p->drawPosition.x = p->previousPosition.x;
                        p->drawPosition.x += v.x;
                        p->drawPosition.y = p->previousPosition.y + v.y;
                        p->drawPosition.z = p->previousPosition.z + v.z;
                        offset -= sizeof(CarFlexibleLineState);
                    } while (--n);
                }
            }
            pIndex--;
        } while (--count);
    }
}

// FUNCTION: CMR2 0x00486be0
void CarCamera_SetFullCarLift(CameraRecord *pRecord, CameraRecord *pOther)
{
    g_viewCarLift[pRecord->index] = 0x10000;
}

// FUNCTION: CMR2 0x00486c00
void CarCamera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther)
{
    Glow_NoOpEntryCallback(pOther->car, pOther->view, 0, 0);
    g_viewCarLift[pRecord->index] = 0x10000;
}

// FUNCTION: CMR2 0x00487130
int StageObject_GetCollisionRecordState(void)
{
    return g_unk0x00591390;
}

// Applies a per-frame delta to one stage object (plus an optional second car
// index) and recurses into its children; `flag` enables the 0x5913d8 path.
// FUNCTION: CMR2 0x0048c870
void StageObject_ApplyRecursiveFrameDelta(BYTE index, char other, int *pDelta, int flag)
{
    int i;

    g_unk0x005914c8 = *(FixVector *)pDelta;
    g_unk0x005913d8 = flag;
    memset(g_unk0x0059146c, 0, sizeof(g_unk0x0059146c));
    g_unk0x0059146c[index] = 1;
    if (other != -1)
        g_unk0x0059146c[other] = 1;
    i = 0;
    if (g_carContactCounts[index] > 0) {
        do {
            StageObject_MoveRecursiveBoxCorners(g_unk0x005913f8[index][i]);
            i++;
        } while (i < g_carContactCounts[index]);
    }
}

// Moves one stage object and its eight box corners by the current frame delta
// and recurses over its children (each object is only moved once).
// match 70%: same logic; the corner pointer walk uses a different base bias
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0048c900
void StageObject_MoveRecursiveBoxCorners(BYTE index)
{
    Car *pCar;
    BYTE i;
    int j;
    int *p;

    if (g_unk0x0059146c[index] != 0)
        return;
    g_unk0x0059146c[index] = 1;
    pCar = Car_Get(index);
    pCar->position.x += g_unk0x005914c8.x;
    pCar->position.y += g_unk0x005914c8.y;
    pCar->position.z += g_unk0x005914c8.z;
    p = (int *)&pCar->corners[0].y;
    j = 8;
    do {
        p[-1] += g_unk0x005914c8.x;
        p[0] += g_unk0x005914c8.y;
        p[1] += g_unk0x005914c8.z;
        p += 3;
    } while (--j);
    if (g_unk0x005913d8 != 0) {
        if (g_carCollisionBoxes[index].built != 0) {
            p = &g_carCollisionBoxes[index].pointWords[1];
            j = 4;
            do {
                p[-1] += g_unk0x005914c8.x;
                p[0] += g_unk0x005914c8.y;
                p[1] += g_unk0x005914c8.z;
                p += 3;
            } while (--j);
        }
    }
    i = 0;
    if (g_carContactCounts[index] != 0) {
        do {
            StageObject_MoveRecursiveBoxCorners(g_unk0x005913f8[index][i]);
            i++;
        } while (i < g_carContactCounts[index]);
    }
}

// Returns the car-indexed box shared by car-pair and sector collision tests.
// FUNCTION: CMR2 0x0048ca40
CollisionBox *Collision_GetCarBox(int index)
{
    return &g_carCollisionBoxes[index];
}

// A plain forward to AI_LoadStageRouteTables (compiled as a tail jump).
// FUNCTION: CMR2 0x0048ca60
void StageObject_ForwardSessionUpdate(void)
{
    AI_LoadStageRouteTables();
}

// FUNCTION: CMR2 0x0048ca90
int StageObject_GetSelectedRecordListState(void)
{
    return g_unk0x005918c8;
}

// Index of the 0x6c-byte record whose position (+0x34) is nearest to pPos.
// FUNCTION: CMR2 0x0048d8b0
unsigned int StageObject_FindNearestCameraSpot(FixVector *pPos)
{
    unsigned int i;
    unsigned int best = 0;
    int bestDistance = 0x270f0000;
    int distance;
    FixVector d;

    for (i = 0; i < (unsigned int)g_unk0x005918c8; i++) {
        d.x = g_unk0x00591750[i].trackEnd.x - pPos->x;
        d.y = g_unk0x00591750[i].trackEnd.y - pPos->y;
        d.z = g_unk0x00591750[i].trackEnd.z - pPos->z;
        distance = FixVec_Length(&d);
        if (distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

// FUNCTION: CMR2 0x0048d930
int StageObject_GetCarCameraSpotIndex(CameraRecord *pRecord)
{
    return g_viewCameraSpot[pRecord->index];
}

extern int g_tracksideZoomTargets[4];
void View_BuildTracksideAimLead(FixVector *pOut, CameraRecord *pRecord, Car *pCar, FixMatrix *pMatrix);

// Aim target of a trackside camera: decays the smoothed lead by 0.9, adds 0.1 of
// the new one (View_BuildTracksideAimLead), aims at the car plus that lead, and
// sets the zoom target from the spot's zoom (car at the spot) to its
// zoomAtRange (car at the trigger range).
// FUNCTION: CMR2 0x0048d950
void View_UpdateTracksideAimTarget(CameraRecord *pRecord, FixMatrix *pMatrix)
{
    BYTE index = pRecord->index;
    CameraSpot *pSpot = &g_unk0x00591750[g_viewCameraSpot[index]];
    FixVector *pPos = &g_tracksideAimTargets[index];
    FixVector *pImpact = &g_tracksideAimLeads[index];
    FixVector impact;
    FixVector delta;
    int t;
    Car *pCar;

    pCar = Car_Get(pRecord->car);
    View_BuildTracksideAimLead(&impact, pRecord, pCar, pMatrix);
    FixVecScale(pImpact, pImpact, 0xe666);
    FixVecScale(&impact, &impact, 0x1999);
    pImpact->x += impact.x;
    pImpact->y += impact.y;
    pImpact->z += impact.z;
    FixMatrix_GetPosition(pPos, pMatrix);
    pPos->x += pImpact->x;
    pPos->y += pImpact->y;
    pPos->z += pImpact->z;
    delta.x = pPos->x - pSpot->position.x;
    delta.y = pPos->y - pSpot->position.y;
    delta.z = pPos->z - pSpot->position.z;
    t = FixDiv(FixVec_Length(&delta), pSpot->range);
    g_tracksideZoomTargets[index] = FixMul(t, pSpot->zoomAtRange) +
                             FixMul(0x10000 - t, pSpot->zoom);
}

// FUNCTION: CMR2 0x00492890
void StageObject_GetRotatedStageLightVector(FixVector *pOut)
{
    SceneNode *obj;

    StageObject_GetSkyNode(&obj);
    FixMatrix_RotateVector(pOut, &g_lightMeshReferencePoint, &obj->current);
}

// The current weather light intensity (16.16; around 0x10000 in full sun,
// near 0 in dark/overcast weather): the sun gate for snow glints.
int StageObject_GetWeatherLightIntensity(void)
{
    return g_stageLighting.intensity;
}

// FUNCTION: CMR2 0x004928c0
void StageObject_GetStageLightValues(int *pOut1, int *pOut2, int *pOut3)
{
    *pOut3 = g_unk0x00592128;
    *pOut1 = g_unk0x0059212c;
    *pOut2 = g_unk0x00592130;
}

// FUNCTION: CMR2 0x004928f0
void StageObject_GetStageLightState(int *pOut)
{
    *pOut = g_unk0x00592134;
}

// FUNCTION: CMR2 0x00492900
void StageObject_SetStageLightState(int value)
{
    g_unk0x00592134 = value;
}

extern Mesh *g_stageMesh2Copy;
extern short g_stageMesh2Count;
// GLOBAL: CMR2 0x005920fc
int g_objectReferencePointX;
// GLOBAL: CMR2 0x00592100
int g_objectReferencePointY;
// GLOBAL: CMR2 0x00592104
int g_objectReferencePointZ;

// Finds the vertex of stage mesh 2 closest to g_lightMeshReferencePoint within a radius
// that shrinks to the best distance found so far, caches its stage-space
// position in 0x5920fc/0x592100/0x592104 and returns its index (-1 if none).
// FUNCTION: CMR2 0x00492910
int StageObject_FindClosestMeshVertex(void)
{
    SceneNode *object;
    SceneNode *unused;
    FixVector delta;
    FixVector d;
    int dist;
    int limit;
    int best;
    int i;
    int v;

    limit = 0x640000;
    best = -1;
    if (g_stageMesh2Count > 0) {
        // The point is taken into the space of the object that owns mesh 2.
        StageObject_GetCloudNodes(&object, &unused);
        FixMatrix_InverseRotateVector(&delta, &g_lightMeshReferencePoint, &object->current);
        for (i = 0; i < g_stageMesh2Count; i++) {
            d.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh2Copy->pVertexData)[i].pos[0], CGraphics::m_65536));
            d.y = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh2Copy->pVertexData)[i].pos[1], CGraphics::m_65536));
            v = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh2Copy->pVertexData)[i].pos[2], CGraphics::m_65536));
            d.x = delta.x - d.x;
            d.y = delta.y - d.y;
            d.z = delta.z - v;
            if (FIX_ABS(d.x) <= limit && FIX_ABS(d.y) <= limit && FIX_ABS(d.z) <= limit) {
                dist = FixVecDot(&d, &d);
                if (dist <= 0x27100000) {
                    limit = FixSqrt(dist);
                    best = i;
                }
            }
        }
        if (best != -1) {
            g_objectReferencePointX = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh2Copy->pVertexData)[best].pos[0], CGraphics::m_65536));
            g_objectReferencePointY = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh2Copy->pVertexData)[best].pos[1], CGraphics::m_65536));
            g_objectReferencePointZ = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh2Copy->pVertexData)[best].pos[2], CGraphics::m_65536));
            return best;
        }
    }
    return best;
}

// FUNCTION: CMR2 0x00492bb0
void StageObject_GetGroundReferenceColour(int *pOut)
{
    *pOut = *(int *)g_groundReferenceMesh->pTriangles->colour;
}

// FUNCTION: CMR2 0x00498570
CarContact *StageObject_GetMotionRecord(int index)
{
    return &g_carContacts[index];
}

// Wraps a 16.16 angle in degrees into [-180, 180).
// FUNCTION: CMR2 0x00498db0
int StageObject_WrapFixedDegreeAngle(int angle)
{
    if (angle >= 0xb40000)
        angle -= 0x1680000;
    if (angle < -0xb40000)
        angle += 0x1680000;
    return angle;
}

// Marks a nearby car as travelling roughly towards the player car.
// match 81%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0047cc50
void StageObject_UpdateApproachingCar(int carIndex, int time)
{
    g_unk0x0058e270[carIndex] = 1;
    Car *pPlayer = Car_Get(0);
    Car *pOther = Car_Get(carIndex);
    int otherZ = pOther->position.z;
    int otherX = pOther->position.x;
    int heading = StageObject_Atan2Degrees(pOther->right.z, pOther->right.x);
    int duration = RallyData_GetRouteAvailabilityState();
    int difference = RallyData_GetActiveCarRaceRecordField0(pPlayer) - time;
    if (difference < -100) difference += duration;
    if (difference >= 0 && difference <= 7) {
        int direction = StageObject_Atan2Degrees(pPlayer->position.z - otherZ,
                                                 pPlayer->position.x - otherX);
        direction = StageObject_WrapFixedDegreeAngle(heading - direction);
        if (direction <= 0xa0000 && direction >= -0xa0000) return;
    }
    g_unk0x0058e270[carIndex] = 0;
}

// Buttons held on any connected device.
// FUNCTION: CMR2 0x0049e940
unsigned int StageObject_GetAnyDeviceHeldButtons(void)
{
    unsigned int buttons = 0;
    int i;

    for (i = 0; i < 8; i++) {
        if (CInput::m_availableDevices[i].type != -1)
            buttons |= CInput::m_availableDevices[i].held;
    }
    return buttons;
}

// Keeps the subset of the six cars that are still inside the race-time,
// squared-distance and heading windows of car `index`, copies the per-car keep
// flags to pOut, accumulates their number into pCount and reports the overtake
// side in pFlag (1 / 0xff). Returns the first byte of the 0x10-byte race-record
// table row of `index`.
// FUNCTION: CMR2 0x0047d0e0
unsigned int AI_SelectCarsWithinOvertakeWindow(int index, BYTE *pOut, int *pCount, BYTE *pFlag)
{
    Car *cars[6];
    int active[6];
    int scratch[6];
    Car *pCar;
    int i;
    int duration;
    int z;
    int x;
    int referenceTime;
    int heading;

    for (i = 0; i < 6; i++)
        active[i] = 1;
    for (i = 0; i < 6; i++) {
        *pFlag = 0;
        cars[i] = Car_Get(i);
    }
    duration = RallyData_GetRouteAvailabilityState();
    pCar = cars[index];
    active[index] = 0;
    z = pCar->position.z;
    x = pCar->position.x;
    *pCount = 0;
    referenceTime = RallyData_GetActiveCarRaceRecordField0(pCar);

    for (i = 0; i < (signed char)g_unk0x0058e0b0[1]; i++) {
        if (active[i] != 0) {
            int delta = RallyData_GetActiveCarRaceRecordField0(cars[i]);
            scratch[i] = delta;
            delta -= referenceTime;
            if (delta < 0 || delta > 5) {
                if (delta < -100)
                    delta += duration;
                if (delta <= 0 || delta > 5)
                    active[i] = 0;
            }
        }
    }

    for (i = 0; i < (signed char)g_unk0x0058e0b0[1]; i++) {
        if (active[i] != 0) {
            int dz = cars[i]->position.z - z;
            int dx = cars[i]->position.x - x;
            int dist = FixMul(dz, dz) + FixMul(dx, dx);
            scratch[i] = dist;
            if (dist > 0x90000)
                active[i] = 0;
        }
    }

    heading = StageObject_Atan2Degrees(pCar->right.z, pCar->right.x);
    for (i = 0; i < (signed char)g_unk0x0058e0b0[1]; i++) {
        if (active[i] != 0) {
            int d = StageObject_Atan2Degrees(cars[i]->right.z, cars[i]->right.x);
            d = StageObject_WrapFixedDegreeAngle(heading - d);
            if (d > 0x140000 || d < -0x140000)
                active[i] = 0;
        }
    }

    for (i = 0; i < (signed char)g_unk0x0058e0b0[1]; i++) {
        if (active[i] != 0) {
            int d = StageObject_WrapFixedDegreeAngle(StageObject_Atan2Degrees(cars[i]->position.z - z,
                                                          cars[i]->position.x - x) - heading);
            active[i] = 0;
            if (d > 0x500000 && d < 0x780000)
                *pFlag = 1;
            if (d < -0x500000 && d > -0x780000)
                *pFlag = 0xff;
        }
    }

    for (i = 0; i < (signed char)g_unk0x0058e0b0[1]; i++) {
        pOut[i] = (BYTE)active[i];
        *pCount += active[i];
    }
    return g_aiRouteNodes[referenceTime].field_0x0;
}

void Graphics_SetFlareStateValue(int value);
void Scene_SetLight(FixVector *pLight, int boost);
int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass,
                                 unsigned short *pSurface, int defaultY);
DWORD Graphics_GetDeviceCaps3C(void);
DWORD Graphics_GetDeviceCaps40(void);
DWORD Graphics_GetDeviceCaps44(void);
CarPartSet *CarPart_GetSet(int carIndex);
void CarPart_HideNode(Car *pCar, int slot, int hidden);
void Car_SetDrawnFlag(int index, char value);
void *CarInfo_GetSection(Car *pCar, int section);
struct KnockoutMatch;
int Knockout_HasHumanLostMatch(KnockoutMatch *pMatch);
short Car_GetOrderCount(void);

// FUNCTION: CMR2 0x00492fd0
void StageObject_SetForwardedFlareState(int value)
{
    Graphics_SetFlareStateValue(value);
}

// Position of the object held in 0x589438 (its matrix at +0x98).
// FUNCTION: CMR2 0x0046f4a0
void StageObject_GetCurrentObjectPosition(FixVector *pOut)
{
    if (g_stageSkyNode != 0)
        FixMatrix_GetPosition(pOut, &g_stageSkyNode->current);
}

// Ground height at a point; the surface id is written over the defaultY slot.
// FUNCTION: CMR2 0x004930b0
int Track_GetGroundHeight5(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass, int defaultY)
{
    return Track_GetGroundHeightSurface(pPoint, pNormal, pTri, pSurfaceClass, (unsigned short *)&defaultY, defaultY);
}

// Sets the stage light, without the boost for country 3.
// FUNCTION: CMR2 0x00492e30
void StageObject_SetUnboostedStageLight(FixVector *pLight)
{
    if ((char)RallyDataCountryIndex() == 3) {
        Scene_SetLight(pLight, 0);
        return;
    }
    Scene_SetLight(pLight, 1);
}

// Clears the value of every car slot not in use (or all of them when
// CGameInfo::GetGameInfoSessionFlag is set).
// FUNCTION: CMR2 0x0047c1b0
void StageObject_ClearUnusedCarSlotValues(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        if (i >= (int)(RallyDataState() & 0xff) || CGameInfo::GetGameInfoSessionFlag() != 0)
            Car_SetDrawnFlag(i, 0xff);
    }
}

// FUNCTION: CMR2 0x00466490
void StageObject_UpdateDeviceEffectSupportFlag(void)
{
    if (Graphics_GetDeviceCaps3C() == 0 && Graphics_GetDeviceCaps40() == 0 && Graphics_GetDeviceCaps44() == 0) {
        g_unk0x00588868 = 0;
        return;
    }
    g_unk0x00588868 = 1;
}

// GLOBAL: CMR2 0x0058896c
int g_unk0x0058896c;

unsigned char RallyData_GetSelectionFlag26(void);
BYTE View_GetActiveCameraFlags(BYTE index);
int Race_IsMultiplayerRecordMode10(void);
void CarPhysics_DrawBodyWheelAndSkidShadows(Car *pCar, int view);
void StageObject_DrawViewPrecipitationAndObjects(int *pViewRect, int b);
void StageObject_DrawListedCarLightBeams(short *pOrder, short count, int view);
void Fireworks_Draw(void);

// Updates the "damaged / off-road" state of every car in the given order.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00466570
void CarDamage_UpdateOrderedCarsOffRoadState(short *param_1, short param_2, int *pViewRect, int param_4)
{
    short *p;
    Car *pCar;
    int i;

    i = 0;
    while (i < (int)param_2) {
        p = param_1 + i;
        pCar = Car_Get(*p);
        if (pCar->isGhost == 0 &&
            (&pCar->firstCameraBlocked)[param_4] == 0) {
            if (Race_IsMultiplayerRecordMode10() != 0) {
                if ((char)RallyData_GetSelectionFlag27() || (char)RallyData_GetSelectionFlag26() ||
                    (unsigned int)i == (View_GetActiveCameraFlags(1) & 0xff))
                    CarPhysics_DrawBodyWheelAndSkidShadows(pCar, 1);
            } else if (StageObject_GetPairedCarValue(pCar->index, param_4) != 7) {
                CarPhysics_DrawBodyWheelAndSkidShadows(pCar, param_4);
            }
        }
        i = i + 1;
    }
    StageObject_DrawListedCarLightBeams(param_1, param_2, param_4);
    StageObject_DrawViewPrecipitationAndObjects(pViewRect, param_4);
    if (g_unk0x0058896c != 0)
        Fireworks_Draw();
}

// Fixed-point to float conversion factors and the fade thresholds of the
// stage object lighting.
// GLOBAL: CMR2 0x00511378
extern const float g_float5 = 5.0f;
// GLOBAL: CMR2 0x005113d0
extern const float g_floatOneOver45 = 1.0f / 45.0f;
// GLOBAL: CMR2 0x005113dc
extern const float g_float480 = 480.0f;
// GLOBAL: CMR2 0x005113e0
extern const float g_float640 = 640.0f;
// GLOBAL: CMR2 0x005113c8
extern const double g_zero0x005113c8 = 0.0;

extern const float g_netZero;
extern const float g_netByteScale;
extern const float g_netOne;

int View_GetActiveCameraMode(BYTE index);

// Fades a stage object in and out from the screen distance between two
// projected points of the player's car.
// match 69%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00466100
void StageObject_UpdateProjectedDistanceFade(int param_1)
{
    Car *pCar;
    SceneNode *pView;
    FixVector pos;
    FixVector dir;
    int screen[2];
    float f2;
    float f4;
    float f1;
    int alpha;

    pCar = Car_Get(1);
    pView = g_viewNodes[param_1];
    FixMatrix_GetPosition(&pos, &pCar->pSceneRoot->current);
    pos.y = pos.y + 0x10000;
    FixMatrix_ProjectWorldPointToView(screen, &pos, pView);
    if (screen[0] != -0x640000 || screen[1] != -0x640000) {
        f2 = (float)(X87S_MUL(screen[0], CGraphics::m_oneOver65536));
        f4 = (float)(X87S_MUL(screen[1], CGraphics::m_oneOver65536));
        FixMatrix_GetUp(&dir, &pView->current);
        dir.x = dir.x + pos.x;
        dir.y = dir.y + pos.y;
        dir.z = dir.z + pos.z;
        FixMatrix_ProjectWorldPointToView(screen, &dir, pView);
        if (screen[0] != -0x640000 || screen[1] != -0x640000) {
            f1 = X87S_DIV(((float)(X87S_MUL(screen[0], CGraphics::m_oneOver65536)) - f2) * g_float640, X87S_FILD(float, g_pGraphics->resX));
            f2 = X87S_DIV(((float)(X87S_MUL(screen[1], CGraphics::m_oneOver65536)) - f4) * g_float480, X87S_FILD(float, g_pGraphics->resY));
            f1 = (float)X87S_SQRT(f1 * f1 + f2 * f2);
            if (f1 <= g_float5)
                return;
            f1 = g_netOne - (f1 - g_float5) * g_floatOneOver45;
            if (f1 >= g_netOne) {
                alpha = 0xff;
            } else {
                if (f1 <= g_netZero)
                    alpha = 0;
                else
                    alpha = (int)FTOI(__int64, f1 * g_netByteScale);
            }
            if (View_GetActiveCameraMode(param_1) == 7)
                alpha = 0x80;
            if (alpha == g_unk0x00588864)
                return;
            StageTiming_SetNodeMeshAlpha(pCar->pSceneRoot, alpha, 0);
            StageTiming_SetNodeTreeMeshAlpha(pCar->pSceneRoot->pFirstChild, alpha, 0);
            StageTiming_SetNodeMeshAlpha(pCar->pBodyNode, alpha, 0);
            StageTiming_SetNodeTreeMeshAlpha(pCar->pBodyNode->pFirstChild, alpha, 0);
            g_unk0x00588864 = alpha;
            return;
        }
    }
    if (g_unk0x00588864 == 0)
        return;
    StageTiming_SetNodeMeshAlpha(pCar->pSceneRoot, 0, 0);
    StageTiming_SetNodeTreeMeshAlpha(pCar->pSceneRoot->pFirstChild, 0, 0);
    StageTiming_SetNodeMeshAlpha(pCar->pBodyNode, 0, 0);
    StageTiming_SetNodeTreeMeshAlpha(pCar->pBodyNode->pFirstChild, 0, 0);
    g_unk0x00588864 = 0;
}

// Swaps *pValue with the value stored for `slot` when that slot is pending.
// FUNCTION: CMR2 0x004660a0
void Replay_SwapPendingSlotValue(int **pValue, int slot, int flag)
{
    int *old;

    if (g_unk0x00588761 == slot) {
        old = (int *)g_activeReplayStream;
        g_activeReplayStream = (ReplayStream *)*pValue;
        *pValue = old;
        g_unk0x00588761 = -1;
        g_unk0x00588760 = flag;
    }
}

// FUNCTION: CMR2 0x0046b670
void CarPart_ApplyHiddenStates(Car *pCar)
{
    CarPartSet *pParts;
    int slot;

    pParts = CarPart_GetSet(pCar->index);
    slot = 0;
    while (slot < 4) {
        CarPart_HideNode(pCar, slot, pParts->partHidden[slot]);
        slot++;
    }
}

// CIN section 4: variable camera offsets indexed by each view mode.
// GLOBAL: CMR2 0x00590d90
FixVector *g_carCameraOffsets[8];

// FUNCTION: CMR2 0x00486700
void CarInfo_CacheCameraOffsets(void)
{
    FixVector **p;
    int i;

    i = 0;
    if (Car_GetOrderCount() > 0) {
        p = g_carCameraOffsets;
        do {
            *p = (FixVector *)CarInfo_GetSection(Car_Get(i), CAR_INFO_CAMERA_OFFSETS);
            i++;
            p++;
        } while (i < Car_GetOrderCount());
    }
}

// GLOBAL: CMR2 0x0058cf70
int g_unk0x0058cf70;

// Clears the championship "pending" flag (bit 23) when set, or when 0x58cf70 is clear.
// FUNCTION: CMR2 0x004729f0
BYTE Knockout_ClearChampionshipPendingFlag(void)
{
    unsigned int *pState;

    pState = RallyData_GetChampionshipState();
    if ((*pState & 0x800000) == 0 && g_unk0x0058cf70 != 0)
        return 0;
    *pState &= 0xff7fffff;
    g_knockoutRaceState = 0;
    CGame::SetFrontendResourceMode(0);
    return 1;
}

// Eight records of 0x48 bytes: ten shorts at +0x1c (reset to -1) and two
// flag bytes at +0x44/+0x45.
// GLOBAL: CMR2 0x0058d6d0
CarLightTextureState g_carLightTextureStates[8];

// Fills the 12 outline values of a stage box (11 boundary levels plus the
// corner colour at +0x16) and repaints its two textures once the cached copy
// differs from the new values.
int StageTiming_GetDashRevValue(int index);
void CarInterior_UpdateGearDigitTexture(Texture *pTexture, int state, int cacheBase, int index);
// FUNCTION: CMR2 0x00477460
void StageObject_UpdateRevCounterTextures(int index)
{
    unsigned short *pNew = g_carRequestedRevTextureLevels[index];
    unsigned short *pOld = g_carAppliedRevTextureLevels[index];
    int changed = 0;
    int limit;
    int slot;
    int i;

    limit = FixMul(StageTiming_GetDashRevValue(index), 0xb0000) >> 16;
    for (i = 0; i <= 0xa; i++)
        pNew[i] = i < limit ? 0xff : 0;
    if (g_carInteriorDashTextures[index].digit != 0)
        CarInterior_UpdateGearDigitTexture(g_carInteriorDashTextures[index].digit, (int)Car_Get(index)->gear, 2, index);
    if (g_carInteriorDashTextures[index].revCounter != 0) {
        slot = index + 8;
        pNew[0xb] = 0x6c;
        // the original leaves the scan by setting the counter to 0xc
        for (i = 0; i < 0xc; i++) {
            if (pNew[i] != pOld[i]) {
                changed = 1;
                i = 0xc;
            }
        }
        if (changed != 0) {
            CGraphics::BltTexture(g_carInteriorDashTextures[index].revCounter, slot);
            CGraphics::RemapTextureAlpha(g_carInteriorDashTextures[index].revCounter, 0xe0, 0xff, 0xd0, pNew[1], 0xc0,
                                         pNew[2], slot);
            CGraphics::RemapTextureAlpha(g_carInteriorDashTextures[index].revCounter, 0xb0, pNew[3], 0xa0, pNew[4], 0x90,
                                         pNew[5], slot);
            CGraphics::RemapTextureAlpha(g_carInteriorDashTextures[index].revCounter, 0x80, pNew[6], 0x70, pNew[7], 0x60,
                                         pNew[8], slot);
            CGraphics::RemapTextureAlpha(g_carInteriorDashTextures[index].revCounter, 0x50, pNew[9], 0x40, pNew[10], 0x30,
                                         pNew[0xb], slot);
            for (i = 0; i < 0xc; i++)
                pOld[i] = pNew[i];
        }
    }
}

// Seven alpha values of the gear digit, selected by the current gear. They
// are compared against the applied values so the texture is repainted only
// when its digit changes.
// FUNCTION: CMR2 0x004775f0
void CarInterior_UpdateGearDigitTexture(Texture *pTexture, int state, int cacheBase, int index)
{
    WORD *pColours = g_carRequestedDigitTextureAlphas[index];
    WORD *pApplied = g_carAppliedDigitTextureAlphas[index];
    int changed = 0;
    int cacheSlot = index + cacheBase * 8;
    int i;

    switch (state) {
    case 0:
        pColours[0] = 0;
        pColours[1] = 0xff;
        pColours[2] = 0xff;
        pColours[3] = 0xff;
        pColours[4] = 0xff;
        pColours[5] = 0xff;
        pColours[6] = 0xff;
        break;
    case 1:
        pColours[0] = 0;
        pColours[1] = 0;
        pColours[2] = 0xff;
        pColours[3] = 0xff;
        pColours[4] = 0;
        pColours[5] = 0;
        pColours[6] = 0;
        break;
    case 2:
        pColours[0] = 0xff;
        pColours[1] = 0xff;
        pColours[2] = 0xff;
        pColours[3] = 0;
        pColours[4] = 0xff;
        pColours[5] = 0xff;
        pColours[6] = 0;
        break;
    case 3:
        pColours[0] = 0xff;
        pColours[1] = 0xff;
        pColours[2] = 0xff;
        pColours[3] = 0xff;
        pColours[4] = 0xff;
        pColours[5] = 0;
        pColours[6] = 0;
        break;
    case 4:
        pColours[0] = 0xff;
        pColours[1] = 0;
        pColours[2] = 0xff;
        pColours[3] = 0xff;
        pColours[4] = 0;
        pColours[5] = 0;
        pColours[6] = 0xff;
        break;
    case 5:
        pColours[0] = 0xff;
        pColours[1] = 0xff;
        pColours[2] = 0;
        pColours[3] = 0xff;
        pColours[4] = 0xff;
        pColours[5] = 0;
        pColours[6] = 0xff;
        break;
    case 6:
        pColours[0] = 0xff;
        pColours[1] = 0xff;
        pColours[2] = 0;
        pColours[3] = 0xff;
        pColours[4] = 0xff;
        pColours[5] = 0xff;
        pColours[6] = 0xff;
        break;
    case 7:
        pColours[0] = 0xff;
        pColours[1] = 0;
        pColours[2] = 0;
        pColours[3] = 0;
        pColours[4] = 0;
        pColours[5] = 0xff;
        pColours[6] = 0;
        break;
    case 8:
        pColours[0] = 0xff;
        pColours[1] = 0xff;
        pColours[2] = 0xff;
        pColours[3] = 0xff;
        pColours[4] = 0xff;
        pColours[5] = 0xff;
        pColours[6] = 0xff;
        break;
    case 9:
        pColours[0] = 0xff;
        pColours[1] = 0xff;
        pColours[2] = 0xff;
        pColours[3] = 0xff;
        pColours[4] = 0;
        pColours[5] = 0;
        pColours[6] = 0xff;
        break;
    default:
        pColours[0] = 0xff;
        pColours[1] = 0;
        pColours[2] = 0;
        pColours[3] = 0;
        pColours[4] = 0;
        pColours[5] = 0;
        pColours[6] = 0;
        break;
    }
    // the original leaves the scan by setting the counter to 7 (a `break` generates different code)
    for (i = 0; i < 7; i++) {
        if (pColours[i] != pApplied[i]) {
            changed = 1;
            i = 7;
        }
    }
    if (changed != 0) {
        CGraphics::BltTexture(pTexture, cacheSlot);
        CGraphics::RemapTextureAlpha(pTexture, 0x80, pColours[0], 0xe0, pColours[1], 0xd0, pColours[2], cacheSlot);
        CGraphics::RemapTextureAlpha(pTexture, 0xc0, pColours[3], 0xb0, pColours[4], 0xa0, pColours[5], cacheSlot);
        CGraphics::RemapTextureAlpha(pTexture, 0x90, pColours[6], 0x90, pColours[6], 0x90, pColours[6], cacheSlot);
        for (i = 0; i < 7; i++)
            pApplied[i] = pColours[i];
    }
}

// Fades two sets of five car-light channels towards their enable masks,
// repaints the cached body textures on change and stores the applied levels.
extern int g_stageObjectTimeStep;
// FUNCTION: CMR2 0x00477ce0
void StageObject_AnimateCarLightLevels(int car)
{
    CarLightTextureState *pRecord = &g_carLightTextureStates[car];
    short values[5];
    int changed = 0;
    int value;
    int i;

    for (i = 0; i < 5; i++) {
        if (0 != (pRecord->channelMasks[0] & (1 << i))) {
            value = pRecord->fadedLevels[0][i] + FixMul(0x80, g_stageObjectTimeStep);
            if (value > 0xff)
                value = 0xff;
        } else {
            value = pRecord->fadedLevels[0][i] - FixMul(0x50, g_stageObjectTimeStep);
            if (value < 0)
                value = 0;
        }
        pRecord->fadedLevels[0][i] = value;
        pRecord->levels[0][i] = value;
        if ((pRecord->channelMasks[1] & (1 << i)) != 0) {
            value = pRecord->fadedLevels[1][i] + FixMul(0x80, g_stageObjectTimeStep);
            if (value > 0xff)
                value = 0xff;
        } else {
            value = pRecord->fadedLevels[1][i] - FixMul(0x50, g_stageObjectTimeStep);
            if (value <= -1)
                value = 0;
        }
        pRecord->fadedLevels[1][i] = (BYTE)value;
        pRecord->levels[1][i] = (BYTE)value;
    }
    if (pRecord->combineFirstTwoChannels != 0) {
        value = pRecord->levels[0][1] + (pRecord->levels[0][0] / 3) * 2;
        if (value > 0xff)
            value = 0xff;
        pRecord->levels[0][1] = (BYTE)value;
        value = pRecord->levels[1][1] + (pRecord->levels[1][0] / 3) * 2;
        if (value > 0xff)
            value = 0xff;
        pRecord->levels[1][1] = (BYTE)value;
    } else {
        pRecord->levels[0][0] = (short)((pRecord->levels[0][0] / 3) * 2);
        pRecord->levels[1][0] = (short)((pRecord->levels[1][0] / 3) * 2);
    }
    for (i = 0; i < 5; i++) {
        if (pRecord->levels[1][i] != pRecord->appliedLevels[1][i] ||
            pRecord->levels[0][i] != pRecord->appliedLevels[0][i])
            changed = 1;
        if (pRecord->levels[1][i] >= pRecord->levels[0][i])
            values[i] = pRecord->levels[1][i];
        else
            values[i] = pRecord->levels[0][i];
    }
    if (changed != 0) {
        Texture **ppTextures = pRecord->textures;
        Texture *pTexture;

        for (i = 0; i < 2; i++) {
            pTexture = ppTextures[i];
            if (pTexture != NULL) {
                CGraphics::BltTexture(pTexture, car);
                CGraphics::RemapTextureAlpha(pTexture, 0xbf, values[1], 0x40, values[3], 0x80, values[2], car);
                CGraphics::RemapTextureAlpha(pTexture, 0x60, values[0], 0xe0, values[4], 0xe0, values[4], car);
            }
        }
        for (i = 0; i < 5; i++) {
            pRecord->appliedLevels[0][i] = pRecord->levels[0][i];
            pRecord->appliedLevels[1][i] = pRecord->levels[1][i];
        }
    }
}

// FUNCTION: CMR2 0x00477f30
void CarLight_InvalidateAppliedLevels(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        g_carLightTextureStates[i].appliedLevels[0][0] = -1;
        g_carLightTextureStates[i].appliedLevels[0][1] = -1;
        g_carLightTextureStates[i].appliedLevels[0][2] = -1;
        g_carLightTextureStates[i].appliedLevels[0][3] = -1;
        g_carLightTextureStates[i].appliedLevels[0][4] = -1;
        g_carLightTextureStates[i].appliedLevels[1][0] = -1;
        g_carLightTextureStates[i].appliedLevels[1][1] = -1;
        g_carLightTextureStates[i].appliedLevels[1][2] = -1;
        g_carLightTextureStates[i].appliedLevels[1][3] = -1;
        g_carLightTextureStates[i].appliedLevels[1][4] = -1;
    }
}

int StageObject_GetCarWeatherRampValue(Car *pCar);
char RallyData_GetUsableRecordCategory(BYTE param1);

// GLOBAL: CMR2 0x005909b8
int g_stageAmbientFlashLevel;
// GLOBAL: CMR2 0x005909c0
BYTE g_stageAmbientBaseColour[4];
// GLOBAL: CMR2 0x005909c4
BYTE g_stageAmbientFlashColour[4];

// Per car: ticks until the next headlight glow may be spawned.
// GLOBAL: CMR2 0x0058e4a8
int g_carHeadlightGlowCooldowns[8];
// Glow record of the stage objects: pRec offsets are relative to this base,
// i.e. 0x18 bytes below the glow fields that 0x47d510 walks.
// GLOBAL: CMR2 0x0058e4c8
CarHeadlightGlowRecord g_carHeadlightGlowRecords[100];

// One flying debris piece / headlight glow record (0x5c bytes, g_carHeadlightGlowRecords).

// Same records as 0x47d5a0 walks, seen from their position field (+0xc): the
// pointer arithmetic of that view lives in 0x47e1e0.

// Spawns the headlight glow of one stage object: finds the first free record,
// places it at the top corner of the car's bounding box, aims it along the
// body's right axis and drops it onto the ground below.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The instruction sequence is the original's; the residual difference is the
// register numbering of the vector temporaries.
// FUNCTION: CMR2 0x0047d5a0
void StageObject_SpawnCarHeadlightGlow(BYTE car)
{
    FixVector dir;
    FixVector half;
    Car *pCar;
    short surface;
    CarHeadlightGlowRecord *pRec;
    int i;

    if (g_carHeadlightGlowCooldowns[car] <= 0) {
        pRec = g_carHeadlightGlowRecords;
        i = 0;
        do {
            if (pRec->active == 0) {
                pCar = Car_Get(car);
                FixVecScale(&dir, &pCar->up, pCar->field_0x770[1]);
                half.x = pCar->corners[1].x - pCar->corners[0].x;
                half.y = pCar->corners[1].y - pCar->corners[0].y;
                half.z = pCar->corners[1].z - pCar->corners[0].z;
                FixVecScale(&half, &half, 0x8000);
                pRec->position.x = half.x + pCar->corners[0].x + dir.x;
                pRec->position.y = half.y + pCar->corners[0].y + dir.y;
                pRec->position.z = half.z + pCar->corners[0].z + dir.z;
                FixVecScale(&pRec->velocity, &pCar->right, 0xcccc);
                pRec->velocity.x += pCar->velocity.x;
                pRec->velocity.y += pCar->velocity.y;
                pRec->velocity.z += pCar->velocity.z;
                FixVecScale(&pRec->velocity, &pCar->right, FixVecDot(&pCar->right, &pRec->velocity));
                pRec->life = 0x320000;
                pRec->fade = 0x10000;
                pRec->active = 1;
                pRec->car = car;
                pRec->height = 0;
                pRec->triangle = -1;
                pRec->height = Track_GetGroundHeightSurface(&pRec->position, &pRec->normal, &pRec->triangle,
                                                            &surface, (unsigned short *)&surface, 0);
                pRec->position.y = pRec->height + 0x8000;
                pRec->prevPosition = pRec->position;
                pRec->prevNormal = pRec->normal;
                pRec->prevFade = pRec->fade;
                i = 100;
                g_carHeadlightGlowCooldowns[car] = 0x100000;
            }
            pRec++;
            i++;
        } while (i < 100);
    }
}


// Interpolates every headlight glow between its spawn record (the copy at
// +0x28/+0x34) and the current car state by the fraction t, normalises the
// direction and moves the light with it.
// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The addresses are the original's (checked against the emitting disassembly);
// MSVC materialises the record pointer 4 bytes higher and compensates with -4
// displacements, so every memory operand reads differently.
// FUNCTION: CMR2 0x0047e1e0
void StageObject_InterpolateHeadlightGlows(int t)
{
    FixVector pos;
    FixVector normal;
    FixVector delta;
    FixVector ground;
    int *pRec;
    int length;
    int size;
    int i;

    pRec = &g_carHeadlightGlowRecords[0].position.x;
#define GLOW_RECORD (*(CarHeadlightGlowRecord *)((BYTE *)pRec - offsetof(CarHeadlightGlowRecord, position)))
    i = 100;
    do {
        if (GLOW_RECORD.active != 0) {
            delta.x = GLOW_RECORD.position.x - GLOW_RECORD.prevPosition.x;
            delta.y = GLOW_RECORD.position.y - GLOW_RECORD.prevPosition.y;
            delta.z = GLOW_RECORD.position.z - GLOW_RECORD.prevPosition.z;
            FixVecScale(&delta, &delta, t);
            pos.x = delta.x + GLOW_RECORD.prevPosition.x;
            pos.y = delta.y + GLOW_RECORD.prevPosition.y;
            pos.z = delta.z + GLOW_RECORD.prevPosition.z;
            delta.x = GLOW_RECORD.normal.x - GLOW_RECORD.prevNormal.x;
            delta.y = GLOW_RECORD.normal.y - GLOW_RECORD.prevNormal.y;
            delta.z = GLOW_RECORD.normal.z - GLOW_RECORD.prevNormal.z;
            FixVecScale(&delta, &delta, t);
            normal.x = delta.x + GLOW_RECORD.prevNormal.x;
            normal.y = delta.y + GLOW_RECORD.prevNormal.y;
            normal.z = delta.z + GLOW_RECORD.prevNormal.z;
            length = FixVecLength(&normal);
            if (length == 0) {
                normal.x = 0;
                normal.y = 0;
                normal.z = 0;
            } else {
                FixVecScaleRecip(&normal, &normal, length);
            }
            size = FixMul(GLOW_RECORD.fade - GLOW_RECORD.prevFade, t) + GLOW_RECORD.prevFade;
            if (size > 0) {
                Glow_SetEnabled(GLOW_RECORD.pGlow, 1);
                Glow_SetIntensity(GLOW_RECORD.pGlow, size);
                Glow_SetPosition(GLOW_RECORD.pGlow, &pos, &pos);
                ground = pos;
                ground.y -= 0x8000;
                Glow_SetLayerPlane(GLOW_RECORD.pGlow, &ground, &normal, 0);
            } else {
                Glow_SetEnabled(GLOW_RECORD.pGlow, 0);
            }
        } else {
            Glow_SetEnabled(GLOW_RECORD.pGlow, 0);
        }
        pRec += sizeof(CarHeadlightGlowRecord) / sizeof(*pRec);
    } while (--i);
#undef GLOW_RECORD
}

// FUNCTION: CMR2 0x0047e490
void StageObject_SetVehicleEffectColour(BYTE *pColour)
{
    g_stageAmbientBaseColour[0] = pColour[0];
    g_stageAmbientBaseColour[1] = pColour[1];
    g_stageAmbientBaseColour[2] = pColour[2];
    g_stageAmbientFlashColour[0] = 0xff;
    g_stageAmbientFlashColour[1] = 0xff;
    g_stageAmbientFlashColour[2] = 0xff;
    g_stageAmbientFlashLevel = 0;
}

// Fades the stage ambient colour towards the base colour while the 0x5909b8
// timer runs out, then applies it with one step of boost.
// match 72%: same logic; MSVC puts the length/scale locals in the other stack
// slots and swaps the addend order
// match 72%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00480220
void StageObject_FadeAmbientColourToBase(void)
{
    BYTE colour[4];
    int length;
    int scale;
    int scale_2;

    length = FixSqrt(g_stageAmbientFlashLevel);
    scale = (g_stageAmbientFlashColour[0] & 0xff) << 16;
    scale = (FixMul(scale, length) >> 16) + (g_stageAmbientBaseColour[0] & 0xff);
    if (scale > 0xff)
        scale = 0xff;
    colour[0] = scale;
    scale_2 = (g_stageAmbientFlashColour[1] & 0xff) << 16;
    scale_2 = (FixMul(scale_2, length) >> 16) + (g_stageAmbientBaseColour[1] & 0xff);
    if (scale_2 > 0xff)
        scale_2 = 0xff;
    colour[1] = scale_2;
    scale_2 = (g_stageAmbientFlashColour[2] & 0xff) << 16;
    scale_2 = (FixMul(scale_2, length) >> 16) + (g_stageAmbientBaseColour[2] & 0xff);
    if (scale_2 > 0xff)
        scale_2 = 0xff;
    colour[2] = scale_2;
    g_stageAmbientFlashLevel -= 0x8000;
    if (g_stageAmbientFlashLevel < 0)
        g_stageAmbientFlashLevel = 0;
    Scene_SetAmbient(colour, 1);
}

// Selects a list of 0x6c-byte records (count first); returns whether it is non-empty.
// FUNCTION: CMR2 0x0048caa0
int StageObject_SelectNonemptyRecordList(int *pList)
{
    unsigned int count = 0;

    if (pList != NULL) {
        count = g_unk0x005918c8 = *pList;
        g_unk0x00591750 = (CameraSpot *)(pList + 1);
    } else {
        g_unk0x005918c8 = count;
        g_unk0x00591750 = NULL;
    }
    return count > 0;
}

// Whether a car's wheel sits on a surface of kind 0, 3, 12, 13 or 26 while StageObject_GetCarWeatherRampValue > 0.
// FUNCTION: CMR2 0x0046eeb0
BYTE StageObject_IsWheelOnActiveEffectSurface(int index, int wheel)
{
    BYTE result;
    Car *pCar;

    result = 0;
    pCar = Car_Get(index);
    switch (pCar->wheelSurface[wheel]) {
    case 0:
    case 3:
    case 0xc:
    case 0xd:
    case 0x1a:
        if (StageObject_GetCarWeatherRampValue(pCar) > 0)
            result = 1;
    }
    return result;
}

// Fraction (0..1.0) of the spot's dolly track the camera has moved along.
// GLOBAL: CMR2 0x00591730
int g_tracksideDollyProgress[4];

// Moves a trackside camera along its dolly track (progress clamped to 1.0).
// FUNCTION: CMR2 0x0048dca0
void View_AdvanceTracksideDolly(CameraRecord *pRecord, int amount)
{
    BYTE car;

    if (amount > 0) {
        car = pRecord->index;
        if ((g_tracksideDollyProgress[car] += amount) > 0x10000)
            g_tracksideDollyProgress[car] = 0x10000;
    }
}

// Lead of a trackside camera's aim ahead of the car: the reference right
// vector minus the car's (normalised) velocity direction, scaled by the car
// speed and the spot's speedScale.
// FUNCTION: CMR2 0x0048dce0
void View_BuildTracksideAimLead(FixVector *pOut, CameraRecord *pRecord, Car *pCar, FixMatrix *pMatrix)
{
    FixVector direction;
    FixVector right;
    int length;
    int scale;
    int speed;

    direction = pCar->velocity;
    speed = pCar->speed;
    FixMatrix_GetRight(&right, pMatrix);
    if (speed < 0x28f) {
        direction.x = 0;
        direction.y = 0;
        direction.z = 0;
    } else {
        length = FixVecLength(&direction);
        if (length == 0) {
            direction.x = 0;
            direction.y = 0;
            direction.z = 0;
        } else {
            FixVecScaleRecip(&direction, &direction, length);
        }
        if (FixVecDot(&right, &direction) < 0) {
            direction.x = -direction.x;
            direction.y = -direction.y;
            direction.z = -direction.z;
        }
    }
    pOut->x = right.x - direction.x;
    pOut->y = right.y - direction.y;
    pOut->z = right.z - direction.z;
    scale = FixMul(pCar->speed, g_unk0x00591750[g_viewCameraSpot[pRecord->index]].speedScale);
    FixVecScale(pOut, pOut, scale);
}

// match 83%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0048df10
int StageObject_IsNegativeRightAngleCameraSpot(CameraRecord *pRecord)
{
    if (pRecord->type == 7) {
        CameraSpot *pSpot = &g_unk0x00591750[g_viewCameraSpot[pRecord->index]];
        if (pSpot->heading < -0x3f4 && pSpot->heading > -0x40b)
            return 1;
    }
    return 0;
}

// FUNCTION: CMR2 0x00486b90
void CarCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther)
{
    int kind;
    int value;

    kind = pOther->type;
    if (kind != 2 && kind != 1 && kind != 10)
        value = 0;
    else
        value = 0x10000;
    g_viewCarLift[pRecord->index] = value;
}

int Knockout_HasHumanLostMatch(KnockoutMatch *pMatch);

// Returns 1 when a match of the current knockout round is still undecided.
// FUNCTION: CMR2 0x00473290
int Knockout_HasUndecidedRoundMatch(void)
{
    unsigned int *pState;
    KnockoutMatch *pMatch = NULL;
    int count = 0;
    int i;

    pState = RallyData_GetChampionshipState();
    switch ((*pState >> 3) & 7) {
    case 1:
        count = 8;
        pMatch = (KnockoutMatch *)(pState + 0x16);
        break;
    case 2:
        count = 4;
        pMatch = (KnockoutMatch *)(pState + 10);
        break;
    case 3:
        count = 2;
        pMatch = (KnockoutMatch *)(pState + 4);
        break;
    case 4:
        count = 1;
        pMatch = (KnockoutMatch *)(pState + 1);
    }
    for (i = 0; i < count; i++, pMatch++) {
        if (Knockout_HasHumanLostMatch(pMatch))
            return 1;
    }
    return 0;
}

// Whether both drivers of the current round are known.
// FUNCTION: CMR2 0x00473310
int Knockout_AreCurrentDriversKnown(void)
{
    unsigned int first;
    unsigned int second;

    RallyData_GetChampionshipState();
    RallyData_GetRoundDrivers(&first, &second);
    if (RallyData_GetUsableRecordCategory((BYTE)first) != -1 && RallyData_GetUsableRecordCategory((BYTE)second) != -1)
        return 1;
    return 0;
}

// GLOBAL: CMR2 0x00591490
int g_collisionSphereRadius;
// GLOBAL: CMR2 0x00591494
int g_collisionRecordHeight;
// GLOBAL: CMR2 0x00591498
FixVector g_collisionSphereCentre;

// True when two spheres (radii r1, r2) overlap.
// match 51%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// True when two spheres (radii r1, r2) overlap. The 32-bit EAX result is
// tested by the callers (0x48a1f0), so the helper returns int, not bool.
// match 81%: identical instruction stream and stack layout except that the
// original kept the radius sum in a register with no home slot (its FixMul
// operands spill into the dead r1/r2 argument slots) while MSVC6 here homes
// it at [ebp-4], shifting the frame by four bytes.
// FUNCTION: CMR2 0x00487b80
int Collision_DoSpheresOverlap(int r1, int r2, int *pA, int *pB)
{
    FixVector delta;
    int r;

    r = r1 + r2;
    delta.x = pA[0] - pB[0];
    delta.y = pA[1] - pB[1];
    delta.z = pA[2] - pB[2];

    if ((delta.x < 0 ? -delta.x : delta.x) <= r && (delta.y < 0 ? -delta.y : delta.y) <= r &&
        (delta.z < 0 ? -delta.z : delta.z) <= r) {
        int dot = FixVecDot(&delta, &delta);
        r2 = r;
        r1 = r;
        return dot < FixMul(r1, r2);
    }
    return 0;
}

// Selects the sphere of an object without a box projection, retaining the
// first mesh collision record's height in the otherwise unread height cache.
// FUNCTION: CMR2 0x00487e00
void Collision_SetObjectSphere(FixVector *pPosition, SectorObjectEntry *pEntry)
{
    g_collisionSphereCentre = *pPosition;
    g_collisionSphereRadius = pEntry->pBounds->radius;
    g_collisionRecordHeight = pEntry->pObject->pMesh->pCollisionRecords->height;
}

// Applies the car's corner-height/slip update and velocity damping when the
// object sphere passes the car box's expanded horizontal overlap test.
// The original hoists both tolerance'd limits before testing the absolute
// values, which is what the two named locals reproduce.
// FUNCTION: CMR2 0x00487e50
void Collision_ApplyObjectOverlapDamping(CollisionBox *pBox, Car *pCar)
{
    FixVector delta;
    int u;
    int v;

    delta.x = g_collisionSphereCentre.x - pBox->pVertex->x;
    delta.y = g_collisionSphereCentre.y - pBox->pVertex->y;
    delta.z = g_collisionSphereCentre.z - pBox->pVertex->z;
    delta.y = 0;
    u = FixVecDot(&delta, &pBox->axisA);
    v = FixVecDot(&delta, &pBox->axisB);
    if (FIX_ABS(u) > pBox->halfWidth && FIX_ABS(v) > pBox->halfLength)
        return;
    {
        int limit0;
        int limit1;
        limit0 = pBox->halfWidth + g_collisionSphereRadius;
        limit1 = pBox->halfLength + g_collisionSphereRadius;
        if (FIX_ABS(u) > limit0 || FIX_ABS(v) > limit1)
            return;
    }
    CarPhysics_UpdateWheelSlipAndVelocityDamping(pCar);
}

// GLOBAL: CMR2 0x0051fadc
BYTE g_unk0x0051fadc[4] = { 0, 1, 3, 2 };
// GLOBAL: CMR2 0x00590ec8
BYTE g_unk0x00590ec8[4];
// GLOBAL: CMR2 0x00590ecc
char g_unk0x00590ecc[4];
// GLOBAL: CMR2 0x005914a4
BYTE g_unk0x005914a4[4];
// GLOBAL: CMR2 0x005914c4
char g_unk0x005914c4[4];
// GLOBAL: CMR2 0x005914d4
char g_unk0x005914d4;
// GLOBAL: CMR2 0x005915f4
char g_unk0x005915f4;
// Box of the stage object being collided with (0x98 bytes: half sizes, axes,
// corners from +0x30, the car offset at +0x94).
// GLOBAL: CMR2 0x005915f8
CollisionBox g_sectorObjectCollisionBox;

// Finds the closest overlap between two 4-corner boxes along the horizontal
// direction `pDir`, testing every pair of edges of their corner quads. Appends
// the winning contact (edge orientation 0=horizontal/2=vertical plus the corner
// index) to the A or B side list and returns 1; returns 0 if nothing overlaps.
// FUNCTION: CMR2 0x00488de0
int Collision_FindQuadEdgeOverlap(FixVector *pVertsA, FixVector *pVertsB, FixVector *pDir, int *pDistance)
{
    BYTE *pNext;
    BYTE corner;
    BYTE cornerVert;
    BYTE edge;
    int rayEdge;
    int best;
    int found;
    int dist;
    int i;
    int j;

    corner = 0;
    edge = 0;
    found = 0;
    best = -0x640000;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            g_collisionQuad[0] = pVertsA[g_unk0x0051fadc[i] + 4];
            g_collisionQuad[1] = pVertsA[g_unk0x0051fadc[(i + 1) % 4] + 4];
            g_collisionQuad[2] = pVertsB[g_unk0x0051fadc[j] + 4];
            g_collisionQuad[3] = pVertsB[g_unk0x0051fadc[(j + 1) % 4] + 4];
            dist = Collision_RayQuad(pDir, &rayEdge, &corner);
            if (dist != 0x7d000000 && dist > best) {
                best = dist;
                switch (corner & 0xff) {
                case 0:
                    cornerVert = g_unk0x0051fadc[i];
                    found = 1;
                    edge = (abs(g_unk0x0051fadc[j] - g_unk0x0051fadc[(j + 1) % 4]) == 1) ? 0 : 2;
                    break;
                case 1:
                    cornerVert = g_unk0x0051fadc[(i + 1) % 4];
                    found = 1;
                    edge = (abs(g_unk0x0051fadc[j] - g_unk0x0051fadc[(j + 1) % 4]) == 1) ? 0 : 2;
                    break;
                case 2:
                    cornerVert = g_unk0x0051fadc[j];
                    found = 0;
                    edge = (abs(g_unk0x0051fadc[i] - g_unk0x0051fadc[(i + 1) % 4]) == 1) ? 0 : 2;
                    break;
                case 3:
                    cornerVert = g_unk0x0051fadc[(j + 1) % 4];
                    found = 0;
                    edge = (abs(g_unk0x0051fadc[i] - g_unk0x0051fadc[(i + 1) % 4]) == 1) ? 0 : 2;
                    break;
                }
            }
        }
    }
    if (best >= 0) {
        if (found != 0) {
            int index = g_unk0x005915f4;
            g_unk0x00590ec8[index] = edge;
            g_unk0x005914c4[index] = cornerVert;
            g_unk0x005915f4++;
            *pDistance = best;
            return 1;
        }
        {
            int index = g_unk0x005914d4;
            g_unk0x005914a4[index] = edge;
            g_unk0x00590ecc[index] = cornerVert;
            g_unk0x005914d4++;
            *pDistance = best;
            return 1;
        }
    }
    return 0;
}

extern int g_stageObjectScale;
extern int g_stageObjectTimeStep;

// Sets the stage effects' scale (the frame rate `value`, 16.16, divided by 25
// and at least 0.6) and their time step (its reciprocal). Physics_UpdateRateHold
// passes the rate of every frame. A frame under a millisecond has an infinite
// rate (Graphics_UpdateFrameStatistics), which fistp turns into a negative
// value and so the longest step, that of 15 frames a second.
// FUNCTION: CMR2 0x00466630
void StageObject_SetFrameRate(int value)
{
    g_stageObjectScale = FixMul(0xa3d, value);
    if (g_stageObjectScale < 0x9999)
        g_stageObjectScale = 0x9999;
    g_stageObjectTimeStep = FixDiv(0x10000, g_stageObjectScale);
}

extern FixVector g_stageDeformHull[12];
extern FixVector g_stageDeformOffset;
extern FixVector g_stageDeformNormal;
extern FixVector g_stageDeformImpact;
extern int g_stageDeformSpeed;
extern int g_stageDeformStrength;
extern int g_stageDeformMode;

// Rebuilds the deformation hull from the part set's X/Z bounds. The first
// four vertices use those bounds at the car floor; the next four are raised
// by the car bounds, and the last four are clamped by the car scales.
// The second argument is a CarPartSet, as confirmed by all three callers.
// Vertices 4 and 5 are raised by field_0x770[1], 6 and 7 by field_0x770[0].
// FUNCTION: CMR2 0x004675c0
void CarDamage_BuildRelativeVelocityHull(Car *pCar, CarPartSet *pParts)
{
    int v;

    g_stageDeformHull[0].x = pParts->maxX;
    g_stageDeformHull[0].y = -pCar->halfExtents.y;
    g_stageDeformHull[0].z = pParts->maxZ;
    g_stageDeformHull[1].x = pParts->maxX;
    g_stageDeformHull[1].y = -pCar->halfExtents.y;
    g_stageDeformHull[1].z = pParts->minZ;
    g_stageDeformHull[2].x = pParts->minX;
    g_stageDeformHull[2].y = -pCar->halfExtents.y;
    g_stageDeformHull[2].z = pParts->maxZ;
    g_stageDeformHull[3].x = pParts->minX;
    g_stageDeformHull[3].y = -pCar->halfExtents.y;
    g_stageDeformHull[3].z = pParts->minZ;
    for (v = 0; v < 4; v++) {
        g_stageDeformHull[v + 4] = g_stageDeformHull[v];
        if (v < 2)
            g_stageDeformHull[v + 4].y += pCar->field_0x770[1];
        else
            g_stageDeformHull[v + 4].y += pCar->field_0x770[0];
        g_stageDeformHull[v + 8] = pCar->upperCornersLocal[v];
        if (g_stageDeformHull[v + 8].x > 0)
            g_stageDeformHull[v + 8].x -= pCar->scale0x764;
        else
            g_stageDeformHull[v + 8].x += pCar->scale0x768;
        if (g_stageDeformHull[v + 8].z > 0)
            g_stageDeformHull[v + 8].z -= pCar->scale0x76c;
        else
            g_stageDeformHull[v + 8].z += pCar->scale0x76c;
    }
}

// Decodes one packed impact into the car dent position, normal, axis, strength
// and mode; the mode 1 radius is expanded from its unsigned quantised byte.
// match 89%: the original folds the 0x10000<<16 division into a plain IDIV in
// the two later scale blocks, and stores the mode as a byte (declared int in
// StageTiming.cpp)
// FUNCTION: CMR2 0x004688b0
void CarDamage_DecodeImpactPayload(CarImpactPayload *p)
{
    g_stageDeformOffset.x = (char)p->position[0] << 16;
    g_stageDeformOffset.y = (char)p->position[1] << 16;
    g_stageDeformOffset.z = (char)p->position[2] << 16;
    FixVecScale(&g_stageDeformOffset, &g_stageDeformOffset,
                FixMul(0xa0000, FixDiv(0x10000, 0x7f0000)));
    g_stageDeformNormal.x = (char)p->normal[0] << 16;
    g_stageDeformNormal.y = (char)p->normal[1] << 16;
    g_stageDeformNormal.z = (char)p->normal[2] << 16;
    FixVecScaleRecip(&g_stageDeformNormal, &g_stageDeformNormal, 0x7f0000);
    g_stageDeformImpact.x = (char)p->axis[0] << 16;
    g_stageDeformImpact.y = (char)p->axis[1] << 16;
    g_stageDeformImpact.z = (char)p->axis[2] << 16;
    FixVecScaleRecip(&g_stageDeformImpact, &g_stageDeformImpact, 0x7f0000);
    g_stageDeformStrength = (BYTE)p->strength << 16;
    g_stageDeformStrength = FixDiv(g_stageDeformStrength, 0xff0000);
    *(BYTE *)&g_stageDeformMode = p->mode;
    if (p->mode == 1) {
        g_stageDeformSpeed = (BYTE)p->axisRadius << 16;
        g_stageDeformSpeed = FixMul(g_stageDeformSpeed, FixMul(0xa0000, FixDiv(0x10000, 0xff0000)));
    }
}

extern CarPartSet *g_carPartSets;

int CarDamage_AverageVertexDisplacement(Car *pCar, CarPartSet *set);

// Rebuilds damageValues of the car's part set from its damage grid, applies
// damageScales and damageBiases, then
// recomputes the derived torques and scales (0x3d8..0x408).
// FUNCTION: CMR2 0x00468c10
void StageObject_RebuildDamagePartValues(Car *pCar)
{
    CarPartSet *pRecord;
    int i;
    int value;

    pRecord = g_carPartSets + pCar->index;
    if (pCar->field_0xb50 == 0)
        return;
    pRecord->damageValues[0] = pRecord->damageGrid[2][0];
    pRecord->damageValues[1] = pRecord->damageGrid[0][0];
    pRecord->damageValues[2] = pRecord->damageGrid[2][2];
    pRecord->damageValues[3] = pRecord->damageGrid[0][2];
    pRecord->damageValues[5] = pRecord->damageGrid[0][0];
    pRecord->damageValues[4] = pRecord->damageGrid[2][0];
    pRecord->damageValues[6] = pRecord->damageGrid[2][0];
    pRecord->damageValues[7] = pRecord->damageGrid[0][0];
    pRecord->damageValues[8] = pRecord->damageGrid[2][2];
    pRecord->damageValues[9] = pRecord->damageGrid[0][2];
    pRecord->damageValues[10] = pRecord->damageGrid[2][0];
    pRecord->damageValues[11] = pRecord->damageGrid[0][0];
    pRecord->damageValues[12] = pRecord->damageGrid[2][2];
    pRecord->damageValues[13] = pRecord->damageGrid[0][2];
    pRecord->damageValues[14] = pRecord->damageGrid[1][0];
    pRecord->damageValues[15] = pRecord->damageGrid[1][0];
    pRecord->damageValues[16] = FixMul(pRecord->damageGrid[1][1] + pRecord->damageGrid[1][0] +
                                       pRecord->damageGrid[1][2], 0x5553);
    pRecord->damageValues[19] = pRecord->damageGrid[1][0];
    pRecord->damageValues[18] = pRecord->damageGrid[1][2];
    pRecord->damageValues[20] = pRecord->damageGrid[1][2];
    pRecord->damageValues[21] = pRecord->damageGrid[1][2];
    pRecord->damageValues[24] = pRecord->damageGrid[1][2];
    pRecord->damageValues[22] = pRecord->damageGrid[1][0];
    pRecord->damageValues[23] = pRecord->damageGrid[1][0];
    pRecord->damageValues[17] = 0;
    pRecord->damageValues[25] = FixMul(pRecord->damageGrid[1][2] + pRecord->damageGrid[1][0] +
                                       pRecord->damageGrid[1][1], 0x5553);
    pRecord->damageValues[26] = pRecord->damageGrid[1][0];
    pRecord->damageValues[27] = pRecord->damageGrid[1][2];
    pRecord->damageValues[28] = pRecord->damageGrid[2][1];
    pRecord->damageValues[29] = pRecord->damageGrid[0][1];
    pRecord->damageValues[30] = pRecord->damageGrid[2][2];
    pRecord->damageValues[31] = pRecord->damageGrid[0][2];
    pRecord->damageValues[32] = pRecord->damageGrid[2][0];
    pRecord->damageValues[33] = pRecord->damageGrid[0][0];
    if (pCar->field_0xb7c == 0)
        pRecord->damageValues[15] = 0;
    if (pCar->field_0xb80 == 0)
        pRecord->damageValues[16] = 0;
    for (i = 0; i < 0x22; i++) {
        pRecord->damageValues[i] = FixMul(pRecord->damageValues[i], pRecord->damageScales[i]);
        pRecord->damageValues[i] = pRecord->damageValues[i] + pRecord->damageBiases[i];
        if (pRecord->damageValues[i] > 0x10000)
            pRecord->damageValues[i] = 0x10000;
    }
    pRecord->engineTorqueScale = 0x10000 - FixMul(pRecord->damageValues[25], 0x2666) -
                                FixMul(pRecord->damageValues[15], 0x1333) -
                                FixMul(pRecord->damageValues[24], 0x666);
    pRecord->suspensionDamageOffset[0] = FixMul(pRecord->damageValues[6], FixMul(0x3333, 0xffff0000));
    pRecord->suspensionDamageOffset[1] = FixMul(pRecord->damageValues[7], FixMul(0x3333, 0xffff0000));
    pRecord->suspensionDamageOffset[2] = FixMul(pRecord->damageValues[8], FixMul(0x3333, 0x8000));
    pRecord->suspensionDamageOffset[3] = FixMul(pRecord->damageValues[9], FixMul(0x3333, 0xffff8000));
    pRecord->steeringWobble = FixMul(pRecord->damageValues[4] * 2, 0x8000);
    pRecord->steeringWobble = FixMul(pRecord->steeringWobble, 0xa0000);
    pRecord->frontBrakeScale = 0x10000 - FixMul(FixMul(pRecord->damageValues[10] +
                                                         pRecord->damageValues[11], 0x8000), 0x3333);
    pRecord->rearBrakeScale = 0x10000 - FixMul(FixMul(pRecord->damageValues[12] +
                                                         pRecord->damageValues[13], 0x8000), 0x3333);
    for (i = 0; i < 4; i++) {
        pRecord->wheelDamageDrag[i] = FixMul(pRecord->damageValues[i], 0xccc);
    }
    if (pCar->baseDriveSplit != 0x10000 && pCar->baseDriveSplit != 0) {
        pCar->driveSplit = FixMul(0x8000, pRecord->damageValues[16]) + pCar->baseDriveSplit;
        value = pCar->driveSplit;
        if (value > 0x10000)
            pCar->driveSplit = 0x10000;
    }
    pRecord->gearShiftDamage = (char)FixMulShift32(pRecord->damageValues[14], 0xf0000);
    value = CarDamage_AverageVertexDisplacement(pCar, pRecord);
    pRecord->bodyDamageDrag = FixMul(0x4000, value);
    if (CarPart_GetHiddenState(pCar, 3) != 0)
        pRecord->bodyDamageDrag = pRecord->bodyDamageDrag + -0x3333;
    pRecord->damageValues[17] = value;
    if (value > 0x10000)
        pRecord->damageValues[17] = 0x10000;
}

// Adds `amount` to the 3x3 grid at +0x21c of the car's 0x4d0-byte record,
// weighted by how close each grid point is to the car's contact offsets
// (0x5dc/0x5e4).
// match 50%: same logic; MSVC6 kept the car pointer in EDI and the FixMul
// temporaries in the parameter slots instead of the slots we get.
// FUNCTION: CMR2 0x00468a80
void StageObject_ApplyWeightedContactDamage(Car *pCar, int amount)
{
    CarPartSet *set;
    int *pGrid;
    int *pElem;
    int row;
    int col;
    int xOff;
    int yOff;
    int halfAmount;
    int xStep;
    int yStep;
    int xLimit;
    int yLimit0;
    int yLimit1;
    int colLimit;

    set = g_carPartSets + pCar->index;
    halfAmount = FixMul(amount, 0x8000);
    amount = FixMul(amount, 0x3333);
    xStep = FixMul(pCar->halfExtents.x, 0xaac0);
    yStep = FixMul(pCar->halfExtents.z, 0xc000);
    xLimit = FixMul(pCar->halfExtents.x, 0x553f) + halfAmount;
    yLimit0 = FixMul(pCar->halfExtents.z, 0x4000) + halfAmount;
    yLimit1 = FixMul(pCar->halfExtents.z, 0x8000) + halfAmount;
    yOff = -yStep;
    pGrid = &set->damageGrid[0][0];
    for (row = 0; row < 3; row++) {
        if (row == 1)
            colLimit = yLimit1;
        else
            colLimit = yLimit0;
        xOff = xStep;
        pElem = pGrid;
        for (col = 0; col < 3; col++, pGrid++) {
            if (FIX_ABS(pCar->field_0x5dc.x - xOff) <= xLimit && FIX_ABS(pCar->field_0x5dc.z - yOff) <= colLimit) {
                *pElem += amount;
                if (*pElem > 0x640000)
                    *pElem = 0x640000;
            }
            xOff -= xStep;
            pElem++;
        }
        yOff += yStep;
    }
}
// Averages (16.16) the per-object distance between every stage object's float
// vertex data and its fixed-point copy, skipping parts 1 and 3 when the record
// flag is set (their object count still feeds the divisor).
// FUNCTION: CMR2 0x00469100
int CarDamage_AverageVertexDisplacement(Car *pCar, CarPartSet *set)
{
    int partA;
    int partB;
    int total;
    int sum;
    int i;
    int j;
    int dx;
    int dy;
    int dz;
    int dist;

    if (g_carDamageModelEnabled[pCar->index] == 0)
        return 0;
    partA = CarPart_GetModelIndex(1, pCar->index);
    partB = CarPart_GetModelIndex(3, pCar->index);
    total = 0;
    sum = 0;
    for (i = 0; i < set->count; i++) {
        if (i == partA) {
            if (CarPart_GetHiddenState(pCar, 1) != 0)
                goto skip;
        } else if (i == partB && CarPart_GetHiddenState(pCar, 3) != 0) {
skip:
            total += set->vertexCount[i];
            continue;
        }
        {
            for (j = 0; j < set->vertexCount[i]; j++) {
                dx = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[0], CGraphics::m_65536)) -
                     set->geometry.vertices[i][j].pos.x;
                dy = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[1], CGraphics::m_65536)) -
                     set->geometry.vertices[i][j].pos.y;
                dz = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[2], CGraphics::m_65536)) -
                     set->geometry.vertices[i][j].pos.z;
                dist = FIX_ABS(dx) + FIX_ABS(dy) + FIX_ABS(dz);
                dist = FixMul(dist, 0x50000);
                if (dist > 0x10000)
                    dist = 0x10000;
                sum += dist;
                total++;
            }
        }
    }
    return FixDiv(sum, total << 16);
}

// Sets the fixed-point lighting values for both stage weather conditions.
// match 24%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00460da0
void StageObject_SetLighting(const StageLightPreset *pPrimary, const StageLightPreset *pSecondary)
{
    Stage_InitLightMeshes();
    g_stageLighting.lightningSector = 0xffff;
    int *pWeatherPair = RallyData_GetDriverSettingPair(RallyDataStageIndex());
    g_stageLighting.primary.intensity = g_stageWeatherIntensity[pWeatherPair[0]];
    g_stageLighting.secondary.intensity = g_stageWeatherIntensity[pWeatherPair[1]];
    if (pPrimary != NULL && pSecondary != NULL) {
        g_stageLighting.primary.meshHeightParameters.x = pPrimary->meshHeightParameters.x;
        g_stageLighting.primary.meshHeightParameters.y = pPrimary->meshHeightParameters.y;
        g_stageLighting.primary.meshHeightParameters.z = pPrimary->meshHeightParameters.z;
        g_stageLighting.secondary.meshHeightParameters.x = pSecondary->meshHeightParameters.x;
        g_stageLighting.secondary.meshHeightParameters.y = pSecondary->meshHeightParameters.y;
        g_stageLighting.secondary.meshHeightParameters.z = pSecondary->meshHeightParameters.z;
        Track_ShiftMeshAndAmbientHeights(pPrimary->meshHeightParameters.x, pPrimary->meshHeightParameters.y, pPrimary->meshHeightParameters.z);
        StageObject_SetStageLightState(0x10000);
        {
            unsigned int r = pPrimary->colour[0][0];
            unsigned int g = pPrimary->colour[0][1];
            unsigned int b = pPrimary->colour[0][2];
            g_stageLighting.primary.lowColour.x = (unsigned int)pPrimary->colour[1][0] << 0x10;
            g_stageLighting.primary.lowColour.y = (unsigned int)pPrimary->colour[1][1] << 0x10;
            g_stageLighting.primary.lowColour.z = (unsigned int)pPrimary->colour[1][2] << 0x10;
            g_stageLighting.primary.highColourDelta.x = (r << 0x10) - g_stageLighting.primary.lowColour.x;
            g_stageLighting.primary.highColourDelta.y = (g << 0x10) - g_stageLighting.primary.lowColour.y;
            g_stageLighting.primary.highColourDelta.z = (b << 0x10) - g_stageLighting.primary.lowColour.z;
        }
        g_stageLighting.secondary.lowColour.x = (unsigned int)pSecondary->colour[1][0] << 0x10;
        g_stageLighting.secondary.lowColour.y = (unsigned int)pSecondary->colour[1][1] << 0x10;
        g_stageLighting.secondary.lowColour.z = (unsigned int)pSecondary->colour[1][2] << 0x10;
        g_stageLighting.secondary.highColourDelta.x = ((unsigned int)pSecondary->colour[0][0] << 0x10) - g_stageLighting.secondary.lowColour.x;
        g_stageLighting.secondary.highColourDelta.y = ((unsigned int)pSecondary->colour[0][1] << 0x10) - g_stageLighting.secondary.lowColour.y;
        g_stageLighting.secondary.highColourDelta.z = ((unsigned int)pSecondary->colour[0][2] << 0x10) - g_stageLighting.secondary.lowColour.z;
        g_stageLighting.primary.heightReferenceColour.x = (unsigned int)pPrimary->colour[2][0] << 0x10;
        g_stageLighting.primary.heightReferenceColour.y = (unsigned int)pPrimary->colour[2][1] << 0x10;
        g_stageLighting.primary.heightReferenceColour.z = (unsigned int)pPrimary->colour[2][2] << 0x10;
        g_stageLighting.secondary.heightReferenceColour.x = (unsigned int)pSecondary->colour[2][0] << 0x10;
        g_stageLighting.secondary.heightReferenceColour.y = (unsigned int)pSecondary->colour[2][1] << 0x10;
        g_stageLighting.secondary.heightReferenceColour.z = (unsigned int)pSecondary->colour[2][2] << 0x10;
        g_stageLighting.primary.heightReferenceBlend = FixDiv((int)pPrimary->colour[2][3] << 16, 0xff0000);
        g_stageLighting.secondary.heightReferenceBlend = FixDiv((int)pSecondary->colour[2][3] << 16, 0xff0000);
        g_stageLighting.primary.sunIconColour.x = (unsigned int)pPrimary->colour[8][0] << 0x10;
        g_stageLighting.primary.sunIconColour.y = (unsigned int)pPrimary->colour[8][1] << 0x10;
        g_stageLighting.primary.sunIconColour.z = (unsigned int)pPrimary->colour[8][2] << 0x10;
        g_stageLighting.secondary.sunIconColour.x = (unsigned int)pSecondary->colour[8][0] << 0x10;
        g_stageLighting.secondary.sunIconColour.y = (unsigned int)pSecondary->colour[8][1] << 0x10;
        g_stageLighting.secondary.sunIconColour.z = (unsigned int)pSecondary->colour[8][2] << 0x10;
        g_stageLighting.primary.sunIconSize = pPrimary->sunIconSize;
        g_stageLighting.secondary.sunIconSize = pSecondary->sunIconSize;
        g_stageLighting.primary.mesh5Colour.x = (unsigned int)pPrimary->colour[6][0] << 0x10;
        g_stageLighting.primary.mesh5Colour.y = (unsigned int)pPrimary->colour[6][1] << 0x10;
        g_stageLighting.primary.mesh5Colour.z = (unsigned int)pPrimary->colour[6][2] << 0x10;
        g_stageLighting.secondary.mesh5Colour.x = (unsigned int)pSecondary->colour[6][0] << 0x10;
        g_stageLighting.secondary.mesh5Colour.y = (unsigned int)pSecondary->colour[6][1] << 0x10;
        g_stageLighting.secondary.mesh5Colour.z = (unsigned int)pSecondary->colour[6][2] << 0x10;
        g_stageLighting.primary.mesh5Alpha = (unsigned int)pPrimary->colour[6][3] << 0x10;
        g_stageLighting.secondary.mesh5Alpha = (unsigned int)pSecondary->colour[6][3] << 0x10;
        g_stageLighting.primary.lightColour.x = (unsigned int)pPrimary->colour[7][0] << 0x10;
        g_stageLighting.primary.lightColour.y = (unsigned int)pPrimary->colour[7][1] << 0x10;
        g_stageLighting.primary.lightColour.z = (unsigned int)pPrimary->colour[7][2] << 0x10;
        g_stageLighting.secondary.lightColour.x = (unsigned int)pSecondary->colour[7][0] << 0x10;
        g_stageLighting.secondary.lightColour.y = (unsigned int)pSecondary->colour[7][1] << 0x10;
        g_stageLighting.secondary.lightColour.z = (unsigned int)pSecondary->colour[7][2] << 0x10;
        g_stageLighting.primary.groundColour.x = (unsigned int)pPrimary->colour[3][0] << 0x10;
        g_stageLighting.primary.groundColour.y = (unsigned int)pPrimary->colour[3][1] << 0x10;
        g_stageLighting.primary.groundColour.z = (unsigned int)pPrimary->colour[3][2] << 0x10;
        g_stageLighting.secondary.groundColour.x = (unsigned int)pSecondary->colour[3][0] << 0x10;
        g_stageLighting.secondary.groundColour.y = (unsigned int)pSecondary->colour[3][1] << 0x10;
        g_stageLighting.secondary.groundColour.z = (unsigned int)pSecondary->colour[3][2] << 0x10;
        g_stageLighting.primary.groundReferenceBlend = FixDiv((int)pPrimary->colour[3][3] << 16, 0xff0000);
        g_stageLighting.secondary.groundReferenceBlend = FixDiv((int)pSecondary->colour[3][3] << 16, 0xff0000);
        g_stageLighting.primary.objectColour.x = (unsigned int)pPrimary->colour[4][0] << 0x10;
        g_stageLighting.primary.objectColour.y = (unsigned int)pPrimary->colour[4][1] << 0x10;
        g_stageLighting.primary.objectColour.z = (unsigned int)pPrimary->colour[4][2] << 0x10;
        g_stageLighting.secondary.objectColour.x = (unsigned int)pSecondary->colour[4][0] << 0x10;
        g_stageLighting.secondary.objectColour.y = (unsigned int)pSecondary->colour[4][1] << 0x10;
        g_stageLighting.secondary.objectColour.z = (unsigned int)pSecondary->colour[4][2] << 0x10;
        g_stageLighting.primary.objectAlpha = (int)((unsigned int)pPrimary->colour[4][3] << 0x10);
        g_stageLighting.secondary.objectAlpha = (int)((unsigned int)pSecondary->colour[4][3] << 0x10);
        g_stageLighting.primary.ambientColour.x = (unsigned int)pPrimary->colour[5][0] << 0x10;
        g_stageLighting.primary.ambientColour.y = (unsigned int)pPrimary->colour[5][1] << 0x10;
        g_stageLighting.primary.ambientColour.z = (unsigned int)pPrimary->colour[5][2] << 0x10;
        g_stageLighting.secondary.ambientColour.x = (unsigned int)pSecondary->colour[5][0] << 0x10;
        g_stageLighting.secondary.ambientColour.y = (unsigned int)pSecondary->colour[5][1] << 0x10;
        g_stageLighting.secondary.ambientColour.z = (unsigned int)pSecondary->colour[5][2] << 0x10;
        g_stageLighting.primary.fogColour.x = (unsigned int)pPrimary->colour[10][0] << 0x10;
        g_stageLighting.primary.fogColour.y = (unsigned int)pPrimary->colour[10][1] << 0x10;
        g_stageLighting.primary.fogColour.z = (unsigned int)pPrimary->colour[10][2] << 0x10;
        g_stageLighting.primary.fogStart = pPrimary->fogStart;
        g_stageLighting.primary.fogEnd = pPrimary->fogEnd;
        g_stageLighting.secondary.fogColour.x = (unsigned int)pSecondary->colour[10][0] << 0x10;
        g_stageLighting.secondary.fogColour.y = (unsigned int)pSecondary->colour[10][1] << 0x10;
        g_stageLighting.secondary.fogColour.z = (unsigned int)pSecondary->colour[10][2] << 0x10;
        g_stageLighting.secondary.fogStart = pSecondary->fogStart;
        g_stageLighting.secondary.fogEnd = pSecondary->fogEnd;
        g_stageLighting.primary.skyColour.x = (unsigned int)pPrimary->colour[9][0] << 0x10;
        g_stageLighting.primary.skyColour.y = (unsigned int)pPrimary->colour[9][1] << 0x10;
        g_stageLighting.primary.skyColour.z = (unsigned int)pPrimary->colour[9][2] << 0x10;
        g_stageLighting.primary.skyAlpha = (unsigned int)pPrimary->colour[9][3] << 0x10;
        g_stageLighting.secondary.skyColour.x = (unsigned int)pSecondary->colour[9][0] << 0x10;
        g_stageLighting.secondary.skyColour.y = (unsigned int)pSecondary->colour[9][1] << 0x10;
        g_stageLighting.secondary.skyColour.z = (unsigned int)pSecondary->colour[9][2] << 0x10;
        g_stageLighting.secondary.skyAlpha = (unsigned int)pSecondary->colour[9][3] << 0x10;
    } else {
        g_stageLighting.primary.meshHeightParameters.x = 0xfffb0000;
        g_stageLighting.primary.meshHeightParameters.y = 0xffda0000;
        g_stageLighting.primary.meshHeightParameters.z = 0xfff30000;
        g_stageLighting.secondary.meshHeightParameters.x = 0xfffb0000;
        g_stageLighting.secondary.meshHeightParameters.y = 0xffda0000;
        g_stageLighting.secondary.meshHeightParameters.z = 0xfff30000;
        Track_ShiftMeshAndAmbientHeights(0xfffb0000, 0xffda0000, 0xfff30000);
        StageObject_SetStageLightState(0x10000);
        g_stageLighting.primary.highColourDelta.x = -0x5d0000;
        g_stageLighting.primary.heightReferenceColour.x = 0xff0000;
        g_stageLighting.secondary.highColourDelta.x = g_stageLighting.primary.highColourDelta.x;
        g_stageLighting.primary.highColourDelta.y = -0x620000;
        g_stageLighting.primary.highColourDelta.z = 0;
        g_stageLighting.primary.heightReferenceColour.y = 0xff0000;
        g_stageLighting.primary.heightReferenceColour.z = 0xff0000;
        g_stageLighting.primary.fogColour.x = 0;
        g_stageLighting.primary.fogColour.y = 0;
        g_stageLighting.primary.fogColour.z = 0;
        g_stageLighting.primary.fogStart = 0;
        g_stageLighting.primary.fogEnd = 0;
        g_stageLighting.primary.skyColour.x = 0;
        g_stageLighting.primary.skyColour.y = 0;
        g_stageLighting.secondary.highColourDelta.y = g_stageLighting.primary.highColourDelta.y;
        g_stageLighting.secondary.highColourDelta.z = g_stageLighting.primary.highColourDelta.z;
        g_stageLighting.primary.sunIconColour.x = 0xff0000;
        g_stageLighting.secondary.heightReferenceColour.x = g_stageLighting.primary.heightReferenceColour.x;
        g_stageLighting.primary.sunIconColour.y = 0xff0000;
        g_stageLighting.primary.sunIconColour.z = 0xff0000;
        g_stageLighting.secondary.heightReferenceColour.y = g_stageLighting.primary.heightReferenceColour.y;
        g_stageLighting.secondary.heightReferenceColour.z = g_stageLighting.primary.heightReferenceColour.z;
        g_stageLighting.primary.lowColour.z = 0xff0000;
        g_stageLighting.secondary.sunIconColour.x = g_stageLighting.primary.sunIconColour.x;
        g_stageLighting.primary.lowColour.x = 0xbc0000;
        g_stageLighting.primary.lowColour.y = 0xca0000;
        g_stageLighting.primary.objectAlpha = 0x640000;
        g_stageLighting.primary.sunIconSize = 0xfa0000;
        g_stageLighting.primary.heightReferenceBlend = 0x8000;
        g_stageLighting.primary.mesh5Colour.x = 0xff0000;
        g_stageLighting.primary.mesh5Colour.y = 0xff0000;
        g_stageLighting.primary.mesh5Colour.z = 0xff0000;
        g_stageLighting.primary.mesh5Alpha = 0xff0000;
        g_stageLighting.primary.lightColour.x = 0xff0000;
        g_stageLighting.primary.lightColour.y = 0xff0000;
        g_stageLighting.primary.lightColour.z = 0xff0000;
        g_stageLighting.primary.groundColour.x = 0x9b0000;
        g_stageLighting.primary.groundColour.y = 0x9b0000;
        g_stageLighting.primary.groundColour.z = 0x9b0000;
        g_stageLighting.primary.groundReferenceBlend = 0x8000;
        g_stageLighting.primary.objectColour.x = 0xff0000;
        g_stageLighting.primary.objectColour.y = 0xff0000;
        g_stageLighting.primary.objectColour.z = 0xff0000;
        g_stageLighting.primary.ambientColour.x = 0xc80000;
        g_stageLighting.primary.ambientColour.y = 0xc80000;
        g_stageLighting.primary.ambientColour.z = 0xc80000;
        g_stageLighting.primary.skyColour.z = 0;
        g_stageLighting.primary.skyAlpha = 0;
        g_stageLighting.secondary.sunIconColour.y = g_stageLighting.primary.sunIconColour.y;
        g_stageLighting.secondary.sunIconColour.z = g_stageLighting.primary.sunIconColour.z;
        g_stageLighting.secondary.sunIconSize = g_stageLighting.primary.sunIconSize;
        g_stageLighting.secondary.lowColour.x = g_stageLighting.primary.lowColour.x;
        g_stageLighting.secondary.lowColour.y = g_stageLighting.primary.lowColour.y;
        g_stageLighting.secondary.mesh5Colour.x = g_stageLighting.primary.mesh5Colour.x;
        g_stageLighting.secondary.mesh5Colour.y = g_stageLighting.primary.mesh5Colour.y;
        g_stageLighting.secondary.mesh5Colour.z = g_stageLighting.primary.mesh5Colour.z;
        g_stageLighting.secondary.lightColour.x = g_stageLighting.primary.lightColour.x;
        g_stageLighting.secondary.lightColour.y = g_stageLighting.primary.lightColour.y;
        g_stageLighting.secondary.lightColour.z = g_stageLighting.primary.lightColour.z;
        g_stageLighting.secondary.groundColour.x = g_stageLighting.primary.groundColour.x;
        g_stageLighting.secondary.lowColour.z = g_stageLighting.primary.lowColour.z;
        g_stageLighting.secondary.groundColour.y = g_stageLighting.primary.groundColour.y;
        g_stageLighting.secondary.groundColour.z = g_stageLighting.primary.groundColour.z;
        g_stageLighting.secondary.heightReferenceBlend = g_stageLighting.primary.heightReferenceBlend;
        g_stageLighting.secondary.groundReferenceBlend = g_stageLighting.primary.groundReferenceBlend;
        g_stageLighting.secondary.objectColour.x = g_stageLighting.primary.objectColour.x;
        g_stageLighting.secondary.objectColour.z = g_stageLighting.primary.objectColour.z;
        g_stageLighting.secondary.objectColour.y = g_stageLighting.primary.objectColour.y;
        g_stageLighting.secondary.ambientColour.x = g_stageLighting.primary.ambientColour.x;
        g_stageLighting.secondary.ambientColour.z = g_stageLighting.primary.ambientColour.z;
        g_stageLighting.secondary.ambientColour.y = g_stageLighting.primary.ambientColour.y;
        g_stageLighting.secondary.fogColour.x = g_stageLighting.primary.fogColour.x;
        g_stageLighting.secondary.fogColour.z = g_stageLighting.primary.fogColour.z;
        g_stageLighting.secondary.fogColour.y = g_stageLighting.primary.fogColour.y;
        g_stageLighting.secondary.skyColour.x = g_stageLighting.primary.skyColour.x;
        g_stageLighting.secondary.mesh5Alpha = g_stageLighting.primary.mesh5Alpha;
        g_stageLighting.secondary.objectAlpha = g_stageLighting.primary.objectAlpha;
        g_stageLighting.secondary.fogStart = g_stageLighting.primary.fogStart;
        g_stageLighting.secondary.fogEnd = g_stageLighting.primary.fogEnd;
        g_stageLighting.secondary.skyColour.y = g_stageLighting.primary.skyColour.y;
        g_stageLighting.secondary.skyColour.z = g_stageLighting.primary.skyColour.z;
        g_stageLighting.secondary.skyAlpha = g_stageLighting.primary.skyAlpha;
    }
    g_stageLighting.blend = 0xffff0000;
    if (g_stageLighting.primary.fogStart == 0 && g_stageLighting.primary.fogEnd == 0 &&
        g_stageLighting.secondary.fogStart == 0 && g_stageLighting.secondary.fogEnd == 0) {
        StageObject_SetForwardedFlareState(0);
        g_stageLighting.fogEnabled = 0;
        return;
    }
    StageObject_SetForwardedFlareState(1);
    g_stageLighting.fogEnabled = 1;
}

// Blends two byte values: b + (a - b) * t, clamped to 255.
// FUNCTION: CMR2 0x004616c0
int StageObject_BlendClampedByteValues(BYTE a, BYTE b, int t)
{
    int v;

    v = (((int)b << 16) + FixMul(((int)a << 16) - ((int)b << 16), t)) >> 16;
    if (v > 0xff)
        v = 0xff;
    return v;
}

// GLOBAL: CMR2 0x005885a0
int g_unk0x005885a0[8][4];
// GLOBAL: CMR2 0x00549ba0
int g_unk0x00549ba0[8][4];

// Advances (mod 200) the counters of a car's four flagged slots and clears the flags.
// FUNCTION: CMR2 0x00464c60
void StageObject_AdvanceFlaggedCarSlotCounters(int car)
{
    int i;

    if (car < 8) {
        for (i = 0; i < 4; i++) {
            if (g_unk0x005885a0[car][i] != 0) {
                g_unk0x005885a0[car][i] = 0;
                g_unk0x00549ba0[car][i] = (g_unk0x00549ba0[car][i] + 1) % StageObject_TrailRing(car);
            }
        }
    }
}

// Updates the four wheel skid trails and fades their colours by surface and slip.
// FUNCTION: CMR2 0x00464cb0
void StageObject_UpdateSkidTrails(int carIndex)
{
    FixVector up = { 0, 0x10000, 0 };
    FixVector side;
    Car *pCar;
    TrailPoint *pPoint;
    int *pBaseColour;
    BYTE baseColour[4];
    int shortLifetime;
    int wheel;
    int pointIndex;
    int slip;
    int opacity;
    int trailIndex;
    int yOffset;
    int length;
    char surface;
    char previous;
    BYTE material;
    BYTE previousMaterial;
    FixVector *pDelta;

    if (carIndex >= 8 || CGameInfo::GetGraphicsOptionBits25To26() == 2)
        return;
    pCar = Car_Get(carIndex);
    if (carIndex == 0)
        g_trailFrame++;
    shortLifetime = 0;
    if ((StageObject_GetViewWeatherStateByte(carIndex) == 1 || StageObject_GetViewWeatherStateByte(carIndex) == 2) && StageObject_GetViewWeatherField54(carIndex) > 0x3333)
        shortLifetime = 1;
    for (wheel = 0; wheel < 4; wheel++) {
        pBaseColour = StageObject_GetViewWeatherSlot(carIndex, wheel);
        if (pBaseColour != NULL) {
            *(int *)baseColour = *pBaseColour;
        } else {
            baseColour[0] = 0;
            baseColour[1] = 0xff;
            baseColour[2] = 0xff;
        }
        for (pointIndex = 0; pointIndex < TRAIL_MAX; pointIndex++) {
            pPoint = &g_trailPoints[carIndex][wheel][pointIndex];
            if (g_trailPointUsed[carIndex][wheel][pointIndex] != 0 &&
                pPoint->age > (shortLifetime ? g_stageSurfaceInfo[8].flags << 2 : g_stageSurfaceInfo[8].flags * 0x50)) {
                g_trailPointUsed[carIndex][wheel][pointIndex] = 0;
                pPoint->alphaLeft = 0;
                pPoint->alphaRight = 0;
            }
        }
    }

    for (wheel = 0; wheel < 4; wheel++) {
        yOffset = -0x20c - wheel * 0x83;
        surface = (char)pCar->wheelSurface[wheel];
        previous = (char)g_trailSurface[carIndex][wheel];
        if ((pCar->cornerOnGround[wheel] == 1 && g_trailTimer[carIndex][wheel] > 2 && g_trailCount[carIndex] >= 2 &&
             pCar->field_0xb74 == 1 &&
             (g_stageSurfaceInfo[(char)g_stageSurfaceMap[surface]].flags & 1) != 0 &&
             (g_stageSurfaceInfo[(char)g_stageSurfaceMap[previous]].flags & 1) != 0) ||
            (pCar->cornerOnGround[wheel] == 1 &&
             (g_stageSurfaceInfo[(char)g_stageSurfaceMap[previous]].flags & 2) != 0 &&
             (g_stageSurfaceInfo[(char)g_stageSurfaceMap[surface]].flags & 2) != 0)) {
            slip = StageObject_GetWheelSlip(carIndex, wheel);
            if ((g_stageSurfaceInfo[(char)g_stageSurfaceMap[surface]].flags & 2) != 0 &&
                pCar->cornerOnGround[wheel] != 0 && pCar->cornerOnGround[wheel ^ 2] != 0) {
                if (surface == 0x19)
                    continue;
                slip = 0x8000;
            } else if (slip <= 0) {
                continue;
            }
            pDelta = &g_trailDelta[carIndex][wheel];
            if ((pDelta->x == 0 && pDelta->z == 0) || g_trailReset[carIndex][wheel] != 0)
                continue;
            material = g_stageSurfaceMap[surface];
            previousMaterial = g_stageSurfaceMap[previous];
            opacity = FixMul(slip, 0xff0000) >> 16;
            g_unk0x005885a0[carIndex][wheel] = 1;
            trailIndex = g_unk0x00549ba0[carIndex][wheel];
            pPoint = &g_trailPoints[carIndex][wheel][trailIndex];
            FixVecCross(&side, pDelta, &up);
            FIX_NORMALIZE_INTO(side, side);
            FixVecScale(&side, &side, 0x1eb8);
            if (CGameInfo::IsActiveCheatEnabled(6) != 0) {
                FixVecScale(&side, &side, 0x28000);
                pPoint->left.x = g_carTyreTrailCenters[carIndex][wheel].x + side.x;
                pPoint->left.y = g_carTyreTrailCenters[carIndex][wheel].y + side.y;
                pPoint->left.z = g_carTyreTrailCenters[carIndex][wheel].z + side.z;
                pPoint->right.x = g_carTyreTrailCenters[carIndex][wheel].x - side.x;
                pPoint->right.y = g_carTyreTrailCenters[carIndex][wheel].y - side.y;
                pPoint->right.z = g_carTyreTrailCenters[carIndex][wheel].z - side.z;
            } else {
                if (Car_UsesNarrowWheels(pCar, 1) != 0)
                    FixVecScale(&side, &side, 0x9999);
                pPoint->left.x = g_carTyreTrailCenters[carIndex][wheel].x + side.x;
                pPoint->left.y = g_carTyreTrailCenters[carIndex][wheel].y + side.y;
                pPoint->left.z = g_carTyreTrailCenters[carIndex][wheel].z + side.z;
                pPoint->right.x = g_carTyreTrailCenters[carIndex][wheel].x - side.x;
                pPoint->right.y = g_carTyreTrailCenters[carIndex][wheel].y - side.y;
                pPoint->right.z = g_carTyreTrailCenters[carIndex][wheel].z - side.z;
            }
            pPoint->left.y += yOffset;
            pPoint->right.y += yOffset;
            pPoint->alphaLeft = (BYTE)opacity;
            pPoint->alphaRight = (BYTE)opacity;
            pPoint->age = 0;
            pPoint->material = material;
            g_trailPointUsed[carIndex][wheel][trailIndex] = 1;
            pPoint->frame = g_trailFrame;
            if (StageObject_TrailHistoryOption() != 0 && (g_trailFrame % (g_stageSurfaceInfo[8].flags > 0 ? g_stageSurfaceInfo[8].flags : 1)) == 0)
                StageObject_TrailHistoryPush(carIndex, wheel, *pPoint);
            if ((g_stageSurfaceInfo[material].flags & 2) == 0) {
                g_trailColor[carIndex][0] += (g_stageSurfaceInfo[previousMaterial].red - g_trailColor[carIndex][0]) / 2;
                g_trailColor[carIndex][1] += (g_stageSurfaceInfo[previousMaterial].green - g_trailColor[carIndex][1]) / 2;
                g_trailColor[carIndex][2] += (g_stageSurfaceInfo[previousMaterial].blue - g_trailColor[carIndex][2]) / 2;
                pPoint->colour[0] = g_trailColor[carIndex][0] * baseColour[0] / 0xff;
                pPoint->colour[1] = g_trailColor[carIndex][1] * baseColour[1] / 0xff;
                pPoint->colour[2] = g_trailColor[carIndex][2] * baseColour[2] / 0xff;
                g_unk0x00543708[carIndex][wheel] = 1;
                g_unk0x00549b20[carIndex][wheel] = 0;
            } else {
                g_trailColor[carIndex][0] = g_stageSurfaceInfo[previousMaterial].red * baseColour[0] / 0xff;
                g_trailColor[carIndex][1] = g_stageSurfaceInfo[previousMaterial].green * baseColour[1] / 0xff;
                g_trailColor[carIndex][2] = g_stageSurfaceInfo[previousMaterial].blue * baseColour[2] / 0xff;
                *(int *)pPoint->colour = *(int *)g_trailColor[carIndex];
                g_unk0x00543708[carIndex][wheel] = 0;
                g_unk0x00549b20[carIndex][wheel] = 1;
            }
            g_trailPoints[carIndex][wheel][(g_unk0x00549ba0[carIndex][wheel] + 1) % StageObject_TrailRing(carIndex)].alphaLeft = 0;
            g_trailPoints[carIndex][wheel][(g_unk0x00549ba0[carIndex][wheel] + 1) % StageObject_TrailRing(carIndex)].alphaRight = 0;
        } else if (g_unk0x00543708[carIndex][wheel] != 0 || g_unk0x00549b20[carIndex][wheel] != 0) {
            g_unk0x005885a0[carIndex][wheel] = 1;
        }
    }
}

// Object classes that fill the four per-car slots of 0x590b7c.
// GLOBAL: CMR2 0x0051f888
char g_carSlotClasses[4] = { 9, 10, 12, 13 };

// Stores an object in every car slot whose class matches it.
// FUNCTION: CMR2 0x00480af0
void StageObject_AssignMatchingCarClassSlots(Car *pCar, SceneNode *pObject, BYTE flag)
{
    int i;

    for (i = 3; i >= 0; i--) {
        if (g_carSlotClasses[i] == (char)pObject->flags) {
            g_carPartNodes[i][pCar->index] = pObject;
            g_carPartModelIndices[i][pCar->index] = flag;
        }
    }
}

// Clears record `index` of the 0x48-byte table at 0x58d6d0.
// FUNCTION: CMR2 0x00477ac0
void CarLight_ResetTextureState(int index)
{
    CarLightTextureState *p = &g_carLightTextureStates[index];

    p->levels[0][0] = 0;
    p->levels[0][1] = 0;
    p->levels[0][2] = 0;
    p->levels[0][3] = 0;
    p->levels[0][4] = 0;
    p->levels[1][0] = 0;
    p->levels[1][1] = 0;
    p->levels[1][2] = 0;
    p->levels[1][3] = 0;
    p->levels[1][4] = 0;
    p->fadedLevels[0][0] = 0;
    p->fadedLevels[0][1] = 0;
    p->fadedLevels[0][2] = 0;
    p->fadedLevels[0][3] = 0;
    p->fadedLevels[0][4] = 0;
    p->fadedLevels[1][0] = 0;
    p->fadedLevels[1][1] = 0;
    p->fadedLevels[1][2] = 0;
    p->fadedLevels[1][3] = 0;
    p->fadedLevels[1][4] = 0;
    p->appliedLevels[0][0] = -1;
    p->appliedLevels[0][1] = -1;
    p->appliedLevels[0][2] = -1;
    p->appliedLevels[0][3] = -1;
    p->appliedLevels[0][4] = -1;
    p->appliedLevels[1][0] = -1;
    p->appliedLevels[1][1] = -1;
    p->appliedLevels[1][2] = -1;
    p->appliedLevels[1][3] = -1;
    p->appliedLevels[1][4] = -1;
    p->channelMasks[0] = 0;
    p->channelMasks[1] = 0;
}

SceneNode *SceneNode_FindByType(SceneNode *pNode, unsigned int type);
void Graphics_SetRecordField2C(Mesh *p, int value);
void Frontend_SetObjectField118(struct Texture *pTexture, int value);

// Resets a car's light texture levels and caches the textures of its two
// body parts (scene nodes of type 0xe).
// FUNCTION: CMR2 0x00477b60
void CarLight_CacheBodyTextures(int car, int unused1, int unused2, BYTE flag)
{
    CarLightTextureState *pRecord = &g_carLightTextureStates[car];
    Mesh *pMesh;
    Texture *pTexture;

    pRecord->combineFirstTwoChannels = flag;
    pRecord->textures[0] = NULL;
    pRecord->textures[1] = NULL;
    CarLight_ResetTextureState(car);
    RallyData_ValidateIndex(car);
    pMesh = (Mesh *)SceneNode_FindByType(Car_Get(car)->pBodyNode, 0xe)->pObject;
    Graphics_SetRecordField2C(pMesh, 0);
    pTexture = CGraphics::m_pTextureManager->textureBuffer[pMesh->pTriangles->textures[0]];
    pRecord->textures[0] = pTexture;
    Frontend_SetObjectField118(pTexture, 2);
    if (Car_Get(car)->pAlternateBodyNode != NULL) {
        pMesh = (Mesh *)SceneNode_FindByType(Car_Get(car)->pAlternateBodyNode, 0xe)->pObject;
        Graphics_SetRecordField2C(pMesh, 0);
        pTexture = CGraphics::m_pTextureManager->textureBuffer[pMesh->pTriangles->textures[0]];
        pRecord->textures[1] = pTexture;
        Frontend_SetObjectField118(pTexture, 2);
    }
}

// Sets or clears bits in the two flag bytes of record `index` of 0x58d6d0.
// FUNCTION: CMR2 0x00477c20
void CarLight_SetChannelMasks(int index, char set0, char set1, BYTE mask)
{
    CarLightTextureState *p;

    p = &g_carLightTextureStates[index];
    if (set0 != 0)
        p->channelMasks[0] |= mask;
    else
        p->channelMasks[0] &= ~mask;
    if (set1 != 0) {
        p->channelMasks[1] |= mask;
        return;
    }
    p->channelMasks[1] &= ~mask;
}

extern unsigned short *g_stageRandomTextures[3];
extern Mesh *g_stageMesh4Copy;

// Gives every triangle of the stage mesh one of the three random textures.
// FUNCTION: CMR2 0x00492b50
void StageObject_RandomizeStageTriangleTextures(void)
{
    int r;
    int n;
    int offset;

    r = rand() % 3;
    if (g_stageRandomTextures[r] != NULL) {
        for (n = g_stageMesh4Copy->triangleCount - 1; n >= 0; n--) {
            offset = n * 0x4c - 0x4c;
            // The biased offset addresses the texture word (+4) of triangle n.
            *(unsigned int *)((BYTE *)g_stageMesh4Copy->pTriangles + (0x50 + offset)) = *g_stageRandomTextures[r];
        }
    }
}

extern int g_carPartTableCarCount;

SceneNode *RallyData_GetChallengeRenderState(void);

// Destroys, in the four node tables, the nodes of every car that belong to
// the current stage kind.
// FUNCTION: CMR2 0x004866a0
void StageObject_DestroyStageKindCarNodes(void)
{
    int car;
    int i;

#define CAR_NODE (g_carPartStateTables.parts[i][car].pNode)
    for (car = 0; car < g_carPartTableCarCount; car++) {
        for (i = 0; i < 4; i++) {
            if (CAR_NODE != NULL && CAR_NODE->pParent == RallyData_GetChallengeRenderState())
                SceneNode_Destroy(CAR_NODE);
        }
    }
}

// FUNCTION: CMR2 0x0048d850
void TracksideCamera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther)
{
    short v;

    v = g_unk0x00591750[g_viewCameraSpot[pRecord->index]].heading;
    if (v > 0x3f4 && v < 0x40b) {
        CarCamera_EndEnter(pRecord, pOther);
        return;
    }
    if (v < -0x3f4 && v > -0x40b)
        CarCamera_EndEnter(pRecord, pOther);
}

// Returns the two signed levels of light channel `slot` scaled by 256.
// FUNCTION: CMR2 0x00477c80
void CarLight_GetScaledChannelLevels(int lane, int *pA, int *pB, int slot)
{
    CarLightTextureState *p = &g_carLightTextureStates[lane];

    if (pA != NULL)
        *pA = (p->levels[0][slot] * 0x10000) / 256;
    if (pB != NULL)
        *pB = (p->levels[1][slot] * 0x10000) / 256;
}

// Resets moving-part damage thresholds/indices and reseeds the random generator.
// FUNCTION: CMR2 0x00480a60
void CarPart_ResetDamageThresholds(void)
{
    srand(400);
    g_carPartDamageThresholds[0] = 0x8000;
    g_carPartDamageThresholds[1] = 0x8000;
    g_carPartDamageThresholds[2] = 0x8000;
    g_carPartDamageThresholds[3] = 0x3333;
    g_carPartDamageIndices[0] = 0x16;
    g_carPartDamageIndices[1] = 0x14;
    g_carPartDamageIndices[2] = 0x15;
    g_carPartDamageIndices[3] = 0x12;
    g_unk0x00590bfc = 1;
    g_unk0x00590bfd = 2;
}

int Sprite_FillRect(BYTE *unused, short *pRect, BYTE *pColour, int layer);

extern BYTE g_barTextColour[4];
// Panel, text and selection colours of the stage-data screen.
// GLOBAL: CMR2 0x0051c984
BYTE g_unk0x0051c984[4] = { 210, 202, 210, 128 };
// GLOBAL: CMR2 0x0051c994
BYTE g_unk0x0051c994[4] = { 143, 135, 143, 255 };

Texture *InRaceMenu_GetUpArrowTexture(void);
Texture *InRaceMenu_GetDownArrowTexture(void);

// Draws the stage-data panel of the pause screen: its background, the row
// separators and, for every item, the label and the highlight sprite.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Identical instruction sequence; the original keeps the loop counter in ESI and
// pushes EBX/EDI inside the loop, our build spills one more register in the prologue,
// which shifts every stack slot by 4 (register-slot renumbering).
// FUNCTION: CMR2 0x004738f0
void StageObject_DrawPauseStageDataPanel(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    short rect2[4];
    int i;
    Texture *texture;
    BYTE *pColour;

    rect2[0] = (short)((int)(g_pGraphics->resX * 0x64) / 0x280);
    rect2[1] = (short)((int)(g_pGraphics->resY * 0xd1) / 0x1e0);
    texture = InRaceMenu_GetUpArrowTexture();
    rect2[2] = texture->width;
    texture = InRaceMenu_GetUpArrowTexture();
    rect2[3] = texture->height;
    rect[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
    rect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0);
    rect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
    rect[3] = (short)((int)(g_pGraphics->resY * 0x26) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_unk0x0051c984, 2);
    // Title of the panel (text 0x76); it was missing from the transcription.
    Font_DrawText(0, CFrontend::GetTextString(0x76),
                  (int)(g_pGraphics->resX * 0x7a) / 0x280,
                  (int)(g_pGraphics->resY * 0xb8) / 0x1e0,
                  (int *)g_barTextColour, 0x11);
    pItem = pMenu->items;
    for (i = 0; i < pMenu->itemCount; i++, pItem++) {
        rect2[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0
                           + (int)(g_pGraphics->resY * 0xd1) / 0x1e0);
        if (pMenu->cursor == i) {
            pColour = g_barTextColour;
            texture = InRaceMenu_GetUpArrowTexture();
        } else {
            pColour = g_unk0x0051c994;
            texture = InRaceMenu_GetDownArrowTexture();
        }
        Font_DrawText(0, CFrontend::GetTextString(pItem->id),
                      (int)(g_pGraphics->resX * 0x78) / 0x280,
                      (int)(g_pGraphics->resY * i * 0x24) / 0x1e0
                          + (int)(g_pGraphics->resY * 0xdd) / 0x1e0,
                      (int *)pColour, 0x11);
        Sprite_Queue((SpriteRect *)&texture->field_0x11c, (SpriteRect *)rect2, texture,
                     2, 0, NULL, NULL, pColour, 8);
        if (i == 0 || pMenu->cursor == i) {
            rect[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
            rect[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0
                              + (int)(g_pGraphics->resY * 0xc6) / 0x1e0);
            rect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
            rect[3] = 1;
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 1);
        }
        rect[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
        rect[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0
                          + (int)(g_pGraphics->resY * 0xea) / 0x1e0);
        rect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
        rect[3] = 1;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 1);
    }
}


extern BYTE g_barTextColour[4];
extern BYTE g_unk0x0051c9a4[4];
extern char g_strVs0x0051c9ac[];
void StageObject_DrawTypingTextFraction(char *text, int fraction, unsigned int font, int x, unsigned int y, int *pColour, unsigned int flags);
void Knockout_DrawAnimatedMatchHeader(int param1, int param2, int param3, int param4, int param5, KnockoutMatch *param6);
void StageObject_DrawAnimatedTextPair(int param1, KnockoutMatch *param2, short *param3, BYTE *param4, BYTE *param5,
                  int param6, int param7, int *param8, int param9);
#define KO_X(n) ((int)(g_pGraphics->resX * (n)) / 0x280)
#define KO_Y(n) ((int)(g_pGraphics->resY * (n)) / 0x1e0)

// Draws the arcade knockout bracket of the current round: the title, then
// for every match its number, both driver panels (dimmed loser, highlighted
// player) and "vs." on the match being raced; while the bracket view is up
// (0x58ca8c) the player's panel instead zooms towards the next round's slot.
// FUNCTION: CMR2 0x004744f0
void Knockout_DrawCurrentRoundBracket(void)
{
    BYTE *pBox2;
    BYTE *pName1;
    BYTE *pBox1;
    BYTE *pName2;
    BYTE *pText;
    short rect[4];
    int vs;
    KnockoutTable *pTable;
    int current;
    unsigned int round;
    KnockoutMatch *pMatch;
    int fading;
    KnockoutMatch *pMatches;
    int y0;
    int count;
    int highlight1;
    int bracket;
    int highlight2;
    int x0;
    int i;
    int x2;
    int y2;
    int y;
    int x;

    sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(0)));
    CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
    Font_DrawText(0, CFrontend::m_stringDest, KO_X(0x1e), Font_GetLineHeight(0) + KO_Y(0x39) + KO_Y(5),
                  (int *)g_barTextColour, 0x11);
    pTable = (KnockoutTable *)RallyData_GetChampionshipState();
    round = (pTable->state >> 3) & 7;
    bracket = Graphics_IsRegisteredTimerRunning(g_unk0x0058ca8c);
    fading = 0;
    if (Graphics_IsRegisteredTimerRunning(&g_pauseMenus.bracketTimer) || (!Graphics_IsRegisteredTimerRunning(g_unk0x0058ca8c) && g_knockoutRaceState == 6))
        fading = 1;
    switch (round) {
    case 1:
        count = 8;
        x0 = KO_X(0x56);
        y0 = KO_Y(0x8e);
        pMatches = pTable->round1;
        break;
    case 2:
        count = 4;
        x0 = KO_X(0x93);
        y0 = KO_Y(0x8e);
        pMatches = pTable->quarters;
        break;
    case 3:
        count = 2;
        x0 = KO_X(0x93);
        y0 = KO_Y(0xcf);
        pMatches = pTable->semis;
        break;
    case 4:
        count = 1;
        x0 = KO_X(0x115);
        y0 = KO_Y(0xcf);
        pMatches = &pTable->final;
        break;
    }
    for (i = 0; i < count; i++) {
        current = 0;
        highlight2 = 0;
        highlight1 = 0;
        vs = 0;
        y = 0;
        x = 0;
        if (bracket) {
            switch (round) {
            case 1:
                x2 = KO_X(0x93);
                y2 = KO_Y(0x8e);
                break;
            case 2:
                x2 = KO_X(0x93);
                y2 = KO_Y(0xcf);
                break;
            case 3:
                x2 = KO_X(0x115);
                y2 = KO_Y(0xcf);
                break;
            }
        }
        switch (round) {
        case 1:
            if (i == 4 || i == 5 || i == 6 || i == 7)
                y = KO_Y(0x82);
            if (i == 2 || i == 3 || i == 6 || i == 7)
                x = KO_X(0x108);
            if (bracket) {
                if (i / 2 == 2 || i / 2 == 3)
                    y2 += KO_Y(0x82);
                if (i / 2 == 1 || i / 2 == 3)
                    x2 += KO_X(0x108);
            }
            break;
        case 2:
            if (i == 2 || i == 3)
                y = KO_Y(0x82);
            if (i == 1 || i == 3)
                x = KO_X(0x108);
            if (bracket && i / 2 == 1)
                x2 += KO_X(0x108);
            break;
        case 3:
            if (i == 1)
                x = KO_X(0x108);
            break;
        }
        y += y0;
        x += (round == 1 && i % 2) ? KO_X(0x78) + x0 : x0;
        if (g_knockoutRaceState == 3 && i == (int)((pTable->state >> 12) & 0xf) - 1)
            current = 1;
        pMatch = &pMatches[i];
        if ((pMatch->flags & 0x3e0) == 0x3e0)
            highlight2 = 1;
        if ((pMatch->flags & 0x1f) == 0x1f)
            highlight1 = 1;
        if (((g_knockoutRaceState == 2 || g_knockoutRaceState == 4 || g_knockoutRaceState == 5) &&
             i == (int)((pTable->state >> 12) & 0xf)) ||
            (g_knockoutRaceState == 3 && i == (int)((pTable->state >> 12) & 0xf) - 1)) {
            pName1 = NULL;
            vs = 1;
            pName2 = NULL;
            pBox1 = pBox2 = pText = g_barTextColour;
        } else if (pMatch->flags & 0x400) {
            if ((pMatch->flags & 0x1800) == 0x800) {
                highlight2 = 1;
                pName1 = g_unk0x0051c9a4;
                pName2 = NULL;
                pBox1 = pBox2 = pText = g_unk0x0051c994;
            } else {
                highlight1 = 1;
                pName1 = NULL;
                pName2 = g_unk0x0051c9a4;
                pBox1 = pBox2 = pText = g_unk0x0051c994;
            }
        } else {
            pBox1 = pBox2 = pText = g_unk0x0051c994;
            pName2 = NULL;
            pName1 = NULL;
        }
        if (!bracket) {
            if (!Graphics_IsRegisteredTimerRunning(&g_unk0x0058ca80) && !fading) {
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x4f), i + 1);
                Font_DrawText(0, CFrontend::m_stringDest, (int)g_pGraphics->resX / 0x280 + x, y - KO_Y(5), (int *)pText, 0x11);
            } else {
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x4f), i + 1);
                StageObject_DrawTypingTextFraction(CFrontend::m_stringDest, g_pauseMenus.bracketFade, 0, (int)g_pGraphics->resX / 0x280 + x, y - KO_Y(5),
                             (int *)pText, 0x11);
            }
            if (fading && highlight1) {
                if (pName1 != NULL)
                    pName1[3] = (BYTE)(FixMul(0xff0000, g_pauseMenus.bracketFade) >> 16);
                if (pBox1 != NULL)
                    pBox1[3] = (BYTE)(FixMul(0xff0000, g_pauseMenus.bracketFade) >> 16);
                if (pText != NULL)
                    pText[3] = (BYTE)(FixMul(0xff0000, g_pauseMenus.bracketFade) >> 16);
            } else {
                if (pName1 != NULL)
                    pName1[3] = 0xff;
                if (pBox1 != NULL)
                    pBox1[3] = 0xff;
                if (pText != NULL)
                    pText[3] = 0xff;
            }
            rect[0] = (short)x;
            rect[1] = (short)y;
            rect[2] = (short)KO_X(0x56);
            rect[3] = (short)KO_Y(0x26);
            StageObject_DrawAnimatedTextPair(0, pMatch, rect, pName1, pBox1, highlight1, current, (int *)pText, 0);
        } else if (highlight2) {
            if (i % 2 == 1)
                y2 += KO_Y(0x2d);
            Knockout_DrawAnimatedMatchHeader(x, y, x2, y2, 0, pMatch);
        }
        if (vs)
            Font_DrawText(0, g_strVs0x0051c9ac, x - KO_X(2), KO_Y(0x2f) + y, (int *)g_barTextColour, 0x14);
        if (!bracket) {
            if (fading && highlight2) {
                if (pName2 != NULL)
                    pName2[3] = (BYTE)(FixMul(0xff0000, g_pauseMenus.bracketFade) >> 16);
                if (pBox2 != NULL)
                    pBox2[3] = (BYTE)(FixMul(0xff0000, g_pauseMenus.bracketFade) >> 16);
                if (pText != NULL)
                    pText[3] = (BYTE)(FixMul(0xff0000, g_pauseMenus.bracketFade) >> 16);
            } else {
                if (pName2 != NULL)
                    pName2[3] = 0xff;
                if (pBox2 != NULL)
                    pBox2[3] = 0xff;
                if (pText != NULL)
                    pText[3] = 0xff;
            }
            rect[0] = (short)x;
            rect[1] = (short)(KO_Y(0x2d) + y);
            rect[2] = (short)KO_X(0x56);
            rect[3] = (short)KO_Y(0x26);
            StageObject_DrawAnimatedTextPair(1, pMatch, rect, pName2, pBox2, highlight2, current, (int *)pText, 0);
        } else if (highlight1) {
            if (i % 2 == 1)
                y2 += KO_Y(0x2d);
            Knockout_DrawAnimatedMatchHeader(x, KO_Y(0x2d) + y, x2, y2, 1, pMatch);
        }
        g_barTextColour[3] = 0xff;
    }
}
#undef KO_X
#undef KO_Y

// Draws the first `fraction` of a text (typing effect; spaces don't count)
// and the next character on its own.
// FUNCTION: CMR2 0x00474420
void StageObject_DrawTypingTextFraction(char *text, int fraction, unsigned int font, int x, unsigned int y, int *pColour, unsigned int flags)
{
    int length = (int)strlen(text);
    int count = FixMul(length << 16, fraction) >> 16;
    int i;
    int width;
    int colour;

    for (i = 0; i < count; i++) {
        if (text[i] == ' ' && count < length)
            count++;
        CFrontend::m_stringDest[i] = text[i];
    }
    CFrontend::m_stringDest[count] = 0;
    Font_DrawText(font, CFrontend::m_stringDest, x, y, pColour, flags);
    if (count < length) {
        width = Font_GetTextWidth(font, (BYTE *)CFrontend::m_stringDest);
        CFrontend::m_stringDest[0] = text[count];
        CFrontend::m_stringDest[1] = 0;
        colour = *pColour;
        Font_DrawText(font, CFrontend::m_stringDest, width + x, y, &colour, flags);
    }
}

// Fills a rectangle whose width is scaled by `scale` (16.16).
// FUNCTION: CMR2 0x00475970
int StageObject_FillWidthScaledRectangle(int scale, BYTE *unused, short *pRect, BYTE *pColour, int layer)
{
    short rect[4];

    rect[0] = pRect[0];
    rect[1] = pRect[1];
    rect[2] = (short)(FixMul((int)pRect[2] << 16, scale) >> 16);
    rect[3] = pRect[3];
    return Sprite_FillRect(unused, rect, pColour, layer);
}

// Wheel slip (field 0x870) above 0.15, as 0..1.
// match 89%: MSVC reuses the add's flags for the sign test (js) instead of
// re-issuing test/jl/jle, and folds the +(-0x2666) into a sub
// FUNCTION: CMR2 0x00465e40
int StageObject_GetNormalizedWheelSlip(int car, int wheel)
{
    int v, w;

    if (Car_Get(car)->wheelSlip[wheel] < 0)
        v = -Car_Get(car)->wheelSlip[wheel];
    else
        v = Car_Get(car)->wheelSlip[wheel];
    w = v - 0x2666;
    if (w < 0 || w <= 0)
        return 0;
    if (w >= 0x10000)
        return 0x10000;
    return w;
}

void Scene_GetLightColourBytes(DWORD *pColour);

// Brightness of the scene light colour: (r + g + b - 70) / 550, clamped to 0..1.
// FUNCTION: CMR2 0x004648f0
int StageObject_GetSceneLightBrightness(void)
{
    BYTE c[4];
    int v;

    Scene_GetLightColourBytes((DWORD *)c);
    v = FixDiv((c[0] + c[1] + c[2] - 0x46) << 16, 0x2260000);
    if (v < 0)
        return 0;
    if (v > 0x10000)
        v = 0x10000;
    return v;
}

// GLOBAL: CMR2 0x00547fa0
FixVector g_rearViewLightOffsetB = { 0, 0, 0 };
// GLOBAL: CMR2 0x00547fe0
FixVector g_rearViewLightOffsetA = { 0, 0, 0 };
// GLOBAL: CMR2 0x00547fec
SceneNode *g_rearViewLightNodeA = NULL;
// GLOBAL: CMR2 0x00547ff0
SceneNode *g_rearViewLightNodeB = NULL;
// GLOBAL: CMR2 0x00547ff4
int g_rearViewLightAttenuation = 0;

void Graphics_SetGeometryStateValue(int value);

// Places the two rear view nodes of a car from its brightness level.
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00464960
void StageObject_PositionRearViewLightNodes(unsigned int param_1)
{
    unsigned int car;
    Car *pCar;
    int value;
    int level;
    FixVector pos;
    FixVector offset;

    car = param_1;
    pCar = Car_Get(car);
    level = StageObject_GetSceneLightBrightness();
    pCar->field_0xb58 = level == 0x10000 ? 0 : 1;
    value = FixMul(0x4c0000, level);
    CarLight_GetScaledChannelLevels(car, (int *)&param_1, (int *)&param_1, 4);
    if (param_1 == 0)
        value = 0x3e80000;
    Graphics_SetGeometryStateValue(param_1 != 0);
    param_1 = 0x10000 - param_1;
    if ((int)param_1 > 0x10000) {
        param_1 = 0x10000;
    } else {
        if ((int)param_1 < 0)
            param_1 = 0;
    }
    param_1 = FixMul((int)param_1, 0x190000);
    level = value + param_1;
    if (level != g_rearViewLightAttenuation) {
        Scene_SetLightAttenuation(g_rearViewLightNodeA, level);
        Scene_SetLightAttenuation(g_rearViewLightNodeB, level);
        g_rearViewLightAttenuation = level;
    }
    FixMatrix_GetPosition(&pos, &pCar->pSceneRoot->current);
    FixMatrix_RotateVector(&offset, &g_rearViewLightOffsetA, &pCar->pSceneRoot->current);
    offset.x += pos.x;
    offset.y += pos.y;
    offset.z += pos.z;
    SceneNode_SetPosition(g_rearViewLightNodeA, &offset);
    FixMatrix_RotateVector(&offset, &g_rearViewLightOffsetB, &pCar->pSceneRoot->current);
    offset.x += pos.x;
    offset.y += pos.y;
    offset.z += pos.z;
    SceneNode_SetPosition(g_rearViewLightNodeB, &offset);
}


// Puts the car's (up to four) attached nodes back to their creation transform
// and forgets them.
// FUNCTION: CMR2 0x00480b40
void CarPart_ResetAttachedNodes(Car *pCar)
{
    SceneNode *pNode;
    int carIndex;
    int slot;

    srand(400);
    g_partCar = pCar;
    carIndex = g_partCar->index;
    for (slot = 3; slot >= 0; slot--) {
        pNode = g_carPartStateTables.parts[slot][carIndex].pNode;
        if (pNode != NULL) {
            pNode->current = pNode->local;
            g_carPartStateTables.parts[slot][carIndex].pNode = NULL;
        }
    }
}

// Per-car values eased towards their targets (0x591710) by a step.
// Zoom of the trackside camera and the zoom it steps towards.
// GLOBAL: CMR2 0x00591690
int g_tracksideZooms[4];
// GLOBAL: CMR2 0x00591710
int g_tracksideZoomTargets[4];

// Steps a trackside camera's aim toward its target by `step`, locking it on
// (and snapping to the target) once it is closer than that.
// FUNCTION: CMR2 0x0048db00
void View_StepTracksideAim(CameraRecord *pRecord, int step)
{
    BYTE index;
    FixVector delta;

    index = pRecord->index;
    if (g_tracksideAimLocked[index] == 0) {
        delta.x = g_tracksideAimTargets[index].x - g_tracksideAims[index].x;
        delta.y = g_tracksideAimTargets[index].y - g_tracksideAims[index].y;
        delta.z = g_tracksideAimTargets[index].z - g_tracksideAims[index].z;
        if ((int)FixVec_Length(&delta) < step) {
            g_tracksideAimLocked[index] = 1;
        } else {
            FixVec_Normalize(&delta, &delta);
            FixVecScale(&delta, &delta, step);
            g_tracksideAims[index].x += delta.x;
            g_tracksideAims[index].y += delta.y;
            g_tracksideAims[index].z += delta.z;
        }
    }
    if (g_tracksideAimLocked[index] != 0)
        g_tracksideAims[index] = g_tracksideAimTargets[index];
}

// Steps a trackside camera's zoom toward its target by `step`.
// FUNCTION: CMR2 0x0048dc30
void View_StepTracksideZoom(CameraRecord *pRecord, int step)
{
    BYTE car;
    int cur;
    int d;

    car = pRecord->index;
    cur = g_tracksideZooms[car];
    d = g_tracksideZoomTargets[car] - cur;
    if (FIX_ABS(d) < step) {
        g_tracksideZooms[car] = g_tracksideZoomTargets[car];
        return;
    }
    if (d > 0) {
        g_tracksideZooms[car] = cur + step;
        return;
    }
    g_tracksideZooms[car] = cur - step;
}

// Five-bit field of the current round entry (bits 0..4, or 5..9 with pHigh).
// FUNCTION: CMR2 0x004735a0
unsigned int Knockout_GetCurrentDriverField(unsigned int *volatile pHigh)
{
    unsigned int *pState;
    unsigned int state;

    pState = RallyData_GetChampionshipState();
    state = *pState;

    switch ((state >> 3) & 7) {
    case 1:
        pState = pState + ((state >> 12) & 0xf) * 3 + 0x16;
        break;
    case 2:
        pState = pState + ((state >> 12) & 0xf) * 3 + 10;
        break;
    case 3:
        pState = pState + ((state >> 12) & 0xf) * 3 + 4;
        break;
    case 4:
        pState = pState + 1;
        break;
    default: pState = pHigh; break;
    }
    unsigned int *which = pHigh;
    state = *pState;
    if (which != NULL)
        state >>= 5;
    return state & 0x1f;
}

extern Car *g_collisionCar;
extern FixVector g_collisionTarget;
extern FixVector g_collisionLineStart;

// 1 when no corner of the collision car lies strictly between the heights of
// the line start and the target.
// FUNCTION: CMR2 0x0048f400
int StageObject_IsCarOutsideCollisionHeightInterval(void)
{
    int start = g_collisionLineStart.y;
    int target = g_collisionTarget.y;
    int result = 1;
    int i;
    if (target > start) {
        for (i = 0; i < 8; i++) {
            int y = g_collisionCar->corners[i].y;
            if (y < target && y > start) {
                result = 0;
                break;
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            int y = g_collisionCar->corners[i].y;
            if (y > target && y < start) {
                result = 0;
                break;
            }
        }
    }
    return result;
}

void Glow_NoOpEntryCallback(BYTE a, BYTE b, int c, int d);

// FUNCTION: CMR2 0x00486b20
void CarCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther)
{
    int kind;

    if (pRecord->type == 1 && pOther->type == 2)
        CarCamera_SetFullCarLift(pOther, pRecord);
    else
        Glow_NoOpEntryCallback(pRecord->car, pRecord->view, 1, 1);
    kind = pOther->type;
    int value;
    if (kind != 2 && kind != 1 && kind != 10)
        value = 0;
    else
        value = 0x10000;
    g_viewCarLift[pRecord->index] = value;
}

extern CarFlexibleLineState **g_carLineStates;

// Resets record `index` (0x3c bytes) of list `list`: its three vectors to the
// origin and its final int to `value`.
// FUNCTION: CMR2 0x00486630
void StageObject_ResetVectorListRecord(int list, int index, int value)
{
    CarFlexibleLineState *p;

    p = g_carLineStates[list] + index;
    p->position.x = 0;
    p->position.y = 0;
    p->position.z = 0;
    p->groundEnd.x = 0;
    p->groundEnd.y = 0;
    p->groundEnd.z = 0;
    p->previousPosition = p->position;
    p->drawPosition = p->position;
    p->velocityX = 0;
    p->velocityZ = 0;
    p->grounded = value;
}

// Sun visibility (0..100) from the lens flare sample.
// GLOBAL: CMR2 0x00543ea0
short g_sunVisibility;

BYTE Flare_SampleVisibility(short *pRect, BYTE *pColour, BYTE tolerance);
void StageObject_GetGroundReferenceColour(int *pOut);

void Scene_GetAmbientColour(DWORD *pColour);
void Scene_SetAmbient(BYTE *pColour, int boost);
unsigned char RallyDataCountryIndex(void);
int RallyData_IsChampionshipFinalStage(void);
void StageObject_SetVehicleEffectColour(BYTE *pColour);

extern ViewLightingRamp *g_viewLightingRamps;
// GLOBAL: CMR2 0x00547acc
StageObjectCount g_stageObjectCount;

int *RallyData_GetDriverPairRecord(int index);
// GLOBAL: CMR2 0x0051b114
int g_weatherFogFactors[9] = { 0, 0, 0, 0x6666, 0x8000, 0x9999, 0x6666, 0x8000, 0x9999 };

// Speed limits of a stage object: fixed in some stages, else scaled by the
// difficulty and the object's type factor.
// FUNCTION: CMR2 0x00461b30
void StageLights_SetWeatherFogDistance(StageLightPreset *pPreset, int type)
{
    unsigned int speed;

    pPreset->fogStart = 0;
    pPreset->fogEnd = 0;
    speed = (CGameInfo::GetGraphicsOptionBits21To24() + 2) * 0x320000;
    if (*RallyData_GetDriverPairRecord(RallyDataStageIndex()) != 0) {
        pPreset->fogStart = 0xf0000;
        pPreset->fogEnd = 0x500000;
        return;
    }
    if (g_weatherFogFactors[type] != 0)
        pPreset->fogEnd = FixDiv(speed, g_weatherFogFactors[type]);
}

int *RallyData_GetDriverSettingPair(int index);
void StageLights_SetWeatherFogDistance(StageLightPreset *pPreset, int type);

// Time of day steps (mm*100+ss, 500 = 05:00) that bound the twelve stage object
// record columns.
// GLOBAL: CMR2 0x0051b034
WORD g_stageObjectTimeSteps[12] = {
    500, 600, 800, 1000, 1200, 1500, 1700, 1800, 1900, 2000, 2200, 2400
};

// Per stage setting, the two rows of the loaded .hor stage object records to
// blend: 0 = plain, 1 = CLO, 2 = STO, 3 = BLI.
// GLOBAL: CMR2 0x0051b0dc
BYTE g_stageObjectWeatherRows[9][2] = {
    { 0, 0 }, { 0, 0 }, { 0, 1 }, { 1, 2 }, { 1, 2 },
    { 1, 2 }, { 1, 3 }, { 1, 3 }, { 1, 3 }
};

// Per stage setting, the 16.16 factor that blends those two rows.
// GLOBAL: CMR2 0x0051b0f0
int g_stageObjectWeatherFactors[9] = {
    0, 0, 0x10000, 0, 0x8000, 0x10000, 0, 0x8000, 0x10000
};

// Stage object record of the primary stage setting (0x4e bytes).
// GLOBAL: CMR2 0x00543d00
BYTE g_stageObjectPrimary[0x4e];
// Stage object record of the secondary stage setting.
// GLOBAL: CMR2 0x00543e38
BYTE g_stageObjectSecondary[0x4e];

void StageLights_InterpolatePreset(BYTE *out, BYTE *from, BYTE *to, int t);

// Blends the stage object record for one time of day into the primary (slot 0)
// or secondary (slot 1) global record: the time picks the two adjacent columns
// of the loaded .hor table, the stage setting pair picks the record rows and
// their fixed factor, and the two column results are interpolated by how far
// the time lies into its band. NULL when a record is missing or invalid.
// FUNCTION: CMR2 0x00461830
BYTE *StageLights_BlendTimeOfDayPreset(unsigned short timeOfDay, int slot, BYTE **records)
{
    BYTE buffer0[0x50];
    BYTE buffer1[0x50];
    int *pWeatherPair;
    int index;
    int lower;
    int i;
    int prev;
    int prevSeconds;
    int length;
    int position;
    int factor;
    int minutes;
    int seconds;
    int rowFrom;
    int rowTo;
    int rowFactor;
    BYTE *pFrom0;
    BYTE *pFrom1;
    BYTE *pTo0;
    BYTE *pTo1;
    BYTE *dest;

    pWeatherPair = RallyData_GetDriverSettingPair(RallyDataStageIndex());
    if (slot == 0)
        index = pWeatherPair[0];
    else
        index = pWeatherPair[1];

    lower = -1;
    for (i = 0; i < 12; i++) {
        if (g_stageObjectTimeSteps[i] >= timeOfDay) {
            lower = i;
            i = 12;
        }
    }
    if (lower == -1)
        return NULL;

    prev = lower - 1;
    if (prev < 0) {
        prev += 12;
        prevSeconds = 0;
    } else {
        minutes = g_stageObjectTimeSteps[prev] / 100;
        seconds = g_stageObjectTimeSteps[prev] - minutes * 100;
        prevSeconds = (minutes * 60 << 16) + (seconds << 16);
    }
    length = ((g_stageObjectTimeSteps[lower] / 100 * 60 + (g_stageObjectTimeSteps[lower] - g_stageObjectTimeSteps[lower] / 100 * 100)) << 16) - prevSeconds;

    minutes = timeOfDay / 100;
    seconds = timeOfDay - minutes * 100;
    if (seconds >= 60)
        return NULL;
    position = (minutes * 60 << 16) + (seconds << 16) - prevSeconds;

    factor = FixDiv(position, length);
    if (factor > 0x10000)
        factor = 0x10000;
    else if (factor < 0)
        factor = 0;

    rowFrom = g_stageObjectWeatherRows[index][0];
    rowTo = g_stageObjectWeatherRows[index][1];
    rowFactor = g_stageObjectWeatherFactors[index];
    pFrom0 = records[rowFrom * 12 + prev];
    pTo0 = records[rowTo * 12 + prev];
    pFrom1 = records[rowFrom * 12 + lower];
    pTo1 = records[rowTo * 12 + lower];
    if (pFrom0 == NULL || pTo0 == NULL || pFrom1 == NULL || pTo1 == NULL)
        return NULL;

    if (slot == 0)
        dest = g_stageObjectPrimary;
    else
        dest = g_stageObjectSecondary;

    StageLights_InterpolatePreset(buffer0, pFrom0, pTo0, rowFactor);
    StageLights_InterpolatePreset(buffer1, pFrom1, pTo1, rowFactor);
    StageLights_InterpolatePreset(dest, buffer0, buffer1, factor);
    return dest;
}

// Blends both stage object records of the stage's setting pair into the two out
// pointers: the primary from the first setting's time of day, the secondary
// from the second one.
// FUNCTION: CMR2 0x00461a30
void StageLights_BlendPresetPair(unsigned short timePrimary, unsigned short timeSecondary, BYTE **records, StageLightPreset **pPrimary, StageLightPreset **pSecondary)
{
    BYTE *pObjectPrimary;
    BYTE *pObjectSecondary;

    pObjectPrimary = StageLights_BlendTimeOfDayPreset(timePrimary, 0, records);
    pObjectSecondary = StageLights_BlendTimeOfDayPreset(timeSecondary, 1, records);
    *pPrimary = (StageLightPreset *)pObjectPrimary;
    *pSecondary = (StageLightPreset *)pObjectSecondary;
}

// Sets the speed limits of the two stage objects of a pair from the stage's
// setting pair; a stopped one takes over the other's limit.
// FUNCTION: CMR2 0x00461a70
void StageLights_SetPairFogDistances(StageLightPreset *pA, StageLightPreset *pB)
{
    int *pPair;
    int a;
    int b;
    int limit;

    if (pA != NULL && pB != NULL) {
        pPair = RallyData_GetDriverSettingPair(RallyDataStageIndex());
        a = pPair[0];
        b = pPair[1];
        if (a == 1) {
            if (b != 0 && b != 1)
                pA->colour[4][3] = 0x32;
            else
                pA->colour[4][3] = 200;
        }
        if (b == 1) {
            if (a != 0 && a != 1)
                pB->colour[4][3] = 0x32;
            else
                pB->colour[4][3] = 200;
        }
        StageLights_SetWeatherFogDistance(pA, a);
        StageLights_SetWeatherFogDistance(pB, b);
        if (pA->fogStart == 0 && pA->fogEnd == 0 &&
            (pB->fogStart != 0 || pB->fogEnd != 0)) {
            limit = pB->fogEnd;
            pA->fogEnd = limit;
            pA->fogStart = limit;
            return;
        }
        if (pB->fogStart == 0 && pB->fogEnd == 0 &&
            (pA->fogStart != 0 || pA->fogEnd != 0)) {
            limit = pA->fogEnd;
            pB->fogEnd = limit;
            pB->fogStart = limit;
        }
    }
}

void StageTiming_ReadStageFaceRecord(short index, short *pA, short *pB, short *pC, short *pD, unsigned short *pFlags);
void StageTiming_CopyTriangleVertices(FixVector *pOut, unsigned short *pIndices, unsigned short *unused);
int Graphics_GetTriangleHeight(unsigned short *pHeightIndices, FixVector *pVertices, FixVector *pPosition);
void Scene_GetLightColour(DWORD *pColour, int level);
void Scene_GetAmbientColour(DWORD *pColour);
void TrackLighting_SetAmbientRGB(int *pRGB);
void CarShadow_SetLevel(int car, int level);

// Averages the ground lighting over a car's four wheel contact points and
// stores the resulting colour and light level.
// match 61%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00462aa0
void StageObject_AverageWheelGroundLighting(unsigned int param_1, int param_2)
{
    Car *pCar;
    short *pIndex;
    FixVector *vertex;
    int i;
    int sum;
    int count;
    unsigned short idx[3];
    unsigned short d;
    unsigned short flags;
    FixVector lighting[3];
    int h;
    BYTE light[4];
    DWORD ambient[1];
    // Original RGB triplet at ebp-0x38/-0x34/-0x30 is passed as one buffer.
    int rgb[3];
    int avg;
    int total;
    int v;

    pCar = Car_Get(param_1);
    i = 0;
    sum = 0;
    count = 0;
    pIndex = &pCar->cornerTriangle[0];
    vertex = &pCar->corners[0];
    do {
        if (*pIndex >= 0) {
            StageTiming_ReadStageFaceRecord(*pIndex, (short *)&idx[0], (short *)&idx[1], (short *)&idx[2],
                         (short *)&d, &flags);
            StageTiming_CopyTriangleVertices(lighting, idx, &d);
            h = Graphics_GetTriangleHeight(idx, lighting, vertex);
            if (h != -0x3e70000) {
                sum = sum + h;
                count = count + 1;
                Scene_GetLightColour((DWORD *)light, h);
                ((BYTE *)g_unk0x00543f28)[(i + pCar->index * 4) * 4] = light[0];
                ((BYTE *)g_unk0x00543f28)[(i + pCar->index * 4) * 4 + 1] = light[1];
                ((BYTE *)g_unk0x00543f28)[(i + pCar->index * 4) * 4 + 2] = light[2];
            }
        }
        i = i + 1;
        pIndex = pIndex + 1;
        vertex = vertex + 1;
    } while (i < 4);
    if (count > 0) {
        avg = FixDiv(sum, count << 16);
        if (avg > 0x10000)
            avg = 0x10000;
        else if (avg < 0)
            avg = 0;
        Scene_GetLightColour((DWORD *)light, avg);
        rgb[0] = (*(int *)(light + 0) & 0xff) << 16;
        rgb[1] = (*(int *)(light + 1) & 0xff) << 16;
        rgb[2] = (*(int *)(light + 2) & 0xff) << 16;
        Scene_GetAmbientColour(ambient);
        rgb[0] = rgb[0] - (*(int *)((BYTE *)ambient + 0) & 0xff) * 0x10000;
        rgb[1] = rgb[1] - (*(int *)((BYTE *)ambient + 1) & 0xff) * 0x10000;
        rgb[2] = rgb[2] - (*(int *)((BYTE *)ambient + 2) & 0xff) * 0x10000;
        if ((View_GetActiveCameraFlags(param_2) & 0xff) == param_1)
            TrackLighting_SetAmbientRGB(rgb);
        pCar->field_0xa70 = avg;
        total = (rgb[0] < 0 ? -rgb[0] : rgb[0]) + (rgb[1] < 0 ? -rgb[1] : rgb[1]) + (rgb[2] < 0 ? -rgb[2] : rgb[2]);
        v = FixMul(total, 0x55);
        if (v > 0x10000)
            v = 0x10000;
        CarShadow_SetLevel(param_1, v);
    }
}

// Interpolates the two animated values of every 0x2c-byte record by t (16.16).
// FUNCTION: CMR2 0x00461bb0
void StageObject_InterpolateSecondaryRampRecords(int t)
{
    int i;
    ViewLightingRamp *p;

    for (i = 0; i < g_stageObjectCount.value; i++) {
        p = &g_viewLightingRamps[i];
        p->drawBlend = FixMul(t, p->blend - p->previousBlend) + p->previousBlend;
        p->drawSunYaw = p->previousSunYaw + FixMul(t, p->sunYaw - p->previousSunYaw);
    }
}
void StageObject_ColourGroundMeshReferencePoint(DWORD *pColour, DWORD *pReference);
void StageObject_ColourObjectMeshReferencePoint(DWORD *pColour, DWORD *pReference);
void TrackLighting_SetLightMeshDiffuseColour(DWORD *pColour);
void TrackLighting_SetMesh5DiffuseColour(DWORD *pColour);
void TrackLighting_SetSkyDiffuseColour(DWORD *pColour);
void StageObject_YawMainAndSunNodes(int angle, int unused, int sunAngle);
void StageObject_OrientLightNodesToView(int view);
void TrackLighting_PushRecolouredMeshVertices(void);
void Track_SetFogAndSkyAlpha(DWORD *pColour, int start, int end);
void StageObject_UpdateSceneAmbientColour(BYTE *pColour);
void Stage_SetHeightColours(BYTE *pLow, BYTE *pHigh, BYTE *pReference, int referenceBlend);
void StageLights_UpdateDirection(void);
extern int g_lightingBlendFrom;
extern int g_lightingBlendTo;

// Blended ramp value and the two step sizes derived from it.
// GLOBAL: CMR2 0x00543d58
int g_sunIconWidthScale;
// GLOBAL: CMR2 0x00543d5c
int g_sunIconHeightScale;
// Stage object colour pushed straight to the stage mesh.
// GLOBAL: CMR2 0x00543eb4
BYTE g_sunIconColour[4];
// Set while the object ramps still have to be re-applied.
// GLOBAL: CMR2 0x00543ef8
int g_weatherRampsPending;
// GLOBAL: CMR2 0x00543f00
BillboardDef g_snowBillboard;

// Rebuilds every lighting colour of the stage from the current weather blend
// factor and pushes them to the stage meshes: height ramp (low/high/reference),
// ground, sky, light and ambient colours plus the sun light vector. Called once
// per view index by the stage renderer.
// g_stageLighting holds two 0xb4-byte expanded presets followed by the
// blend, intensity, current/previous lightning sectors and fog enable flag.
// FUNCTION: CMR2 0x00461c30
void StageObject_RebuildViewWeatherLighting(int index)
{
    ViewWeatherState *pView = &g_viewWeather[index];
    ViewLightingRamp *pObject = &g_viewLightingRamps[index];
    BYTE lowColour[4] = {0, 0, 0, 0xff};
    BYTE highColour[4] = {0, 0, 0, 0xff};
    BYTE rampColour[4] = {0, 0, 0, 0xff};
    BYTE referenceColour[4] = {0, 0, 0, 0xff};
    BYTE groundColour[4] = {0, 0, 0, 0xff};
    BYTE groundRefColour[4] = {0, 0, 0, 0xff};
    BYTE objectColour[4] = {0, 0, 0, 0xff};
    BYTE ambientColour[4] = {0, 0, 0, 0xff};
    BYTE objectRefColour[4] = {0xff, 0xff, 0xff, 0xff};
    BYTE lightColour[4] = {0xff, 0xff, 0xff, 0xff};
    BYTE skyColour[4] = {0xff, 0xff, 0xff, 0xff};
    BYTE heightColour[4] = {0xff, 0xff, 0xff, 0xff};
    FixVector v[3];
    FixVector colour;
    FixVector delta;
    FixVector ambientMix;
    int blend;
    int value;
    int flag;

    // 0x461cfe/0x461d12 require both changes before this refresh;
    // the lightning-sector and dirty-flag tests are independent.
    if ((pObject->drawBlend != g_stageLighting.blend && g_lightingBlendFrom != g_lightingBlendTo) ||
        g_stageLighting.previousLightningSector != g_stageLighting.lightningSector || pObject->lightingDirty != 0) {
        g_stageLighting.blend = pObject->drawBlend;
        pObject->lightingDirty = 0;
        if (g_stageLighting.blend > 0x10000)
            g_stageLighting.blend = 0x10000;
        if (g_stageLighting.lightningSector == -1 || pView->intensity <= 0xcccc || pView->kind != 1)
            flag = 0;
        else
            flag = 1;
        g_stageLighting.intensity = g_stageLighting.secondary.intensity - g_stageLighting.primary.intensity;
        g_stageLighting.intensity = FixMul(g_stageLighting.intensity, g_stageLighting.blend) + g_stageLighting.primary.intensity;

        v[2].x = g_stageLighting.secondary.lowColour.x - g_stageLighting.primary.lowColour.x;
        v[2].y = g_stageLighting.secondary.lowColour.y - g_stageLighting.primary.lowColour.y;
        v[2].z = g_stageLighting.secondary.lowColour.z - g_stageLighting.primary.lowColour.z;
        FixVecScale(&v[2], &v[2], g_stageLighting.blend);
        v[2].x += g_stageLighting.primary.lowColour.x;
        v[2].y += g_stageLighting.primary.lowColour.y;
        v[2].z += g_stageLighting.primary.lowColour.z;

        v[0].x = g_stageLighting.secondary.highColourDelta.x - g_stageLighting.primary.highColourDelta.x;
        v[0].y = g_stageLighting.secondary.highColourDelta.y - g_stageLighting.primary.highColourDelta.y;
        v[0].z = g_stageLighting.secondary.highColourDelta.z - g_stageLighting.primary.highColourDelta.z;
        FixVecScale(&v[0], &v[0], g_stageLighting.blend);
        v[0].x += g_stageLighting.primary.highColourDelta.x;
        v[0].y += g_stageLighting.primary.highColourDelta.y;
        v[0].z += g_stageLighting.primary.highColourDelta.z;
        lowColour[0] = (BYTE)(v[2].x >> 16);
        lowColour[1] = (BYTE)(v[2].y >> 16);
        lowColour[2] = (BYTE)(v[2].z >> 16);

        colour.x = v[0].x + v[2].x;
        colour.y = v[0].y + v[2].y;
        colour.z = v[0].z + v[2].z;
        highColour[0] = (BYTE)(colour.x >> 16);
        highColour[1] = (BYTE)(colour.y >> 16);
        highColour[2] = (BYTE)(colour.z >> 16);

        v[1].x = g_stageLighting.secondary.heightReferenceColour.x - g_stageLighting.primary.heightReferenceColour.x;
        v[1].y = g_stageLighting.secondary.heightReferenceColour.y - g_stageLighting.primary.heightReferenceColour.y;
        v[1].z = g_stageLighting.secondary.heightReferenceColour.z - g_stageLighting.primary.heightReferenceColour.z;
        FixVecScale(&v[1], &v[1], g_stageLighting.blend);
        v[1].x += g_stageLighting.primary.heightReferenceColour.x;
        v[1].y += g_stageLighting.primary.heightReferenceColour.y;
        v[1].z += g_stageLighting.primary.heightReferenceColour.z;
        referenceColour[0] = (BYTE)(v[1].x >> 16);
        referenceColour[1] = (BYTE)(v[1].y >> 16);
        referenceColour[2] = (BYTE)(v[1].z >> 16);

        colour.x = g_stageLighting.secondary.sunIconColour.x - g_stageLighting.primary.sunIconColour.x;
        colour.y = g_stageLighting.secondary.sunIconColour.y - g_stageLighting.primary.sunIconColour.y;
        colour.z = g_stageLighting.secondary.sunIconColour.z - g_stageLighting.primary.sunIconColour.z;
        FixVecScale(&colour, &colour, g_stageLighting.blend);
        colour.x += g_stageLighting.primary.sunIconColour.x;
        colour.y += g_stageLighting.primary.sunIconColour.y;
        colour.z += g_stageLighting.primary.sunIconColour.z;
        g_sunIconColour[3] = 0xff;
        g_sunIconColour[0] = (BYTE)(colour.x >> 16);
        g_sunIconColour[1] = (BYTE)(colour.y >> 16);
        g_sunIconColour[2] = (BYTE)(colour.z >> 16);

        value = g_stageLighting.secondary.sunIconSize - g_stageLighting.primary.sunIconSize;
        value = FixMul(value, g_stageLighting.blend) + g_stageLighting.primary.sunIconSize;
        g_sunIconWidthScale = FixMul(value, 0x66);
        g_sunIconHeightScale = FixMul(value, 0x88);
        value = g_stageLighting.secondary.heightReferenceBlend - g_stageLighting.primary.heightReferenceBlend;
        blend = FixMul(value, g_stageLighting.blend) + g_stageLighting.primary.heightReferenceBlend;

        colour.x = g_stageLighting.secondary.objectColour.x - g_stageLighting.primary.objectColour.x;
        colour.y = g_stageLighting.secondary.objectColour.y - g_stageLighting.primary.objectColour.y;
        colour.z = g_stageLighting.secondary.objectColour.z - g_stageLighting.primary.objectColour.z;
        FixVecScale(&colour, &colour, g_stageLighting.blend);
        colour.x += g_stageLighting.primary.objectColour.x;
        colour.y += g_stageLighting.primary.objectColour.y;
        colour.z += g_stageLighting.primary.objectColour.z;
        value = g_stageLighting.secondary.objectAlpha - g_stageLighting.primary.objectAlpha;
        value = FixMul(value, g_stageLighting.blend) + g_stageLighting.primary.objectAlpha;
        objectColour[0] = (BYTE)(colour.x >> 16);
        objectColour[1] = (BYTE)(colour.y >> 16);
        objectColour[2] = (BYTE)(colour.z >> 16);
        objectColour[3] = (BYTE)(value >> 16);

        colour.x = g_stageLighting.secondary.ambientColour.x - g_stageLighting.primary.ambientColour.x;
        colour.y = g_stageLighting.secondary.ambientColour.y - g_stageLighting.primary.ambientColour.y;
        colour.z = g_stageLighting.secondary.ambientColour.z - g_stageLighting.primary.ambientColour.z;
        FixVecScale(&colour, &colour, g_stageLighting.blend);
        colour.x += g_stageLighting.primary.ambientColour.x;
        colour.y += g_stageLighting.primary.ambientColour.y;
        colour.z += g_stageLighting.primary.ambientColour.z;
        FixVecScale(&colour, &colour, FixMul(0x3333, g_stageLighting.intensity) + 0xcccc);
        ambientColour[0] = (BYTE)(colour.x >> 16);
        ambientColour[1] = (BYTE)(colour.y >> 16);
        ambientColour[2] = (BYTE)(colour.z >> 16);
        ambientMix.x = colour.x;
        ambientMix.y = colour.y;
        ambientMix.z = colour.z;

        delta.x = g_stageLighting.secondary.groundColour.x - g_stageLighting.primary.groundColour.x;
        delta.y = g_stageLighting.secondary.groundColour.y - g_stageLighting.primary.groundColour.y;
        delta.z = g_stageLighting.secondary.groundColour.z - g_stageLighting.primary.groundColour.z;
        FixVecScale(&delta, &delta, g_stageLighting.blend);
        delta.x += g_stageLighting.primary.groundColour.x;
        delta.y += g_stageLighting.primary.groundColour.y;
        delta.z += g_stageLighting.primary.groundColour.z;
        delta.x -= ambientMix.x;
        delta.y -= ambientMix.y;
        delta.z -= ambientMix.z;
        FixVecScale(&delta, &delta, g_stageLighting.intensity);
        delta.x += ambientMix.x;
        delta.y += ambientMix.y;
        delta.z += ambientMix.z;
        groundColour[0] = (BYTE)(delta.x >> 16);
        groundColour[1] = (BYTE)(delta.y >> 16);
        groundColour[2] = (BYTE)(delta.z >> 16);
        value = g_stageLighting.secondary.groundReferenceBlend - g_stageLighting.primary.groundReferenceBlend;
        value = FixMul(value, g_stageLighting.blend) + g_stageLighting.primary.groundReferenceBlend;

        colour.x = v[1].x - delta.x;
        colour.y = v[1].y - delta.y;
        colour.z = v[1].z - delta.z;
        FixVecScale(&colour, &colour, FixMul(value, blend));
        colour.x += delta.x;
        colour.y += delta.y;
        colour.z += delta.z;
        colour.x -= ambientMix.x;
        colour.y -= ambientMix.y;
        colour.z -= ambientMix.z;
        FixVecScale(&colour, &colour, g_stageLighting.intensity);
        colour.x += ambientMix.x;
        colour.y += ambientMix.y;
        colour.z += ambientMix.z;
        groundRefColour[0] = (BYTE)(colour.x >> 16);
        groundRefColour[1] = (BYTE)(colour.y >> 16);
        groundRefColour[2] = (BYTE)(colour.z >> 16);

        colour.x = g_stageLighting.secondary.skyColour.x - g_stageLighting.primary.skyColour.x;
        colour.y = g_stageLighting.secondary.skyColour.y - g_stageLighting.primary.skyColour.y;
        colour.z = g_stageLighting.secondary.skyColour.z - g_stageLighting.primary.skyColour.z;
        FixVecScale(&colour, &colour, g_stageLighting.blend);
        colour.x += g_stageLighting.primary.skyColour.x;
        colour.y += g_stageLighting.primary.skyColour.y;
        colour.z += g_stageLighting.primary.skyColour.z;
        value = g_stageLighting.secondary.skyAlpha - g_stageLighting.primary.skyAlpha;
        value = FixMul(value, g_stageLighting.blend) + g_stageLighting.primary.skyAlpha;
        skyColour[0] = (BYTE)(colour.x >> 16);
        skyColour[1] = (BYTE)(colour.y >> 16);
        skyColour[2] = (BYTE)(colour.z >> 16);
        skyColour[3] = (BYTE)(value >> 16);

        if (g_stageLighting.fogEnabled != 0) {
            colour.x = g_stageLighting.secondary.fogColour.x - g_stageLighting.primary.fogColour.x;
            colour.y = g_stageLighting.secondary.fogColour.y - g_stageLighting.primary.fogColour.y;
            colour.z = g_stageLighting.secondary.fogColour.z - g_stageLighting.primary.fogColour.z;
            FixVecScale(&colour, &colour, g_stageLighting.blend);
            colour.x += g_stageLighting.primary.fogColour.x;
            colour.y += g_stageLighting.primary.fogColour.y;
            colour.z += g_stageLighting.primary.fogColour.z;
            heightColour[0] = (BYTE)(colour.x >> 16);
            heightColour[1] = (BYTE)(colour.y >> 16);
            heightColour[2] = (BYTE)(colour.z >> 16);
            heightColour[3] = 0xff;
            Track_SetFogAndSkyAlpha((DWORD *)heightColour,
                         g_stageLighting.primary.fogStart +
                             FixMul(g_stageLighting.blend, g_stageLighting.secondary.fogStart - g_stageLighting.primary.fogStart),
                         g_stageLighting.primary.fogEnd +
                             FixMul(g_stageLighting.secondary.fogEnd - g_stageLighting.primary.fogEnd, g_stageLighting.blend));
        }
        if (flag) {
            if (!(groundColour[0] <= 0xeb))
                groundColour[0] = 0xff;
            else
                groundColour[0] = (BYTE)(groundColour[0] + 0x14);
            if (groundColour[1] > 0xeb)
                groundColour[1] = 0xff;
            else
                groundColour[1] = (BYTE)(groundColour[1] + 0x14);
            if (groundColour[2] > 0xeb)
                groundColour[2] = 0xff;
            else
                groundColour[2] = (BYTE)(groundColour[2] + 0x14);
        }

        colour.x = g_stageLighting.secondary.mesh5Colour.x - g_stageLighting.primary.mesh5Colour.x;
        colour.y = g_stageLighting.secondary.mesh5Colour.y - g_stageLighting.primary.mesh5Colour.y;
        colour.z = g_stageLighting.secondary.mesh5Colour.z - g_stageLighting.primary.mesh5Colour.z;
        FixVecScale(&colour, &colour, g_stageLighting.blend);
        colour.x += g_stageLighting.primary.mesh5Colour.x;
        colour.y += g_stageLighting.primary.mesh5Colour.y;
        colour.z += g_stageLighting.primary.mesh5Colour.z;
        value = g_stageLighting.secondary.mesh5Alpha - g_stageLighting.primary.mesh5Alpha;
        value = FixMul(value, g_stageLighting.blend) + g_stageLighting.primary.mesh5Alpha;
        rampColour[0] = (BYTE)(colour.x >> 16);
        rampColour[1] = (BYTE)(colour.y >> 16);
        rampColour[2] = (BYTE)(colour.z >> 16);
        rampColour[3] = (BYTE)(value >> 16);

        colour.x = g_stageLighting.secondary.lightColour.x - g_stageLighting.primary.lightColour.x;
        colour.y = g_stageLighting.secondary.lightColour.y - g_stageLighting.primary.lightColour.y;
        colour.z = g_stageLighting.secondary.lightColour.z - g_stageLighting.primary.lightColour.z;
        FixVecScale(&colour, &colour, g_stageLighting.blend);
        colour.x += g_stageLighting.primary.lightColour.x;
        colour.y += g_stageLighting.primary.lightColour.y;
        colour.z += g_stageLighting.primary.lightColour.z;
        colour.x -= ambientMix.x;
        colour.y -= ambientMix.y;
        colour.z -= ambientMix.z;
        FixVecScale(&colour, &colour, g_stageLighting.intensity);
        colour.x += ambientMix.x;
        colour.y += ambientMix.y;
        colour.z += ambientMix.z;

        if (flag) {
            if (!(objectColour[3] <= 0xc8))
                objectRefColour[3] = 0xfa;
            else
                objectRefColour[3] = (BYTE)(objectColour[3] + 0x32);
            referenceColour[0] = 0xff;
            referenceColour[1] = 0xff;
            referenceColour[2] = 0xff;
            referenceColour[3] = 0xff;
            groundRefColour[0] = 0xff;
            groundRefColour[1] = 0xff;
            groundRefColour[2] = 0xff;
            groundRefColour[3] = 0xff;
            colour.x = 0xff0000;
            colour.y = 0xff0000;
            colour.z = 0xff0000;
            blend = 0x4ccc;
        } else {
            *(int *)objectRefColour = *(int *)objectColour;
        }

        StageObject_ColourObjectMeshReferencePoint((DWORD *)objectColour, (DWORD *)objectRefColour);
        Stage_SetHeightColours(lowColour, highColour, referenceColour, blend);
        StageObject_ColourGroundMeshReferencePoint((DWORD *)groundColour, (DWORD *)groundRefColour);
        TrackLighting_SetMesh5DiffuseColour((DWORD *)rampColour);
        TrackLighting_SetSkyDiffuseColour((DWORD *)skyColour);
        lightColour[3] = (BYTE)-(int)flag;
        TrackLighting_SetLightMeshDiffuseColour((DWORD *)lightColour);
        StageObject_UpdateSceneAmbientColour(ambientColour);
        StageObject_SetUnboostedStageLight(&colour);
        Track_ShiftMeshAndAmbientHeights(g_stageLighting.primary.meshHeightParameters.x + FixMul(g_stageLighting.secondary.meshHeightParameters.x - g_stageLighting.primary.meshHeightParameters.x, g_stageLighting.blend),
                     g_stageLighting.primary.meshHeightParameters.y, g_stageLighting.primary.meshHeightParameters.z);
        StageLights_UpdateDirection();
    }
    StageObject_YawMainAndSunNodes(0, 0, pObject->drawSunYaw);
    if (g_weatherRampsPending != 0)
        g_weatherRampsPending = 0;
    StageObject_OrientLightNodesToView(index);
    TrackLighting_PushRecolouredMeshVertices();
}

// Sets the scene's ambient colour when it changes.
// FUNCTION: CMR2 0x00462cb0
void StageObject_UpdateSceneAmbientColour(BYTE *pColour)
{
    BYTE ambient[4];

    Scene_GetAmbientColour((DWORD *)ambient);
    if (ambient[0] != pColour[0] || ambient[1] != pColour[1] || ambient[2] != pColour[2]) {
        if ((BYTE)RallyDataCountryIndex() == 3)
            Scene_SetAmbient(pColour, 0);
        else
            Scene_SetAmbient(pColour, 1);
    }
    if ((BYTE)RallyData_IsChampionshipFinalStage())
        StageObject_SetVehicleEffectColour(pColour);
}

// FUNCTION: CMR2 0x00462d10
void StageObject_UpdateSunVisibility(short *pRect)
{
    BYTE colour[4];
    union { int value; BYTE bytes[4]; } c;

    StageObject_GetGroundReferenceColour(&c.value);
    colour[0] = c.bytes[0];
    colour[1] = c.bytes[1];
    colour[2] = c.bytes[2];
    g_sunVisibility = 100 - Flare_SampleVisibility(pRect, colour, 0x28);
    if (g_sunVisibility < 0) {
        g_sunVisibility = 0;
        return;
    }
    if (g_sunVisibility > 100)
        g_sunVisibility = 100;
}

// ---------------------------------------------------------------------------
// Stage lights: up to eight glowing lamps (e.g. start lights) whose on/off
// pattern per state comes from g_stageLightStates, plus the car lamp textures.

void Frontend_SetObjectField118(struct Texture *pTexture, int value);

struct StageLight {
    GlowLight *pGlow;        // glow source
    GlowLight *pGlow2;       // second glow when the kind doubles them
    int level;          // 0..1, eased towards the pattern
};

// Position (x, y in units of the transform axes) of each light per kind.
// GLOBAL: CMR2 0x0051b1a8
int g_stageLightOffsets[4][8][2] = {
    -112787, 246022, -75038, 246022, -38273, 246022, -196, 246022, 37093, 246022, 74579, 246022, 111935, 246022, 0, 0,
    -238616, 170590, -238616, 143130, -238616, 114950, -215220, 175964, -215220, 160104, -215220, 144244, -215220, 128385, -215220, 112525,
    -624558, 203882, -604962, 203882, -585891, 203882, -567934, 203882, -550633, 203882, -531693, 203882, -513736, 203882, 0, 0,
    -567672, 203882, -548143, 203882, -529072, 203882, -511049, 203882, -493748, 203882, -474873, 203882, -456851, 203882,
};
// Depth of the lights per kind.
// GLOBAL: CMR2 0x0051b2a8
int g_stageLightDepth[4] = {
    -58982, -38469, -31653, -31653,
};
// Colour (0 red, 1 green) of each light per kind.
// GLOBAL: CMR2 0x0051b2b8
int g_stageLightColour[4][8] = {
    0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
};
// On/off pattern of each light, per kind (4), state (7) and light (8).
// GLOBAL: CMR2 0x0051b338
int g_stageLightStates[4][7][8] = {
    0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0,
    1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
    1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    1,
};
// Offset (along the light row's right axis) of each light's second glow;
// 0 when the kind has only one glow per light.
// GLOBAL: CMR2 0x0051b6b8
int g_stageLightDouble[4] = {
    0, 459341, 1048444, 944439,
};
// Glow size per kind and colour.
// GLOBAL: CMR2 0x0051b6c8
int g_stageLightSize[4][3] = {
    19660, 19660, 0, 13107, 13107, 6553, 9830, 9830,
    0, 9830, 9830,
};
// Countries (bit per stage index) that use the eight-light layout.
// GLOBAL: CMR2 0x0051b6f8
unsigned short g_stageLightEightMask[10] = {
    0x0000, 0x03ff, 0x03ff, 0x0000, 0x0002, 0x0200, 0x03ff, 0x0250,
};
// GLOBAL: CMR2 0x00547ad8
int g_stageLightKind;
// GLOBAL: CMR2 0x00547adc
int g_stageLightHasMatrix;
// GLOBAL: CMR2 0x00547ae0
StageLight g_stageLights[8];
// GLOBAL: CMR2 0x00547b40
FixMatrix g_stageLightMatrix;
// Indexed by light colour (red, green), followed by the current one.
// GLOBAL: CMR2 0x00547cc8
Texture *g_stageLightTextures[3];
#define g_stageLightRedTexture (g_stageLightTextures[0])
#define g_stageLightGreenTexture (g_stageLightTextures[1])
#define g_stageLightTexture (g_stageLightTextures[2])
// GLOBAL: CMR2 0x00547cd4
int g_stageLightsActive;
// GLOBAL: CMR2 0x00547cd8
int g_stageLightCount;

// Per car: the light block of the car file (a count followed by the
// CarLightPoint records) and a pointer to its first record.
// GLOBAL: CMR2 0x00547f80
CarLightProfile *g_carLightProfiles[8];
// Lamp textures of the two lamp layers, by slot: brake, reverse, hazard,
// second hazard, head.
// GLOBAL: CMR2 0x00547fac
Texture *g_carLightTexA[5];
// GLOBAL: CMR2 0x00547fc0
CarLightPoint *g_carLightPoints[8];
// GLOBAL: CMR2 0x00547ff8
Texture *g_carLightTexB[5];

// GLOBAL: CMR2 0x0051b724
char g_strLightRedTga[] = "\\NEWIMAGE\\lgt_red.tga";
// GLOBAL: CMR2 0x0051b70c
char g_strLightGreenTga[] = "\\NEWIMAGE\\lgt_gre.tga";
// GLOBAL: CMR2 0x0051b9d4
char g_strHazardLiteTga[] = "\\NEWIMAGE\\hazdlite.tga";
// GLOBAL: CMR2 0x0051b9bc
char g_strRevLiteTga[] = "\\NEWIMAGE\\revlite.tga";
// GLOBAL: CMR2 0x0051b9a4
char g_strHeadLiteTga[] = "\\NEWIMAGE\\headlite.tga";
// GLOBAL: CMR2 0x0051b98c
char g_strBrakeLiteTga[] = "\\NEWIMAGE\\brkelite.tga";

StageFile *StageTiming_GetStageFile0(void);

extern char g_strPathConcat[];

#define LOAD_STAGE_TEXTURE(dst, name)                                                          \
    sprintf(CFrontend::m_stringDest, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), name);    \
    dst = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile0(), CFrontend::m_stringDest, \
                                    &loaded, NULL, 0, 0)

// Places the lights: rows of the given 4x3 vectors are the right, up,
// forward axes and the position (NULL: no transform).
// FUNCTION: CMR2 0x00463290
void StageLights_SetTransform(FixVector *pAxes)
{
    FixVector v;
    FixVector *pNext;

    if (pAxes == NULL) {
        g_stageLightHasMatrix = 0;
        return;
    }
    g_stageLightHasMatrix = 1;
    v = *pAxes;
    pNext = pAxes + 1;
    FixMatrix_SetRight(&v, &g_stageLightMatrix);
    v = *pNext++;
    FixMatrix_SetUp(&v, &g_stageLightMatrix);
    v = *pNext;
    FixMatrix_SetForward(&v, &g_stageLightMatrix);
    pNext++;
    v = *pNext;
    FixMatrix_SetPosition(&v, &g_stageLightMatrix);
}

// FUNCTION: CMR2 0x00463360
void StageLights_LoadTextures(void)
{
    bool loaded;

    LOAD_STAGE_TEXTURE(g_stageLightRedTexture, g_strLightRedTga);
    LOAD_STAGE_TEXTURE(g_stageLightGreenTexture, g_strLightGreenTga);
    Frontend_SetObjectField118(g_stageLightRedTexture, 1);
    Frontend_SetObjectField118(g_stageLightGreenTexture, 1);
    g_stageLightTexture = g_stageLightRedTexture;
}

// Eases every light towards its pattern for the current state and updates
// its glow(s).
// FUNCTION: CMR2 0x00463bd0
void StageLights_Update(void)
{
    StageLight *p;
    int target;
    int d;
    int i;

    if (g_stageLightsActive == 0)
        return;
    for (i = 0, p = g_stageLights; i < g_stageLightCount; i++, p++) {
        target = g_stageLightStates[g_stageLightKind][g_unk0x00547b80][i] != 0 ? 0x10000 : 0;
        d = target - p->level;
        if (FIX_ABS(d) < 0x3333)
            p->level = target;
        else if (d > 0)
            p->level += 0x3333;
        else
            p->level -= 0x3333;
        if (p->level > 0) {
            Glow_SetEnabled(p->pGlow, 1);
            if (g_stageLightDouble[g_stageLightKind] != 0)
                Glow_SetEnabled(p->pGlow2, 1);
        } else {
            Glow_SetEnabled(p->pGlow, 0);
            if (g_stageLightDouble[g_stageLightKind] != 0)
                Glow_SetEnabled(p->pGlow2, 0);
        }
        Glow_SetIntensity(p->pGlow, p->level);
        if (g_stageLightDouble[g_stageLightKind] != 0)
            Glow_SetIntensity(p->pGlow2, p->level);
    }
}

// Turns every light off (state 6).
// FUNCTION: CMR2 0x00463d00
void StageLights_Off(void)
{
    int i;

    g_unk0x00547b80 = 6;
    if (g_stageLightsActive == 0)
        return;
    for (i = 0; i < g_stageLightCount; i++) {
        g_stageLights[i].level = 0;
        Glow_SetEnabled(g_stageLights[i].pGlow, 0);
        if (g_stageLightDouble[g_stageLightKind] != 0)
            Glow_SetEnabled(g_stageLights[i].pGlow2, 0);
    }
}

// Resets the glow table and loads the lamp textures of both cars.
// FUNCTION: CMR2 0x00463d60
void CarLights_LoadTextures(void)
{
    bool loaded;

    Glow_ResetEntries();
    CGame::RegisterCallback(Glow_ResetEntries, NULL);
    LOAD_STAGE_TEXTURE(g_carLightTexA[2], g_strHazardLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexA[1], g_strRevLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexA[4], g_strHeadLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexA[0], g_strBrakeLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexA[3], g_strHazardLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexB[2], g_strHazardLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexB[1], g_strRevLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexB[4], g_strHeadLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexB[0], g_strBrakeLiteTga);
    LOAD_STAGE_TEXTURE(g_carLightTexB[3], g_strHazardLiteTga);
}

void StageObject_GetRotatedStageLightVector(FixVector *pOut);
void StageObject_GetStageLightValues(int *pOut1, int *pOut2, int *pOut3);
void CarContact_SetSkidTrailAxis(FixVector *v);

// Direction of the stage light (sun direction raised by the sky offset),
// normalised; passed on to CarContact_SetSkidTrailAxis.
// GLOBAL: CMR2 0x005477f8
FixVector g_stageLightDirection;

// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00463070
void StageLights_UpdateDirection(void)
{
    FixVector d;
    FixVector s;
    int lift;
    int unused1;
    int unused2;
    int m;

    StageObject_GetRotatedStageLightVector(&d);
    StageObject_GetStageLightValues(&lift, &unused1, &unused2);
    d.y += lift;
    if (FIX_ABS(d.x) > FIX_ABS(d.y) && FIX_ABS(d.x) > FIX_ABS(d.z))
        m = FIX_ABS(d.x);
    else if (FIX_ABS(d.y) > FIX_ABS(d.x) && FIX_ABS(d.y) > FIX_ABS(d.z))
        m = FIX_ABS(d.y);
    else
        m = FIX_ABS(d.z);
    FixVecScaleRecip(&s, &d, m);
    FIX_NORMALIZE_INTO(g_stageLightDirection, s);
    CarContact_SetSkidTrailAxis(&g_stageLightDirection);
}

// Light row axes with the scale removed.
// GLOBAL: CMR2 0x00547b88
FixMatrix g_stageLightBasis;

unsigned char RallyDataStageIndex(void);
struct GlowLight;

#define LIGHT_SIZE(i) FixMul(lenX, g_stageLightSize[g_stageLightKind][g_stageLightColour[g_stageLightKind][i]])

// Creates the glows of the stage lights (start gantry) from the placed
// transform: layout by country and stage, one or two glows per light, all off.
// match 53%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00463410
void StageLights_Create(void)
{
    FixVector pos[8];
    FixVector v;
    FixVector local;
    FixVector zero;
    FixVector dir;
    FixVector origin;
    FixVector pair;
    int lenX, lenY, lenZ;
    BYTE stage;
    int i;

    local.x = 0;
    local.y = 0;
    local.z = -0x10000;
    zero.x = 0;
    zero.y = 0;
    zero.z = 0;
    g_stageLightsActive = 0;
    if (g_stageLightHasMatrix == 0)
        return;
    FixMatrix_GetPosition(&origin, &g_stageLightMatrix);
    if (origin.x == 0 && origin.y == 0 && origin.z == 0)
        return;
    stage = RallyDataStageIndex();
    if ((char)RallyDataStageIndex() == 10) {
        g_stageLightCount = 7;
        g_stageLightKind = 3;
        if ((char)RallyDataCountryIndex() == 3)
            g_stageLightKind = 2;
    } else if ((g_stageLightEightMask[RallyDataCountryIndex() & 0xff] & (unsigned short)(1 << stage)) == 0) {
        g_stageLightCount = 7;
        g_stageLightKind = 0;
    } else {
        g_stageLightCount = 8;
        g_stageLightKind = 1;
    }
    FixMatrix_GetRight(&v, &g_stageLightMatrix);
    lenX = FixVecLength(&v);
    FixVecScaleRecip(&v, &v, lenX);
    FixMatrix_SetRight(&v, &g_stageLightBasis);
    FixMatrix_GetUp(&v, &g_stageLightMatrix);
    lenY = FixVecLength(&v);
    FixVecScaleRecip(&v, &v, lenY);
    FixMatrix_SetUp(&v, &g_stageLightBasis);
    FixMatrix_GetForward(&v, &g_stageLightMatrix);
    lenZ = FixVecLength(&v);
    FixVecScaleRecip(&v, &v, lenZ);
    FixMatrix_SetForward(&v, &g_stageLightBasis);
    FixMatrix_RotateVector(&dir, &local, &g_stageLightBasis);
    FIX_NORMALIZE_INTO(dir, dir);
    FixMatrix_GetRight(&pair, &g_stageLightMatrix);
    FixVecScale(&pair, &pair, g_stageLightDouble[g_stageLightKind]);
    for (i = 0; i < g_stageLightCount; i++) {
        v.x = FixMul(lenX, g_stageLightOffsets[g_stageLightKind][i][0]);
        v.y = FixMul(lenY, g_stageLightOffsets[g_stageLightKind][i][1]);
        v.z = FixMul(g_stageLightDepth[g_stageLightKind], lenZ);
        FixMatrix_RotateVector(&pos[i], &v, &g_stageLightBasis);
        pos[i].x += origin.x;
        pos[i].y += origin.y;
        pos[i].z += origin.z;
        g_stageLights[i].pGlow = Glow_Add(
            2, &pos[i], &dir, &zero, LIGHT_SIZE(i), LIGHT_SIZE(i),
            g_stageLightTextures[g_stageLightColour[g_stageLightKind][i]],
            g_stageLightTextures[g_stageLightColour[g_stageLightKind][i]], 0, 0, 0, &dir, 0);
        if (g_stageLightDouble[g_stageLightKind] != 0) {
            pos[i].x += pair.x;
            pos[i].y += pair.y;
            pos[i].z += pair.z;
            g_stageLights[i].pGlow2 = Glow_Add(
                2, &pos[i], &dir, &zero, LIGHT_SIZE(i), LIGHT_SIZE(i),
                g_stageLightTextures[g_stageLightColour[g_stageLightKind][i]],
                g_stageLightTextures[g_stageLightColour[g_stageLightKind][i]], 0, 0, 0, &dir, 0);
        }
        g_stageLights[i].level = 0;
        Glow_SetEnabled(g_stageLights[i].pGlow, 0);
        if (g_stageLightDouble[g_stageLightKind] != 0)
            Glow_SetEnabled(g_stageLights[i].pGlow2, 0);
    }
    g_unk0x00547b80 = 6;
    g_stageLightsActive = 1;
}

// ---------------------------------------------------------------------------
// Replay buffers and stage event records.

extern int g_unk0x00588d3c;
extern ReplayStream *g_replayStreams[8];

// Both sets share a sixteen-entry slot table, traversed as a single array.
// GLOBAL: CMR2 0x00588d40
ReplayStream **g_replaySlots[16];
// GLOBAL: CMR2 0x00588ea0
ReplayStream *g_loadedReplayStreams[8];

// FUNCTION: CMR2 0x0046c540
void Replay_InitSlots(void)
{
    int i;

    g_unk0x00588d3c = 0;
    memset(g_replayStreams, 0, sizeof(g_replayStreams));
    for (i = 0; i < 8; i++)
        g_replaySlots[i] = &g_replayStreams[i];
    g_replayLevelState.bufferCount = 0;
    memset(g_loadedReplayStreams, 0, sizeof(g_loadedReplayStreams));
    for (i = 0; i < 8; i++)
        g_replaySlots[i + 8] = &g_loadedReplayStreams[i];
}


// GLOBAL: CMR2 0x00588ec8
BYTE g_unk0x00588ec8;

// Frees the replay buffers (the second set only when not needed any more).
// FUNCTION: CMR2 0x0046c6d0
int Replay_FreeBuffers(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (g_replayStreams[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_replayStreams[i]);
            g_replayStreams[i] = NULL;
        }
        if (CGameInfo::GetGameInfoSessionFlag() || CGameInfo::GetConfiguredGameMode() == 3 || CGameInfo::GetConfiguredGameMode() == 7) {
            if (g_loadedReplayStreams[i] != NULL && g_replayLevelState.pending[i] == 0) {
                CFileBuffer::FreeGenericFileBuffer(g_loadedReplayStreams[i]);
                g_loadedReplayStreams[i] = NULL;
                g_replayLevelState.pending[i] = 0;
            }
        }
    }
    g_unk0x00588d3c = 0;
    g_replayLevelState.bufferCount = 0;
    g_unk0x00588ec8 = 0;
    return 1;
}

// Stops recording into a replay buffer, closing the current segment.
// match 36%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046cc60
int Replay_StopRecording(BYTE *pBuffer)
{
    ReplayStream *p = (ReplayStream *)pBuffer;

    if (p == NULL || p->recording == 0)
        return 0;
    p->recording = 0;
    if (p->recordStarted != 0 && p->type != 2) {
        p->pLaneSamples[p->laneCount]++;
        p->laneCount++;
    } else if (p->type == 2) {
        p->laneCount = 1;
    }
    p->recordStarted = 0;
    p->field_0x18 = 0;
    return 1;
}

// GLOBAL: CMR2 0x0051bfec
char g_strReplaySaved[] = "\n";

// Writes a replay buffer (header plus its used arrays) to a file.
// FUNCTION: CMR2 0x0046d3f0
int Replay_Save(BYTE *pBuffer, char *pName)
{
    std::vector<unsigned char> bytes;
    if (!ReplayDisk_Write((ReplayStream *)pBuffer, bytes) || bytes.size() > UINT32_MAX)
        return 0;
    CInstallInfo::WriteFileToDisk(pName, 0, bytes.data(), (DWORD)bytes.size());
    puts(pName);
    puts(g_strReplaySaved);
    return 1;
}

// Points the arrays of a replay buffer into its data block (after the 0x110
// header), according to its type.
// FUNCTION: CMR2 0x0046d470
void Replay_SetupPointers(ReplayStream *p, int unused)
{
    int recordSize;
    int extraSize;

    if (p->type == 0) {
        p->pInputs = (ReplayInputLane *)((BYTE *)p + sizeof(ReplayStream));
        p->pStates = NULL;
        recordSize = 0x114c;
    } else {
        p->pStates = (ReplayStateLane *)((BYTE *)p + sizeof(ReplayStream));
        p->pInputs = NULL;
        recordSize = 0x5c;
    }
    if (p->type == 2) {
        p->pSamples = (BYTE *)p + p->laneCapacity * recordSize + sizeof(ReplayStream);
        p->pFrames = NULL;
        extraSize = 0x10;
    } else {
        p->pFrames = (BYTE *)p + p->laneCapacity * recordSize + sizeof(ReplayStream);
        p->pSamples = NULL;
        extraSize = 4;
    }
    p->pLaneSamples = (short *)((BYTE *)p + (p->samplesPerLane * extraSize + recordSize) * p->laneCapacity + sizeof(ReplayStream));
}

// Velocity between the two replay poses (from at +0, to at +0x40), minus three
// steps of the car's current velocity, over six: stored at +0x80.
// FUNCTION: CMR2 0x0046e340
void Replay_ComputePoseVelocityCorrection(ReplayPose *pPose, Car *pCar)
{
    FixVector to;
    FixVector from;
    FixVector d;
    FixVector drift;

    FixMatrix_GetPosition(&from, &pPose->from);
    FixMatrix_GetPosition(&to, &pPose->to);
    d.x = to.x - from.x;
    d.y = to.y - from.y;
    d.z = to.z - from.z;
    FixVecScale(&drift, &pCar->velocity, 0x30000);
    d.x -= drift.x;
    d.y -= drift.y;
    d.z -= drift.z;
    FixVecScale(&d, &d, 0x2aaa);
    pPose->velocity = d;
}

// Timed stage events (0x1c bytes each).
struct EventRec {
    int pos;            // 0x0  packed x/y of the event's area in the texture
    short a;            // 0x4  width
    short b;            // 0x6  height
    BYTE field_0x8[8];
    unsigned short step;    // 0x10
    short range;        // 0x12
    unsigned short counter; // 0x14
    short field_0x16;   // 0x16
    BYTE paused;        // 0x18
    BYTE active;        // 0x19 has an area
    BYTE pad_0x1a[2];
};

// GLOBAL: CMR2 0x00588ed8
EventRec g_eventRecords[11];
// GLOBAL: CMR2 0x00589210
int g_eventScales[2];
#define g_eventScale (g_eventScales[0])
// GLOBAL: CMR2 0x00589318
int g_pendingEventDrawCount;
// GLOBAL: CMR2 0x0058931c
int g_eventCount;
// GLOBAL: CMR2 0x00589320
int g_unk0x00589320[4];
// GLOBAL: CMR2 0x00589330
BYTE g_eventsDirty;
// GLOBAL: CMR2 0x00588ed0
Texture *g_eventTextures[2];
#define g_eventTexture (g_eventTextures[0])

// Scales the event steps by their share of the largest a*b product.
// FUNCTION: CMR2 0x0046e6a0
void Events_ComputeSteps(void)
{
    EventRec *p;
    int maxProduct;
    int product;
    int share;
    int i;

    maxProduct = 0;
    for (i = g_eventCount, p = g_eventRecords; i > 0; i--, p++) {
        product = p->b * p->a;
        if (product > maxProduct)
            maxProduct = product;
    }
    for (i = 0, p = g_eventRecords; i < g_eventCount; i++, p++) {
        share = FixDiv((p->b * p->a) << 16, (maxProduct / 2) << 16);
        p->step = (unsigned short)FixMul(share, g_eventScale);
        p->range = (short)FixMul(share, 30000);
        p->counter = 0;
        p->field_0x16 = 0;
        p->paused = 0;
    }
    for (i = 2; i < g_eventCount; i++) {
        g_eventRecords[i].step >>= 1;
        if (g_eventRecords[i].step == 0)
            g_eventRecords[i].step = 1;
    }
}

// GLOBAL: CMR2 0x00589331
BYTE g_unk0x00589331;

// GLOBAL: CMR2 0x00589334
int g_eventStampQuadrant;

// Stamp patterns of the event sprites: a 4x4 grid per rotation and a 16x16
// grid per rotation.
// GLOBAL: CMR2 0x0051c240
BYTE g_eventSpriteStamps[0x40] = {
    0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
};
// GLOBAL: CMR2 0x0051c280
BYTE g_unk0x0051c280[0x400] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void Events_Reset(void);

// Resets the stage events and finds the event texture (name ending in BODF).
// FUNCTION: CMR2 0x0046e580
void Events_Init(char *unused, int slot, char animate)
{
    int i;
    Texture *pTexture;

    Events_Reset();
    g_eventCount = 0;
    g_unk0x00589331 = animate == 0;
    // PORT: the slot is the car's index, but there are two slots. From car 2
    // the original wrote past both arrays, into g_eventRecords (which
    // Events_Add then rewrites) and the space after g_eventScales; here it
    // overwrote the pending draw count and crashed network rallies of three
    // or more cars. Only slot 0 is read.
    if (slot < 0 || slot >= 2)
        return;
    for (i = 0; i < 2048; i++) {
        pTexture = CGraphics::m_pTextureManager->textureBuffer[i];
        if (pTexture != NULL &&
            strncmp(pTexture->name + strlen(pTexture->name) - 8, CGraphics::m_strSuffixBODF, 4) == 0) {
            g_eventTextures[slot] = CGraphics::m_pTextureManager->textureBuffer[i];
            break;
        }
    }
    if (g_eventTextures[slot] != NULL)
        g_eventScales[slot] = g_eventTextures[slot]->width;
}

// Adds a stage event (up to 11) for the texture area pArea (packed position,
// width, height) and recomputes the event steps.
// FUNCTION: CMR2 0x0046e620
void Events_Add(EventRec *pArea, int unused)
{
    if (g_eventCount < 11 && g_eventTextures != NULL) {
        g_eventRecords[g_eventCount].active = 0;
        g_eventRecords[g_eventCount].paused = 0;
        g_eventRecords[g_eventCount].step = 0;
        g_eventRecords[g_eventCount].counter = 0;
        if (pArea != NULL && pArea->a > 0 && pArea->b > 0) {
            g_eventRecords[g_eventCount].pos = pArea->pos;
            *(int *)&g_eventRecords[g_eventCount].a = *(int *)&pArea->a;
            g_eventRecords[g_eventCount].active = 1;
        }
        g_eventCount++;
    }
    Events_ComputeSteps();
}

// FUNCTION: CMR2 0x0046e530
void Events_Reset(void)
{
    EventRec *p;
    int i;

    g_pendingEventDrawCount = 0;
    if (g_eventCount > 0) {
        p = g_eventRecords;
        for (i = g_eventCount; i != 0; i--, p++) {
            p->counter = 0;
            p->field_0x16 = 0;
            p->paused = 0;
        }
    }
    g_eventsDirty = 0;
    for (i = 0; i < 4; i++)
        g_unk0x00589320[i] = 0;
}

// Advances an event's counter; returns 1 when it wraps past 255.
// FUNCTION: CMR2 0x0046ed40
BYTE Events_Tick(int index)
{
    BYTE wrapped;

    wrapped = 0;
    if (g_eventRecords[index].paused == 0) {
        g_eventRecords[index].counter += g_eventRecords[index].step;
        if (g_eventRecords[index].counter >= 0x100) {
            g_eventRecords[index].counter = 0;
            wrapped = 1;
        }
    }
    return wrapped;
}

void Graphics_ReloadTexture(Texture *pTexture);

// FUNCTION: CMR2 0x0046ef20
void Events_Flush(void)
{
    if (g_eventCount > 0 && g_eventsDirty != 0) {
        Graphics_ReloadTexture(g_eventTexture);
        Events_Reset();
    }
}

// Cloud cover (0 light .. 2 heavy) of each weather setting.
// GLOBAL: CMR2 0x0051c688
BYTE g_cloudLevels[9] = { 1, 1, 2, 2, 2, 2, 2, 2, 2 };
// GLOBAL: CMR2 0x0051c694
char g_strCloudLight[] = "Cloud_Light";
// GLOBAL: CMR2 0x0051c6a0
char g_strCloudMed[] = "Cloud_Med";
// GLOBAL: CMR2 0x0051c6ac
char g_strCloudHeavy[] = "Cloud_Heavy";
// GLOBAL: CMR2 0x0051c6b8
char g_strPcLow[] = "PcLow\\";
// GLOBAL: CMR2 0x0051c6c0
char g_strPc[] = "Pc\\";
// GLOBAL: CMR2 0x0051c6c4
char g_strCloudsDir[] = "%s\\Clouds\\";

int *RallyData_GetDriverSettingPair(int index);

// Builds the path of the stage's cloud texture in CFrontend::m_stringDest
// from the heavier of the two weather settings.
// match 79%: the two level loads stay in byte registers (cl) instead of being
// zero-extended into 32-bit ones (bl/edx) as in the original
// FUNCTION: CMR2 0x0046ef50
// Stage-selected weather drives an added world-space density layer. The
// legacy textured sky stays as a background/fallback, not the ray mask.
void StageObject_PrepareVolumetricClouds(void)
{
    FixVector light={0,0x10000,0};
    if(g_stageSkyNode)StageObject_GetRotatedStageLightVector(&light);
    int *pair=RallyData_GetDriverSettingPair(RallyDataStageIndex());
    int level=0;
    if(pair && pair[0]>=0&&pair[0]<9&&pair[1]>=0&&pair[1]<9)
        level=g_cloudLevels[pair[0]]>g_cloudLevels[pair[1]]?g_cloudLevels[pair[0]]:g_cloudLevels[pair[1]];
    // The loaded light reference points toward the sun, not along its rays.
    Gfx_SetCloudEnvironment(level>=2?.72f:level==1?.56f:.42f,450.f,
        light.x/65536.f,light.y/65536.f,light.z/65536.f);
}

void StageObject_BuildCloudTexturePath(void)
{
    int *pPair;
    int level;
    char *suffix;

    sprintf(CFrontend::m_stringDest, g_strCloudsDir, CInstallInfo::GetTracksDir());
    if (CGameInfo::GetGraphicsOptionBits27To28() == 0)
        suffix = g_strPc;
    else
        suffix = g_strPcLow;
    strcat(CFrontend::m_stringDest, suffix);
    pPair = RallyData_GetDriverSettingPair(RallyDataStageIndex());
    level = 0;
    if (g_cloudLevels[pPair[0]] > 0)
        level = g_cloudLevels[pPair[0]];
    if (g_cloudLevels[pPair[1]] > level)
        level = g_cloudLevels[pPair[1]];
    switch (level) {
    case 2:
        strcat(CFrontend::m_stringDest, g_strCloudHeavy);
        break;
    case 1:
        strcat(CFrontend::m_stringDest, g_strCloudMed);
        break;
    case 0:
        strcat(CFrontend::m_stringDest, g_strCloudLight);
        break;
    }
}

// Releases the file loaded by 0x46f060 (registered callback).
// FUNCTION: CMR2 0x0046f030
BYTE StageObject_ReleaseLoadedObjectFile(void)
{
    if (g_stageCloudArchive.buffer) {
        CFileBuffer::FreeGenericFileBuffer(g_stageCloudArchive.buffer);
        g_stageCloudArchive.buffer = NULL;
    }
    g_stageCloudArchive.didFileLoad = FALSE;
    g_stageCloudArchive.fileSize = 0;
    return 1;
}

int View_GetActiveCameraMode(BYTE index);

// Places the four view nodes of a split screen from the current view position.
// match 80%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046f330
void StageObject_PositionSplitViewNodes(int param_1)
{
    int car;
    int node;
    FixVector pos;
    FixVector vecB;
    FixVector vecC;
    FixVector offset;

    car = param_1;
    node = View_GetActiveCameraMode(car);
    if (node == 3)
        FixMatrix_GetPosition(&pos, &Car_Get(car)->pBodyNode->current);
    else
        FixMatrix_GetPosition(&pos, &g_viewNodes[car]->current);
    pos.y = pos.y - 0xf0000;
    vecC.x = pos.x;
    vecC.z = pos.z;
    vecB.x = pos.x;
    vecB.z = pos.z;
    vecC.y = pos.y;
    vecB.y = pos.y;
    StageObject_GetStageLightValues(&offset.x, &offset.y, &offset.z);
    pos.y = pos.y + offset.x;
    vecC.y = vecC.y + offset.y;
    vecB.y = vecB.y + offset.z;
    if ((char)RallyDataCountryIndex() == 3 && (char)RallyDataStageIndex() == 7)
        vecB.y = vecB.y - 0x40000;
    if (g_stageSkyNode != 0)
        SceneNode_SetPosition(g_stageSkyNode, &pos);
    if (g_stageGroundNode != 0)
        SceneNode_SetPosition(g_stageGroundNode, &vecC);
    if (g_stageCloudNode != 0)
        SceneNode_SetPosition(g_stageCloudNode, &vecB);
    if (g_stageCloudTopNode != 0) {
        vecB.y = vecB.y - 0x140000;
        SceneNode_SetPosition(g_stageCloudTopNode, &vecB);
    }
    StageObject_GetStageLightState(&param_1);
    offset.x = 0;
    offset.y = param_1;
    offset.z = 0;
    FixMatrix_SetUp(&offset, &g_stageSkyNode->current);
}

void CarCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther);
void CarCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther);

#define RECORD_NEAR_90(v) (((v) > 0x3f4 && (v) < 0x40b) || ((v) < -0x3f4 && (v) > -0x40b))

// FUNCTION: CMR2 0x0048d7b0
void TracksideCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther)
{
    short v;

    v = g_unk0x00591750[g_viewCameraSpot[pRecord->index]].heading;
    if (RECORD_NEAR_90(v))
        CarCamera_BeginLeave(pRecord, pOther);
}

// FUNCTION: CMR2 0x0048d800
void TracksideCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther)
{
    short v;

    v = g_unk0x00591750[g_viewCameraSpot[pOther->index]].heading;
    if (RECORD_NEAR_90(v))
        CarCamera_BeginEnter(pOther, pRecord);
}

// Ground-normal view (+0x18) of the same 100 headlight-glow records. These
// interior views must share storage with the spawn/collision record base.

// Creates the glow of every record and resets the records.
// FUNCTION: CMR2 0x0047d510
void StageObject_CreateRecordGlows(void)
{
    FixVector unused;
    int i;

    for (i = 0; i < 100; i++) {
        g_carHeadlightGlowRecords[i].active = 0;
        g_carHeadlightGlowRecords[i].pGlow =
            Glow_Add(1, &unused, &unused, &unused, 0x3333, 0x3333, g_carLightTexA[2],
                     g_carLightTexB[2], 0x10000, 0, 0xb4, &unused, 0x20000);
        Glow_SetEnabled(g_carHeadlightGlowRecords[i].pGlow, 0);
        g_carHeadlightGlowRecords[i].height = 0;
        g_carHeadlightGlowRecords[i].triangle = -1;
        g_carHeadlightGlowRecords[i].normal.x = 0;
        g_carHeadlightGlowRecords[i].normal.y = 0x10000;
        g_carHeadlightGlowRecords[i].normal.z = 0;
    }
    memset(g_carHeadlightGlowCooldowns, 0, sizeof(g_carHeadlightGlowCooldowns));
}

#include "WheelTrail.h"

// Skid mark points of every car wheel: a used flag and a 0x28-byte record
// for each of the 200 points.
// GLOBAL: CMR2 0x00548220
BYTE g_trailPointUsed[8][4][TRAIL_MAX];
// GLOBAL: CMR2 0x00549b20
int g_unk0x00549b20[8][4];
// GLOBAL: CMR2 0x00549da0
TrailPoint g_trailPoints[8][4][TRAIL_MAX];
int StageObject_TrailRing(int car)
{
    (void)car;
    int n = Sys_GetOption("video.tire_track_length", 0);
    return n <= 200 ? 200 : n > TRAIL_MAX ? TRAIL_MAX : n;
}

// Clears every wheel's skid marks and trail state.
// FUNCTION: CMR2 0x00465530
void CarSkid_ClearWheelTrails(void)
{
    int car;
    int point;
    int wheel;
    int *p;
    int *q;

    for (car = 0; car < 8; car++) {
        for (point = 0; point < TRAIL_MAX; point++) {
            for (wheel = 0; wheel < 4; wheel++) {
                g_trailPointUsed[car][wheel][point] = 0;
                g_trailPoints[car][wheel][point].age = 0;
            }
        }
    }
    for (car = 0; car < 8; car++)
        for (wheel = 0; wheel < 4; wheel++)
            g_trailHist[car][wheel].Clear();
    p = (int *)g_trailDelta + 1;
    memset(g_unk0x00549ba0, 0, sizeof(g_unk0x00549ba0));
    memset(g_trailState, 0, sizeof(g_trailState));
    memset(g_trailTimer, 0, sizeof(g_trailTimer));
    memset(g_trailPrevState, 0, sizeof(g_trailPrevState));
    memset(g_unk0x00549b20, 0, sizeof(g_unk0x00549b20));
    memset(g_unk0x00543708, 0, sizeof(g_unk0x00543708));
    for (wheel = 0; wheel < 8 * 4; wheel++)
        ((int *)g_trailReset)[wheel] = 1;
    do {
        q = p;
        for (wheel = 0; wheel < 4; wheel++) {
            q[-1] = 0;
            q[0] = 0;
            q[1] = 0;
            q += 3;
        }
        p = q;
    } while ((INT_PTR)q < (INT_PTR)((int *)g_trailDelta + 8 * 12 + 1));
}

// GLOBAL: CMR2 0x0051bce0
char g_strNewImageSkidBlankTga[] = "\\NEWIMAGE\\skid\\skids_blank.tga";
// GLOBAL: CMR2 0x0051bd00
char g_strNewImageSkidMarkTga[] = "\\NEWIMAGE\\skid\\skids_mark.tga";
// GLOBAL: CMR2 0x0051bd20
char g_strNewImageSkid1Tga[] = "\\NEWIMAGE\\skid\\skids_1.tga";

// GLOBAL: CMR2 0x00588740
Texture *g_skidTrailTexture;
// GLOBAL: CMR2 0x00588744
Texture *g_skidMarkTexture;
// GLOBAL: CMR2 0x00588748
Texture *g_skidBlankTexture;

// Loads the three skid mark textures and picks the trail lifetime and the
// skid colour for the current weather and country.
// FUNCTION: CMR2 0x00465600
void StageObject_LoadSkidTrailTextures(void)
{
    char name[260];
    bool loaded;
    BYTE country;

    CarSkid_ClearWheelTrails();
    sprintf(name, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strNewImageSkid1Tga);
    g_skidTrailTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile0(), name, &loaded, 0, 0, 0);
    sprintf(name, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strNewImageSkidMarkTga);
    g_skidMarkTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile0(), name, &loaded, 0, 0, 0);
    sprintf(name, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strNewImageSkidBlankTga);
    g_skidBlankTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile0(), name, &loaded, 0, 0, 0);
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7)
        g_stageSurfaceInfo[8].flags = 4;
    else
        g_stageSurfaceInfo[8].flags = 2;
    country = (BYTE)RallyDataCountryIndex();
    if (country != 0) {
        if (country > 3 && country <= 5) {
            g_stageSurfaceInfo[2].red = 0xb7;
            g_stageSurfaceInfo[2].green = 0x89;
            g_stageSurfaceInfo[2].blue = 0x43;
            *(int *)&g_stageSurfaceInfo[7].red = *(int *)&g_stageSurfaceInfo[2].red;
        } else {
            g_stageSurfaceInfo[2].red = 0xb7;
            g_stageSurfaceInfo[2].green = 0x89;
            g_stageSurfaceInfo[2].blue = 0x43;
            *(int *)&g_stageSurfaceInfo[7].red = *(int *)&g_stageSurfaceInfo[2].red;
        }
    } else {
        g_stageSurfaceInfo[2].red = 0x89;
        g_stageSurfaceInfo[2].green = 0x7a;
        g_stageSurfaceInfo[2].blue = 0x6f;
        *(int *)&g_stageSurfaceInfo[7].red = *(int *)&g_stageSurfaceInfo[2].red;
    }
}

// GLOBAL: CMR2 0x00590b50
FixVector g_carLineImpulse;

int FixMatrix_InverseRotateVector(FixVector *pOut, FixVector *pV, FixMatrix *pM);
extern float g_oneOverRandMax;

// Applies an impulse (scaled by a random 0.7..1.0) against the vehicle's
// velocity, in its body frame.
// match 34%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004853c0
void StageObject_ApplyRandomizedBodyImpulse(FixVector *pImpulse)
{
    FixVector scaled;
    FixVector local;
    int random;
    int scale;

    FixMatrix_InverseRotateVector(&g_carLineImpulse, &g_partCar->velocity, g_partCar->pWorld);
    random = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_LOAD(rand()), g_oneOverRandMax), CGraphics::m_65536));
    scale = FixMul(0x4ccc, random) + 0xb333;
    FixVecScale(&scaled, pImpulse, scale);
    FixMatrix_InverseRotateVector(&local, &scaled, g_partCar->pWorld);
    g_carLineImpulse.x -= local.x;
    g_carLineImpulse.y -= local.y;
    g_carLineImpulse.z -= local.z;
}

void SceneNode_SetViewMaskTree(SceneNode *pNode, BYTE mask);
int Replay_DecodeInputPacket(ReplayStream *pState, BYTE *pIn, BYTE *pOut, BYTE *pCounter, Car *pCar);

// Hides every node of the cars whose replay buffer is a finished ghost run.
// FUNCTION: CMR2 0x0046e440
void Replay_HideFinishedGhostCarNodes(void)
{
    int i;
    ReplayStream *pBuffer;
    Car *pCar;

    for (i = 0; i < 16; i++) {
        pBuffer = *g_replaySlots[i];
        if (pBuffer != NULL && pBuffer->playing != 0 && pBuffer->type == 2 &&
            pBuffer->field_0xe8 == 0) {
            pCar = Car_Get(pBuffer->car);
            SceneNode_SetViewMaskTree(pCar->pBodyNode, 0);
            SceneNode_SetViewMaskTree(pCar->pWheelNodes[0], 0);
            SceneNode_SetViewMaskTree(pCar->pWheelNodes[1], 0);
            SceneNode_SetViewMaskTree(pCar->pWheelNodes[2], 0);
            SceneNode_SetViewMaskTree(pCar->pWheelNodes[3], 0);
            if (pCar->pAlternateBodyNode != NULL)
                SceneNode_SetViewMaskTree(pCar->pAlternateBodyNode, 0);
            if (pCar->pExtraNodes[0] != NULL) {
                SceneNode_SetViewMaskTree(pCar->pExtraNodes[0], 0);
                SceneNode_SetViewMaskTree(pCar->pExtraNodes[1], 0);
                SceneNode_SetViewMaskTree(pCar->pExtraNodes[2], 0);
                SceneNode_SetViewMaskTree(pCar->pExtraNodes[3], 0);
            }
        }
    }
}

// Decodes a replay input packet into a car's control record.
// FUNCTION: CMR2 0x0046c4b0
int Replay_DecodeCarControls(ReplayStream *pState, BYTE *pIn, BYTE car, BYTE *pCounter)
{
    Car *pCar = Car_Get(car);

    return Replay_DecodeInputPacket(pState, pIn, (BYTE *)pCar->flag0x1d0, pCounter, pCar);
}

// GLOBAL: CMR2 0x0058e0a0
Car *g_controlCar;
// Which of the steering/throttle/brake controls are analogue.
// GLOBAL: CMR2 0x0058e0a8
BYTE g_carAxisInputEnabled[3];
// Button bit of each car control (read from the controller mapping).
// GLOBAL: CMR2 0x0051f4b0
unsigned short g_carButtonMasks[9] = { 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100 };

short *Car_GetOrder(void);

// Resets a car's controls for the start of the stage (automatic box on,
// velocity damped).
// FUNCTION: CMR2 0x0047b870
void StageObject_ResetCarStartControls(int index)
{
    int driver = Car_GetOrder()[index];
    g_controlCar = Car_Get(driver);
    g_controlCar->field_0x1dc = 0;
    g_controlCar->handbrake = 0;
    g_controlCar->field_0x1d4[0] = 0;
    g_controlCar->flag0x1d0[3] = 0;
    g_controlCar->flag0x1d0[2] = 0;
    g_controlCar->flag0x1d0[1] = 0;
    g_controlCar->flag0x1d0[0] = 0;
    if (g_controlCar->automaticReverse != 0)
        g_controlCar->flag0x1d0[3] = 0;
    else
        g_controlCar->flag0x1d0[3] = 1;
    g_controlCar->flag0x1d0[2] = 0;
    g_controlCar->handbrake = 1;
    g_controlCar->automaticGearbox = 0;
    g_controlCar->field_0x1e4 = 0;
    FixVecScale(&g_controlCar->velocity, &g_controlCar->velocity, 0xf851);
}

DWORD Input_GetControllerAnalogSteering(unsigned short slot);

// Reads the controller mapping of a slot into the car control masks and the
// analogue flags.
// FUNCTION: CMR2 0x0047bca0
void StageObject_ReadCarControllerMapping(int slot)
{
    g_carButtonMasks[0] = CInput::GetButtonMapping(slot, 0);
    g_carButtonMasks[1] = CInput::GetButtonMapping(slot, 1);
    g_carButtonMasks[2] = CInput::GetButtonMapping(slot, 2);
    g_carButtonMasks[3] = CInput::GetButtonMapping(slot, 3);
    g_carButtonMasks[4] = CInput::GetButtonMapping(slot, 4);
    g_carButtonMasks[5] = CInput::GetButtonMapping(slot, 5);
    g_carButtonMasks[6] = CInput::GetButtonMapping(slot, 6);
    g_carButtonMasks[7] = CInput::GetButtonMapping(slot, 7);
    g_carButtonMasks[8] = CInput::GetButtonMapping(slot, 8);
    if (Input_GetControllerAnalogSteering(slot) == 0 || (int)CInput::GetEnabledControllerAxisBinding(slot, 0) == -1)
        g_carAxisInputEnabled[0] = 0;
    else
        g_carAxisInputEnabled[0] = 1;
    if (CInput::GetControllerAnalogPedals(slot) == 0 || (int)CInput::GetEnabledControllerAxisBinding(slot, 2) == -1)
        g_carAxisInputEnabled[1] = 0;
    else
        g_carAxisInputEnabled[1] = 1;
    if (CInput::GetControllerAnalogPedals(slot) != 0 && (int)CInput::GetEnabledControllerAxisBinding(slot, 3) != -1) {
        g_carAxisInputEnabled[2] = 1;
        return;
    }
    g_carAxisInputEnabled[2] = 0;
}

// Encodes a car's control record into a 4-byte replay packet.
// match 73%: identical up to the flag register (ECX vs EDX); reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046bdc0
void Replay_EncodeControlPacket(BYTE *pIn, BYTE *pOut, int active, int handbrake, int lightA, int lightB)
{
    int hb;
    int flag;
    BYTE v;
    BYTE byte3;

    hb = (handbrake != 0 && active != 0) ? 1 : 0;
    v = pOut[3];
    v = (v & 0x7f) | (hb << 7);
    pOut[3] = v;
    flag = (lightA != 0 && active != 0) ? 1 : 0;
    pOut[0] = ((pOut[0] ^ flag) & 1) ^ pOut[0];
    flag = 0;
    if (lightB != 0 && active != 0)
        flag = 1;
    pOut[0] = (flag << 1) | (pOut[0] & 0xfd);
    if (pIn[0] != 0) {
        v |= 0x40;
        pOut[3] = v;
        byte3 = pIn[0];
    } else {
        v &= 0xbf;
        pOut[3] = v;
        byte3 = pIn[1];
    }
    pOut[3] = ((byte3 ^ v) & 0x3f) ^ v;
    pOut[0] = (pIn[2] << 2) | (pOut[0] & 3);
    pOut[1] = (pIn[3] << 2) | (pOut[1] & 3);
    pOut[2] = (pIn[8] << 7) | (pOut[2] & 0x7f);
    pOut[1] = (((pIn[4] + 1) ^ pOut[1]) & 3) ^ pOut[1];
    v = ((pIn[0xc] & 1) << 6) | (pOut[2] & 0xbf);
    pOut[2] = (((v + 1) ^ v) & 0x3f) ^ v;
}

// Queued stage event draws (area, x, y, rows).
struct EventDraw {
    EventRec *pEvent;
    char x;
    char y;
    char rows;
    BYTE pad;
};
// GLOBAL: CMR2 0x00589010
EventDraw g_eventDraws[64];

// Stamps the pending event draws into one quadrant of the event texture and
// clears the pending list.
// match 37%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The unused argument is required by the original stdcall ret 4.
// FUNCTION: CMR2 0x0046e780
void StageObject_StampPendingEventDraws(int unused)
{
    RECT rect;
    short *pPos;
    short i;
    int j;
    int k;
    short x, y;
    int n;
    short idx;
    BYTE c;
    BYTE colour[4];

    if (g_eventCount <= 0)
        return;
    if (g_pendingEventDrawCount <= 0)
        return;
    colour[0] = 0x2f;
    colour[1] = 0x27;
    colour[2] = 0x14;
    colour[3] = 0xc0;
    g_eventStampQuadrant = g_eventStampQuadrant + 1;
    if (g_eventStampQuadrant > 3)
        g_eventStampQuadrant = 0;
    switch (g_eventStampQuadrant) {
    case 0:
        rect.left = 0;
        rect.top = 0;
        rect.right = g_eventTexture->width / 2 - 1;
        rect.bottom = g_eventTexture->height / 2 - 1;
        break;
    case 1:
        rect.left = g_eventTexture->width / 2 - 1;
        rect.top = 0;
        rect.right = g_eventTexture->width - 1;
        rect.bottom = g_eventTexture->height / 2 - 1;
        break;
    case 2:
        rect.left = 0;
        rect.top = g_eventTexture->height / 2 - 1;
        rect.right = g_eventTexture->width / 2 - 1;
        rect.bottom = g_eventTexture->height - 1;
        break;
    default:
        rect.left = g_eventTexture->width / 2 - 1;
        rect.top = g_eventTexture->height / 2 - 1;
        rect.right = g_eventTexture->width - 1;
        rect.bottom = g_eventTexture->height - 1;
        break;
    }
    CGraphics::LockTexture(g_eventTexture, &rect);
    i = 0;
    if (g_pendingEventDrawCount > 0) {
        do {
            j = 0;
            pPos = (short *)g_eventDraws[i].pEvent;
            x = (BYTE)g_eventDraws[i].x + pPos[0];
            y = (BYTE)g_eventDraws[i].y + pPos[1];
            if (CGameInfo::GetPreviewMode() == 0) {
                x = x * 4;
                y = y * 4;
            }
            idx = rand() % 4;
            n = CGameInfo::GetPreviewMode() ? 4 : 0x10;
            if (n > 0) {
                do {
                    k = 0;
                    do {
                        if ((CGameInfo::GetPreviewMode() == 0))
                            c = g_unk0x0051c280[(idx * 0x10 + j) * 0x10 + k];
                        else
                            c = g_eventSpriteStamps[(j + idx * 4) * 4 + k];
                        if (c != 0) {
                            if (rect.left < x + j && x + j < rect.right && rect.top < y + k &&
                                y + k < rect.bottom)
                                CGraphics::BlendPixel(g_eventTexture, x + j - rect.left,
                                                      y + k - rect.top, colour);
                        }
                        k++;
                    } while (k < n);
                    j++;
                } while (j < n);
            }
            i = i + 1;
            g_eventsDirty = 1;
        } while ((short)i < g_pendingEventDrawCount);
    }
    CGraphics::UnlockTexture(g_eventTexture);
    g_pendingEventDrawCount = 0;
}

BYTE Events_Tick(int index);

// Queues a draw of event `index` at (x, y) when it ticks and is on the area;
// pauses it once it has been drawn `range` times.
// match 53%: identical stream except the record offset lands in EDI (EBP in the original)
// FUNCTION: CMR2 0x0046ec40
void StageObject_QueueTimedEventDraw(int index, int x, int y)
{
    EventRec *p = &g_eventRecords[index];
    int over;

    if (index < g_eventCount && g_eventRecords[index].active != 0 && Events_Tick(index) && x >= 0 &&
        y >= 0 && x < p->a * 4 - 4 && y < p->b && g_pendingEventDrawCount < 0x40 && p != NULL) {
        over = y - p->b + 4;
        g_eventDraws[g_pendingEventDrawCount].pEvent = p;
        g_eventDraws[g_pendingEventDrawCount].x = (char)x;
        g_eventDraws[g_pendingEventDrawCount].y = (char)y;
        if (over > 0)
            g_eventDraws[g_pendingEventDrawCount].rows = 4 - (char)over;
        else
            g_eventDraws[g_pendingEventDrawCount].rows = 4;
        if ((unsigned short)g_eventRecords[index].field_0x16 >=
            (unsigned short)g_eventRecords[index].range) {
            g_eventRecords[index].paused = 1;
            g_pendingEventDrawCount++;
            return;
        }
        g_pendingEventDrawCount++;
        g_eventRecords[index].field_0x16++;
    }
}

// Sets the screen rectangles of the views from the screen size (full, top,
// bottom, left and right halves).
// FUNCTION: CMR2 0x00464b60
void StageObject_InitScreenRectangles(void)
{
    short *pFull = (short *)g_unk0x00548110[0];
    short *p = (short *)g_unk0x0051b9f0;
    int *pSize = (int *)g_pGraphics;
    int half;

    pFull[0] = 0;
    pFull[1] = 0;
    pFull[2] = (short)pSize[0];
    pFull[3] = (short)pSize[1];
    p[0] = 0;
    p[1] = 0;
    p[2] = (short)pSize[0];
    p[3] = (short)pSize[1];
    p[4] = 0;
    p[5] = 0;
    p[6] = (short)pSize[0];
    half = pSize[1] / 2;
    p[8] = 0;
    p[7] = (short)half;
    p[9] = (short)(pSize[1] / 2);
    p[10] = (short)pSize[0];
    half = pSize[1] / 2;
    p[12] = 0;
    p[11] = (short)half;
    p[13] = 0;
    p[14] = (short)(pSize[0] / 2);
    p[15] = (short)pSize[1];
    half = pSize[0] / 2;
    p[17] = 0;
    p[16] = (short)half;
    p[18] = (short)(pSize[0] / 2);
    p[19] = (short)pSize[1];
}

#define FIXVEC_EQ(a, b) ((a).x == (b).x && (a).y == (b).y && (a).z == (b).z)

// Interpolates every stage object's matrix between its two keys and flags
// the ones that moved.
// FUNCTION: CMR2 0x00471950
void StageObject_InterpolateAllObjectMatrices(int t)
{
    int i;
    FixMatrix old;
    MovingObject *pEntry;

    for (i = 0; i < g_movingObjects.count; i++) {
        pEntry = &g_movingObjects.entries[i];
        old = pEntry->current;
        FixMatrix_Interpolate(&pEntry->current, &pEntry->keyA, &pEntry->keyB, t, t, t, 1);
        if (FIXVEC_EQ(old.right, pEntry->current.right) && FIXVEC_EQ(old.up, pEntry->current.up) &&
            FIXVEC_EQ(old.forward, pEntry->current.forward) && FIXVEC_EQ(old.position, pEntry->current.position))
            pEntry->moved = 0;
        else
            pEntry->moved = 1;
    }
}

int StageObject_RunNodeActionAndSoundCallback(void *pNode, int value);

// Copies each stage object's interpolated matrix onto its scene node and
// calls the node refresh when the key changed.
// FUNCTION: CMR2 0x00471a60
void StageObject_ApplyInterpolatedNodeMatrices(int param_1)
{
    int i;
    MovingObject *p;

    i = 0;
    if (g_movingObjects.count > 0) {
        p = g_movingObjects.entries;
        do {
            p->pObject->current = p->current;
            if (p->moved != 0) {
                p->pObject->world = p->current;
                p->actionState = StageObject_RunNodeActionAndSoundCallback(p->pObject, p->actionState);
                p->moved = 0;
            }
            i = i + 1;
            p = p + 1;
        } while (i < (int)(g_movingObjects.count & 0xff));
    }
}

char Race_GetBaseCarCount(void);
void Car_ResetWheelLoadsAfterShift(Car *pCar);

// Seeds the stage's random numbers (unless replaying) and gives the computer
// cars their start revs by difficulty.
// FUNCTION: CMR2 0x0047c1e0
void StageObject_SeedRaceRandomAndCPURevs(char replay, char restart)
{
    int i;
    int r;
    int v;
    int start;
    Car *pCar;

    if (replay == 0 || restart != 0)
        g_stageRandomSeed = CMain::GetFrameTime();
    if (CGameInfo::GetGameInfoSessionFlag())
        g_stageRandomSeed = 0;
    srand(g_stageRandomSeed);
    rand();
    for (i = 0; i < (BYTE)Race_GetBaseCarCount(); i++) {
        r = rand();
        switch (CGameInfo::GetConfiguredDifficulty()) {
        case 0:
            v = r * 2;
            break;
        case 1:
            v = r + 0x8000;
            break;
        case 2:
            if (i < 3)
                v = 0x10000;
            else
                v = r + 0x8000;
            break;
        default:
            v = 0;
        }
        if (r > 0x4ccc)
            start = 0;
        else
            start = 0x10000 - v % 10;
        if (i >= (int)(RallyDataState() & 0xff)) {
            pCar = Car_Get(i);
            pCar->engineSpeed = FixMul(start, pCar->engineSpeedLimit);
            if (replay != 0 && restart == 0)
                Car_ResetWheelLoadsAfterShift(pCar);
        }
    }
}

void CarEffects_DrawTyreMarks(int index);

// Refreshes the skid trails of every car in the race order, skipping the
// replay-style modes where CGameInfo::GetGraphicsOptionBits25To26() returns 2.
// FUNCTION: CMR2 0x00465780
void StageObject_UpdateRaceSkidTrails(int param_1)
{
    short s;
    int i;

    if (CGameInfo::GetGraphicsOptionBits25To26() != 2) {
        g_trailDrawView = param_1 & 0xff;
        i = 0;
        s = Car_GetOrderCount();
        if (0 < s) {
            do {
                if (i < 8 && StageObject_GetPairedCarValue(i, param_1) != 7)
                    CarEffects_DrawTyreMarks(i);
                i = i + 1;
                s = Car_GetOrderCount();
            } while (i < s);
        }
    }
}

CarContact *StageObject_GetMotionRecord(int index);

// Centre of each wheel contact patch, raised by 0xccc for tyre trail rendering.
// GLOBAL: CMR2 0x00549c20
FixVector g_carTyreTrailCenters[8][4];

// Rebuilds each tyre trail centre halfway between the wheel patch edge midpoints.
// FUNCTION: CMR2 0x004657d0
void StageObject_RebuildCarExhaustPoints(int car)
{
    int wheel;
    FixVector *pA;
    FixVector *pB;
    FixVector d;

    if (car < 8 && StageObject_GetMotionRecord(car) != NULL) {
        for (wheel = 0; wheel < 4; wheel++) {
            pA = &StageObject_GetMotionRecord(car)->wheelFrontMid[wheel];
            pB = &StageObject_GetMotionRecord(car)->wheelRearMid[wheel];
            d.x = pA->x;
            d.x -= pB->x;
            d.y = pA->y - pB->y;
            d.z = pA->z - pB->z;
            FixVecScale(&d, &d, 0x8000);
            d.x += pB->x;
            d.y += pB->y;
            d.z += pB->z;
            g_carTyreTrailCenters[car][wheel] = d;
            g_carTyreTrailCenters[car][wheel].y += 0xccc;
        }
    }
}

BYTE Car_GetDrawnFlag(int index);
void Replay_EncodeControlPacket(BYTE *pIn, BYTE *pOut, int active, int handbrake, int lightA, int lightB);

// Encodes a car's controls into a replay packet.
// FUNCTION: CMR2 0x0046c450
void Replay_EncodeCarControls(BYTE *pOut, BYTE car)
{
    Car *pCar = Car_Get(car);
    DeviceInfo *pDev = CInput::GetAvailableDeviceRecord((char)Car_GetDrawnFlag(pCar->index));

    Replay_EncodeControlPacket((BYTE *)pCar->flag0x1d0, pOut, pDev->type == 3, pCar->steeringInputMode,
                 pCar->analogThrottle, pCar->analogBrake);
}

int Race_IsMultiplayerRecordMode10(void);

// Screen rectangle of a view: full screen with one player, else the half
// for the split direction.
// FUNCTION: CMR2 0x00464b10
BYTE *StageObject_GetPlayerViewRectangle(int view)
{
    StageObject_InitScreenRectangles();
    if ((BYTE)RallyDataState() != 1 && Race_IsMultiplayerRecordMode10() == 0) {
        if (CGameInfo::IsSplitBarEnabled())
            return (BYTE *)&g_unk0x0051b9f0[1 + view];
        return (BYTE *)&g_unk0x0051b9f0[3 + view];
    }
    return (BYTE *)g_unk0x0051b9f0;
}

// Starts a fresh stage object session.
// FUNCTION: CMR2 0x0047bda0
void StageObject_StartFreshSession(void)
{
    StageObject_ResetObjectValuesToUnity();
    StageObject_ClearUnusedCarSlotValues();
    StageObject_SeedRaceRandomAndCPURevs(0, 0);
    StageObject_SetBaseCarCountAndState();
}

// Starts replay mode with the selected restart flag.
// FUNCTION: CMR2 0x0047bdc0
void StageObject_StartReplaySession(char restart)
{
    StageObject_SeedRaceRandomAndCPURevs(1, restart);
}

// Checks whether a replay packet agrees with current controls.
// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Replay input packet (4 bytes, one per recorded frame).
struct ReplayPacket {
    BYTE b0lo : 2;
    BYTE b0hi : 6;
    BYTE b1lo : 2;
    BYTE b1hi : 6;
    BYTE count : 6;
    BYTE b2hi : 2;
    BYTE b3lo : 7;
    BYTE b3hi : 1;
};

// FUNCTION: CMR2 0x0046cbe0
int Replay_CanExtendControlPacketRun(BYTE *pPacket, BYTE car)
{
    ReplayPacket *packet = (ReplayPacket *)pPacket;
    ReplayPacket current;

    if (packet->count == 0)
        return 1;
    if (packet->count >= 0x3f)
        return 0;
    Replay_EncodeCarControls((BYTE *)&current, car);
    if (packet->b0hi == current.b0hi && packet->b1hi == current.b1hi && packet->b3lo == current.b3lo &&
        packet->b1lo == current.b1lo && packet->b2hi == current.b2hi && packet->b0lo == current.b0lo &&
        packet->b3hi == current.b3hi)
        return 1;
    return 0;
}

// Interpolates a stage object record between two frames.
// match 63%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00461710
void StageLights_InterpolatePreset(BYTE *out, BYTE *from, BYTE *to, int t)
{
    int i;

    for (i = 0x1c; i < 0x48; i += 4) {
        out[i] = (BYTE)StageObject_BlendClampedByteValues(to[i], from[i], t);
        out[i + 1] = (BYTE)StageObject_BlendClampedByteValues(to[i + 1], from[i + 1], t);
        out[i + 2] = (BYTE)StageObject_BlendClampedByteValues(to[i + 2], from[i + 2], t);
        out[i + 3] = (BYTE)StageObject_BlendClampedByteValues(to[i + 3], from[i + 3], t);
    }
    for (i = 0; i < 7; i++)
        ((int *)out)[i] = ((int *)from)[i] + FixMul(((int *)to)[i] - ((int *)from)[i], t);
    for (i = 0x48; i < 0x4e; i++)
        out[i] = (BYTE)StageObject_BlendClampedByteValues(to[i], from[i], t);
}

#include <stdlib.h>

// Unit vector of v into out (zero stays zero). The original inlines this
// as a function, not a macro: both pointer temporaries are homed up front.
inline void StageObj_NormalizeInto(FixVector *out, FixVector *v)
{
    int len = FixVecLength(v);

    if (len == 0) {
        out->x = 0;
        out->y = 0;
        out->z = 0;
    } else {
        FixVecScaleRecip(out, v, len);
    }
}


void Frontend_SetObjectField118(struct Texture *pTexture, int value);

// Finds the rev counter and digit textures for a player.
// FUNCTION: CMR2 0x00477340
void StageObject_FindPlayerRevTextures(int player)
{
    char base[260];
    int i;

    for (i = 0; i < (int)CGraphics::m_textureCount; i++) {
        _splitpath(CGraphics::m_pTextureManager->textureBuffer[i]->name, CFrontend::m_stringDest, CFrontend::m_stringDest,
                   base, CFrontend::m_stringDest);
        sprintf(CFrontend::m_stringDest, "%s", (char *)CGenericFileLoader::StrUpperPolish((BYTE *)base));
        if (strcmp(CFrontend::m_stringDest, CGraphics::m_strSuffixREVCT) == 0) {
            Frontend_SetObjectField118((g_carInteriorDashTextures[player].revCounter =
                                                             CGraphics::m_pTextureManager->textureBuffer[i]), 2);
        }
        if (strcmp(CFrontend::m_stringDest, CGraphics::m_strSuffixDIGIT) == 0)
            g_carInteriorDashTextures[player].digit = CGraphics::m_pTextureManager->textureBuffer[i];
    }
}

BYTE StageTiming_NoOpLightRelease(void);

// Starts stage objects and registers their frame callback.
// FUNCTION: CMR2 0x0048ca70
void StageObject_StartAndRegisterFrameCallback(void)
{
    StageObject_StartFreshSession();
    CGame::RegisterCallback(StageTiming_NoOpLightRelease, NULL);
}

// Allocates and registers a replay buffer.
// match 54%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046c5a0
BYTE *Replay_AllocateStreamBuffer(short frames, short samples, int type)
{
    ReplayStream *p;
    int recordSize;
    int extraSize;
    BYTE slot;

    if (*(BYTE *)&g_unk0x00588ec8 == 0) {
        CGame::RegisterCallback(Replay_FreeBuffers, NULL);
        *(BYTE *)&g_unk0x00588ec8 = 1;
    }
    if (g_unk0x00588d3c != 8) {
        if (type == 0)
            recordSize = frames * 0x114c;
        else
            recordSize = frames * 0x5c;
        if (type == 2)
            extraSize = samples * frames * 0x10;
        else
            extraSize = samples * frames * 4;
        p = (ReplayStream *)CFileBuffer::AllocateLockedBuffer(extraSize + frames * 2 + sizeof(ReplayStream) + recordSize);
        if (p != NULL) {
            p->type = type;
            p->laneCapacity = frames;
            p->samplesPerLane = samples;
            Replay_SetupPointers(p, 0);
            p->playing = 0;
            p->recording = 0;
            p->field_0x14 = 0;
            p->field_0x18 = 0;
            p->laneCount = 0;
            for (slot = 0; slot < 8; slot++) {
                if (g_replayStreams[slot] == NULL) {
                    g_replayStreams[slot] = p;
                    break;
                }
            }
            g_unk0x00588d3c++;
            return (BYTE *)p;
        }
    }
    return NULL;
}

// Loads a replay buffer and validates its recorded dimensions.
// match 38%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046d2d0
BYTE *Replay_LoadValidatedBuffer(char *path)
{
    if (g_unk0x00588ec8 == 0) {
        CGame::RegisterCallback(Replay_FreeBuffers, NULL);
        g_unk0x00588ec8 = 1;
    }
    if (g_replayLevelState.bufferCount >= 8) return NULL;
    DWORD size = 0;
    BYTE *file = (BYTE *)CFileBuffer::GetGenericFileBuffer(path, 1);
    const bool owned = file != NULL;
    if (owned)
        size = CGenericFileLoader::GetGenericFileSize();
    else
        file = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), path, NULL, &size, 0);
    if (!file) return NULL;
    // Archive members are borrowed: decode into owned native storage and
    // release only standalone file allocations, including on invalid input.
    ReplayStream *stream = ReplayDisk_Read(file, size);
    if (owned) CFileBuffer::FreeGenericFileBuffer(file);
    if (!stream) return NULL;
    g_replayLevelState.pending[g_replayLevelState.bufferCount] = 0;
    g_loadedReplayStreams[g_replayLevelState.bufferCount++] = stream;
    return (BYTE *)stream;
}

void FixMatrix_RotateAboutRight(FixMatrix *pOut, unsigned short angle);

// OpenCMR2 cockpit eye shift diagnostics (16.16): latest widening and eye displacement along the camera forward axis, per view.
static int g_lastCockpitWidening[8] = {65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536}, g_lastCockpitStep[8];
void View_GetCockpitShift(unsigned int view, int *widening, int *step) { *widening = g_lastCockpitWidening[view & 7]; *step = g_lastCockpitStep[view & 7]; }

// GLOBAL: CMR2 0x0051c9b0
short g_carMountRightRotationAngle = 0x71;

// Builds a stage object's world matrix from its car and mount point.
// match 41%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004778b0
void StageObject_BuildCarMountWorldMatrix(CameraRecord *object, void *unused)
{
    FixVector position = g_carDriverPoseStates[object->car].mountOffset;
    FixMatrix orient;
    FixMatrix mount;
    FixMatrix combined;

    FixMatrix_Identity(&mount);
    FixMatrix_SetPosition(&position, &mount);
    FixMatrix_Identity(&orient);
    orient.right.x = 0;
    orient.right.y = 0;
    orient.right.z = -0x10000;
    orient.up.x = 0;
    orient.up.y = 0x10000;
    orient.up.z = 0;
    orient.forward.x = 0x10000;
    orient.forward.y = 0;
    orient.forward.z = 0;
    FixMatrix_RotateAboutRight(&orient, (unsigned short)g_carMountRightRotationAngle);
    FixMatrix_SetPosition(&g_carDriverPoseStates[object->car].mountOffset, &orient);
    FixMatrix_Multiply(&combined, &orient,
                       &g_carInteriorNodeRows[object->car].driverPoseNode->current);
    FixMatrix_Multiply(&object->matrix, &combined,
                       Car_Get(object->car)->pBodyMatrix);
    {   // OpenCMR2 cockpit eye shift (visual only, default 0 = unchanged): moves the eye along the camera's forward axis by the view's widening.
        // Positive = forward axis, negative = the other way; which sign hides the model edges is to be tuned natively.
        int shiftPct = Sys_GetOption("video.cockpit_eye_shift", 0);
        {   // diagnostics: what this view's eye actually did on its latest build, for the F10 overlay
            int widening0 = View_CurrentWidening(object->index);
            g_lastCockpitWidening[object->index & 7] = widening0;
            g_lastCockpitStep[object->index & 7] = CockpitEyeShift_Amount(widening0, shiftPct);
        }
        if (shiftPct != 0) {
            int widening = View_CurrentWidening(object->index);
            int step = CockpitEyeShift_Amount(widening, shiftPct);
            object->matrix.position.x += FixMul(object->matrix.forward.x, step);
            object->matrix.position.y += FixMul(object->matrix.forward.y, step);
            object->matrix.position.z += FixMul(object->matrix.forward.z, step);
        }
    }
    {   // OpenCMR2 debug camera move (video.camera_move_cockpit), signed along the camera forward axis, + forward, - back, 0.05 model units per step, scaled by speed (0 standing still).
        int step = View_CameraMoveStep(3, object->index);
        if (step != 0) {
            object->matrix.position.x += FixMul(object->matrix.forward.x, step);
            object->matrix.position.y += FixMul(object->matrix.forward.y, step);
            object->matrix.position.z += FixMul(object->matrix.forward.z, step);
        }
    }
    object->liftPitch = 0;
    object->field_0x50 = 0;
    object->carLift = 0x10000;
    object->zoom = 0xa000;
    object->nearPlane = 0x1999;
    object->lookDistance = 0x10000;
}

GenericFile *Race_GetLoadedStageFile(void);
Menu *StageUI_GetCurrentPauseMenu(void);
void StageUI_DrawChampionshipBar(void);
// El prototipo real: devuelve int y el 5o parametro es BYTE (los llamadores de Game.cpp/RallyData.cpp
// y la definicion en Game.cpp:2575 coinciden en esa firma). Un prototipo viejo aqui generaba otro
// nombre manglado y rompia el enlace.
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
struct SceneNode *Sector_BuildC3DModelScene(void *data, struct SceneNode *parent, struct GenericFile *archive);
SceneNode *RallyData_GetChallengeSceneState(void);
int *RallyData_GetViewScreenRectangle(int view);
void NetRace_DrawPlayerFlashOverlay(unsigned int player, short *pRect, int check);
struct Menu;
void Menu_CallCallback2(Menu *pMenu);

// GLOBAL: CMR2 0x0051c950
char g_strTempObj[] = "TEMP.OBJ";
// GLOBAL: CMR2 0x0051c95c
char g_strTempSht[] = "TEMP.SHT";

// Replaces the active replay buffer with a freshly allocated one.
// FUNCTION: CMR2 0x00465f60
void Replay_ReplaceActiveBuffer(int frames, int samples)
{
    g_activeReplayStream = (ReplayStream *)Replay_AllocateStreamBuffer(frames, samples, 0);
    Replay_ResetSelectionIndices();
}

// Loads a replay buffer from a path, falling back to a 10-second default one.
// FUNCTION: CMR2 0x00465f90
void Replay_LoadPathOrDefaultBuffer(char *path)
{
    g_activeReplayStream = (ReplayStream *)Replay_LoadValidatedBuffer(path);
    if (g_activeReplayStream == NULL)
        Replay_ReplaceActiveBuffer(1, 0x1d4c);
    Replay_ResetSelectionIndices();
}

// Loads the stage's TEMP.OBJ model into memory.
// FUNCTION: CMR2 0x00472830
void StageObject_LoadTempObjectModel(void)
{
    void *pObj;
    GenericFile *pFile;

    pObj = CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), g_strTempObj, NULL, NULL, 0);
    if (pObj != NULL) {
        pFile = Race_GetLoadedStageFile();
        Sector_BuildC3DModelScene(pObj, RallyData_GetChallengeRenderState(), pFile);
    }
}

// Loads the stage's TEMP.SHT model into memory.
// FUNCTION: CMR2 0x00472870
void StageObject_LoadTempShadowModel(void)
{
    void *pObj;
    GenericFile *pFile;

    pObj = CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), g_strTempSht, NULL, NULL, 0);
    if (pObj != NULL) {
        pFile = Race_GetLoadedStageFile();
        Sector_BuildC3DModelScene(pObj, RallyData_GetChallengeRenderState(), pFile);
    }
}

// Clears the screen and redraws the split bars and championship positions.
// FUNCTION: CMR2 0x004759d0
void StageObject_ClearAndDrawSplitPositions(int unused1, int unused2)
{
    int i;

    // PORT: the Rally Challenge bracket is a full-screen menu, rendered
    // inside the race state machine rather than the frontend dispatcher.
    MenuScreen_Begin();
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    StageUI_DrawChampionshipBar();
    Menu_CallCallback2(StageUI_GetCurrentPauseMenu());
    i = 0;
    if ((BYTE)RallyDataState() > 0) {
        do {
            NetRace_DrawPlayerFlashOverlay(i, (short *)StageObject_GetPlayerViewRectangle(i), 0);
            i++;
        } while (i < (int)(RallyDataState() & 0xff));
    }
    Game_DrawSceneViewport(RallyData_GetChallengeRenderState(), RallyData_GetChallengeSceneState(), (void *)RallyData_GetViewScreenRectangle(0), 0, 0);
    Graphics_PresentFrameAndResetCounters();
    MenuScreen_End();
}

struct ObjectMatrix16 { int v[16]; };

// Builds a stage object's orientation matrix from a car and a node, flipping
// the right-hand column and rotating about the object's right axis.
// FUNCTION: CMR2 0x00477850
void StageObject_BuildCarNodeOrientation(CameraRecord *object, int *src)
{
    int *dst = (int *)&object->matrix;

    *(ObjectMatrix16 *)dst = *(ObjectMatrix16 *)src;
    dst[0] = -src[8];
    object->matrix.right.y = -src[9];
    object->matrix.right.z = -src[10];
    object->matrix.forward.x = src[0];
    object->matrix.forward.y = src[1];
    object->matrix.forward.z = src[2];
    FixMatrix_RotateAboutRight((FixMatrix *)dst, (unsigned short)g_carMountRightRotationAngle);
    StageObject_BuildCarMountWorldMatrix(object, src);
}

// Builds the four top-plane footprint points from the box's two horizontal
// axes and half extents, then translates them by the car's world position.
// Remaining mismatch: temporary stack slots and instruction scheduling.
// FUNCTION: CMR2 0x00486fc0
void Collision_BuildBoxFootprint(CollisionBox *pBox, FixVector *pPosition)
{
    FixVector lengthAxis, widthAxis;
    int corner;

    FixVecScale(&widthAxis, &pBox->axisA, pBox->halfWidth);
    FixVecScale(&lengthAxis, &pBox->axisB, pBox->halfLength);
    pBox->primaryPoints[0].x = widthAxis.x - lengthAxis.x;
    pBox->primaryPoints[0].y = widthAxis.y - lengthAxis.y;
    pBox->primaryPoints[0].z = widthAxis.z - lengthAxis.z;
    pBox->primaryPoints[1].x = lengthAxis.x + widthAxis.x;
    pBox->primaryPoints[1].y = lengthAxis.y + widthAxis.y;
    pBox->primaryPoints[1].z = lengthAxis.z + widthAxis.z;
    FixVecScale(&widthAxis, &widthAxis, -0x10000);
    pBox->primaryPoints[2].x = widthAxis.x - lengthAxis.x;
    pBox->primaryPoints[2].y = widthAxis.y - lengthAxis.y;
    pBox->primaryPoints[2].z = widthAxis.z - lengthAxis.z;
    pBox->primaryPoints[3].x = lengthAxis.x + widthAxis.x;
    pBox->primaryPoints[3].y = lengthAxis.y + widthAxis.y;
    pBox->primaryPoints[3].z = lengthAxis.z + widthAxis.z;
    for (corner = 0; corner < 4; corner++) {
        pBox->primaryPoints[corner].x += pPosition->x;
        pBox->primaryPoints[corner].y += pPosition->y;
        pBox->primaryPoints[corner].z += pPosition->z;
        pBox->primaryPoints[corner].y = pBox->top;
    }
}

// Reconstructs a world-space object collision box from its packed projection.
// The projection producer stores the axes, height interval and half extents
// in 16-bit units of 512; the four footprint points use the lower height.
// FUNCTION: CMR2 0x00487c40
void Collision_BuildObjectBox(CollisionBox *pBox, SectorObjectEntry *pEntry, FixVector *pPosition)
{
    ObjectPointBounds *pProjection = pEntry->pBounds->pProjection;
    FixVector widthAxis, lengthAxis;
    int corner;

    if (pBox->built == 0) {
        pBox->halfWidth = (int)pProjection->halfWidth << 9;
        pBox->halfLength = (int)pProjection->halfLength << 9;
        pBox->axisA.x = (int)pProjection->directionX << 9;
        pBox->axisA.y = 0;
        pBox->axisA.z = (int)pProjection->directionZ << 9;
        pBox->axisB.x = (int)pProjection->directionZ << 9;
        pBox->axisB.y = 0;
        pBox->axisB.z = -((int)pProjection->directionX << 9);
        pBox->top = pProjection->top * 0x200 + pPosition->y;
        pBox->bottom = pProjection->bottom * 0x200 + pPosition->y;
        pBox->pVertex = pPosition;
        pBox->pArray = 0;
        FixVecScale(&widthAxis, &pBox->axisA, pBox->halfWidth);
        FixVecScale(&lengthAxis, &pBox->axisB, pBox->halfLength);
        pBox->primaryPoints[1].x = widthAxis.x + lengthAxis.x;
        pBox->primaryPoints[1].z = widthAxis.z + lengthAxis.z;
        pBox->primaryPoints[0].x = widthAxis.x - lengthAxis.x;
        pBox->primaryPoints[0].z = widthAxis.z - lengthAxis.z;
        FixVecScale(&widthAxis, &widthAxis, -0x10000);
        pBox->primaryPoints[2].x = widthAxis.x - lengthAxis.x;
        pBox->primaryPoints[2].z = widthAxis.z - lengthAxis.z;
        pBox->primaryPoints[3].x = widthAxis.x + lengthAxis.x;
        pBox->primaryPoints[3].z = widthAxis.z + lengthAxis.z;
        for (corner = 0; corner < 4; corner++) {
            pBox->primaryPoints[corner].x += pPosition->x;
            pBox->primaryPoints[corner].y = pBox->bottom;
            pBox->primaryPoints[corner].z += pPosition->z;
        }
        pBox->built = 1;
    }
}

// Blends two colours according to a fade timer and writes the result.
// match 75%: colour blend block differs in scheduling/register use (same logic)
// match 74%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0047f510
void Firework_InterpolateRocketColours(FireworkRocket *p, BYTE *pOut, BYTE *pFrom, BYTE *pTo)
{
    int t = p->burstTime;
    FixVector c;
    int v, r, g, b;

    if (t > 0xa0000) {
        if (p->blinking != 0) {
            v = t - p->blinkTime;
            if (v > 0x50000) {
                *(DWORD *)pOut = *(DWORD *)pFrom;
                return;
            }
            if (v < -0x50000) {
                *(DWORD *)pOut = *(DWORD *)pTo;
                return;
            }
            v = FixMul(v, 0x1999) + 0x8000;
            c.x = (pFrom[0] << 16) - (pTo[0] << 16);
            c.y = (pFrom[1] << 16) - (pTo[1] << 16);
            c.z = (pFrom[2] << 16) - (pTo[2] << 16);
            FixVecScale(&c, &c, v);
            c.x += pTo[0] << 16;
            c.y += pTo[1] << 16;
            c.z += pTo[2] << 16;
            r = c.x;
            g = c.y;
            b = c.z;
            if (r > 0xff0000)
                r = 0xff0000;
            if (g > 0xff0000)
                g = 0xff0000;
            if (b > 0xff0000)
                b = 0xff0000;
            pOut[0] = (BYTE)(r >> 16);
            pOut[1] = (BYTE)(g >> 16);
            pOut[2] = (BYTE)(b >> 16);
            pOut[3] = 0xff;
            return;
        }
        *(DWORD *)pOut = *(DWORD *)pFrom;
        return;
    }
    v = FixMul(t, 0x1999);
    if (v > 0x10000)
        v = 0x10000;
    else if (v < 0)
        v = 0;
    if (p->blinking != 0) {
        c.x = pTo[0] << 16;
        c.y = pTo[1] << 16;
        c.z = pTo[2] << 16;
    } else {
        c.x = pFrom[0] << 16;
        c.y = pFrom[1] << 16;
        c.z = pFrom[2] << 16;
    }
    pOut[0] = (BYTE)(FixMul(c.x, v) >> 16);
    pOut[1] = (BYTE)(FixMul(c.y, v) >> 16);
    pOut[2] = (BYTE)(FixMul(c.z, v) >> 16);
    pOut[3] = 0xff;
}

// Sets per-car visibility bits used by the stage object renderer.
// match 72%: pairs of flag bytes are not scheduled in parallel like the original (same logic)
// match 72%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046b790
void StageObject_SetCarVisibilityBits(int type, int car, int index)
{
    BYTE bit = 1 << car;

    switch (type) {
    case 0:
        g_unk0x00588ba4[8] |= bit;
        g_unk0x00588ba4[9] |= bit;
        g_unk0x00588ba4[10] |= bit;
        g_unk0x00588ba4[11] |= bit;
    case 3:
        g_unk0x00588ba4[12] |= bit;
        g_unk0x00588ba4[13] |= bit;
        g_unk0x00588ba4[14] |= bit;
        break;
    case 1:
        g_unk0x00588ba4[8] |= bit;
        g_unk0x00588ba4[9] |= bit;
        g_unk0x00588ba4[10] |= bit;
        g_unk0x00588ba4[13] |= bit;
        g_unk0x00588ba4[14] |= bit;
        break;
    case 2:
        g_unk0x00588ba4[8] |= bit;
        g_unk0x00588ba4[9] |= bit;
        g_unk0x00588ba4[10] |= bit;
        return;
    case 4:
        g_unk0x00588ba4[12] |= bit;
        return;
    case 5:
        g_unk0x00588ba4[8] |= bit;
        break;
    case 8:
        g_unk0x00588ba4[8] |= bit;
        g_unk0x00588ba4[11] |= bit;
        return;
    default:
        return;
    }
    g_unk0x00588ba4[index] |= bit;
}

int NetPlayers_GetPlayerField8(int index);
unsigned int NetPlayers_IsPlayerPresent(int index);
unsigned int NetPlayers_GetPlayerFlag23(int index);

// Resets the per-view flags of a car's body, wheel and extra nodes.
// match 69%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046b8f0
void StageObject_ResetCarNodeViewFlags(Car *pCar)
{
    SceneNode *node;

    *(int *)(g_unk0x00588ba4 + 8) = 0;
    *(short *)(g_unk0x00588ba4 + 12) = 0;
    g_unk0x00588ba4[14] = 0;
    g_unk0x00588ba4[pCar->index] = 0;
    StageObject_SetCarVisibilityBits(g_replayLevelState.levels[pCar->index * 2], 0, pCar->index);
    StageObject_SetCarVisibilityBits(g_replayLevelState.levels[pCar->index * 2 + 1], 1, pCar->index);
    pCar->pSceneRoot->viewMask = 0xff;
    SceneNode_SetViewMaskTree(pCar->pWheelNodes[0], g_unk0x00588ba4[9]);
    SceneNode_SetViewMaskTree(pCar->pWheelNodes[1], g_unk0x00588ba4[9]);
    SceneNode_SetViewMaskTree(pCar->pWheelNodes[2], g_unk0x00588ba4[9]);
    SceneNode_SetViewMaskTree(pCar->pWheelNodes[3], g_unk0x00588ba4[9]);
    SceneNode_SetViewMaskTree(pCar->pBodyNode, g_unk0x00588ba4[10]);
    node = SceneNode_FindByType(pCar->pBodyNode, 6);
    if (node != NULL)
        node->viewMask = g_unk0x00588ba4[13];
    node = SceneNode_FindByType(pCar->pBodyNode, 0xe);
    if (node != NULL)
        node->viewMask = g_unk0x00588ba4[14];
    if (pCar->pAlternateBodyNode != NULL) {
        SceneNode_SetViewMaskTree(pCar->pAlternateBodyNode, g_unk0x00588ba4[12]);
        node = SceneNode_FindByType(pCar->pAlternateBodyNode, 6);
        if (node != NULL)
            node->viewMask = g_unk0x00588ba4[13];
        node = SceneNode_FindByType(pCar->pAlternateBodyNode, 0xe);
        if (node != NULL)
            node->viewMask = g_unk0x00588ba4[14];
    }
    if (pCar->pExtraNodes[0] != NULL)
        SceneNode_SetViewMaskTree(pCar->pExtraNodes[0], g_unk0x00588ba4[12]);
    if (pCar->pExtraNodes[1] != NULL)
        SceneNode_SetViewMaskTree(pCar->pExtraNodes[1], g_unk0x00588ba4[12]);
    if (pCar->pExtraNodes[2] != NULL)
        SceneNode_SetViewMaskTree(pCar->pExtraNodes[2], g_unk0x00588ba4[12]);
    if (pCar->pExtraNodes[3] != NULL)
        SceneNode_SetViewMaskTree(pCar->pExtraNodes[3], g_unk0x00588ba4[12]);
    if (g_carInteriorRoots[pCar->index] != 0)
        SceneNode_SetViewMaskTree(g_carInteriorRoots[pCar->index],
                                  g_unk0x00588ba4[11]);
    if (g_carDetailBodyNodes[pCar->index] != 0)
        g_carDetailBodyNodes[pCar->index]->viewMask = g_unk0x00588ba4[8];
    if (g_carDefaultBodyNodes[pCar->index] != 0)
        g_carDefaultBodyNodes[pCar->index]->viewMask = g_unk0x00588ba4[8];
    StageObject_ClearNodeValueBelowThreshold(pCar->pSceneRoot, 10);
    StageObject_ClearNodeTreeValuesBelowThreshold(pCar->pSceneRoot->pFirstChild, 10);
    StageObject_ClearNodeValueBelowThreshold(pCar->pBodyNode, 10);
    StageObject_ClearNodeTreeValuesBelowThreshold(pCar->pBodyNode->pFirstChild, 10);
}

// Rebuilds the per-wheel visibility values of the cars in race order.
// match 52%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046bb40
void StageObject_RebuildOrderedWheelVisibility(void)
{
    int flags[2];
    short *pOrder;
    Car *pCar;
    int i;
    short n;
    int index;
    unsigned int swap;

    i = 0;
    do {
        switch (View_GetActiveCameraMode(i)) {
        case 1:
            flags[i] = 6;
            break;
        case 2:
            flags[i] = 5;
            break;
        case 3:
            flags[i] = 8;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            flags[i] = 1;
            break;
        default:
            flags[i] = 0;
        }
        i++;
    } while (i < 2);
    pOrder = Car_GetOrder();
    for (n = Car_GetOrderCount() - 1; n >= 0; n--) {
        pCar = Car_Get(pOrder[n]);
        swap = (((Mesh *)pCar->pBodyNode->pObject)->flags >> 0x12) & 1;
        for (i = 0; i < 2; i++) {
            index = pCar->index;
            if ((View_GetActiveCameraFlags(i) & 0xff) == index) {
                if (flags[i] == 1 && g_carPairValuePending[index] != 0) {
                    if (swap)
                        StageObject_SetPairedCarValue(index, 4, i);
                    else
                        StageObject_SetPairedCarValue(index, 3, i);
                } else if (flags[i] == 1 && swap)
                    StageObject_SetPairedCarValue(index, 2, i);
                else
                    StageObject_SetPairedCarValue(index, flags[i], i);
            } else if ((BYTE)RallyDataState() > 1 && StageObject_UsesExtendedMode() == 0) {
                StageObject_SetPairedCarValue(pCar->index, 7, i);
            } else if (g_carPairValuePending[pCar->index] == 0) {
                if (swap)
                    StageObject_SetPairedCarValue(pCar->index, 2, i);
                else
                    StageObject_SetPairedCarValue(pCar->index, 1, i);
            } else {
                if (swap)
                    StageObject_SetPairedCarValue(pCar->index, 4, i);
                else
                    StageObject_SetPairedCarValue(pCar->index, 3, i);
            }
        }
    }
    if ((char)CGameInfo::GetGameModeOptionBit19() != 0) {
        for (i = 0; i < 7; i++) {
            if ((char)NetPlayers_IsPlayerPresent(i) == 0 && (char)NetPlayers_GetPlayerFlag23(i) != 0) {
                StageObject_SetPairedCarValue(NetPlayers_GetPlayerField8(i), 7, 0);
                StageObject_SetPairedCarValue(NetPlayers_GetPlayerField8(i), 7, 1);
            }
        }
    }
}

extern Car *g_collisionCar;

// Rotates both cockpit wipers about their current up axes, using the signed
// sweep limits and the model-class direction, then advances the sweep state.
// FUNCTION: CMR2 0x00476a40
void CarInterior_AnimateWipers(int index)
{
    int off = index * sizeof(CarInteriorNodes);
    // Retain the shared byte cursor and every node/profile reload.
#define INTERIOR_NODES ((CarInteriorNodes *)((BYTE *)g_carInteriorNodeRows + off))
#define INTERIOR_PROFILE ((CarInteriorProfilePointers *)((BYTE *)g_carInteriorProfilePointers + off))
    SceneNode *pNode;
    FixVector axis;
    FixVector position;

    pNode = INTERIOR_NODES->wiperNodes[0];
    if (pNode != NULL) {
        SceneNode_SetRotation(pNode, INTERIOR_PROFILE->firstWiperRotation);
        axis.x = (INTERIOR_NODES->wiperNodes[0])->current.up.x;
        axis.y = (INTERIOR_NODES->wiperNodes[0])->current.up.y;
        axis.z = (INTERIOR_NODES->wiperNodes[0])->current.up.z;
        short angle = g_carWiperStates[index].angle;
        if (g_carWiperDirectionReversed[index] != 0)
            angle = -angle;
        FixMatrix_FromAxisAngle(&g_carWiperRotationMatrix, &axis, angle);
        position.x = (INTERIOR_NODES->wiperNodes[0])->current.position.x;
        position.y = (INTERIOR_NODES->wiperNodes[0])->current.position.y;
        position.z = (INTERIOR_NODES->wiperNodes[0])->current.position.z;
        (INTERIOR_NODES->wiperNodes[0])->current.position.x = 0;
        (INTERIOR_NODES->wiperNodes[0])->current.position.y = 0;
        (INTERIOR_NODES->wiperNodes[0])->current.position.z = 0;
        FixMatrix_Multiply(&(INTERIOR_NODES->wiperNodes[0])->current, &(INTERIOR_NODES->wiperNodes[0])->current, &g_carWiperRotationMatrix);
        (INTERIOR_NODES->wiperNodes[0])->current.position.x = position.x;
        (INTERIOR_NODES->wiperNodes[0])->current.position.y = position.y;
        (INTERIOR_NODES->wiperNodes[0])->current.position.z = position.z;
    }
    pNode = INTERIOR_NODES->wiperNodes[1];
    if (pNode != NULL) {
        SceneNode_SetRotation(pNode, INTERIOR_PROFILE->secondWiperRotation);
        short *pNum = INTERIOR_PROFILE->secondWiperMaxAngle;
        axis.x = (INTERIOR_NODES->wiperNodes[1])->current.up.x;
        axis.y = (INTERIOR_NODES->wiperNodes[1])->current.up.y;
        axis.z = (INTERIOR_NODES->wiperNodes[1])->current.up.z;
        short *pDen = INTERIOR_PROFILE->firstWiperMaxAngle;
        int angle;
        if (g_carWiperDirectionReversed[index] != 0)
            angle = -(*pNum * g_carWiperStates[index].angle / *pDen);
        else
            angle = *pNum * g_carWiperStates[index].angle / *pDen;
        FixMatrix_FromAxisAngle(&g_carWiperRotationMatrix, &axis, angle);
        position.x = (INTERIOR_NODES->wiperNodes[1])->current.position.x;
        position.y = (INTERIOR_NODES->wiperNodes[1])->current.position.y;
        position.z = (INTERIOR_NODES->wiperNodes[1])->current.position.z;
        (INTERIOR_NODES->wiperNodes[1])->current.position.x = 0;
        (INTERIOR_NODES->wiperNodes[1])->current.position.y = 0;
        (INTERIOR_NODES->wiperNodes[1])->current.position.z = 0;
        FixMatrix_Multiply(&(INTERIOR_NODES->wiperNodes[1])->current, &(INTERIOR_NODES->wiperNodes[1])->current, &g_carWiperRotationMatrix);
        (INTERIOR_NODES->wiperNodes[1])->current.position.x = position.x;
        (INTERIOR_NODES->wiperNodes[1])->current.position.y = position.y;
        (INTERIOR_NODES->wiperNodes[1])->current.position.z = position.z;
    }
    CarInterior_UpdateWiperSweep(index);
#undef INTERIOR_NODES
#undef INTERIOR_PROFILE
}

// Rain intensity selects slow/fast wiper steps. Mode 1 finishes the current
// sweep; angle and step keep their signed-short overflow and strict limits.
int Windshield_FrontGlassBroken(int slot);   // WindshieldDirt.cpp
// FUNCTION: CMR2 0x00476c70
void CarInterior_UpdateWiperSweep(int index)
{
    short *pMax;

    // The 3D cockpit blades must obey the same control as the dirt overlay.
    // Off finishes the current sweep and parks. Rain must not restart it.
    // The front glass is broken (damage state 2, not merely cracked): the blades stop after finishing the current sweep.
    if (Sys_GetOption("video.windscreen_wipers", 1) == 0 || Windshield_FrontGlassBroken(index)) {
        if (g_carWiperStates[index].angle == 0) {
            g_carWiperStates[index].mode = 0;
            g_carWiperStates[index].sweepingForward = 1;
            return;
        }
        g_carWiperStates[index].mode = 1;
    } else if (StageObject_GetViewWeatherStateByte(index) == 1) {
        // Re-enable while parking must resume, even below old start threshold.
        if (g_carWiperStates[index].mode == 1)
            g_carWiperStates[index].mode = 0;
        if (StageObject_GetViewWeatherField54(index) > 0x3333 && g_carWiperStates[index].mode == 0) {
            g_carWiperStates[index].mode = 2;
            g_carWiperStates[index].step = rand() % 0xb + 0x2d;
        }
        if (StageObject_GetViewWeatherField54(index) > 0x8000 && g_carWiperStates[index].mode == 2) {
            g_carWiperStates[index].mode = 3;
            g_carWiperStates[index].step = rand() % 0xb + 0x5b;
        }
        if (StageObject_GetViewWeatherField54(index) < 0x1999 && g_carWiperStates[index].mode == 2)
            g_carWiperStates[index].mode = 1;
        if (StageObject_GetViewWeatherField54(index) < 0x6666 && g_carWiperStates[index].mode == 3) {
            g_carWiperStates[index].mode = 2;
            g_carWiperStates[index].step = rand() % 0xb + 0x2d;
        }
    } else {
        // OpenCMR2: the blades also run dry ("make em work always") - a slow
        // continuous sweep so they clear the dirt that builds without rain.
        // Parking (mode 1) still finishes its sweep first and then restarts.
        if (g_carWiperStates[index].mode == 0) {
            g_carWiperStates[index].mode = 2;
            g_carWiperStates[index].step = rand() % 0xb + 0x2d;
        } else if (g_carWiperStates[index].mode == 3) {
            g_carWiperStates[index].mode = 2;   // dry: never the fast rain step
            g_carWiperStates[index].step = rand() % 0xb + 0x2d;
        }
    }
    if (g_carWiperStates[index].mode != 0) {
        if (g_carWiperStates[index].sweepingForward != 0) {
            g_carWiperStates[index].angle += g_carWiperStates[index].step;
            pMax = ((CarInteriorProfilePointers *)((BYTE *)g_carInteriorProfilePointers + index * sizeof(CarInteriorProfilePointers)))->firstWiperMaxAngle;
            if (g_carWiperStates[index].angle > *pMax) {
                g_carWiperStates[index].sweepingForward = 0;
                g_carWiperStates[index].angle = *pMax;
                return;
            }
        } else {
            g_carWiperStates[index].angle -= g_carWiperStates[index].step;
            if (g_carWiperStates[index].angle < 0) {
                g_carWiperStates[index].sweepingForward = 1;
                if (g_carWiperStates[index].mode == 1)
                    g_carWiperStates[index].mode = 0;
                g_carWiperStates[index].angle = 0;
            }
        }
    }
}

// Updates per-wheel slip tables and damps the car's velocity.
// match 82%: below the 90% bar; kept as FUNCTION so reccmp measures it.
// FUNCTION: CMR2 0x0048df50
void CarPhysics_UpdateWheelSlipAndVelocityDamping(Car *param_1)
{
    int i;
    int off;
    int v;

    g_collisionCar = param_1;
    i = 0;
    off = 0;
    do {
        int a, b, d, u;

        v = g_collisionCar->speed;
        if (v > 0x10000)
            v = 0x10000;
        v = FixMul(v, 0x6666);
        a = g_collisionCar->corners[i].x;
        b = g_collisionCar->corners[i].z;
        if ((a < 0 ? -a : a) - (b < 0 ? -b : b) < 0)
            d = (b < 0 ? -b : b) - (a < 0 ? -a : a);
        else
            d = (a < 0 ? -a : a) - (b < 0 ? -b : b);
        u = FixMul((d % 0x401) << 6, v);
        if (g_collisionCar->objectContactState[off] == 0 ||
            u >= g_collisionCar->field_0x8fc[off]) {
            g_collisionCar->field_0x8fc[off] = u;
            g_collisionCar->objectContactState[off] = 1;
            g_collisionCar->field_0x564[i].x = 0;
            g_collisionCar->field_0x564[i].y = 0x10000;
            g_collisionCar->field_0x564[i].z = 0;
        }
        off++;
        i++;
    } while (off < 4);
    FixVecScale(&g_collisionCar->velocity, &g_collisionCar->velocity,
                0xf851);
}

// Builds the eight world corners of an oriented box: the half extents go
// through the columns of the rotation, the four corners of the positive half
// are combined from the three products and the other four are their
// negations; the centre is added last.
// FUNCTION: CMR2 0x00487140
void Collision_BuildOrientedBoxWorldCorners(FixVector *pCorners, FixVector *pCenter, FixMatrix *pRot, FixVector *pHalf)
{
    int i;
    int hx;
    int hy;
    int hz;
    int a;
    int b;
    int c;

    hx = pHalf->x;
    hy = pHalf->y;
    hz = pHalf->z;
    a = FixMul(pRot->right.x, hx);
    b = FixMul(pRot->up.x, hy);
    c = FixMul(pRot->forward.x, hz);
    pCorners[4].x = c + b + a;
    pCorners[5].x = (b - c) + a;
    pCorners[7].x = (b - c) - a;
    pCorners[6].x = (c - a) + b;
    a = FixMul(pRot->right.y, hx);
    b = FixMul(pRot->up.y, hy);
    c = FixMul(pRot->forward.y, hz);
    pCorners[4].y = c + b + a;
    pCorners[5].y = (b - c) + a;
    pCorners[7].y = (b - c) - a;
    pCorners[6].y = (c - a) + b;
    a = FixMul(pRot->right.z, hx);
    b = FixMul(pRot->up.z, hy);
    c = FixMul(pRot->forward.z, hz);
    pCorners[4].z = c + b + a;
    pCorners[5].z = (b - c) + a;
    pCorners[7].z = (b - c) - a;
    pCorners[6].z = (c - a) + b;
    pCorners[1].x = -pCorners[6].x;
    pCorners[1].y = -pCorners[6].y;
    pCorners[1].z = -pCorners[6].z;
    pCorners[2].x = -pCorners[5].x;
    pCorners[2].y = -pCorners[5].y;
    pCorners[2].z = -pCorners[5].z;
    pCorners[3].x = -pCorners[4].x;
    pCorners[3].y = -pCorners[4].y;
    pCorners[3].z = -pCorners[4].z;
    pCorners[0].x = -pCorners[7].x;
    pCorners[0].y = -pCorners[7].y;
    pCorners[0].z = -pCorners[7].z;
    pCorners[0].x += pCenter->x;
    pCorners[0].y += pCenter->y;
    pCorners[0].z += pCenter->z;
    {
        FixVector *p = &pCorners[1];
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    p++;
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    p++;
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    p++;
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    p++;
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    p++;
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    p++;
    p->x += pCenter->x;
    p->y += pCenter->y;
    p->z += pCenter->z;
    }
}

int Car_GetWheelSpeed(Car *pCar, BYTE wheel, int unit);
void StageTiming_CollectDescriptorCarValues(char *pDesc, int *pValues, int *pOut);
void CarSkid_SpawnWheelDustPuff(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6,
                  int param_7, int param_8);

// Spawns a dust/smoke puff at a randomised position relative to a wheel.
// match 48%: randomised offset evaluation order differs from the original
// match 47%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046ed80
void CarSkid_SpawnWheelDustPuff(int param_1, int param_2, int param_3, int param_4, int param_5, int param_6,
                  int param_7, int param_8)
{
    short s;
    int x1, x2, a, b, c;
    int amp;

    s = (short)(rand() % (param_8 / 2));
    param_4 = (s * param_4) / (param_8 / 2);
    rand();
    if (s > 0x400) {
        amp = param_5;
    } else {
        // param_5 and param_8 are dead here; the original reuses their slots.
        param_5 = (int)FTOI(__int64, X87S_MUL((double)param_5, CGraphics::m_65536));
        param_8 = g_sinTable[s & 0xfff];
        amp = FIX_ABS(FixMul(param_8, param_5)) >> 0x10;
    }
    a = rand() % (amp + 1);
    x2 = a * param_7 + param_3;
    b = (rand() % 0x11 - 8) / (rand() % 3 + 1);
    x1 = (param_4 - 0x10) * param_6 + param_2 + b;
    c = (rand() % 0x11 - 8) / (rand() % 3 + 1);
    StageObject_QueueTimedEventDraw(param_1, x1, x2 + c);
}

// Emits skid/dust effects for the wheels that are slipping.
// match 49%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0046ea80
void CarSkid_EmitSlippingWheelEffects(int param_1, int param_2)
{
    int i;

    i = 2;
    if (param_1 == 2 || param_1 == 3) {
        EventRec *p;
        if (g_eventCount > 2) {
            p = &g_eventRecords[2];
            do {
                int r1 = rand();
                short s = p->a;
                int r2 = rand();
                StageObject_QueueTimedEventDraw(i, (r1 * s) / 0x7fff, (r2 * p->b) / 0x7fff);
                i++;
                p++;
            } while (i < g_eventCount);
        }
    } else {
        int e = (param_1 != 0) ? 1 : 0;
        int va = g_eventRecords[e].a;
        int vd = va / 5;
        int vb = g_eventRecords[e].b;
        int vf = va - vd;
        int vg = vb - 8;
        int x, y;
        if (param_2 < 0x8000)
            param_2 = 0;
        else if (param_2 > 0x18000)
            param_2 = 0x18000;
        x = (param_2 * (vf - vd)) / 0xb333;
        y = (param_2 * vg) / 0xb333;
        if (x > 0x20) {
            if (param_1 == 0) {
                if (g_unk0x00589331 != 0)
                    CarSkid_SpawnWheelDustPuff(0, vd, vb - 1, x, y, 1, -1, 0x4fa);
                else
                    CarSkid_SpawnWheelDustPuff(0, vf - 1, 1, x, y, -1, 1, 0x4fa);
            } else if (g_unk0x00589331 != 0) {
                CarSkid_SpawnWheelDustPuff(1, vd, 1, x, y, 1, 1, 0x4fa);
            } else {
                CarSkid_SpawnWheelDustPuff(1, vf - 1, vb - 1, x, y, -1, -1, 0x4fa);
            }
        }
    }
    g_unk0x00589320[param_1] = g_unk0x00589320[param_1] + 1;
}

// Spawns skid effects for all four wheels of a car.
// FUNCTION: CMR2 0x0046ea10
void StageObject_SpawnCarWheelSkidEffects(int param_1)
{
    int i;

    if (g_eventCount > 0) {
        Car_Get(param_1);
        i = 0;
        do {
            if (StageObject_IsWheelOnActiveEffectSurface(param_1, i) != 0) {
                int v = Car_GetWheelSpeed(Car_Get(param_1), (BYTE)i, 0);
                if (v > 0x1e0000) {
                    int n = 2;
                    do {
                        CarSkid_EmitSlippingWheelEffects(i, v / 0x3c);
                        n--;
                    } while (n != 0);
                }
            }
            i++;
        } while (i < 4);
    }
}

// GLOBAL: CMR2 0x0058e088
int g_unk0x0058e088[6];

// Projects a stage object's rotation table into its output rows.
// match 23%: nested projection loops do not match the original layout
// match 23%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0047ca30
void StageObject_ProjectRotationTableRows(AINetwork *param_1)
{
    char *pRow;
    int **ppData;
    int *pDst;
    int row;

    if (param_1->layerCount > 1) {
        pRow = param_1->layerSizes;
        pDst = (int *)g_unk0x0058e0b8;
        ppData = &param_1->pWeights[1];
        row = 0xc;
        do {
            int j = 1;
            // Weights are consecutive for every output of this layer. The
            // original advances this cursor across neurons, not just inputs.
            int *pData = *ppData;
            if (pRow[1] > 0) {
                do {
                    int *pTable;
                    int sum;
                    unsigned int cols;
                    sum = 0;
                    pTable = pDst;
                    cols = *pRow + 1;
                    if (cols > 0) {
                        do {
                            if (*pData != 0)
                                sum += FixMul(*pTable, *pData);
                            pData++;
                            pTable++;
                            cols--;
                        } while (cols != 0);
                    }
                    if (param_1->layerCount - 1 < (int)(pRow - param_1->layerSizes) + 1) {
                        ((int *)g_unk0x0058e0b8)[row + j] = sum;
                    } else {
                        int idx = FixDiv(sum, 0x10000000);
                        if (idx < 0) {
                            if (-idx >= 0x40)
                                ((int *)g_unk0x0058e0b8)[row + j] = 0xffff0000;
                            else
                                ((int *)g_unk0x0058e0b8)[row + j] = -g_aiRouteTables.pActivation[-idx];
                        } else if (idx < 0x40) {
                            ((int *)g_unk0x0058e0b8)[row + j] = g_aiRouteTables.pActivation[idx];
                        } else {
                            ((int *)g_unk0x0058e0b8)[row + j] = 0x10000;
                        }
                    }
                    j++;
                } while (j <= pRow[1]);
            }
            row += 0xc;
            ppData++;
            pRow++;
            pDst += 0xc;
        } while ((int)(pRow - param_1->layerSizes) + 1 < param_1->layerCount);
    }
}

// Builds a stage object's per-row output from its type tables.
// FUNCTION: CMR2 0x0047c9a0
void StageObject_BuildTypeTableRowOutput(int param_1, int *pControls, int *param_3)
{
    unsigned int mask = 0;
    int i = 0;

    do {
        BYTE b = *(BYTE *)(g_aiRouteTables.descriptors[param_1] + 1 + i);
        int v = (int)(signed char)b;
        if (v != 0) {
            unsigned int bit = 1 << v;
            if ((mask & bit) == 0) {
                mask |= bit;
                StageTiming_CollectDescriptorCarValues(g_aiRouteTables.networks[v]->inputs, param_3, (int *)g_unk0x0058e0b8);
                StageObject_ProjectRotationTableRows(g_aiRouteTables.networks[v]);
            }
            // The original addresses the projection rows through 0x58e088,
            // 0x30 bytes before 0x58e0b8. These globals need not be adjacent
            // after linking, so address the actual row storage directly.
            *(int *)((BYTE *)pControls + i) =
                ((int *)g_unk0x0058e0b8)[(int)*(char *)(g_aiRouteTables.descriptors[param_1] + i) +
                                       (g_aiRouteTables.networks[v]->layerCount - 1) * 0xc];
        }
        i += 4;
    } while (i < 0x14);
}

int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri,
                                 short *pSurfaceClass, unsigned short *pSurface, int defaultY);

// Updates each corner's ground height (and surface) against the track; the
// corners past `count` copy the height of the corner four places before.
// FUNCTION: CMR2 0x004930e0
void StageObject_UpdateCarCornerGroundHeights(Car *pCar, int count)
{
    CarPartSet *p;
    short i;
    short missing;

    p = CarPart_GetSet(pCar->index);
    missing = 0;
    i = 0;
    if (count > 0) {
        do {
            int v = Track_GetGroundHeightSurface(&pCar->corners[i], &pCar->cornerAxis[i],
                                                 &pCar->cornerTriangle[i], &pCar->wheelSurface[i],
                                                 (unsigned short *)&pCar->wheelSurfaceType[i],
                                                 pCar->cornerHeight[i]);
            pCar->cornerHeight[i] = v;
            if (pCar->cornerTriangle[i] == -1)
                missing = missing + 1;
            if (i < 4) {
                int t = pCar->field_0x978[i] + p->damageValues[6 + i];
                if (t > 0x10000)
                    t = 0x10000;
                pCar->cornerHeight[i] += FixMul(t, pCar->field_0x938[i]);
            }
            if (pCar->objectContactState[i] != 0) {
                pCar->cornerHeight[i] += pCar->field_0x8fc[i];
                pCar->cornerAxis[i] = pCar->field_0x564[i];
                pCar->wheelSurface[i] = 0x2f;
            }
            if (pCar->wheelSurface[i] == 0xf && pCar->field_0xa7c == 0 && pCar->field_0xbf8 == 0) {
                pCar->field_0xa7c = 0x190000;
            }
            if (i < 4)
                pCar->cornerHeight[i] -= pCar->wheelOffset[i][1];
            i++;
        } while ((int)i < count);
    }
    for (i = count; i < 8; i++)
        pCar->cornerHeight[i] = pCar->cornerHeight[i - 4] - 0x50000;
    if (missing == count)
        pCar->field_0xa80 = pCar->field_0xa80 + 0x10000;
    else
        pCar->field_0xa80 = 0;
}

// Views into g_stageBlock for the object fade tables at 0x58d2d0/0x58d360/0x58d478.

// Returns the steering blend fraction through the gear/handbrake transition.
// match 83%: fade state machine branches differ from the original
// match 83%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00476850
int CarInterior_UpdateSteeringBlendFraction(int car, Car *pCar)
{
    int result = 0;

    if (g_carSteeringBlendModes[car] == 1 &&
        ((unsigned int)g_carSteeringBlendLastGears[car] != (int)pCar->requestedGear ||
         pCar->handbrake != 0)) {
        g_carSteeringBlendLastGears[car] = pCar->requestedGear;
        g_carSteeringBlendModes[car] = 2;
        g_carSteeringBlendFrames[car] = 0;
    }
    if (g_carSteeringBlendModes[car] == 2) {
        int local_c = (int)FTOI(__int64, X87S_MUL((double)(BYTE)g_carSteeringBlendFrames[car], CGraphics::m_65536));
        BYTE c;
        result = FixDiv(local_c, 0x70000);
        g_carSteeringBlendLastGears[car] = pCar->requestedGear;
        c = g_carSteeringBlendFrames[car];
        g_carSteeringBlendFrames[car] = c;
        g_carSteeringBlendFrames[car] += 1;
        if ((BYTE)(c + 1) > 7) {
            g_carSteeringBlendModes[car] = 3;
            g_carSteeringBlendFrames[car] = 0;
        }
    }
    if (g_carSteeringBlendModes[car] == 3) {
        result = 0x10000;
        g_carSteeringBlendFrames[car] = g_carSteeringBlendFrames[car] + 1;
        if ((unsigned int)g_carSteeringBlendLastGears[car] != (int)pCar->requestedGear ||
            pCar->handbrake != 0) {
            g_carSteeringBlendFrames[car] = 0;
            g_carSteeringBlendLastGears[car] = pCar->requestedGear;
        }
        if ((BYTE)g_carSteeringBlendFrames[car] > 3) {
            g_carSteeringBlendModes[car] = 4;
            g_carSteeringBlendFrames[car] = 0;
        }
    }
    if (g_carSteeringBlendModes[car] == 4) {
        int local_c = (int)FTOI(__int64, X87S_MUL((double)(BYTE)g_carSteeringBlendFrames[car], CGraphics::m_65536));
        result = 0x10000 - FixDiv(local_c, 0x70000);
        if ((unsigned int)g_carSteeringBlendLastGears[car] == (int)pCar->requestedGear &&
            pCar->handbrake == 0) {
            BYTE c = g_carSteeringBlendFrames[car];
            g_carSteeringBlendFrames[car] = c;
            g_carSteeringBlendFrames[car] += 1;
            if ((BYTE)(c + 1) > 7) {
                g_carSteeringBlendModes[car] = 1;
                g_carSteeringBlendFrames[car] = 0;
                return 0;
            }
        } else {
            BYTE c = g_carSteeringBlendFrames[car];
            g_carSteeringBlendLastGears[car] = pCar->requestedGear;
            g_carSteeringBlendModes[car] = 2;
            g_carSteeringBlendFrames[car] = 7 - c;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// TEMPORARY link scaffolding: callees that are not decompiled yet. Their
// signatures come from the original's `ret N`; the bodies are empty so the
// calls sites compile and reccmp can measure the callers.
int Scene_ApplyGroundLightToMeshTree(SceneNode *pNode, int param_2);
void Sound_NoOpMusicCallback(INT_PTR unused);

// Runs Scene_ApplyGroundLightToMeshTree on a node, then Sound_NoOpMusicCallback; returns the first result.
// FUNCTION: CMR2 0x004b5320
int StageObject_RunNodeActionAndSoundCallback(void *pNode, int value)
{
    int result;

    result = Scene_ApplyGroundLightToMeshTree((SceneNode *)pNode, value);
    Sound_NoOpMusicCallback((INT_PTR)pNode);
    return result;
}


extern Mesh *g_stageMesh1Copy;
extern Mesh *g_stageMesh3Copy;
extern short g_stageMesh1Count;
extern short g_stageMesh3Count;
extern FixVector g_stageRangeOrigin;
extern BYTE g_groundColourAlpha;
extern BYTE g_stageColourAlpha;
extern int g_stageColourMode;
extern int g_stageColourStep;
// Byte colour r, g, b, a -> 0xAARRGGBB
struct StageRGBA {
    BYTE r, g, b, a;
};
#define STAGE_ARGB(c) (((((DWORD)(c).a << 8 | (c).r) << 8 | (c).g) << 8) | (c).b)

// Colours the ground mesh: the vertex at the reference point gets the
// reference colour, every other one the plain colour.
// FUNCTION: CMR2 0x004920d0
void StageObject_ColourGroundMeshReferencePoint(DWORD *pColour, DWORD *pReference)
{
    StageRGBA colour;
    StageRGBA reference;
    int i;
    MeshVertexF *vertex;
    float f0;
    float f1;
    float f2;

    colour = *(StageRGBA *)pColour;
    reference = *(StageRGBA *)pReference;
    for (i = g_stageMesh1Count - 1; i >= 0; i--) {
        vertex = &((MeshVertexF *)g_stageMesh1Copy->pVertexData)[i];
        f0 = vertex->x;
        f1 = vertex->y;
        f2 = vertex->z;
        if (g_stageRangeOrigin.x == (int)FTOI(__int64, X87S_MUL(f0, CGraphics::m_65536)) &&
            g_stageRangeOrigin.y == (int)FTOI(__int64, X87S_MUL(f1, CGraphics::m_65536)) &&
            g_stageRangeOrigin.z == (int)FTOI(__int64, X87S_MUL(f2, CGraphics::m_65536)))
            vertex->diffuseColour = STAGE_ARGB(reference);
        else
            vertex->diffuseColour = STAGE_ARGB(colour);
        ((MeshVertexF *)g_stageMesh1Copy->pVertexData)[i].specularColour = (DWORD)g_groundColourAlpha << 24;
    }
    g_stageColourMode = 1;
}

// Colours the object meshes the same way (reference point at 0x5920fc), and
// the whole of the third mesh in the plain colour.
// FUNCTION: CMR2 0x00492220
void StageObject_ColourObjectMeshReferencePoint(DWORD *pColour, DWORD *pReference)
{
    StageRGBA colour;
    StageRGBA reference;
    int i;
    MeshVertexF *vertex;
    float f0;
    float f1;
    float f2;

    colour = *(StageRGBA *)pColour;
    reference = *(StageRGBA *)pReference;
    for (i = g_stageMesh2Count - 1; i >= 0; i--) {
        vertex = &((MeshVertexF *)g_stageMesh2Copy->pVertexData)[i];
        f0 = vertex->x;
        f1 = vertex->y;
        f2 = vertex->z;
        if (g_objectReferencePointX == (int)FTOI(__int64, X87S_MUL(f0, CGraphics::m_65536)) &&
            g_objectReferencePointY == (int)FTOI(__int64, X87S_MUL(f1, CGraphics::m_65536)) &&
            g_objectReferencePointZ == (int)FTOI(__int64, X87S_MUL(f2, CGraphics::m_65536)))
            vertex->diffuseColour = STAGE_ARGB(reference);
        else
            vertex->diffuseColour = STAGE_ARGB(colour);
        ((MeshVertexF *)g_stageMesh2Copy->pVertexData)[i].specularColour = (DWORD)g_stageColourAlpha << 24;
    }
    if (g_stageMesh3Copy != NULL) {
        for (i = g_stageMesh3Count - 1; i >= 0; i--) {
            ((MeshVertexF *)g_stageMesh3Copy->pVertexData)[i].diffuseColour = STAGE_ARGB(colour);
            ((MeshVertexF *)g_stageMesh3Copy->pVertexData)[i].specularColour = (DWORD)g_stageColourAlpha << 24;
        }
    }
    g_stageColourStep = 1;
}

extern const double g_oneOver180;  // defined in StageTiming.cpp
extern const double g_pi;

// Yaws the stage's main object to `angle` (16.16 degrees; -999 keeps it
// square) and the sun objects to `sunAngle`, by rewriting the right and
// forward axes of their matrices.
// match 33%: same operations and multiply order as the original; MSVC schedules the
// FixMatrix_SetRight pushes before the fsin there, which throws the diff alignment off.
// FUNCTION: CMR2 0x004926f0
void StageObject_YawMainAndSunNodes(int angle, int unused, int sunAngle)
{
    SceneNode *pObject;
    SceneNode *pSun;
    SceneNode *pSun2;
    FixVector right;
    FixVector forward;
    double radians;

    StageObject_GetSkyNode(&pObject);
    if (angle != -999 << 16) {
        right.y = 0;
        forward.y = 0;
        radians = X87S_MUL(X87S_MUL(X87S_MUL(angle, g_oneOver180), g_pi), CGraphics::m_oneOver65536);
        forward.z = right.x = (int)FTOI(__int64, X87S_MUL(cos(radians), CGraphics::m_65536));
        right.z = (int)FTOI(__int64, X87S_MUL(sin(radians), CGraphics::m_65536));
        forward.x = -right.z;
        FixMatrix_SetRight(&right, &pObject->current);
        FixMatrix_SetForward(&forward, &pObject->current);
    } else {
        right.x = 0x10000;
        forward.z = 0x10000;
        right.y = 0;
        right.z = 0;
        forward.x = 0;
        forward.y = 0;
        FixMatrix_SetRight(&right, &pObject->current);
        FixMatrix_SetForward(&forward, &pObject->current);
    }
    StageObject_GetCloudNodes(&pSun, &pSun2);
    right.y = 0;
    forward.y = 0;
    radians = X87S_MUL(X87S_MUL(X87S_MUL(sunAngle, g_oneOver180), g_pi), CGraphics::m_oneOver65536);
    forward.z = right.x = (int)FTOI(__int64, X87S_MUL(cos(radians), CGraphics::m_65536));
    right.z = (int)FTOI(__int64, X87S_MUL(sin(radians), CGraphics::m_65536));
    forward.x = -right.z;
    FixMatrix_SetRight(&right, &pSun->current);
    FixMatrix_SetForward(&forward, &pSun->current);
    if (pSun2 != NULL) {
        FixMatrix_SetRight(&right, &pSun2->current);
        FixMatrix_SetForward(&forward, &pSun2->current);
    }
}

extern SceneNode *g_stageLightNode;
extern SceneNode *g_stageLightRoot;

// Turns the two stage light nodes to face the view: their right, forward and
// up axes become the view's right, up and -forward, scaled to 0xbc6.
// FUNCTION: CMR2 0x00492bd0
void StageObject_OrientLightNodesToView(int view)
{
    FixMatrix *pView;
    FixVector axis;

    if (g_stageLightNode != NULL) {
        pView = &g_viewNodes[view]->current;
        FixMatrix_GetRight(&axis, pView);
        FixVecScale(&axis, &axis, 0xbc6);
        FixMatrix_SetRight(&axis, &g_stageLightNode->current);
        FixMatrix_GetUp(&axis, pView);
        FixVecScale(&axis, &axis, 0xbc6);
        FixMatrix_SetForward(&axis, &g_stageLightNode->current);
        FixMatrix_GetForward(&axis, pView);
        FixVecScale(&axis, &axis, -0xbc6);
        FixMatrix_SetUp(&axis, &g_stageLightNode->current);
    }
    if (g_stageLightRoot != NULL) {
        pView = &g_viewNodes[view]->current;
        FixMatrix_GetRight(&axis, pView);
        FixVecScale(&axis, &axis, 0xbc6);
        FixMatrix_SetRight(&axis, &g_stageLightRoot->current);
        FixMatrix_GetUp(&axis, pView);
        FixVecScale(&axis, &axis, 0xbc6);
        FixMatrix_SetForward(&axis, &g_stageLightRoot->current);
        FixMatrix_GetForward(&axis, pView);
        FixVecScale(&axis, &axis, -0xbc6);
        FixMatrix_SetUp(&axis, &g_stageLightRoot->current);
    }
}

void Car_ReloadModels(int, int, int);
void StageTiming_ResetScaledViewObjectStates(void);
void RallyData_MarkAllElementsReached(void);
extern int g_unk0x0067f228;

short Car_GetOrderCount(void);
SceneNode *SceneNode_FindByType(SceneNode *pNode, unsigned int type);
void StageObject_RebuildMirroredTiltMatrix(CameraRecord *pObj, int *pSrc);

extern BYTE g_viewObjectCarIndices[16];

// Picks the scene node of a car's object payload by the payload type letter
// ('C' or 'A' select the 9-mode node, anything else the 5-mode one) and stores
// it in both per-object tables, then copies the payload into the object.
// FUNCTION: CMR2 0x00486740
void StageObject_SelectAndCopyCarNodePayload(CameraRecord *pObj, int *pSrc, BYTE index, BYTE value)
{
    g_carPartStateTables.modes[pObj->index] = value;
    g_viewObjectCarIndices[pObj->index] = index;
    if ((short)index < Car_GetOrderCount() && Car_Get(index)->pBodyNode != NULL) {
        if (StageTiming_GetStartTableRecord(index)->detailCode == 'C' || StageTiming_GetStartTableRecord(index)->detailCode == 'A')
            g_carDetailBodyNodes[index] =
                SceneNode_FindByType(Car_Get(index)->pBodyNode, 9);
        else
            g_carDetailBodyNodes[index] =
                SceneNode_FindByType(Car_Get(index)->pBodyNode, 5);
        g_carDefaultBodyNodes[index] = SceneNode_FindByType(Car_Get(index)->pBodyNode, 5);
    }
    StageObject_RebuildMirroredTiltMatrix(pObj, pSrc);
}

// Marks a moving part hidden and refreshes the derived damage values.
// FUNCTION: CMR2 0x00469bf0
void CarPart_MarkHidden(Car *pCar, int slot)
{
    g_carPartSets[pCar->index].partHidden[slot] = 1;
    StageObject_RebuildDamagePartValues(pCar);
}

// Saves the car's torque state into pState and copies the current race record
// block into the following slot of pState.
// FUNCTION: CMR2 0x0046c320
void Replay_SaveCarTorqueAndRecordState(int *pState, BYTE car)
{
    Car *pCar = Car_Get(car);

    *pState = pCar->engineSpeed;
    Car_ReloadModels(car, 1, 1);
    pCar->engineSpeed = *pState;
    if (pCar->field_0xb48 != 1)
        Car_ResetWheelLoadsAfterShift(pCar);
    pState = pState + 1;
    pCar->automaticGearbox = 1;
    Replay_CopyBlock6((Block6 *)RallyData_GetCarRaceRecord(car), (Block6 *)pState);
    RallyData_ResetRaceRecordAndRouteProbe(car);
}

// Restores the car's torque state from pState, copies pState's race record
// block back into the car record and revalidates the stage state.
// FUNCTION: CMR2 0x0046c390
void Replay_RestoreCarTorqueAndRecordState(int *pState, BYTE car)
{
    Car *pCar = Car_Get(car);

    Car_ReloadModels(car, 1, 1);
    pCar->engineSpeed = *pState;
    if (pCar->field_0xb48 != 1)
        Car_ResetWheelLoadsAfterShift(pCar);
    pCar->automaticGearbox = 1;
    Replay_CopyBlock6((Block6 *)(pState + 1), (Block6 *)RallyData_GetCarRaceRecord(car));
    RallyData_ResetRaceRecordAndRouteProbe(car);
    StageTiming_ResetScaledViewObjectStates();
    RallyData_MarkAllElementsReached();
    RallyData_ValidateIndex(car);
}

// GLOBAL: CMR2 0x0058ca74
int g_unk0x0058ca74;
// GLOBAL: CMR2 0x0058ca68
int g_unk0x0058ca68;
// GLOBAL: CMR2 0x0058c934
int g_unk0x0058c934;
// GLOBAL: CMR2 0x0058c950
int g_unk0x0058c950;

// Saves the scene-node and render-object counts around loading the two stage
// model variants (TEMP.OBJ and TEMP.SHT).
// FUNCTION: CMR2 0x00471af0
void StageObject_LoadModelVariantsAndSaveCounts(void)
{
    g_unk0x0058ca74 = g_sceneNodeCount;
    g_unk0x0058ca68 = g_unk0x0067f228;
    StageObject_LoadTempObjectModel();
    g_unk0x0058c934 = g_sceneNodeCount;
    g_unk0x0058c950 = g_unk0x0067f228;
    StageObject_LoadTempShadowModel();
    StageObjects_LoadSkyAndGround();
}

BYTE *StageUI_GetRaceResultTable(void);
BYTE StageUI_GetRaceEndEventCount(void);
int StageUI_GetRaceResultValue(void);
int View_IsModeAvailable(BYTE index, int mode);
int RallyData_GetDriverCarSelection(BYTE index);
void View_SwitchCamera(unsigned char, int, int, unsigned char, int);

// GLOBAL: CMR2 0x0058df98
unsigned int g_unk0x0058df98;
// GLOBAL: CMR2 0x0058df9c
unsigned int g_unk0x0058df9c;
// GLOBAL: CMR2 0x0058e0a4
unsigned int g_unk0x0058e0a4;

// Driver-camera cycle: while the cycle key is held the active driver is
// advanced (or, on the championship round screen, picked from the round
// drivers) and the requested view mode is applied to the car.
// FUNCTION: CMR2 0x0047bad0
void StageObject_CycleDriverCameraSelection(unsigned int param_1, unsigned int param_2)
{
    int mode;

    if ((char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[param_2].packedState == 7 ||
        (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[param_2].packedState == 8) {
        if ((param_1 & g_carButtonMasks[8]) != 0) {
            if (View_GetActiveCameraMode(param_2) != 10)
                View_SwitchCamera(g_controlCar->index, 10, 0xffff,
                             View_GetActiveCameraFlags(g_controlCar->index), 0);
        }
        if ((param_1 & g_carButtonMasks[8]) == 0) {
            if (View_GetActiveCameraMode(g_controlCar->index) == 10) {
                switch (StageUI_GetRaceResultValue()) {
                case 4:
                    g_unk0x0058e0a4 = (StageUI_GetRaceEndEventCount() & 0xff) + param_2;
                    break;
                case 0:
                case 1:
                    g_unk0x0058e0a4 = param_2;
                    break;
                case 2:
                    RallyData_GetRoundDrivers(&g_unk0x0058df98, &g_unk0x0058df9c);
                    if (RallyData_GetUsableRecordCategory(g_unk0x0058df98 & 0xff) == -1)
                        g_unk0x0058e0a4 = g_unk0x0058df98;
                    else
                        g_unk0x0058e0a4 = g_unk0x0058df9c;
                    break;
                case 3:
                    if (param_2 == 0)
                        RallyData_GetRoundDrivers(&g_unk0x0058e0a4, &g_unk0x0058df9c);
                    else
                        RallyData_GetRoundDrivers(&g_unk0x0058df98, &g_unk0x0058e0a4);
                    break;
                }
                CGameInfo::GetConfiguredPlayerCount();
                mode = RallyData_GetDriverCarSelection(g_unk0x0058e0a4 & 0xff);
                if (View_IsModeAvailable(g_controlCar->index, mode) != 0) {
                    View_SwitchCamera(g_controlCar->index,
                                 RallyData_GetDriverCarSelection(g_unk0x0058e0a4 & 0xff), 0xffff,
                                 View_GetActiveCameraFlags(g_controlCar->index), 0);
                } else {
                    View_SwitchCamera(g_controlCar->index, 4, 0xffff,
                                 View_GetActiveCameraFlags(g_controlCar->index), 0);
                }
            }
        }
    }
}

extern Texture *g_sunGlowTexture;

// 0x547abc is g_stageLighting.intensity (the lighting block runs to 0x547ac8).
#define g_weatherLightIntensity (g_stageLighting.intensity)

// Builds a car's horizontal collision box once per frame. All callers pass
// the body axes (right followed by up), world position and eight world corners.
// When right is nearly vertical, use up as the first horizontal box axis.
// Remaining mismatch: register allocation, temporary slots and scheduling.
// FUNCTION: CMR2 0x00486c30
void Collision_BuildCarBox(CollisionBox *pBox, int *pBodyAxes, FixVector *pPosition, FixVector *pCorners)
{
    FixVector *pCorner;
    FixVector offset;
    int axisLength;
    int alongA;
    int alongB;
    int absRightY;
    int remainingCorners;

    if (pBox->built != 0)
        return;

    pBox->bottom = 0;
    pBox->top = 0;
    pBox->halfLength = 0;
    pBox->halfWidth = 0;
    absRightY = pBodyAxes[1];
    if (absRightY < 0)
        absRightY = -absRightY;
    if (absRightY > 0xfd70) {
        pBox->axisA.x = pBodyAxes[3];
        pBox->axisA.y = pBodyAxes[4];
        pBox->axisA.z = pBodyAxes[5];
        pBox->axisA.y = 0;
        axisLength = FixVecLength(&pBox->axisA);
        if (axisLength == 0) {
            pBox->axisA.x = 0;
            pBox->axisA.y = 0;
            pBox->axisA.z = 0;
        } else {
            FixVecScaleRecip(&pBox->axisA, &pBox->axisA, axisLength);
        }
    } else {
        pBox->axisA.x = pBodyAxes[0];
        pBox->axisA.y = pBodyAxes[1];
        pBox->axisA.z = pBodyAxes[2];
        pBox->axisA.y = 0;
        axisLength = FixVecLength(&pBox->axisA);
        if (axisLength == 0) {
            pBox->axisA.x = 0;
            pBox->axisA.y = 0;
            pBox->axisA.z = 0;
        } else {
            FixVecScaleRecip(&pBox->axisA, &pBox->axisA, axisLength);
        }
    }
    pBox->axisB.y = 0;
    pBox->axisB.x = pBox->axisA.z;
    pBox->axisB.z = -pBox->axisA.x;

    pCorner = pCorners;
    remainingCorners = 8;
    do {
        offset.x = pCorner->x - pPosition->x;
        offset.y = pCorner->y - pPosition->y;
        offset.z = pCorner->z - pPosition->z;
        alongA = FixVecDot(&offset, &pBox->axisA);
        alongB = FixVecDot(&offset, &pBox->axisB);
        if (alongA > 0 && alongA > pBox->halfWidth)
            pBox->halfWidth = alongA;
        if (alongB > 0 && alongB > pBox->halfLength)
            pBox->halfLength = alongB;
        if (offset.y > pBox->top)
            pBox->top = offset.y;
        if (offset.y < pBox->bottom)
            pBox->bottom = offset.y;
        pCorner++;
    } while (--remainingCorners);

    pBox->top += pPosition->y;
    pBox->bottom += pPosition->y;
    pBox->pArray = pCorners;
    pBox->pVertex = pPosition;
    pBox->built = 1;
    Collision_BuildBoxFootprint(pBox, pPosition);
}

// Text colour and layer used by the knockout screen header.
// GLOBAL: CMR2 0x0051c980
BYTE g_unk0x0051c980 = 3;
// GLOBAL: CMR2 0x0051c9a4
BYTE g_unk0x0051c9a4[4] = { 0x61, 0x61, 0x7d, 0xff };
extern BYTE g_barTextColour[4];

// Draws the two header lines of a knockout match: interpolates the panel
// rectangle, then prints both driver names with the shared bar colours.
// FUNCTION: CMR2 0x00474fe0
void Knockout_DrawAnimatedMatchHeader(int param1, int param2, int param3, int param4, int param5, KnockoutMatch *param6)
{
    short rect[4];
    int x;
    int y;

    x = (param3 - param1) * g_pauseMenus.bracketFade / 0x10000 + param1;
    rect[0] = (short)x;
    y = (param4 - param2) * g_pauseMenus.bracketFade / 0x10000 + param2;
    rect[1] = (short)y;
    rect[2] = (short)((int)g_pGraphics->resX * 0x56 / 0x280);
    rect[3] = (short)((int)g_pGraphics->resY * 0x26 / 0x1e0);
    StageObject_DrawOutlinedStageBox(rect, g_unk0x0051c9a4, g_barTextColour, 0);
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) != 0 &&
        CFrontend::IsTextureHeightSupported(0x400) != 0) {
        Font_DrawText(0, (char *)CGenericFileLoader::StrUpperPolish((BYTE *)Knockout_GetDriverNameForSide(param6, param5)),
                      (int)g_pGraphics->resX * 0x54 / 0x280 + x,
                      (int)g_pGraphics->resY * 0x10 / 0x1e0 - (int)g_pGraphics->resY * 2 / 0x1e0 + y,
                      (int *)g_barTextColour, 0x14);
        Font_DrawText(g_unk0x0051c980,
                      (char *)CGenericFileLoader::StrUpperPolish((BYTE *)StageObject_FormatAnimatedLapTime((int *)param6, param5, 0)),
                      (int)g_pGraphics->resX * 0x54 / 0x280 + x,
                      (int)g_pGraphics->resY * 0x1c / 0x1e0 + y + (int)g_pGraphics->resY * 7 / 0x1e0,
                      (int *)g_barTextColour, 0x14);
        return;
    }
    Font_DrawText(0, (char *)CGenericFileLoader::StrUpperPolish((BYTE *)Knockout_GetDriverNameForSide(param6, param5)),
                  (int)g_pGraphics->resX * 0x54 / 0x280 + x,
                  (int)g_pGraphics->resY * 0x10 / 0x1e0 + y,
                  (int *)g_barTextColour, 0x14);
    Font_DrawText(g_unk0x0051c980,
                  (char *)CGenericFileLoader::StrUpperPolish((BYTE *)StageObject_FormatAnimatedLapTime((int *)param6, param5, 0)),
                  (int)g_pGraphics->resX * 0x54 / 0x280 + x,
                  (int)g_pGraphics->resY * 0x1c / 0x1e0 + y,
                  (int *)g_barTextColour, 0x14);
}

BYTE Graphics_IsRegisteredTimerRunning(BYTE *p);

// Draws one pair of text lines at a stage-object rectangle, scaling the
// rectangle vertically when the panel is animating in.
// match 81%: the original's Font_DrawText takes 16-bit x/y (it adds them as words and
// pushes the register unextended); our Font.h declares them 32-bit, so every coordinate
// costs one movsx. Logic and constants are exact.
// FUNCTION: CMR2 0x00475430
void StageObject_DrawAnimatedTextPair(int param1, KnockoutMatch *param2, short *param3, BYTE *param4, BYTE *param5,
                  int param6, int param7, int *param8, int param9)
{
    short rect[4];

    if (Graphics_IsRegisteredTimerRunning(&g_unk0x0058ca80) != 0 || param9 != 0) {
        rect[0] = param3[0];
        rect[1] = param3[1];
        rect[2] = (short)(FixMul(param3[2] << 16, g_pauseMenus.bracketFade) >> 16);
        rect[3] = (short)(FixMul(param3[3] << 16, g_pauseMenus.bracketFade) >> 16);
        StageObject_DrawOutlinedStageBox(rect, param4, param5, param6);
        return;
    }
    rect[0] = param3[0];
    rect[1] = param3[1];
    rect[2] = param3[2];
    rect[3] = param3[3];
    StageObject_DrawOutlinedStageBox(rect, param4, param5, param6);
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) != 0 &&
        CFrontend::IsTextureHeightSupported(0x400) != 0) {
        Font_DrawText(0, (char *)CGenericFileLoader::StrUpperPolish((BYTE *)Knockout_GetDriverNameForSide(param2, param1)),
                      (short)((short)((int)g_pGraphics->resX * 0x54 / 0x280) + param3[0]),
                      (short)((short)((int)g_pGraphics->resY * 0x10 / 0x1e0 - (int)g_pGraphics->resY * 2 / 0x1e0) + param3[1]),
                      param8, 0x14);
        Font_DrawText(g_unk0x0051c980,
                      (char *)CGenericFileLoader::StrUpperPolish((BYTE *)StageObject_FormatAnimatedLapTime((int *)param2, param1, param7)),
                      (short)((short)((int)g_pGraphics->resX * 0x54 / 0x280) + param3[0]),
                      (short)((short)((int)g_pGraphics->resY * 7 / 0x1e0 + (int)g_pGraphics->resY * 0x1c / 0x1e0) + param3[1]),
                      param8, 0x14);
        return;
    }
    Font_DrawText(0, (char *)CGenericFileLoader::StrUpperPolish((BYTE *)Knockout_GetDriverNameForSide(param2, param1)),
                  (short)((short)((int)g_pGraphics->resX * 0x54 / 0x280) + param3[0]),
                  (short)((short)((int)g_pGraphics->resY * 0x10 / 0x1e0) + param3[1]),
                  param8, 0x14);
    Font_DrawText(g_unk0x0051c980,
                  (char *)CGenericFileLoader::StrUpperPolish((BYTE *)StageObject_FormatAnimatedLapTime((int *)param2, param1, param7)),
                  (short)((short)((int)g_pGraphics->resX * 0x54 / 0x280) + param3[0]),
                  (short)((short)((int)g_pGraphics->resY * 0x1c / 0x1e0) + param3[1]),
                  param8, 0x14);
}

BYTE *StageUI_GetRaceResultTable(void);

// Steps every live replay object: advances the frame counter of the current
// record and appends the next one, rebuilding the lookup row when the current
// frame is exhausted.
// FUNCTION: CMR2 0x0046c8e0
void Replay_AdvanceLiveRecordingStreams(void)
{
    int i;
    ReplayStream *p;
    short *pIndex;
    BYTE *pFrame;
    BYTE *pInfo;
    ReplayInputLane *pInput;
    ReplayStateLane *pState;
    ReplayEvent *pEvent;
    int flag;
    int valid;
    BYTE value;

    for (i = 0; i < 16; i++) {
        if (g_replaySlots[i] == NULL)
            continue;
        p = *g_replaySlots[i];
        if (p == NULL || p->recording == 0 || p->type == 2)
            continue;
        if (Car_Get(p->car)->simulationStepsRemaining <= 0u)
            continue;
        valid = 0;
        flag = 0;
        if (p->recordStarted != 0)
            valid = 1;
        else
            flag = 1;
        pIndex = &p->pLaneSamples[p->laneCount];
        if (valid != 0 && p->recordStarted != 0) {
            pFrame = p->pFrames + (p->samplesPerLane * p->laneCount + *pIndex) * 4;
            if (Replay_CanExtendControlPacketRun(pFrame, p->car) == 0) {
                (*pIndex)++;
                if (*pIndex == p->samplesPerLane) {
                    p->recordStarted = 0;
                    (*pIndex)++;
                    p->laneCount++;
                    if (p->laneCount == p->laneCapacity)
                        Replay_StopRecording((BYTE *)p);
                    goto done;
                }
                pFrame = p->pFrames + (p->samplesPerLane * p->laneCount + *pIndex) * 4;
                pFrame[2] &= 0xc0;
            }
            Replay_EncodeCarControls(pFrame, p->car);
            pInfo = StageUI_GetRaceResultTable();
            if (CGameInfo::GetGameModeOptionBit19())
                value = (char)((CallbackStateMachine *)pInfo)->records[0].packedState;
            else
                value = (char)((CallbackStateMachine *)pInfo)->records[p->car].packedState;
            if (p->type == 0) {
                pInput = p->pInputs + p->laneCount;
                if (value != pInput->events[pInput->eventCount - 1].value) {
                    pEvent = &pInput->events[pInput->eventCount];
                    pEvent->value = value;
                    pEvent->frame = *pIndex;
                    pEvent->count = pFrame[2] & 0x3f;
                    pInput->eventCount++;
                    if (pEvent->count == 0) {
                        pEvent->frame--;
                        pEvent->count = p->pFrames[(p->samplesPerLane * p->laneCount + *pIndex) * 4 - 2] & 0x3f;
                    } else {
                        pEvent->count--;
                    }
                }
            } else {
                pState = p->pStates + p->laneCount;
                if (value != pState->events[pState->eventCount - 1].value) {
                    pEvent = &pState->events[pState->eventCount];
                    pEvent->value = value;
                    pEvent->frame = *pIndex;
                    pEvent->count = pFrame[2] & 0x3f;
                    pState->eventCount++;
                    if (pEvent->count == 0) {
                        pEvent->frame--;
                        pEvent->count = p->pFrames[(p->samplesPerLane * p->laneCount + *pIndex) * 4 - 2] & 0x3f;
                    } else {
                        pEvent->count--;
                    }
                }
            }
        }
done:
        if (flag != 0) {
            pFrame = p->pFrames + p->laneCount * p->samplesPerLane * 4;
            p->pLaneSamples[p->laneCount] = 0;
            pFrame[2] &= 0xc0;
            p->field_0x18 = 1;
            if (p->type == 0)
                p->pInputRecLane = p->pInputs + p->laneCount;
            else
                p->pStateRecLane = p->pStates + p->laneCount;
            p->recordStarted = 1;
        }
    }
}

// Draws the stage icon of one view node: projects its world position to the
// screen, blends the object ramp into the stage colour and queues the sprite.
// FUNCTION: CMR2 0x00462d80
void StageObject_DrawProjectedViewIcon(int *pViewRect, int param_2)
{
    SpriteRect uv;
    SpriteRect dst;
    SpriteRect icon;
    FixVector vR;
    FixVector vP;
    FixVector vOut;
    BYTE colour[4];
    SceneNode *view;
    int ramp;
    int value;

    int diff;
    int scale;

    icon.w = 0x10;
    icon.h = 0x10;
    icon.x = 0;
    icon.y = 0;
    Gfx_SetSunScreenPosition(0,0,FALSE);
    Gfx_SetFogLights(NULL, 0);
    if (param_2 == 0)
        SnowMaterial_UpdateFrame();   // video.snow_material: once per frame, view 0
    if (g_sunGlowTexture == 0)
        return;
    uv.x = g_sunGlowTexture->field_0x11c;
    uv.y = g_sunGlowTexture->field_0x11e;
    uv.w = g_sunGlowTexture->width;
    uv.h = g_sunGlowTexture->height;
    view = g_viewNodes[param_2];
    // The fog pass's car lights (video.fog_lights): game/FogBeamProjection.h.
    {   GfxFogLight fogLights[LIGHT_FIELD_MAX];
        int fogLightTotal = FogBeam_ProjectLights(fogLights, LIGHT_FIELD_MAX, view, (float)g_pGraphics->resX, (float)g_pGraphics->resY,
                                          (float)CGraphics::m_projection33, (float)CGraphics::m_projection43);
        Gfx_SetFogLights(fogLights, fogLightTotal);
    }
    // World-anchored fog banks (video.fog_spatial, render/fog_camera.h): camera position and horizontal axes in world units, so the
    // shader can pin density banks to the world instead of the screen. Off (strength 0) unless the spatial option is on.
    {   float strength = Sys_GetOption("video.fog_spatial", 0) ? 1.0f : 0.0f;
        const float p11 = (float)CGraphics::m_projection11;
        FixVector o = {0, 0, 0}, fz = {0, 0, 0x10000}, rx = {0x10000, 0, 0}, wo, wf, wr;
        FixMatrix_TransformPoint(&wo, &o, &view->world);
        FixMatrix_TransformPoint(&wf, &fz, &view->world);
        FixMatrix_TransformPoint(&wr, &rx, &view->world);
        const float k = 1.0f / 65536.0f;
        Gfx_SetFogCamera(wo.x * k, wo.z * k, (wf.x - wo.x) * k, (wf.z - wo.z) * k, (wr.x - wo.x) * k, (wr.z - wo.z) * k,
                         p11 > 0.0f ? 1.0f / p11 : 0.0f, p11 > 0.0f ? strength : 0.0f);
    }
    StageObject_GetRotatedStageLightVector(&vR);
    StageObject_GetCurrentObjectPosition(&vP);
    vR.x += vP.x;
    vR.y += vP.y;
    vR.z += vP.z;
    FixMatrix_ProjectWorldPointToView((int *)&vOut, &vR, view);
    vOut.x >>= 16;
    vOut.y >>= 16;
    Gfx_SetSunScreenPosition(vOut.x/(float)g_pGraphics->resX,vOut.y/(float)g_pGraphics->resY,
        vOut.x>=0 && vOut.y>=0 && vOut.x<g_pGraphics->resX && vOut.y<g_pGraphics->resY);
    icon.x = (short)(vOut.x - icon.w / 2);
    icon.y = (short)(vOut.y - icon.h / 2);
    StageObject_UpdateSunVisibility((short *)&icon);
    ramp = FixMul(-0x20000, g_weatherLightIntensity) + 0x20000;
    if (ramp < 0)
        ramp = 0;
    else if (ramp > 0x10000)
        ramp = 0x10000;
    value = FixMul((int)g_sunVisibility << 16, 0x28f);
    diff = value - g_viewWeather[param_2].sunVisibility;
    if ((diff < 0 ? -diff : diff) <= FixMul(0x4ccc, g_stageObjectTimeStep)) {
        g_viewWeather[param_2].sunVisibility = value;
    } else {
        if (diff > 0)
            g_viewWeather[param_2].sunVisibility += FixMul(0x4ccc, g_stageObjectTimeStep);
        else
            g_viewWeather[param_2].sunVisibility -= FixMul(0x4ccc, g_stageObjectTimeStep);
    }
    ramp += g_viewWeather[param_2].sunVisibility;
    if (ramp > 0x10000)
        ramp = 0x10000;
    scale = FixMul(0x10000 - ramp, 0x30000);
    if (scale > 0x10000)
        scale = 0x10000;
    colour[0] = (BYTE)(FixMul(g_sunIconColour[0] << 16, scale) >> 16);
    colour[1] = (BYTE)(FixMul(g_sunIconColour[1] << 16, scale) >> 16);
    colour[2] = (BYTE)(FixMul(g_sunIconColour[2] << 16, scale) >> 16);
    colour[3] = 0xff;
    dst.w = (short)(FixMul(g_sunIconWidthScale, *(int *)g_pGraphics << 16) >> 16);
    dst.h = (short)(FixMul(g_sunIconHeightScale, *((int *)g_pGraphics + 1) << 16) >> 16);
    dst.x = (short)(vOut.x - dst.w / 2);
    dst.y = (short)(vOut.y - dst.h / 2);
    Sprite_Queue(&uv, &dst, g_sunGlowTexture, 2, 0, 0, 0, colour, 8);
}


int RallyData_IsElementFlagSet(SectorObjectEntry **pEntry, int bit);
void RallyData_SetElementFlagBit(SectorObjectEntry **pp, int bit, int set);
void RallyData_MarkElementReachedByCar(SectorObjectEntry *pElement, int car);

// Queues a moving stage object: appends it to the free table, or (when the
// table is full) evicts the entry whose objects are farthest from the cars.
// FUNCTION: CMR2 0x0046fe70
void StageObject_QueueOrEvictMovingObject(SectorObjectEntry *param_1, int param_2, int param_3)
{
    MovingObject *entry;
    FixVector pos;
    FixVector delta;
    Car *pCar;
    int d;
    int v;
    int best;
    BYTE bestCount;
    BYTE bestIx;
    int j;
    int c;

    if (RallyData_IsElementFlagSet(&param_1, param_3) != 0) {
        RallyData_SetElementFlagBit(&param_1, param_3, 0);
        pCar = Car_Get(param_3);
        if (pCar->isGhost == 0 && (char)RallyDataState() == 1) {
            param_1->pObject->position.y += -0x3e80000;
            param_1->pObject->visible = 0;
        }
    }
    pCar = Car_Get(param_3);
    if (pCar->isGhost != 0)
        return;
    if (g_movingObjects.count < 0x28) {
        g_movingObjects.entries[g_movingObjects.count].pEntry = param_1;
        g_movingObjects.entries[g_movingObjects.count].sectorIndex = param_2;
        StageObject_InitMovingObject(&g_movingObjects.entries[g_movingObjects.count], param_3);
        g_movingObjects.count++;
        return;
    }
    c = 0;
    if (g_movingObjects.count > 0) {
        entry = &g_movingObjects.entries[0];
        do {
            RallyData_CopyRaisedElementVector((int *)&pos, &entry->pEntry);
            for (j = 0; j < Car_GetOrderCount(); j++) {
                delta.x = Car_Get(j)->position.x - pos.x;
                delta.y = Car_Get(j)->position.y - pos.y;
                delta.z = Car_Get(j)->position.z - pos.z;
                d = (FIX_ABS(delta.x) > FIX_ABS(delta.z)) ? FIX_ABS(delta.x) : FIX_ABS(delta.z);
                if (d > 0x28f) {
                    FixVecScaleRecip(&delta, &delta, d);
                    delta.y = 0;
                    v = FixVecLength(&delta);
                    g_movingObjects.carDistance[j + c * 8] = v;
                    g_movingObjects.carDistance[j + c * 8] = FixMul(v, d);
                } else {
                    g_movingObjects.carDistance[j + c * 8] = 0;
                }
            }
            g_farVotes[c] = 0;
            c++;
            entry++;
        } while (c < g_movingObjects.count);
    }
    for (j = 0; j < Car_GetOrderCount(); j++) {
        best = g_movingObjects.carDistance[j];
        bestIx = 0;
        for (c = 1; c < g_movingObjects.count; c++) {
            if (g_movingObjects.carDistance[j + c * 8] > best) {
                best = g_movingObjects.carDistance[j + c * 8];
                bestIx = c;
            }
        }
        g_farVotes[bestIx]++;
    }
    bestCount = g_farVotes[0];
    bestIx = 0;
    for (c = 1; c < g_movingObjects.count; c++) {
        if (g_farVotes[c] > bestCount) {
            bestCount = g_farVotes[c];
            bestIx = c;
        }
    }
    entry = &g_movingObjects.entries[bestIx];
    for (j = 0; j < 8; j++)
        RallyData_MarkElementReachedByCar(entry->pEntry, j);
    entry->pEntry = param_1;
    entry->sectorIndex = param_2;
    StageObject_InitMovingObject(entry, param_3);
}

extern DeformVertex ***g_carOriginalPartVertices;
extern BYTE **g_carOriginalPartNodeTypes;

// Builds the vertex buffer of one stage object for one car: converts the
// float source vertices to 16.16 fixed point and packs the normal bytes.
// FUNCTION: CMR2 0x0046afe0
void CarDamage_BuildPartVertexBuffer(int param_1, SceneNode *param_2, CarPartSet *set)
{
    int i;
    int j;
    int total;
    int t;
    int len;
    BYTE rgb[3];
    FixVector v;
    DeformVertex *pDst;
    FixVector *pPos;
    FixVector p;

    g_carOriginalPartVertices[param_1] = (DeformVertex **)CFileBuffer::AllocateLockedBuffer(set->count * sizeof(DeformVertex *));
    total = set->count * 4;
    g_carOriginalPartNodeTypes[param_1] = (BYTE *)CFileBuffer::AllocateLockedBuffer(set->count);
    for (i = 0; i < set->count; i++) {
        g_carOriginalPartVertices[param_1][i] =
            (DeformVertex *)CFileBuffer::AllocateLockedBuffer(set->vertexCount[i] << 5);
        total += set->vertexCount[i] << 5;
        g_carOriginalPartNodeTypes[param_1][i] = (BYTE)set->geometry.nodes[i]->flags;
        for (j = 0; j < set->vertexCount[i]; j++) {
            pDst = &g_carOriginalPartVertices[param_1][i][j];
            pPos = &pDst->pos;
            pDst->pos.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[0], CGraphics::m_65536));
            pDst->pos.y = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[1], CGraphics::m_65536));
            pDst->pos.z = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[2], CGraphics::m_65536));
            pDst->normal.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].normal[0], CGraphics::m_65536));
            pDst->normal.y = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].normal[1], CGraphics::m_65536));
            pDst->normal.z = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].normal[2], CGraphics::m_65536));
            p = *pPos;
            if (p.x >= 0)
                pDst->rawNormal[0] = 0x81;
            else
                pDst->rawNormal[0] = 0x7f;
            if (p.y >= 0)
                pDst->rawNormal[1] = 0x81;
            else
                pDst->rawNormal[1] = 0x7f;
            if (p.z >= 0)
                pDst->rawNormal[2] = 0x81;
            else
                pDst->rawNormal[2] = 0x7f;
            rgb[0] = (BYTE)(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].colour >> 16);
            rgb[1] = (BYTE)(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].colour >> 8);
            rgb[2] = (BYTE)((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].colour;
            t = rgb[0] - 0x80;
            if (t < -0x7f)
                t = -0x7f;
            else if (t > 0x7f)
                t = 0x7f;
            pDst->rawNormal[0] = (signed char)t;
            t = rgb[1] - 0x80;
            if (t < -0x7f)
                t = -0x7f;
            else if (t > 0x7f)
                t = 0x7f;
            pDst->rawNormal[1] = (signed char)t;
            t = rgb[2] - 0x80;
            if (t < -0x7f)
                t = -0x7f;
            else if (t > 0x7f)
                t = 0x7f;
            pDst->rawNormal[2] = (signed char)t;
            v.x = -pDst->rawNormal[0] << 9;
            v.y = -pDst->rawNormal[1] << 9;
            v.z = -pDst->rawNormal[2] << 9;
            len = FixVecLength(&v);
            if (len == 0) {
                pDst->normal8[0] = 0;
                pDst->normal8[1] = 0;
                pDst->normal8[2] = 0;
            } else {
                FixVecScaleRecip(&v, &v, len);
                t = v.x >> 9;
                if (t > 0x7f)
                    t = 0x7f;
                else if (t < -0x7f)
                    t = -0x7f;
                pDst->normal8[0] = (signed char)t;
                t = v.y >> 9;
                if (t > 0x7f)
                    t = 0x7f;
                else if (t < -0x7f)
                    t = -0x7f;
                pDst->normal8[1] = (signed char)t;
                t = v.z >> 9;
                if (t > 0x7f)
                    t = 0x7f;
                else if (t < -0x7f)
                    t = -0x7f;
                pDst->normal8[2] = (signed char)t;
            }
        }
    }
}

int View_FindFreeModeSlot(unsigned char view);
void RallyData_SetDriverCarSelection(BYTE index, int value);

// Applies the driver-camera cycle: while the cycle key is held it picks the
// target driver (either from the current knockout round or by advancing the
// active one) and hands the resulting view mode to the rally data layer.
// match 53%: implementada, MSVC6 cachea param_1 en EDI y coloca distinta la tabla del switch
// FUNCTION: CMR2 0x0047b970
void View_UpdateDriverCameraCycle(unsigned int param_1)
{
    unsigned int uVar4;
    unsigned int uVar5;
    BYTE *p;

    p = StageUI_GetRaceResultTable();
    if ((char)((CallbackStateMachine *)p)->records[0].packedState == 10) {
        if (Race_IsMultiplayerRecordMode10() != 0) {
            if (CGameInfo::GetConfiguredGameMode() != 2)
                uVar4 = View_FindFreeModeSlot(1);
            else
                uVar4 = param_1;
        } else {
            uVar4 = View_FindFreeModeSlot(0);
        }
    } else {
        uVar4 = View_FindFreeModeSlot((BYTE)g_controlCar->index);
    }

    p = StageUI_GetRaceResultTable();
    if (((char)((CallbackStateMachine *)p)->records[0].packedState == 8) || (p = StageUI_GetRaceResultTable(), (char)((CallbackStateMachine *)p)->records[0].packedState == 7)) {
        uVar5 = StageUI_GetRaceResultValue();
        switch (uVar5) {
        case 4:
            g_unk0x0058e0a4 = (StageUI_GetRaceEndEventCount() & 0xff) + param_1;
            break;
        case 0:
        case 1:
            g_unk0x0058e0a4 = param_1;
            break;
        case 2:
            RallyData_GetRoundDrivers(&g_unk0x0058df98, &g_unk0x0058df9c);
            if (RallyData_GetUsableRecordCategory(g_unk0x0058df98 & 0xff) == -1)
                g_unk0x0058e0a4 = g_unk0x0058df98;
            else
                g_unk0x0058e0a4 = g_unk0x0058df9c;
            break;
        case 3:
            if (param_1 == 0)
                RallyData_GetRoundDrivers(&g_unk0x0058e0a4, &g_unk0x0058df9c);
            else
                RallyData_GetRoundDrivers(&g_unk0x0058df98, &g_unk0x0058e0a4);
            break;
        }
        if ((int)g_unk0x0058e0a4 < (int)(unsigned int)CGameInfo::GetConfiguredPlayerCount() &&
            (uVar4 == 1 || uVar4 == 2 || uVar4 == 3 || uVar4 == 5 || uVar4 == 4)) {
            RallyData_SetDriverCarSelection((BYTE)g_unk0x0058e0a4, uVar4);
        }
    }
}

// Packs the local bounds of eight rotated collision corners. The loader
// supplies right/up/forward as a three-vector array; use up when right is
// nearly vertical, then project each point onto the horizontal basis.
// Heights and half extents are stored in signed 16-bit units of 512.
// FUNCTION: CMR2 0x004873f0
void Collision_BuildObjectPointBounds(FixVector *pBasis, FixVector *pCorners, SectorObjectBounds *pBounds)
{
    ObjectPointBounds *pProjection = pBounds->pProjection;
    FixVector axisA;
    FixVector axisB;
    FixVector point;
    int halfLength = 0;
    int halfWidth = 0;
    int absRightY;
    int alongA;
    int alongB;
    int axisLength;
    FixVector *pCorner;
    int remainingCorners;

    pProjection->bottom = 0;
    pProjection->top = 0;

    absRightY = pBasis[0].y;
    if (absRightY < 0)
        absRightY = -absRightY;

    if (absRightY > 0xfd70) {
        axisA = pBasis[1];
        axisA.y = 0;
        axisLength = FixVecLength(&axisA);
        if (axisLength == 0) {
            axisA.x = 0;
            axisA.y = 0;
            axisA.z = 0;
        } else {
            FixVecScaleRecip(&axisA, &axisA, axisLength);
        }
    } else {
        axisA = pBasis[0];
        axisA.y = 0;
        axisLength = FixVecLength(&axisA);
        if (axisLength == 0) {
            axisA.x = 0;
            axisA.y = 0;
            axisA.z = 0;
        } else {
            FixVecScaleRecip(&axisA, &axisA, axisLength);
        }
    }
    axisB.x = axisA.z;
    axisB.y = 0;
    axisB.z = -axisA.x;

    pCorner = pCorners;
    remainingCorners = 8;
    do {
        point.x = pCorner->x;
        point.y = pCorner->y;
        point.z = pCorner->z;
        alongA = FixVecDot(&point, &axisA);
        alongB = FixVecDot(&axisB, &point);
        if (alongA > 0 && alongA > halfWidth)
            halfWidth = alongA;
        if (alongB > 0 && alongB > halfLength)
            halfLength = alongB;
        if (point.y > pProjection->top * 0x200)
            pProjection->top = (short)(point.y >> 9);
        if (point.y < pProjection->bottom * 0x200)
            pProjection->bottom = (short)(point.y >> 9);
        pCorner++;
    } while (--remainingCorners != 0);

    pProjection->directionX = (short)(axisA.x >> 9);
    pProjection->directionZ = (short)(axisA.z >> 9);
    pProjection->halfWidth = (short)(halfWidth >> 9);
    pProjection->halfLength = (short)(halfLength >> 9);
}

extern double g_minus65536;

// Chooses the wall-collision response of a car part from its lateral/forward
// offsets and the wall distances stored in the reference record; returns the
// facing side (1..4) or a slanted-response code (5/6), 0 when clear.
// FUNCTION: CMR2 0x0047c5e0
unsigned int AI_SelectWallCollisionResponse(int *pState)
{
    int side;
    int lateral;
    int nearGap;
    int farGap;
    int gap;
    int forward;
    int index;
    char farLimit;

#define WALL_DISTANCE(i) ((int)FTOI(__int64, X87S_MUL((double)(int)g_aiRouteNodes[index].wall[(i) - 8], g_minus65536)))

    side = 0;
    if (pState[0x17] < -0xa0000)
        side = 1;
    if (pState[0x17] > 0xa0000)
        side = 2;

    lateral = pState[0xd];
    if (lateral < 0) {
        index = pState[0x15];
        nearGap = lateral - WALL_DISTANCE(0xd);
        farLimit = g_aiRouteNodes[index].wall[0xe - 8];
        farGap = lateral - WALL_DISTANCE(0xe);
        if (farGap < 0x40000 && farLimit <= 0x10)
            gap = farGap;
        else
            gap = 0;
        if (nearGap > 0 && nearGap < 0x40000)
            gap = nearGap;
        if (gap > 0) {
            if (pState[0xe] > 0x5a0000)
                return 1;
            if (pState[0xe] - FixMul(0xf0000, -gap) + 0x3c0000 < 0)
                return 2;
        }
        if (nearGap < 0) {
            forward = pState[0xe];
            if (forward > 0) {
                if (forward > 0x6e0000)
                    return 1;
                return forward < 0x460000 ? 3 : 5;
            }
            if (forward < -0x6e0000)
                return 2;
            return forward > -0x460000 ? 4 : 6;
        }
        gap = pState[0xd] - WALL_DISTANCE(0xc);
        if (gap < 0 && side != 2) {
            if (pState[0xe] > 0x3c0000)
                return 1;
            if (pState[0xe] < -0x2d0000)
                return 2;
            if (pState[0xe] - FixMul(0x50000, -gap) < 0)
                return 3;
        }
    } else {
        index = pState[0x15];
        lateral = -lateral;
        nearGap = lateral - WALL_DISTANCE(9);
        farLimit = g_aiRouteNodes[index].wall[10 - 8];
        farGap = lateral - WALL_DISTANCE(10);
        gap = 0;
        if (farGap < 0x40000 && farLimit <= 0x10)
            gap = farGap;
        if (nearGap > 0 && nearGap < 0x40000)
            gap = nearGap;
        if (gap > 0) {
            if (pState[0xe] < -0x5a0000)
                return 3;
            if (pState[0xe] - FixMul(0xf0000, gap) - 0x3c0000 > 0)
                return 4;
        }
        if (nearGap < 0) {
            forward = pState[0xe];
            if (forward > 0) {
                if (forward > 0x6e0000)
                    return 4;
                return forward < 0x460000 ? 2 : 6;
            }
            if (forward < -0x6e0000)
                return 3;
            return forward > -0x460000 ? 1 : 5;
        }
        gap = -pState[0xd] - WALL_DISTANCE(8);
        if (gap < 0 && side != 1) {
            if (pState[0xe] > 0x2d0000)
                return 4;
            if (pState[0xe] < -0x3c0000)
                return 3;
            if (pState[0xe] - FixMul(0x50000, gap) > 0)
                return 1;
        }
    }
    if (pState[0xe] > 0x780000)
        return 4;
    return pState[0xe] < -0x780000 ? 2 : 0;

#undef WALL_DISTANCE
}

int RallyData_GetRouteAvailabilityState(void);
int RallyData_GetActiveCarRaceRecordField0(Car *p);
void RallyData_GetRouteNodeGroundPosition(int index, int *pOut);
extern double g_fixedDegreesToAngle12;

// Picks the closest car ahead of the reference angle among the active cars,
// rejecting those out of range or outside the angular window, and writes the
// chosen one's relative state code to *param_2.
// FUNCTION: CMR2 0x0047cd10
int AI_FindClosestCarInAngleWindow(int param_1, int *param_2, int param_3, int *param_4)
{
    Car *cars[6];
    int flags[6];
    int maxAhead[6];
    int routePos[6];
    int i;
    int wrap;
    int base;
    int refX;
    int refZ;
    int node[3];
    int dx;
    int dz;
    int length;
    int limit;
    int angle;
    int carHeading;
    int nodeHeading;
    int ahead;

    *param_2 = 0;
    memset(flags, 0, sizeof(flags));
    for (i = 0; i < 6; i++)
        cars[i] = Car_Get(i);
    wrap = RallyData_GetRouteAvailabilityState();
    refX = cars[param_1]->position.z;
    refZ = cars[param_1]->position.x;
    base = param_4[4];

    for (i = 0; i < (signed char)g_unk0x0058e0b0[0]; i++) {
        if (i != param_1 && cars[i]->speed < 0xc800)
            flags[i] = 1;
    }
    for (i = 0; i < (signed char)g_unk0x0058e0b0[0]; i++) {
        if (flags[i] != 0) {
            angle = StageObject_WrapFixedDegreeAngle(
                base - StageObject_Atan2Degrees(cars[i]->right.z, cars[i]->right.x));
            if (angle >= -0x5a0000 && angle <= 0x5a0000)
                maxAhead[i] = 5;
            else
                maxAhead[i] = 10;
        }
    }
    for (i = 0; i < (signed char)g_unk0x0058e0b0[0]; i++) {
        if (flags[i] != 0) {
            routePos[i] = RallyData_GetActiveCarRaceRecordField0(cars[i]);
            ahead = routePos[i] - param_3;
            if (ahead < -100)
                ahead += wrap;
            if (ahead < 0 || ahead > maxAhead[i])
                flags[i] = 0;
        }
    }
    for (param_1 = 0; param_1 < (signed char)g_unk0x0058e0b0[0]; param_1++) {
        if (flags[param_1] == 0)
            continue;
        dx = cars[param_1]->position.z - refX;
        dz = cars[param_1]->position.x - refZ;
        length = FixSqrt(FixMul(dx, dx) + FixMul(dz, dz));
        carHeading = StageObject_Atan2Degrees(dx, dz);
        angle = StageObject_WrapFixedDegreeAngle(base - carHeading);
        if (angle > 0x2d0000 || angle < -0x2d0000) {
            flags[param_1] = 0;
            continue;
        }
        limit = FixDiv(length - 0x70000, 0x70000) + 0x20000;
        if (limit > 0x50000)
            limit = 0x50000;
        else if (limit < 0)
            limit = 0;
        length = FixMul(g_sinTable[(short)FTOI(__int64, X87S_MUL((double)angle, g_fixedDegreesToAngle12)) & 0xfff], length);
        i = param_3 + 5;
        if (i - wrap >= 0)
            i -= wrap;
        RallyData_GetRouteNodeGroundPosition(i, node);
        nodeHeading = StageObject_Atan2Degrees(node[2] - refX, node[0] - refZ);
        StageObject_WrapFixedDegreeAngle(base - nodeHeading);
        if (length > limit || length < -limit) {
            flags[param_1] = 0;
            continue;
        }
        if (StageObject_WrapFixedDegreeAngle(carHeading - nodeHeading) >= 0) {
            if (*param_4 > 0x320000)
                flags[param_1] = 3;
            else
                flags[param_1] = 1;
        } else {
            if (*param_4 > 0x320000)
                flags[param_1] = 4;
            else
                flags[param_1] = 2;
        }
        *param_2 = flags[param_1];
        return param_1;
    }
    return -1;
}

// Initialises the per-car stage-object record for one lane: validates it, stores
// the car index, zeroes the timers and dispatches to the type-specific reset
// (object list, light list or mesh list) resetting the light buffer too.
// FUNCTION: CMR2 0x0046c750
int Replay_InitCarStreamState(ReplayStream *p, int unused, BYTE car)
{
    BYTE *pInfo;

    if (p == NULL || p->recording != 0 || p->playing != 0)
        return 0;
    p->steeringModeOption = 0;
    if (car < 2)
        p->steeringModeOption = CInput::GetControllerSmoothSteering(car);
    p->car = car;
    p->recording = 1;
    p->laneCount = 0;
    p->recordStarted = 1;
    p->pLaneSamples[0] = 0;
    if (p->type != 2)
        p->pFrames[2] &= 0xc0;
    if (p->type == 0) {
        pInfo = StageUI_GetRaceResultTable();
        p->pInputs[0].events[0].value = (BYTE)((unsigned int)((CallbackStateMachine *)pInfo)->records[p->car].packedState >> 16);
        p->pInputs[0].events[0].frame = 0;
        p->pInputs[0].events[0].count = 0;
        p->pInputs[0].eventCount = 1;
    } else {
        pInfo = StageUI_GetRaceResultTable();
        if (CGameInfo::GetGameModeOptionBit19())
            p->pStates[0].events[0].value = (BYTE)((unsigned int)((CallbackStateMachine *)pInfo)->records[0].packedState >> 16);
        else
            p->pStates[0].events[0].value = (BYTE)((unsigned int)((CallbackStateMachine *)pInfo)->records[p->car].packedState >> 16);
        p->pStates[0].events[0].frame = 0;
        p->pStates[0].events[0].count = 0;
        p->pStates[0].eventCount = 1;
    }
    if (p->type == 0)
        Replay_CaptureCarSnapshot(p->pInputs, car);
    else if (p->type == 1)
        Replay_SaveCarTorqueAndRecordState((int *)p->pStates, car);
    else if ((CGameInfo::GetGameModeOptionBit19() && p->car == 0) || !CGameInfo::GetGameModeOptionBit19())
        Car_Get(p->car)->field_0xc18 = 0;
    p->field_0x18 = 0;
    p->step = 0;
    return 1;
}

// Views into the stage object block declared in StageBlock.h, used by the
// stage object pose code.
// Per-object pose record of the stage object pass (0x24 bytes).

// GLOBAL: CMR2 0x0058d2f8  (per object: 3 int, animated pose offset)
// GLOBAL: CMR2 0x0058d368  (per object: 0x24-byte timing record, angle at +0)
// GLOBAL: CMR2 0x0058d374  (per object: int pose offset, +4/+8 are y/z)
#define g_carDriverPoseOffsetView (&g_carDriverPoseStates[0].offset)
// GLOBAL: CMR2 0x0058d560  (scratch 4x4 16.16 matrix, 16 int = 0x40 bytes)

// Integrates one stage object's pose: rebuilds the object matrix from the car
// basis, low-pass filters the object's angles against the car's body axes and
// moves the object's scene node.
// FUNCTION: CMR2 0x00476e00
void CarInterior_IntegrateDriverPose(CameraRecord *param_1, FixMatrix *param_2, int unused)
{
#define POSE (g_carDriverPoseStates[param_1->car])
    Car *pCar;
    FixVector acc;
    FixVector off;
    FixVector pos;
    FixAngles target;
    int right;

    target.pad = 0;
    pCar = Car_Get(param_1->car);

    // Object matrix: rows 0 and 2 swapped (the old row 2 negated), translation
    // cleared, then rotated about the object's right axis.
    g_carDriverPoseMatrix = *param_2;
    g_carDriverPoseMatrix.right.x = -param_2->forward.x;
    g_carDriverPoseMatrix.right.y = -param_2->forward.y;
    g_carDriverPoseMatrix.right.z = -param_2->forward.z;
    g_carDriverPoseMatrix.forward.x = param_2->right.x;
    g_carDriverPoseMatrix.forward.y = param_2->right.y;
    g_carDriverPoseMatrix.forward.z = param_2->right.z;
    g_carDriverPoseMatrix.position.x = 0;
    g_carDriverPoseMatrix.position.y = 0;
    g_carDriverPoseMatrix.position.z = 0;
    FixMatrix_RotateAboutRight(&g_carDriverPoseMatrix,
                               ((unsigned int)param_2->right.x & 0xffff0000) |
                                   (unsigned int)(unsigned short)g_carMountRightRotationAngle);

    // Advances the stored 12-bit phase angle by 3.
    POSE.phaseAngle = (POSE.phaseAngle + 3) % 0x1000;

    // Angle of the car body axes against the car's last acceleration.
    acc.x = pCar->velocity.x - pCar->velocityNext.x;
    acc.y = pCar->velocity.y - pCar->velocityNext.y;
    acc.z = pCar->velocity.z - pCar->velocityNext.z;
    right = FixDiv(FixVecDot(&acc, &pCar->right), 0x1e0000);
    target.z = (unsigned short)right;
    acc.x = pCar->velocity.x - pCar->velocityNext.x;
    acc.y = pCar->velocity.y - pCar->velocityNext.y;
    acc.z = pCar->velocity.z - pCar->velocityNext.z;
    target.x = (unsigned short)FixDiv(FixVecDot(&acc, &pCar->forward), 0x1e0000);
    target.y = *g_carInteriorProfilePointers[param_1->car].driverRestYaw;

    // Eases each angle a fifth of the way towards its target.
    POSE.rotation.x += ((short)target.x - (short)POSE.rotation.x) / 5;
    POSE.rotation.y += ((short)target.y - (short)POSE.rotation.y) / 5;
    POSE.rotation.z += ((short)right - (short)POSE.rotation.z) / 5;
    if (g_carDriverPoseNeedsReset[param_1->car] != 0)
        POSE.rotation = target;
    SceneNode_SetRotation(g_carInteriorNodeRows[param_1->car].driverPoseNode, &POSE.rotation);

    // Pose offset from the body axes, clamped and eased towards its target.
    off.x = FixDiv(-FixVecDot(&acc, &pCar->right), 0x4ccc);
    if (off.x < -0x28f)
        off.x = -0x28f;
    else if (off.x > 0x1999)
        off.x = 0x1999;
    off.z = FixDiv(-FixVecDot(&acc, &pCar->forward), 0x4ccc);
    if (off.z < -0xf5c)
        off.z = -0xf5c;
    else if (off.z > 0xf5c)
        off.z = 0xf5c;
    off.y = 0;
    if (g_carDriverPoseNeedsReset[param_1->car] != 0) {
        POSE.offset = off;
        g_carDriverPoseNeedsReset[param_1->car] = 0;
    } else {
        off.x -= POSE.offset.x;
        off.y = -POSE.offset.y;
        off.z -= POSE.offset.z;
        FixVecScale(&off, &off, 0xa3d);
        POSE.offset.x += off.x;
        POSE.offset.y += off.y;
        POSE.offset.z += off.z;
    }

    pos.x = g_carDriverRestPositions[param_1->car].x + POSE.offset.x;
    pos.y = g_carDriverRestPositions[param_1->car].y + POSE.offset.y;
    pos.z = g_carDriverRestPositions[param_1->car].z + POSE.offset.z;
    if (CGameInfo::IsActiveCheatEnabled(6) != 0)
        pos.y += FixMul(Car_Get(param_1->car)->cheatBodyLift, 0x8000);
    SceneNode_SetPosition(g_carInteriorNodeRows[param_1->car].driverPoseNode, &pos);
#undef POSE
}


// ---------------------------------------------------------------------------
// W151.Impl46a500_46acb0: callees that are not declared in any header used by
// this translation unit yet.
// ---------------------------------------------------------------------------
void Car_SpawnDebris(int size, FixVector *pPos, Car *pCar, FixVector *pAxes, int count, int glassChance);
void ForceFeedback_UpdateSlot(Car *pCar, FixVector *pIn, int nonzero);
void Race_PlayImpactAndShakeCar(unsigned int view, int volume, char heavy, int listener);
// Shared cooldown timestamps are defined and annotated in StageTiming.cpp.
extern int g_deformImpactTicks[2];

// Car impact update: checks the four axle travel limits, samples the current
// suspension extremes per object type and, when one of this car's objects is
// hit above 0x4ccc units/sec, builds the contact frame (forward / lateral /
// up) around the hit point, spawns the debris, adds the impact damage to the
// object record and refreshes the impact sound / force-feedback state.
// match 60%: implementada (contacto de impacto: marco forward/lateral/up, debris,
// dano y sonido); difiere el reparto de registros del bloque de normalizacion
// FUNCTION: CMR2 0x0046a500
void CarDamage_UpdateSuspensionImpactContacts(Car *pCar)
{
    CarPartSet *pParts;
    unsigned int i;
    int iBig;
    int iFlagC;
    int idx;
    int dot;
    int speed;
    BOOL bVar5;
    BOOL bVar14;
    unsigned int uVar15;
    unsigned int uVar16;
    FixVector axes[3]; // debris frame: direction, half normal, lateral axis
    FixVector vHit;

    iFlagC = 0;
    pParts = CarPart_GetSet(pCar->index);
    bVar5 = FALSE;
    bVar14 = TRUE;
    i = 0;
    do {
        int limit;

        limit = pCar->wheelRideHeight[i];
        if (limit < 0x51e)
            limit = 0x51e;
        if (pCar->wheelSpringOffset[i] >= -limit) {
            bVar14 = FALSE;
            i = 4;
        }
        i++;
    } while (i < 4);
    iBig = 0;
    if (bVar14) {
        int *p;
        int n;

        bVar5 = TRUE;
        iBig = 0;
        p = pCar->wheelSpringVelocity;
        n = 4;
        do {
            int v;

            v = *p;
            if (v < 0)
                v = -v;
            if (0xd91 < v)
                iBig = 1;
            p++;
            n--;
        } while (n != 0);
    }
    if (pCar->field_0xb74 != 0 && iBig != 0) {
        bVar5 = FALSE;
        idx = 0;
        do {
            if (StageObject_IsEligibleType((short)(unsigned short)pCar->wheelSurface[idx], 0,
                                           (int)pCar->surfaceDragLevel) != 0) {
                bVar5 = TRUE;
                idx = 4;
            }
            idx++;
        } while (idx < 4);
        if (bVar5 && 0x4ccc < pCar->speed) {
            iFlagC = 1;
            FixVecScale(&axes[0], &pCar->velocity, -0x10000);
            dot = FixVecDot(&axes[0], &pCar->groundNormal);
            FixVecScale(&axes[2], &pCar->groundNormal, dot);
            axes[0].x -= axes[2].x;
            axes[0].y -= axes[2].y;
            axes[0].z -= axes[2].z;
            StageObj_NormalizeInto(&axes[0], &axes[0]);
            FixVecCross(&axes[2], &axes[0], &pCar->groundNormal);
            StageObj_NormalizeInto(&axes[2], &axes[2]);
            FixVecCross(&axes[1], &axes[2], &axes[0]);
            StageObj_NormalizeInto(&axes[1], &axes[1]);
            FixVecScale(&axes[1], &axes[1], 0x8000);
            vHit.x = pCar->corners[0].x - pCar->corners[3].x;
            vHit.y = pCar->corners[0].y - pCar->corners[3].y;
            vHit.z = pCar->corners[0].z - pCar->corners[3].z;
            FixVecScale(&vHit, &vHit, 0x8000);
            vHit.x = vHit.x + pCar->corners[3].x - pCar->position.x;
            vHit.y = vHit.y + pCar->corners[3].y - pCar->position.y;
            vHit.z = vHit.z + pCar->corners[3].z - pCar->position.z;
            Car_SpawnDebris(pCar->speed, &vHit, pCar, &axes[0], 0x1e0000, 0);
            speed = pCar->speed;
            if (speed > 0x10000)
                speed = 0x10000;
            pParts->damageGrid[1][1] += FixMul(speed, 0x28f);
            StageObject_RebuildDamagePartValues(pCar);
        }
        uVar15 = RallyDataState();
        if ((int)pCar->index < (int)(uVar15 & 0xff)) {
            i = (int)pCar->index;
            uVar16 = CMain::GetFrameDelta();
            if (0x19 < (unsigned int)(uVar16 - g_deformImpactTicks[i]) && pCar->speed > 0) {
                if (pCar->speed > 0x10000) {
                    uVar15 = 0x5c28;
                } else {
                    uVar15 = (unsigned int)FixMul(pCar->speed, 0x5c28);
                }
                i = (int)pCar->index;
                Race_PlayImpactAndShakeCar((unsigned int)i, (int)uVar15, (char)iFlagC, i);
                uVar16 = CMain::GetFrameDelta();
                g_deformImpactTicks[(int)pCar->index] = (int)uVar16;
            }
        }
        ForceFeedback_UpdateSlot(pCar, &pCar->velocity, 0);
        FixVecScale(&pCar->velocity, &pCar->velocity, 0xf851);
    } else if (pCar->field_0xb74 != 0 && bVar5) {
        ForceFeedback_UpdateSlot(pCar, &pCar->velocity, 0);
    }

}

// Rebuilds the per-part bounding box of a stage object record for one car:
// converts the packed integer vertices of every part to floats, tracks the
// per-part min/max in 16.16 units, expands the record's global x/z bounds and
// stores each part centre and its half extents.
// FUNCTION: CMR2 0x0046acb0
void CarDamage_RebuildPartBounds(int param_1, SceneNode *param_2, CarPartSet *param_3)
{
    CarPartSet *set = param_3;
    int i;
    int j;
    int match;
    int key;
    int maxX;
    int maxY;
    int maxZ;
    int minX;
    int minY;
    int minZ;
    int fx;
    int fy;
    int fz;
    FixVector pos;
    FixVector normal;
    FixVector extents;

    if (g_carDamageModelEnabled[param_1] == 0)
        return;
    if (g_carOriginalPartVertices[param_1] == 0)
        CarDamage_BuildPartVertexBuffer(param_1, param_2, param_3);
    for (i = 0; i < set->count; i++) {
        key = set->geometry.nodes[i]->flags & 0xff;
        match = -1;
        for (j = 0; j <= set->count; j++) {
            if (g_carOriginalPartNodeTypes[param_1][j] == key) {
                match = j;
                j = set->count;
            }
        }
        if (match >= 0) {
            set->geometry.vertices[i] = g_carOriginalPartVertices[param_1][match];
            maxX = maxY = maxZ = -0x640000;
            minX = minY = minZ = 0x640000;
            for (j = 0; j < set->vertexCount[i]; j++) {
                pos = set->geometry.vertices[i][j].pos;
                ((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[0] = X87S_MUL(pos.x, CGraphics::m_oneOver65536);
                ((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[1] = X87S_MUL(pos.y, CGraphics::m_oneOver65536);
                ((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[2] = X87S_MUL(pos.z, CGraphics::m_oneOver65536);
                normal = set->geometry.vertices[i][j].normal;
                ((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].normal[0] = X87S_MUL(normal.x, CGraphics::m_oneOver65536);
                ((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].normal[1] = X87S_MUL(normal.y, CGraphics::m_oneOver65536);
                ((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].normal[2] = X87S_MUL(normal.z, CGraphics::m_oneOver65536);
                fx = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[0], CGraphics::m_65536));
                fy = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[1], CGraphics::m_65536));
                fz = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)set->geometry.meshes[i]->pVertexData)[j].pos[2], CGraphics::m_65536));
                if (fx > maxX)
                    maxX = fx;
                if (fx < minX)
                    minX = fx;
                if (fy > maxY)
                    maxY = fy;
                if (fy < minY)
                    minY = fy;
                if (fz > maxZ)
                    maxZ = fz;
                if (fz < minZ)
                    minZ = fz;
                if (fx >= 0) {
                    if (fx > set->maxX)
                        set->maxX = fx;
                } else if (fx < set->minX) {
                    set->minX = fx;
                }
                if (fz >= 0) {
                    if (fz > set->maxZ)
                        set->maxZ = fz;
                } else if (fz < set->minZ) {
                    set->minZ = fz;
                }
            }
            extents.x = minX - maxX;
            extents.y = minY - maxY;
            extents.z = minZ - maxZ;
            FixVecScale(&extents, &extents, 0x8000);
            set->centres[i].x = extents.x + maxX;
            set->centres[i].y = extents.y + maxY;
            set->centres[i].z = extents.z + maxZ;
            set->halfExtents[i].x = maxX - set->centres[i].x;
            set->halfExtents[i].y = maxY - set->centres[i].y;
            set->halfExtents[i].z = maxZ - set->centres[i].z;
        }
    }
}

// Constantes de cuantizacion de los registros de luz (compartidas con
// NetRace.cpp). Las de NetRace/Graphics/Car/GameInfo ya tienen su anotacion
// // GLOBAL: en su fichero de definicion; aqui solo van los extern.
extern const float g_netZero;
extern const float g_netElevationScale;
extern const float g_netByteScale;
extern const float g_netHeadingScale;
extern const float g_netSignedShortScale;
extern const double g_netAcosScale;

// FixAcos with the shared -4095.0 constant of the network/replay packers.
static inline short Replay_Acos(int x)
{
    int neg = 0;
    double t;

    if (x < 0) {
        x = -x;
        neg = 1;
    }
    if (x > 0x10000) {
        return g_acosTable[4095];
    }
    t = X87S_MUL(X87S_MUL((double)x, CGraphics::m_oneOver65536), g_netAcosScale);
    if (neg) {
        return -g_acosTable[-FTOI(__int64, t)];
    }
    return g_acosTable[-FTOI(__int64, t)];
}
extern const float g_netMinusOne;
extern const float g_netOne;
extern float g_65536f;
extern double g_fixedDegreesToAngle12;
extern const float g_float12Over17;   // defined in NetRace.cpp (single definition)
extern const float g_float24Over17;   // defined in NetRace.cpp (single definition)
extern const float g_float128;   // defined in NetRace.cpp (single definition)
extern const float g_floatOneOver32765;   // defined in NetRace.cpp (single definition)

// Rebuilds the per-frame light/colour record of a car from its render matrix:
// the car position relative to the sector it stands in is packed as a signed
// 16.16 angle pair into param_2[1], the sector index into param_2[2], the two
// basis vectors (right/forward) are converted with the atan2/acos tables into
// the two colour bytes of param_2[2] / param_2[3], and the light intensity and
// entity/flag bits are gathered into param_2[3].
// FUNCTION: CMR2 0x0046d8d0
void Replay_EncodeCarPoseSample(Car *pCar, ReplaySample *pSample)
{
    FixVector basis[2];
    FixVector size;
    FixVector pos;
    FixVector *pBasis;
    Sector *pSector;
    float fX;
    float fZ;
    float heading;
    int i;
    int angle;
    int pitch;
    int level;
    int flag;
    float elevation;

    FixMatrix_GetPosition(&pos, pCar->pWorld);
    pSample->y = pos.y;
    pSample->sector = pCar->sector;
    pSector = g_sectors[pCar->sector];
    pos.x -= pSector->x;
    pos.y -= pSector->y;
    pos.z -= pSector->z;
    fX = (float)(X87S_MUL((X87S_MUL((double)pos.x, CGraphics::m_oneOver65536)), CGraphics::m_oneOver128));
    fZ = (float)(X87S_MUL((X87S_MUL((double)pos.z, CGraphics::m_oneOver65536)), CGraphics::m_oneOver128));

    if (fX >= g_netOne)
        pSample->x = 0x7ffd;
    else if (fX <= g_netMinusOne)
        pSample->x = -0x7ffd;
    else
        pSample->x = (int)FTOI(__int64, fX * g_netSignedShortScale);
    if (fZ >= g_netOne)
        pSample->z = 0x7ffd;
    else if (fZ <= g_netMinusOne)
        pSample->z = -0x7ffd;
    else
        pSample->z = (int)FTOI(__int64, fZ * g_netSignedShortScale);

    FixMatrix_GetRight(&basis[0], pCar->pWorld);
    FixMatrix_GetForward(&basis[1], pCar->pWorld);
    for (i = 0; i < 2; i++) {
        pBasis = &basis[i];
        size.x = pBasis->x;
        if (pBasis->x < 0)
            size.x = -pBasis->x;
        size.y = pBasis->y < 0 ? -pBasis->y : pBasis->y;
        size.z = pBasis->z < 0 ? -pBasis->z : pBasis->z;
        if (size.x == 0)
            angle = 0;
        else
            angle = FixAtan2(size.z, size.x) * 0x1680;
        pitch = (0x400 - Replay_Acos(size.y)) * 0x1680;
        if (pBasis->x >= 0 && pBasis->z <= 0)
            angle = 0x1680000 - angle;
        else if (pBasis->x <= 0 && pBasis->z >= 0)
            angle = 0xb40000 - angle;
        else if (pBasis->x <= 0 && pBasis->z <= 0)
            angle += 0xb40000;
        if (pBasis->y <= 0)
            pitch = 0xb40000 - pitch;
        heading = (float)(X87S_MUL((X87S_MUL((X87S_MUL((double)angle, CGraphics::m_oneOver65536)), g_netHeadingScale)), g_netByteScale));
        elevation = (float)(X87S_MUL((X87S_MUL((X87S_MUL((double)pitch, CGraphics::m_oneOver65536)), g_netElevationScale)), g_netByteScale));
        if (heading < g_netZero)
            heading = 0.0f;
        else if (heading > g_netByteScale)
            heading = 255.0f;
        if (elevation < g_netZero)
            elevation = g_netZero;
        else if (elevation > g_netByteScale)
            elevation = g_netByteScale;
        if (i == 0) {
            pSample->rightAngles = ((int)FTOI(__int64, heading) & 0xff) | ((int)FTOI(__int64, elevation) << 8);
        } else {
            pSample->forwardHeading = (int)FTOI(__int64, heading);
            pSample->forwardPitch = (int)FTOI(__int64, elevation);
        }
    }

    level = FixMul(FixDiv(*(short *)&pCar->wheelSteeringAngle * 0x1680, pCar->maxSteeringAngleDegrees * 0x1680) + 0x10000,
                   0xf8000);
    if (level < 0)
        level = 0;
    else if (level > 0x1f0000)
        level = 0x1f0000;
    pSample->level = level >> 16;
    pSample->steering = pCar->throttleTorque != 0 ? 1 : 0;
    pSample->flag24 = *(unsigned int *)&pCar->braking;
    pSample->flag27 = pCar->field_0xc14;
    pCar->field_0xc14 = 0;
    pSample->bits21 = pCar->shakeLevel;
    pCar->shakeLevel = 0;
    flag = 0;
    if (pCar->pAlternateBodyNode != NULL && pCar->pAlternateBodyNode->viewMask != 0)
        flag = 1;
    if (pCar->pBodyNode != NULL && pCar->pBodyNode->viewMask != 0)
        flag = 1;
    pSample->flag26 = flag;
}

// Builds the world matrix of one light/effect record: the position comes from
// the record's quantised offset inside its sector, the right/forward basis is
// decoded from the two packed byte angles (12-bit table sin/cos) and the up
// vector from their cross product, and the record's flag/intensity word is
// decoded into the six output pointers.
inline int FloatToFix(float f)
{
    // PORT: fld/fmul/fistp: the product is exact, fistp rounds to nearest.
    return (int)lrintf(f * g_65536f);
}

// FUNCTION: CMR2 0x0046de20
void Replay_DecodeCarPoseSample(unsigned int *pFlag24, unsigned int *pFlag27, unsigned int *pSteering, int *pLevel,
                  FixMatrix *pMatrix, int *pBits21, unsigned int *pFlag26, ReplaySample *pSample)
{
    FixVector axes[3];
    FixVector pos;
    float pitch[2];
    float heading[2];
    short yaw[2];
    short tilt[2];
    float offX;
    float offZ;
    Sector *pSector;
    int i;
    short a;
    short b;
    FixVector *pAxis;
    DWORD angles;

    offX = X87S_MUL((X87S_MUL((float)(short)(((DWORD *)pSample)[1] & 0xffff), g_floatOneOver32765)), g_float128);
    offZ = X87S_MUL((X87S_MUL((float)(short)(((DWORD *)pSample)[1] >> 16), g_floatOneOver32765)), g_float128);
    pos.x = FloatToFix(offX);
    pos.y = 0;
    pos.z = FloatToFix(offZ);
    pSector = g_sectors[(short)pSample->sector];
    pos.x += pSector->x;
    pos.z += pSector->z;
    pos.y = pSample->y;
    FixMatrix_SetPosition(&pos, pMatrix);

    angles = ((DWORD *)pSample)[2];
    heading[0] = (float)(WORD)((BYTE *)&angles)[2];
    pitch[0] = (float)(((WORD *)&angles)[1] >> 8);
    heading[1] = (float)((BYTE *)pSample)[0xc];
    pitch[1] = (float)(WORD)((BYTE *)pSample)[0xd];
    for (i = 0; i < 2; i++) {
        float vh = (heading[i] *= g_float24Over17);
        float vp = (pitch[i] *= g_float12Over17);
        yaw[i] = (short)FTOI(__int64, X87S_MUL((double)FloatToFix(vh), g_fixedDegreesToAngle12));
        tilt[i] = (short)FTOI(__int64, X87S_MUL((double)FloatToFix(vp), g_fixedDegreesToAngle12));
        a = yaw[i];
        b = tilt[i];
        axes[i * 2].x = FixMul(g_sinTable[(unsigned short)b & 0xfff], g_sinTable[((unsigned short)a + 0x400) & 0xfff]);
        axes[i * 2].y = g_sinTable[((unsigned short)b + 0x400) & 0xfff];
        axes[i * 2].z = FixMul(g_sinTable[(unsigned short)b & 0xfff], g_sinTable[(unsigned short)a & 0xfff]);
        StageObj_NormalizeInto(&axes[i * 2], &axes[i * 2]);
    }
    FixVecCross(&axes[1], &axes[2], &axes[0]);
    StageObj_NormalizeInto(&axes[1], &axes[1]);
    FixMatrix_SetRight(&axes[0], pMatrix);
    FixMatrix_SetUp(&axes[1], pMatrix);
    FixMatrix_SetForward(&axes[2], pMatrix);

    *pLevel = FixMul(pSample->level << 16, 0x1083) - 0x10000;
    *pSteering = pSample->steering << 16;
    *pFlag24 = pSample->flag24;
    *pFlag27 = pSample->flag27;
    *pBits21 = FixMul(pSample->bits21 << 16, 0x2000);
    *pFlag26 = pSample->flag26;
}

void CarLight_GetBrokenFlags(int *pBroken0, int *pBroken1, Car *pCar);
int Surface_GetTransitionBlend(int surface, int t);
SceneNode *Scene_CreateLight(int type, int r, int g, int b, FixVector *pPosition, FixAngles *pAngles,
                             SceneNode *pParent);

// GLOBAL: CMR2 0x00547ce0
int g_carDamageHazardTimer[8];
// GLOBAL: CMR2 0x00547d00
GlowLight *g_carLightGlowSlots[8 * 0x14]; // 20 glow slots per car, up to 8 cars (runs to 0x547f80)

// Creates the CIN point glows, links exhaust emitters and records each point's
// closest deform vertex. Car zero also creates the two rear-view lights.
// FUNCTION: CMR2 0x00463fe0
void CarLight_BuildPointGlows(Car *pCar)
{
// the original re-reads the car index at every use
#define LCAR (pCar->index)
    int nearestDistanceSquared[20];
    CarPartSet *pParts;
    FixVector scale;
    FixAngles lightAngles;
    FixVector position;
    int meshIndex;
    int carIndex;
    int lightIndex;
    CarLightPoint *pPoint;

    lightIndex = 0;
    lightAngles.z = 0xff1d;
    scale.x = 0;
    scale.y = 0;
    scale.z = 0;
    lightAngles.x = 0;
    lightAngles.y = 0;
    lightAngles.pad = 0;
    if (LCAR == 0) {
        g_rearViewLightOffsetA.x = 0x1e0000;
        g_rearViewLightOffsetA.y = 0;
        g_rearViewLightOffsetA.z = 0;
        g_rearViewLightOffsetB.x = 0xf0000;
        g_rearViewLightOffsetB.y = 0;
        g_rearViewLightOffsetB.z = 0;
        position.x = 0;
        position.y = 0;
        position.z = 0;
        g_rearViewLightNodeA = Scene_CreateLight(0, 0x140000, 0x140000, 0x140000, &position, &lightAngles,
                                            RallyData_GetChallengeRenderState());
        g_rearViewLightNodeB = Scene_CreateLight(0, 0x140000, 0x140000, 0x140000, &position, &lightAngles,
                                            RallyData_GetChallengeRenderState());
    }
    {
        CarLightProfile *pSet = (CarLightProfile *)CarInfo_GetSection(pCar, CAR_INFO_LIGHTS);
        int exhaustSide = 0;
        int *pNearestDistance;
        char projectedBrightness;
        SceneNode *pNode;

        g_carLightProfiles[LCAR] = pSet;
        g_carLightPoints[LCAR] = pSet->points;
        if (0 < g_carLightProfiles[LCAR]->count) {
            pNearestDistance = nearestDistanceSquared;
            do {
                pPoint = &g_carLightPoints[LCAR][lightIndex];
                if (CarDamage_GetModelEnabled(pCar) == 0 || pPoint->part == -1) {
                    pNode = pCar->pBodyNode;
                } else {
                    pNode = CarPart_GetNode(pCar, (int)pPoint->part);
                    if (pNode == NULL)
                        pNode = pCar->pBodyNode;
                }
                Frontend_SetObjectField118(g_carLightTexA[pPoint->slot], 1);
                Frontend_SetObjectField118(g_carLightTexB[pPoint->slot], 1);
                scale.x = 0x10000;
                projectedBrightness = 0;
                if (pPoint->type == 9)
                    projectedBrightness = (char)0xff;
                position.x = 0;
                position.y = 0;
                position.z = 0;
                g_carLightGlowSlots[lightIndex + LCAR * 0x14] =
                    Glow_Add(2, &pPoint->pos, &pPoint->dir, &scale, pPoint->size, pPoint->size,
                                  g_carLightTexA[pPoint->slot], g_carLightTexB[pPoint->slot],
                                  pPoint->intensity, pNode, (BYTE)projectedBrightness, &position, 0x10000);
                Glow_SetEnabled(g_carLightGlowSlots[lightIndex + LCAR * 0x14], 1);
                *pNearestDistance = 0x3e80000;
                if (pPoint->type == 9) {
                    CarExhaust_SetPoint(pPoint, exhaustSide, LCAR);
                    CarExhaust_SetGlow(g_carLightGlowSlots[lightIndex + LCAR * 0x14], exhaustSide, LCAR);
                    exhaustSide++;
                }
                pNearestDistance++;
                lightIndex++;
            } while (lightIndex < g_carLightProfiles[LCAR]->count);
        }
    }
    if (CarDamage_GetModelEnabled(pCar) != 0) {
        int vertexIndex;

        pParts = CarPart_GetSet(LCAR);
        for (meshIndex = 0; meshIndex < pParts->count; meshIndex++) {
            for (vertexIndex = 0; vertexIndex < pParts->vertexCount[meshIndex]; vertexIndex++) {
                for (lightIndex = 0; lightIndex < g_carLightProfiles[LCAR]->count; lightIndex++) {
                    int distanceSquared;
                    FixVector vertexPosition;

                    pPoint = &g_carLightPoints[LCAR][lightIndex];
                    vertexPosition = pParts->geometry.vertices[meshIndex][vertexIndex].pos;
                    position.x = pPoint->pos.x - vertexPosition.x;
                    position.y = pPoint->pos.y - vertexPosition.y;
                    position.z = pPoint->pos.z - vertexPosition.z;
                    distanceSquared = FixVecDot(&position, &position);
                    if (distanceSquared < nearestDistanceSquared[lightIndex]) {
                        nearestDistanceSquared[lightIndex] = distanceSquared;
                        pPoint->object = (short)meshIndex;
                        pPoint->vertex = (short)vertexIndex;
                    }
                }
            }
        }
    }
    g_rearViewLightAttenuation = 0xffff0000;
    for (carIndex = 0; carIndex < 8; carIndex++)
        g_carDamageHazardTimer[carIndex] = 0;
#undef LCAR
}

// Drives the two light flag bytes of a car (brake/reverse/hazard/head) and
// updates the glow light of every collision point: it enables/fades it from
// the per-lane tuning table, colours it by the point type and hangs it on the
// car body, or disables every glow when the car is not racing.
// FUNCTION: CMR2 0x004643f0
void CarLight_UpdateFlagsAndGlows(Car *pCar)
{
// the original re-reads the car index at every use
#define LIGHT_CAR (pCar->index)
#define LIGHT_COUNT(c) (g_carLightProfiles[c]->count)
#define LIGHT_POINTS(c) g_carLightPoints[c]
    int channelLevels[11];
    FixVector planePos;
    FixVector colour;
    FixVector pos;
    FixVector meshPosition;
    CarPartSet *pParts;
    CarLightPoint *pPoint;
    GlowLight *glow;
    int broken0;
    int broken1;
    int previousHazardTime;
    int lightIndex;
    int glowIntensity;
    int wheelCorner;
    int originalX;
    int originalY;
    int originalZ;
    int meshIndex;
    int vertexIndex;
    DeformVertex *pVertex;

    pParts = CarPart_GetSet(LIGHT_CAR);
    if (pCar->field_0xb70 != 0) {
        CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 2);
        CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 8);
        for (lightIndex = 0; lightIndex < LIGHT_COUNT(LIGHT_CAR); lightIndex++)
            Glow_SetEnabled(g_carLightGlowSlots[lightIndex + LIGHT_CAR * 0x14], 0);
        return;
    }
    CarLight_GetBrokenFlags(&broken0, &broken1, pCar);
    if (pCar->braking != 0)
        CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 2);
    else
        CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 2);
    if (pCar->reversing != 0)
        CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 8);
    else
        CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 8);
    if (pCar->field_0xb58 != 0) {
        CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 1);
        // OpenCMR2: the shared type-8 channel was never damage-gated, so its
        // lamps kept shining with every lamp glass broken. Darken it only
        // when BOTH sides' lamp glass is broken: the channel carries a
        // single shared level, so single-side gating could extinguish an
        // intact lamp. Per-side behaviour stays exactly as the original.
        CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0 || broken1 == 0, broken0 == 0 || broken1 == 0, 0x10);
    } else {
        CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 1);
        CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 0x10);
    }
    if (pParts->damageValues[23] > 0x4ccc) {
        previousHazardTime = g_carDamageHazardTimer[LIGHT_CAR];
        g_carDamageHazardTimer[LIGHT_CAR] = previousHazardTime;
        g_carDamageHazardTimer[LIGHT_CAR] += g_stageObjectTimeStep;
        if (g_carDamageHazardTimer[LIGHT_CAR] > 0x140000)
            g_carDamageHazardTimer[LIGHT_CAR] = 0;
        if (g_carDamageHazardTimer[LIGHT_CAR] == 0)
            CarLight_SetChannelMasks(LIGHT_CAR, 0, 0, 4);
        if (previousHazardTime < 0xa0000 && g_carDamageHazardTimer[LIGHT_CAR] >= 0xa0000)
            CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 4);
    }
    CarLight_GetScaledChannelLevels(LIGHT_CAR, &channelLevels[0], &channelLevels[1], 1);
    CarLight_GetScaledChannelLevels(LIGHT_CAR, &channelLevels[2], &channelLevels[3], 3);
    CarLight_GetScaledChannelLevels(LIGHT_CAR, &channelLevels[4], &channelLevels[5], 2);
    CarLight_GetScaledChannelLevels(LIGHT_CAR, &channelLevels[6], &channelLevels[7], 0);
    CarLight_GetScaledChannelLevels(LIGHT_CAR, &channelLevels[8], &channelLevels[8], 4);
    channelLevels[10] = 0;
    channelLevels[9] = 0;
    planePos = pCar->corners[0];
    planePos.y = pCar->cornerHeight[0];
    for (lightIndex = 0; lightIndex < LIGHT_COUNT(LIGHT_CAR); lightIndex++) {
        pPoint = &LIGHT_POINTS(LIGHT_CAR)[lightIndex];
        glow = g_carLightGlowSlots[lightIndex + LIGHT_CAR * 0x14];
        if (glow == NULL)
            continue;
        if (channelLevels[pPoint->type] == 0) {
            Glow_SetEnabled(glow, 0);
        } else {
            Glow_SetEnabled(glow, 1);
            Glow_SetIntensity(glow, FixMul(channelLevels[pPoint->type], pPoint->intensity));
            glowIntensity = FixMul(channelLevels[pPoint->type], pPoint->intensity);
            glowIntensity = FixMul(glowIntensity, 0x8000);
            switch (pPoint->slot) {
            case 0:
                colour.x = 0xe000;
                colour.y = 0x1c28;
                colour.z = 0;
                break;
            case 1:
            case 4:
                colour.x = 0x10000;
                colour.y = 0x10000;
                colour.z = 0x10000;
                break;
            default:
                colour.x = 0x10000;
                colour.y = 0x9893;
                colour.z = 0;
                break;
            }
            // Record the light for the fog pass (video.fog_lights) with the same
            // world position and tint the glow sprite uses (colour pre-intensity).
            {   FixVector worldPos, worldDir = glow->dir;
                if (glow->pNode != NULL) {
                    FixVector nodePos;
                    FixMatrix_RotateVector(&worldPos, &glow->pos, &glow->pNode->world);
                    FixMatrix_RotateVector(&worldDir, &glow->dir, &glow->pNode->world);
                    FixMatrix_GetPosition(&nodePos, &glow->pNode->world);
                    worldPos.x += nodePos.x;
                    worldPos.y += nodePos.y;
                    worldPos.z += nodePos.z;
                } else {
                    worldPos = glow->pos;
                }
                LightField_Add(&worldPos, &worldDir, glowIntensity * 2 > 0x10000 ? 0x10000 : glowIntensity * 2,
                               colour.x, colour.y, colour.z, pPoint->type == 8 ? 1 : 0);   // type 8 = front headlight in the Subaru CIN (lamps 10-11 of 13)
            }
            FixVecScale(&colour, &colour, glowIntensity);
            Glow_NoOpEntryCallback((BYTE)(INT_PTR)glow, (BYTE)colour.x, colour.y, colour.z);
        }
        wheelCorner = 0;
        if (glow->pos.x < 0)
            wheelCorner = 2;
        if (glow->pos.z < 0)
            wheelCorner++;
        Glow_SetLayerPlane(glow, &planePos, &pCar->groundNormal,
                           Surface_GetTransitionBlend(pCar->wheelSurface[wheelCorner],
                                        StageObject_GetCarWeatherRampValue(pCar)));
        if (CarDamage_GetModelEnabled(pCar) != 0) {
            vertexIndex = (unsigned short)pPoint->vertex;
            meshIndex = (unsigned short)pPoint->object;
            pVertex = pParts->geometry.vertices[meshIndex] + vertexIndex;
            originalX = pVertex->pos.x;
            originalY = pVertex->pos.y;
            originalZ = pVertex->pos.z;
            Mesh_ReadVertexFixed(pParts->geometry.meshes, meshIndex, vertexIndex, &meshPosition);
            pos.x = meshPosition.x - originalX;
            pos.y = meshPosition.y - originalY;
            pos.z = meshPosition.z - originalZ;
            pos.x += pPoint->pos.x;
            pos.y += pPoint->pos.y;
            pos.z += pPoint->pos.z;
            Glow_SetPosition(glow, &pos, &pPoint->dir);
        }
    }
#undef LIGHT_CAR
#undef LIGHT_COUNT
#undef LIGHT_POINTS
}

// Dependencias de la cadena de 0x46cce0 (0x46c2a0 / 0x46c410 / 0x469690):
// prototipos que no estan en ninguna cabecera incluida por esta unidad.
extern CarDamageRecord *g_carDamageRecords;
void CarDamage_ApplyCollisionDeformImpulse(Car *pCar, int *param_2, FixVector *param_3, int param_4, unsigned char param_5,
                  int param_6);
void StageTiming_RebuildDamagedPartMeshes(Car *pCar);
void StageTiming_FlagCarPartBreaks(Car *pCar, int param_2);
void CarDamage_DecodeImpactPayload(CarImpactPayload *p);
void StageObject_RebuildDamagePartValues(Car *pCar);
void StageObject_ResetVectorListRecord(int list, int index, int value);
void CarPart_HideNode(Car *pCar, int slot, int hidden);
void CarPart_ResetAttachedNodes(Car *pCar);
void RallyData_MarkAllElementsReached(void);
void StageTiming_ResetScaledViewObjectStates(void);
void Stage_RestoreCarsToRoutePositions(void);
void StageTiming_PlaceEventStartingGrid(char param_1);
void StageTiming_ResetParticipatingDriverRecords(int param_1);
void StageObject_StartReplaySession(char restart);
void Car_ReloadModels(int, int, int);
CarPartSet *CarPart_GetSet(int carIndex);
void RallyData_ResetRaceRecordAndRouteProbe(int index);
void RallyData_RestoreAllCarRaceRecords(void);
RaceRecord *RallyData_GetCarRaceRecord(int index);
unsigned char RallyData_GetSelectionFlag26(void);
unsigned int RallyData_GetSelectionFlag27(void);
unsigned char RallyDataState(void);

// Resets the per-car stage-object block: clears the pose/timing fields, walks
// the object chain calling the pre-step of every entry, then recomputes the
// 0x22 light intensities from the stored bytes and mirrors three palette
// entries and four geometry offsets.
// match 22%: implementada; difiere el codegen del bucle de la cadena de objetos
// (indice*0xd + base) y de la division 64-bit de las intensidades
// FUNCTION: CMR2 0x00469690
void StageObject_ResetCarObjectState(Car *pCar)
{
    CarPartSet *set;
    CarDamageRecord *pRecord;
    CarDamageLink *pLink;
    FixVector saved5dc;
    FixVector saved5c4;
    int i;

    set = g_carPartSets + pCar->index;
    pRecord = g_carDamageRecords + pCar->index;
    CarPart_ResetAttachedNodes(pCar);
    StageTiming_RebuildDamagedPartMeshes(pCar);
    set->engineTorqueScale = 0x10000;
    set->frontBrakeScale = 0x10000;
    set->rearBrakeScale = 0x10000;
    set->wheelDamageDrag[0] = 0;
    set->wheelDamageDrag[1] = 0;
    set->wheelDamageDrag[2] = 0;
    set->wheelDamageDrag[3] = 0;
    set->bodyDamageDrag = 0;
    set->steeringWobble = 0;
    set->gearShiftDamage = 0;
    saved5dc = pCar->field_0x5dc;
    saved5c4 = pCar->field_0x5c4;
    pLink = &pRecord->impacts.links[pRecord->impacts.firstLink];
    if (pRecord->impacts.count) {
        while (pLink != NULL) {
            CarDamage_DecodeImpactPayload(&pLink->impact);
            CarDamage_ApplyCollisionDeformImpulse(pCar, 0, 0, 0, 0, 1);
            if (pLink->next == -1)
                break;
            pLink = &pRecord->impacts.links[pLink->next];
        }
    }
    pCar->field_0x5dc = saved5dc;
    pCar->field_0x5c4 = saved5c4;
    for (i = 0; i < 0x22; i++) {
        set->damageBiases[i] = pRecord->damage.intensity[i] << 16;
        set->damageBiases[i] = FixDiv(set->damageBiases[i], 0xff0000);
    }
    for (i = 0; i < 3; i++) {
        set->lineGrounded[i] = pRecord->damage.lineGrounded[i];
        StageObject_ResetVectorListRecord(pCar->index, i, set->lineGrounded[i]);
    }
    for (i = 0; i < 4; i++) {
        set->partHidden[i] = pRecord->damage.partHidden[i];
        CarPart_HideNode(pCar, i, set->partHidden[i]);
    }
    StageObject_RebuildDamagePartValues(pCar);
    StageTiming_FlagCarPartBreaks(pCar, 1);
    if (Sys_GetOption("video.diag_replay_glass", 0) != 0 && CGameInfo::GetConfiguredGameMode() == 4) {
        for (i = 0; i < 8; i++)
            Sys_Log("replay-glass-diag: reset-restore car %d slot %d saved-intensity %d bias %x dmg %x broken %d damaged %d",
                    (int)pCar->index, i, (int)pRecord->damage.intensity[26 + i],
                    set->damageBiases[26 + i], set->damageValues[26 + i],
                    set->partBroken[i], set->partDamaged[i]);
    }
}

// Initialises the stage-object state of one car (list type 0): resets the car's
// per-type block, copies it into the stage block, mirrors the road book entry
// when the car has one, and refreshes the rally-data bookkeeping.
// FUNCTION: CMR2 0x0046c2a0
void Replay_InitObjectListCarState(ReplayInputLane *pLane, BYTE param_2)
{
    Car *pCar;
    RaceRecord *pRecord;

    pCar = Car_Get((int)param_2);
    StageObject_ResetCarObjectState(pCar);
    Replay_RestoreCarState((Block0x309 *)pLane->carState, pCar);
    if (pCar->field_0xb50 != 0) {
        Replay_RestoreAuxState((Block0x134 *)pLane->partState, (Block0x134 *)CarPart_GetSet((int)param_2));
    }
    pRecord = RallyData_GetCarRaceRecord((int)param_2);
    Replay_CopyBlock6((Block6 *)pLane->raceRecord, (Block6 *)pRecord);
    RallyData_ResetRaceRecordAndRouteProbe((int)param_2);
    StageTiming_ResetScaledViewObjectStates();
    RallyData_MarkAllElementsReached();
    RallyData_ValidateIndex(param_2);
}

// Initialises the stage-object state of one car (list type 2): same reset as
// above but without the per-type block copy or the road book mirror.
// FUNCTION: CMR2 0x0046c410
void Replay_InitMeshListCarState(ReplayStateLane *pLane, BYTE param_2)
{
    Car *pCar;

    pCar = Car_Get((int)param_2);
    StageObject_ResetCarObjectState(pCar);
    RallyData_ResetRaceRecordAndRouteProbe((int)param_2);
    StageTiming_ResetScaledViewObjectStates();
    RallyData_MarkAllElementsReached();
    RallyData_ValidateIndex(param_2);
}

// Selects one lane of one car's stage-object state and initialises it for the
// given type: validates the record, stores the lane/timer fields, dispatches to
// the type-specific reset (0x46c2a0 / 0x46c390 / 0x46c410), flushes pending
// events, copies the light byte and, for type 2, rebuilds the two pose matrices
// and the car's matrix/mirror state.
// FUNCTION: CMR2 0x0046cce0
int Replay_SelectAndInitializeLane(ReplayStream *p, short lane, short start, BYTE car)
{
    if (p == NULL || p->recording != 0 || p->playing != 0)
        return 0;
    if (lane >= p->laneCount)
        return 0;
    p->car = car;
    p->playing = 1;
    p->lane = lane;
    p->playStarted = 1;
    if (start == 0)
        p->frame = 0;
    else
        p->frame = -start;
    p->field_0x21 = 0;
    if (p->type == 0)
        Replay_InitObjectListCarState(p->pInputs + lane, car);
    else if (p->type == 1)
        Replay_RestoreCarTorqueAndRecordState((int *)(p->pStates + lane), car);
    else
        Replay_InitMeshListCarState(p->pStates + lane, car);
    Events_Flush();
    p->field_0x14 = 0;
    if (p->type == 0)
        p->field_0x10c = p->pInputs[lane].events[0].value;
    else
        p->field_0x10c = p->pStates[lane].events[0].value;
    if (car == 0 && (RallyData_GetSelectionFlag26() || ((char)RallyData_GetSelectionFlag27() && !CGameInfo::GetGameModeOptionBit19()))) {
        RallyData_RestoreAllCarRaceRecords();
        StageTiming_PlaceEventStartingGrid(1);
        if (RallyData_GetSelectionFlag26()) {
            Car_ReloadModels(RallyDataState(), Car_GetOrderCount() - RallyDataState(), 1);
            StageObject_StartReplaySession(0);
        }
        Stage_RestoreCarsToRoutePositions();
        StageTiming_ResetParticipatingDriverRecords(0);
    }
    p->step = 0;
    if (p->type == 2) {
        Replay_DecodeCarPoseSample(&p->field_0xec, &p->flagPrev, (unsigned int *)&p->steerFrom, &p->headingFrom, &p->pose.from,
                     &p->eventPrev, &p->field_0xe8, (ReplaySample *)p->pSamples);
        Replay_DecodeCarPoseSample(&p->field_0xec, &p->flag, (unsigned int *)&p->steerTo, &p->headingTo, &p->pose.to,
                     &p->event, &p->field_0xe8, (ReplaySample *)p->pSamples + 1);
        if ((CGameInfo::GetGameModeOptionBit19() && p->car == 0) || !CGameInfo::GetGameModeOptionBit19())
            Car_Get(p->car)->field_0xc18 = 1;
        Car_Get(p->car)->velocity.x = 0;
        Car_Get(p->car)->velocity.y = 0;
        Car_Get(p->car)->velocity.z = 0;
        Car_Get(p->car)->velocityNext.x = 0;
        Car_Get(p->car)->velocityNext.y = 0;
        Car_Get(p->car)->velocityNext.z = 0;
        Replay_ComputePoseVelocityCorrection(&p->pose, Car_Get(p->car));
        Car_Get(p->car)->field_0xbf8 = 1;
    }
    return 1;
}
// ===========================================================================
// Stage-object pass, animation and collision code restored for the layer 1
// pass (see CONOCIMIENTO.md). Declarations of helper symbols that live in
// other translation units, hoisted so the functions below build.
// ===========================================================================
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
void Car_SpawnDebris(int size, FixVector *pPos, Car *pCar, FixVector *pAxes, int count, int glassChance);
void CarDamage_UpdateSuspensionImpactContacts(Car *pCar);
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
// Nota: StageObject_UpdatePlayerControlIndicators debe colocarse DESPUES de StageObject_ReadCarControllerMapping y de las definiciones
// de g_carButtonMasks / g_controlCar / g_carAxisInputEnabled (todas en este fichero).
int GameMenu_IsPauseHeaderActive(void);
int Race_GetPlayerRecordField4(BYTE index);
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
void CarDamage_ApplyCollisionDeformImpulse(Car *pCar, int *param_2, FixVector *param_3, int param_4,
                  unsigned char param_5, int param_6);
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
extern CarFlexibleLineDescriptor *g_carLineDescriptors[8];
extern CarFlexibleLineState **g_carLineStates;
extern int g_physicsTimeStep;
// Per-channel colour gains of the current frame, written by
// StageObject_DrawListedCarLightBeams, each clamped to 0x10000.
// GLOBAL: CMR2 0x00590c54
int g_frameColourGainRed;
// GLOBAL: CMR2 0x00590c58
int g_frameColourGainGreen;
// GLOBAL: CMR2 0x00590c5c
int g_frameColourGainBlue;
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
extern FixVector g_collisionPush;
extern FixVector g_collisionPushB;
void Collision_SplitBoxSeparationMovement(CollisionBox *pA, CollisionBox *pB, FixVector *pDir, int amount, int scale);
int Collision_FindQuadEdgeOverlap(FixVector *pVertsA, FixVector *pVertsB, FixVector *pDir, int *pDistance);
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
extern double g_fixedDegreesToAngle12;
extern Car *g_collisionCar;
extern CollisionBox *g_collisionFace;
void StageObject_ApplyRecursiveFrameDelta(BYTE index, char other, int *pDelta, int flag);
void CarPhysics_ApplyImpulse(FixVector *pImpulse, FixVector *pPoint, int usePoint);
// ---- GLOBALS nuevos (definir UNA sola vez en el proyecto) ----
// GLOBAL: CMR2 0x005918e0
FixVector g_unk0x005918e0;
// GLOBAL: CMR2 0x005918f0
FixVector g_unk0x005918f0;
// GLOBAL: CMR2 0x00591900
FixVector g_unk0x00591900;
// GLOBAL: CMR2 0x0059190c
FinRecord *g_pCollisionFinRecord;
// GLOBAL: CMR2 0x00591920
FixVector g_unk0x00591920;
// GLOBAL: CMR2 0x00591930
int g_unk0x00591930;
// GLOBAL: CMR2 0x00591938
FixVector g_unk0x00591938;
// GLOBAL: CMR2 0x00591950
FixVector g_unk0x00591950;
// GLOBAL: CMR2 0x00591968
FixVector g_unk0x00591968;
// GLOBAL: CMR2 0x00591978
FixVector g_unk0x00591978;
// GLOBAL: CMR2 0x00591990
FixVector g_unk0x00591990;
// GLOBAL: CMR2 0x0059199c
BYTE g_unk0x0059199c;
// GLOBAL: CMR2 0x005919a0
int g_unk0x005919a0;
// GLOBAL: CMR2 0x00591ad0
FixVector g_unk0x00591ad0;
// GLOBAL: CMR2 0x0051fb00
// Twenty-six restitution coefficients followed by twenty-six friction
// coefficients (the original's second lookup starts at 0x51fb68).
int g_unk0x0051fb00[52] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x4ccc, 0, 0, 0, 0, 0, 0, 0, 0,
    0x28f, 0x28f, 0x28f, 0x28f, 0x28f, 0x28f, 0x28f, 0x28f,
    0x28f, 0x28f, 0x28f, 0x28f, 0xa3d, 0xccc, 0x51e, 0xccc,
    0x28f, 0x7ae, 0xb85, 0x51e, 0x51e, 0x28f, 0x28f, 0x28f,
    0x28f, 0xccc
};
// ---- DECLS extras (integrar al principio de StageObjects.cpp si no existen ya) ----
void Race_PlayScrapeAndShakeCar(int view, int strength, int listener);
void StageObject_IntegrateViewDeformationGrid(int param_1, ViewWeatherState *param_2, int param_3);
int NetRace_GetListenerDistanceAttenuation(unsigned int view, int listener);
bool NetRace_IsValueWithinCurveRange(int value, SoundCurve *pRange);
unsigned int NetRace_InterpolateWordCurve(int value, SoundCurve *pCurve);
// unsigned short, no unsigned int: el original devuelve 16 bits (sus llamadores hacen 'and eax,0xffff').
// Corregido por la auditoria de W165 en NetRace.cpp; la declaracion tiene que seguir el mismo contrato.
unsigned short NetRace_ScaleViewAngleByDistance(int param_1, int param_2, unsigned short param_3);
int Sound_IsPlaying(unsigned int handle);
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume);
void Sound_SetPan(unsigned int handle, int pan);
void CarPart_ResolveLocalPointGroundContact(unsigned int param_1);
struct SoundCurve;
extern struct SoundCurve g_engineWhineSoundCurve;
extern CameraRecord g_viewStates[2];
extern int g_unk0x0058ddc8Pair[2]; // two sound handles (0x58ddc8, 0x58ddcc)
// ---- GLOBALS nuevos ----
// First engine sample slot of each loaded car sound set.
// GLOBAL: CMR2 0x0058ddb4
int g_unk0x0058ddb4[2];
// GLOBAL: CMR2 0x0051f27c
int g_unk0x0051f27c = 0x10000;
// Loop start of the second gear-whine sample, per car (one entry per
// CFrontend::GetArchivePrimaryIDEntry value; the Escort Mk1 uses the last).
// GLOBAL: CMR2 0x0051f2d8
int g_unk0x0051f2d8[14] = {
    0x486a, 0x4ec0, 0xf6e0, 0x715a, 0x715a, 0x590e, 0x590e,
    0x5126, 0x3366, 0x0b42, 0x10f36, 0x4388, 0x280a, 0xb17a
};
// Advances one stage-object record between two animation states: on entering the
// 1/2 states it re-derives the scaled component and notifies the timing module;
// on returning to idle it clears the derived components.
// FUNCTION: CMR2 0x00460b60
void StageObject_AdvanceAnimatedRecordState(ViewWeatherState *p, int unused)
{
    int next = p->targetKind;
    int state = p->kind;
    if (next == state)
        return;
    if (state == 0) {
        if (next != 1 && next != 2)
            return;
        p->particleCount = 0;
        p->targetParticleCount = FixMul(FixMul(g_weatherParticleQualityScale, p->intensity), p->particleCapacity << 16);
        p->kind = p->targetKind;
        StageObject_IntegrateViewDeformationGrid(0, p, unused);
        return;
    }
    if ((state == 1 || state == 2) && next == 0) {
        p->targetParticleCount = 0;
        if (p->particleCount == 0)
            p->kind = 0;
    }
}
// Walks a short list of scene objects backwards and, for each of their eight
// corner records that is enabled, has an eligible surface under it and moves
// above 0x1999, rebuilds the orthonormal contact frame around the corner (or
// drops the hit point half a body below the car for the corners without a
// wheel) and throws debris from it; each object ends with its impact
// bookkeeping.
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00469e40
void StageObject_UpdateEnabledCornerContactFrames(Car *param_1, short *param_2, short param_3)
{
    int count;
    Car *pCar;
    int idx;
    int size;
    int dot;
    FixVector axes[3];
    FixVector vHit;
    FixVector vTmp;
    for (count = param_3 - 1; count >= 0; count--) {
        pCar = param_1 + param_2[count];
        for (idx = 0; idx < 8; idx++) {
            if (pCar->cornerFlags[idx] != 0)
                continue;
            if (StageObject_IsEligibleType(pCar->wheelSurface[idx],
                                           pCar->field_0xb74, (int)pCar->surfaceDragLevel) == 0)
                continue;
            if (pCar->speed <= 0x1999)
                continue;
            // Impact strength: the wheel travel of the corners that own a
            // wheel, the object speed for the rest.
            if (idx < 4 && pCar->field_0xb74 != 0) {
                size = (pCar->wheelSlip[idx] < 0 ? -pCar->wheelSlip[idx] : pCar->wheelSlip[idx]) +
                       (pCar->wheelSlipLateral[idx] < 0 ? -pCar->wheelSlipLateral[idx] : pCar->wheelSlipLateral[idx]);
                size = FixMul(size, 0x10000);
                if (size > 0x10000)
                    size = 0x10000;
            } else {
                size = pCar->speed;
            }
            if (size <= 0x4ccc)
                continue;
            // Contact frame: axis 0 is the velocity put square to the corner
            // axis, axis 2 the cross of the two and axis 1 the cross of the
            // others.
            FixVecScale(&axes[0], &pCar->velocity, -0x10000);
            dot = FixVecDot(&axes[0], &pCar->cornerAxis[idx]);
            FixVecScale(&vTmp, &pCar->cornerAxis[idx], dot);
            axes[0].x -= vTmp.x;
            axes[0].y -= vTmp.y;
            axes[0].z -= vTmp.z;
            StageObj_NormalizeInto(&axes[0], &axes[0]);
            FixVecCross(&axes[2], &axes[0], &pCar->cornerAxis[idx]);
            StageObj_NormalizeInto(&axes[2], &axes[2]);
            FixVecCross(&axes[1], &axes[2], &axes[0]);
            StageObj_NormalizeInto(&axes[1], &axes[1]);
            if (idx < 4 && pCar->field_0xb74 != 0) {
                vTmp = pCar->wheelEmitter[idx];
                vTmp.y -= 0x5999;
                FixMatrix_RotateVector(&vHit, &vTmp, pCar->pWorld);
            } else {
                vHit.x = pCar->pWorld->right.y;
                vHit.y = pCar->pWorld->up.y;
                vHit.z = pCar->pWorld->forward.y;
                FixVecScale(&vHit, &vHit, -0x8000);
            }
            if (idx < 4 && pCar->field_0xb74 != 0)
                Car_SpawnDebris(size, &vHit, pCar, axes, 0, 0);
            else
                Car_SpawnDebris(size, &vHit, pCar, axes, 0x90000, 0x9999);
        }
        CarDamage_UpdateSuspensionImpactContacts(pCar);
    }
}

// Advances a type 2 replay stream: on every third frame it moves to the next
// sample (snapping the pose pair and fetching the next one), then interpolates
// the car's matrix, heading, steering and velocity between the two samples.
// match 91%: the original homes the car pointer in a local slot and uses the
// dead parameter slot for the FixMul operands.
// FUNCTION: CMR2 0x0046d610
void Replay_InterpolatePoseStream(ReplayStream *p)
{
    Car *pCar;
    int t;
    int value;

    if (p == NULL)
        return;
    pCar = Car_Get(p->car);
    if (p->playing == 0 || p->type != 2 || pCar->simulationStepsRemaining <= 0u)
        return;
    if (p->step >= 3) {
        p->pose.from = p->pose.to;
        p->headingFrom = p->headingTo;
        p->steerFrom = p->steerTo;
        p->eventPrev = p->event;
        p->flagPrev = p->flag;
        if (p->event > 0)
            Race_PlayScrapeAndShakeCar(pCar->index, p->event, pCar->index);
        p->frame++;
        if (p->frame < p->pLaneSamples[p->lane])
            Replay_DecodeCarPoseSample(&p->field_0xec, &p->flag, (unsigned int *)&p->steerTo, &p->headingTo, &p->pose.to,
                         &p->event, &p->field_0xe8,
                         (ReplaySample *)p->pSamples + p->samplesPerLane * p->lane + p->frame);
        else
            Replay_ResetBufferIfActive(p);
        Replay_ComputePoseVelocityCorrection(&p->pose, pCar);
        p->step = 0;
        if (p->flagPrev != 0)
            pCar->field_0xbf8 = 1;
    }
    if (p->flag != 0)
        t = 0;
    else
        t = FixDiv(p->step << 16, 0x30000);
    FixMatrix_Interpolate(pCar->pWorld, &p->pose.from, &p->pose.to, t, t, t, 1);
    value = FixMul(FixMul(p->headingTo - p->headingFrom, t) + p->headingFrom, pCar->maxSteeringAngleDegrees * 0x1680);
    pCar->engineSpeed = 0;
    pCar->wheelSteeringAngle = (unsigned short)FTOI(__int64, X87S_MUL((double)value, g_fixedDegreesToAngle12));
    pCar->throttleTorque = FixMul(pCar->maxThrottleTorque, FixMul(p->steerTo - p->steerFrom, t) + p->steerFrom);
    pCar->velocityNext = pCar->velocity;
    pCar->velocity.x += p->pose.velocity.x;
    pCar->velocity.y += p->pose.velocity.y;
    pCar->velocity.z += p->pose.velocity.z;
    *(unsigned int *)&pCar->braking = p->field_0xec;
    p->step++;
}
// Dispatches one stage object's per-frame update when its stage-block slot is
// active, refreshing the car's order, light and mesh state.
// FUNCTION: CMR2 0x004765e0
void StageObject_DispatchActiveCarObjectUpdate(CameraRecord *pObj, FixMatrix *pRef, int b)
{
    if (g_carInteriorLoaded[pObj->car] != 0) {
        CarInterior_IntegrateDriverPose(pObj, pRef, b);
        StageObject_BlendCarMountTransform(pObj->car);
        CarInterior_AnimateWipers(pObj->car);
        StageObject_UpdateRevCounterTextures(pObj->car);
        StageObject_BuildCarMountWorldMatrix(pObj, pRef);
    }
}
// Picks the nearest stage cars to the player's view (up to two) and starts or
// updates their engine sounds, then adjusts pan and volume from the distance.
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0047aa70
void SurfaceSound_UpdateNearestLocalCarEngines(void)
{
    int carIds[8];
    int distances[5];
    int used[5];
    FixMatrix rot;
    FixVector pos;
    FixVector viewPos;
    FixVector delta;
    int i;
    int count;
    int slots;
    int limit;
    int *pHandle;
    unsigned int chosen;
    int bestDist;
    int dist;
    int scale;
    int pitch;
    slots = 2;
    count = (int)RallyDataState();
    count = (int)Car_GetOrderCount() - count;
    if (count == 0)
        return;
    if (count < slots)
        slots = count;
    if (0 < count) {
        for (i = 0; i < count; i++)
            used[i] = 1;
        for (i = 0; i < count; i++) {
            Car *pCar = Car_Get((int)(RallyDataState() & 0xff) + i);
            FixMatrix_CopyRotationFrom(&rot, pCar->pWorld);
            FixMatrix_GetPosition(&pos, &rot);
            FixMatrix_GetPosition(&viewPos, &g_viewStates[0].matrix);
            delta.x = pos.x - viewPos.x;
            delta.y = pos.y - viewPos.y;
            delta.z = pos.z - viewPos.z;
            distances[i] = (int)FixVec_Length(&delta);
        }
    }
    if (slots <= 0)
        return;
    limit = slots;
    pHandle = g_unk0x0058ddc8Pair;
    do {
        chosen = 0;
        bestDist = 0x42400000;
        if (0 < count) {
            for (i = 0; i < count; i++) {
                if (distances[i] < bestDist && used[i] != 0) {
                    bestDist = distances[i];
                    chosen = (unsigned int)(i + 1);
                }
            }
        }
        used[chosen - 1] = 0;
        dist = NetRace_GetListenerDistanceAttenuation(chosen, 0);
        dist = FixMul(0x10000, dist);
        {
            Car *pCar = Car_Get(chosen);
            int car7ac = pCar->field_0x7ac;
            scale = FixMul(car7ac, pCar->field_0x798);
            pitch = FixMulShift32(scale, 0x19640000);
            if (pitch < 2000)
                pitch = 2000;
            else if (pitch > 0x1964)
                pitch = 0x1964;
        }
        if (Sound_IsPlaying((unsigned int)*pHandle) == 0) {
            int volScale = FixMul(g_unk0x0058dda8, FixMul(dist, g_unk0x0051f27c));
            int dist2 = NetRace_GetListenerDistanceAttenuation(chosen, 0);
            volScale = FixMul(dist2, volScale);
            *pHandle = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[0] + 6),
                                    volScale, 0x5622,
                                    g_unk0x0051f2d8[CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue(0))], 1, 0);
        }
        if (NetRace_IsValueWithinCurveRange(pitch, &g_engineWhineSoundCurve)) {
            unsigned int pan = NetRace_InterpolateWordCurve(pitch, &g_engineWhineSoundCurve);
            unsigned short pan2 = (unsigned short)NetRace_ScaleViewAngleByDistance(0, (int)chosen, (unsigned short)pan);
            int volScale;
            int dist2;
            Sound_SetPan((unsigned int)*pHandle, pan2);
            volScale = FixMul(g_unk0x0058dda8, FixMul(dist, g_unk0x0051f27c));
            dist2 = NetRace_GetListenerDistanceAttenuation(chosen, 0);
            Sound_SetPlayingSlotVolume((unsigned int)*pHandle, FixMul(dist2, volScale));
        } else {
            Sound_SetPlayingSlotVolume((unsigned int)*pHandle, 0);
        }
        pHandle++;
    } while (--limit);
}
// Same as SurfaceSound_UpdateNearestLocalCarEngines for the network race: gathers the active player cars by
// their network index instead of the local order list.
// match 38%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0047ad20
void SurfaceSound_UpdateNearestNetworkCarEngines(void)
{
    int carIds[8];
    int distances[8];
    int used[8];
    FixMatrix rot;
    FixVector pos;
    FixVector viewPos;
    FixVector delta;
    int i;
    int n;
    int count;
    int slots;
    int limit;
    int *pHandle;
    unsigned int chosen;
    int bestDist;
    int dist;
    int scale;
    int pitch;
    count = 0;
    for (i = 0; i < 7; i++) {
        if ((BYTE)NetPlayers_IsPlayerPresent(i) != 0)
            carIds[count++] = NetPlayers_GetPlayerField8(i);
    }
    if (count == 0)
        return;
    slots = 2;
    if (count < 2)
        slots = count;
    for (i = 0; i < 8; i++)
        used[i] = 0;
    for (i = 0; i < count; i++)
        used[i] = 1;
    for (i = 0; i < count; i++) {
        Car *pCar = Car_Get(carIds[i]);
        FixMatrix_CopyRotationFrom(&rot, pCar->pWorld);
        FixMatrix_GetPosition(&pos, &rot);
        FixMatrix_GetPosition(&viewPos, &g_viewStates[0].matrix);
        delta.x = pos.x - viewPos.x;
        delta.y = pos.y - viewPos.y;
        delta.z = pos.z - viewPos.z;
        distances[i] = (int)FixVec_Length(&delta);
    }
    if (slots <= 0)
        return;
    pHandle = g_unk0x0058ddc8Pair;
    limit = slots;
    do {
        chosen = 0;
        bestDist = 0x42400000;
        for (i = 0; i < count; i++) {
            if (i == 0) {
                bestDist = distances[0];
                chosen = (unsigned int)carIds[0];
            }
            if (distances[i] < bestDist && used[i] != 0) {
                chosen = (unsigned int)carIds[i];
                bestDist = distances[i];
            }
        }
        used[chosen] = 0;
        dist = NetRace_GetListenerDistanceAttenuation(chosen, 0);
        dist = FixMul(0x10000, dist);
        {
            Car *pCar = Car_Get(chosen);
            int car798 = pCar->field_0x798;
            scale = FixMul(car798, pCar->field_0x7ac);
            pitch = FixMulShift32(scale, 0x19640000);
            if (pitch < 2000)
                pitch = 2000;
            else if (pitch > 0x1964)
                pitch = 0x1964;
        }
        if (Sound_IsPlaying((unsigned int)*pHandle) == 0) {
            int volScale = FixMul(g_unk0x0058dda8, FixMul(dist, g_unk0x0051f27c));
            int dist2 = NetRace_GetListenerDistanceAttenuation(chosen, 0);
            volScale = FixMul(dist2, volScale);
            *pHandle = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[0] + 6),
                                    volScale, 0x5622,
                                    g_unk0x0051f2d8[CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue(0))], 1, 0);
        }
        if (NetRace_IsValueWithinCurveRange(pitch, &g_engineWhineSoundCurve)) {
            unsigned int pan = NetRace_InterpolateWordCurve(pitch, &g_engineWhineSoundCurve);
            unsigned short pan2 = (unsigned short)NetRace_ScaleViewAngleByDistance(0, (int)chosen, (unsigned short)pan);
            int volScale;
            int dist2;
            Sound_SetPan((unsigned int)*pHandle, pan2);
            volScale = FixMul(g_unk0x0058dda8, FixMul(dist, g_unk0x0051f27c));
            dist2 = NetRace_GetListenerDistanceAttenuation(chosen, 0);
            Sound_SetPlayingSlotVolume((unsigned int)*pHandle, FixMul(dist2, volScale));
        } else {
            Sound_SetPlayingSlotVolume((unsigned int)*pHandle, 0);
        }
        pHandle++;
    } while (--limit);
}
// PORT: a thumbstick's travel (eased steering, a gamepad's, steeringInputMode
// 1) spread over the lock by speed, so that each part of the travel serves an
// equal share of the steering drivers do at that speed, while its end still
// gives full lock, as a key held long enough does. Fitted on the 24 recorded
// runs of the super specials (three difficulties, both sides, 74,000 steps
// driven with keys): the response x + (x^2 - x)k matches the quantiles of the
// lock they used with k = 0 at a speed of 0.25, 0.5 at 0.55, then 0.38 more
// per unit of speed up to 1 (a square from 1.85). At speed 1.25, 60% of the
// travel gives 41% of the lock instead of 60%.
static int StageObject_ShapeThumbstickSteering(int value, int speed)
{
    int blend;

    if (value > 0x10000)
        value = 0x10000;
    if (speed <= 0x4000)
        blend = 0;
    else if (speed <= 0x8ccc)
        blend = FixMul(speed - 0x4000, 0x1aaaa);
    else
        blend = 0x8000 + FixMul(speed - 0x8ccc, 0x6148);
    if (blend > 0x10000)
        blend = 0x10000;
    return value + FixMul(FixMul(value, value) - value, blend);
}

// Refreshes the on-screen control indicators of the active car (steer, throttle,
// brake, handbrake) from the input device driving a player: analogue axes scale
// the indicators, digital buttons light them full.
// FUNCTION: CMR2 0x0047b0e0
void StageObject_UpdatePlayerControlIndicators(int player, int device)
{
    DeviceInfo *pDev;
    DeviceInfo *pSrc;
    BYTE *p;
    int axisSteer;
    int axisThrottle;
    int axisBrake;
    int idx;
    int raw;
    int half;
    int *pAxis;
    char c;
    if (CGameInfo::IsInRaceMenuOpen())
        return;
    if (CGameInfo::GetGameModeOptionBit19()) {
        if (player > 0)
            return;
    } else if (player >= (int)(RallyDataState() & 0xff)) {
        return;
    }
    StageObject_ReadCarControllerMapping(player);
    pDev = CInput::GetAvailableDeviceRecord(device);
    if (pDev->type != 1) {
        pSrc = CInput::GetAvailableDeviceRecord(player);
        pDev->held |= pSrc->held;
        pDev->pressed |= pSrc->pressed;
        pDev->repeated |= pSrc->repeated;
    }
    p = StageUI_GetRaceResultTable();
    if ((char)((CallbackStateMachine *)p)->records[player].packedState != '\n' || GameMenu_IsPauseHeaderActive() != 0) {
        p = StageUI_GetRaceResultTable();
        if ((char)((CallbackStateMachine *)p)->records[player].packedState == '\n') {
            idx = Race_IsMultiplayerRecordMode10() ? 1 : 0;
            if (View_GetActiveCameraMode((BYTE)idx) != 10 && View_GetActiveCameraMode((BYTE)idx) != 7) {
                if ((pDev->pressed & g_carButtonMasks[7]) != 0)
                    View_UpdateDriverCameraCycle(player);
            }
        } else {
            if (View_GetActiveCameraMode((BYTE)player) != 10 && View_GetActiveCameraMode((BYTE)player) != 7) {
                if ((pDev->pressed & g_carButtonMasks[7]) != 0)
                    View_UpdateDriverCameraCycle(player);
            }
        }
    }
    if (Race_GetPlayerRecordField4((BYTE)player) == 0) {
        if ((pDev->pressed & g_carButtonMasks[7]) == 0 && View_GetActiveCameraMode((BYTE)player) != 8)
            StageObject_CycleDriverCameraSelection(pDev->held, player);
        if (pDev->type == 3 || pDev->type == 2) {
            axisSteer = g_carAxisInputEnabled[0] ? (int)CInput::GetEnabledControllerAxisBinding(player, 0) : -1;
            if (g_carAxisInputEnabled[1]) {
                axisThrottle = (int)CInput::GetEnabledControllerAxisBinding(player, 2);
                axisBrake = (int)CInput::GetEnabledControllerAxisBinding(player, 3);
            } else {
                axisThrottle = -1;
                axisBrake = -1;
            }
            if (axisSteer != -1) {
                if (CInput::GetControllerSmoothSteering((short)g_controlCar->index) != 0)
                    g_controlCar->steeringInputMode = 1;
                else
                    g_controlCar->steeringInputMode = 2;
                pAxis = &((int *)pDev)[axisSteer * 5 + 0x11f];
                half = *pAxis < 0 ? -*pAxis : *pAxis;
                if (g_controlCar->steeringInputMode == 1)   // PORT
                    half = StageObject_ShapeThumbstickSteering(half, g_controlCar->speed);
                half = FixMul(half, 0x3f0000) >> 16;
                if (*pAxis < 0)
                    g_controlCar->flag0x1d0[0] = (char)half;
                else
                    g_controlCar->flag0x1d0[1] = (char)half;
            } else {
                // PORT: steering on buttons with the pedals on axes (a D-pad
                // and triggers) steers as keys do, not as a stick pushed to
                // its end, which a direct steering mode follows at once.
                g_controlCar->steeringInputMode = 0;
                if ((pDev->held & g_carButtonMasks[0]) != 0)
                    g_controlCar->flag0x1d0[0] = 0x3f;
                else if ((pDev->held & g_carButtonMasks[1]) != 0)
                    g_controlCar->flag0x1d0[1] = 0x3f;
            }
            if (axisThrottle != -1) {
                g_controlCar->analogThrottle = 1;
                pAxis = &((int *)pDev)[axisThrottle * 5 + 0x11f];
                half = *pAxis < 0 ? -*pAxis : *pAxis;
                half = FixMul(half, 0x3f0000) >> 16;
                raw = *pAxis;
                if ((raw <= 0 && axisThrottle == axisBrake && CInput::GetControllerThrottleOnPositive(device) == 0) ||
                    (raw > 0 && axisThrottle == axisBrake && CInput::GetControllerThrottleOnPositive(device) != 0)) {
                    g_controlCar->flag0x1d0[2] = (char)half;
                } else if (axisThrottle == axisBrake) {
                    g_controlCar->analogBrake = 1;
                    g_controlCar->flag0x1d0[3] = (char)half;
                } else {
                    half = raw / 2;
                    idx = half + 0x8000;
                    if (idx < 0)
                        idx = -0x8000 - half;
                    g_controlCar->flag0x1d0[2] = (char)(0x3f - (FixMul(idx, 0x3f0000) >> 16));
                }
            } else {
                g_controlCar->analogThrottle = 0;
                if ((pDev->held & g_carButtonMasks[2]) != 0)
                    g_controlCar->flag0x1d0[2] = 0x3f;
            }
            if (axisBrake != -1) {
                if (axisBrake != axisThrottle) {
                    g_controlCar->analogBrake = 1;
                    raw = ((int *)pDev)[axisBrake * 5 + 0x11f];
                    half = raw / 2;
                    idx = half + 0x8000;
                    if (idx < 0)
                        idx = -0x8000 - half;
                    c = (char)(0x3f - FixMulShift32(idx, 0x3f0000));
                    if (c != 0)
                        g_controlCar->flag0x1d0[3] = c;
                }
            } else {
                g_controlCar->analogBrake = 0;
                if ((pDev->held & g_carButtonMasks[3]) != 0)
                    g_controlCar->flag0x1d0[3] = 0x3f;
            }
        } else {
            g_controlCar->analogThrottle = 0;
            g_controlCar->analogBrake = 0;
            g_controlCar->steeringInputMode = 0;
            if ((pDev->held & g_carButtonMasks[2]) != 0)
                g_controlCar->flag0x1d0[2] = 0x3f;
            if ((pDev->held & g_carButtonMasks[3]) != 0)
                g_controlCar->flag0x1d0[3] = 0x3f;
            if ((pDev->held & g_carButtonMasks[0]) != 0)
                g_controlCar->flag0x1d0[0] = 0x3f;
            else if ((pDev->held & g_carButtonMasks[1]) != 0)
                g_controlCar->flag0x1d0[1] = 0x3f;
        }
        if ((pDev->held & g_carButtonMasks[4]) != 0)
            g_controlCar->handbrake = 1;
        if (g_controlCar->automaticGearbox != 0) {
            if ((pDev->pressed & g_carButtonMasks[5]) != 0)
                g_controlCar->field_0x1d4[0] = 1;
            if ((pDev->pressed & g_carButtonMasks[6]) != 0)
                g_controlCar->field_0x1d4[0] = 0xff;
        }
    }
    if (CGameInfo::IsActiveCheatEnabled(2) != 0) {
        c = g_controlCar->flag0x1d0[0];
        g_controlCar->flag0x1d0[0] = g_controlCar->flag0x1d0[1];
        g_controlCar->flag0x1d0[1] = c;
    }
}
// Tests every active headlight glow record of the other cars against the
// oriented bounding box of this car (extents along the two box axes and the
// world y range), and for the ones found inside it builds the impact direction
// from the glow position, accumulates the impulse in the deformation vector and
// torque of the car and applies the mode 2 collision deformation.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0047d850
void StageObject_TestHeadlightGlowsAgainstCarBox(Car *pCar, CollisionBox *pBox)
{
    FixVector offset;
    FixVector dir;
    FixVector localDelta;
    FixVector localDirection;
    FixVector cross;
    FixVector *pAnchor;
    int *pRec;
    int i;
    int dot1;
    int dot2;
    pRec = (int *)g_carHeadlightGlowRecords;
#define GLOW_RECORD (*(CarHeadlightGlowRecord *)pRec)
    i = 100;
    do {
        if (GLOW_RECORD.active != 0 && GLOW_RECORD.car != pCar->index) {
            pAnchor = pBox->pVertex;
            offset.x = GLOW_RECORD.position.x - pAnchor->x;
            offset.y = GLOW_RECORD.position.y - pAnchor->y;
            offset.z = GLOW_RECORD.position.z - pAnchor->z;
            if (FIX_ABS(offset.x) < 0x640000) {
                if (FIX_ABS(offset.y) < 0x640000) {
                    if (FIX_ABS(offset.z) < 0x640000 && GLOW_RECORD.position.y >= pBox->bottom &&
                        GLOW_RECORD.position.y <= pBox->top) {
                        dot1 = FixVecDot(&offset, &pBox->axisA);
                        dot2 = FixVecDot(&offset, &pBox->axisB);
                        if (FIX_ABS(dot1) <= pBox->halfWidth && FIX_ABS(dot2) <= pBox->halfLength) {
                            FixVecScale(&dir, &GLOW_RECORD.velocity, 0x20000);
                            FixVecScaleRecip(&dir, &dir, FixVecLength(&dir));
                            dir.y += 0x6666;
                            FixVecScale(&dir, &dir, 0x60000);
                            offset.y -= 0x38000;
                            pCar->velocity.y += 0x4000;
                            FixMatrix_InverseRotateVector(&localDelta, &offset, pCar->pWorld);
                            FixMatrix_InverseRotateVector(&localDirection, &dir, pCar->pWorld);
                            FixVecCross(&cross, &localDirection, &localDelta);
                            pCar->field_0x5c4.x += dir.x;
                            pCar->field_0x5c4.y += dir.y;
                            pCar->field_0x5c4.z += dir.z;
                            pCar->field_0x5d0.x += cross.x;
                            pCar->field_0x5d0.y += cross.y;
                            pCar->field_0x5d0.z += cross.z;
                            pCar->useUpperCollisionCorners = 1;
                            pCar->field_0x96c = 0x10000;
                            FIX_NORMALIZE_INTO(dir, dir);
                            CarDamage_ApplyCollisionDeformImpulse(pCar, (int *)&GLOW_RECORD.position, &dir, 0, 2, 0);
                            GLOW_RECORD.active = 0;
                            GLOW_RECORD.life = 0;
                        }
                    }
                }
            }
        }
        pRec += sizeof(CarHeadlightGlowRecord) / sizeof(*pRec);
        i--;
    } while (i != 0);
#undef GLOW_RECORD
}
// Stops the stage-object records of every climbing car: cars resting on a very
// steep ground normal get the ground-aligned step, the rest the free step.
// match 42%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00484e00
void CarDamage_StepClimbingCarRecords(short *param_1, short param_2)
{
    FixVector impulse;
    short *pIndex;
    int n;

    StageObject_DeriveLoadedScaleVector(&impulse);
    n = param_2 - 1;
    if (n < 0)
        return;
    pIndex = param_1 + n;
    n++;
    do {
        int idx = *pIndex;
        int m;
        g_partCar = (Car *)Car_Get(idx);
        g_partSet = CarPart_GetSet(idx);
        if (g_partCar->isGhost == 0) {
            if (g_partCar->field_0xb64 == 0 &&
                g_partCar->useUpperCollisionCorners != 0 &&
                FixVecDot(&g_partCar->groundNormal,
                          &g_partCar->up) < -0xcccc &&
                (g_partCar->cornerFlags[5] == 0 ||
                 g_partCar->cornerFlags[4] == 0 ||
                 g_partCar->cornerFlags[7] == 0 ||
                 g_partCar->cornerFlags[6] == 0)) {
                m = *g_carLineCounts[g_partCar->index] - 1;
                for (; m >= 0; m--)
                    CarPart_ResolveLocalPointGroundContact(m);
            } else {
                StageObject_ApplyRandomizedBodyImpulse(&impulse);
                m = *g_carLineCounts[g_partCar->index] - 1;
                for (; m >= 0; m--)
                    StageObject_IntegrateCarMotionRecord(m);
            }
        }
        pIndex--;
    } while (--n);
}

int View_IsNegativeRightAngleCameraSpot(int view);
void StageObject_DrawObjectDustTrail(unsigned int index, int *pTarget, int flag);
extern int g_frameColourGainGreen;
extern int g_frameColourGainBlue;

// Draws the light beams of every car in the order list for one view (not the
// player's own car from the in-car cameras): the beam colour follows the
// scene light, and each beam is flagged when the car's light node is lit for
// this view.
// FUNCTION: CMR2 0x00485690
void StageObject_DrawListedCarLightBeams(short *pOrder, short count, int view)
{
    FixVector viewPos;
    BYTE colour[4];
    int self;
    int inCar;
    int i;
    int n;
    int car;
    int lit;
    BYTE mask;

    FixMatrix_GetPosition(&viewPos, &g_viewStates[view].matrix);
    inCar = View_IsNegativeRightAngleCameraSpot(view);
    if (inCar == 0 && View_GetActiveCameraMode((BYTE)view) == 10)
        inCar = 1;
    self = View_GetActiveCameraFlags((BYTE)view);
    for (i = count - 1; i >= 0; i--) {
        car = pOrder[i];
        g_partCar = (Car *)Car_Get(car);
        if (g_partCar->isGhost != 0 || (inCar != 0 && self == car) ||
            (&g_partCar->firstCameraBlocked)[view] != 0)
            continue;
        Scene_GetLightColour((DWORD *)colour, g_partCar->field_0xa70);
        g_frameColourGainRed = FixMul(colour[0] << 16, 0x106);
        if (g_frameColourGainRed > 0x10000)
            g_frameColourGainRed = 0x10000;
        g_frameColourGainGreen = FixMul(colour[1] << 16, 0x106);
        if (g_frameColourGainGreen > 0x10000)
            g_frameColourGainGreen = 0x10000;
        g_frameColourGainBlue = FixMul(colour[2] << 16, 0x106);
        if (g_frameColourGainBlue > 0x10000)
            g_frameColourGainBlue = 0x10000;
        mask = (BYTE)(1 << view);
        lit = 0;
        if (g_partCar->pAlternateBodyNode == NULL) {
            if (g_partCar->pWheelNodes[0]->viewMask & mask)
                lit = 1;
        } else if (g_partCar->pAlternateBodyNode->viewMask != 0) {
            if (mask & g_partCar->pAlternateBodyNode->viewMask)
                lit = 1;
        } else if (g_partCar->pWheelNodes[0]->viewMask & mask) {
            lit = 1;
        }
        for (n = *g_carLineCounts[g_partCar->index] - 1; n >= 0; n--)
            StageObject_DrawObjectDustTrail(n, (int *)&viewPos, lit);
    }
}

// Draws a flexible line from a car model. Attached lines use the body world
// matrix and interpolated deflection to draw five curved segments. Grounded
// lines use their stored world endpoints. Both paths fade with distance.
// The historical function name is retained while its callers are recovered.
// match 52%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00485860
void StageObject_DrawObjectDustTrail(unsigned int index, int *pTarget, int flag)
{
    CarFlexibleLineDescriptor *pDesc;
    CarFlexibleLineState *pEntry;
    FixVector *pAxis;
    FixMatrix *pMatrix;
    BYTE colour[4];
    FixVector base;
    FixVector end;
    FixVector delta;
    FixVector matrixPos;
    FixVector dir;
    FixVector side;
    FixVector back;
    FixVector right;
    FixVector axis;
    FixVector start;
    char car;
    int fade;
    int len;
    int i;
    int s;
    car = g_partCar->index;
    pEntry = g_carLineStates[car] + (index & 0xff);
    pDesc = g_carLineDescriptors[car] + (index & 0xff);
    colour[0] = (BYTE)FixMulShift32(g_frameColourGainRed, pDesc->colour[0] << 16);
    colour[1] = (BYTE)FixMulShift32(g_frameColourGainGreen, pDesc->colour[1] << 16);
    colour[2] = (BYTE)FixMulShift32(g_frameColourGainBlue, pDesc->colour[2] << 16);
    colour[3] = pDesc->colour[3];
    if (pEntry->grounded != 0) {
        // Flag set: one straight segment from +0x0 to +0x9, each end nudged at
        // most one unit towards the target.
        delta.x = pTarget[0] - pEntry->position.x;
        delta.y = pTarget[1] - pEntry->position.y;
        delta.z = pTarget[2] - pEntry->position.z;
        fade = StageObject_DistanceFade(&delta);
        if (fade <= 0)
            return;
        i = FixMulShift32(colour[3] << 16, fade);
        if (i > 0xff)
            i = 0xff;
        else if (i < 0)
            i = 0;
        colour[3] = (BYTE)i;
        delta.x = pTarget[0] - pEntry->position.x;
        delta.y = pTarget[1] - pEntry->position.y;
        delta.z = pTarget[2] - pEntry->position.z;
        len = FixVecLength(&delta);
        if (len > 0x10000)
            FixVecScale(&delta, &delta, FixDiv(0x10000, len));
        base.x = pEntry->position.x + delta.x;
        base.y = pEntry->position.y + delta.y;
        base.z = pEntry->position.z + delta.z;
        delta.x = pTarget[0] - pEntry->groundEnd.x;
        delta.y = pTarget[1] - pEntry->groundEnd.y;
        delta.z = pTarget[2] - pEntry->groundEnd.z;
        len = FixVecLength(&delta);
        if (len > 0x10000)
            FixVecScale(&delta, &delta, FixDiv(0x10000, len));
        end.x = pEntry->groundEnd.x + delta.x;
        end.y = pEntry->groundEnd.y + delta.y;
        end.z = pEntry->groundEnd.z + delta.z;
        Line2D_Queue((int *)&base, (int *)&end, colour, colour);
        return;
    }
    if (flag == 0)
        return;
    {
        // World matrix of the optional second body node, falling back to the
        // first body node when it is absent or hidden.
        SceneNode *pObj = g_partCar->pAlternateBodyNode;
        if (pObj == NULL) {
            pMatrix = &g_partCar->pBodyNode->world;
        } else {
            if (pObj->viewMask == 0)
                pObj = g_partCar->pBodyNode;
            pMatrix = &pObj->world;
        }
    }
    // The interpolated deflection determines this segment's direction.
    dir = pEntry->drawPosition;
    pAxis = &pDesc->axis;
    // Contact point: the record's local offset rotated into the matrix frame.
    FixMatrix_GetPosition(&matrixPos, pMatrix);
    FixMatrix_RotateVector(&base, &pDesc->position, pMatrix);
    base.x += matrixPos.x;
    base.y += matrixPos.y;
    base.z += matrixPos.z;
    delta.x = base.x - pTarget[0];
    delta.y = base.y - pTarget[1];
    delta.z = base.z - pTarget[2];
    fade = StageObject_DistanceFade(&delta);
    if (fade <= 0)
        return;
    i = FixMulShift32(colour[3] << 16, fade);
    if (i > 0xff)
        i = 0xff;
    else if (i < 0)
        i = 0;
    colour[3] = (BYTE)i;
    // Direction: smoothed by the physics step on x/z only, normalised and
    // scaled by the descriptor's length.
    dir.x = FixMul(dir.x, g_physicsTimeStep);
    dir.z = FixMul(dir.z, g_physicsTimeStep);
    len = FixVecLength(&dir);
    if (len == 0) {
        dir.x = 0;
        dir.y = 0;
        dir.z = 0;
    } else {
        FixVecScaleRecip(&dir, &dir, len);
    }
    FixVecScale(&dir, &dir, pDesc->length);
    // Sideways sweep: the record's axis in matrix space, scaled by 0.2 * dir.y.
    FixMatrix_RotateVector(&side, pAxis, pMatrix);
    FixVecScale(&side, &side, FixMul(dir.y, FixDiv(0x10000, 0x50000)));
    // Two vectors perpendicular to the axis (the axis' x and z rows of the
    // "rotate a unit vector to the axis" basis).
    if (pAxis->x == 0) {
        back.x = 0x10000;
        back.y = 0;
        back.z = 0;
    } else {
        delta.x = 0x10000;
        delta.y = 0;
        delta.z = 0;
        FixVecScale(&back, pAxis, pAxis->x);
        back.x = delta.x - back.x;
        back.y = delta.y - back.y;
        back.z = delta.z - back.z;
        len = FixVecLength(&back);
        if (len == 0) {
            back.x = 0;
            back.y = 0;
            back.z = 0;
        } else {
            FixVecScaleRecip(&back, &back, len);
        }
    }
    if (pAxis->z == 0) {
        right.x = 0;
        right.y = 0;
        right.z = 0x10000;
    } else {
        delta.x = 0;
        delta.y = 0;
        delta.z = 0x10000;
        FixVecScale(&right, pAxis, pAxis->z);
        right.x = delta.x - right.x;
        right.y = delta.y - right.y;
        right.z = delta.z - right.z;
        len = FixVecLength(&right);
        if (len == 0) {
            right.x = 0;
            right.y = 0;
            right.z = 0;
        } else {
            FixVecScaleRecip(&right, &right, len);
        }
    }
    // The curved direction is the two basis vectors weighted by the direction's
    // x and z, rotated back into world space.
    FixVecScale(&back, &back, dir.x);
    FixVecScale(&right, &right, dir.z);
    delta.x = right.x + back.x;
    delta.y = right.y + back.y;
    delta.z = right.z + back.z;
    FixMatrix_RotateVector(&axis, &delta, pMatrix);
    start = base;
    for (i = 1; i < 6; i++) {
        s = FixMul(FixDiv(0x10000, 0x50000), i << 16);
        s = FixMul(s, s);
        FixVecScale(&right, &axis, s);
        end.x = start.x + right.x;
        end.y = start.y + right.y;
        end.z = start.z + right.z;
        FixVecScale(&back, &side, i << 16);
        end.x += back.x;
        end.y += back.y;
        end.z += back.z;
        Line2D_Queue((int *)&base, (int *)&end, colour, colour);
        base = end;
    }

}
// Overlap test between two oriented 2D collision boxes: the four corners of each
// box are checked against the other box's half extents in that box's own frame,
// and the deepest push distance found along the direction from `pOffset` to box
// B is handed to the collision resolver. When no corner overlaps, the box edge
// (quad) test is tried as a fallback. Returns 1 when a contact was resolved.
// match 49%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00488640
int Collision_TestOrientedBoxCornerOverlap(CollisionBox *pBoxA, CollisionBox *pBoxB, FixVector *pOffset, int scale)
{
    FixVector dir;
    FixVector negDir;
    FixVector delta;
    FixVector corner;
    FixVector *posA;
    FixVector *posB;
    int projAxis0;
    int projAxis1;
    int projCorner0;
    int projCorner1;
    int t0;
    int t1;
    int best;
    int maxDist;
    int hit;
    int passHit;
    int hit0;
    int hit1;
    int length;
    int index;
    int i;
    if (pBoxA->top < pBoxB->bottom || pBoxB->top < pBoxA->bottom)
        return 0;
    g_unk0x005914d4 = 0;
    g_unk0x005915f4 = 0;
    g_collisionPush.x = 0;
    g_collisionPush.y = 0;
    g_collisionPush.z = 0;
    g_collisionPushB.x = 0;
    g_collisionPushB.y = 0;
    g_collisionPushB.z = 0;
    posA = pBoxA->pVertex;
    posB = pBoxB->pVertex;
    // Direction from the offset point to box B, flattened and normalised.
    delta.x = posB->x - pOffset->x;
    delta.y = posB->y - pOffset->y;
    delta.z = posB->z - pOffset->z;
    delta.y = 0;
    FIX_NORMALIZE_INTO(dir, delta);
    // Projections of box A's two axes on the direction.
    projAxis0 = FixVecDot(&pBoxA->axisA, &dir);
    projAxis1 = FixVecDot(&pBoxA->axisB, &dir);
    hit = 0;
    passHit = 0;
    maxDist = 0;
    // Pass 1: box B's corners tested in box A's frame.
    for (i = 0; i < 4; i++) {
        delta.x = pBoxB->points[i].x - pBoxA->pVertex->x;
        delta.y = pBoxB->points[i].y - pBoxA->pVertex->y;
        delta.z = pBoxB->points[i].z - pBoxA->pVertex->z;
        delta.y = 0;
        projCorner0 = FixVecDot(&pBoxA->axisA, &delta);
        projCorner1 = FixVecDot(&pBoxA->axisB, &delta);
        if (FIX_ABS(projCorner0) > pBoxA->halfWidth || FIX_ABS(projCorner1) > pBoxA->halfLength)
            continue;
        hit0 = 0;
        hit1 = 0;
        t0 = 0;
        t1 = 0;
        best = 0x7d000000;
        // Distance to the box A face the corner leaves through, per axis; an
        // axis whose direction projection is ~0 (|cos| <= 0x41) can not slide.
        if (projAxis0 > 0x41) {
            t0 = FixDiv(pBoxA->halfWidth - projCorner0, projAxis0);
            hit0 = 1;
        } else if (projAxis0 < -0x41) {
            t0 = FixDiv(FIX_ABS(pBoxA->halfWidth + projCorner0), -projAxis0);
            hit0 = 1;
        }
        if (projAxis1 > 0x41) {
            t1 = FixDiv(pBoxA->halfLength - projCorner1, projAxis1);
            hit1 = 1;
        } else if (projAxis1 < -0x41) {
            t1 = FixDiv(FIX_ABS(pBoxA->halfLength + projCorner1), -projAxis1);
            hit1 = 1;
        }
        index = g_unk0x005914d4;
        if (hit0 && t0 < 0x7d000000) {
            best = t0;
            g_unk0x005914a4[index] = 0;
        }
        if (hit1 && t1 < best) {
            best = t1;
            g_unk0x005914a4[index] = 2;
        }
        if ((hit0 || hit1) && best > maxDist)
            maxDist = best;
        g_unk0x00590ecc[index] = (char)i;
        passHit = 1;
        hit = 1;
        g_unk0x005914d4 = (char)(index + 1);
    }
    if (passHit != 0)
        Collision_SplitBoxSeparationMovement(pBoxA, pBoxB, &dir, maxDist, scale);
    // Pass 2: box A's corners tested in box B's frame, with the direction negated.
    FixVecScale(&negDir, &dir, -0x10000);
    projAxis0 = FixVecDot(&negDir, &pBoxB->axisA);
    projAxis1 = FixVecDot(&pBoxB->axisB, &negDir);
    passHit = 0;
    maxDist = 0;
    for (i = 0; i < 4; i++) {
        delta.x = pBoxA->points[i].x - pBoxB->pVertex->x;
        delta.y = pBoxA->points[i].y - pBoxB->pVertex->y;
        delta.z = pBoxA->points[i].z - pBoxB->pVertex->z;
        delta.y = 0;
        projCorner0 = FixVecDot(&delta, &pBoxB->axisA);
        projCorner1 = FixVecDot(&pBoxB->axisB, &delta);
        if (FIX_ABS(projCorner0) > pBoxB->halfWidth || FIX_ABS(projCorner1) > pBoxB->halfLength)
            continue;
        hit0 = 0;
        hit1 = 0;
        t0 = 0;
        t1 = 0;
        best = 0x7d000000;
        if (projAxis0 >= 0x42) {
            t0 = FixDiv(pBoxB->halfWidth - projCorner0, projAxis0);
            hit0 = 1;
        } else if (projAxis0 < -0x41) {
            t0 = FixDiv(FIX_ABS(pBoxB->halfWidth + projCorner0), -projAxis0);
            hit0 = 1;
        }
        if (projAxis1 > 0x41) {
            t1 = FixDiv(pBoxB->halfLength - projCorner1, projAxis1);
            hit1 = 1;
        } else if (projAxis1 < -0x41) {
            t1 = FixDiv(FIX_ABS(pBoxB->halfLength + projCorner1), -projAxis1);
            hit1 = 1;
        }
        index = g_unk0x005915f4;
        if (hit0 && t0 < 0x7d000000) {
            best = t0;
            g_unk0x00590ec8[index] = 0;
        }
        if (hit1 && t1 < best) {
            best = t1;
            g_unk0x00590ec8[index] = 2;
        }
        if ((hit0 || hit1) && best > maxDist)
            maxDist = best;
        g_unk0x005914c4[index] = (char)i;
        passHit = 1;
        hit = 1;
        g_unk0x005915f4 = (char)(index + 1);
    }
    if (passHit != 0)
        Collision_SplitBoxSeparationMovement(pBoxA, pBoxB, &dir, maxDist, scale);
    if (hit == 0) {
        hit = Collision_FindQuadEdgeOverlap((FixVector *)pBoxA, (FixVector *)pBoxB, &dir, &maxDist);
        if (hit != 0)
            Collision_SplitBoxSeparationMovement(pBoxA, pBoxB, &dir, maxDist, scale);
    }
    return hit;
}
// Resolves the car's contact with the face tracked in g_collisionFace: picks
// the contact axis, slides the car (position, eight corners and the face's
// four vertices) out of the surface and latches the impact direction and point
// from the surface descriptor in g_pCollisionFinRecord.
// match 61%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0048e730
int Collision_ResolveSectorFaceContact(int *param_1, int *param_2, int param_3, char param_4)
{
    int xRatio;
    int yRatio;
    int axis;
    int bestRatio;
    int haveX;
    int haveY;
    int len;
    int posDot0;
    int posDot1;
    int dot0;
    int dot1;
    int dot;
    int impulse;
    int i;
    BYTE surface;
    unsigned short angle;
    xRatio = 0;
    yRatio = 0;
    axis = -1;
    Collision_BuildCarBox(g_collisionFace, &g_collisionCar->right.x,
                 &g_collisionCar->position,
                 &g_collisionCar->corners[0]);
    // Offset of the tracked point from the car, flattened to the ground plane.
    g_unk0x00591968.x = param_1[0] - g_collisionCar->position.x;
    g_unk0x00591968.y = param_1[1] - g_collisionCar->position.y;
    g_unk0x00591968.z = param_1[2] - g_collisionCar->position.z;
    g_unk0x00591968.y = 0;
    posDot0 = FixVecDot(&g_unk0x00591968, &g_collisionFace->axisA);
    posDot1 = FixVecDot(&g_unk0x00591968, &g_collisionFace->axisB);
    if (FIX_ABS(posDot0) <= g_collisionFace->halfWidth && FIX_ABS(posDot1) <= g_collisionFace->halfLength) {
        g_unk0x00591950.x = param_1[0] - g_collisionCar->positionPrev.x;
        g_unk0x00591950.y = param_1[1] - g_collisionCar->positionPrev.y;
        g_unk0x00591950.z = param_1[2] - g_collisionCar->positionPrev.z;
        g_unk0x00591950.y = 0;
        len = FixVecLength(&g_unk0x00591950);
        if (len > 0) {
            FixVecScaleRecip(&g_unk0x00591920, &g_unk0x00591950, len);
            // Fraction of the move left before the car's box leaves each face axis; the
            // smallest of the two is the axis the car is pushed back along.
            dot0 = FixVecDot(&g_unk0x00591920, &g_collisionFace->axisA);
            dot1 = FixVecDot(&g_unk0x00591920, &g_collisionFace->axisB);
            haveX = 0;
            haveY = 0;
            bestRatio = 0x7d000000;
            if (dot0 > 0x41) {
                xRatio = FixDiv(g_collisionFace->halfWidth - posDot0, dot0);
                haveX = 1;
            } else if (dot0 < -0x41) {
                xRatio = FixDiv(FIX_ABS(g_collisionFace->halfWidth + posDot0), -dot0);
                haveX = 1;
            }
            if (dot1 > 0x41) {
                yRatio = FixDiv(g_collisionFace->halfLength - posDot1, dot1);
                haveY = 1;
            } else if (dot1 < -0x41) {
                yRatio = FixDiv(FIX_ABS(g_collisionFace->halfLength + posDot1), -dot1);
                haveY = 1;
            }
            if (haveX && xRatio < bestRatio) {
                axis = 0;
                bestRatio = xRatio;
            }
            if (haveY && yRatio < bestRatio) {
                axis = 1;
                bestRatio = yRatio;
            }
            FixVecScale(&g_unk0x00591920, &g_unk0x00591920, -0x10000);
            FixVecScale(&g_unk0x00591900, &g_unk0x00591920, bestRatio);
            g_collisionCar->position.x += g_unk0x00591900.x;
            g_collisionCar->position.y += g_unk0x00591900.y;
            g_collisionCar->position.z += g_unk0x00591900.z;
            for (i = 0; i < 8; i++) {
                g_collisionCar->corners[i].x += g_unk0x00591900.x;
                g_collisionCar->corners[i].y += g_unk0x00591900.y;
                g_collisionCar->corners[i].z += g_unk0x00591900.z;
            }
            for (i = 0; i < 4; i++) {
                g_collisionFace->points[i].x += g_unk0x00591900.x;
                g_collisionFace->points[i].y += g_unk0x00591900.y;
                g_collisionFace->points[i].z += g_unk0x00591900.z;
            }
            StageObject_ApplyRecursiveFrameDelta((BYTE)g_collisionCar->index, -1, (int *)&g_unk0x00591900, 0);
            // Surface descriptor of the tracked point: 0xff means there is no contact
            // surface, in which case the second tracked point decides the direction.
            surface = g_pCollisionFinRecord->surfaces[param_3 != 0];
            if (surface != 0xff) {
                // Impact direction from the surface angle (12-bit angle into the sine
                // table, with the +0x400 entry as the perpendicular component).
                angle = (unsigned short)FTOI(__int64, X87S_MUL((double)FixMul((int)surface << 16, 0x1cccc), g_fixedDegreesToAngle12));
                g_unk0x00591990.x = g_sinTable[angle & 0xfff];
                g_unk0x00591990.y = 0;
                g_unk0x00591990.z = g_sinTable[(angle + 0x400) & 0xfff];
                // Only a surface the car is moving into produces a bounce.
                dot = FixVecDot(&g_collisionCar->velocity, &g_unk0x00591990);
                if (dot < 0) {
                    impulse = -FixMul(dot, g_unk0x0051fb00[param_4] + 0x4ccc);
                    FixVecScale(&g_unk0x005918f0, &g_unk0x00591990, impulse);
                    g_unk0x00591978 = *(FixVector *)param_1;
                    g_unk0x00591978.y = g_collisionCar->position.y;
                    CarPhysics_ApplyImpulse(&g_unk0x005918f0, &g_unk0x00591978, 1);
                    len = FixVecLength(&g_unk0x005918f0);
                    if (g_unk0x00591930 == 0 || g_unk0x005919a0 < len) {
                        g_unk0x005919a0 = len;
                        g_unk0x005918e0 = g_unk0x005918f0;
                        g_unk0x00591ad0 = g_unk0x00591978;
                        g_unk0x00591930 = 1;
                        g_unk0x0059199c = 2;
                        g_unk0x00591938 = g_unk0x00591990;
                        return 1;
                    }
                }
            } else if (axis != -1) {
                g_unk0x00591990.x = param_2[0] - param_1[0];
                g_unk0x00591990.y = param_2[1] - param_1[1];
                g_unk0x00591990.z = param_2[2] - param_1[2];
                if (FIX_ABS(g_unk0x00591990.x) > FIX_ABS(g_unk0x00591990.y) &&
                    FIX_ABS(g_unk0x00591990.x) > FIX_ABS(g_unk0x00591990.z))
                    FixVecScaleRecip(&g_unk0x00591990, &g_unk0x00591990, FIX_ABS(g_unk0x00591990.x));
                else if (FIX_ABS(g_unk0x00591990.y) > FIX_ABS(g_unk0x00591990.x) &&
                         FIX_ABS(g_unk0x00591990.y) > FIX_ABS(g_unk0x00591990.z))
                    FixVecScaleRecip(&g_unk0x00591990, &g_unk0x00591990, FIX_ABS(g_unk0x00591990.y));
                else
                    FixVecScaleRecip(&g_unk0x00591990, &g_unk0x00591990, FIX_ABS(g_unk0x00591990.z));
                len = FixVecLength(&g_unk0x00591990);
                if (len == 0) {
                    g_unk0x00591990.x = 0;
                    g_unk0x00591990.y = 0;
                    g_unk0x00591990.z = 0;
                } else {
                    FixVecScaleRecip(&g_unk0x00591990, &g_unk0x00591990, len);
                }
                if (FixVecDot(&g_unk0x00591990, &g_collisionCar->velocity) > 0) {
                    // The contact axis picked above doubles as the bounce direction.
                    if (axis == 0)
                        g_unk0x00591990 = g_collisionFace->axisA;
                    else
                        g_unk0x00591990 = g_collisionFace->axisB;
                    dot = FixVecDot(&g_collisionCar->velocity, &g_unk0x00591990);
                    impulse = -FixMul(dot, g_unk0x0051fb00[param_4] + 0x10000);
                    FixVecScale(&g_unk0x005918f0, &g_unk0x00591990, impulse);
                    g_unk0x00591978 = *(FixVector *)param_1;
                    g_unk0x00591978.y = g_collisionCar->position.y;
                    CarPhysics_ApplyImpulse(&g_unk0x005918f0, &g_unk0x00591978, 1);
                    len = FixVecLength(&g_unk0x005918f0);
                    if (g_unk0x00591930 == 0 || g_unk0x005919a0 < len) {
                        g_unk0x005919a0 = len;
                        g_unk0x005918e0 = g_unk0x005918f0;
                        g_unk0x00591ad0 = g_unk0x00591978;
                        g_unk0x00591930 = 1;
                        g_unk0x0059199c = 2;
                        g_unk0x00591938 = g_unk0x00591990;
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

// --- trackside cameras (camera type 7) --------------------------------------
// A view record (see Car.cpp) whose camera stands at one of the stage's
// camera spots: g_unk0x00591750 lists them (0x6c bytes each: +2 heading, +4/+0x10/+0x1c
// basis, +0x28 position, +0x34 end of its dolly track, +0x40 trigger range,
// +0x4c.. zoom and shake settings). Spots turned (almost) 90 degrees behave as
// chase cameras. Every array below is indexed by the record index (0..3).

// The car has come within the spot's range: the camera turns to follow it.
// GLOBAL: CMR2 0x005916d0
int g_tracksideFollowing[4];
// Distance from the camera to the car (for the shake).
// GLOBAL: CMR2 0x005916e0
int g_tracksideCarDistances[4];
// Smoothed per-frame steps of the dolly, the zoom and the aim.
// GLOBAL: CMR2 0x00591700
int g_tracksideDollySpeeds[4];
// GLOBAL: CMR2 0x00591720
int g_tracksideZoomSpeeds[4];
// GLOBAL: CMR2 0x00591754
int g_tracksideAimSpeeds[4];

extern float g_oneOverRandMax;
extern double g_minus65536;
void StageObject_SelectAndCopyCarNodePayload(CameraRecord *pObj, int *pSrc, BYTE index, BYTE value);
void StageObject_InterpolateReferenceMatrix(CameraRecord *pObj, int *pSrc, int param_3);
void StageObject_RebuildMirroredTiltMatrix(CameraRecord *pObj, int *pSrc);
void View_PositionFromCarProfile(CameraRecord *pObj, FixMatrix *pRef);
void View_UpdateTracksideAimTarget(CameraRecord *pRecord, FixMatrix *pMatrix);
void View_StepTracksideAim(CameraRecord *pRecord, int step);
void View_StepTracksideZoom(CameraRecord *pRecord, int step);
void View_AdvanceTracksideDolly(CameraRecord *pRecord, int amount);
void View_BuildTracksideAimLead(FixVector *pOut, CameraRecord *pRecord, Car *pCar, FixMatrix *pMatrix);
void View_RestartTracksideCameraDolly(CameraRecord *pRecord, FixMatrix *pRef);
void View_BuildTracksideCameraMatrix(CameraRecord *pRecord, FixMatrix *pRef);

#define SPOT(i) (&g_unk0x00591750[g_viewCameraSpot[i]])

// Places a trackside camera on spot `spot` for a view record.
// FUNCTION: CMR2 0x0048cae0
void View_PlaceTracksideCameraAtSpot(CameraRecord *pRecord, FixMatrix *pRef, int spot)
{
    CameraSpot *pSpot;
    BYTE index;
    short heading;
    FixVector position;
    FixVector d;
    int near_;

    pSpot = &g_unk0x00591750[spot];
    index = pRecord->index;
    g_viewCameraSpot[index] = spot;
    heading = pSpot->heading;
    if (heading > 0x3f4 && heading < 0x40b) {
        StageObject_SelectAndCopyCarNodePayload(pRecord, (int *)pRef, (BYTE)Car_Get(pRecord->car)->index, 1);
    } else if (heading < -0x3f4 && heading > -0x40b) {
        StageObject_SelectAndCopyCarNodePayload(pRecord, (int *)pRef, (BYTE)Car_Get(pRecord->car)->index, 2);
    } else {
    FixMatrix_GetPosition(&position, pRef);
    g_tracksideAimSpeeds[index] = 0;
    g_tracksideZoomSpeeds[index] = 0;
    d.x = position.x - pSpot->position.x;
    d.y = position.y - pSpot->position.y;
    d.z = position.z - pSpot->position.z;
    if ((int)FixVec_Length(&d) < pSpot->range ||
        (FixVecDot(&d, &pSpot->forward) < 0 && pSpot->range > 0))
        near_ = 1;
    else
        near_ = 0;
    g_tracksideFollowing[index] = near_;
    g_tracksideAimLocked[index] = near_;
        View_RestartTracksideCameraDolly(pRecord, pRef);
        g_tracksideDollyProgress[index] = 0;
    }
}

// Per-frame update of a trackside camera: once the car is in range, eases the
// aim, zoom and dolly steps towards the spot's values and moves the aim, the
// zoom and the dolly by them.
// FUNCTION: CMR2 0x0048cc30
void View_UpdateTracksideZoomAndShake(CameraRecord *pRecord, FixMatrix *pRef)
{
    CameraSpot *pSpot;
    Car *pCar;
    BYTE index;
    short heading;
    FixVector position;
    FixVector d;
    int distance;
    int aimStep;
    int zoomStep;

    index = pRecord->index;
    heading = g_unk0x00591750[g_viewCameraSpot[index]].heading;
    pSpot = SPOT(index);
    if (heading > 0x3f4 && heading < 0x40b) {
        pCar = Car_Get(pRecord->car);
        StageObject_InterpolateReferenceMatrix(pRecord, (int *)pRef, (pCar->field_0xc04[0] == 0 && pCar->field_0xb60 == 0) ? 0 : 1);
        return;
    }
    if (heading < -0x3f4 && heading > -0x40b) {
        pCar = Car_Get(pRecord->car);
        StageObject_InterpolateReferenceMatrix(pRecord, (int *)pRef, (pCar->field_0xc04[0] == 0 && pCar->field_0xb60 == 0) ? 0 : 1);
        return;
    }
    FixMatrix_GetPosition(&position, pRef);
    d.x = pSpot->position.x - position.x;
    d.y = pSpot->position.y - position.y;
    d.z = pSpot->position.z - position.z;
    distance = FixVec_Length(&d);
    if (g_tracksideFollowing[index] != 0) {
        if (distance < g_tracksideCarDistances[index]) {
            aimStep = pSpot->aimStep[0];
            zoomStep = pSpot->zoomStep[0];
        } else {
            aimStep = pSpot->aimStep[1];
            zoomStep = pSpot->zoomStep[1];
        }
        g_tracksideAimSpeeds[index] = FixMul(aimStep, 0x3333) + FixMul(g_tracksideAimSpeeds[index], 0xcccc);
        g_tracksideZoomSpeeds[index] = FixMul(zoomStep, 0x3333) + FixMul(g_tracksideZoomSpeeds[index], 0xcccc);
        g_tracksideDollySpeeds[index] = FixMul(pSpot->dollyStep, 0x1999) + FixMul(g_tracksideDollySpeeds[index], 0xe666);
        View_UpdateTracksideAimTarget(pRecord, pRef);
        View_StepTracksideAim(pRecord, g_tracksideAimSpeeds[index]);
        View_StepTracksideZoom(pRecord, g_tracksideZoomSpeeds[index]);
        View_AdvanceTracksideDolly(pRecord, g_tracksideDollySpeeds[index]);
        View_BuildTracksideCameraMatrix(pRecord, pRef);
        return;
    }
    if (distance < pSpot->range)
        g_tracksideFollowing[index] = 1;
    View_BuildTracksideCameraMatrix(pRecord, pRef);
}

// Restarts a trackside camera: back to the start of its dolly track (or to the
// car when the car is already in range).
// FUNCTION: CMR2 0x0048ce80
void View_RestartTracksideCameraDolly(CameraRecord *pRecord, FixMatrix *pRef)
{
    CameraSpot *pSpot;
    BYTE index;
    short heading;
    FixVector position;
    FixVector d;

    index = pRecord->index;
    heading = g_unk0x00591750[g_viewCameraSpot[index]].heading;
    pSpot = SPOT(index);
    if (heading > 0x3f4 && heading < 0x40b) {
        StageObject_RebuildMirroredTiltMatrix(pRecord, (int *)pRef);
        return;
    }
    if (heading < -0x3f4 && heading > -0x40b) {
        StageObject_RebuildMirroredTiltMatrix(pRecord, (int *)pRef);
        return;
    }
    g_tracksideDollyProgress[index] = 0;
    if (g_tracksideFollowing[index] != 0) {
        View_BuildTracksideAimLead(&g_tracksideAimLeads[index], pRecord, Car_Get(pRecord->car), pRef);
        View_UpdateTracksideAimTarget(pRecord, pRef);
        if (g_tracksideAimSpeeds[index] > 0)
            g_tracksideDollyProgress[index] = 0x10000;
        g_tracksideAimLocked[index] = 1;
    } else {
        FixMatrix_GetPosition(&position, pRef);
        d.x = position.x - pSpot->position.x;
        d.y = position.y - pSpot->position.y;
        d.z = position.z - pSpot->position.z;
        if ((int)FixVec_Length(&d) < pSpot->range ||
            (FixVecDot(&d, &pSpot->forward) < 0 && pSpot->range > 0)) {
            View_UpdateTracksideAimTarget(pRecord, pRef);
        } else {
            FixVecScale(&g_tracksideAimTargets[index], &pSpot->forward, pSpot->range);
            g_tracksideAimTargets[index].x += pSpot->position.x;
            g_tracksideAimTargets[index].y += pSpot->position.y;
            g_tracksideAimTargets[index].z += pSpot->position.z;
            g_tracksideZoomTargets[index] = pSpot->zoom;
        }
        g_tracksideAimLeads[index].x = 0;
        g_tracksideAimLeads[index].y = 0;
        g_tracksideAimLeads[index].z = 0;
    }
    g_tracksideDollySpeeds[index] = 0;
    g_tracksideAims[index] = g_tracksideAimTargets[index];
    g_tracksideZooms[index] = g_tracksideZoomTargets[index];
    View_BuildTracksideCameraMatrix(pRecord, pRef);
}

// Builds a trackside camera's matrix: its position (moved along the dolly track
// and shaken while the car is close), looking along the spot's basis or at the
// car, turned by the spot's heading, with the spot's zoom.
// FUNCTION: CMR2 0x0048d0f0
void View_BuildTracksideCameraMatrix(CameraRecord *pRecord, FixMatrix *pRef)
{
    CameraRecord *pCameraRecord = pRecord;
    FixMatrix turn;
    FixVector right;
    FixVector up;
    FixVector offset;
    FixVector toCar;
    FixVector carPos;
    FixVector shake;
    FixVector camera;
    CameraSpot *pSpot;
    unsigned int index;
    short heading;
    int amplitude;
    int zoom;

    index = pCameraRecord->index;
    heading = g_unk0x00591750[g_viewCameraSpot[index]].heading;
    pSpot = SPOT(index);
    if (heading > 0x3f4 && heading < 0x40b) {
        View_PositionFromCarProfile(pRecord, pRef);
        return;
    }
    if (heading < -0x3f4 && heading > -0x40b) {
        View_PositionFromCarProfile(pRecord, pRef);
        return;
    }
    FixMatrix_GetPosition(&carPos, pRef);
    camera = *&pSpot->position;
    if (pSpot->dollyStep > 0) {
        right.x = pSpot->trackEnd.x - pSpot->position.x;
        right.y = pSpot->trackEnd.y - pSpot->position.y;
        right.z = pSpot->trackEnd.z - pSpot->position.z;
        FixVecScale(&up, &right, g_tracksideDollyProgress[index]);
        FixVecScale(&up, &up, 0x20000);
        camera.x += up.x;
        camera.y += up.y;
        camera.z += up.z;
    }
    if (g_tracksideCarDistances[index] < pSpot->shakeRange) {
        amplitude = FixMul(pSpot->shakeAmplitude, 0x10000 - FixDiv(g_tracksideCarDistances[index], pSpot->shakeRange));
        amplitude = FixMul(amplitude, Car_Get(pCameraRecord->car)->speed / 2);
        shake.x = -0x8000 - (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), g_minus65536));
        shake.y = -0x8000 - (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), g_minus65536));
        shake.z = -0x8000 - (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), g_minus65536));
        FIX_NORMALIZE_INTO(shake, shake);
        FixVecScale(&shake, &shake, amplitude);
        camera.x += shake.x;
        camera.y += shake.y;
        camera.z += shake.z;
    }
    toCar.x = camera.x - carPos.x;
    toCar.y = camera.y - carPos.y;
    toCar.z = camera.z - carPos.z;
    g_tracksideCarDistances[index] = FixVec_Length(&toCar);
    if (g_tracksideFollowing[index] != 0) {
        offset.x = g_tracksideAims[index].x - camera.x;
        offset.y = g_tracksideAims[index].y - camera.y;
        offset.z = g_tracksideAims[index].z - camera.z;
        FixVec_Normalize(&offset, &offset);
        zoom = g_tracksideZooms[index];
        up.x = 0;
        up.z = 0;
        up.y = 0x10000;
        FixVecCross(&right, &up, &offset);
        FixVecCross(&up, &offset, &right);
        FixVec_Normalize(&right, &right);
        FixVec_Normalize(&up, &up);
        FixVec_Normalize(&offset, &offset);
        FixMatrix_SetRight(&right, &pRecord->matrix);
        FixMatrix_SetUp(&up, &pRecord->matrix);
        FixMatrix_SetForward(&offset, &pRecord->matrix);
    } else {
        FixMatrix_SetRight(&pSpot->right, &pRecord->matrix);
        FixMatrix_SetUp(&pSpot->up, &pRecord->matrix);
        FixMatrix_SetForward(&pSpot->forward, &pRecord->matrix);
        zoom = pSpot->zoom;
    }
    if (zoom < 0x10000)
        pCameraRecord->zoom = FixMul(0xa000, 0x10000);
    else
        pCameraRecord->zoom = FixMul(0xa000, zoom);
    pCameraRecord->matrix.position.x = 0;
    pCameraRecord->matrix.position.y = 0;
    pCameraRecord->matrix.position.z = 0;
    FixMatrix_Identity(&turn);
    turn.forward.z = 0x10000;
    // The original indexes the sine table at heading + 90 degrees here.
    turn.right.x = g_sinTable[(unsigned short)(pSpot->heading + 0x400) & 0xfff];
    turn.up.y = turn.right.x;
    turn.right.y = -FixSin((unsigned short)pSpot->heading);
    turn.right.z = 0;
    turn.up.x = FixSin((unsigned short)pSpot->heading);
    turn.up.z = 0;
    turn.forward.x = 0;
    turn.forward.y = 0;
    FixMatrix_Multiply(&pRecord->matrix, &turn, &pRecord->matrix);
    FixMatrix_SetPosition(&camera, &pRecord->matrix);
    pCameraRecord->carLift = 0;
    pCameraRecord->nearPlane = 0x1999;
    pCameraRecord->nearPlane = FixMul(pCameraRecord->nearPlane, 0x50000);
    pCameraRecord->nearPlane = FixMul(pCameraRecord->nearPlane, FixDiv(pCameraRecord->zoom, 0xa000));
    pCameraRecord->field_0x50 = 0;
    pCameraRecord->liftPitch = 0;
    pCameraRecord->lookDistance = 0x10000;
}
#undef SPOT

// GLOBAL: CMR2 0x0051c9b4
char g_strCarModelC5Bfl[] = "%sc5.bfl";
// GLOBAL: CMR2 0x0051c9c0
char g_strCarModelA5Bfl[] = "%sa5.bfl";
// GLOBAL: CMR2 0x0051c9cc
char g_strCarModelC5C3d[] = "%sc5.c3d";
// GLOBAL: CMR2 0x0051c9d8
char g_strCarModelA5C3d[] = "%sa5.c3d";

char *Car_GetDirectoryPath(int car);
void StageObject_FindPlayerRevTextures(int player);
struct SceneNode *Sector_BuildC3DModelScene(void *data, struct SceneNode *parent, struct GenericFile *archive);


// video.cockpit_glass_fog: flags every mesh of an interior (cockpit) model -
// all of its LOD records - so its semi-transparent parts (windscreen glass)
// skip depth writes and the fog pass can see the world depth through the
// glass. The draw path also requires mesh flag 0x8 (alpha blending) and a part
// that actually has semi-transparent vertices, so opaque interior parts keep
// writing depth and interior self-occlusion is unchanged.
static void StageObject_FlagInteriorGlassNoZWrite(SceneNode *pNode)
{
    while (pNode != NULL) {
        if (pNode->type == SCENE_NODE_MESH && pNode->pObject != NULL) {
            Mesh *pMesh = (Mesh *)pNode->pObject;
            int lods = pMesh->lodCount > 0 ? pMesh->lodCount : 1;
            for (int i = 0; i < lods; i++) {
                Mesh *pLod = (Mesh *)((BYTE *)pMesh + i * offsetof(Mesh, field_0x108));
                pLod->flags |= MESH_FLAG_GLASS_NO_ZWRITE;
            }
        }
        if (pNode->pFirstChild != NULL)
            StageObject_FlagInteriorGlassNoZWrite(pNode->pFirstChild);
        pNode = pNode->pNext;
    }
}

// video.cockpit_door_extend (OpenCMR2 prototype, default 0 = off): the Subaru Impreza 555 cockpit model (suba5.c3d) ends its driver-side door
// card in a vertical cut at the rear (x -0.059, z +0.644, y -0.465..-0.109 in mesh space), through which the ground shows. This moves the two
// rear vertices of that card back by N/100 units, extending their position and UV along the card's existing gradient, then re-uploads the
// vertices. It only acts when the vertex pattern of the original card is found (so other cars and an already extended mesh are untouched).
static int StageObject_ExtendInteriorDoorCard(SceneNode *pNode, float extend)
{
    int moved = 0;
    for (; pNode != NULL; pNode = pNode->pNext) {
        if (pNode->type == SCENE_NODE_MESH && pNode->pObject != NULL) {
            Mesh *pMesh = (Mesh *)pNode->pObject;
            int lods = pMesh->lodCount > 0 ? pMesh->lodCount : 1;
            for (int i = 0; i < lods; i++) {
                Mesh *pLod = (Mesh *)((BYTE *)pMesh + i * offsetof(Mesh, field_0x108));
                if (pLod->pVertexData == NULL || pLod->field_0x10 != 327)
                    continue;
                MeshVertexF *v = (MeshVertexF *)pLod->pVertexData;
                int n = pLod->field_0x10, top = -1, bot = -1, topNb = -1, botNb = -1;
                #define NEAR_(a, b) (fabsf((a) - (b)) < 0.004f)
                for (int k = 0; k < n; k++) {
                    if (NEAR_(v[k].z, 0.644f) && NEAR_(v[k].x, -0.059f) && NEAR_(v[k].y, -0.109f)) top = k;
                    else if (NEAR_(v[k].z, 0.644f) && NEAR_(v[k].x, -0.059f) && NEAR_(v[k].y, -0.465f) && NEAR_(v[k].textureCoordinates[0][0], 0.533f)) bot = k;   // no normal test: ny is 0.894 here, on the tolerance edge
                    else if (NEAR_(v[k].z, 0.642f) && NEAR_(v[k].x, 0.528f) && NEAR_(v[k].y, -0.051f) && NEAR_(v[k].textureCoordinates[0][0], 0.529f)) topNb = k;
                    else if (NEAR_(v[k].z, 0.644f) && NEAR_(v[k].x, 0.379f) && NEAR_(v[k].y, -0.465f) && NEAR_(v[k].textureCoordinates[0][0], 0.533f)) botNb = k;
                }
                #undef NEAR_
                if (top < 0 || bot < 0 || topNb < 0 || botNb < 0)
                    continue;
                // UV bounds of the original card: it samples only a small patch of its atlas, so extrapolated UVs are clamped to
                // that patch (instead of reading unrelated atlas content beyond it).
                float uMin = 1e9f, uMax = -1e9f, vMin = 1e9f, vMax = -1e9f;
                int ref[4] = { top, bot, topNb, botNb };
                for (int r = 0; r < 4; r++) {
                    float uu = v[ref[r]].textureCoordinates[0][0], vv = v[ref[r]].textureCoordinates[0][1];
                    if (uu < uMin) uMin = uu;
                    if (uu > uMax) uMax = uu;
                    if (vv < vMin) vMin = vv;
                    if (vv > vMax) vMax = vv;
                }
                // The card is duplicated: other vertices share the rear corner positions with a second UV set (e.g. the second
                // triangle pair at the same corners). Remember the originals so those copies move with the corners.
                float topX = v[top].x, topY = v[top].y, topZ = v[top].z, botX = v[bot].x, botY = v[bot].y, botZ = v[bot].z;
                float topU0 = v[top].textureCoordinates[0][0], topV0 = v[top].textureCoordinates[0][1];
                float botU0 = v[bot].textureCoordinates[0][0], botV0 = v[bot].textureCoordinates[0][1];
                float dx = v[top].x - v[topNb].x;   // negative: from the neighbour toward the rear
                float t = extend / -dx;
                // Rear top corner: continue the neighbour->corner line (position and UV).
                v[top].x -= extend;
                v[top].y += (v[top].y - v[topNb].y) * t;
                for (int c = 0; c < 2; c++) {
                    v[top].textureCoordinates[c][0] += (v[top].textureCoordinates[c][0] - v[topNb].textureCoordinates[c][0]) * t;
                    v[top].textureCoordinates[c][1] += (v[top].textureCoordinates[c][1] - v[topNb].textureCoordinates[c][1]) * t;
                }
                // Rear bottom corner: the card's lower edge is horizontal; keep y, extend UV along it.
                float tb = extend / (v[botNb].x - v[bot].x);
                v[bot].x -= extend;
                for (int c = 0; c < 2; c++) {
                    v[bot].textureCoordinates[c][0] += (v[bot].textureCoordinates[c][0] - v[botNb].textureCoordinates[c][0]) * tb;
                    v[bot].textureCoordinates[c][1] += (v[bot].textureCoordinates[c][1] - v[botNb].textureCoordinates[c][1]) * tb;
                }
                int rear[2] = { top, bot };
                for (int r = 0; r < 2; r++) {
                    float *uv = v[rear[r]].textureCoordinates[0];
                    if (uv[0] < uMin) uv[0] = uMin;
                    if (uv[0] > uMax) uv[0] = uMax;
                    if (uv[1] < vMin) uv[1] = vMin;
                    if (uv[1] > vMax) uv[1] = vMax;
                }
                {   // coincident copies of the two rear corners: same position change, same UV change, clamped to the card patch
                    #define NEAR_(a, b) (fabsf((a) - (b)) < 0.0015f)
                    float dTopX = v[top].x - topX, dTopY = v[top].y - topY, dTopU = v[top].textureCoordinates[0][0] - topU0, dTopV = v[top].textureCoordinates[0][1] - topV0;
                    float dBotX = v[bot].x - botX, dBotU = v[bot].textureCoordinates[0][0] - botU0, dBotV = v[bot].textureCoordinates[0][1] - botV0;
                    for (int k = 0; k < n; k++) {
                        if (k == top || k == bot) continue;
                        float *uv = v[k].textureCoordinates[0];
                        if (NEAR_(v[k].x, topX) && NEAR_(v[k].y, topY) && NEAR_(v[k].z, topZ)) {
                            v[k].x += dTopX; v[k].y += dTopY; uv[0] += dTopU; uv[1] += dTopV;
                        } else if (NEAR_(v[k].x, botX) && NEAR_(v[k].y, botY) && NEAR_(v[k].z, botZ)) {
                            v[k].x += dBotX; uv[0] += dBotU; uv[1] += dBotV;
                        } else continue;
                        if (uv[0] < uMin) uv[0] = uMin;
                        if (uv[0] > uMax) uv[0] = uMax;
                        if (uv[1] < vMin) uv[1] = vMin;
                        if (uv[1] > vMax) uv[1] = vMax;
                    }
                    #undef NEAR_
                }
                Mesh_RefreshVertices(pLod);
                moved++;
            }
        }
        if (pNode->pFirstChild != NULL)
            moved += StageObject_ExtendInteriorDoorCard(pNode->pFirstChild, extend);
    }
    return moved;
}

// video.cockpit_extend_rear: registers the original Subaru door-card rear edge (same vertex match as the door extend above, run BEFORE it
// moves anything) with the texture of the part that draws it, so WindshieldGlass can draw continuation strips in the card's mesh space.
static int StageObject_RegisterCockpitExtension(SceneNode *pNode, int car)
{
    int found = 0;
    for (; pNode != NULL; pNode = pNode->pNext) {
        if (pNode->type == SCENE_NODE_MESH && pNode->pObject != NULL) {
            Mesh *pMesh = (Mesh *)pNode->pObject;
            int lods = pMesh->lodCount > 0 ? pMesh->lodCount : 1;
            for (int i = 0; i < lods && !found; i++) {
                Mesh *pLod = (Mesh *)((BYTE *)pMesh + i * offsetof(Mesh, field_0x108));
                if (pLod->pVertexData == NULL || pLod->field_0x10 != 327)
                    continue;
                MeshVertexF *v = (MeshVertexF *)pLod->pVertexData;
                int n = pLod->field_0x10, top = -1, bot = -1, topNb = -1;
                #define NEAR_(a, b) (fabsf((a) - (b)) < 0.004f)
                for (int k = 0; k < n; k++) {
                    if (NEAR_(v[k].z, 0.644f) && NEAR_(v[k].x, -0.059f) && NEAR_(v[k].y, -0.109f)) top = k;
                    else if (NEAR_(v[k].z, 0.644f) && NEAR_(v[k].x, -0.059f) && NEAR_(v[k].y, -0.465f) && NEAR_(v[k].textureCoordinates[0][0], 0.533f)) bot = k;
                    else if (NEAR_(v[k].z, 0.642f) && NEAR_(v[k].x, 0.528f) && NEAR_(v[k].y, -0.051f) && NEAR_(v[k].textureCoordinates[0][0], 0.529f)) topNb = k;
                }
                #undef NEAR_
                if (top < 0 || bot < 0 || topNb < 0)
                    continue;
                int tex = -1;
                for (int pi = 0; pi < pLod->partCount && pi < 50 && tex < 0; pi++) {
                    MeshPart *pPart = pLod->pParts[pi];
                    if (pPart == NULL || pPart->pData == NULL)
                        continue;
                    for (int q = 0; q < pPart->indexCount; q++)
                        if (pPart->minIndex + pPart->pData[q] == bot) { tex = pPart->texture; break; }
                }
                float u = 0.5f * (v[top].textureCoordinates[0][0] + v[bot].textureCoordinates[0][0]);
                float vv = 0.5f * (v[top].textureCoordinates[0][1] + v[bot].textureCoordinates[0][1]);
                float slope = (v[top].y - v[topNb].y) / (v[top].x - v[topNb].x);
                WindshieldGlass_SetCockpitExtension(car, pNode, tex, u, vv, v[top].x, v[top].y, v[bot].y, v[top].z, slope, -0.108f);
                // Passenger card ends much farther forward than the driver card.
                // Match its real rear wall/floor corners before adding coverage.
                int pt=-1,pb=-1,pf=-1,dbn=-1;
                for(int k=0;k<n;k++) {
                    if(fabsf(v[k].x-.6133f)<.004f&&fabsf(v[k].y+.0152f)<.004f&&fabsf(v[k].z+.6231f)<.004f)pt=k;
                    if(fabsf(v[k].x-.5015f)<.004f&&fabsf(v[k].y+.4648f)<.004f&&fabsf(v[k].z+.6444f)<.004f)pb=k;
                    if(fabsf(v[k].x-.3793f)<.004f&&fabsf(v[k].y+.4648f)<.004f&&fabsf(v[k].z+.1082f)<.004f)pf=k;
                    if(fabsf(v[k].x-.379f)<.004f&&fabsf(v[k].y+.4648f)<.004f&&fabsf(v[k].z-.6444f)<.004f&&fabsf(v[k].textureCoordinates[0][0]-.5331f)<.004f)dbn=k;
                }
                if(pt>=0&&pb>=0&&pf>=0&&dbn>=0) {
                    CockpitExtensionProfile profiles[2]={};
                    auto copyP=[](float *p,const MeshVertexF&w){p[0]=w.x;p[1]=w.y;p[2]=w.z;};
                    copyP(profiles[0].top,v[top]);copyP(profiles[0].bottom,v[bot]);
                    profiles[0].floorInner[0]=v[bot].x;profiles[0].floorInner[1]=v[bot].y;profiles[0].floorInner[2]=-.1082f;
                    copyP(profiles[1].top,v[pt]);copyP(profiles[1].bottom,v[pb]);copyP(profiles[1].floorInner,v[pf]);
                    for(int side=0;side<2;side++) {
                        int corners[4]={top,topNb,bot,dbn};
                        for(int q=0;q<4;q++)for(int uv=0;uv<2;uv++)profiles[side].uv[q][uv]=v[corners[q]].textureCoordinates[0][uv];
                        profiles[side].repeatLength=fabsf(v[topNb].x-v[top].x);
                    }
                    WindshieldGlass_SetCockpitExtensionProfiles(car,profiles);
                }
                Sys_Log("cockpit rear extension: car %d registered door-card rear edge x %.3f, part texture %d, uv %.3f %.3f", car, v[top].x, tex, u, vv);
                found = 1;
            }
        }
        if (!found && pNode->pFirstChild != NULL)
            found = StageObject_RegisterCockpitExtension(pNode->pFirstChild, car);
        if (found)
            break;
    }
    return found;
}

// Loads the interior (cockpit) model of a player's car and hooks its nodes
// (steering wheel, dash, driver) into the car's scene graph.
// FUNCTION: CMR2 0x004760a0
void StageObject_LoadAndAttachCarInterior(BYTE record, BYTE car)
{
    Car *pCar;
    int ok;
    CarInteriorNodes *pRow;
    FixVector *pOffset;

    ok = 1;
    if (CGameInfo::GetGameModeOptionBit19() && car > 0)
        return;
    pCar = Car_Get(car);
    if ((short)car < Car_GetOrderCount() && pCar->pBodyNode != NULL) {
        g_carDetailBodyNodes[car] = SceneNode_FindByType(pCar->pBodyNode, 9);
        g_carDefaultBodyNodes[car] = SceneNode_FindByType(pCar->pBodyNode, 5);
    } else {
        ok = 0;
    }
    if (car >= 2)
        return;
    pRow = &g_carInteriorNodeRows[car];
    if (pRow->driverPoseNode != 0)
        return;
    if ((char)RallyData_GetFlag24() || ok == 0)
        return;
    if (CGameInfo::GetPreviewMode() == 0)
        sprintf(CFrontend::m_stringDest, g_strCarModelA5C3d,
                Car_GetDirectoryPath(RallyData_GetDriverRecordSelectionValue(StageUI_GetRaceEndEventCount() + car)));
    else
        sprintf(CFrontend::m_stringDest, g_strCarModelC5C3d,
                Car_GetDirectoryPath(RallyData_GetDriverRecordSelectionValue(StageUI_GetRaceEndEventCount() + car)));
    g_carInteriorModelBuffers[car] = CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, FALSE);
    if (CGameInfo::GetPreviewMode() == 0)
        sprintf(CFrontend::m_stringDest, g_strCarModelA5Bfl,
                Car_GetDirectoryPath(RallyData_GetDriverRecordSelectionValue(StageUI_GetRaceEndEventCount() + car)));
    else
        sprintf(CFrontend::m_stringDest, g_strCarModelC5Bfl,
                Car_GetDirectoryPath(RallyData_GetDriverRecordSelectionValue(StageUI_GetRaceEndEventCount() + car)));
    CGenericFileLoader::LoadIntoFileRecord(g_carInteriorArchives, CFrontend::m_stringDest);
    CarInterior_CacheProfilePointers(car);
    if (g_carInteriorModelBuffers[car] == NULL)
        return;
    g_carInteriorRoots[car] = Sector_BuildC3DModelScene(g_carInteriorModelBuffers[car],
                                                pCar->pBodyNode,
                                                g_carInteriorArchives);
    if (Sys_GetOption("video.cockpit_glass_fog", 1))
        StageObject_FlagInteriorGlassNoZWrite(g_carInteriorRoots[car]);
    if (Sys_GetOption("video.cockpit_extend_rear", 0) > 0)
        StageObject_RegisterCockpitExtension(g_carInteriorRoots[car], car);
    if (Sys_GetOption("video.cockpit_door_extend", 0) > 0) {
        int ext = Sys_GetOption("video.cockpit_door_extend", 0);
        if (ext > 80) ext = 80;
        int n = StageObject_ExtendInteriorDoorCard(g_carInteriorRoots[car], ext / 100.0f);
        Sys_Log("cockpit door extend: car %d extend %d/100, %d door-card meshes changed%s", car, ext, n,
                n == 0 ? " (original Subaru door card not found, nothing changed)" : "");
    }
    pRow->steeringReferenceNode = SceneNode_Create(g_carInteriorRoots[car]);
    pRow->steeringRestNode = SceneNode_FindByType(g_carInteriorRoots[car], 0x1a);
    pRow->steeringBlendNode = SceneNode_FindByType(g_carInteriorRoots[car], 0x1c);
    pRow->driverPoseNode = SceneNode_Create(pCar->pBodyNode);
    pRow->steeringWheelNode = SceneNode_FindByType(g_carInteriorRoots[car], 0x1b);
    pRow->wiperNodes[0] = SceneNode_FindByType(g_carInteriorRoots[car], 0x16);
    pRow->wiperNodes[1] = SceneNode_FindByType(g_carInteriorRoots[car], 0x17);
    memcpy(&pRow->steeringReferenceNode->current, &pRow->steeringBlendNode->current, sizeof(FixMatrix));
    pRow->steeringReferenceNode->pObject = NULL;
    pRow->steeringReferenceNode->type = 3;
    pRow->steeringReferenceNode->flags = pRow->steeringBlendNode->flags;
    SceneNode_Reparent((SceneNode *)pRow->steeringBlendNode, g_carInteriorRoots[car]);
    pOffset = g_carInteriorProfilePointers[car].driverPoseOffset;
    pRow->driverPoseNode->current.position.x = pRow->steeringWheelNode->current.position.x + pOffset->x;
    pRow->driverPoseNode->current.position.y = pRow->steeringWheelNode->current.position.y + pOffset->y;
    pRow->driverPoseNode->current.position.z = pRow->steeringWheelNode->current.position.z + pOffset->z;
    g_carInteriorLoaded[car] = 1;
    g_carInteriorModelClasses[(BYTE)record] = (BYTE)pCar->type;
    g_carDriverRestPositions[car].x = pRow->driverPoseNode->current.position.x;
    g_carDriverRestPositions[car].y = pRow->driverPoseNode->current.position.y;
    g_carDriverRestPositions[car].z = pRow->driverPoseNode->current.position.z;
    StageObject_FindPlayerRevTextures(car);
    // OpenCMR2: resolve this car's cockpit windshield glass (video.windshield_cracks)
    // now that the interior nodes exist; a resolve failure just logs and keeps
    // the feature off for the car.
    if (Sys_GetOption("video.windshield_cracks", 1))
        WindshieldGlass_SetupCar(car);
}

// --- CPU driver input (0x47b000-0x47c5ac) -----------------------------------

#define AI_INT(off) (*(int *)((BYTE *)g_aiDriveState + (off)))
#define AI_CHAR(off) (*(char *)((BYTE *)g_aiDriveState + (off)))

// AI data of the stage: base pointer and the per-car route headers.
// GLOBAL: CMR2 0x0058e378
BYTE *g_stageAiData;
// GLOBAL: CMR2 0x0058e37c
int *g_unk0x0058e37c[6];
// GLOBAL: CMR2 0x0051f4c4
char g_strAi2Format[] = "%s.ai2";
// GLOBAL: CMR2 0x0051f4cc
char g_strAi1Format[] = "%s.ai1";
// GLOBAL: CMR2 0x0051f4d4
char g_strAi0Format[] = "%s.ai0";

int RallyData_GetActiveCarRaceRecordField0(Car *p);
void StageTiming_RecomputeCarSplitBarSamples(Car *pCar, unsigned int mask, int *pOut, int variant);
BYTE *StageTiming_RelocateCarRecordRoutePoints(BYTE *p, BYTE **unused, int count);
BYTE Car_GetDrawnFlag(int index);
void Car_ResetWheelLoadsAfterShift(Car *pCar);
void StageObject_UpdatePlayerControlIndicators(int player, int device);
BYTE *GameMenu_GetChampionshipTransitionState(void);
BYTE *StageTiming_GetStageArchiveState(void);
unsigned int RallyData_GetFlag22(void);

// Works out the controls of a CPU car from the route data: the route
// segment's steering hint, the obstacle state (AI_SelectWallCollisionResponse), the overtaking
// logic and the driver's errors. `preview` != 0 only updates the AI state.
// FUNCTION: CMR2 0x0047bdd0
void AI_UpdateRouteDrivingControls(Car *pCar, int car, int preview)
{
    int modes[3];
    // StageObject_BuildTypeTableRowOutput writes five channels; the last is the handbrake.
    int controls[5];
    int *pRoute;
    int table;
    int node;
    int variant;
    int ahead;
    int behind;
    int force;

    controls[0] = 0;
    controls[1] = 0;
    controls[2] = 0;
    pRoute = g_unk0x0058e37c[car];
    controls[3] = 0;
    AI_CHAR(0xa4) = 0;
    modes[0] = 0;
    *((BYTE *)g_aiDriveState + 0xab + car) = 0;
    modes[1] = 0;
    controls[4] = 0;
    table = *pRoute;
    if (table == 0)
        return;
    node = RallyData_GetActiveCarRaceRecordField0(pCar);
    AI_INT(0x54) = node;
    StageObject_LookupKeyedValuePair(car, table, node, &variant, &modes[2]);
    *(unsigned int *)g_aiRouteTables.routes[table] |= 0xfffffffc;
    StageTiming_RecomputeCarSplitBarSamples(pCar, *(unsigned int *)g_aiRouteTables.routes[table], g_aiDriveState, variant);
    ahead = -AI_INT(0x34) - (int)FTOI(__int64, X87S_MUL((double)g_aiRouteNodes[node].wall[0xa - 8], g_minus65536));
    behind = AI_INT(0x34) - (int)FTOI(__int64, X87S_MUL((double)g_aiRouteNodes[node].wall[0xe - 8], g_minus65536));
    g_unk0x0058e230[car] = behind;
    if (ahead < behind)
        g_unk0x0058e230[car] = ahead;
    AI_CHAR(0xa4) = (char)AI_SelectWallCollisionResponse(g_aiDriveState);
    if ((char)RallyData_GetSelectionBits10To11() == 2 && (char)RallyData_GetSelectionBits12To13() == 1 && (unsigned int)node > 0xdd &&
        (unsigned int)node < 0xe4)
        AI_CHAR(0xa4) = 0;
    AI_FindClosestCarInAngleWindow(car, modes, node, g_aiDriveState);
    if (CGameInfo::IsActiveCheatEnabled(3))
        *((BYTE *)g_aiDriveState + 0xab + car) =
            (BYTE)AI_SelectCarsWithinOvertakeWindow(car, (BYTE *)g_aiDriveState + 0xa5, &modes[1], (BYTE *)g_aiDriveState + 0xb1 + car);
    if (CGameInfo::IsActiveCheatEnabled(0))
        StageObject_UpdateApproachingCar(car, node);
    if (AI_CHAR(0xa4) == 0) {
        StageObject_BuildTypeTableRowOutput(modes[2], controls, g_aiDriveState);
        switch (modes[0]) {
        case 1:
            controls[0] = 0;
            controls[1] = 0x3f;
            break;
        case 2:
            controls[0] = 0x3f;
            controls[1] = 0;
            break;
        case 3:
            controls[0] = 0;
            controls[1] = 0x3f;
            controls[3] = 0x3f;
            break;
        case 4:
            controls[1] = 0;
            controls[0] = 0x3f;
            controls[3] = 0x3f;
            break;
        }
        if (*((char *)g_aiDriveState + 0xb1 + car) == -1) {
            if (AI_INT(0x78) > -0x140000) {
                controls[0] = 0;
                controls[1] = 0x3f;
            }
        } else if (*((char *)g_aiDriveState + 0xb1 + car) == 1 && AI_INT(0x78) < 0x140000) {
            controls[0] = 0x3f;
            controls[1] = 0;
        }
        if (AI_INT(0xc) < -30000 && AI_INT(0) > 0xa0000)
            controls[2] = 0;
        if (AI_INT(0x64) > -0xa0000 && AI_INT(8) < -0x7d)
            controls[0] = 0;
        if (AI_INT(0x64) < 0xa0000 && AI_INT(8) > 0x7d)
            controls[1] = 0;
    } else {
        switch (AI_CHAR(0xa4)) {
        case 1:
            controls[1] = 0x3f;
            controls[2] = 0x3f;
            break;
        case 2:
            controls[1] = 0x3f;
            controls[3] = 0x3f;
            break;
        case 3:
            controls[0] = 0x3f;
            controls[2] = 0x3f;
            break;
        case 4:
            controls[0] = 0x3f;
            controls[3] = 0x3f;
            break;
        case 5:
            controls[2] = 0x3f;
            break;
        case 6:
            controls[3] = 0x3f;
            break;
        case 7:
            controls[1] = 0x3f;
            break;
        case 8:
            controls[0] = 0x3f;
            break;
        }
        if ((AI_INT(0x38) > 0x2d0000 || AI_INT(0x38) < -0x2d0000) && AI_INT(0) >= 0x190000) {
            controls[2] = 0;
            controls[3] = 0;
        }
    }
    if (CGameInfo::IsActiveCheatEnabled(0) && StageObject_GetTypeTableIndexValue(car))
        force = 1;
    else
        force = controls[4];
    if (preview != 0)
        return;
    pCar->flag0x1d0[0] = 0;
    pCar->flag0x1d0[1] = 0;
    pCar->flag0x1d0[2] = 0;
    pCar->flag0x1d0[3] = 0;
    pCar->handbrake = 0;
    if (controls[0] >= 1)
        pCar->flag0x1d0[0] = 0x3f;
    if (controls[1] >= 1)
        pCar->flag0x1d0[1] = 0x3f;
    if (controls[2] > 0)
        pCar->flag0x1d0[2] = 0x3f;
    if (controls[3] > 0)
        pCar->flag0x1d0[3] = 0x3f;
    if (force > 0)
        pCar->handbrake = 1;
}

// CPU driving of the car in race order slot `slot`.
// FUNCTION: CMR2 0x0047b620
void StageObject_DriveCPUOrderSlot(int slot)
{
    if ((char)RallyData_GetSelectionFlag26())
        AI_UpdateRouteDrivingControls(g_controlCar, slot, 0);
}

// Per-frame input of the car in race order slot `slot`: player cars read their
// device, CPU cars are driven by the AI; the automatic gearbox is engaged.
// FUNCTION: CMR2 0x0047b000
void CarInput_UpdateRaceOrderSlot(int slot)
{
    char device;

    g_controlCar = Car_Get(Car_GetOrder()[slot]);
    g_controlCar->field_0x1dc = 0;
    g_controlCar->handbrake = 0;
    *((BYTE *)g_controlCar + 0x1d4) = 0;
    g_controlCar->flag0x1d0[3] = 0;
    g_controlCar->flag0x1d0[2] = 0;
    g_controlCar->flag0x1d0[1] = 0;
    g_controlCar->flag0x1d0[0] = 0;
    if ((char)RallyData_GetFlag22())
        CGameInfo::GetConfiguredGameMode();
    device = (char)Car_GetDrawnFlag(g_controlCar->index);
    if (device != -1)
        StageObject_UpdatePlayerControlIndicators(slot, device);
    else
        StageObject_DriveCPUOrderSlot(slot);
    if (g_controlCar->automaticGearbox == 0) {
        if (g_controlCar->field_0xb48 != 1)
            Car_ResetWheelLoadsAfterShift(g_controlCar);
        g_controlCar->automaticGearbox = 1;
        return;
    }
    g_controlCar->automaticGearbox = 1;
}

// Per-frame input of a car waiting at the start line: players keep their
// device, CPU cars blip the throttle at random intervals.
// FUNCTION: CMR2 0x0047b640
void CarInput_UpdateWaitingStartSlot(int slot)
{
    char device;
    char count;
    short *pOrder;

    pOrder = Car_GetOrder();
    Car_GetOrderCount();
    g_controlCar = Car_Get(pOrder[slot]);
    g_controlCar->field_0x1e0 = 0;
    g_controlCar->field_0x1dc = 0;
    g_controlCar->handbrake = 0;
    *((BYTE *)g_controlCar + 0x1d4) = 0;
    g_controlCar->flag0x1d0[3] = 0;
    g_controlCar->flag0x1d0[2] = 0;
    g_controlCar->flag0x1d0[1] = 0;
    g_controlCar->flag0x1d0[0] = 0;
    device = (char)Car_GetDrawnFlag(g_controlCar->index);
    if (device != -1) {
        StageObject_UpdatePlayerControlIndicators(slot, device);
    } else {
        count = *(char *)&g_controlCar->field_0xb47;
        if (count < 0) {
            *(char *)&g_controlCar->field_0xb47 = count + 1;
            if (*(char *)&g_controlCar->field_0xb47 == 0)
                goto reroll;
        } else if (count < 1) {
        reroll:
            *(char *)&g_controlCar->field_0xb47 = (char)(rand() % 10) + 5;
        } else {
            *(char *)&g_controlCar->field_0xb47 = count - 1;
            if (*(char *)&g_controlCar->field_0xb47 == 0)
                *(char *)&g_controlCar->field_0xb47 = -5 - (char)(rand() % 10);
        }
        if (*(char *)&g_controlCar->field_0xb47 > 0)
            g_controlCar->flag0x1d0[2] = 0x3f;
        else
            g_controlCar->flag0x1d0[2] = 0;
    }
    g_controlCar->handbrake = 1;
    g_controlCar->field_0x1dc = 0;
    g_controlCar->automaticGearbox = 0;
}

// Input of a car that has finished: driven as usual, then braked to a stop.
// FUNCTION: CMR2 0x0047b7b0
void StageObject_BrakeFinishedCarSlot(int slot)
{
    CarInput_UpdateRaceOrderSlot(slot);
    g_controlCar = Car_Get(Car_GetOrder()[slot]);
    if (g_controlCar->automaticReverse != 0)
        g_controlCar->flag0x1d0[3] = 0;
    else
        g_controlCar->flag0x1d0[3] = 1;
    g_controlCar->flag0x1d0[2] = 0;
    g_controlCar->handbrake = 1;
    g_controlCar->automaticGearbox = 0;
    g_controlCar->field_0x1e4 = 0;
    FixVecScale(&g_controlCar->velocity, &g_controlCar->velocity, 0xf851);
}

// Loads the stage's AI route data: the three difficulty files, the one for the
// current difficulty copied into the stage buffer, and the pointer tables into
// it (routes, 0x88-byte tables, 0x14-, 0x10- and 0x20-byte records). Returns
// the end of the data.
// FUNCTION: CMR2 0x0047c2f0
BYTE *AI_LoadStageRouteTables(void)
{
    BYTE *files[3];
    DWORD sizes[3];
    DWORD size;
    BYTE *pData;
    BYTE *pEnd;
    BYTE level;
    BYTE *p;
    AINetwork *pEntry;
    int i;
    int k;

    pData = StageTiming_GetStageArchiveState();
    size = 0;
    sprintf(CFrontend::m_stringDest, g_strAi0Format, GameMenu_GetChampionshipTransitionState());
    files[0] = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), CFrontend::m_stringDest,
                                                    0, &size, 0);
    sizes[0] = size;
    size = 0;
    sprintf(CFrontend::m_stringDest, g_strAi1Format, GameMenu_GetChampionshipTransitionState());
    files[1] = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), CFrontend::m_stringDest,
                                                    0, &size, 0);
    sizes[1] = size;
    size = 0;
    sprintf(CFrontend::m_stringDest, g_strAi2Format, GameMenu_GetChampionshipTransitionState());
    files[2] = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), CFrontend::m_stringDest,
                                                    0, &size, 0);
    sizes[2] = size;
    if ((char)RallyData_GetFlag24() == 0)
        return pData;
    level = CGameInfo::GetConfiguredDifficulty();
    if (pData == NULL) {
        for (i = 0; i < 3; i++) {
            if (files[i] != NULL) {
                pData = files[i];
                break;
            }
        }
        if (i == 3)
            pData = pEnd;
    }
    if (pData != files[level])
        memcpy(pData, files[level], sizes[level]);
    pEnd = pData + sizes[level];
    if (pData == NULL)
        return pEnd;
    g_stageAiData = pData;
    p = pData;
    for (i = 0; i < 6; i++) {
        g_unk0x0058e37c[i] = (int *)p;
        p += 4;
    }
    g_aiRouteTables.counts[1] = *p;
    p += 4;
    for (i = 1; i <= g_aiRouteTables.counts[1]; i++) {
        g_aiRouteTables.routes[i] = p;
        p += 0x88;
    }
    g_aiRouteTables.counts[2] = *p;
    p += 4;
    for (i = 1; i <= g_aiRouteTables.counts[2]; i++) {
        g_aiRouteTables.descriptors[i] = (char *)p;
        p += 0x14;
    }
    g_aiRouteTables.counts[0] = *p;
    p += 4;
    for (i = 1; i <= g_aiRouteTables.counts[0]; i++) {
        g_aiRouteTables.points[i] = p;
        p += 0x10;
    }
    g_aiRouteTables.counts[3] = *p;
    p += 4;
    // PORT: the network records hold 32-bit slots for the weight pointers set
    // below; they are copied to native AINetworks, freed with the file data.
    {
        AINetwork *pNative = (AINetwork *)CFileBuffer::AllocateLockedBuffer(
            g_aiRouteTables.counts[3] * sizeof(AINetwork) + 1);

        for (i = 1; i <= g_aiRouteTables.counts[3]; i++) {
            pEntry = &pNative[i - 1];
            memset(pEntry, 0, sizeof(*pEntry));
            memcpy(pEntry, p, offsetof(AINetwork, pWeights));
            g_aiRouteTables.networks[i] = pEntry;
            p += 0x20;
        }
        Disk_AttachToBuffer(pData, pNative);
    }
    for (i = 1; i <= g_aiRouteTables.counts[3]; i++) {
        pEntry = g_aiRouteTables.networks[i];
        if (1 < pEntry->layerCount) {
            for (k = 1; k < pEntry->layerCount; k++) {
                pEntry->pWeights[k] = (int *)p;
                p += (pEntry->layerSizes[k - 1] + 1) * pEntry->layerSizes[k] * 4;
            }
        }
    }
    g_aiRouteTables.pActivation = (int *)p;
    g_aiRouteNodes = (AIRouteNodeInfo *)(p + 0x100);
    StageTiming_RelocateCarRecordRoutePoints((BYTE *)(g_aiRouteNodes + 370), g_aiRouteTables.points, g_aiRouteTables.counts[0]);
    return pEnd;
}
#undef AI_INT
#undef AI_CHAR

void Replay_InitObjectListCarState(ReplayInputLane *pLane, BYTE param_2);
void Replay_RestoreCarTorqueAndRecordState(int *pState, BYTE car);
int Replay_DecodeCarControls(ReplayStream *pState, BYTE *pIn, BYTE car, BYTE *pCounter);
struct ReplayStream;
void Replay_InterpolatePoseStream(ReplayStream *p);
void CarPhysics_UpdateBodyContactAndSkidTrail(Car *pCar);
void StageObject_AnimateCarLightLevels(int car);
void CarLight_UpdateFlagsAndGlows(Car *pCar);
void StageTiming_UpdateAllViewWeather(void);

// Plays back one frame of a replay stream: at the start of a section it
// restores the recorded car state, then feeds the recorded controls and picks
// up the camera event of the current frame. Ends the replay after the last
// section.
// FUNCTION: CMR2 0x0046cfa0
void Replay_PlayStreamFrame(ReplayStream *pState)
{
    ReplayStream *p;
    Car *pCar;
    ReplayEvent *pEvents;
    ReplayEvent *pEvent;
    ReplayEvent *pEvent2;
    int count;
    int k;
    char found;

    p = pState;
    if (p == NULL)
        return;
    pCar = Car_Get(p->car);
    if (p->playing == 0 || p->type == 2 || pCar->simulationStepsRemaining <= 0u)
        return;
    if (p->playStarted != 0) {
        if (p->frame < 0) {
            if (p->type == 0)
                Replay_InitObjectListCarState(p->pInputs + p->lane, p->car);
            else
                Replay_RestoreCarTorqueAndRecordState((int *)(p->pStates + p->lane), p->car);
            if (p->frame != -1) {
                for (k = 0; k < 4; k++)
                    pCar->wheelLoad[k] = 0;
            }
            p->frame++;
        }
        if (p->frame < 0)
            return;
        if (p->pLaneSamples[p->lane] != 0) {
            if (Replay_DecodeCarControls(pState, p->pFrames + (p->samplesPerLane * p->lane + p->frame) * 4, p->car,
                             &p->field_0x21))
                p->frame++;
            found = -1;
            if (p->type == 0) {
                count = p->pInputs[p->lane].eventCount;
                pEvents = p->pInputs[p->lane].events;
                for (k = 0; k < count; k++) {
                    pEvent = &pEvents[k];
                    if (p->frame < pEvent->frame ||
                        (p->frame == pEvent->frame && (short)p->field_0x21 < pEvent->count)) {
                        found = (char)k;
                        k = count;
                    }
                }
            } else {
                count = p->pStates[p->lane].eventCount;
                pEvents = p->pStates[p->lane].events;
                for (k = 0; k < count; k++) {
                    pEvent2 = &pEvents[k];
                    if (p->frame < pEvent2->frame ||
                        (p->frame == pEvent2->frame && (short)p->field_0x21 < pEvent2->count)) {
                        found = (char)k;
                        k = count;
                    }
                }
            }
            if (found == -1) {
                if (p->type == 0)
                    found = p->pInputs[p->lane].eventCount;
                else
                    found = p->pStates[p->lane].eventCount;
            }
            // Both formats index the event table from its base. In the original
            // assembly the final load follows two pops, so its stack offset
            // refers to pEvents, not to the last event examined by the loop.
            found--;
            if (found >= 0)
                p->field_0x10c = pEvents[found].value;
        }
        if (p->pLaneSamples[p->lane] == p->frame) {
            p->playStarted = 0;
            p->lane++;
            if (p->lane == p->laneCount)
                Replay_ResetBufferIfActive(pState);
        }
    } else {
        p->frame = 0;
        p->field_0x21 = 0;
        p->field_0x14 = 1;
        if (p->type == 0) {
            p->playStarted = 1;
            p->pInputLane = p->pInputs + p->lane;
        } else {
            p->playStarted = 1;
            p->pStateLane = p->pStates + p->lane;
        }
    }
}

// Plays back one frame of every replay stream (in single-player time trial
// only the non-player ones).
// FUNCTION: CMR2 0x0046d270
void Replay_PlaybackAllStreamFrames(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (CGameInfo::GetGameInfoSessionFlag() == 0 || CGameInfo::GetConfiguredGameMode() != 6)
            Replay_PlayStreamFrame(*g_replaySlots[i]);
    }
}

// Records one frame of every replay stream (same condition as Replay_PlaybackAllStreamFrames).
// FUNCTION: CMR2 0x0046d5e0
void Replay_RecordAllStreamFrames(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (CGameInfo::GetGameInfoSessionFlag() == 0 || CGameInfo::GetConfiguredGameMode() != 6)
            Replay_InterpolatePoseStream(*g_replaySlots[i]);
    }
}

// Restarts the replay of the ghost car, keeping its controller index.
// FUNCTION: CMR2 0x00466030
void Replay_RestartGhostCar(int a, int b)
{
    unsigned int index;

    index = (BYTE)g_ghostCar->index;
    Replay_SelectAndInitializeLane(g_activeReplayStream, a, b, index);
    g_ghostCar->index = index;
    g_ghostCar->isGhost = 1;
}

// Per-frame physics of the cars in `pOrder` that are not replayed, then the
// stage weather.
// FUNCTION: CMR2 0x004664c0
void StageObject_UpdateListedCarPhysicsAndWeather(short *pOrder, short count)
{
    int i;
    Car *pCar;

    LightField_Clear();   // the glow updates below re-record this step's lights
    for (i = 0; i < count; i++, pOrder++) {
        pCar = Car_Get(*pOrder);
        if (pCar->isGhost == 0) {
            if (pCar->field_0xb70 == 0)
                CarPhysics_UpdateBodyContactAndSkidTrail(pCar);
            StageObject_AnimateCarLightLevels(i);
            CarLight_UpdateFlagsAndGlows(pCar);
        }
    }
    StageTiming_UpdateAllViewWeather();
}

// Picks a random point on a random edge of a random triangle of mesh `index`
// of a car's damage parts (vertices are floats, 0x30 bytes apart). Returns 0
// when the part has no mesh.
// FUNCTION: CMR2 0x00469c30
int CarDamage_PickRandomTriangleEdgePoint(FixVector *pOut, CarPartSet *pParts, int index)
{
    MeshTriangle *pTri;
    int triangleIndex;
    float *pA;
    float *pB;
    DeformFloatVertex *pVerts;
    int t;
    int edge;
    FixVector a;
    FixVector d;

    if (pParts->geometry.meshes[index] == 0) {
        pOut->x = 0;
        pOut->y = 0;
        pOut->z = 0;
        return 0;
    }
    triangleIndex = rand() % pParts->geometry.meshes[index]->triangleCount;
    edge = rand() % 3;
    t = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536));
    pTri = pParts->geometry.meshes[index]->pTriangles + triangleIndex;
    switch (edge) {
    case 0:
        pVerts = (DeformFloatVertex *)pParts->geometry.meshes[index]->pVertexData;
        pA = pVerts[pTri->vertexIndex[0]].pos;
        pB = pVerts[pTri->vertexIndex[1]].pos;
        break;
    case 1:
        pVerts = (DeformFloatVertex *)pParts->geometry.meshes[index]->pVertexData;
        pA = pVerts[pTri->vertexIndex[1]].pos;
        pB = pVerts[pTri->vertexIndex[2]].pos;
        break;
    default:
        pVerts = (DeformFloatVertex *)pParts->geometry.meshes[index]->pVertexData;
        pA = pVerts[pTri->vertexIndex[2]].pos;
        pB = pVerts[pTri->vertexIndex[0]].pos;
        break;
    }
    a.x = (int)FTOI(__int64, X87S_MUL(pA[0], CGraphics::m_65536));
    a.y = (int)FTOI(__int64, X87S_MUL(pA[1], CGraphics::m_65536));
    a.z = (int)FTOI(__int64, X87S_MUL(pA[2], CGraphics::m_65536));
    d.x = (int)FTOI(__int64, X87S_MUL(pB[0], CGraphics::m_65536)) - a.x;
    d.y = (int)FTOI(__int64, X87S_MUL(pB[1], CGraphics::m_65536)) - a.y;
    d.z = (int)FTOI(__int64, X87S_MUL(pB[2], CGraphics::m_65536)) - a.z;
    FixVecScale(&d, &d, t);
    pOut->x = d.x + a.x;
    pOut->y = d.y + a.y;
    pOut->z = d.z + a.z;
    return 1;
}

// GLOBAL: CMR2 0x00547908
BillboardDef g_bodySparkBillboard;

extern int g_stageObjectTimeStep;
extern Texture *g_splat2Texture;
CarPartSet *CarPart_GetSet(int carIndex);
int FixMatrix_RotateVector(FixVector *pOut, FixVector *pV, FixMatrix *pM);


struct Quad2DVertices;
struct Quad2D;
void Quad2D_Queue(Quad2DVertices *pVerts, Texture *pTexture, UINT_PTR pDest);
int CarDamage_EmitBodySparkBillboards(int amount, Car *pCar);
void Scene_GetLightColour(DWORD *pColour, int level);
extern PrecipitationQuad g_precipitation;
extern Texture *g_rainTexture;
extern Texture *g_splatTexture;
extern Texture *g_snowTextures[3];
extern BillboardDef g_rainSplashBillboard;
extern BillboardDef g_snowBillboard;
extern double g_fixedDegreesToAngle12;

extern Texture *g_rainTexture, *g_splatTexture, *g_snowTextures[3];

// Draws the precipitation of one view. Rain: each drop is a streak quad
// along the wind (+0x20) widened across the view, plus a splash billboard for
// the drops that hit the ground, then the car throws spray. Snow: each flake
// is a billboard swaying along +0x38 with the sine of its phase.
extern Texture *g_rainTexture;
// FUNCTION: CMR2 0x00460390
void StageWeather_DrawViewPrecipitation(int index, int view)
{
    ViewWeatherState *pView;
    FixVector *pPos;
    int count;
    int i;
    int fade;
    int amount;
    BYTE alpha;
    FixVector saved;
    FixVector forward;
    FixVector side;
    FixVector trail;
    FixVector offset;
    float sideF[3];
    float trailF[3];
    StageWeatherParticle *pNode;

    alpha = 0x1e;
    pView = &g_viewWeather[index];
    // PORT: video.weather_particle_amount 0 hides precipitation (the original
    // sim still runs untouched - only this draw is skipped); >100 adds the
    // port sidecar slice after each original slice below.
    if (!WeatherSidecar_PrecipitationDrawn(WeatherSidecar_Amount()))
        return;
    int sidecarActive = WeatherSidecar_ActiveCount(WeatherSidecar_Amount());
    int sideFirst = 0, sideCapacity = 0;
    WeatherSidecar_ViewSlice(pView->firstParticle, pView->particleCapacity, sidecarActive, sideFirst, sideCapacity);
    pPos = &pView->centre;
    count = pView->particleCount >> 16;
    saved = *pPos;
    if (pView->kind == 1) {
        fade = pView->precipitationFade;
        if (fade == 0x10000) {
            g_rainSplashBillboard.a = 0xaa;
            g_bodySparkBillboard.a = 0xaa;
        } else if (fade == 0) {
            alpha = 0;
            g_rainSplashBillboard.a = 0;
            g_bodySparkBillboard.a = 0;
        } else {
            alpha = (BYTE)(FixMul(0x1e0000, fade) >> 16);
            g_rainSplashBillboard.a = (BYTE)(FixMul(pView->precipitationFade, 0xaa0000) >> 16);
            g_bodySparkBillboard.a = (BYTE)(FixMul(pView->precipitationFade, 0xaa0000) >> 16);
        }
        g_precipitation.quad.v[1].colour = g_precipitation.quad.v[2].colour =
            (alpha << 24) | 0x969696;
        FixMatrix_GetForward(&forward, &g_viewNodes[view]->current);
        FixVecCross(&side, &forward, &pView->relativeVelocity);
        FIX_NORMALIZE_INTO(side, side);
        amount = FixMul(pView->particleCount, g_precipitation.particleScale);
        if (amount > 0x10000)
            amount = 0x10000;
        FixVecScale(&side, &side, FixMul(0xccc, FixMul(0xcccd, amount) + 0x13333));
        sideF[0] = (float)(X87S_MUL(side.x, CGraphics::m_oneOver65536));
        sideF[1] = (float)(X87S_MUL(side.y, CGraphics::m_oneOver65536));
        sideF[2] = (float)(X87S_MUL(side.z, CGraphics::m_oneOver65536));
        FixVecScale(&trail, &pView->relativeVelocity, 0x20000);
        trailF[0] = (float)(X87S_MUL(trail.x, CGraphics::m_oneOver65536));
        trailF[1] = (float)(X87S_MUL(trail.y, CGraphics::m_oneOver65536));
        trailF[2] = (float)(X87S_MUL(trail.z, CGraphics::m_oneOver65536));
        for (i = 0; i < count; i++) {
            pNode = &g_weatherParticles[pView->firstParticle + i];
            g_precipitation.quad.v[2].x = (float)(X87S_MUL(pNode->x, CGraphics::m_oneOver65536));
            g_precipitation.quad.v[2].y = (float)(X87S_MUL(pNode->y, CGraphics::m_oneOver65536));
            g_precipitation.quad.v[2].z = (float)(X87S_MUL(pNode->z, CGraphics::m_oneOver65536));
            g_precipitation.quad.v[0].x = g_precipitation.quad.v[2].x - trailF[0];
            g_precipitation.quad.v[0].y = g_precipitation.quad.v[2].y - trailF[1];
            g_precipitation.quad.v[0].z = g_precipitation.quad.v[2].z - trailF[2];
            g_precipitation.quad.v[1].x = g_precipitation.quad.v[2].x + sideF[0];
            g_precipitation.quad.v[1].y = g_precipitation.quad.v[2].y + sideF[1];
            g_precipitation.quad.v[1].z = g_precipitation.quad.v[2].z + sideF[2];
            g_precipitation.quad.v[2].x -= sideF[0];
            g_precipitation.quad.v[2].y -= sideF[1];
            g_precipitation.quad.v[2].z -= sideF[2];
            Quad2D_Queue(&g_precipitation.quad, g_rainTexture, 0x16);
            if (pNode->wrapped != 0) {
                forward = *(FixVector *)pNode;
                forward.y -= 0xc0000;
                FixVecScale(&offset, &pView->splashDisplacement,
                            FixMul(pNode->splashGroundHeight - forward.y, pView->inverseFallDisplacement));
                g_rainSplashBillboard.pos.x = offset.x + forward.x;
                g_rainSplashBillboard.pos.y = offset.y + forward.y;
                g_rainSplashBillboard.pos.z = offset.z + forward.z;
                Billboard_Add(&g_rainSplashBillboard, g_splatTexture);
            }
        }
        // PORT: sidecar drops through the same streak/splash path.
        int sideDraw = WeatherSidecar_DrawCount(count, pView->particleCapacity, sidecarActive);
        for (i = 0; i < sideDraw; i++) {
            pNode = &g_weatherSidecar[sideFirst + i];
            g_precipitation.quad.v[2].x = (float)(X87S_MUL(pNode->x, CGraphics::m_oneOver65536));
            g_precipitation.quad.v[2].y = (float)(X87S_MUL(pNode->y, CGraphics::m_oneOver65536));
            g_precipitation.quad.v[2].z = (float)(X87S_MUL(pNode->z, CGraphics::m_oneOver65536));
            g_precipitation.quad.v[0].x = g_precipitation.quad.v[2].x - trailF[0];
            g_precipitation.quad.v[0].y = g_precipitation.quad.v[2].y - trailF[1];
            g_precipitation.quad.v[0].z = g_precipitation.quad.v[2].z - trailF[2];
            g_precipitation.quad.v[1].x = g_precipitation.quad.v[2].x + sideF[0];
            g_precipitation.quad.v[1].y = g_precipitation.quad.v[2].y + sideF[1];
            g_precipitation.quad.v[1].z = g_precipitation.quad.v[2].z + sideF[2];
            g_precipitation.quad.v[2].x -= sideF[0];
            g_precipitation.quad.v[2].y -= sideF[1];
            g_precipitation.quad.v[2].z -= sideF[2];
            Quad2D_Queue(&g_precipitation.quad, g_rainTexture, 0x16);
            if (pNode->wrapped != 0) {
                forward = *(FixVector *)pNode;
                forward.y -= 0xc0000;
                FixVecScale(&offset, &pView->splashDisplacement,
                            FixMul(pNode->splashGroundHeight - forward.y, pView->inverseFallDisplacement));
                g_rainSplashBillboard.pos.x = offset.x + forward.x;
                g_rainSplashBillboard.pos.y = offset.y + forward.y;
                g_rainSplashBillboard.pos.z = offset.z + forward.z;
                Billboard_Add(&g_rainSplashBillboard, g_splatTexture);
            }
        }
        if (CGameInfo::IsInRaceMenuOpen() == 0) {
            amount = FixMul(pView->particleCount, g_precipitation.particleScale);
            if (amount > 0x10000)
                amount = 0x10000;
            CarDamage_EmitBodySparkBillboards(amount, Car_Get(View_GetActiveCameraFlags((BYTE)view)));
        }
    } else {
        Scene_GetLightColour((DWORD *)&g_snowBillboard.r, 0xcccc);
        // Snow glints (video.snow_glint): flakes twinkle in the sun (strongest
        // looking toward it - forward scatter) and brighten inside car light
        // beams. Per-flake phase from its own sway cycle, deterministic.
        int glintAmount = Sys_GetOption("video.snow_glint", 200);
        float fwdScatter = 0.f, sunLevelF = 0.f, sunDirX = 0.f, sunDirY = 0.f, sunDirZ = 0.f;
        if (glintAmount > 0) {
            FixVector sunV;
            StageObject_GetRotatedStageLightVector(&sunV);
            float sl = sqrtf((float)sunV.x * sunV.x + (float)sunV.y * sunV.y + (float)sunV.z * sunV.z);
            if (sl > 1e-3f) {
                sunDirX = sunV.x / sl; sunDirY = sunV.y / sl; sunDirZ = sunV.z / sl;
                int wl = StageObject_GetWeatherLightIntensity();
                sunLevelF = wl <= 0 ? 0.f : wl >= 0x10000 ? 1.f : wl / 65536.f;
                if (g_viewNodes[view]) {
                    FixVector vf;
                    FixMatrix_GetForward(&vf, &g_viewNodes[view]->world);
                    float dot = (vf.x * sunV.x + vf.y * sunV.y + vf.z * sunV.z) / (65536.f * sl) ;
                    float d = dot > 0.f ? dot : 0.f, fs = d;
                    for (int fsq = 0; fsq < 3; ++fsq) fs *= fs;   // d^8 forward lobe
                    fwdScatter = fs * sunLevelF;
                }
            }
        }
        for (i = 0; i < count; i++) {
            pNode = &g_weatherParticles[pView->firstParticle + i];
            g_snowBillboard.pos = *(FixVector *)pNode;
            FixVecScale(&offset, &pView->snowSwayAxis,
                        FixMul(g_sinTable[(unsigned short)FTOI(__int64, X87S_MUL(pNode->snowPhaseDegrees, g_fixedDegreesToAngle12)) & 0xfff],
                               pNode->swayAmplitude));
            g_snowBillboard.pos.x += offset.x;
            g_snowBillboard.pos.y += offset.y;
            g_snowBillboard.pos.z += offset.z;
            if (glintAmount > 0) {
                float phaseTurns = pNode->snowPhaseDegrees / 360.f;
                float m = SnowGlint_FlakeTwinkle((unsigned)(pView->firstParticle + i) * 0x45d9f3bu + 7u,
                                                 phaseTurns, fwdScatter);
                // Car light beams: flakes inside a beam brighten toward the light's tint.
                float boost = 0.f;
                for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX; ++li) {
                    FogLightSource *L = &g_fogLightSources[li];
                    boost += SnowGlint_ConeBoost(g_snowBillboard.pos.x / 65536.f, g_snowBillboard.pos.y / 65536.f,
                                                 g_snowBillboard.pos.z / 65536.f,
                                                 L->pos.x / 65536.f, L->pos.y / 65536.f, L->pos.z / 65536.f,
                                                 L->dir.x / 65536.f, L->dir.y / 65536.f, L->dir.z / 65536.f,
                                                 L->intensity / 65536.f);
                }
                m *= (glintAmount / 100.f);
                m += boost * 1.2f;
                int sr = (int)(g_snowBillboard.r * m); if (sr > 255) sr = 255;
                int sg = (int)(g_snowBillboard.g * m); if (sg > 255) sg = 255;
                int sb = (int)(g_snowBillboard.b * m); if (sb > 255) sb = 255;
                BYTE savedR = g_snowBillboard.r, savedG = g_snowBillboard.g, savedB = g_snowBillboard.b;
                g_snowBillboard.r = (BYTE)sr; g_snowBillboard.g = (BYTE)sg; g_snowBillboard.b = (BYTE)sb;
                Billboard_Add(&g_snowBillboard, g_snowTextures[pNode->textureVariant]);
                g_snowBillboard.r = savedR; g_snowBillboard.g = savedG; g_snowBillboard.b = savedB;
            } else {
                Billboard_Add(&g_snowBillboard, g_snowTextures[pNode->textureVariant]);
            }
        }
        // PORT: sidecar flakes through the same sway/glint path; the glint
        // hash seed is offset by 400 so sidecar phases decorrelate.
        int sideDraw = WeatherSidecar_DrawCount(count, pView->particleCapacity, sidecarActive);
        for (i = 0; i < sideDraw; i++) {
            pNode = &g_weatherSidecar[sideFirst + i];
            g_snowBillboard.pos = *(FixVector *)pNode;
            FixVecScale(&offset, &pView->snowSwayAxis,
                        FixMul(g_sinTable[(unsigned short)FTOI(__int64, X87S_MUL(pNode->snowPhaseDegrees, g_fixedDegreesToAngle12)) & 0xfff],
                               pNode->swayAmplitude));
            g_snowBillboard.pos.x += offset.x;
            g_snowBillboard.pos.y += offset.y;
            g_snowBillboard.pos.z += offset.z;
            if (glintAmount > 0) {
                float phaseTurns = pNode->snowPhaseDegrees / 360.f;
                float m = SnowGlint_FlakeTwinkle((unsigned)(400 + sideFirst + i) * 0x45d9f3bu + 7u,
                                                 phaseTurns, fwdScatter);
                float boost = 0.f;
                for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX; ++li) {
                    FogLightSource *L = &g_fogLightSources[li];
                    boost += SnowGlint_ConeBoost(g_snowBillboard.pos.x / 65536.f, g_snowBillboard.pos.y / 65536.f,
                                                 g_snowBillboard.pos.z / 65536.f,
                                                 L->pos.x / 65536.f, L->pos.y / 65536.f, L->pos.z / 65536.f,
                                                 L->dir.x / 65536.f, L->dir.y / 65536.f, L->dir.z / 65536.f,
                                                 L->intensity / 65536.f);
                }
                m *= (glintAmount / 100.f);
                m += boost * 1.2f;
                int sr = (int)(g_snowBillboard.r * m); if (sr > 255) sr = 255;
                int sg = (int)(g_snowBillboard.g * m); if (sg > 255) sg = 255;
                int sb = (int)(g_snowBillboard.b * m); if (sb > 255) sb = 255;
                BYTE savedR = g_snowBillboard.r, savedG = g_snowBillboard.g, savedB = g_snowBillboard.b;
                g_snowBillboard.r = (BYTE)sr; g_snowBillboard.g = (BYTE)sg; g_snowBillboard.b = (BYTE)sb;
                Billboard_Add(&g_snowBillboard, g_snowTextures[pNode->textureVariant]);
                g_snowBillboard.r = savedR; g_snowBillboard.g = savedG; g_snowBillboard.b = savedB;
            } else {
                Billboard_Add(&g_snowBillboard, g_snowTextures[pNode->textureVariant]);
            }
        }
    }
    *pPos = saved;
}


// Per-view stage objects draw: the view's precipitation, then the object
// pass when racing, replaying or in the demo.
// FUNCTION: CMR2 0x00460330
void StageObject_DrawViewPrecipitationAndObjects(int *pViewRect, int view)
{
    ViewWeatherState *pType;

    pType = &g_viewWeather[view];
    if (pType->kind == 1 || pType->kind == 2)
        StageWeather_DrawViewPrecipitation(view, view);
    if ((char)RallyDataState() != 1 && Race_IsMultiplayerRecordMode10() == 0 && CGameInfo::IsConfiguredMultiplayer() == 0)
        return;
    StageObject_DrawProjectedViewIcon(pViewRect, view);
}

// Sparks and glints thrown off a car's body: `amount` (per second) of random
// points on its damage parts get a billboard of the spark texture.
// FUNCTION: CMR2 0x00460ca0
int CarDamage_EmitBodySparkBillboards(int amount, Car *pCar)
{
    CarPartSet *pParts;
    int count;
    int result;
    int n;
    int part;
    FixVector point;
    FixVector rotated;
    FixVector origin;

    pParts = CarPart_GetSet(pCar->index);
    count = FixMul(0x1e0000, FixMul(g_stageObjectTimeStep, amount)) >> 16;
    result = count;
    if (count > 0) {
        result = rand();
        n = result % count;
        result /= count;
        for (; n > 0; n--) {
            part = rand() % pParts->count;
            result = CarDamage_PickRandomTriangleEdgePoint(&point, pParts, part);
            if (result != 0) {
                result = point.y;
                if (point.y > 0) {
                    FixMatrix_RotateVector(&rotated, &point, &pParts->geometry.nodes[part]->world);
                    FixMatrix_GetPosition(&origin, &pParts->geometry.nodes[part]->world);
                    rotated.x += origin.x;
                    rotated.y += origin.y;
                    rotated.z += origin.z;
                    g_bodySparkBillboard.pos = rotated;
                    Billboard_Add(&g_bodySparkBillboard, g_splat2Texture);
                }
            }
        }
    }
    return result;
}

void Replay_RecordCarPoseSample(ReplaySample *pSample, BYTE index);
int Replay_StopRecording(BYTE *pBuffer);

// Records the car states of the replay streams of type 2 (every third frame):
// one 16-byte sample per call into the current section, until it is full.
// FUNCTION: CMR2 0x0046d510
void Replay_RecordPeriodicCarSamples(void)
{
    int i;
    ReplayStream *p;
    short *pCount;
    short n;

    for (i = 0; i < 16; i++) {
        if (g_replaySlots[i] == NULL)
            continue;
        p = *g_replaySlots[i];
        if (p == NULL || p->recording == 0 || p->type != 2)
            continue;
        if (Car_Get(p->car)->simulationStepsRemaining <= 0u)
            continue;
        if (p->step == 0) {
            pCount = p->pLaneSamples + p->laneCount;
            n = *pCount;
            if (n < p->samplesPerLane) {
                Replay_RecordCarPoseSample((ReplaySample *)p->pSamples + p->samplesPerLane * p->laneCount + n, p->car);
                (*pCount)++;
            } else {
                Replay_StopRecording((BYTE *)p);
            }
        }
        p->step++;
        if (p->step >= 3)
            p->step = 0;
    }
}

extern FixVector g_collisionPush;
extern FixVector g_collisionContactNormal;
extern int g_contactImpactLowThreshold;
extern int g_contactImpactHighThreshold;
extern int g_collisionObjectHasBox;
int Collision_TestOrientedBoxCornerOverlap(CollisionBox *pBoxA, CollisionBox *pBoxB, FixVector *pOffset, int scale);
int FixMatrix_InverseRotateVector(FixVector *pOut, FixVector *pV, FixMatrix *pM);

// Resolves a contact between a car's box and a turned stage object's box
// (0x5915f8): the push-out direction is the sum of the contact edge normals,
// the contact point the mean of the contact corners (in car space). Unless
// the object is pushable, a one-sided contact also moves the object. Returns
// whether the boxes overlapped.
// FUNCTION: CMR2 0x00487f60
int Collision_ResolveCarTurnedObjectContact(Car *pCar, StageObject **pEntry, CollisionBox *pBox, CollisionBox *pObject)
{
    FixVector sum;
    FixVector point;
    FixVector dir;
    int count;
    int solid;
    int hit;
    int i;
    int d;
    int k;
    FixVector *pEdge;
    int dx;
    int dy;
    int dz;

    count = 0;
    if ((*(unsigned int *)(*pEntry)->field_0x10 & 0x2001000) != 0) {
        solid = 1;
        hit = Collision_TestOrientedBoxCornerOverlap(pBox, pObject, &pCar->positionPrev, 0x10000);
    } else {
        solid = 0;
        hit = Collision_TestOrientedBoxCornerOverlap(pBox, pObject, &pCar->positionPrev, 0);
    }
    if (hit != 0) {
        sum.x = 0;
        sum.y = 0;
        sum.z = 0;
        dir.x = 0;
        dir.y = 0;
        dir.z = 0;
        if (g_unk0x005915f4 > 0) {
            for (i = 0; i < g_unk0x005915f4; i++) {
                point.x = pCar->corners[g_unk0x005914c4[i]].x - pCar->position.x;
                point.y = pCar->corners[g_unk0x005914c4[i]].y - pCar->position.y;
                point.z = pCar->corners[g_unk0x005914c4[i]].z - pCar->position.z;
                sum.x += point.x;
                point.y = 0;
                sum.z += point.z;
                if (g_unk0x00590ec8[i] == 0) {
                    if (FixVecDot(&dir, &pObject->axisA) < 0) {
                        dir.x -= pObject->axisA.x;
                        dir.y -= pObject->axisA.y;
                        dir.z -= pObject->axisA.z;
                    } else {
                        dir.x += pObject->axisA.x;
                        dir.y += pObject->axisA.y;
                        dir.z += pObject->axisA.z;
                    }
                } else {
                    if (FixVecDot(&dir, &pObject->axisB) < 0) {
                        dir.x -= pObject->axisB.x;
                        dir.y -= pObject->axisB.y;
                        dir.z -= pObject->axisB.z;
                    } else {
                        dir.x += pObject->axisB.x;
                        dir.y += pObject->axisB.y;
                        dir.z += pObject->axisB.z;
                    }
                }
            }
            count = g_unk0x005915f4;
        }
        if (g_unk0x005914d4 > 0) {
            for (i = 0; i < g_unk0x005914d4; i++) {
                point = pObject->points[g_unk0x00590ecc[i]];
                point.x -= pCar->position.x;
                point.z -= pCar->position.z;
                sum.x += point.x;
                sum.z += point.z;
                point.y = 0;
                if (g_unk0x005914a4[i] == 0) {
                    if (FixVecDot(&dir, &pBox->axisA) < 0) {
                        dir.x -= pBox->axisA.x;
                        dir.y -= pBox->axisA.y;
                        dir.z -= pBox->axisA.z;
                    } else {
                        dir.x += pBox->axisA.x;
                        dir.y += pBox->axisA.y;
                        dir.z += pBox->axisA.z;
                    }
                } else {
                    if (FixVecDot(&dir, &pBox->axisB) < 0) {
                        dir.x -= pBox->axisB.x;
                        dir.y -= pBox->axisB.y;
                        dir.z -= pBox->axisB.z;
                    } else {
                        dir.x += pBox->axisB.x;
                        dir.y += pBox->axisB.y;
                        dir.z += pBox->axisB.z;
                    }
                }
            }
            count += g_unk0x005914d4;
        }
        FixVecScaleRecip(&sum, &sum, count << 16);
        FIX_NORMALIZE_INTO(dir, dir);
        if (solid == 0 && (g_unk0x005915f4 == 0 || g_unk0x005914d4 == 0)) {
            d = FixVecDot(&g_collisionPush, &dir);
            FixVecScale(&point, &dir, d);
            dx = point.x - g_collisionPush.x;
            dy = point.y - g_collisionPush.y;
            dz = point.z - g_collisionPush.z;
            if (pBox->pVertex != NULL && pBox->pArray != NULL) {
                pBox->pVertex->x += dx;
                pBox->pVertex->y += dy;
                pBox->pVertex->z += dz;
                for (k = 0; k < 8; k++) {
                    pBox->pArray[k].x += dx;
                    pBox->pArray[k].y += dy;
                    pBox->pArray[k].z += dz;
                }
                for (k = 0; k < 4; k++) {
                    pBox->points[k].x += dx;
                    pBox->points[k].y += dy;
                    pBox->points[k].z += dz;
                }
            }
            StageObject_ApplyRecursiveFrameDelta(pCar->index, -1, (int *)&point, 1);
        }
        g_collisionContactNormal = dir;
        FixMatrix_InverseRotateVector(&pCar->field_0x5dc, &sum, pCar->pWorld);
        g_contactImpactLowThreshold = 0x8000;
        g_contactImpactHighThreshold = 0x1570a;
    }
    return hit;
}

SectorObjectEntry **Sector_GetListA(unsigned int sector, unsigned int *pCount);
SectorObjectEntry **Sector_GetListB(unsigned int sector, unsigned int *pCount);
void RallyData_CopyRaisedElementVector(int *pDest, SectorObjectEntry **pParam1);
int RallyData_IsElementFlagSet(SectorObjectEntry **pEntry, int bit);
int Collision_CarVsBox(Car *car, CollisionBox *pBox, int scale);
int Collision_ResolveStaticObstacleContact(Car *pCar, StageObject **ppObject, FixVector *unusedPosition, int contactId);

// Collides a car with the stage objects of the four sectors it touches
// (static objects, then moving ones): box test, then the plain, wall or
// turned-object response, and the object's reaction. Keeps the list of
// touched surfaces. Returns the last response.
// FUNCTION: CMR2 0x004878a0
int Collision_TestCarAgainstSectorObjects(Car *pCar)
{
    CollisionBox *pBox;
    short sectors[4];
    int moving;
    int n;
    int k;
    int i;
    int count;
    int sector;
    int result;
    SectorObjectEntry *pEntry;
    SectorObjectEntry **ppEntries;
    StageObject *pObject;
    FixVector position;

    result = 0;
    pCar->pendingContactCount = 0;
    pBox = &g_carCollisionBoxes[pCar->index];
    memset(pCar->objectContactState, 0, sizeof(pCar->objectContactState));
    memcpy(&sectors[1], pCar->neighbours.s, 2 * sizeof(sectors[1]));
    sectors[0] = pCar->sector;
    sectors[3] = pCar->neighbours.s[2];
    for (moving = pCar->field_0xc18 != 0; moving < 2; moving++) {
        for (k = 0; k < 4; k++) {
            if (sectors[k] < 0)
                continue;
            sector = sectors[k];
            if (moving == 0)
                ppEntries = Sector_GetListA(sector, (unsigned int *)&count);
            else
                ppEntries = Sector_GetListB(sector, (unsigned int *)&count);
            pEntry = ppEntries != NULL ? *ppEntries : NULL;
            for (i = 0; i < count; i++, pEntry++) {
                pObject = pEntry->pObject;
                if (moving == 0)
                    position = pObject->position;
                else
                    RallyData_CopyRaisedElementVector((int *)&position, &pEntry);
                if ((pObject->flags & 0x8000) == 0 && (BYTE)RallyDataState() == 2)
                    continue;
                g_collisionObjectHasBox = pEntry->pBounds->pProjection != 0;
                result = Collision_DoSpheresOverlap(pCar->collisionRadius, pEntry->pBounds->radius, (int *)&pCar->position, (int *)&position);
                if (result == 0)
                    continue;
                // Broad bounds only select a collision candidate. No runtime
                // record currently supplies a verified breakable-tree contract:
                // video.tree_break must not bypass the original detailed test
                // or turn a sign, building or near miss into a broken tree.
                result = 0;
                if (moving == 1 && RallyData_IsElementFlagSet(&pEntry, pCar->index) == 0)
                    continue;
                Collision_BuildCarBox(pBox, (int *)&pCar->right, &pCar->position, pCar->corners);
                result = 0;
                if (g_collisionObjectHasBox != 0) {
                    g_sectorObjectCollisionBox.built = 0;
                    Collision_BuildObjectBox(&g_sectorObjectCollisionBox, pEntry, &position);
                } else {
                    Collision_SetObjectSphere(&position, pEntry);
                }
                if (g_collisionObjectHasBox != 0) {
                    result = Collision_ResolveCarTurnedObjectContact(pCar, &pEntry->pObject, pBox, &g_sectorObjectCollisionBox);
                } else {
                    if (pObject->flags & 0x4000) {
                        if (pCar->field_0xc20 == 0)
                            Collision_ApplyObjectOverlapDamping(pBox, pCar);
                        continue;
                    }
                    if (!((pObject->flags & 0x2001000) != 0)) {
                        result = Collision_CarVsBox(pCar, pBox, 0x10000);
                    } else {
                        result = Collision_CarVsBox(pCar, pBox, 0);
                    }
                }
                if (result != 0 && Collision_ResolveStaticObstacleContact(pCar, &pEntry->pObject, &position, 0) != 0)
                    StageObject_QueueOrEvictMovingObject(pEntry, sector, pCar->index);
            }
        }
    }
    pCar->previousContactCount = pCar->pendingContactCount;
    for (i = 0; i < (char)pCar->previousContactCount; i++)
        pCar->contactSurfaces[0][i] = pCar->contactSurfaces[1][i];
    return result;
}

// Collision scratch of the edge being tested (second end point offsets).
// GLOBAL: CMR2 0x005918d4
int g_unk0x005918d4;
// GLOBAL: CMR2 0x00591948
FinRecord *g_unk0x00591948;
// GLOBAL: CMR2 0x00591960
int g_unk0x00591960;

extern Car *g_collisionCar;
extern FixVector g_collisionTarget;
extern FixVector g_collisionLineStart;
extern FixVector g_collisionDirection;
extern int g_collisionDirectionDirty;
extern int g_unk0x005918d0;
extern int g_unk0x0059195c;
extern int g_unk0x005919b8;
extern CollisionBox *g_collisionFace;
extern int g_unk0x0051fb00[52];
int Collision_ResolveSectorFaceContact(int *param_1, int *param_2, int param_3, char param_4);
int StageObject_IsCarOutsideCollisionHeightInterval(void);
int Collision_ResolveSectorEdgeContact(char type, int param);
int VehiclePhysics_ClassifyCandidateFace(int useParallelSide);
void Sound_NoOpMusicCallback(INT_PTR unused);
void CarPhysics_UpdateWheelSlipAndVelocityDamping(Car *param_1);
void CarDamage_ApplyCollisionDeformImpulse(Car *pCar, int *param_2, FixVector *param_3, int param_4, unsigned char param_5, int param_6);

// Tests the end points of the current sector edge against the car's radius
// and runs the corner collision for each one inside it.
// FUNCTION: CMR2 0x0048e580
int Collision_TestSectorEdgeEndpoints(char type)
{
    int radius;
    int radius2;
    int hitA;
    int hitB;
    FixVector d;

    hitB = 0;
    hitA = 0;
    radius = g_collisionCar->collisionRadius;
    int r = radius;
    radius2 = FixMul(r, radius);
    d.x = g_collisionTarget.x - g_collisionCar->position.x;
    d.y = g_collisionTarget.y - g_collisionCar->position.y;
    d.z = g_collisionTarget.z - g_collisionCar->position.z;
    d.y = 0;
    if ((d.x < 0 ? -d.x : d.x) <= radius && radius >= 0 && (d.z < 0 ? -d.z : d.z) <= radius &&
        FixVecDot(&d, &d) < radius2)
        hitA = Collision_ResolveSectorFaceContact((int *)&g_collisionTarget, (int *)&g_collisionLineStart, 0, type);
    d.x = g_collisionLineStart.x - g_collisionCar->position.x;
    d.y = g_collisionLineStart.y - g_collisionCar->position.y;
    d.z = g_collisionLineStart.z - g_collisionCar->position.z;
    d.y = 0;
    if ((d.x < 0 ? -d.x : d.x) <= radius && radius >= 0 && (d.z < 0 ? -d.z : d.z) <= radius &&
        FixVecDot(&d, &d) < radius2)
        hitB = Collision_ResolveSectorFaceContact((int *)&g_collisionLineStart, (int *)&g_collisionTarget, 1, type);
    if (hitA == 0 && hitB == 0)
        return 0;
    return 1;
}

// Collides a car with the edges of its sector: each edge near enough gets the
// response of its surface type (walls, water, sound triggers, finish, drop
// zones), then the scraping effects; ends a drop-out timer.
// FUNCTION: CMR2 0x0048e0a0
void Collision_TestCarAgainstSectorEdges(Car *pCar, int param)
{
    BYTE *p;
    int nearX;
    int nearZ;
    int a;
    int b;
    FixVector saved;
    int sector;

    g_collisionCar = pCar;
    g_collisionFace = Collision_GetCarBox(pCar->index);
    g_collisionCar->field_0xb42--;
    if (g_collisionCar->field_0xb42 < 0)
        g_collisionCar->field_0xb42 = 0;
    g_unk0x00591948 = 0;
    sector = g_collisionCar->sector;
    if (sector == -1)
        return;
    g_pCollisionFinRecord = g_sectors[sector]->pFinRecords;
    g_unk0x005919b8 = FixMul(g_collisionCar->collisionRadius, 0x13333);
    while (g_pCollisionFinRecord != NULL) {
        nearZ = 0;
        g_unk0x00591930 = 0;
        nearX = 0;
        g_collisionTarget = g_pCollisionFinRecord->target;
        g_collisionLineStart = g_pCollisionFinRecord->lineStart;
        g_unk0x005918d0 = g_collisionTarget.x - g_collisionCar->position.x;
        g_unk0x0059195c = g_collisionTarget.z - g_collisionCar->position.z;
        g_unk0x005918d4 = g_collisionLineStart.x - g_collisionCar->position.x;
        g_unk0x00591960 = g_collisionLineStart.z - g_collisionCar->position.z;
        if (g_unk0x005918d0 >= 0) {
            if (g_unk0x005918d4 >= 0)
                nearX = 1;
        } else if (g_unk0x005918d4 < 0) {
            nearX = 1;
        }
        if (g_unk0x0059195c >= 0) {
            if (g_unk0x00591960 >= 0)
                nearZ = 1;
        } else if (g_unk0x00591960 < 0) {
            nearZ = 1;
        }
        if (nearX && FIX_ABS(g_unk0x005918d0) >= g_unk0x005919b8 && FIX_ABS(g_unk0x005918d4) >= g_unk0x005919b8)
            goto next;
        if (nearZ && FIX_ABS(g_unk0x0059195c) >= g_unk0x005919b8 && FIX_ABS(g_unk0x00591960) >= g_unk0x005919b8)
            goto next;
        g_collisionDirection = g_pCollisionFinRecord->direction;
        g_collisionDirectionDirty = 0;
        switch (g_pCollisionFinRecord->type) {
        case 6:
            if (StageObject_IsCarOutsideCollisionHeightInterval() == 0 && VehiclePhysics_ClassifyCandidateFace(1) != 0)
                g_collisionCar->field_0xbf8 = 1;
            break;
        case 24:
            if (StageObject_IsCarOutsideCollisionHeightInterval() == 0 && VehiclePhysics_ClassifyCandidateFace(1) != 0 && g_collisionCar->field_0xa7c == 0 &&
                g_collisionCar->field_0xbf8 == 0)
                g_collisionCar->field_0xa7c = 0x190000;
            break;
        case 7:
            Sound_NoOpMusicCallback(0);
            break;
        case 8:
            Sound_NoOpMusicCallback(1);
            break;
        case 9:
            Sound_NoOpMusicCallback(2);
            break;
        case 10:
            Sound_NoOpMusicCallback(3);
            break;
        case 11:
            Sound_NoOpMusicCallback(4);
            break;
        case 23:
            if (g_collisionCar->field_0xc20 != 0)
                goto next;
            if (StageObject_IsCarOutsideCollisionHeightInterval() == 0 && VehiclePhysics_ClassifyCandidateFace(1) != 0)
                CarPhysics_UpdateWheelSlipAndVelocityDamping(g_collisionCar);
            break;
        case 0:
        case 1:
            goto next;
        default:
            if (StageObject_IsCarOutsideCollisionHeightInterval() == 0) {
                if ((g_pCollisionFinRecord->flags & 1) && Collision_TestSectorEdgeEndpoints(g_pCollisionFinRecord->type) &&
                    g_pCollisionFinRecord->type == 0x19)
                    g_collisionCar->field_0xbf8 = 1;
                if (Collision_ResolveSectorEdgeContact(g_pCollisionFinRecord->type, param) &&
                    g_pCollisionFinRecord->type == 0x19)
                    g_collisionCar->field_0xbf8 = 1;
            }
            break;
        }
        if (g_unk0x00591930 != 0) {
            p = (BYTE *)g_collisionCar + 0x5c4;
            saved = *(FixVector *)p;
            *(FixVector *)p = g_unk0x005918e0;
            FixVecScaleRecip(&g_collisionCar->field_0x5c4, &g_collisionCar->field_0x5c4,
                             0x10000 - g_unk0x0051fb00[g_pCollisionFinRecord->type]);
            if (g_collisionCar->field_0xb42 <= 0)
                CarDamage_ApplyCollisionDeformImpulse(g_collisionCar, (int *)&g_unk0x00591ad0, &g_unk0x00591938, 0, g_unk0x0059199c, 0);
            g_collisionCar->field_0x5c4 = saved;
            g_collisionCar->field_0xb42 = 10;
        }
    next:
        g_pCollisionFinRecord = g_pCollisionFinRecord->pNext;
    }
    if (g_collisionCar->field_0xa7c > 0) {
        g_collisionCar->field_0xa7c -= 0x10000;
        if (g_collisionCar->field_0xa7c < 0)
            g_collisionCar->field_0xa7c = 0;
        if (g_collisionCar->field_0xa7c == 0)
            g_collisionCar->field_0xbf8 = 1;
    }
}

int StageObject_UsesExtendedMode(void);
void Collision_TestOrderedCarPairs(Car *pCars, short *pOrder, short count);
int StageObject_GetCarSlotStateValue(int index);

// Collisions of the cars in `pOrder` for the frame: car against car (when the
// mode allows it), car against stage objects, car against the sector edges;
// then each car's contact box state is kept for the next frame.
// The car-indexed CollisionBox table keeps its built flag (0x28) at 0x2c
// and its first four points (0x30) in points[4..7] (0x60) for the next frame.
// FUNCTION: CMR2 0x004877a0
void Collision_UpdateOrderedCars(Car *pCars, short *pOrder, short count)
{
    int i;
    Car *pCar;
    CollisionBox *pBox;
    int car;

    for (i = count - 1; i >= 0; i--)
        g_carCollisionBoxes[pOrder[i]].built = 0;
    if (StageObject_UsesExtendedMode())
        Collision_TestOrderedCarPairs(pCars, pOrder, count);
    for (i = count - 1; i >= 0; i--) {
        car = pOrder[i];
        pCar = pCars + car;
        pBox = &g_carCollisionBoxes[car];
        if (pCar->field_0xb64 == 0)
            Collision_TestCarAgainstSectorObjects(pCar);
        if (pCar->field_0xc18 == 0 &&
            (pCar->field_0xb64 == 0 || StageObject_GetCarSlotStateValue(pCar->index) < 0x20000))
            Collision_TestCarAgainstSectorEdges(pCar, car);
        pBox->hasSecondaryVertices = pBox->built;
        if (pBox->built != 0)
            memcpy(&pBox->points[4], pBox->points, 4 * sizeof(pBox->points[0]));
    }
}

// Where the fireworks go off, relative to the stage origin.
// GLOBAL: CMR2 0x0051f4e0
FixVector g_fireworksOrigin = { -0xcc0000, 0, 0x3d0000 };
// GLOBAL: CMR2 0x0051f51c
char g_strFireworksLaunch3[] = "\\Fireworks\\Launch_3.wav";
// GLOBAL: CMR2 0x0051f534
char g_strFireworksLaunch2[] = "\\Fireworks\\Launch_2.wav";
// GLOBAL: CMR2 0x0051f54c
char g_strFireworksLaunch1[] = "\\Fireworks\\Launch_1.wav";
// GLOBAL: CMR2 0x0051f564
char g_strFireworksBang3[] = "\\Fireworks\\Bang_3.wav";
// GLOBAL: CMR2 0x0051f57c
char g_strFireworksBang2[] = "\\Fireworks\\Bang_2.wav";
// GLOBAL: CMR2 0x0051f594
char g_strFireworksBang1[] = "\\Fireworks\\Bang_1.wav";
// GLOBAL: CMR2 0x0051f5ac
char g_strFireworksLoudBang3[] = "\\Fireworks\\Loud_Bang_3.wav";
// GLOBAL: CMR2 0x0051f5c8
char g_strFireworksLoudBang2[] = "\\Fireworks\\Loud_Bang_2.wav";
// GLOBAL: CMR2 0x0051f5e4
char g_strFireworksLoudBang1[] = "\\Fireworks\\Loud_Bang_1.wav";
// GLOBAL: CMR2 0x0051f600
char g_strFireworksFalling2[] = "\\Fireworks\\Falling_2.wav";
// GLOBAL: CMR2 0x0051f61c
char g_strFireworksFalling1[] = "\\Fireworks\\Falling_1.wav";

extern char g_strPathConcat[];
extern double g_minusFixedDegreesToAngle12;
void StageObject_SetVehicleEffectState(int value);
int Sound_GetSampleCount(void);
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile);

#define LOAD_FIREWORK_SOUND(str)                                                  \
    sprintf(CFrontend::m_stringDest, g_strPathConcat, pDir, str);                 \
    Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile4())

// Sets up the fireworks of the end-of-rally show: `count` rockets (0x938-byte
// records), the two spark billboards, the 3x3 table of burst directions
// (every 30 degrees), the four launch points and the sounds.
// FUNCTION: CMR2 0x0047e4d0
void Fireworks_Init(BYTE count)
{
    int i;
    int k;
    int step;
    int a;
    int b;
    int start;
    int n;
    int *pSinB;
    FixVector *p;
    char *pDir;
    int sinA;
    short cosA;

    g_unk0x00590afc = count;
    if (count != 0) {
        g_unk0x00590af8 = CFileBuffer::AllocateLockedBuffer(count * 0x938);
        g_unk0x00590b04 = CFileBuffer::AllocateLockedBuffer(0x28);
        g_unk0x00590b08 = CFileBuffer::AllocateLockedBuffer(0x28);
        g_unk0x00590b0c = (void **)CFileBuffer::AllocateLockedBuffer(3 * sizeof(void *));
        for (i = 0; i < 3; i++)
            g_unk0x00590b0c[i] = CFileBuffer::AllocateLockedBuffer(0x24);
        for (i = 0; i < g_unk0x00590afc; i++) {
            ((FireworkRocket *)g_unk0x00590af8)[i].state = 0;
            ((FireworkRocket *)g_unk0x00590af8)[i].trailLen = 0;
        }
        ((BillboardDef *)g_unk0x00590b04)->top = -0x8000;
        ((BillboardDef *)g_unk0x00590b04)->left = 0x8000;
        ((BillboardDef *)g_unk0x00590b04)->bottom = 0x8000;
        ((BillboardDef *)g_unk0x00590b04)->right = -0x8000;
        ((BillboardDef *)g_unk0x00590b04)->field_0x20 = 0;
        ((BillboardDef *)g_unk0x00590b04)->flags &= 0xfe;
        ((BillboardDef *)g_unk0x00590b04)->flags &= 0xfd;
        memcpy(g_unk0x00590b08, g_unk0x00590b04, 0x28);
        ((BillboardDef *)g_unk0x00590b08)->top = -0x6000;
        ((BillboardDef *)g_unk0x00590b08)->left = 0x6000;
        ((BillboardDef *)g_unk0x00590b08)->bottom = 0x6000;
        ((BillboardDef *)g_unk0x00590b08)->right = -0x6000;
        step = FixDiv(0x5a0000, 0x30000);
        k = FixDiv(0x5a0000, 0x30000);
        a = FixMul(step, 0x8000);
        start = FixMul(k, 0x8000);
        b = start;
        for (i = 0; i < 3; i++) {
            sinA = (unsigned short)(int)FTOI(__int64, X87S_MUL(X87S_FILD(float, a), g_fixedDegreesToAngle12)) & 0xfff;
            cosA = (0x400 - (unsigned short)(int)FTOI(__int64, X87S_MUL(X87S_FILD(float, a), g_minusFixedDegreesToAngle12))) & 0xfff;
            for (n = 0; n < 3; n++) {
                ((FixVector *)g_unk0x00590b0c[i])[n].x =
                    FixMul(g_sinTable[sinA],
                           g_sinTable[(unsigned short)(int)FTOI(__int64, X87S_MUL(X87S_FILD(float, b), g_fixedDegreesToAngle12)) & 0xfff]);
                ((FixVector *)g_unk0x00590b0c[i])[n].y =
                    g_sinTable[(0x400 - (unsigned short)(int)FTOI(__int64, X87S_MUL(X87S_FILD(float, b), g_minusFixedDegreesToAngle12))) & 0xfff];
                ((FixVector *)g_unk0x00590b0c[i])[n].z =
                    FixMul(g_sinTable[cosA],
                           g_sinTable[(unsigned short)(int)FTOI(__int64, X87S_MUL(X87S_FILD(float, b), g_fixedDegreesToAngle12)) & 0xfff]);
                b += k;
            }
            a += step;
            b = start;
        }
    }
    g_unk0x00590b00 = g_carLightTexA[1];
    g_unk0x005909c8[0].x = 0x5b50000;
    g_unk0x005909c8[0].y = 0x20000;
    g_unk0x005909c8[1].x = 0x5b50000;
    g_unk0x005909c8[1].y = 0x20000;
    g_unk0x005909c8[2].x = 0x5b50000;
    g_unk0x005909c8[2].y = 0x20000;
    g_unk0x005909c8[3].x = 0x5b50000;
    g_unk0x005909c8[3].y = 0x20000;
    g_unk0x005909c8[0].z = 0xc20000;
    g_unk0x005909c8[1].z = 0xce0000;
    g_unk0x005909c8[2].z = 0xda0000;
    g_unk0x005909c8[3].z = 0xe60000;
    for (p = g_unk0x005909c8; p < g_unk0x005909c8 + 4; p++) {
        p->x += g_fireworksOrigin.x;
        p->y += g_fireworksOrigin.y;
        p->z += g_fireworksOrigin.z;
    }
    pDir = CInstallInfo::GetSoundsDir();
    StageObject_SetVehicleEffectState(Sound_GetSampleCount());
    LOAD_FIREWORK_SOUND(g_strFireworksFalling1);
    LOAD_FIREWORK_SOUND(g_strFireworksFalling2);
    LOAD_FIREWORK_SOUND(g_strFireworksLoudBang1);
    LOAD_FIREWORK_SOUND(g_strFireworksLoudBang2);
    LOAD_FIREWORK_SOUND(g_strFireworksLoudBang3);
    LOAD_FIREWORK_SOUND(g_strFireworksBang1);
    LOAD_FIREWORK_SOUND(g_strFireworksBang2);
    LOAD_FIREWORK_SOUND(g_strFireworksBang3);
    LOAD_FIREWORK_SOUND(g_strFireworksLaunch1);
    LOAD_FIREWORK_SOUND(g_strFireworksLaunch2);
    LOAD_FIREWORK_SOUND(g_strFireworksLaunch3);
    CGame::RegisterCallback(StageObject_ReleaseVehicleModelFiles, NULL);
}
#undef LOAD_FIREWORK_SOUND

short RallyData_GetActiveCarRaceRecordField14(Car *p);
int RallyData_GetRouteStateValue(void);

// Simple CPU driving used near the end of the route (the demo/attract drive):
// keeps the speed between two limits, steers back towards the route and
// brakes on a sharp heading error. Returns 0 when the route is about to end.
// `preview` != 0 only computes.
// FUNCTION: CMR2 0x0047d330
int AI_UpdateAttractDrivingControls(int car, int preview)
{
    Car *pCar;
    int state[0x2e];
    unsigned int node;
    int left;
    int right;
    int throttle;
    int brake;
    int handbrake;
    int mode;

    pCar = Car_Get(car);
    node = (unsigned short)RallyData_GetActiveCarRaceRecordField14(pCar);
    if (node + 5 > (unsigned int)RallyData_GetRouteStateValue())
        return 0;
    state[0x15] = node;
    StageTiming_RecomputeCarSplitBarSamples(pCar, 0x342, state, 0);
    handbrake = 0;
    left = 0;
    right = 0;
    throttle = 0;
    brake = 0;
    if (state[0] < 0x320000)
        throttle = 0x3f;
    if (state[0] > 0x460000)
        brake = 0x3f;
    if (state[7] > 0x50000)
        right = 0x3f;
    if (state[7] < -0x50000)
        left = 0x3f;
    mode = 0;
    if (state[0xd] < 0) {
        if (state[0xe] > 0x5a0000)
            mode = 4;
    } else if (state[0xe] > 0x5a0000) {
        mode = 4;
    }
    if (state[0xe] < -0x5a0000)
        mode = 2;
    else if (mode == 0)
        goto apply;
    switch (mode) {
    case 1:
        throttle = 0x3f;
        right = throttle;
        break;
    case 2:
        brake = 0x3f;
        right = brake;
        break;
    case 3:
        left = 0x3f;
        throttle = left;
        break;
    case 4:
        left = 0x3f;
        brake = left;
        break;
    case 5:
        left = 0x3f;
        throttle = 0;
        brake = left;
        break;
    case 6:
        throttle = 0;
        brake = 0x3f;
        right = brake;
        break;
    case 7:
        left = 0x3f;
        break;
    case 8:
        right = 0x3f;
        break;
    }
    if (state[0] >= 0x190000)
        throttle = 0;
apply:
    if (preview != 0)
        return 1;
    pCar->flag0x1d0[0] = 0;
    pCar->flag0x1d0[1] = 0;
    pCar->flag0x1d0[2] = 0;
    pCar->flag0x1d0[3] = 0;
    pCar->handbrake = 0;
    if (left > 0)
        pCar->flag0x1d0[0] = 0x3f;
    if (right > 0)
        pCar->flag0x1d0[1] = 0x3f;
    if (throttle > 0)
        pCar->flag0x1d0[2] = 0x3f;
    if (brake > 0)
        pCar->flag0x1d0[3] = 0x3f;
    if (handbrake > 0)
        pCar->handbrake = 1;
    return 1;
}

void Particle_Spawn(int typeIndex, FixVector *pSource, FixVector *pPosition, int field0x48, FixMatrix *attachmentMatrix,
                    BYTE *pColour, BYTE field0x54, void *callbackParam, BYTE field0x55);
#define RAND_FIX() ((int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536)))


// Per-frame update of the flying debris (100 records of 0x5c bytes): ages
// each piece, moves it along its velocity over the ground and trails a dust
// particle off it, fading the last seconds; also counts down the 8 debris
// timers.
// FUNCTION: CMR2 0x0047dd70
void CarDamage_UpdateFlyingDebris(void)
{
    CarHeadlightGlowRecord *pRec;
    int i;
    short tri;
    FixVector d;
    FixVector n;
    FixVector r;
    FixVector m;
    int dot;
    int fade;
    int ground;

    for (i = 0; i < 8; i++) {
        if (g_carHeadlightGlowCooldowns[i] > 0) {
            g_carHeadlightGlowCooldowns[i] -= g_physicsTimeStep;
            if (g_carHeadlightGlowCooldowns[i] < 0)
                g_carHeadlightGlowCooldowns[i] = 0;
        }
    }
    pRec = g_carHeadlightGlowRecords;
    for (i = 100; i != 0; i--, pRec++) {
        if (pRec->active == 0)
            continue;
        pRec->prevPosition = pRec->position;
        pRec->prevNormal = pRec->normal;
        pRec->prevFade = pRec->fade;
        pRec->life -= g_physicsTimeStep;
        if (pRec->life <= 0) {
            pRec->active = 0;
            continue;
        }
        d = pRec->position;
        pRec->position.x += pRec->velocity.x;
        pRec->position.y += pRec->velocity.y;
        pRec->position.z += pRec->velocity.z;
        ground = Track_GetGroundHeightSurface(&pRec->position, &pRec->normal, &pRec->triangle,
                                              &tri, (unsigned short *)&tri, pRec->height);
        pRec->height = ground;
        pRec->position.y = ground + 0x8000;
        d.x = pRec->position.x - d.x;
        d.y = pRec->position.y - d.y;
        d.z = pRec->position.z - d.z;
        FIX_NORMALIZE_INTO(n, d);
        r.x = 0x8000 - RAND_FIX();
        r.y = 0x8000 - RAND_FIX();
        r.z = 0x8000 - RAND_FIX();
        dot = FixVecDot(&n, &r);
        FixVecScale(&m, &n, dot);
        r.x -= m.x;
        r.y -= m.y;
        r.z -= m.z;
        FixVecScale(&n, &n, -0x8000);
        r.x += n.x;
        r.y += n.y;
        r.z += n.z;
        FixVecScale(&r, &r, 0x4ccc);
        r.x += d.x;
        r.y += d.y;
        r.z += d.z;
        m.x = pRec->position.x - r.x;
        m.y = pRec->position.y - r.y;
        m.z = pRec->position.z - r.z;
        Particle_Spawn(0x1f, &m, &r, pRec->position.y - 0x50000, 0, NULL, 0, 0,
                       Car_Get(pRec->car)->pBodyNode->viewMask);
        if (pRec->life > 0x50000) {
            pRec->fade = 0x10000;
        } else {
            fade = FixMul(pRec->life, 0x3333);
            if (fade > 0x10000)
                fade = 0x10000;
            else if (fade < 0)
                fade = 0;
            pRec->fade = fade;
        }
    }
}
#undef RAND_FIX

// Flash colour of each firework colour index.
// GLOBAL: CMR2 0x0051f504
DWORD g_fireworkFlashColours[6] = {
    0xff3c1955, 0xff551f10, 0xff555514, 0xff1a4155, 0xff0c2355, 0xff0d5535
};

void Sound_Free(unsigned int handle);

#define RAND_FIX() ((int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536)))
#define RAND_SIGNED()                                                                              \
    (RAND_FIX() > 0x8000 ? (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), g_minus65536)) : RAND_FIX())

// Per-frame update of the fireworks. A rocket (0x938 bytes) rises trailing a
// 20-point spark trail; when its fuse runs out it bursts, either into debris
// or into 18 sparks thrown along the 3x3 burst directions (mirrored up and
// down) that then fall under gravity until the burst timer ends.

// FUNCTION: CMR2 0x0047eab0
void Fireworks_Update(void)
{
    FireworkRocket *pRocket;
    int rocketIndex;
    int i;
    int j;
    int k;
    int debrisCount;
    int speed;
    int projection;
    FixVector *pBurstDirection;
    int previousX;
    int previousY;
    int previousZ;
    FixVector displacement;
    FixVector trailDirection;
    FixVector scaledVector;
    FixVector randomVector;

    for (rocketIndex = 0; rocketIndex < g_unk0x00590afc; rocketIndex++) {
        pRocket = (FireworkRocket *)g_unk0x00590af8 + rocketIndex;
        pRocket->prevPos = pRocket->pos;
        for (k = 0; k < 20; k++)
            pRocket->prevTrail[k] = pRocket->trail[k];
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 6; j++)
                pRocket->prevSparks[i][j] = pRocket->sparks[i][j];
        }
        switch (pRocket->state) {
        case 1:
            pRocket->vel.y -= pRocket->rise;
            previousX = pRocket->pos.x;
            previousY = pRocket->pos.y;
            previousZ = pRocket->pos.z;
            pRocket->pos.z += pRocket->vel.z;
            pRocket->pos.x += pRocket->vel.x;
            pRocket->pos.y += pRocket->vel.y;
            displacement.x = previousX - pRocket->pos.x;
            displacement.y = previousY - pRocket->pos.y;
            displacement.z = previousZ - pRocket->pos.z;
            FIX_NORMALIZE_INTO(trailDirection, displacement);
            for (k = 4; k != 0; k--) {
                FixVecScale(&scaledVector, &displacement, RAND_FIX());
                pRocket->trail[pRocket->trailHead].x = pRocket->pos.x + scaledVector.x;
                pRocket->trail[pRocket->trailHead].y = pRocket->pos.y + scaledVector.y;
                pRocket->trail[pRocket->trailHead].z = pRocket->pos.z + scaledVector.z;
                randomVector.x = RAND_SIGNED();
                randomVector.y = RAND_SIGNED();
                randomVector.z = RAND_SIGNED();
                FixVecScale(&randomVector, &randomVector, 0xccc);
                projection = FixVecDot(&randomVector, &trailDirection);
                FixVecScale(&scaledVector, &trailDirection, projection);
                randomVector.x -= scaledVector.x;
                randomVector.y -= scaledVector.y;
                randomVector.z -= scaledVector.z;
                pRocket->trail[pRocket->trailHead].x += randomVector.x;
                pRocket->trail[pRocket->trailHead].y += randomVector.y;
                pRocket->trail[pRocket->trailHead].z += randomVector.z;
                if (RAND_FIX() > 0x8000)
                    pRocket->trailFlag[pRocket->trailHead] = 1;
                else
                    pRocket->trailFlag[pRocket->trailHead] = 0;
                pRocket->trailHead++;
                if (pRocket->trailHead >= 20)
                    pRocket->trailHead -= 20;
            }
            pRocket->trailLen += 4;
            if (pRocket->trailLen >= 20)
                pRocket->trailLen = 19;
            pRocket->fuse -= 0x10000;
            if (pRocket->fuse <= 0) {
                if (pRocket->sound != -1 && Sound_IsPlaying(pRocket->sound))
                    Sound_Free(pRocket->sound);
                pRocket->state = 2;
                g_stageAmbientFlashLevel = 0x10000;
                *(DWORD *)g_stageAmbientFlashColour = g_fireworkFlashColours[pRocket->colour];
                if (pRocket->burst != 0) {
                    pRocket->prevPos = pRocket->pos;
                    for (k = 0; k < 20; k++)
                        pRocket->prevTrail[k] = pRocket->trail[k];
                    for (i = 0; i < 3; i++) {
                        for (j = 0; j < 3; j++) {
                            pRocket->sparks[i][j].z = 0;
                            pRocket->sparks[i][j].y = 0;
                            pRocket->sparks[i][j].x = 0;
                            pRocket->sparks[i][j + 3].z = 0;
                            pRocket->sparks[i][j + 3].y = 0;
                            pRocket->sparks[i][j + 3].x = 0;
                            pRocket->prevSparks[i][j].z = 0;
                            pRocket->prevSparks[i][j].y = 0;
                            pRocket->prevSparks[i][j].x = 0;
                            pRocket->prevSparks[i][j + 3].z = 0;
                            pRocket->prevSparks[i][j + 3].y = 0;
                            pRocket->prevSparks[i][j + 3].x = 0;
                            speed = pRocket->sparkSpeed + FixMul(RAND_FIX(), 0xa3d);
                            pBurstDirection = (FixVector *)g_unk0x00590b0c[i] + j;
                            FixVecScale(&pRocket->sparkVel[i][j], pBurstDirection, speed);
                            FixVecScale(&pRocket->sparkVel[i][j], &pRocket->sparkVel[i][j], 0x60000);
                            pRocket->sparkVel[i][j + 3] = pRocket->sparkVel[i][j];
                            pRocket->sparkVel[i][j + 3].y = -pRocket->sparkVel[i][j].y;
                        }
                    }
                    Sound_PlaySampleWithParameters((unsigned short)(rand() % 6 + 2 + g_unk0x005909bc), 0x10000, 0x5622, 0, 0, 0);
                } else if (pRocket->type == 2) {
                    for (debrisCount = rand() % 4 + 1; debrisCount > 0; debrisCount--) {
                        scaledVector.x = RAND_SIGNED();
                        scaledVector.y = RAND_SIGNED();
                        scaledVector.z = RAND_SIGNED();
                        FixVecScale(&scaledVector, &scaledVector, 0xe666);
                        StageObject_SpawnDebris(&pRocket->pos, &scaledVector, 1);
                    }
                }
            }
            break;
        case 2:
            if (pRocket->trailLen > 0) {
                pRocket->trailLen -= 4;
                pRocket->trailHead += 4;
                if (pRocket->trailHead >= 20)
                    pRocket->trailHead -= 20;
            }
            for (i = 0; i < 3; i++) {
                for (j = 0; j < 6; j++) {
                    pRocket->sparkVel[i][j].y -= pRocket->sparkGravity;
                    pRocket->sparks[i][j].x += pRocket->sparkVel[i][j].x;
                    pRocket->sparks[i][j].y += pRocket->sparkVel[i][j].y;
                    pRocket->sparks[i][j].z += pRocket->sparkVel[i][j].z;
                }
            }
            pRocket->burstTime -= 0x10000;
            pRocket->blink = pRocket->blink == 0;
            if (pRocket->burstTime <= 0)
                pRocket->state = 0;
            break;
        }
    }
}
#undef RAND_FIX
#undef RAND_SIGNED

void CarEffects_InitDebris(void);
void CarContact_InitStageRecords(void);
void StageWeather_InitParticleAndLightState(void);
void CarEffects_Init(void);
void StageTiming_InitEffectParticleTypes(void);
int RallyData_IsChampionshipFinalStage(void);

// Sets up the stage objects of a race: object tables, the championship-end
// fireworks, the headlight glows, the wheel trails and lights of every car,
// the debris, weather, effects and stage lights.
// FUNCTION: CMR2 0x00466360
void StageObjects_Init(void)
{
    short *pOrder;
    short count;
    int i;
    int car;

    StageObject_UpdateDeviceEffectSupportFlag();
    g_unk0x0058896c = (char)RallyData_IsChampionshipFinalStage() != 0;
    if (((char)RallyData_IsChampionshipFinalStage() || (char)RallyData_GetFlag24() || (char)RallyData_GetSelectionFlag27()) &&
        CGameInfo::IsActiveCheatEnabled(0))
        StageObject_CreateRecordGlows();
    pOrder = Car_GetOrder();
    count = Car_GetOrderCount();
    for (i = 0; i < count; i++) {
        car = pOrder[i];
        CarExhaust_SetPoint(0, 0, i);
        CarExhaust_SetPoint(0, 1, i);
        CarExhaust_SetGlow(0, 0, i);
        CarExhaust_SetGlow(0, 1, i);
        if (Car_Get(car)->isGhost == 0) {
            CarLight_BuildPointGlows(Car_Get(car));
            if (Car_Get(car)->type == 6 || Car_Get(car)->type == 7 ||
                Car_Get(car)->type == 10)
                CarLight_CacheBodyTextures(i, 0, 0, 0);
            else
                CarLight_CacheBodyTextures(i, 0, 0, 1);
        }
    }
    CarEffects_InitDebris();
    CarContact_InitStageRecords();
    StageWeather_InitParticleAndLightState();
    CarEffects_Init();
    StageLights_Create();
    StageTiming_InitEffectParticleTypes();
    if (g_unk0x0058896c != 0)
        Fireworks_Init(0x14);
}

// Per-frame update of the stage objects: stage lights, the attract-mode
// debris, and the fireworks once they are on.
// FUNCTION: CMR2 0x00466520
void StageObjects_Update(void)
{
    StageLights_Update();
    if (((char)RallyData_IsChampionshipFinalStage() || (char)RallyData_GetFlag24() || (char)RallyData_GetSelectionFlag27()) &&
        CGameInfo::IsActiveCheatEnabled(0))
        CarDamage_UpdateFlyingDebris();
    if (g_unk0x0058896c != 0) {
        Fireworks_Update();
        StageObject_FadeAmbientColourToBase();
    }
}

int StageTiming_GetValidStartTime(int index);
void Knockout_SetCurrentMatchTimes(unsigned int first, unsigned int second);
void Knockout_PropagateWinners(void);
BYTE Race_GetStateByte(void);

// Finishes the knockout match of the current round: stores the split times of
// the two drivers (or the fallback order when a driver is unknown), clears or
// marks the match entry, flags the loser, advances the round index of the
// championship state and re-propagates the bracket.
// match 96%: the only remaining differences are the stack slots of the four
// locals (the original keeps the two times in the two lower slots, this build
// puts them in the middle); every instruction and operand value is identical.
// Bit layout of KnockoutMatch::flags as the original manipulates it: two
// five-bit driver indices, the completion bit of the match and the two-bit
// winner flag (1 second driver faster, 2 first driver faster).
struct KnockoutMatchBits {
    unsigned int driver1 : 5;
    unsigned int driver2 : 5;
    unsigned int played : 1;
    unsigned int winner : 2;
    unsigned int pad : 19;
};

// Bit layout of KnockoutTable::state: the kind of bracket (1..4) at bits 3-5
// and the index of the round within that kind at bits 12-15.
struct KnockoutStateBits {
    unsigned int pad0 : 3;
    unsigned int kind : 3;
    unsigned int pad1 : 6;
    unsigned int round : 4;
    unsigned int pad2 : 16;
};

// FUNCTION: CMR2 0x00472a30
void Knockout_RecordCurrentMatchResults(void)
{
    struct { int times[2]; unsigned int drivers[2]; } results;
    KnockoutTable *pState;
    unsigned int state;
    int round;
    char t;

    pState = (KnockoutTable *)RallyData_GetChampionshipState();
    RallyData_GetRoundDrivers(&results.drivers[0], &results.drivers[1]);
    if (RallyData_GetUsableRecordCategory((BYTE)results.drivers[0]) != -1 && RallyData_GetUsableRecordCategory((BYTE)results.drivers[1]) != -1) {
        StageTiming_GetSplitTimesForPositions(results.drivers[0], results.drivers[1], &results.times[0], &results.times[1]);
    } else if (RallyData_GetUsableRecordCategory((BYTE)results.drivers[0]) == -1) {
        results.times[0] = StageTiming_GetValidStartTime(0);
        results.times[1] = StageTiming_GetValidStartTime(1);
    } else {
        results.times[1] = StageTiming_GetValidStartTime(0);
        results.times[0] = StageTiming_GetValidStartTime(1);
    }
    if (Knockout_HasUnknownMatchDriver(RallyData_GetRoundEntry()) != 0) {
        ((KnockoutMatch *)RallyData_GetRoundEntry())->time1 = 0;
        ((KnockoutMatch *)RallyData_GetRoundEntry())->time2 = 0;
        if ((((KnockoutMatch *)RallyData_GetRoundEntry())->flags & 0x1f) == 0x1f)
            ((KnockoutMatchBits *)RallyData_GetRoundEntry())->winner = 2;
        else
            ((KnockoutMatchBits *)RallyData_GetRoundEntry())->winner = 1;
    } else {
        Knockout_SetCurrentMatchTimes(results.times[0], results.times[1]);
    }
    if (Race_GetStateByte() != 0xff) {
        if ((BYTE)RallyDataState() == 2) {
            t = (char)Race_GetStateByte();
            ((KnockoutMatchBits *)RallyData_GetRoundEntry())->winner = -2 - t;
        } else if (RallyData_GetUsableRecordCategory(
                       (BYTE)((KnockoutMatch *)RallyData_GetRoundEntry())->flags & 0x1f) == -1) {
            ((KnockoutMatchBits *)RallyData_GetRoundEntry())->winner = 2;
        } else {
            ((KnockoutMatchBits *)RallyData_GetRoundEntry())->winner = 1;
        }
    }
    state = pState->state;
    round = (state >> 12) & 0xf;
    ((KnockoutStateBits *)pState)->round++;
    switch ((pState->state >> 3) & 7) {
    case 1:
        pState->round1[round].flags |= 0x400;
        if ((pState->state & 0xf000) > 0x7000)
            pState->state |= 0x400000;
        break;
    case 2:
        pState->quarters[round].flags |= 0x400;
        if ((pState->state & 0xf000) > 0x3000)
            pState->state |= 0x400000;
        break;
    case 3:
        pState->semis[round].flags |= 0x400;
        if ((pState->state & 0xf000) > 0x1000)
            pState->state |= 0x400000;
        break;
    case 4:
        pState->state |= 0x400000;
        pState->final.flags |= 0x400;
        break;
    }
    Knockout_PropagateWinners();
}

Texture *InRaceMenu_GetUpArrowTexture(void);
Texture *InRaceMenu_GetDownArrowTexture(void);
void StageObject_DrawTypingTextFraction(char *text, int fraction, unsigned int font, int x, unsigned int y, int *pColour,
                  unsigned int flags);
int StageObject_FillWidthScaledRectangle(int scale, BYTE *unused, short *pRect, BYTE *pColour, int layer);

// Draws the in-race pause menu: for every entry the background of the selected
// row, its label (the stage name with the round number in the knockout modes,
// the entry name otherwise) and the row separators, then the knockout bracket
// of the current round. The rows move down by KO_Y(6) on screens at least
// 1024 pixels wide whose texture limits accept that size.
// match 58%: the prologue, the constants, the call arguments and the control
// flow are identical; the residual diff is register/slot allocation inside the
// loop body (the original keeps the row height, the loop counter and the two
// rectangles in different registers than this build does). Kept as FUNCTION on
// purpose so reccmp measures it (see CONVENCIONES).
// FUNCTION: CMR2 0x00473d60
void StageObject_DrawInRacePauseMenu(Menu *pMenu)
{
    int i;
    short rect[4];
    short rect2[4];
    Texture *texture;
    int y0;
    int y;
    int halfHeight;
    BYTE *pColour;
    unsigned int *pState;

    rect[0] = (short)((int)(g_pGraphics->resX * 0x64) / 0x280);
    rect[1] = 0;
    texture = InRaceMenu_GetUpArrowTexture();
    rect[2] = texture->width;
    texture = InRaceMenu_GetUpArrowTexture();
    rect[3] = texture->height;
    y0 = (int)(g_pGraphics->resY * 0x17c) / 0x1e0;
    pState = RallyData_GetChampionshipState();
    for (i = 0; i < pMenu->itemCount; i++) {
        texture = InRaceMenu_GetUpArrowTexture();
        halfHeight = texture->height / 2;
        rect[1] = (short)((int)(g_pGraphics->resY * 0x24) / 0x1e0 * i +
                          (int)(g_pGraphics->resY * 0x14) / 0x1e0 - halfHeight + y0);
        y = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * i
            - (int)(g_pGraphics->resY * (InRaceMenu_GetUpArrowTexture()->height / 2)) / 0x1e0
            + (int)(g_pGraphics->resY * 0x14) / 0x1e0 + y0
            + (int)(g_pGraphics->resY * 0xe) / 0x1e0;
        if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
            CFrontend::IsTextureHeightSupported(0x400))
            y += (int)(g_pGraphics->resY * 6) / 0x1e0;
        if (pMenu->cursor == i) {
            pColour = g_barTextColour;
            texture = InRaceMenu_GetUpArrowTexture();
        } else {
            pColour = g_unk0x0051c994;
            texture = InRaceMenu_GetDownArrowTexture();
        }
        switch (g_knockoutRaceState) {
        case 1:
            if (i == 0)
                sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x77));
            else
                sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x78));
            break;
        case 2:
            if (i == 0)
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x79),
                        ((*pState >> 12) & 0xf) + 1);
            else
                sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x78));
            break;
        case 5:
            if (i == 0)
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x7a),
                        ((*pState >> 12) & 0xf) + 1);
            else
                sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x78));
            if ((*pState & 0x400000) == 0 && (*pState & 0x38) != 0x20)
                continue;
            break;
        default:
            continue;
        }
        if (i == 0 || pMenu->cursor == i) {
            if (Graphics_IsRegisteredTimerRunning(&g_pauseMenus.textTimer)) {
                rect2[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
                rect2[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0 + y0);
                rect2[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
                rect2[3] = 1;
                StageObject_FillWidthScaledRectangle(g_pauseMenus.textFade, &g_pGraphics->field309_0x150, rect2, pColour, 1);
            } else {
                rect2[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
                rect2[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0 + y0);
                rect2[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
                rect2[3] = 1;
                Sprite_FillRect(&g_pGraphics->field309_0x150, rect2, pColour, 1);
            }
        }
        rect2[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
        rect2[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0 + y0);
        rect2[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
        rect2[3] = 1;
        if (Graphics_IsRegisteredTimerRunning(&g_pauseMenus.textTimer)) {
            Sprite_Queue((SpriteRect *)&texture->field_0x11c, (SpriteRect *)rect, texture, 2,
                         0, NULL, NULL, pColour, 8);
            StageObject_DrawTypingTextFraction(CFrontend::m_stringDest, g_pauseMenus.textFade, 0,
                         (int)(g_pGraphics->resX * 0x7a) / 0x280,
                         y, (int *)pColour, 0x21);
        } else {
            Sprite_Queue((SpriteRect *)&texture->field_0x11c, (SpriteRect *)rect, texture, 2,
                         0, NULL, NULL, pColour, 8);
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                          y, (int *)pColour, 0x21);
        }
        rect2[0] = (short)((int)(g_pGraphics->resX * 0x63) / 0x280);
        rect2[1] = (short)((int)(g_pGraphics->resY * i * 0x24) / 0x1e0 +
                           (int)(g_pGraphics->resY * 0x24) / 0x1e0 + y0);
        rect2[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
        rect2[3] = 1;
        if ((Graphics_IsRegisteredTimerRunning(&g_pauseMenus.textTimer)))
            StageObject_FillWidthScaledRectangle(g_pauseMenus.textFade, &g_pGraphics->field309_0x150, rect2, pColour, 2);
        else
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect2, pColour, 2);
    }
    Font_SetBlendMode(2);
    Knockout_DrawCurrentRoundBracket();
}
// match 88%: logica, llamadas y constantes identicas al original; el residuo es
// reparto de registros y de slots de pila temporales (el original cachea el puntero
// del vector en esi para los tres stores del caso len==0 y materializa los dos
// operandos del FixMul del rotor) y el orden de los stores de angles[].


void CarPart_StoreRotatedBasis(unsigned short *pAngles);
void VehicleMotion_UpdateWorldPosition(void);
extern int g_carPartVibrationAngle;
extern const double g_fixedRadiansToAngle12;

// FUNCTION: CMR2 0x00484310
void CarPart_IntegrateDetachedMotion(void)
{
    unsigned short angles[3];
    FixVector scratch;
    FixVector motion;
    FixVector cross;
    FixVector local;
    int modified;
    int type;
    int t;

    if (g_partCar->type != 9 &&
        g_partCar->type != 0xb) {
        motion.x = 0;
        motion.y = 0;
        motion.z = 0;
        scratch.x = g_partCar->velocity.x -
                    g_partCar->velocityNext.x;
        scratch.y = g_partCar->velocity.y -
                    g_partCar->velocityNext.y;
        scratch.z = g_partCar->velocity.z -
                    g_partCar->velocityNext.z;
        FixVecLength(&scratch);
        FixVecScale(&scratch, &scratch, -0x60000);
        if (g_partCar->type != 8) {
            type = g_carPartModelIndices[2][g_partCar->index];
            if (*(BYTE *)&g_partSet->geometry.nodes[type]->flags == 0xc)
                scratch.x = -scratch.x;
        }
        motion.x += scratch.x;
        motion.y += scratch.y;
        motion.z += scratch.z;
        motion.y -= 0x53f7;
        FixMatrix_InverseRotateVector(&local, &motion,
                                      g_partState->pForceFrame);
        scratch.x = g_partState->previousPosition.x -
                    g_partState->position.x;
        scratch.y = g_partState->previousPosition.y -
                    g_partState->position.y;
        scratch.z = g_partState->previousPosition.z -
                    g_partState->position.z;
        if (g_partCar->type == 8)
            scratch.x = 0;
        scratch.y = 0x3333;
        FixVecCross(&cross, &local, &scratch);
        if ((g_partState->flags & 8) == 0 && cross.z > 0)
            g_partState->flags |= 8;
        if ((g_partState->flags & 8) != 0) {
            FixVecScale(&g_partState->angularVelocity,
                        &g_partState->angularVelocity, 0xe49b);
            t = -FixMul(cross.z, g_partState->stiffness.z);
            t = FixMul(t, g_physicsTimeStep);
            g_partState->angularVelocity.z += t;
            angles[2] = FTOI(short, X87S_MUL((double)(FixMul(g_partState->angularVelocity.z,
                                                g_physicsTimeStep) -
                                          FixMul(t, g_physicsTimeStep / 2)), g_fixedRadiansToAngle12));
            angles[0] = 0;
            angles[1] = 0;
            FixBasis_Rotate(&g_partState->basis, angles);
            modified = 0;
            if (g_partCar->type == 8) {
                if (g_partState->basis.right.y < 0) {
                    g_partState->basis.right.y = 0;
                    modified = 1;
                    if (g_partState->angularVelocity.z < 0) {
                        if (g_partState->angularVelocity.z < -0xf5c)
                            g_partState->angularVelocity.z =
                                -FixMul(0x9999, g_partState->angularVelocity.z);
                        else {
                            g_partState->flags &= 0xf7;
                            g_partState->angularVelocity.z = 0;
                        }
                    }
                }
                if (g_partState->basis.right.x < 0x6666) {
                    g_partState->basis.right.x = 0x6666;
                    if (g_partState->angularVelocity.z > 0)
                        g_partState->angularVelocity.z = 0;
                    goto renormalise;
                }
            } else {
                if (g_partState->basis.right.y > 0) {
                    g_partState->basis.right.y = 0;
                    modified = 1;
                    if (g_partState->angularVelocity.z > 0) {
                        if (g_partState->angularVelocity.z > 0xf5c)
                            g_partState->angularVelocity.z =
                                -FixMul(0x9999, g_partState->angularVelocity.z);
                        else {
                            g_partState->angularVelocity.z = 0;
                            g_partState->flags &= 0xf7;
                        }
                    }
                }
                if (g_partState->basis.right.x < 0xcccc) {
                    g_partState->basis.right.x = 0xcccc;
                    if (g_partState->angularVelocity.z < 0)
                        g_partState->angularVelocity.z = 0;
                    goto renormalise;
                }
            }
            if (modified == 0) {
                goto anglesZ;
            }
        renormalise:
            FIX_NORMALIZE_INTO((g_partState->basis.right),
                               (g_partState->basis.right));
            FixVecScale(&scratch, &g_partState->basis.right,
                        FixVecDot(&g_partState->basis.right,
                                  &g_partState->basis.forward));
            g_partState->basis.forward.x -= scratch.x;
            g_partState->basis.forward.y -= scratch.y;
            g_partState->basis.forward.z -= scratch.z;
            FIX_NORMALIZE_INTO((g_partState->basis.forward),
                               (g_partState->basis.forward));
            FixVecCross(&scratch, &g_partState->basis.forward,
                        &g_partState->basis.right);
            StageObj_NormalizeInto(&g_partState->basis.up, &scratch);
        }
    anglesZ:
        angles[0] = 0;
        angles[1] = 0;
        angles[2] = FTOI(short, X87S_MUL((double)FixMul((short)g_carPartVibrationAngle * 0x1680, 0x10000), g_minusFixedDegreesToAngle12));
    } else {
        if (g_partCar->type == 9) {
            angles[0] = 0;
            angles[2] = 0;
            angles[1] = FTOI(short, X87S_MUL((double)FixMul((short)g_carPartVibrationAngle * 0x1680, 0x10000), g_minusFixedDegreesToAngle12));
        } else {
            angles[1] = 0;
            angles[2] = 0;
            angles[0] = FTOI(short, X87S_MUL((double)FixMul((short)g_carPartVibrationAngle * 0x1680, 0x30000), g_minusFixedDegreesToAngle12));
        }
    }
    CarPart_StoreRotatedBasis(angles);
    VehicleMotion_UpdateWorldPosition();
}
// Draws the fireworks every frame: the rising rocket as a single billboard, or
// for a burst the 18 spark clusters (each up to four mirrored billboards) plus
// the fading 20-point trail. See Fireworks_Update for the record layout.
// FUNCTION: CMR2 0x0047f740
void Fireworks_Draw(void)
{
#define SPARK_DEF ((BillboardDef *)g_unk0x00590b08)
#define SPARK_COLOUR(c) (*(int *)&SPARK_DEF->r = *(int *)(c))
    FireworkRocket *p;
    int i;
    int k;
    int j;
    int n;
    int colourIndex;
    int fade;
    int sparkX;
    int sparkY;
    int sparkZ;
    int *pFlag;
    FixVector *pTrail;
    int baseR;
    int baseG;
    int baseB;
    int litR;
    int litG;
    int litB;
    BYTE colourA[4];
    BYTE colourB[4];
    BYTE colourC[4];

    for (i = 0; i < g_unk0x00590afc; i++) {
        p = &((FireworkRocket *)g_unk0x00590af8)[i];
        switch (p->state) {
        case 1:
            ((BillboardDef *)g_unk0x00590b04)->pos = p->drawPos;
            *(int *)&((BillboardDef *)g_unk0x00590b04)->r = *(int *)p->rocketColour;
            Billboard_Add((BillboardDef *)g_unk0x00590b04, g_unk0x00590b00);
            colourIndex = 0;
            break;
        case 2:
            Firework_InterpolateRocketColours(p, colourA, p->colourA[0], p->colourA[1]);
            Firework_InterpolateRocketColours(p, colourB, p->colourB[0], p->colourB[1]);
            if (p->burst != 0) {
                for (k = 0; k < 3; k++) {
                    for (j = 0; j < 6; j++) {
                        sparkX = p->drawSparks[k][j].x;
                        sparkY = p->drawSparks[k][j].y;
                        sparkZ = p->drawSparks[k][j].z;
                        if (p->sparkOn[k][j][0] != 0) {
                            SPARK_DEF->pos.x = p->drawPos.x + sparkX;
                            SPARK_DEF->pos.y = p->drawPos.y + sparkY;
                            SPARK_DEF->pos.z = p->drawPos.z + sparkZ;
                            if (p->sparkAlt[k][j][0] != 0) {
                                if (p->blink != 0)
                                    SPARK_COLOUR(colourB);
                                else
                                    SPARK_COLOUR(colourA);
                            } else if (p->blink != 0) {
                                SPARK_COLOUR(colourA);
                            } else {
                                SPARK_COLOUR(colourB);
                            }
                            Billboard_Add(SPARK_DEF, g_unk0x00590b00);
                        }
                        if (p->sparkOn[k][j][1] != 0) {
                            sparkX = -sparkX;
                            SPARK_DEF->pos.x = p->drawPos.x + sparkX;
                            SPARK_DEF->pos.y = p->drawPos.y + sparkY;
                            SPARK_DEF->pos.z = p->drawPos.z + sparkZ;
                            if (p->sparkAlt[k][j][1] != 0) {
                                if (p->blink != 0)
                                    SPARK_COLOUR(colourB);
                                else
                                    SPARK_COLOUR(colourA);
                            } else if (p->blink != 0) {
                                SPARK_COLOUR(colourA);
                            } else {
                                SPARK_COLOUR(colourB);
                            }
                            Billboard_Add(SPARK_DEF, g_unk0x00590b00);
                        }
                        if (p->sparkOn[k][j][2] != 0) {
                            SPARK_DEF->pos.x = p->drawPos.x + sparkX;
                            SPARK_DEF->pos.y = p->drawPos.y + sparkY;
                            sparkZ = -sparkZ;
                            SPARK_DEF->pos.z = p->drawPos.z + sparkZ;
                            if (p->sparkAlt[k][j][2] != 0) {
                                if (p->blink != 0)
                                    SPARK_COLOUR(colourB);
                                else
                                    SPARK_COLOUR(colourA);
                            } else if (p->blink != 0) {
                                SPARK_COLOUR(colourA);
                            } else {
                                SPARK_COLOUR(colourB);
                            }
                            Billboard_Add(SPARK_DEF, g_unk0x00590b00);
                        }
                        if (p->sparkOn[k][j][3] != 0) {
                            sparkX = -sparkX;
                            SPARK_DEF->pos.x = p->drawPos.x + sparkX;
                            SPARK_DEF->pos.y = p->drawPos.y + sparkY;
                            SPARK_DEF->pos.z = p->drawPos.z + sparkZ;
                            if (p->sparkAlt[k][j][3] != 0) {
                                if (p->blink != 0)
                                    SPARK_COLOUR(colourB);
                                else
                                    SPARK_COLOUR(colourA);
                            } else if (p->blink == 0) {
                                SPARK_COLOUR(colourB);
                            } else {
                                SPARK_COLOUR(colourA);
                            }
                            Billboard_Add(SPARK_DEF, g_unk0x00590b00);
                        }
                    }
                }
            }
            colourIndex = 0x14 - p->trailLen;
            break;
        }
        if (p->state != 0) {
            colourIndex += p->trailHead;
            fade = 0x3333;
            if (colourIndex >= 0x14)
                colourIndex -= 0x14;
            baseR = p->trailColour[0] << 16;
            baseG = p->trailColour[1] << 16;
            baseB = p->trailColour[2] << 16;
            litR = baseR + 0x640000;
            litG = baseG + 0x640000;
            litB = baseB + 0x640000;
            for (j = 0; j < p->trailLen; j += 4) {
                colourC[3] = 0xff;
                colourA[0] = (BYTE)FixMulShift32(baseR, fade);
                colourA[1] = (BYTE)FixMulShift32(baseG, fade);
                colourA[2] = (BYTE)FixMulShift32(baseB, fade);
                colourA[3] = 0xff;
                colourC[0] = (BYTE)FixMulShift32(litR, fade);
                colourC[1] = (BYTE)FixMulShift32(litG, fade);
                colourC[2] = (BYTE)FixMulShift32(litB, fade);
                fade += 0x3333;
                pTrail = &p->drawTrail[colourIndex];
                n = 4;
                pFlag = &p->trailFlag[colourIndex];
                do {
                    if (*pFlag != 0)
                        SPARK_COLOUR(colourC);
                    else
                        SPARK_COLOUR(colourA);
                    SPARK_DEF->pos = *pTrail;
                    colourIndex++;
                    pFlag++;
                    pTrail++;
                    if (colourIndex >= 0x14) {
                        colourIndex -= 0x14;
                        pFlag -= 0x14;
                        pTrail -= 0x14;
                    }
                    Billboard_Add(SPARK_DEF, g_unk0x00590b00);
                } while (--n != 0);
            }
        }
    }
#undef SPARK_DEF
#undef SPARK_COLOUR
}
