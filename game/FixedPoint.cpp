#include "CornerCameraRoll.h"
#include "CameraOperator.h"
#include "EffectAmounts.h"
#include "port/sys.h"
#include "FovPreview.h"
#include "FovDolly.h"
#include "FloatToInt.h"
#include "FixedPoint.h"
#include "GraphicsOptions.h"
#include "render/fog_math.h"
#include "FogDynamics.h"
#include "MovingObject.h"
#include "Graphics.h"
#include "GameInfo.h"
#include "Car.h"
#include "CarParts.h"
#include "StageTiming.h"
#include "main.h"
#include <stdlib.h>

// GLOBAL: CMR2 0x0072d67c
int g_fixMatrixMultiplyCount;

// Converts a 16.16 matrix into a Direct3D float matrix.
// FUNCTION: CMR2 0x004ba090
GfxMatrix *FixMatrix_ToFloat(GfxMatrix *pOut, FixMatrix *pIn)
{
    pOut->_11 = X87S_MUL(X87S_FILD(float, pIn->right.x), CGraphics::m_oneOver65536);
    pOut->_12 = X87S_MUL(X87S_FILD(float, pIn->right.y), CGraphics::m_oneOver65536);
    pOut->_13 = X87S_MUL(X87S_FILD(float, pIn->right.z), CGraphics::m_oneOver65536);
    pOut->_14 = X87S_MUL(X87S_FILD(float, pIn->rw), CGraphics::m_oneOver65536);
    pOut->_21 = X87S_MUL(X87S_FILD(float, pIn->up.x), CGraphics::m_oneOver65536);
    pOut->_22 = X87S_MUL(X87S_FILD(float, pIn->up.y), CGraphics::m_oneOver65536);
    pOut->_23 = X87S_MUL(X87S_FILD(float, pIn->up.z), CGraphics::m_oneOver65536);
    pOut->_24 = X87S_MUL(X87S_FILD(float, pIn->uw), CGraphics::m_oneOver65536);
    pOut->_31 = X87S_MUL(X87S_FILD(float, pIn->forward.x), CGraphics::m_oneOver65536);
    pOut->_32 = X87S_MUL(X87S_FILD(float, pIn->forward.y), CGraphics::m_oneOver65536);
    pOut->_33 = X87S_MUL(X87S_FILD(float, pIn->forward.z), CGraphics::m_oneOver65536);
    pOut->_34 = X87S_MUL(X87S_FILD(float, pIn->fw), CGraphics::m_oneOver65536);
    pOut->_41 = X87S_MUL(X87S_FILD(float, pIn->position.x), CGraphics::m_oneOver65536);
    pOut->_42 = X87S_MUL(X87S_FILD(float, pIn->position.y), CGraphics::m_oneOver65536);
    pOut->_43 = X87S_MUL(X87S_FILD(float, pIn->position.z), CGraphics::m_oneOver65536);
    pOut->_44 = X87S_MUL(X87S_FILD(float, pIn->pw), CGraphics::m_oneOver65536);
    return pOut;
}

// Float 4x4 matrix product out = a * b (row vectors), through local copies.
// FUNCTION: CMR2 0x004b9ff0
GfxMatrix *FloatMatrix_Multiply(GfxMatrix *pOut, GfxMatrix *pA, GfxMatrix *pB)
{
    float a[4][4];
    float b[4][4];
    float result[4][4];
    int i, j, k;

    *(GfxMatrix *)a = *pA;
    *(GfxMatrix *)b = *pB;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result[i][j] = 0.0f;
            for (k = 0; k < 4; k++) {
                float value = b[k][j];
                result[i][j] += value * a[i][k];
            }
        }
    }
    *pOut = *(GfxMatrix *)result;
    return pOut;
}

// Converts a Direct3D float matrix into 16.16.
// FUNCTION: CMR2 0x004ba160
FixMatrix *FloatMatrix_ToFix(FixMatrix *pOut, GfxMatrix *pIn)
{
    ((int *)pOut)[0] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[0], CGraphics::m_65536));
    ((int *)pOut)[1] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[1], CGraphics::m_65536));
    ((int *)pOut)[2] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[2], CGraphics::m_65536));
    ((int *)pOut)[3] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[3], CGraphics::m_65536));
    ((int *)pOut)[4] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[4], CGraphics::m_65536));
    ((int *)pOut)[5] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[5], CGraphics::m_65536));
    ((int *)pOut)[6] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[6], CGraphics::m_65536));
    ((int *)pOut)[7] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[7], CGraphics::m_65536));
    ((int *)pOut)[8] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[8], CGraphics::m_65536));
    ((int *)pOut)[9] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[9], CGraphics::m_65536));
    ((int *)pOut)[10] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[10], CGraphics::m_65536));
    ((int *)pOut)[11] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[11], CGraphics::m_65536));
    ((int *)pOut)[12] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[12], CGraphics::m_65536));
    ((int *)pOut)[13] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[13], CGraphics::m_65536));
    ((int *)pOut)[14] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[14], CGraphics::m_65536));
    ((int *)pOut)[15] = (int)FTOI(__int64, X87S_MUL(((float *)pIn)[15], CGraphics::m_65536));
    return pOut;
}

// Projects a point through a 16.16 view-projection matrix to screen offsets
// from the centre (16.16 pixels, y up). The z row is computed but unused.
extern const float g_floatHalf;

// FUNCTION: CMR2 0x004ba870
// Moved to FixedMatrixCore.cpp (PORT): projection primitives.

// FUNCTION: CMR2 0x004ba2b0

// Applies a rotation about the matrix's right axis while preserving position.
// FUNCTION: CMR2 0x00422e70
void FixMatrix_RotateAboutRight(FixMatrix *pOut, unsigned short angle)
{
    FixVector position;
    FixMatrix rotation;
    FixVector axis;

    FixMatrix_GetPosition(&position, pOut);
    pOut->position.x = 0;
    pOut->position.y = 0;
    pOut->position.z = 0;
    FixMatrix_Identity(&rotation);

    axis.x = 0x10000;
    axis.y = 0;
    axis.z = 0;
    FixMatrix_SetRight(&axis, &rotation);

    axis.x = 0;
    axis.y = g_sinTable[(angle + 0x400) & 0xfff];
    axis.z = g_sinTable[angle & 0xfff];
    FixMatrix_SetUp(&axis, &rotation);

    axis.x = 0;
    axis.y = -g_sinTable[angle & 0xfff];
    axis.z = g_sinTable[(angle + 0x400) & 0xfff];
    FixMatrix_SetForward(&axis, &rotation);

    FixMatrix_Multiply(pOut, &rotation, pOut);
    FixMatrix_SetPosition(&position, pOut);
}

// Inverts an affine 16.16 matrix (3x3 rotation/scale plus translation) through
// its cofactors; a singular matrix gives the identity.
// FUNCTION: CMR2 0x004ba440

// FUNCTION: CMR2 0x004b9f20
FixMatrix *FixMatrix_Multiply(FixMatrix *pOut, FixMatrix *pA, FixMatrix *pB)
{
    int b[16];
    int a[16];
    int result[16];
    int i, j, k;

    g_fixMatrixMultiplyCount++;
    *(FixMatrix *)a = *pA;
    *(FixMatrix *)b = *pB;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result[i * 4 + j] = 0;
            for (k = 0; k < 4; k++)
                result[i * 4 + j] += FixMul(a[i * 4 + k], b[k * 4 + j]);
        }
    }
    *pOut = *(FixMatrix *)result;
    return pOut;
}

// FixMatrix_RotateVector (CMR2 0x004b9c20) lives in FixedMatrixCore.cpp - moved
// unchanged so the rendered-beam harness links the production primitive.

// out = (v . right, v . up, v . forward) (parent -> local space).
// FUNCTION: CMR2 0x004b9da0
int FixMatrix_InverseRotateVector(FixVector *pOut, FixVector *pV, FixMatrix *pM)
{
    pOut->x = FixMul(pM->right.x, pV->x) + FixMul(pM->right.y, pV->y) + FixMul(pM->right.z, pV->z);
    pOut->y = FixMul(pM->up.x, pV->x) + FixMul(pM->up.y, pV->y) + FixMul(pM->up.z, pV->z);
    pOut->z = FixMul(pM->forward.x, pV->x) + FixMul(pM->forward.y, pV->y) + FixMul(pM->forward.z, pV->z);
    return 1;
}

// Rotates a float vector through the matrix columns.
// FUNCTION: CMR2 0x004b9d40
int FloatMatrix_RotateVector(float *pOut, float *pV, float *pM)
{
    pOut[0] = pV[0] * pM[0] + pM[4] * pV[1] + pM[8] * pV[2];
    pOut[1] = pM[1] * pV[0] + pM[5] * pV[1] + pM[9] * pV[2];
    pOut[2] = pM[2] * pV[0] + pM[6] * pV[1] + pM[10] * pV[2];
    return 1;
}

// Rotates a float vector by the three rows of a float 4x4 matrix.
// FUNCTION: CMR2 0x004b9ec0
int FloatMatrix_InverseRotateVector(float *pOut, float *pV, float *pM)
{
    double first = pM[2] * pV[2];
    first = X87S_ADD(first, pM[1] * pV[1]);
    pOut[0] = (float)(X87S_ADD(first, pM[0] * pV[0]));
    pOut[1] = pM[5] * pV[1] + (pM[4] * pV[0] + pM[6] * pV[2]);
    pOut[2] = pM[9] * pV[1] + (pM[8] * pV[0] + pM[10] * pV[2]);
    return 1;
}

// Copies the rotation part (right/up/forward) of a matrix.
// FUNCTION: CMR2 0x004bab80
void FixMatrix_CopyRotation(FixMatrix *pSrc, FixMatrix *pDst)
{
    pDst->right.x = pSrc->right.x;
    pDst->right.y = pSrc->right.y;
    pDst->right.z = pSrc->right.z;
    pDst->up.x = pSrc->up.x;
    pDst->up.y = pSrc->up.y;
    pDst->up.z = pSrc->up.z;
    pDst->forward.x = pSrc->forward.x;
    pDst->forward.y = pSrc->forward.y;
    pDst->forward.z = pSrc->forward.z;
    pDst->position.x = pSrc->position.x;
    pDst->position.y = pSrc->position.y;
    pDst->position.z = pSrc->position.z;
}

