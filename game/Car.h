#ifndef _CAR_H
#define _CAR_H
#include "FloatToInt.h"

#include "FixedPoint.h"
#include "SceneNode.h"
#include "Graphics.h"

// Car instance (0xc24 bytes, one per slot in g_carBuffer). Only the fields
// used by the decompiled code are named; offsets are in the comments.
// Surface grip parameters of one box corner (0x24 bytes), blended from the
// surface under it and the next one (Car_UpdateSurfaceParams).
struct CarCornerGrip {
    int gripA;          // 0x00
    int gripB;          // 0x04
    int grip2A;         // 0x08
    int grip2B;         // 0x0c
    int drag;           // 0x10
    int field_0x14;     // 0x14
    int field_0x18;     // 0x18
    int field_0x1c;     // 0x1c
    int field_0x20;     // 0x20
};

// Surface effect of one wheel (12 bytes).
struct CarWheelSurface {
    BYTE effect[2];     // 0x00 spray / dust effect of the surface
    BYTE pad_0x2[2];
    int drag;           // 0x04
    int extraGrip;      // 0x08
};

// The three sectors around the one a car is in, copied as one block.
struct SectorNeighbours {
    short s[3];
};

struct Car {
    // Fixed-word prefix: original cursors walk across grip/effect records.
    union {
        struct {
            FixMatrix physicsMatrix;          // 0x00 pWorld points here
            FixMatrix bodyMatrix;             // 0x40 pBodyMatrix points here
            CarCornerGrip cornerGrip[8];      // 0x80
            CarWheelSurface wheelSurfaceFx[4]; // 0x1a0
        };
        int surfaceOutputWords[116];
        BYTE surfaceOutputBytes[0x1d0];
    };
    char flag0x1d0[4];                // 0x1d0
    BYTE field_0x1d4[0x4];
    int handbrake;                    // 0x1d8  handbrake engaged (the rear wheels stop being driven)
    int field_0x1dc;                  // 0x1dc
    int field_0x1e0;                  // 0x1e0
    int field_0x1e4;                  // 0x1e4
    int field_0x1e8[4];               // 0x1e8
    FixVector bodySize;                // 0x1f8  full body dimensions; halfExtents is half this vector
    FixVector halfExtents;            // 0x204
    union {
        struct {
            FixVector wheelPos[4];            // 0x210  wheel positions in body space
            FixVector upperCornersLocal[4];    // 0x240  four upper body-space collision corners
        };
        FixVector collisionCornersLocal[8]; // 0x210 four lower + four upper corners
    };
    FixVector corners[8];             // 0x270  world-space box corners
    FixVector position;               // 0x2d0
    FixVector field_0x2dc;            // 0x2dc
    FixVector positionPrev;           // 0x2e8  position of the previous step
    FixVector positionPrev2;          // 0x2f4  and of the one before
    FixVector cornerPrev[4];          // 0x300  previous positions of the four lower corners
    FixVector cornerPrev2[4];         // 0x330  and the ones before
    // The original copies all nine words together at start/reset and restores
    // them together on a player fade. These overlays are runtime state.
    union {
        struct {
            FixVector right;         // 0x360 body axes (rows of the body matrix)
            FixVector up;            // 0x36c
            FixVector forward;       // 0x378
        };
        int bodyAxes[9];
        FixVector bodyAxisVectors[3]; // same right/up/forward rows, without a byte cursor
    };
    union {
        struct {
            FixVector targetRight;   // 0x384 saved right, restored with up/forward
            FixVector targetUp;      // 0x390 up/forward the body relaxes towards
            FixVector targetForward; // 0x39c
        };
        int targetAxes[9];
    };
    FixVector frontWheelAxis;         // 0x3a8  lateral axis of the front (steered) wheels
    FixVector frontWheelDir;          // 0x3b4  rolling direction of the front wheels 0/1: rearWheelDir turned by the steering
    FixVector wheelEmitter[4];        // 0x3c0  wheel dust/smoke emitter in body space
    FixVector rearWheelDir;           // 0x3f0  rolling direction of the rear wheels 2/3, eases towards the body axis
    FixVector inertia;                // 0x3fc  used to turn the summed torque into angular acceleration
    FixVector velocity;               // 0x408
    FixVector velocityNext;           // 0x414  velocityNext - velocity is the acceleration of the last step
    FixVector angularVelocity;        // 0x420  body space
    FixVector cornerVelocity[8];      // 0x42c  world-space velocity of each box corner
    FixVector groundNormal;           // 0x48c  normal of the box face the car rests on (Car_UpdateGroundNormal)
    FixVector normal0x498;            // 0x498
    FixVector cornerAxis[8];          // 0x4a4  per-corner reference axis
    FixVector cornerNormal[8];        // 0x504  per-corner contact normal
    FixVector field_0x564[8];         // 0x564  per-corner ground normal ([0] used while cornerOnGround[4] is set)
    FixVector field_0x5c4;            // 0x5c4
    FixVector field_0x5d0;            // 0x5d0
    FixVector field_0x5dc;            // 0x5dc
    FixVector cornerLoad[8];          // 0x5e8  normal force of the ground at each corner
    FixVector cornerForce[8];         // 0x648  force accumulated at each corner
    FixVector baseForce;              // 0x6a8  constant force applied every step
    FixVector groundDir[2];           // 0x6b4  front/rear rolling direction on the ground plane
    FixVector groundAxis[2];          // 0x6cc  front/rear lateral axis on the ground plane
    FixVector wheelLean;              // 0x6e4  lean of the wheel frame, integrated like lean
    FixVector lean;                   // 0x6f0  body lean (x/z tilt) fed into the body matrix
    int wheelOffset[4][2];            // 0x6fc  per-wheel wobble: sin / -cos of wheelPhase, scaled by the setup
    SceneNode *pSceneRoot;            // 0x71c root scene with wheel nodes 1..4
    SceneNode *pBodyNode;             // 0x720 body mesh node id 5
    SceneNode *pAlternateBodyNode;    // 0x724 optional alternate body node id 5
    SceneNode *pExtraNodes[4];        // 0x728  optional extra parts (0x728 set means all four)
    SceneNode *pWheelNodes[4];        // 0x738
    SceneNode *pViewNodeNear;         // 0x748  child node placed towards the view
    SceneNode *pViewNodeFar;          // 0x74c  child node placed away from the view
    FixMatrix *pWorld;                // 0x750
    FixMatrix *pBodyMatrix;           // 0x754
    int collisionRadius;               // 0x758
    int mass;                          // 0x75c
    int inverseMass;                   // 0x760
    int scale0x764;                   // 0x764
    int scale0x768;                   // 0x768
    int scale0x76c;                   // 0x76c
    int field_0x770[2];               // 0x770
    int speed;                        // 0x778  length of the velocity vector
    int tipRatio;                     // 0x77c  sideways slide relative to the tip-over threshold, eased
    int engineNetTorque;               // 0x780  throttle torque minus engine drag
    int engineDragCoefficient;         // 0x784  drag factor of the engine speed
    int maxThrottleTorque;             // 0x788
    int baseThrottleTorque;            // 0x78c
    int throttleRampStep;              // 0x790
    int engineSpeedLimit;              // 0x794
    int field_0x798;
    int throttleTorque;                // 0x79c  throttle torque ramp; also influences rear wheel alignment
    int throttlePhase;                 // 0x7a0
    int engineSpeed;                   // 0x7a4
    int field_0x7a8;
    int field_0x7ac;                  // 0x7ac
    int revLimiterTorque;              // 0x7b0  excess revs after limiting
    int driveSplit;                   // 0x7b4  drive split between the axles (Car_SetDriveSplit)
    int baseDriveSplit;                // 0x7b8
    int gearRatio[8];                  // 0x7bc
    int gearSpeed[8];                 // 0x7dc  per-gear speed table (Car_SelectGearSpeedTable)
    int steeringOutwardRate;          // 0x7fc  angle step steering away from centre (CarSetup::steering)
    int steeringCentringRate;         // 0x800  angle step steering back towards centre
    int steeringForceScale;           // 0x804  scales the steering accumulator inputs
    int wheelTravelLoadBias[4];       // 0x808 mean-relative wheel travel, scaled/clamped for corner loads
    int steeringInput;                 // 0x818  ramps -1..1 with flag0x1d0[0]/[1] (0x494540)
    int steeringAccumulator;           // 0x81c
    int steeringReturnRate;            // 0x820  rate applied when steering returns towards zero
    int steeringTorqueScale;           // 0x824  steering torque
    int steeringSpeedScale;            // 0x828
    int maxBrakeForce;                 // 0x82c
    int brakeBias;                    // 0x830  front share of the brake force (the rear gets 1 - bias)
    int brakeRampStep;                 // 0x834
    int brakeInput;                   // 0x838  brake pedal, scaled by the setup brake strengths (+0x3fc / +0x400)
    int brakePhase;                    // 0x83c  swing phase of 0x838
    int handbrakeRampStep;             // 0x840
    int maxHandbrakeForce;             // 0x844  target of the 0x848 swing
    int handbrakeForce;               // 0x848  added to the rear brake while the handbrake is on; swings to maxHandbrakeForce
    int handbrakePhase;                // 0x84c  swing phase (0..1)
    int wheelTorque[4];               // 0x850  drive/brake torque per wheel
    int wheelLoad[4];                 // 0x860  paired per axle; Car_BalanceWheelPairs evens each pair out
    int wheelSlip[4];                 // 0x870  rolling slip of each wheel
    union {
        struct {
            int wheelSlipLateral[4];          // 0x880  lateral slip of each wheel
            int wheelSpinForWheelLean[4];      // 0x890  filtered wheel spin (front lean)
            int wheelSpinForBodyLean[4];       // 0x8a0  filtered wheel spin (body lean)
            int cornerMass;                   // 0x8b0  mass carried by each touching corner
            int tyreGrip;                     // 0x8b4  tyre grip, times the physics scale each step
            int cornerWeight[8];               // 0x8b8  per-corner load shares, rebuilt from suspension/lean
        };
        int surfaceCompressionWords[22]; // 0x880..0x8d7, original contiguous walk
    };
    int suspensionLoadScale;          // 0x8d8 mass / 8, converts height differences to load shares
    int cornerHeight[8];              // 0x8dc  ground height under each box corner
    int field_0x8fc[8];               // 0x8fc  per-corner height offset while cornerOnGround[4 + i]
    int groundRightDot;                // 0x91c  ground normal projected on the body right axis
    int groundUpDot;                   // 0x920  ground normal projected on the body up axis
    int groundForwardDot;              // 0x924  ground normal projected on the body forward axis
    int field_0x928[4];               // 0x928
    int field_0x938[4];               // 0x938  per-wheel scale of the part offset
    int field_0x948[4];               // 0x948
    int groundHeightCorrection;        // 0x958  signed correction from the deepest ground contact
    int field_0x95c;                  // 0x95c
    int field_0x960;
    int field_0x964;
    int field_0x968;
    int field_0x96c;                  // 0x96c
    int field_0x970[2];               // 0x970
    int field_0x978[4];               // 0x978  per-wheel offset added to the part's value
    int wheelRideHeight[4];          // 0x988 setting-derived body lift / spring-impact limit
    int wheelLeanHeight[4];          // 0x998 lean-frame corner Y plus suspension damage offset
    int bodyLeanHeight[4];           // 0x9a8 body-lean corner Y plus suspension damage offset
    int wheelLeanDamping;              // 0x9b8  damping of wheelLean
    int bodyLeanDamping;               // 0x9bc
    int wheelTravelLoadScale;        // 0x9c0 converts mean-relative wheel travel to load bias
    int field_0x9c4;                  // 0x9c4
    int wheelSpringVelocity[4];      // 0x9c8 integrated restoring/damping force
    int wheelSpringOffset[4];        // 0x9d8 integrated velocity; clamped below at -0x3333
    int wheelSpringStiffness[4];     // 0x9e8 coefficient multiplying spring offset
    int wheelSpringDamping[4];       // 0x9f8 coefficient multiplying spring velocity
    int field_0xa08;
    int cornerGripA[8];               // 0xa0c  grip limits of a corner without a wheel
    int cornerGripB[8];               // 0xa2c
    int field_0xa4c[4];               // 0xa4c
    int field_0xa5c[4];               // 0xa5c
    BYTE field_0xa6c[0x4];
    int field_0xa70;                  // 0xa70
    int surfaceNoise;               // 0xa74 smoothed wheel-surface noise
    int surfaceNoiseTarget;                  // 0xa78 surface-noise target, copied to +0xa74
    int field_0xa7c;                  // 0xa7c
    int field_0xa80;
    int field_0xa84;
    int cheatWheelDrop;                // 0xa88
    int cheatBodyLift;                 // 0xa8c  fixed body/view lift for cheat 6
    int simulationInterpolation;     // 0xa90 fractional step in 16.16, used by transforms/views
    BYTE field_0xa94[4];              // 0xa94 purpose not established
    float simulationRateHz;           // 0xa98 default 25 steps per second
    unsigned short steepTime;         // 0xa9c  steps spent on a slope too steep to stand on
    short cornerTriangle[8];          // 0xa9e  cached collision triangle under each corner
    short wheelSurface[8];            // 0xaae  surface class under each corner
    short previousWheelSurface[4];     // 0xabe  surface of each wheel at the last step
    short wheelSurfaceType[8];        // 0xac6  surface id under each corner
    union {
        BYTE field_0xad6[0x28];
        struct {
            short contactSurfaces[2][5]; // 0xad6 previous IDs, 0xae0 pending IDs
            BYTE field_0xaea[0x14];
        };
    };
    short engineStartTimer;            // 0xafe  engine startup countdown
    short sector;                     // 0xb00  stage sector the car is in
    SectorNeighbours neighbours;      // 0xb02  the sectors around it (Sector_GetNeighbours)
    unsigned short wheelPhase[4];     // 0xb08  wobble angle of each wheel (12-bit)
    unsigned short wheelSteeringAngle; // 0xb10  12-bit front wheel steering angle
    short targetSteeringAngle;         // 0xb12  target from the steering accumulator
    short renderSteeringAngle;         // 0xb14  smoothed steering angle used by wheel scene nodes
    short maxSteeringAngleDegrees;     // 0xb16  converted to the 12-bit wheel steering scale
    short tipAngle;                   // 0xb18  12-bit angle the body tips by
    char index;                       // 0xb1a  index of this car (timing records and every per-car table)
    char type;                        // 0xb1b  car model / part layout (8, 9, 0xb: special bodies)
    BYTE flags;                       // 0xb1c
    char field_0xb1d;                 // 0xb1d
    char gear;                        // 0xb1e  current gear (0 neutral, 7 reverse)
    char field_0xb1f;                 // 0xb1f
    char requestedGear;                // 0xb20  requested gear
    char autoShiftDelay;               // 0xb21  shift delay
    char lastShiftDirection;           // 0xb22  1 down, 2 up
    BYTE field_0xb23[0x1];
    char damageShiftDelay;             // 0xb24  damage delay snapshot, counted down in neutral
    BYTE field_0xb25;                 // 0xb25  countdowns started by 0x43b020
    BYTE field_0xb26;
    BYTE field_0xb27;
    BYTE field_0xb28;                 // 0xb28
    BYTE surfaceDragLevel;             // 0xb29 group of nine surface drag values
    BYTE field_0xb2a;                 // 0xb2a
    char deepestCorner;                // 0xb2b  0..3, or -1 when no corner penetrates
    char cornerFlags[8];              // 0xb2c  set while a corner is disabled
    char field_0xb34;                 // 0xb34
    union {
        BYTE field_0xb35[0xd];
        struct {
            BYTE field_0xb35Prefix[9];
            char previousContactCount; // 0xb3e signed count of previous surface IDs
            char pendingContactCount;  // 0xb3f signed count of pending surface IDs
            BYTE field_0xb40[2];
        };
    };
    char field_0xb42;                 // 0xb42
    BYTE simulationStepsRemaining;    // 0xb43 scheduled steps not yet consumed this frame
    char field_0xb44;                 // 0xb44
    BYTE field_0xb45;
    BYTE shakeLevel;                  // 0xb46  camera shake requested this frame
    BYTE field_0xb47;
    int field_0xb48;                  // 0xb48
    int engineRestartPending;          // 0xb4c  engine restart flag
    int field_0xb50;
    int braking;                       // 0xb54  brake pedal above its threshold
    int field_0xb58;
    int reversing;                     // 0xb5c  current gear ratio is negative
    int field_0xb60;                  // 0xb60
    int field_0xb64;                  // 0xb64
    int firstCameraBlocked;            // 0xb68  rejection flag for the first camera
    int field_0xb6c;
    int field_0xb70;
    int field_0xb74;                  // 0xb74
    int revLimiterActive;              // 0xb78  rev limiter active
    int field_0xb7c;                  // 0xb7c
    int field_0xb80;                  // 0xb80
    int shiftInProgress;               // 0xb84
    int steeringInputMode;
    int analogThrottle;
    int analogBrake;
    int automaticReverse;              // 0xb94  automatic reverse selected; swaps pedal routing
    int gearAtOrBelowBest;             // 0xb98  current gear is at or below the best one
    int automaticGearbox;              // 0xb9c  automatic gearbox enabled
    int field_0xba0[3];               // 0xba0
    union {
        struct {
            int cornerOnGround[8];    // 0xbac per corner: touching the ground
            int field_0xbcc[4];       // 0xbcc
        };
        struct {
            int field_0xbac[4];
            int objectContactState[8]; // 0xbbc cleared before object traversal;
                                       // first four alias cornerOnGround[4..7]
        };
    };
    int field_0xbdc;                  // 0xbdc
    int field_0xbe0;                  // 0xbe0
    int field_0xbe4;                  // 0xbe4
    int wheelSlipping[4];             // 0xbe8  set while the wheel spins faster than the ground
    int field_0xbf8;                  // 0xbf8
    int field_0xbfc;                  // 0xbfc
    int useUpperCollisionCorners;      // 0xc00  8 corners instead of 4 when set
    int field_0xc04[2];               // 0xc04
    int isGhost;                  // 0xc0c
    int field_0xc10;                  // 0xc10
    unsigned int field_0xc14;
    int field_0xc18;                  // 0xc18
    int field_0xc1c;
    int field_0xc20;
};