// FUNCTION: CMR2 0x004bac40
void FixMatrix_GetPosition(FixVector *pOut, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->position;
    *pOut = *p;
}

// FUNCTION: CMR2 0x004bac60
void FixMatrix_GetRight(FixVector *pOut, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->right;
    *pOut = *p;
}

// FUNCTION: CMR2 0x004bac80
void FixMatrix_GetUp(FixVector *pOut, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->up;
    *pOut = *p;
}

// FUNCTION: CMR2 0x004baca0
void FixMatrix_GetForward(FixVector *pOut, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->forward;
    *pOut = *p;
}

// FUNCTION: CMR2 0x004bacc0
void FixMatrix_SetPosition(FixVector *pV, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->position;
    *p = *pV;
}

// FUNCTION: CMR2 0x004bace0
void FixMatrix_SetRight(FixVector *pV, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->right;
    *p = *pV;
}

// FUNCTION: CMR2 0x004bad00
void FixMatrix_SetUp(FixVector *pV, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->up;
    *p = *pV;
}

// FUNCTION: CMR2 0x004bad20
void FixMatrix_SetForward(FixVector *pV, FixMatrix *pM)
{
    FixVector *p;

    p = &pM->forward;
    *p = *pV;
}

// Rodrigues rotation of vectors a and b about the unit axis k, in place,
// with the scratch values in locals (see ROTATE_ABOUT_AXIS in SceneNode.cpp
// for the version using globals).
#define ROTATE_BASIS(k, a, b, angle)                                                  \
    {                                                                                 \
        FixMatrix m;                                                                  \
        FixVector v;                                                                  \
                                                                                      \
        s = FixSin(-(angle));                                                         \
        c = FixCos(angle);                                                            \
        omc = 0x10000 - c;                                                            \
        kxx = FixMul(k.x, k.x);                                                       \
        kyy = FixMul(k.y, k.y);                                                       \
        kzz = FixMul(k.z, k.z);                                                       \
        kxs = FixMul(k.x, s);                                                         \
        kys = FixMul(k.y, s);                                                         \
        kzs = FixMul(k.z, s);                                                         \
        kxy = FixMul(FixMul(k.x, k.y), omc);                                          \
        kxz = FixMul(FixMul(k.x, k.z), omc);                                          \
        kyz = FixMul(FixMul(k.z, k.y), omc);                                          \
                                                                                      \
        m.right.x = FixMul(c, 0x10000 - kxx) + kxx;                                   \
        m.right.y = kxy - kzs;                                                        \
        m.right.z = kys + kxz;                                                        \
        m.up.x = kzs + kxy;                                                           \
        m.up.y = FixMul(c, 0x10000 - kyy) + kyy;                                      \
        m.forward.y = kxs + kyz;                                                      \
        m.up.z = kyz - kxs;                                                           \
        m.forward.x = kxz - kys;                                                      \
        m.forward.z = FixMul(c, 0x10000 - kzz) + kzz;                                 \
        m.position.x = 0;                                                             \
        m.position.y = 0;                                                             \
        m.position.z = 0;                                                             \
                                                                                      \
        v.x = FixMul(m.right.x, a.x) + FixMul(m.up.x, a.y) + FixMul(m.forward.x, a.z); \
        v.y = FixMul(m.right.y, a.x) + FixMul(m.up.y, a.y) + FixMul(m.forward.y, a.z); \
        v.z = FixMul(m.right.z, a.x) + FixMul(m.up.z, a.y) + FixMul(m.forward.z, a.z); \
        len = FixVecLength(&v);                                                       \
        if (len == 0) {                                                               \
            a.x = 0;                                                                  \
            a.y = 0;                                                                  \
            a.z = 0;                                                                  \
        } else {                                                                      \
            FixVecScaleRecip(&a, &v, len);                                            \
        }                                                                             \
                                                                                      \
        v.x = FixMul(m.right.x, b.x) + FixMul(m.up.x, b.y) + FixMul(m.forward.x, b.z); \
        v.y = FixMul(m.right.y, b.x) + FixMul(m.up.y, b.y) + FixMul(m.forward.y, b.z); \
        v.z = FixMul(m.right.z, b.x) + FixMul(m.up.z, b.y) + FixMul(m.forward.z, b.z); \
        len = FixVecLength(&v);                                                       \
        if (len == 0) {                                                               \
            b.x = 0;                                                                  \
            b.y = 0;                                                                  \
            b.z = 0;                                                                  \
        } else {                                                                      \
            FixVecScaleRecip(&b, &v, len);                                            \
        }                                                                             \
    }

// Rotates a basis in place by the three 12-bit angles (about up, then
// forward, then right).
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00429f20
void FixBasis_Rotate(FixBasis *pBasis, unsigned short *pAngles)
{
    int s, c, omc, kxx, kyy, kzz, kxs, kys, kzs, kxy, kxz, kyz;
    int len;

    if (pAngles[1] != 0)
        ROTATE_BASIS(pBasis->up, pBasis->right, pBasis->forward, pAngles[1])
    if (pAngles[2] != 0)
        ROTATE_BASIS(pBasis->forward, pBasis->right, pBasis->up, pAngles[2])
    if (pAngles[0] != 0)
        ROTATE_BASIS(pBasis->right, pBasis->up, pBasis->forward, pAngles[0])
}

// Builds the rotation matrix for a rotation of angle (12-bit) about the
// unit axis (Rodrigues), with the scratch products left in the globals.
// FUNCTION: CMR2 0x004adb10
void FixMatrix_FromAxisAngle(FixMatrix *pOut, FixVector *pAxis, int angle)
{
    int neg;

    neg = -angle;
    g_rotSin = g_sinTable[neg & 0xfff];
    g_rotCos = g_sinTable[(neg + 0x400) & 0xfff];
    g_rotOneMinusCos = 0x10000 - g_rotCos;
    g_rotAxisXX = FixMul(pAxis->x, pAxis->x);
    g_rotAxisYY = FixMul(pAxis->y, pAxis->y);
    g_rotAxisZZ = FixMul(pAxis->z, pAxis->z);
    g_rotAxisXY = FixMul(FixMul(pAxis->x, pAxis->y), g_rotOneMinusCos);
    g_rotAxisXZ = FixMul(FixMul(pAxis->x, pAxis->z), g_rotOneMinusCos);
    g_rotAxisYZ = FixMul(FixMul(pAxis->z, pAxis->y), g_rotOneMinusCos);

    pOut->right.x = FixMul(g_rotCos, 0x10000 - g_rotAxisXX) + g_rotAxisXX;
    pOut->right.y = g_rotAxisXY - FixMul(pAxis->z, g_rotSin);
    pOut->right.z = FixMul(pAxis->y, g_rotSin) + g_rotAxisXZ;
    pOut->rw = 0;
    pOut->up.x = FixMul(pAxis->z, g_rotSin) + g_rotAxisXY;
    pOut->up.y = FixMul(g_rotCos, 0x10000 - g_rotAxisYY) + g_rotAxisYY;
    pOut->up.z = g_rotAxisYZ - FixMul(pAxis->x, g_rotSin);
    pOut->uw = 0;
    pOut->forward.x = g_rotAxisXZ - FixMul(pAxis->y, g_rotSin);
    pOut->forward.y = FixMul(pAxis->x, g_rotSin) + g_rotAxisYZ;
    pOut->forward.z = FixMul(g_rotCos, 0x10000 - g_rotAxisZZ) + g_rotAxisZZ;
    pOut->fw = 0;
    pOut->position.x = 0;
    pOut->position.y = 0;
    pOut->position.z = 0;
    pOut->pw = 0x10000;
}

// Rotation about the Z axis by a 12-bit angle.
// FUNCTION: CMR2 0x004ba320
void FixMatrix_RotationZ(FixMatrix *pOut, short angle)
{
    int m[16] = { g_sinTable[(angle + 0x400) & 0xfff], FixSin(angle), 0, 0,
                  -FixSin(angle), g_sinTable[(angle + 0x400) & 0xfff], 0, 0,
                  0, 0, 0x10000, 0,
                  0, 0, 0, 0x10000 };

    *pOut = *(FixMatrix *)m;
}

// out = in * M (rotation plus translation).
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004baa40

// Applies pM to pIn about the pivot (x, y): translate(-pivot), pM,
// translate(+pivot). pIn is updated with each intermediate result.
// FUNCTION: CMR2 0x004ba3b0
void FixMatrix_TransformAboutPivot(FixVector *pOut, FixVector *pIn, FixVector *pPivot, FixMatrix *pM)
{
    FixMatrix t;

    FixMatrix_Identity(&t);
    t.position.x = -pPivot->x;
    t.position.y = -pPivot->y;
    FixMatrix_TransformPoint(pOut, pIn, &t);
    pIn->x = pOut->x;
    pIn->y = pOut->y;
    pIn->z = pOut->z;
    FixMatrix_TransformPoint(pOut, pIn, pM);
    pIn->x = pOut->x;
    pIn->y = pOut->y;
    pIn->z = pOut->z;
    t.position.x = pPivot->x;
    t.position.y = pPivot->y;
    FixMatrix_TransformPoint(pOut, pIn, &t);
}

// FUNCTION: CMR2 0x004babe0
void FixMatrix_CopyRotationFrom(FixMatrix *pDst, FixMatrix *pSrc)
{
    pDst->right.x = pSrc->right.x;
    pDst->right.y = pSrc->right.y;
    pDst->right.z = pSrc->right.z;
    pDst->up.x = pSrc->up.x;
    pDst->up.y = pSrc->up.y;
    pDst->up.z = pSrc->up.z;
    pDst->forward.x = pSrc->forward.x;
    pDst->forward.y = pSrc->forward.y;
    pDst->forward.z = pSrc->forward.z;
    pDst->position.x = pSrc->position.x;
    pDst->position.y = pSrc->position.y;
    pDst->position.z = pSrc->position.z;
}

// Length of a vector; components beyond +-100.0 are scaled down by 512
// first so the squares do not overflow.
// FUNCTION: CMR2 0x004bb0a0
unsigned int FixVec_Length(FixVector *pV)
{
    FixVector scaled;

    if (pV->x <= 0x640000 && pV->x >= -0x640000 && pV->y <= 0x640000 && pV->y >= -0x640000 && pV->z <= 0x640000 && pV->z >= -0x640000)
        return FixVecLength(pV);
    scaled.x = pV->x / 512;
    scaled.y = pV->y / 512;
    scaled.z = pV->z / 512;
    return FixVecLength(&scaled) << 9;
}

// Projects a world point to screen pixels through the view node's camera
// (off-screen marker -100,-100 when behind the camera).
// FUNCTION: CMR2 0x004bad40
// Normalises a vector; components beyond +-100.0 are scaled down by 512
// first so the squares do not overflow.
// FUNCTION: CMR2 0x004bae10
void FixVec_Normalize(FixVector *pOut, FixVector *pIn)
{
    FixVector scaled;
    int len;
    int scaledLen;

    if (pIn->x <= 0x640000 && pIn->x >= -0x640000 && pIn->y <= 0x640000 && pIn->y >= -0x640000 && pIn->z <= 0x640000 && pIn->z >= -0x640000) {
        len = FixVecLength(pIn);
        if (len == 0) {
            pOut->x = 0;
            pOut->y = 0;
            pOut->z = 0;
            return;
        }
        FixVecScaleRecip(pOut, pIn, len);
        return;
    }
    scaled.x = pIn->x / 512;
    scaled.y = pIn->y / 512;
    scaled.z = pIn->z / 512;
    scaledLen = FixVecLength(&scaled);
    if (scaledLen == 0) {
        pOut->x = 0;
        pOut->y = 0;
        pOut->z = 0;
        return;
    }
    FixVecScaleRecip(pOut, &scaled, scaledLen);
}

// Scratch vectors of FixMatrix_Interpolate
// GLOBAL: CMR2 0x005391d8
FixVector g_interpA;
// GLOBAL: CMR2 0x005391b8
FixVector g_interpB;
// GLOBAL: CMR2 0x005391e8
FixVector g_interpRight;
// GLOBAL: CMR2 0x00538e20
FixVector g_interpUp;
// GLOBAL: CMR2 0x00539008
FixVector g_interpForward;
// GLOBAL: CMR2 0x00538e10
FixVector g_interpPos;

#define FIX_LERP(out, a, b, t)                                                      \
    out.x = b.x - a.x;                                                              \
    out.y = b.y - a.y;                                                              \
    out.z = b.z - a.z;                                                              \
    FixVecScale(&out, &out, t);                                                     \
    out.x += a.x;                                                                   \
    out.y += a.y;                                                                   \
    out.z += a.z;

#define FIX_NORMALIZE(v)                                                            \
    {                                                                               \
        int len = FixVecLength(&v);                                                 \
        if (len == 0) {                                                             \
            v.x = 0;                                                                \
            v.y = 0;                                                                \
            v.z = 0;                                                                \
        } else {                                                                    \
            FixVecScaleRecip(&v, &v, len);                                          \
        }                                                                           \
    }

// Interpolates between two matrices: the right vector and either the up
// (mode 0) or forward vector are lerped and renormalised, the third axis
// comes from cross products; the position is lerped with tPos.
// FUNCTION: CMR2 0x004224e0
void FixMatrix_Interpolate(FixMatrix *pOut, FixMatrix *pA, FixMatrix *pB, int tRight, int tAxis, int tPos, int mode)
{
    FixMatrix_GetRight(&g_interpA, pA);
    FixMatrix_GetRight(&g_interpB, pB);
    FIX_LERP(g_interpRight, g_interpA, g_interpB, tRight)
    if (mode != 0) {
        FixMatrix_GetForward(&g_interpA, pA);
        FixMatrix_GetForward(&g_interpB, pB);
        FIX_LERP(g_interpForward, g_interpA, g_interpB, tAxis)
        FIX_NORMALIZE(g_interpForward)
        FixVecCross(&g_interpUp, &g_interpForward, &g_interpRight);
        FIX_NORMALIZE(g_interpUp)
    } else {
        FixMatrix_GetUp(&g_interpA, pA);
        FixMatrix_GetUp(&g_interpB, pB);
        FIX_LERP(g_interpUp, g_interpA, g_interpB, tAxis)
        FIX_NORMALIZE(g_interpUp)
        FixVecCross(&g_interpForward, &g_interpRight, &g_interpUp);
        FIX_NORMALIZE(g_interpForward)
    }
    FixVecCross(&g_interpRight, &g_interpUp, &g_interpForward);
    FixMatrix_GetPosition(&g_interpA, pA);
    FixMatrix_GetPosition(&g_interpB, pB);
    FIX_LERP(g_interpPos, g_interpA, g_interpB, tPos)
    FixMatrix_Identity(pOut);
    FixMatrix_SetRight(&g_interpRight, pOut);
    FixMatrix_SetUp(&g_interpUp, pOut);
    FixMatrix_SetForward(&g_interpForward, pOut);
    FixMatrix_SetPosition(&g_interpPos, pOut);
}

// Rotates a basis by the angular velocity pW: each row gains pW x row.
// match 69%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00441430
void FixBasis_Integrate(FixVector *pRows, FixVector *pW)
{
    FixVector delta;
    int i;

    for (i = 0; i < 3; i++) {
        FixVecCross(&delta, pW, &pRows[i]);
        pRows[i].x += delta.x;
        pRows[i].y += delta.y;
        pRows[i].z += delta.z;
    }
}

extern int g_viewNearPlanes[2];
extern int g_viewFarPlanes[2];
extern int g_viewZooms[2];
int Race_IsMultiplayerRecordMode10(void);
int RallyData_IsChampionshipFinalStage(void);
int RallyData_IsHeadToHeadRaceMode(void);

// PORT (SilentPatchCMR2): the field of view from video.fov in degrees, 70 being
// the original's 0x275c2; like SilentPatch, the scale is inversely
// proportional to it (30 to 150).
static int g_appliedFovSetting;   // OpenCMR2: the video.fov value the cached scale was built from (overlay shows this, not the live INI)
static int g_previewFov;   // OpenCMR2 debug: the live video.fov_preview in force this frame (0 = off)
// OpenCMR2: the fov number actually shown (16.16), eased once per frame toward the preview or the saved value. The projection and the dolly both read
// it, so a manual preview change moves the lens and the camera together and smoothly.
static int View_ShownFov16(int target)
{
    static int shown, lastTime;
    int now = CMain::GetFrameTime();
    if (now != lastTime || shown <= 0) {
        shown = FovPreview_Ease(shown, target * 65536, now - lastTime);
        lastTime = now;
    }
    return shown;
}
static int View_GetFovScale(void)
{
    static int scale;
    int preview = Sys_GetOption("video.fov_preview", 0);
    g_previewFov = preview > 0 ? FovPreview_Effective(preview, 70) : 0;
    if (scale == 0) {
        int fov = Sys_GetOption("video.fov", 70);
        fov = fov < 30 ? 30 : fov > 150 ? 150 : fov;
        g_appliedFovSetting = fov;
        scale = FTOI(int, X87S_DIV(0x275c2 * 70.0, fov));
    }
    int shown = View_ShownFov16(g_previewFov ? g_previewFov : g_appliedFovSetting);
    if (g_previewFov || shown != g_appliedFovSetting * 65536)   // preview on, or still easing back to the saved view
        return FTOI(int, X87S_DIV(0x275c2 * 70.0 * 65536.0, (double)shown));
    return scale;
}

// OpenCMR2: the user's draw distance as a percent of the stage's far plane
// ([video] video.draw_distance), clamped to a sane range.
static int View_GetDrawDistancePercent(void)
{
    return View_ClampDrawDistancePercent(Sys_GetOption("video.draw_distance", 100));
}

// PORT: the original moved the far plane in one frame whenever a route node
// changed the view distance, and a strip of the stage appeared or vanished on
// the horizon. The far plane drawn now follows that distance, closing half the
// gap in about a third of a second. It starts at the distance, without
// sliding, on a new view and after a pause of the frames (a load or a hitch).
static int g_viewShownFarPlanes[2];
static int g_viewFarPlaneTimes[2];

void View_ResetShownFarPlane(BYTE view)
{
    g_viewShownFarPlanes[view] = 0;
}

static int View_GetShownFarPlane(BYTE view)
{
    int target = g_viewFarPlanes[view];
    int now = CMain::GetFrameTime();
    int elapsed = now - g_viewFarPlaneTimes[view];

    g_viewFarPlaneTimes[view] = now;
    if (g_viewShownFarPlanes[view] <= 0 || elapsed < 0 || elapsed > 250)
        g_viewShownFarPlanes[view] = target;
    else
        g_viewShownFarPlanes[view] +=
            (int)((long long)(target - g_viewShownFarPlanes[view]) * elapsed / (elapsed + 500));
    return g_viewShownFarPlanes[view];
}

// OpenCMR2: last projection scale actually sent to the renderer, per view, for the debug overlay.
static int g_lastProjZoom[8], g_lastProjFovY[8], g_lastProjFovX[8];
void View_GetEffectiveProjection(unsigned int view, int *zoom, int *fovY, int *fovSetting)
{
    *zoom = g_lastProjZoom[view & 7];
    *fovY = g_lastProjFovY[view & 7];
    *fovSetting = g_previewFov ? g_previewFov : g_appliedFovSetting ? g_appliedFovSetting : Sys_GetOption("video.fov", 70);   // preview, else applied (cached) base, not a later INI edit
}
// Debug overlay: the live preview (0 = off) and the saved base the preview replaces.
// Debug overlay: the field of view in degrees that the projection matrix really gives: the matrix has 0.5 * fov in elements _11 / _22 (CGraphics::SetProjection),
// so the half angle is atan(1 / (0.5 * fov)). Horizontal and vertical, from the last values sent for the view.
void View_GetMeasuredFov(unsigned int view, double *vertDeg, double *horzDeg)
{
    *vertDeg = FovPreview_MeasuredDegrees(g_lastProjFovY[view & 7] / 65536.0);
    *horzDeg = FovPreview_MeasuredDegrees(g_lastProjFovX[view & 7] / 65536.0);
}
void View_GetFovPreview(int *preview, int *base)
{
    *preview = g_previewFov;
    *base = g_appliedFovSetting ? g_appliedFovSetting : Sys_GetOption("video.fov", 70);
}