// One wheel in a transform snapshot (0x18 bytes). Angles are signed
// 16.16 degrees: x deformation tilt, y steering, z integrated wheel spin.
struct CarWheelRecord {
    FixVector position;             // 0x00 hub position in body space
    int tiltDegrees;                // 0x0c
    int steeringDegrees;            // 0x10
    int spinDegrees;                // 0x14
};

// Stored transforms of a car, written by the physics and applied to the
// scene nodes each frame.
struct CarTransforms {
    FixMatrix body;                 // 0x0   applied to Car::pSceneRoot
    FixMatrix body2;                // 0x40  applied to Car::pBodyNode
    CarWheelRecord wheels[4];       // 0x80
    FixVector groundNormal;         // 0xe0  normal of the ground under the car
    int cornerHeight[4];            // 0xec  ground height under the body corners
};

// GLOBAL: CMR2 0x0053b560
extern CarTransforms g_carTransforms[8];
// Mirror of the per-car transforms the physics row (0x53a3a8) is restored
// from, refreshed at the end of the update (0x42af50, 0x42bcd0). Defined in
// Car.cpp with its GLOBAL annotation.
extern CarTransforms g_carTransformsShadow[8];
// GLOBAL: CMR2 0x0053bda0
extern FixMatrix g_carWheelTransforms[8][4];
// GLOBAL: CMR2 0x0053a3a0
extern short g_carOrderCount;
// GLOBAL: CMR2 0x0053b500
extern short g_carOrder[48];
// GLOBAL: CMR2 0x0053a324
extern int g_carViewScale[15][2];
// GLOBAL: CMR2 0x00538e2c
extern SceneNode *g_viewNodes[3];