// Sets the camera projection for the selected player's view.
// match 66%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00422d40
int View_SpeedFovZoom(BYTE view, int zoom);
bool View_IsDroneCamera(int view);
void View_SetPlayerProjection(unsigned int player)
{
    BYTE i = (BYTE)player;
    // Physics effect updates can precede camera selection. Gate again at the
    // actual rendered view, clearing eased blur/lines without rewriting settings.
    Gfx_SetSpeedEffectsSuppressed(View_IsDroneCamera(player));
    int zoom = View_SpeedFovZoom(i, g_viewZooms[i]);  // OpenCMR2 speed_fov (visual only; identity when off)
    // PORT (SilentPatchCMR2): the original scales X by resX/resY, which is
    // right only at 4:3 and stretches wider resolutions. (16/9) * (resY/resX)
    // is the same at 4:3 and keeps the vertical view on any other shape.
    int fovX = FixMul(0x123d7, FixMul(FixMul(zoom, View_GetFovScale()),
        (int)FTOI(__int64, X87S_MUL(X87S_MUL((16.0 / 9.0), (X87S_DIV((double)g_pGraphics->resY, (double)g_pGraphics->resX))), CGraphics::m_65536))));
    int fovY = FixMul(0x2147a, FixMul(zoom, View_GetFovScale()));
    g_lastProjZoom[i & 7] = zoom;
    g_lastProjFovY[i & 7] = fovY;
    g_lastProjFovX[i & 7] = fovX;
    // PORT: the far plane eases to the view distance (View_GetShownFarPlane).
    // OpenCMR2: the user's draw distance scales the far plane
    // ([video] video.draw_distance).
    int farPlane = (int)((long long)View_GetShownFarPlane(i) * View_GetDrawDistancePercent() / 100);
    // Opt-in fog-aware draw culling ([video] video.fog_cull): shorten the far plane to
    // the fully fogged shell. Render-only; route easing above still applies; physics,
    // the route window and collision are untouched. See Fog_CullDistance.
    if (Sys_GetOption("video.fog_cull", 0) && Sys_GetOption("video.fog", 0)) {
        // Dynamic fog (video.fog_spatial / video.fog_evolve): cull for the clearest
        // stretch the curves can reach, never the current value, so opening fog
        // cannot pop culled geometry back into view.
        float fogCull = Fog_CullDistance((int)(Sys_GetOption("video.fog_amount", 65) *
            FogDynamics_MinMult(Sys_GetOption("video.fog_spatial", 0), Sys_GetOption("video.fog_evolve", 100))),
            Sys_GetOption("video.fog_clear", 0));
        int fogCullFixed = (int)(fogCull * 65536.0f);
        if (fogCull > 0.f && farPlane > fogCullFixed) farPlane = fogCullFixed;
    }

    if (Race_IsMultiplayerRecordMode10() == 0 && RallyData_IsHeadToHeadRaceMode()) {
        if (CGameInfo::IsSplitBarEnabled())
            fovY *= 2;
        else
            fovX *= 2;
    }
    if (CGameInfo::IsActiveCheatEnabled(2)) {
        CGraphics::SetProjection(-fovX, fovY, farPlane, g_viewNearPlanes[i]);
        return;
    }
    if ((BYTE)RallyData_IsChampionshipFinalStage() && farPlane < 0x960000) {
        CGraphics::SetProjection(fovX, fovY, 0x960000, g_viewNearPlanes[i]);
        return;
    }
    CGraphics::SetProjection(fovX, fovY, farPlane, g_viewNearPlanes[i]);
}

// Layout of one force-feedback slot (0x539200). The full definition lives in
// StageTiming.cpp; keep both copies in sync.
struct ForceFeedbackSlot {
    int appliedSpring;          // the forces last sent: spring, damper, constant
    int appliedDamper;
    int appliedConstant;
    int spring;                 // the forces of this step
    int damper;
    int constant;               // the steering force
    int textureForce;           // ControllerData::textureForce, scales the steering force
    int effectsForce;           // ControllerData::effectsForce, scales spring and damper
    int roadNoise;              // random force of the road surface, alternating sign
    int impactKick;             // fading kick of a front or rear impact
    int impactShake;            // strength of the last impact, added to the road noise
    signed char device;         // device index, < 0 when none
    BYTE pad_0x2d[3];
    int impactKickReversed;     // the kick pushes the other way
    int active;                 // slot in use
};
extern ForceFeedbackSlot g_forceFeedbackSlots[2];
extern ForceFeedbackSlot *g_pForceFeedbackSlot;
extern Car *g_pForceFeedbackCar;

// Rebuilds the rotation part of a matrix as an orthonormal basis that keeps
// the current forward direction and the world up axis.
// FUNCTION: CMR2 0x00423070
void FixMatrix_RebuildBasis(FixMatrix *pOut)
{
    FixVector forward;
    FixVector up;
    FixVector right;
    int len;

    up.x = 0;
    up.y = 0x10000;
    up.z = 0;
    FixMatrix_GetForward(&forward, pOut);
    FixVecCross(&right, &up, &forward);
    len = FixVecLength(&right);
    if (len == 0) {
        right.x = 0;
        right.y = 0;
        right.z = 0;
    } else {
        FixVecScaleRecip(&right, &right, len);
    }
    FixVecCross(&forward, &right, &up);
    FixMatrix_SetRight(&right, pOut);
    FixMatrix_SetUp(&up, pOut);
    FixMatrix_SetForward(&forward, pOut);
}

// Updates the player's force-feedback slot from the car's slip: the input
// vector is rotated into car space and its z / length scaled to 16.16.
// FUNCTION: CMR2 0x004241d0
void ForceFeedback_UpdateSlot(Car *pCar, FixVector *pIn, int nonzero)
{
    FixVector v;
    int x;

    // PORT: the slots belong to the two local players, but collisions call
    // this for every car; the original indexes past the array with AI cars
    // (index >= 2) and overwrites the globals that follow, g_pForceFeedbackSlot
    // among them. Those updates go to a scratch slot instead.
    static ForceFeedbackSlot s_otherCarSlot;
    g_pForceFeedbackCar = pCar;
    if (pCar->index >= 0 && pCar->index < 2)
        g_pForceFeedbackSlot = &g_forceFeedbackSlots[pCar->index];
    else
        g_pForceFeedbackSlot = &s_otherCarSlot;
    FixMatrix_InverseRotateVector(&v, pIn, pCar->pWorld);
    if (nonzero != 0) {
        x = v.z;
        if (x < 0)
            x = -x;
        x = FixMul(x - 0x1999, 0x28000);
        if (x < 0)
            x = 0;
        else if (x > 0x10000)
            x = 0x10000;
        if (x > g_pForceFeedbackSlot->impactKick) {
            g_pForceFeedbackSlot->impactKick = x;
            if (v.z >= 0)
                g_pForceFeedbackSlot->impactKickReversed = 0;
            else
                g_pForceFeedbackSlot->impactKickReversed = 1;
        }
    }
    x = (unsigned int)FixVecLength(&v);
    x -= 0x1999;
    x = FixMul(x, 0x28000);
    if (x < 0)
        x = 0;
    else if (x > 0x10000)
        x = 0x10000;
    if (x > g_pForceFeedbackSlot->impactShake)
        g_pForceFeedbackSlot->impactShake = x;
}

// Chase camera state (HudDash.cpp): mode per view record, the records' up and
// forward vectors (entries 2..5) and the pitch.
extern BYTE g_playerCameraMode[8];
extern FixVector g_chaseCameraViewOffsets[6];
extern FixVector g_chaseCameraOffsets[4];
extern short g_chaseCameraPitch[4];
void ChaseCamera_GetViewOffset(FixVector *pOut, CameraRecord *pSel);

void ChaseCamera_BuildMatrix(CameraRecord *pObj, FixMatrix *pRef);

// OpenCMR2 FOV dolly (visual only, game/FovDolly.h): the chase camera moves toward the car by the amount its view is wider than the
// saved video.fov (speed FOV and the FOV preview), so the car keeps about the same screen size. Chase types 4..6, not the drone.
int View_SpeedFovMagnification(BYTE view);
int View_CameraMoveStep(int cameraType, int view);   // Car.cpp: signed 16.16 move along the camera forward axis, scaled by speed
static int g_lastDollyScale[8] = {65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536}, g_lastWidening[8] = {65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536};
// Widening (16.16, 65536 = same as saved) of a view right now: speed FOV times preview over saved. Shared by the chase dolly and the cockpit eye shift.
int View_CurrentWidening(unsigned int viewIn)
{
    int view = viewIn & 7;
    int saved = g_appliedFovSetting ? g_appliedFovSetting : Sys_GetOption("video.fov", 70);
    saved = saved < 30 ? 30 : saved > 150 ? 150 : saved;
    int preview = Sys_GetOption("video.fov_preview", 0);
    int target = preview > 0 ? FovPreview_Effective(preview, 70) : saved;
    return FovDolly_Widening(View_SpeedFovMagnification((BYTE)view), saved * 65536, View_ShownFov16(target));
}
static int View_FovDollyScale(CameraRecord *pObj)
{
    int view = pObj->index & 7;
    int saved = g_appliedFovSetting ? g_appliedFovSetting : Sys_GetOption("video.fov", 70);
    saved = saved < 30 ? 30 : saved > 150 ? 150 : saved;
    int preview = Sys_GetOption("video.fov_preview", 0);
    int target = preview > 0 ? FovPreview_Effective(preview, 70) : saved;
    int shown = View_ShownFov16(target);   // same eased value the projection uses this frame
    int widening = FovDolly_Widening(View_SpeedFovMagnification((BYTE)view), saved * 65536, shown);
    int scale = Sys_GetOption("video.fov_dolly_curve", 0) ? FovDolly_ScaleLinear(widening, Sys_GetOption("video.fov_dolly", 150)) : FovDolly_Scale(widening, Sys_GetOption("video.fov_dolly", 150));
    g_lastWidening[view] = widening;
    g_lastDollyScale[view] = scale;
    return scale;
}
void View_GetFovDolly(unsigned int view, int *scale, int *widening)
{
    *scale = g_lastDollyScale[view & 7];
    *widening = g_lastWidening[view & 7];
}

// Caches the reference matrix's up and right basis vectors in the player's
// camera slots, then builds the view object's matrix from them.
// FUNCTION: CMR2 0x00447a00
void ChaseCamera_Restart(CameraRecord *pObj, FixMatrix *pRef)
{
    CameraOperator_Reset(pObj->index);
    FixMatrix_GetUp(&g_chaseCameraOffsets[2 + pObj->index], pRef);
    FixMatrix_GetRight(&g_chaseCameraViewOffsets[2 + pObj->index], pRef);
    ChaseCamera_BuildMatrix(pObj, pRef);
}

// Builds a view object's matrix: the basis comes from the player's camera
// up/forward vectors, the position from the player's view offset plus the
// object's own, and then the object's remaining fields are set.
// FUNCTION: CMR2 0x00447a40
void ChaseCamera_BuildMatrix(CameraRecord *pObj, FixMatrix *pRef)
{
    FixMatrix identity;
    FixVector right;
    FixVector base;
    FixVector off;
    FixVector pos;
    FixMatrix *pM;
    BYTE sel;
    int index;
    int v;

    sel = pObj->index;
    ChaseCamera_GetViewOffset(&base, pObj);
    if (pObj->type >= 4 && pObj->type <= 6 && !CameraOperator_Enabled(pObj)) {
        int dolly = View_FovDollyScale(pObj);
        if (dolly != 65536) { base.x = FixMul(base.x, dolly); base.z = FixMul(base.z, dolly); }   // distance only: the dolly no longer lowers the camera, so the car doesn't block more road as the view widens (visual only)
    }
    FixMatrix_Identity(&identity);
    FixMatrix_SetPosition(&base, &identity);
    index = sel & 0xff;

    FixVecCross(&right, &g_chaseCameraOffsets[2 + index], &g_chaseCameraViewOffsets[2 + index]);
    pM = &pObj->matrix;
    FixMatrix_SetRight(&right, pM);
    FixMatrix_SetUp(&g_chaseCameraOffsets[2 + index], pM);
    FixMatrix_SetForward(&g_chaseCameraViewOffsets[2 + index], pM);
    pObj->matrix.position.x = 0;
    pObj->matrix.position.y = 0;
    pObj->matrix.position.z = 0;
    FixMatrix_Multiply(pM, &identity, pM);
    FixMatrix_GetPosition(&pos, pM);
    FixMatrix_GetPosition(&off, pRef);
    pos.x += off.x;
    pos.y += off.y;
    pos.z += off.z;
    FixMatrix_SetPosition(&pos, pM);
    if (g_playerCameraMode[index] == 4)
        FixMatrix_RotateAboutRight(pM, (unsigned short)g_chaseCameraPitch[2 + pObj->view]);
    {   // OpenCMR2 debug camera move (visual only, video.camera_move_chase): signed move along the final look direction, + forward, - back, 0.05 model units per step.
        int step = (pObj->type >= 4 && pObj->type <= 6 && !CameraOperator_Enabled(pObj)) ? View_CameraMoveStep(pObj->type, pObj->index) : 0;
        if (step != 0) {
            pObj->matrix.position.x += FixMul(pObj->matrix.forward.x, step);
            pObj->matrix.position.y += FixMul(pObj->matrix.forward.y, step);
            pObj->matrix.position.z += FixMul(pObj->matrix.forward.z, step);
        }
    }

    // OpenCMR2: human-operator camera shake — smoothed random jitter on the chase
    // camera position, scaled by car speed and impact strength
    // ([video] video.camera_effects). The camera drifts like a handheld
    // operator instead of jumping randomly every frame.
    if (Sys_GetOption("video.camera_shake",Sys_GetOption("video.camera_effects",1)) != 0 && !CameraOperator_Enabled(pObj)) {
        extern BYTE g_viewObjectCarIndices[16];
        int carIx = g_viewObjectCarIndices[pObj->index];
        Car *pCar = Car_Get(carIx);
        int amp = Effect_Scale(Shake_Amplitude(pCar->speed, pCar->shakeLevel),Sys_GetOption("video.camera_shake_amount",100));
        if (amp > 0) {
            static FixVector shakeTarget;
            static FixVector shakeCurrent;
            static int shakeFrame = 0;
            FixVector jitter;
            jitter.x = (rand() & 0xffff) - 0x8000;
            jitter.y = (rand() & 0xffff) - 0x8000;
            jitter.z = (rand() & 0xffff) - 0x8000;
            int len = FixVecLength(&jitter);
            if (len > 0) {
                FixVecScaleRecip(&jitter, &jitter, len);
            }
            Shake_Smooth(&shakeCurrent, &jitter, amp, &shakeFrame);
            pObj->matrix.position.x += shakeCurrent.x;
            pObj->matrix.position.y += shakeCurrent.y;
            pObj->matrix.position.z += shakeCurrent.z;
        }
    }


    pObj->carLift = 0;
    pObj->nearPlane = 0x10000;
    pObj->liftPitch = 0x10000;
    pObj->zoom = 0xa000;
    v = base.z;
    if (v < 0)
        v = -v;
    pObj->lookDistance = v;
    pObj->field_0x50 = v - 0x10000;
    CameraOperator_Apply(pObj);
    CornerCameraRoll_Apply(pObj);
}

extern CarPartSet *g_physicsPartSet;
CarPartSet *CarPart_GetSet(int carIndex);

// Copies the car's body and world matrices into the local transform of its two
// body scene nodes.
// FUNCTION: CMR2 0x0043ecd0
void Car_SyncBodySceneNodes(Car *pCar)
{
    FixVector v;

    g_pCurrentCar = pCar;
    g_physicsPartSet = CarPart_GetSet((int)pCar->index);
    Car_StoreBodyMatrix();
    FixMatrix_GetRight(&v, g_pCurrentCar->pBodyMatrix);
    g_pCurrentCar->pBodyNode->current.right = v;
    FixMatrix_GetUp(&v, g_pCurrentCar->pBodyMatrix);
    g_pCurrentCar->pBodyNode->current.up = v;
    FixMatrix_GetForward(&v, g_pCurrentCar->pBodyMatrix);
    g_pCurrentCar->pBodyNode->current.forward = v;
    FixMatrix_GetPosition(&v, g_pCurrentCar->pBodyMatrix);
    g_pCurrentCar->pBodyNode->current.position = v;
    FixMatrix_GetRight(&v, g_pCurrentCar->pWorld);
    g_pCurrentCar->pSceneRoot->current.right = v;
    FixMatrix_GetUp(&v, g_pCurrentCar->pWorld);
    g_pCurrentCar->pSceneRoot->current.up = v;
    FixMatrix_GetForward(&v, g_pCurrentCar->pWorld);
    g_pCurrentCar->pSceneRoot->current.forward = v;
    FixMatrix_GetPosition(&v, g_pCurrentCar->pWorld);
    g_pCurrentCar->pSceneRoot->current.position = v;
}

void Car_UpdateCorners(Car *pCar);

// Resets a car's physics state from a saved one (used when the body is put
// back on the road): copies the stored fields and rebuilds the matrix-derived
// vectors of the body.
// FUNCTION: CMR2 0x00426d80
void Car_RestorePhysicsFromRecord(Car *pDst, CarNetRecord *pSrc)
{
    int i;

    pDst->braking = pSrc->braking;
    pDst->engineSpeed = 0;
    pDst->wheelSteeringAngle = pSrc->wheelSteeringAngle;
    pDst->throttleTorque = pSrc->throttleTorque;
    pDst->velocityNext = pDst->velocity;
    pDst->field_0xb35[0] = pSrc->flag_0xcc;
    pDst->useUpperCollisionCorners = pSrc->useUpperCollisionCorners;
    pDst->velocity = pSrc->velocity;
    pDst->angularVelocity = pSrc->angularVelocity;
    FixMatrix_CopyRotation(&pSrc->matrix, pDst->pWorld);
    pDst->pWorld->position.y = pDst->positionPrev.y;

    for (i = 0; i < 4; i++) {
        // the previous corner positions move one slot back (0x300 -> 0x330)
        pDst->cornerPrev2[i] =
            pDst->cornerPrev[i];
        pDst->cornerPrev[i] = pDst->corners[i];
        pDst->previousWheelSurface[i] = pDst->wheelSurface[i];
    }
    for (i = 0; i < 8; i++) {
        pDst->cornerNormal[i] = pDst->cornerAxis[i];
        pDst->objectContactState[i] = 0;
    }
    pDst->field_0x7a8 = pDst->engineSpeed;
    pDst->positionPrev2 = pDst->positionPrev;
    pDst->positionPrev = pDst->position;
    pDst->normal0x498 = pDst->groundNormal;
    memcpy(pDst->targetAxes, pDst->bodyAxes, sizeof(pDst->targetAxes));

    FixMatrix_GetPosition(&pDst->position, pDst->pWorld);
    FixMatrix_GetRight(&pDst->right, pDst->pWorld);
    FixMatrix_GetUp(&pDst->up, pDst->pWorld);
    FixMatrix_GetForward(&pDst->forward, pDst->pWorld);
    Car_UpdateCorners(pDst);
    pDst->field_0x5c4.x = 0;
    pDst->field_0x5c4.y = 0;
    pDst->field_0x5c4.z = 0;
    pDst->field_0x5d0.x = 0;
    pDst->field_0x5d0.y = 0;
    pDst->field_0x5d0.z = 0;
    pDst->field_0xc20 = 1;
}