// arccos as a 12-bit angle: 4096 entries for a dot product in [-1, 1]
extern short g_acosTable[4096];

inline short FixAcos(int x)
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
    t = X87S_MUL(X87S_MUL((double)x, CGraphics::m_oneOver65536), -4095.0);
    if (neg) {
        return -g_acosTable[-FTOI(__int64, t)];
    }
    return g_acosTable[-FTOI(__int64, t)];
}

// GLOBAL: CMR2 0x0053aba8
extern Car *g_cars[40];
// GLOBAL: CMR2 0x0053bd68
extern int g_carCount;
// GLOBAL: CMR2 0x0053c9a4
extern Car *g_carBuffer;
// GLOBAL: CMR2 0x0053cc18
extern Car *g_pCurrentCar;

// GLOBAL: CMR2 0x00519c8c
extern int g_physicsTimeStep;
// GLOBAL: CMR2 0x00519c90
extern int g_physicsScale;

// Scratch globals of the body lean solver (Car_UpdateBodyLean)
// GLOBAL: CMR2 0x0053c9f8
extern FixVector g_leanDamping;
// GLOBAL: CMR2 0x0053ca18
extern FixVector g_leanAccel;
// GLOBAL: CMR2 0x0053ca30
extern FixVector g_leanDelta;
// GLOBAL: CMR2 0x0053ca78
extern FixVector g_carAccel;
// GLOBAL: CMR2 0x0053caa8
extern FixBasis g_leanBasis;