// Integrates each wheel's restoring/damping force into spring velocity, then
// offset. Near the ground, the corner's second position difference also
// contributes an impulse to velocity.
// FUNCTION: CMR2 0x0042e8e0
void CarPhysics_IntegrateWheelSuspension(void)
{
    FixVector secondDifference;
    FixVector previousStep;
    FixVector currentStep;
    FixVector bodySecondDifference;
    int groundDistance;
    int velocityStep;
    int wheel;

    for (wheel = 0; wheel < 4; wheel++) {
        if (g_pCurrentCar->field_0xb74 != 0) {
            groundDistance = g_pCurrentCar->cornerHeight[wheel] - g_pCurrentCar->corners[wheel].y;
            if (groundDistance < 0)
                groundDistance = -groundDistance;
            if (groundDistance < 0x4ccc &&
                (g_pCurrentCar->cornerPrev2[wheel].x != 0 ||
                 g_pCurrentCar->cornerPrev2[wheel].y != 0 ||
                 g_pCurrentCar->cornerPrev2[wheel].z != 0)) {
                previousStep.x = g_pCurrentCar->cornerPrev[wheel].x - g_pCurrentCar->cornerPrev2[wheel].x;
                previousStep.y = g_pCurrentCar->cornerPrev[wheel].y - g_pCurrentCar->cornerPrev2[wheel].y;
                previousStep.z = g_pCurrentCar->cornerPrev[wheel].z - g_pCurrentCar->cornerPrev2[wheel].z;
                currentStep.x = g_pCurrentCar->corners[wheel].x - g_pCurrentCar->cornerPrev[wheel].x;
                currentStep.y = g_pCurrentCar->corners[wheel].y - g_pCurrentCar->cornerPrev[wheel].y;
                currentStep.z = g_pCurrentCar->corners[wheel].z - g_pCurrentCar->cornerPrev[wheel].z;
                secondDifference.x = currentStep.x - previousStep.x;
                secondDifference.y = currentStep.y - previousStep.y;
                secondDifference.z = currentStep.z - previousStep.z;
                FixMatrix_InverseRotateVector(&bodySecondDifference, &secondDifference, g_pCurrentCar->pWorld);
                // Car-space Y of the corner's second position difference.
                velocityStep = bodySecondDifference.y;
                velocityStep = FixMul(velocityStep, FixMul(0x9c28, g_physicsScale));
                if (velocityStep > 0)
                    g_pCurrentCar->wheelSpringVelocity[wheel] -= velocityStep;
            }
        }
        velocityStep = FixMul(g_physicsTimeStep,
                   -(FixMul(g_pCurrentCar->wheelSpringStiffness[wheel], g_pCurrentCar->wheelSpringOffset[wheel]) +
                     FixMul(g_pCurrentCar->wheelSpringDamping[wheel], g_pCurrentCar->wheelSpringVelocity[wheel])));
        g_pCurrentCar->wheelSpringVelocity[wheel] += velocityStep;
        g_pCurrentCar->wheelSpringOffset[wheel] +=
            FixMul(g_physicsTimeStep, g_pCurrentCar->wheelSpringVelocity[wheel]);
        if (g_pCurrentCar->wheelSpringOffset[wheel] < -0x3333) {
            g_pCurrentCar->wheelSpringOffset[wheel] = -0x3333;
            if (g_pCurrentCar->wheelSpringVelocity[wheel] < 0)
                g_pCurrentCar->wheelSpringVelocity[wheel] = 0;
        }
    }
}

// Per-object tables of the stage object payload (see StageObjects.cpp for the
// tables at 0x590d90 and 0x590db0 they index).

// GLOBAL: CMR2 0x00590ec0
BYTE g_viewObjectCarIndices[16];
extern FixVector *g_carCameraOffsets[8];
extern int g_viewCarLift[64];

// Positions a view from the per-mode offset of its car's CIN section 4,
// transforms it through the current basis and adds the reference position.
// FUNCTION: CMR2 0x004869e0
void View_PositionFromCarProfile(CameraRecord *pObj, FixMatrix *pRef)
{
    FixMatrix identity;
    FixVector src;
    FixVector pos;
    FixVector off;
    FixMatrix *pM;
    unsigned int mode;
    int base;

    if (RallyData_IsHeadToHeadRaceMode() != 0 && CGameInfo::IsSplitBarEnabled() != 0 && Race_IsMultiplayerRecordMode10() == 0) {
        if (g_carPartStateTables.modes[pObj->index] == 0)
            mode = 3;
        else if (g_carPartStateTables.modes[pObj->index] == 2)
            mode = 4;
        else
            mode = g_carPartStateTables.modes[pObj->index];
    } else {
        mode = g_carPartStateTables.modes[pObj->index];
    }
    src = ((FixVector *)g_carCameraOffsets[g_viewObjectCarIndices[pObj->index]])[mode];

    FixMatrix_Identity(&identity);
    FixMatrix_SetPosition(&src, &identity);
    pM = &pObj->matrix;
    pObj->matrix.position.x = 0;
    pObj->matrix.position.y = 0;
    pObj->matrix.position.z = 0;
    FixMatrix_Multiply(pM, &identity, pM);
    FixMatrix_GetPosition(&pos, pM);
    FixMatrix_GetPosition(&off, pRef);
    pos.x += off.x;
    pos.y += off.y;
    pos.z += off.z;
    FixMatrix_SetPosition(&pos, pM);
    {   // OpenCMR2 debug camera move (video.camera_move_car) for the car-mounted views (types 1, 2, 10: bonnet, bumper...). The position is rebuilt above every call, so this never accumulates.
        int step = View_CameraMoveStep(pObj->type, pObj->index);
        if (step != 0) {
            pM->position.x += FixMul(pM->forward.x, step);
            pM->position.y += FixMul(pM->forward.y, step);
            pM->position.z += FixMul(pM->forward.z, step);
        }
    }

    base = g_viewCarLift[pObj->index];
    pObj->zoom = 0xa000;
    pObj->carLift = base;
    pObj->nearPlane = 0x1999;
    pObj->liftPitch = 0;
    pObj->lookDistance = 0x10000;
    pObj->field_0x50 = 0;
}

// Per-octant reference vectors used to probe the ground.
// GLOBAL: CMR2 0x00589458
FixVector g_groundProbeOctantVectors[8];

int Track_GetGroundHeight(FixVector *pPoint, FixVector *pNormal, short *pTri, unsigned short *pSurface,
                          int defaultY);



// Normalises pV into pOut (zero when pV is zero) and returns its length.
inline int FixVecNormalizeLen(FixVector *pOut, FixVector *pV)
{
    int len;

    len = FixVecLength(pV);
    if (len == 0) {
        pOut->x = 0;
        pOut->y = 0;
        pOut->z = 0;
    } else {
        FixVecScaleRecip(pOut, pV, len);
    }
    return len;
}

// Probes the ground under a stage object: casts the ground normal at the given
// point and returns the signed distance from the point to the ground plane.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004702f0
int StageObject_ProbeGroundDistance(MovingObject *pObj, FixVector *pPoint)
{
    unsigned short surface;
    FixVector neg;
    FixVector rot;
    FixVector v;
    FixVector ground;
    int idx;

    pObj->groundHeight = Track_GetGroundHeight(pPoint, &pObj->groundNormal, &pObj->groundTriangle, &surface,
                                               pObj->groundHeight);
    if (pObj->groundTriangle == -1) {
        pObj->groundNormal.x = 0;
        pObj->groundNormal.y = 0x10000;
        pObj->groundNormal.z = 0;
        pObj->groundHeight = pPoint->y - 0xa0000;
    }
    FixVecNormalizeLen(&pObj->groundNormal, &pObj->groundNormal);
    FixVecScale(&neg, &pObj->groundNormal, -0x10000);
    FixMatrix_InverseRotateVector(&rot, &neg, pObj->pMatrix);
    idx = 0;
    if (rot.y >= 0)
        idx = 4;
    if (rot.x < 0)
        idx += 2;
    if (rot.z < 0)
        idx += 1;
    ground = *pPoint;
    ground.y = pObj->groundHeight;
    FixMatrix_RotateVector(&v, &g_groundProbeOctantVectors[idx], pObj->pMatrix);
    v.x += pPoint->x;
    v.y += pPoint->y;
    v.z += pPoint->z;
    v.x = ground.x - v.x;
    v.y = ground.y - v.y;
    v.z = ground.z - v.z;
    pPoint = &pObj->groundNormal;
    return FixDiv(FixVecDot(&v, pPoint), pObj->groundNormal.y);
}

extern double g_fixedDegreesToAngle12;

// Rebuilds an object's local matrix from a reference matrix: mirrors it about
// the object's split axis and applies the car's tilt rotation.
// Differential coverage: affine axes, tilt and split positioning with real helpers.
// FUNCTION: CMR2 0x00486910
void StageObject_RebuildMirroredTiltMatrix(CameraRecord *pObj, int *pSrc)
{
    FixMatrix *pMat = &pObj->matrix;
    FixMatrix *pRef = (FixMatrix *)pSrc;
    int idx;
    int angle;
    int v;

    memcpy(pMat, pSrc, 0x40);
    idx = pObj->index;
    if (g_carPartStateTables.modes[idx] == 2) {
        pMat->right.x = pRef->forward.x;
        pMat->right.y = pRef->forward.y;
        pMat->right.z = pRef->forward.z;
        pMat->forward.x = -pRef->right.x;
        pMat->forward.y = -pRef->right.y;
        angle = -pRef->right.z;
    } else {
        pMat->right.x = -pRef->forward.x;
        pMat->right.y = -pRef->forward.y;
        pMat->right.z = -pRef->forward.z;
        pMat->forward.x = pRef->right.x;
        pMat->forward.y = pRef->right.y;
        angle = pRef->right.z;
    }
    pMat->forward.z = angle;
    angle = g_carPartStateTables.modes[idx] == 0 ? 10 : 0;
    v = (int)FTOI(__int64, X87S_MUL((double)angle, CGraphics::m_65536));
    FixMatrix_RotateAboutRight((FixMatrix *)pMat,
                               (unsigned short)FTOI(__int64, X87S_MUL((double)v, g_fixedDegreesToAngle12)));
    View_PositionFromCarProfile(pObj, (FixMatrix *)pSrc);
}

// Rebuilds an object's local matrix from a reference matrix and interpolates
// it against the previous one (mode 0 keeps the third axis).
// match 65%: same structure and calls; register allocation of the source loads differs
// (setne vs the original's neg/sbb mask for the angle).
// FUNCTION: CMR2 0x00486810
void StageObject_InterpolateReferenceMatrix(CameraRecord *pObj, int *pSrc, int param_3)
{
    int m[16];
    int angle;
    int tAxis;
    __int64 v;

    memcpy(m, pSrc, 0x40);
    if (g_carPartStateTables.modes[pObj->index] == 2) {
        m[0] = pSrc[8];
        m[1] = pSrc[9];
        m[2] = pSrc[10];
        m[8] = -pSrc[0];
        m[9] = -pSrc[1];
        m[10] = -pSrc[2];
    } else {
        m[0] = -pSrc[8];
        m[1] = -pSrc[9];
        m[2] = -pSrc[10];
        m[8] = pSrc[0];
        m[9] = pSrc[1];
        m[10] = pSrc[2];
    }
    angle = g_carPartStateTables.modes[pObj->index] == 0 ? 10 : 0;
    v = (int)FTOI(__int64, X87S_MUL((double)angle, CGraphics::m_65536));
    FixMatrix_RotateAboutRight((FixMatrix *)m,
                               (unsigned short)FTOI(__int64, X87S_MUL((double)(int)v, g_fixedDegreesToAngle12)));
    if (param_3 != 0)
        tAxis = 0x10000;
    else
        tAxis = FixMul(0x4ccc, g_physicsTimeStep);
    FixMatrix_Interpolate(&pObj->matrix, &pObj->matrix, (FixMatrix *)m,
                          0x10000, tAxis, 0x10000, 0);
    View_PositionFromCarProfile(pObj, (FixMatrix *)pSrc);
}

// (defined in StageTiming.cpp, which owns the GLOBAL annotation)
extern CarNetRecord g_carNetRecords[8];

void Car_InvalidateTransforms(int index);
void NetRace_AdvanceRemoteCarAccumulator(CarNetRecord *p);

// Refreshes the network pose record of every car of the given order that is
// still in play (field_0xc20): the record matrix takes the body axes and the
// body position, the record velocity (0x70) and angular velocity (0x7c) are
// extrapolated one step from the body state, and the resulting movement is
// turned into the unit direction at 0x94 the remote cars are advanced along.
// Records whose pose did not change are marked (0xdc = 0) instead.
// match 79%: implementada; el bucle y todos los helpers de vectores inline (13 shrd / 15 shld /
// 28 imul / 1 idiv, como el original) generan las mismas instrucciones, pero MSVC6 mantiene la
// base del registro en EDI con otro reparto de temporales.
// FUNCTION: CMR2 0x00426fc0
void NetRace_ExtrapolateOrderedCarPoses(Car *pCars, short *pOrder, short count)
{
    FixVector saved;
    FixVector v;
    FixVector mpos;
    FixVector d;
    FixVector scaled;
    Car *pCar;
    CarNetRecord *pRec;
    int dot;
    int len;
    int ax;
    int az;
    int i;

    for (i = count - 1; i >= 0; i--) {
        pCar = pCars + pOrder[i];
        pRec = &g_carNetRecords[pOrder[i]];

        if (pCar->field_0xc20 == 0)
            continue;

        FixMatrix_SetPosition(&pCar->position, &pRec->matrix);
        FixMatrix_SetRight(&pCar->right, &pRec->matrix);
        FixMatrix_SetUp(&pCar->up, &pRec->matrix);
        FixMatrix_SetForward(&pCar->forward, &pRec->matrix);

        if (pRec->resetPose != 0) {
            Car_InvalidateTransforms(pCar->index);
            pCar->field_0xbf8 = 1;
            pRec->resetPose = 0;
        }

        saved = pRec->velocity;
        pRec->velocity = pCar->velocity;
        v = pCar->field_0x5c4;
        FixVecScale(&v, &v, 0x3333);
        FixVecScale(&v, &v, g_physicsTimeStep);
        pRec->velocity.x += v.x;
        pRec->velocity.y += v.y;
        pRec->velocity.z += v.z;

        pRec->angularVelocity = pCar->angularVelocity;
        v.x = FixMul(pCar->field_0x5d0.x, -FixMul(pCar->inertia.x, pCar->mass));
        v.y = FixMul(-FixMul(pCar->mass, pCar->inertia.y), pCar->field_0x5d0.y);
        v.z = FixMul(pCar->field_0x5d0.z, -FixMul(pCar->inertia.z, pCar->mass));
        pRec->angularVelocity.x += v.x;
        pRec->angularVelocity.y += v.y;
        pRec->angularVelocity.z += v.z;

        pRec->useUpperCollisionCorners = pCar->useUpperCollisionCorners;
        pCar->field_0x960 = pRec->field_0xb4;
        NetRace_AdvanceRemoteCarAccumulator(pRec);

        dot = FixVecDot(&pCar->up, &pCar->groundNormal);
        pCar->field_0x96c -= FixMul(0x1eb8, g_physicsTimeStep);
        if (pCar->field_0x96c < 0)
            pCar->field_0x96c = 0;
        if (pCar->field_0x96c <= 0 && dot > 0xfc28) {
            ax = pCar->angularVelocity.x;
            if (ax < 0)
                ax = -ax;
            if (ax < 0x28f) {
                az = pCar->angularVelocity.z;
                if (az < 0)
                    az = -az;
                if (az < 0x28f) {
                    pCar->useUpperCollisionCorners = 0;
                    pCar->field_0x96c = 0;
                    pRec->useUpperCollisionCorners = 0;
                }
            }
        }

        if (saved.x == pRec->velocity.x &&
            saved.z == pRec->velocity.z) {
            pRec->moving = 0;
        } else {
            pRec->moving = 1;
            pRec->moveDir.x = pRec->velocity.x - saved.x;
            pRec->moveDir.y = pRec->velocity.y - saved.y;
            pRec->moveDir.z = pRec->velocity.z - saved.z;

            len = FixVecLength(&pRec->moveDir);
            if (len == 0) {
                pRec->moveDir.x = 0;
                pRec->moveDir.y = 0;
                pRec->moveDir.z = 0;
            } else {
                FixVecScaleRecip(&pRec->moveDir,
                                 &pRec->moveDir, len);
            }

            FixMatrix_GetPosition(&mpos, &pRec->matrix);
            d.x = pRec->position.x - mpos.x;
            d.y = pRec->position.y - mpos.y;
            d.z = pRec->position.z - mpos.z;
            FixVecScale(&scaled, &pRec->moveDir,
                        FixVecDot(&d, &pRec->moveDir));
            d.x -= scaled.x;
            d.y -= scaled.y;
            d.z -= scaled.z;
            pRec->position.x = d.x + mpos.x;
            pRec->position.y = d.y + mpos.y;
            pRec->position.z = d.z + mpos.z;
            FixMatrix_GetPosition(&pRec->contactPoint, &pRec->matrix);
        }
    }
}

// Moving stage objects: the table walked by the per-frame update below, one
// 0x128-byte record per object (defined in StageObjects.cpp, which owns the
// GLOBAL annotations of the table and of the active count).

extern MovingObjects g_movingObjects;

// 16.16 -> 12-bit angle of a turn rate (its negated twin is 0x511398 in Car.cpp);
// defined in StageTiming.cpp, which owns the GLOBAL annotation.
extern const double g_fixedRadiansToAngle12;

// The original converts the 16.16 value on the FPU (fild / fmul / fistp) and
// keeps the low word of the nearest-even result with the original FPU mode.
// MSVC6 /QIfist is required; a standard C++ cast alone would truncate instead.
#define FIX_ANGLE(v) ((short)FTOI(__int64, X87S_MUL((double)(v), g_fixedRadiansToAngle12)))

// The eight ground-probe vectors of an object: its reference axes at 0x104 (x),
// 0x108 (y) and 0x10c (z) with the sign pattern StageObject_ProbeGroundDistance selects with its
// octant index (y >= 0 -> +4, x < 0 -> +2, z < 0 -> +1).
#define FILL_PROBE_TABLE(pObj)                                                          \
    {                                                                                   \
        int probeX = (pObj)->probe.x;                                                   \
        int probeY = (pObj)->probe.y;                                                   \
        int probeZ = (pObj)->probe.z;                                                   \
                                                                                        \
        g_groundProbeOctantVectors[0].x = probeX;                                                  \
        g_groundProbeOctantVectors[0].y = -probeY;                                                 \
        g_groundProbeOctantVectors[0].z = probeZ;                                                  \
        g_groundProbeOctantVectors[1].x = probeX;                                                  \
        g_groundProbeOctantVectors[1].y = -probeY;                                                 \
        g_groundProbeOctantVectors[1].z = -probeZ;                                                 \
        g_groundProbeOctantVectors[2].x = -probeX;                                                 \
        g_groundProbeOctantVectors[2].y = -probeY;                                                 \
        g_groundProbeOctantVectors[2].z = probeZ;                                                  \
        g_groundProbeOctantVectors[3].x = -probeX;                                                 \
        g_groundProbeOctantVectors[3].y = -probeY;                                                 \
        g_groundProbeOctantVectors[3].z = -probeZ;                                                 \
        g_groundProbeOctantVectors[4].x = probeX;                                                  \
        g_groundProbeOctantVectors[4].y = probeY;                                                  \
        g_groundProbeOctantVectors[4].z = probeZ;                                                  \
        g_groundProbeOctantVectors[5].x = probeX;                                                  \
        g_groundProbeOctantVectors[5].y = probeY;                                                  \
        g_groundProbeOctantVectors[5].z = -probeZ;                                                 \
        g_groundProbeOctantVectors[6].x = -probeX;                                                 \
        g_groundProbeOctantVectors[6].y = probeY;                                                  \
        g_groundProbeOctantVectors[6].z = probeZ;                                                  \
        g_groundProbeOctantVectors[7].x = -probeX;                                                 \
        g_groundProbeOctantVectors[7].y = probeY;                                                  \
        g_groundProbeOctantVectors[7].z = -probeZ;                                                 \
    }