void Car_AllocateTable(int count);
void Car_ApplyViewTransforms(int viewIndex);
void Car_UpdateViewNodes(int viewIndex);
Car *Car_Get(int index);
void Car_UpdateGroundNormal(void);
void Car_RelaxBodyAxes(int bFast);
void Car_StoreBodyMatrix(void);
void Car_UpdateBodyMatrix(void);
void Car_UpdateCornerVelocities(void);
void Car_UpdateBodyAxes(void);
void Car_UpdateEngineSpeed(void);
void Car_ApplyCornerFriction(int grip);
void Car_BalanceWheelPairs(void);
void Car_UpdateWheelLoads(void);
void Car_UpdateCorners(Car *pCar);
void Car_ApplyCornerOffsets(void);
void Car_UpdateBodyAxesNoDamping(void);
void Car_Integrate(void);
void Car_UpdateWheelForces(void);
void Car_UpdateBodyLean(void);
void Car_UpdateAutomaticGear(void);

// Network pose record of a car (0xec bytes); eight rows start at 0x5393d8
// (g_carNetRecords, owned by StageTiming.cpp). The remote cars are advanced
// from it between packets.
struct CarNetRecord {
    FixMatrix matrix;           // 0x00 body matrix sent / rebuilt
    FixVector right;            // 0x40 body axes the matrix is built from
    FixVector up;               // 0x4c
    FixVector forward;          // 0x58
    FixVector position;         // 0x64
    FixVector velocity;         // 0x70
    FixVector angularVelocity;  // 0x7c
    FixVector accel;            // 0x88 per-step drive along moveDir
    FixVector moveDir;          // 0x94 unit direction of the last movement
    FixVector contactPoint;     // 0xa0
    int engineSpeed;            // 0xac
    int engineSpeedInv;         // 0xb0 1/engineSpeed
    int field_0xb4;             // 0xb4 smoothed field_0xbc
    int throttleTorque;        // 0xb8 normalized pedal on decode, then model-scaled torque
    int field_0xbc;
    int field_0xc0;
    unsigned short wheelSteeringAngle;     // 0xc4
    unsigned short holdTicks;   // 0xc6
    unsigned short seq;         // 0xc8 packet sequence it was advanced to
    unsigned short lastSeq;     // 0xca
    BYTE flag_0xcc;
    BYTE pad_0xcd[3];
    int braking;               // 0xd0 saved Car::braking
    int useUpperCollisionCorners; // 0xd4 saved Car::useUpperCollisionCorners
    int resetPose;              // 0xd8 write the basis straight into the matrix
    int moving;                 // 0xdc moveDir is valid
    int updated;                // 0xe0 a packet arrived since the last step
    int field_0xe4;
    int resync;                 // 0xe8 drop the tick delta once
};

#include "CameraRecord.h"

// Defined in FixedPoint.cpp; declared here because adding it to FixedPoint.h
// perturbs the code MSVC6 generates for every translation unit that includes it.
void FixBasis_Integrate(FixVector *pRows, FixVector *pW);

#endif