// Per-frame update of the moving stage objects. Each active object of
// g_movingObjects.entries (count in g_movingObjects.count) gets its result matrix refreshed
// from its source matrix and then:
//   mode 1 - the turn rate at 0xe0 is applied to the object basis, the basis is
//            re-orthonormalised and the object is dropped onto the ground;
//   mode 2 - gravity is integrated into the velocity at 0xec and the position,
//            the basis is reflected against the ground and the object matrix is
//            placed at the car-relative offset 0xf8;
// after which the ground-probe octant table is rebuilt and every node of the
// object's chain is flagged as moved. Any other mode only refreshes the matrix.
// match 62.2% (auditado W165): BUG CORREGIDO - el modo 2 leia 'world' sin inicializar; el original
// calcula world = RotateVector(pObj+0xf8) + GetPosition(pMatrix) al principio del bloque (se ve en el
// despacho: 'dec eax; jne' manda el modo 2 a esa secuencia). Al anadirlo la secuencia de llamadas
// coincide exactamente con la del original (27), pero su planificacion baja el % (63.3 -> 62.2): el
// resto de los diffs son reparto de registros/slots y el orden de los bloques.
// inlines (18 shrd / 96 shld / 8 idiv) y las mismas 6 conversiones fild/fmul/fistp.
// FUNCTION: CMR2 0x00470580
void StageObject_UpdateMovingTransforms(void)
{
    FixBasis basis;
    FixVector vecD4;
    FixVector vecE0;
    FixVector world;
    FixVector pos;
    FixVector step;
    FixVector grav;
    FixVector v;
    FixAngles angles;
    FixMatrix *pMatrix;
    MovingObject *pObj;
    SceneNode *pNode;
    int len;
    int len2;
    int dot;
    int height;
    int i;

    if (g_movingObjects.count == 0)
        return;

    pObj = g_movingObjects.entries;

    for (i = 0; i < (int)(g_movingObjects.count & 0xff); i++) {
        pMatrix = pObj->pMatrix;
        vecD4 = pObj->groundNormal;

        pObj->keyA = pObj->keyB;

        switch (pObj->mode) {
        case 1:
            // Turn rate (scaled first) becomes this step's rotation of the object basis.
            vecE0 = pObj->spin;
            FixVecScale(&vecE0, &vecE0, 0x11999);
            pObj->spin = vecE0;

            FixMatrix_GetRight(&basis.right, pMatrix);
            FixMatrix_GetUp(&basis.up, pMatrix);
            FixMatrix_GetForward(&basis.forward, pMatrix);

            angles.x = FIX_ANGLE(FixMul(g_physicsTimeStep, vecE0.x));
            angles.y = FIX_ANGLE(FixMul(g_physicsTimeStep, vecE0.y));
            angles.z = FIX_ANGLE(FixMul(g_physicsTimeStep, vecE0.z));
            FixBasis_Rotate(&basis, (unsigned short *)&angles);

            // Rebuilds an orthonormal basis out of up/right unless the object is
            // already upright (up . velocity rate > 0).
            dot = FixVecDot(&vecD4, &basis.up);
            if (dot <= 0) {
                FixVecScale(&pos, &vecD4, dot);
                pos.x = basis.up.x - pos.x;
                pos.y = basis.up.y - pos.y;
                pos.z = basis.up.z - pos.z;
                len = FixVecLength(&pos);
                if (len == 0) {
                    basis.up.x = 0;
                    basis.up.y = 0;
                    basis.up.z = 0;
                } else {
                    FixVecScaleRecip(&basis.up, &pos, len);
                }

                dot = FixVecDot(&basis.up, &basis.right);
                FixVecScale(&pos, &basis.up, dot);
                pos.x = basis.right.x - pos.x;
                pos.y = basis.right.y - pos.y;
                pos.z = basis.right.z - pos.z;
                len = FixVecLength(&pos);
                if (len == 0) {
                    basis.right.x = 0;
                    basis.right.y = 0;
                    basis.right.z = 0;
                } else {
                    FixVecScaleRecip(&basis.right, &pos, len);
                }

                FixVecCross(&pos, &basis.right, &basis.up);
                len = FixVecLength(&pos);
                if (len == 0) {
                    basis.forward.x = 0;
                    basis.forward.y = 0;
                    basis.forward.z = 0;
                } else {
                    FixVecScaleRecip(&basis.forward, &pos, len);
                }
                pObj->mode = 0;
            }

            FILL_PROBE_TABLE(pObj)
            FixMatrix_RotateVector(&world, &pObj->field_0xf8, pMatrix);
            FixMatrix_GetPosition(&pos, pMatrix);
            world.x += pos.x;
            world.y += pos.y;
            world.z += pos.z;

            pMatrix->position.y += StageObject_ProbeGroundDistance(pObj, &world);
            FixMatrix_SetRight(&basis.right, pMatrix);
            FixMatrix_SetUp(&basis.up, pMatrix);
            FixMatrix_SetForward(&basis.forward, pMatrix);

            for (pNode = pObj->pObject; pNode != NULL; pNode = pNode->pParent)
                pNode->dirty = 1;

            if (pObj->mode == 0)
                pObj->keyA = pObj->keyB;
            break;

        case 2:
            FixMatrix_GetRight(&basis.right, pMatrix);
            FixMatrix_GetUp(&basis.up, pMatrix);
            FixMatrix_GetForward(&basis.forward, pMatrix);
            FixMatrix_RotateVector(&world, &pObj->field_0xf8, pMatrix);
            FixMatrix_GetPosition(&pos, pMatrix);
            world.x += pos.x;
            world.y += pos.y;
            world.z += pos.z;

            FILL_PROBE_TABLE(pObj)
            vecE0 = pObj->spin;
            angles.x = FIX_ANGLE(FixMul(g_physicsTimeStep, vecE0.x));
            angles.y = FIX_ANGLE(FixMul(g_physicsTimeStep, vecE0.y));
            angles.z = FIX_ANGLE(FixMul(g_physicsTimeStep, vecE0.z));
            FixBasis_Rotate(&basis, (unsigned short *)&angles);

            // Gravity of the object (zeroed while its "on the ground" flag at
            // 0x120 is set) integrated into the velocity at 0xec, then one step
            // of it is applied to the position.
            grav.x = 0;
            grav.y = 0;
            grav.z = 0;
            if (pObj->field_0x120 == 0)
                grav.y = -0xa3d;
            if (CGameInfo::IsActiveCheatEnabled(1) != 0)
                FixVecScale(&grav, &grav, 0x8000);
            FixVecScale(&grav, &grav, g_physicsTimeStep);

            pObj->velocity.x += grav.x;
            pObj->velocity.y += grav.y;
            pObj->velocity.z += grav.z;

            FixVecScale(&step, &pObj->velocity, g_physicsTimeStep);
            FixVecScale(&grav, &grav, g_physicsTimeStep / 2);
            step.x -= grav.x;
            step.y -= grav.y;
            step.z -= grav.z;
            world.x += step.x;
            world.y += step.y;
            world.z += step.z;

            if (pObj->field_0x120 != 0) {
                // Airborne: the spin is damped twice and, while it is slow
                // enough, the basis is rebuilt from the fallen-over up axis.
                FixVecScale(&vecE0, &vecE0, 0xcccc);
                len = FixVecLength(&vecE0);
                FixVecScale(&vecE0, &vecE0, 0xcccc);
                len2 = FixVecLength(&vecE0);

                if (len < 0x28f && len2 < 0x28f) {
                    pObj->spin.x = 0;
                    pObj->spin.y = 0;
                    pObj->spin.z = 0;
                    pObj->velocity.x = 0;
                    pObj->velocity.y = 0;
                    pObj->velocity.z = 0;

                    if (FixVecDot(&basis.forward, &vecD4) >= 0) {
                        v = vecD4;
                    } else {
                        FixVecScaleRecip(&v, &vecD4, -0x10000);
                    }

                    pos.x = v.x - basis.forward.x;
                    pos.y = v.y - basis.forward.y;
                    pos.z = v.z - basis.forward.z;
                    len = FixVecLength(&pos);
                    if (len > 0x1999) {
                        FixVecScaleRecip(&pos, &pos, len);
                        FixVecScale(&pos, &pos, 0x1999);
                        pos.x += basis.forward.x;
                        pos.y += basis.forward.y;
                        pos.z += basis.forward.z;
                        len = FixVecLength(&pos);
                        if (len == 0) {
                            basis.forward.x = 0;
                            basis.forward.y = 0;
                            basis.forward.z = 0;
                        } else {
                            FixVecScaleRecip(&basis.forward, &pos, len);
                        }
                    } else {
                        basis.forward = v;
                        pObj->mode = 0;
                    }

                    dot = FixVecDot(&basis.right, &basis.forward);
                    FixVecScale(&pos, &basis.forward, dot);
                    pos.x = basis.right.x - pos.x;
                    pos.y = basis.right.y - pos.y;
                    pos.z = basis.right.z - pos.z;
                    len = FixVecLength(&pos);
                    if (len == 0) {
                        basis.right.x = 0;
                        basis.right.y = 0;
                        basis.right.z = 0;
                    } else {
                        FixVecScaleRecip(&basis.right, &pos, len);
                    }

                    FixVecCross(&pos, &basis.forward, &basis.right);
                    len = FixVecLength(&pos);
                    if (len == 0) {
                        basis.up.x = 0;
                        basis.up.y = 0;
                        basis.up.z = 0;
                    } else {
                        FixVecScaleRecip(&basis.up, &pos, len);
                    }
                }

                FixMatrix_SetRight(&basis.right, pMatrix);
                FixMatrix_SetUp(&basis.up, pMatrix);
                FixMatrix_SetForward(&basis.forward, pMatrix);
                world.y += StageObject_ProbeGroundDistance(pObj, &world);
            } else {
                FixMatrix_SetRight(&basis.right, pMatrix);
                FixMatrix_SetUp(&basis.up, pMatrix);
                FixMatrix_SetForward(&basis.forward, pMatrix);
                if (pObj->velocity.y < 0) {
                    height = StageObject_ProbeGroundDistance(pObj, &world);
                    if (height > -0xccc) {
                        pObj->mode = 1;
                        world.y += height;
                    }
                }
            }

            FixMatrix_RotateVector(&pos, &pObj->field_0xf8, pMatrix);
            pos.x = world.x - pos.x;
            pos.y = world.y - pos.y;
            pos.z = world.z - pos.z;
            FixMatrix_SetPosition(&pos, pMatrix);

            for (pNode = pObj->pObject; pNode != NULL; pNode = pNode->pParent)
                pNode->dirty = 1;

            if (pObj->mode == 0)
                pObj->keyA = pObj->keyB;
            break;
        }

        pObj++;
    }
}
