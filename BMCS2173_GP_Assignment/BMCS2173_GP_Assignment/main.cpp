
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

using std::max;

#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "GLU32.lib")
#pragma warning(disable:4996)

#define WINDOW_TITLE "Main Character"
#include "head_geometry_include.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float PI = 3.1415926535f;
const float TWO_PI = 6.283185307f;

struct Vec3 { float x, y, z; };
struct Vec2 { float u, v; };

struct PanelRing {
    float offsetOut;
    float offsetZ;
    float widthMul;
    float bulge;
};

struct HairPanel {
    float thetaStart, thetaEnd;
    float phiStart;
    float phiConform;
    PanelRing rings[10];
    int numRings;
    int widthSegs;
    int lengthSegs;
    float colorTint;
    float colorShade;
    int bright;
    float thetaSweep;
    HairPanel() { thetaSweep = 0.0f; }
};

//================================
// Camera variables
//================================

float cameraAngle = 0.0f;
float cameraHeight = 3.5f;
float cameraDistance = 8.0f;
float cameraX, cameraY, cameraZ;
float cameraViewMatrix[16];

//================================
// Character joint / animation variables
//================================

float leftArmAngle  = 0.0f;
float rightArmAngle = 0.0f;
float leftLegAngle  = 0.0f;
float rightLegAngle = 0.0f;

bool  attackAnimation = false;
float attackAngle     = 0.0f;

bool leftFingerActive[5] = {false, false, false, false, false};
float leftFingerProgress[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

bool rightFingerActive[5] = {false, false, false, false, false};
float rightFingerProgress[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

bool leftHandRotated = false;
float leftHandRotAngle = 0.0f;

bool rightHandRotated = false;
float rightHandRotAngle = 0.0f;

// Movement and key state
float charX = 0.0f;
float charZ = 0.0f;
float charRotation = 0.0f;
float walkPhase = 0.0f;
float walkArmSwing = 0.0f;
bool keys[256];

bool weapon1_status = false; // Meteor Hammer equipped
bool weapon2_status = false; // Fan equipped
float fanSpreadAngle  = 0.0f;  // start folded
float fanTargetAngle  = 0.0f;

// K animation variables
bool  kAnimationActive = false;
float kAnimProgress    = 0.0f;
float kAnimDistance    = 0.0f;
float kAnimSpin        = 0.0f;
bool  preK_weapon1_status = false;
bool  preK_weapon2_status = false;
bool  preK_leftFingerActive[5] = {false, false, false, false, false};
float preK_leftFingerProgress[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float preK_fanTargetAngle = 0.0f;

// J animation variables
bool  jAnimationActive = false;
float jAnimProgress    = 0.0f;
float jAnimChainExtend = 0.0f;
float jAnimHammerScale = 1.0f;
float screenShakeTimer = 0.0f;
float preJ_leftArmAngle = 0.0f;
bool  preJ_leftFingerActive[5] = {false, false, false, false, false};
float preJ_leftFingerProgress[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

bool isNight = false;
bool isShadowPass = false;
bool isWireframe = false;

float lightX = 12.0f;
float lightY = 18.0f;
float lightZ = -30.0f;

DWORD lastTime = 0;

//================================
// Textures
//================================
GLuint texGrass;        // background ground
GLuint texSkin;         // body / arm skin
GLuint texFabric;       // dark-blue fabric
GLuint texGold;         // gold trim
GLuint texWhiteSleeve;  // arm white sleeve
GLuint texWhiteLeather; // boot white leather
GLuint texDarkLeather;  // boot sole

GLuint texWeaponMetal;  // weapon metal
GLuint texWeaponWood;   // weapon wood
GLuint texWeaponChain;  // weapon chain

GLuint texFanWood;      // fan wood
GLuint texFan;          // fan fabric
GLuint texHeadObj;      // Loaded for head8 head model

// Body uses a display-list cache to avoid repeating heavy procedural math
GLuint cachedBodyList = 0;

//================================
// Utility: Math and Head/Hair logic (from head8.cpp)
//================================

Vec3 v3(float x, float y, float z) { Vec3 v = { x, y, z }; return v; }
Vec3 v3add(Vec3 a, Vec3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 v3sub(Vec3 a, Vec3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
Vec3 v3scale(Vec3 v, float s) { return v3(v.x * s, v.y * s, v.z * s); }
Vec3 v3cross(Vec3 a, Vec3 b) { return v3(a.y* b.z - a.z * b.y, a.z* b.x - a.x * b.z, a.x* b.y - a.y * b.x); }
float v3dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float v3len(Vec3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
Vec3 v3norm(Vec3 v) {
    float l = v3len(v);
    if (l < 0.00001f) return v3(0, 1, 0);
    return v3(v.x / l, v.y / l, v.z / l);
}
Vec3 v3lerp(Vec3 a, Vec3 b, float t) { return v3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t); }

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float smoothstep(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }

float catRom(float t, float p0, float p1, float p2, float p3) {
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 + (-p0 + 3 * p1 - 3 * p2 + p3) * t3);
}

Vec3 catRomV(float t, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3) {
    return v3(catRom(t, p0.x, p1.x, p2.x, p3.x), catRom(t, p0.y, p1.y, p2.y, p3.y), catRom(t, p0.z, p1.z, p2.z, p3.z));
}

float hf(int s) {
    s = (s ^ 61) ^ (s >> 16); s += (s << 3); s ^= (s >> 4);
    s *= 0x27d4eb2d; s ^= (s >> 15);
    return (float)(s & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}
float rr(int s, float lo, float hi) { return lo + hf(s) * (hi - lo); }

// ----------------------------------------------------------------
// Lighting Configs for Head/Hair (from head8.cpp)
// ----------------------------------------------------------------
void setupLightingHead() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2); // Rim light for high fidelity
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat ambientLight[] = { 0.20f, 0.20f, 0.20f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

    GLfloat lightPos0[] = { 4.0f, 5.0f, 5.0f, 1.0f };
    GLfloat white[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, black);

    GLfloat lightPos1[] = { -4.0f, 2.0f, -1.0f, 1.0f };
    GLfloat lightDiff1[] = { 0.3f, 0.3f, 0.4f, 1.0f }; // Soft cool fill
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiff1);
    
    GLfloat lightPos2[] = { 0.0f, 5.0f, -5.0f, 1.0f }; // Rim light from behind
    GLfloat lightDiff2[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, lightPos2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, lightDiff2);
    
    GLfloat specColor[] = { 0.15f, 0.15f, 0.15f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specColor);
    glMateriali(GL_FRONT, GL_SHININESS, 16);
}

void setupLightingHair() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat amb[] = { 0.30f, 0.30f, 0.38f, 1 };
    GLfloat kp[] = { 2.5f, 4, 5.5f, 1 };
    GLfloat kd[] = { 0.85f, 0.85f, 0.92f, 1 };
    GLfloat ks[] = { 1, 1, 1, 1 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_POSITION, kp);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, kd);
    glLightfv(GL_LIGHT0, GL_SPECULAR, ks);

    GLfloat fp[] = { -3, 1.5f, -2, 1 };
    GLfloat fd[] = { 0.18f, 0.20f, 0.32f, 1 };
    glLightfv(GL_LIGHT1, GL_POSITION, fp);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fd);

    GLfloat rp[] = { 0, 3, -4, 1 };
    GLfloat rd[] = { 0.22f, 0.28f, 0.42f, 1 };
    GLfloat rs[] = { 0.5f, 0.6f, 0.85f, 1 };
    glLightfv(GL_LIGHT2, GL_POSITION, rp);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, rd);
    glLightfv(GL_LIGHT2, GL_SPECULAR, rs);

    GLfloat ms[] = { 0.55f, 0.65f, 0.90f, 1 };
    GLfloat msh[] = { 60 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ms);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, msh);
}

// ----------------------------------------------------------------
// Hair Model logic (from head8.cpp)
// ----------------------------------------------------------------
const float SR_X = 0.48f;
const float SR_Y = 0.50f;
const float SR_Z = 0.48f;
const float SC_Z = 0.38f;

Vec3 skullPt(float theta, float phi) {
    return v3(SR_X * sinf(phi) * cosf(theta),
        SR_Y * sinf(phi) * sinf(theta),
        SC_Z + SR_Z * cosf(phi));
}

Vec3 skullNrm(float theta, float phi) {
    Vec3 p = skullPt(theta, phi);
    return v3norm(v3(p.x / (SR_X * SR_X), p.y / (SR_Y * SR_Y), (p.z - SC_Z) / (SR_Z * SR_Z)));
}

void emitQ(Vec3 p0, Vec3 n0, Vec3 p1, Vec3 n1, Vec3 p2, Vec3 n2, Vec3 p3, Vec3 n3) {
    glBegin(GL_QUADS);
    glNormal3f(n0.x, n0.y, n0.z); glVertex3f(p0.x, p0.y, p0.z);
    glNormal3f(n1.x, n1.y, n1.z); glVertex3f(p1.x, p1.y, p1.z);
    glNormal3f(n2.x, n2.y, n2.z); glVertex3f(p2.x, p2.y, p2.z);
    glNormal3f(n3.x, n3.y, n3.z); glVertex3f(p3.x, p3.y, p3.z);
    glEnd();
}

void setHairColorGradient(float shade, float tint, float v, int bright) {
    float rTop = 0.05f, gTop = 0.12f, bTop = 0.55f;
    float rBot = 0.35f, gBot = 0.08f, bBot = 0.50f;
    if (bright) {
        rTop *= 1.30f; gTop *= 1.30f; bTop *= 1.30f;
        rBot *= 1.30f; gBot *= 1.30f; bBot *= 1.30f;
    }
    float r = rTop + (rBot - rTop) * v;
    float g = gTop + (gBot - gTop) * v;
    float b = bTop + (bBot - bTop) * v;
    r += tint * 0.05f; g += tint * 0.05f; b += tint * 0.05f;
    glColor3f(r * shade, g * shade, b * shade);
}

void drawPanel(const HairPanel& hp) {
    int wSegs = hp.widthSegs;
    int lSegs = hp.lengthSegs;
    int cols = wSegs + 1;
    int rows = lSegs + 1;
    Vec3* pts = new Vec3[rows * cols];
    Vec3* nrm = new Vec3[rows * cols];
    for (int r = 0; r < rows; ++r) {
        float v = (float)r / (float)lSegs;
        float globalR = v * (float)(hp.numRings - 1);
        int ri = (int)globalR;
        if (ri >= hp.numRings - 1) ri = hp.numRings - 2;
        float rt = globalR - (float)ri;
        int r0 = (ri - 1 < 0) ? 0 : ri - 1;
        int r1 = ri;
        int r2 = (ri + 1 >= hp.numRings) ? hp.numRings - 1 : ri + 1;
        int r3 = (ri + 2 >= hp.numRings) ? hp.numRings - 1 : ri + 2;
        float oOut = catRom(rt, hp.rings[r0].offsetOut, hp.rings[r1].offsetOut, hp.rings[r2].offsetOut, hp.rings[r3].offsetOut);
        float oZ = catRom(rt, hp.rings[r0].offsetZ, hp.rings[r1].offsetZ, hp.rings[r2].offsetZ, hp.rings[r3].offsetZ);
        float wMul = catRom(rt, hp.rings[r0].widthMul, hp.rings[r1].widthMul, hp.rings[r2].widthMul, hp.rings[r3].widthMul);
        float bulge = catRom(rt, hp.rings[r0].bulge, hp.rings[r1].bulge, hp.rings[r2].bulge, hp.rings[r3].bulge);
        for (int c = 0; c < cols; ++c) {
            float u = (float)c / (float)wSegs;
            float theta = hp.thetaStart + (hp.thetaEnd - hp.thetaStart) * u;
            float centerTheta = (hp.thetaStart + hp.thetaEnd) * 0.5f;
            theta = centerTheta + (theta - centerTheta) * wMul;
            theta += hp.thetaSweep * v * v;
            float phi = hp.phiStart + hp.phiConform * clampf(v * 1.5f, 0, 1);
            Vec3 sp = skullPt(theta, phi);
            Vec3 sn = skullNrm(theta, phi);
            float edgeFade = 1.0f - 2.0f * fabsf(u - 0.5f);
            float bulgeFactor = bulge * edgeFade * edgeFade;
            Vec3 p = v3add(sp, v3scale(sn, oOut + bulgeFactor));
            p.z += oZ;
            pts[r * cols + c] = p;
        }
    }
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = r * cols + c;
            Vec3 center = pts[idx];
            Vec3 du, dv;
            if (c < cols - 1) du = v3sub(pts[idx + 1], center);
            else du = v3sub(center, pts[idx - 1]);
            if (r < rows - 1) dv = v3sub(pts[(r + 1) * cols + c], center);
            else dv = v3sub(center, pts[(r - 1) * cols + c]);
            nrm[idx] = v3norm(v3cross(du, dv));
        }
    }
    for (int r = 0; r < lSegs; ++r) {
        float v = (float)r / (float)lSegs;
        float shade = hp.colorShade * (1.0f - 0.30f * v);
        setHairColorGradient(shade, hp.colorTint, v, hp.bright);
        for (int c = 0; c < wSegs; ++c) {
            int i00 = r * cols + c;
            int i10 = r * cols + c + 1;
            int i01 = (r + 1) * cols + c;
            int i11 = (r + 1) * cols + c + 1;
            emitQ(pts[i00], nrm[i00], pts[i10], nrm[i10], pts[i11], nrm[i11], pts[i01], nrm[i01]);
            Vec3 fn00 = v3scale(nrm[i00], -1); Vec3 fn10 = v3scale(nrm[i10], -1);
            Vec3 fn11 = v3scale(nrm[i11], -1); Vec3 fn01 = v3scale(nrm[i01], -1);
            float bshade = shade * 0.40f;
            setHairColorGradient(bshade, hp.colorTint, v, hp.bright);
            emitQ(pts[i01], fn01, pts[i11], fn11, pts[i10], fn10, pts[i00], fn00);
            setHairColorGradient(shade, hp.colorTint, v, hp.bright);
        }
    }
    delete[] pts; delete[] nrm;
}

void drawBackHair() {
    const int numPanels = 10;
    for (int i = 0; i < numPanels; ++i) {
        float centerAngle = PI * 1.5f + PI * 0.85f * ((float)i / (numPanels - 1) - 0.5f);
        float halfWidth = PI / (float)numPanels * 1.5f;
        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth; hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.12f + rr(i * 31, 0, 0.03f);
        hp.phiConform = PI * 0.40f + rr(i * 37, -0.05f, 0.05f);
        hp.widthSegs = 4; hp.lengthSegs = 10;
        hp.colorTint = rr(i * 41, 0.0f, 0.8f);
        hp.colorShade = 0.85f + rr(i * 43, 0, 0.15f);
        hp.bright = 0;
        float depthVar = rr(i * 47, 0, 0.02f);
        float totalDrop = 1.4f + rr(i * 53, -0.15f, 0.20f);
        hp.numRings = 8;
        hp.rings[0] = { 0.04f + depthVar, 0.0f,    1.0f,  0.02f };
        hp.rings[1] = { 0.05f + depthVar, -0.05f,   1.0f,  0.04f };
        hp.rings[2] = { 0.08f + depthVar, -0.18f,   0.95f, 0.05f };
        hp.rings[3] = { 0.05f + depthVar, -0.38f,   0.88f, 0.04f };
        hp.rings[4] = { 0.12f + depthVar, -0.60f,   0.78f, 0.03f };
        hp.rings[5] = { 0.06f + depthVar, -0.85f,   0.65f, 0.02f };
        hp.rings[6] = { 0.12f + depthVar, -totalDrop * 0.88f, 0.40f, 0.01f };
        hp.rings[7] = { 0.02f + depthVar, -totalDrop + 0.08f, 0.20f, 0.00f };
        drawPanel(hp);
    }
}

void drawSideHair() {
    for (int side = 0; side < 2; ++side) {
        const int numPanels = 8;
        for (int i = 0; i < numPanels; ++i) {
            float baseAngle = (side == 0) ? (PI * 0.0f) : (PI * 1.0f);
            float centerAngle = baseAngle + PI * 0.40f * ((float)i / (numPanels - 1) - 0.5f);
            float halfWidth = PI / (float)(numPanels) * 0.8f;
            HairPanel hp;
            hp.thetaStart = centerAngle - halfWidth; hp.thetaEnd = centerAngle + halfWidth;
            hp.phiStart = PI * 0.04f + rr(i * 59 + side * 500, 0, 0.04f);
            hp.phiConform = PI * 0.38f + rr(i * 61 + side * 500, -0.04f, 0.04f);
            hp.widthSegs = 5; hp.lengthSegs = 14;
            hp.colorTint = rr(i * 67 + side * 500, 0.1f, 0.9f);
            hp.colorShade = 0.90f + rr(i * 71 + side * 500, 0, 0.10f);
            hp.bright = 0;
            float dv = rr(i * 73 + side * 500, 0, 0.02f);
            float totalDrop = 1.25f + rr(i * 79 + side * 500, -0.15f, 0.25f);
            hp.numRings = 7;
            hp.rings[0] = { 0.04f + dv, 0.0f,   1.0f,  0.03f }; hp.rings[1] = { 0.06f + dv, -0.06f,  1.0f,  0.05f };
            hp.rings[2] = { 0.10f + dv, -0.22f,  0.92f, 0.04f }; hp.rings[3] = { 0.05f + dv, -0.45f,  0.82f, 0.03f };
            hp.rings[4] = { 0.12f + dv, -0.72f,  0.68f, 0.02f }; hp.rings[5] = { 0.05f + dv, -totalDrop * 0.85f, 0.42f, 0.01f };
            hp.rings[6] = { 0.10f + dv, -totalDrop,         0.18f, 0.00f };
            drawPanel(hp);
        }
    }
}

void drawBangs() {
    const int numBangs = 16;
    for (int i = 0; i < numBangs; ++i) {
        float t = (float)i / (numBangs - 1);
        float centerAngle = PI * 0.5f + PI * 0.22f * (t - 0.5f);
        float halfWidth = PI * 0.035f; 
        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth; hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.02f; hp.phiConform = PI * 0.22f;
        hp.widthSegs = 4; hp.lengthSegs = 10;
        hp.colorTint = 0.2f + 0.1f * sinf(t * PI * 6.0f);
        hp.colorShade = 0.88f + 0.04f * sinf(t * PI * 4.0f);
        hp.bright = 0;
        float midDist = fabsf(t - 0.5f);
        float bangLen = (midDist < 0.2f) ? (0.30f + 0.6f * midDist) : (0.42f - 0.20f * (midDist - 0.2f));
        hp.thetaSweep = (t - 0.5f) * 1.5f;
        hp.numRings = 5;
        hp.rings[0] = { 0.05f, 0.0f,   1.0f,  0.02f }; hp.rings[1] = { 0.07f, -0.04f,  1.0f,  0.03f };
        hp.rings[2] = { 0.10f, -bangLen * 0.4f,  0.90f, 0.02f }; hp.rings[3] = { 0.08f, -bangLen * 0.8f,  0.60f, 0.01f };
        hp.rings[4] = { 0.03f, -bangLen,          0.15f, 0.00f }; 
        drawPanel(hp);
    }
}

void drawTopHair() {
    const int numPanels = 8;
    for (int i = 0; i < numPanels; ++i) {
        float centerAngle = TWO_PI * ((float)i / numPanels);
        float halfWidth = PI / (float)numPanels * 1.6f;
        if (centerAngle > PI * 0.3f && centerAngle < PI * 0.7f) continue;
        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth; hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.01f; hp.phiConform = PI * 0.22f;
        hp.widthSegs = 4; hp.lengthSegs = 6;
        hp.colorTint = rr(i * 137, 0.3f, 0.7f);
        hp.colorShade = 0.90f + rr(i * 139, 0, 0.10f);
        hp.bright = 0;
        float topLen = 0.25f + rr(i * 141, -0.10f, 0.15f);
        hp.numRings = 4;
        hp.rings[0] = { 0.04f, 0.02f,  1.0f,  0.01f }; hp.rings[1] = { 0.05f, 0.0f,   1.0f,  0.03f };
        hp.rings[2] = { 0.06f, -topLen * 0.35f,  0.95f, 0.03f }; hp.rings[3] = { 0.05f, -topLen,          0.70f, 0.00f };
        drawPanel(hp);
    }
}

void drawAccentStrands() {
    const int numStrands = 12;
    for (int i = 0; i < numStrands; ++i) {
        float angle = PI * 1.0f + PI * 1.0f * hf(i * 149);
        float halfWidth = PI * 0.03f + rr(i * 151, 0, 0.015f);
        HairPanel hp;
        hp.thetaStart = angle - halfWidth; hp.thetaEnd = angle + halfWidth;
        hp.phiStart = PI * 0.04f + rr(i * 157, 0, 0.04f);
        hp.phiConform = PI * 0.35f + rr(i * 163, -0.04f, 0.04f);
        hp.widthSegs = 2; hp.lengthSegs = 8;
        hp.colorTint = rr(i * 167, 0, 1);
        hp.colorShade = 0.80f + rr(i * 173, 0, 0.20f);
        hp.bright = (hf(i * 179) > 0.6f) ? 1 : 0;
        float strandLen = 0.8f + rr(i * 181, -0.2f, 0.5f);
        float dv = rr(i * 191, 0, 0.02f);
        hp.numRings = 5;
        hp.rings[0] = { 0.055f + dv, 0.0f,   1.0f,  0.01f }; hp.rings[1] = { 0.065f + dv, -0.06f,  1.0f,  0.02f };
        hp.rings[2] = { 0.075f + dv, -0.30f,  0.75f, 0.01f }; hp.rings[3] = { 0.06f + dv,  -strandLen * 0.8f, 0.40f, 0.00f };
        hp.rings[4] = { 0.04f + dv,  -strandLen + 0.06f, 0.15f, 0.00f };
        drawPanel(hp);
    }
}

void drawLayerStrands() {
    const int numPanels = 10;
    for (int i = 0; i < numPanels; ++i) {
        float angle = PI * 1.0f + PI * 1.0f * ((float)i / (numPanels - 1));
        float halfWidth = PI * 0.04f + rr(i * 233, 0, 0.02f);
        HairPanel hp;
        hp.thetaStart = angle - halfWidth; hp.thetaEnd = angle + halfWidth;
        hp.phiStart = PI * 0.06f + rr(i * 239, 0, 0.04f);
        hp.phiConform = PI * 0.36f + rr(i * 241, -0.04f, 0.04f);
        hp.widthSegs = 2; hp.lengthSegs = 8;
        hp.colorTint = rr(i * 251, 0, 1);
        hp.colorShade = 0.82f + rr(i * 257, 0, 0.18f);
        hp.bright = (hf(i * 263) > 0.5f) ? 1 : 0;
        float totalDrop = 1.1f + rr(i * 269, -0.2f, 0.3f);
        float dv = rr(i * 271, 0, 0.015f);
        hp.numRings = 6;
        hp.rings[0] = { 0.05f + dv, 0.0f,   1.0f,  0.015f }; hp.rings[1] = { 0.06f + dv, -0.05f,  1.0f,  0.025f };
        hp.rings[2] = { 0.07f + dv, -0.22f,  0.85f, 0.02f }; hp.rings[3] = { 0.08f + dv, -0.50f,  0.65f, 0.01f };
        hp.rings[4] = { 0.06f + dv, -totalDrop * 0.88f, 0.35f, 0.005f }; hp.rings[5] = { 0.04f + dv, -totalDrop + 0.06f,  0.15f, 0.00f };
        drawPanel(hp);
    }
}

void drawRabbitEars() {
    glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.14f, 0.04f, 0.88f);
        glRotatef(side * 18.0f, 0, 1, 0); glRotatef(-8.0f, 1, 0, 0);
        const int stacks = 50; const int slices = 36; float height = 0.70f;
        for (int i = 0; i < stacks; i++) {
            float f1 = (float)i / stacks; float f2 = (float)(i + 1) / stacks;
            float z1 = f1 * height; float z2 = f2 * height;
            float peakPos = 0.60f; float maxWidth = 0.15f;
            float earWidth1 = (f1 < peakPos) ? (maxWidth * (0.35f + 0.65f * powf(f1 / peakPos, 0.7f))) : (maxWidth * (0.30f + 0.70f * cosf((f1 - peakPos) / (1.0f - peakPos) * 3.14159f * 0.5f)));
            float earWidth2 = (f2 < peakPos) ? (maxWidth * (0.35f + 0.65f * powf(f2 / peakPos, 0.7f))) : (maxWidth * (0.30f + 0.70f * cosf((f2 - peakPos) / (1.0f - peakPos) * 3.14159f * 0.5f)));
            float curve1 = 0.12f * f1 * f1; float curve2 = 0.12f * f2 * f2;
            float thick1 = 0.035f * sinf(f1 * 3.14159f * 0.85f + 0.15f); float thick2 = 0.035f * sinf(f2 * 3.14159f * 0.85f + 0.15f);
            if (i == stacks - 1) {
                glBegin(GL_TRIANGLE_FAN); glColor3f(1.0f, 1.0f, 1.0f); glNormal3f(0, 0, 1); glVertex3f(0, -curve2 - 0.005f, z2 + earWidth2 * 0.4f);
                for (int j = 0; j <= slices; j++) {
                    float th = (float)j / slices * TWO_PI;
                    glNormal3f(cosf(th) * 0.3f, sinf(th) * 0.3f, 0.9f); glVertex3f(earWidth1 * cosf(th), thick1 * sinf(th) - curve1, z1);
                }
                glEnd();
            } else {
                glBegin(GL_QUAD_STRIP);
                for (int j = 0; j <= slices; j++) {
                    float th = (float)j / slices * TWO_PI; float cosTh = cosf(th); float sinTh = sinf(th);
                    float faceFactor = max(0.0f, sinTh - 0.50f) / 0.50f;
                    float hFact = clampf(f1 * 4.0f, 0, 1) * clampf((1.0f - f1) * 3.0f, 0, 1);
                    float pinkIntensity = faceFactor * hFact;
                    glColor3f(1.0f, 1.0f - 0.30f * pinkIntensity, 1.0f - 0.22f * pinkIntensity);
                    float depthMul = 1.0f - 0.55f * pinkIntensity;
                    glNormal3f(cosTh, sinTh * (1.0f - 0.7f * pinkIntensity), 0.15f);
                    glVertex3f(earWidth2 * cosTh, thick2 * depthMul * sinTh - curve2, z2);
                    glVertex3f(earWidth1 * cosTh, thick1 * depthMul * sinTh - curve1, z1);
                }
                glEnd();
            }
        }
        glPopMatrix();
    }
    glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D);
}

void drawEarrings() {
    glDisable(GL_TEXTURE_2D);
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.37f, -0.02f, 0.15f);
        int slides = 12; float rTop = 0.015f, rBot = 0.035f; float zTop = 0.0f, zBot = -0.10f;
        glColor3f(0.9f, 0.75f, 0.2f);
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= slides; i++) {
            float th = (float)i / slides * TWO_PI; float ct = cosf(th), st = sinf(th);
            glNormal3f(ct, st, 0.2f); glVertex3f(rBot * ct, rBot * st, zBot); glVertex3f(rTop * ct, rTop * st, zTop);
        }
        glEnd();
        glColor3f(0.95f, 0.95f, 0.95f);
        float tTop = zBot, tBot = -0.18f; float rtTop = 0.025f, rtBot = 0.005f;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= slides; i++) {
            float th = (float)i / slides * TWO_PI; float ct = cosf(th), st = sinf(th);
            glNormal3f(ct, st, 0.1f); glVertex3f(rtBot * ct, rtBot * st, tBot); glVertex3f(rtTop * ct, rtTop * st, tTop);
        }
        glEnd();
        glPopMatrix();
    }
}

void drawHeadpiece() {
    glDisable(GL_CULL_FACE); glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    glTranslatef(0.0f, 0.42f, 0.81f); glRotatef(35.0f, 1.0f, 0.0f, 0.0f);
    auto drawGem = [](float scaleX, float scaleY, float r, float g, float b, bool pointDown=false) {
        glColor3f(r, g, b); glBegin(GL_TRIANGLE_FAN); glNormal3f(0, 1, 0); glVertex3f(0, 0.03f, 0);
        int pts = 16;
        for (int i = 0; i <= pts; i++) {
            float th = (float)i / pts * TWO_PI; float st = sinf(th), ct = cosf(th);
            if (pointDown) { if (st < 0) st *= 1.5f; } else { if (st > 0) st *= 1.5f; }
            float nx = ct, ny = 0.5f, nz = st; float len = sqrtf(nx*nx + ny*ny + nz*nz);
            glNormal3f(nx/len, ny/len, nz/len); glVertex3f(ct * scaleX, 0, st * scaleY);
        }
        glEnd();
    };
    drawGem(0.05f, 0.07f, 0.85f, 0.70f, 0.15f, true);
    glTranslatef(0.0f, 0.005f, 0.0f); drawGem(0.03f, 0.05f, 0.1f, 0.7f, 1.0f, true);
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix(); glTranslatef(side * 0.08f, 0.0f, 0.04f); glRotatef(side * -45.0f, 0.0f, 0.0f, 1.0f);
        drawGem(0.06f, 0.13f, 0.85f, 0.70f, 0.15f, false);
        glTranslatef(0.0f, 0.005f, 0.0f); drawGem(0.04f, 0.10f, 0.1f, 0.7f, 1.0f, false);
        glPopMatrix();
    }
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix(); glTranslatef(side * 0.18f, -0.015f, 0.02f); glRotatef(side * -20.0f, 0.0f, 0.0f, 1.0f); 
        drawGem(0.035f, 0.05f, 0.85f, 0.70f, 0.15f, false);
        glTranslatef(0.0f, 0.005f, 0.0f); drawGem(0.02f, 0.035f, 0.1f, 0.7f, 1.0f, false);
        glPopMatrix();
    }
    glPopMatrix();
    glColor3f(0.85f, 0.70f, 0.15f); glPushMatrix(); glTranslatef(0.0f, 0.45f, 0.76f); glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    glBegin(GL_QUAD_STRIP); int bandSegs = 20;
    for (int i = 0; i <= bandSegs; i++) {
        float t = (float)i / bandSegs; float a = t * PI;
        float x = -0.28f * cosf(a), y = -0.10f * sinf(a), z = -0.08f * sinf(a);
        float thick = 0.025f * sinf(a); float centerDist = fabsf(t - 0.5f);
        if(centerDist < 0.10f) z -= 0.04f * (0.10f - centerDist) / 0.10f; 
        glNormal3f(0, 1, 0.3f); glVertex3f(x, y, z - 0.015f); glVertex3f(x, y, z + thick + 0.015f);
    }
    glEnd(); glPopMatrix();
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix(); glTranslatef(side * 0.20f, 0.39f, 0.70f);
        int segments = 16;
        for (int i = 0; i < segments; i++) {
            float t1 = (float)i / segments; float t2 = (float)(i+1) / segments;
            float h1X = (side * 1.0f) * (0.16f * t1 + 0.03f * t1 * t1), h1Y = -0.05f * t1 - 0.1f * t1 * t1, h1Z = 0.08f * t1 + 0.12f * t1 * t1;
            float r1 = 0.02f * (1.1f - t1) * (1.0f - 0.2f*t1);
            float h2X = (side * 1.0f) * (0.16f * t2 + 0.03f * t2 * t2), h2Y = -0.05f * t2 - 0.1f * t2 * t2, h2Z = 0.08f * t2 + 0.12f * t2 * t2;
            float r2 = 0.02f * (1.1f - t2) * (1.0f - 0.2f*t2);
            glBegin(GL_QUAD_STRIP);
            for (int k = 0; k <= 6; k++) { 
                float a = (float)k / 6.0f * TWO_PI; float ct = cosf(a), st = sinf(a);
                glNormal3f(ct, 0.3f, st); glVertex3f(h2X + ct * r2, h2Y, h2Z + st * r2); glVertex3f(h1X + ct * r1, h1Y, h1Z + st * r1);
            }
            glEnd();
        }
        for (int i = 0; i < segments; i++) {
            float t1 = (float)i / segments; float t2 = (float)(i+1) / segments;
            float h1X = (side * 1.0f) * (0.12f * t1 + 0.05f * t1 * t1), h1Y = -0.05f * t1, h1Z = -0.05f * t1 - 0.05f * t1 * t1;
            float r1 = 0.015f * (1.1f - t1);
            float h2X = (side * 1.0f) * (0.12f * t2 + 0.05f * t2 * t2), h2Y = -0.05f * t2, h2Z = -0.05f * t2 - 0.05f * t2 * t2;
            float r2 = 0.015f * (1.1f - t2);
            glBegin(GL_QUAD_STRIP);
            for (int k = 0; k <= 6; k++) {
                float a = (float)k / 6.0f * TWO_PI; float ct = cosf(a), st = sinf(a);
                glNormal3f(ct, 0.3f, st); glVertex3f(h2X + ct * r2, h2Y, h2Z + st * r2); glVertex3f(h1X + ct * r1, h1Y, h1Z + st * r1);
            }
            glEnd();
        }
        glPopMatrix();
    }
}

void drawHairModel() {
    glDisable(GL_CULL_FACE);
    drawRabbitEars(); drawHeadpiece(); drawEarrings();
    drawTopHair(); drawBackHair(); drawSideHair(); drawLayerStrands(); drawBangs(); drawAccentStrands();
}

//================================
// Utility: load a BMP texture
//================================

GLuint loadBMP(const char* filename)
{
    GLuint texture = 0;
    FILE* file = fopen(filename, "rb");
    if (!file) { printf("Failed to open %s\n", filename); return 0; }

    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) { fclose(file); return 0; }
    if (header[0] != 'B' || header[1] != 'M') { fclose(file); return 0; }

    int width     = *(int*)&(header[18]);
    int height    = *(int*)&(header[22]);
    int imageSize = *(int*)&(header[34]);
    if (imageSize == 0) imageSize = width * height * 3;

    unsigned char* data = new unsigned char[imageSize];
    fread(data, 1, imageSize, file);
    fclose(file);

    // BMP stores BGR; flip to RGB while respecting 4-byte row padding
    int rowSize = ((width * 3) + 3) & ~3;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = y * rowSize + x * 3;
            unsigned char tmp = data[i];
            data[i]     = data[i + 2];
            data[i + 2] = tmp;
        }
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return texture;
}

//================================
// Camera helpers
//================================

void updateCamera()
{
    cameraX = sin(cameraAngle) * cameraDistance;
    cameraZ = cos(cameraAngle) * cameraDistance;
    cameraY = cameraHeight;
}

void resetAll()
{
    cameraAngle    = 0.0f;
    cameraHeight   = 3.5f;
    cameraDistance = 8.0f;
    cameraX = 0.0f; cameraY = 3.5f; cameraZ = 8.0f;

    leftArmAngle  = 0.0f; rightArmAngle = 0.0f;
    leftLegAngle  = 0.0f; rightLegAngle = 0.0f;

    attackAnimation = false; attackAngle = 0.0f;
    for (int i = 0; i < 5; i++) {
        leftFingerActive[i] = false; rightFingerActive[i] = false;
        leftFingerProgress[i] = 0.0f; rightFingerProgress[i] = 0.0f;
    }
    
    leftHandRotated = false; rightHandRotated = false;
    leftHandRotAngle = 0.0f; rightHandRotAngle = 0.0f;

    charX = 0.0f; charZ = 0.0f; charRotation = 0.0f;
    walkPhase = 0.0f; walkArmSwing = 0.0f;
    for (int i = 0; i < 256; i++) keys[i] = false;

    weapon1_status = false;
    weapon2_status = false;
    fanTargetAngle = 0.0f;

    kAnimationActive = false;
    kAnimProgress = 0.0f;
    kAnimDistance = 0.0f;
    kAnimSpin = 0.0f;

    isNight = false;

    updateCamera();
}

//================================
// Weapon 1: Meteor Hammer logic
//================================

void getPathPoint(float t, float r, float L, float& px, float& py, float& nx, float& ny, float& traveled) {
    float arcLen = (float)M_PI * r;
    float totalLen = 2.0f * arcLen + 2.0f * L;
    float dist = t * totalLen;
    if (dist <= L) {
        px = -L / 2.0f + dist; py = r; nx = 0.0f; ny = 1.0f; traveled = dist;
    } else if (dist <= L + arcLen) {
        float a = (dist - L) / r; float angle = (float)M_PI / 2.0f - a;
        px = L / 2.0f + r * cos(angle); py = r * sin(angle); nx = cos(angle); ny = sin(angle); traveled = dist;
    } else if (dist <= 2.0f * L + arcLen) {
        float d = dist - (L + arcLen); px = L / 2.0f - d; py = -r; nx = 0.0f; ny = -1.0f; traveled = dist;
    } else {
        float d = dist - (2.0f * L + arcLen); float a = d / r; float angle = -(float)M_PI / 2.0f - a;
        px = -L / 2.0f + r * cos(angle); py = r * sin(angle); nx = cos(angle); ny = sin(angle); traveled = dist;
    }
}

void drawChainLink(GLUquadric* quad, float innerRadius, float outerRadius, float L, int nsides, int rings) {
    float arcLen = (float)M_PI * outerRadius;
    float totalLen = 2.0f * arcLen + 2.0f * L;
    for (int i = 0; i < rings; i++) {
        float t0 = (float)i / rings; float t1 = (float)(i + 1) / rings;
        float px0, py0, nx0, ny0, dist0_path; getPathPoint(t0, outerRadius, L, px0, py0, nx0, ny0, dist0_path);
        float px1, py1, nx1, ny1, dist1_path; getPathPoint(t1, outerRadius, L, px1, py1, nx1, ny1, dist1_path);
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= nsides; j++) {
            float phi = (float)j * 2.0f * (float)M_PI / nsides;
            float cosPhi = cos(phi); float sinPhi = sin(phi);
            float n0x = nx0 * cosPhi; float n0y = ny0 * cosPhi; float n0z = sinPhi;
            float v0x = px0 + innerRadius * n0x; float v0y = py0 + innerRadius * n0y; float v0z = innerRadius * n0z;
            float tx0 = (float)j / nsides; float ty0 = dist0_path / totalLen * 4.0f;
            glNormal3f(n0x, n0y, n0z); glTexCoord2f(tx0, ty0); glVertex3f(v0x, v0y, v0z);
            float n1x = nx1 * cosPhi; float n1y = ny1 * cosPhi; float n1z = sinPhi;
            float v1x = px1 + innerRadius * n1x; float v1y = py1 + innerRadius * n1y; float v1z = innerRadius * n1z;
            float ty1 = dist1_path / totalLen * 4.0f;
            glNormal3f(n1x, n1y, n1z); glTexCoord2f(tx0, ty1); glVertex3f(v1x, v1y, v1z);
        }
        glEnd();
    }
}

void drawMeteorHammer() {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH); gluQuadricTexture(quad, GL_TRUE);
    
    // Increase scale for better visibility and presence
    glPushMatrix();
    float weaponScale = 0.18f;
    
    // J skill scaling
    float currentHammerScale = jAnimHammerScale;
    glScalef(weaponScale, weaponScale, weaponScale);

    // 1. Wooden Handle
    if (!isShadowPass) {
        glBindTexture(GL_TEXTURE_2D, texWeaponWood);
    }
    glPushMatrix();
    glTranslatef(0.0f, -0.9f, 0.0f); // Centering handle in hand
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quad, 0.15f, 0.15f, 1.8f, 20, 20); 
    glPopMatrix();

    // 2. Metal parts
    glBindTexture(GL_TEXTURE_2D, texWeaponMetal);
    // Pommel / Top Rim
    for(int s_idx = 0; s_idx < 2; s_idx++) {
        int s = (s_idx == 0) ? -1 : 1;
        glPushMatrix();
        glTranslatef(0.0f, s == -1 ? -0.9f : 0.9f, 0.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        drawChainLink(quad, 0.05f, 0.16f, 0.0f, 15, 15);
        glPushMatrix(); glTranslatef(0, 0, -0.02f); gluDisk(quad, 0, 0.17f, 20, 1); glPopMatrix();
        glPopMatrix();
    }

    // 3. Chains
    glBindTexture(GL_TEXTURE_2D, texWeaponChain);
    // Link 0: Fixed to handle
    glPushMatrix();
    glTranslatef(0.0f, 1.1f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    drawChainLink(quad, 0.045f, 0.10f, 0.12f, 15, 15);
    glPopMatrix();

    // CENTRIFUGAL FORCE OVERRIDE for J animation
    // Skip hanging down logic if J animation is in full spin
    bool skipHanging = false;
    if (jAnimationActive) {
        // Rotation check: after first 180 degrees (roughly 0.5s into a 2s spin)
        if (jAnimProgress > 0.25f) skipHanging = true;
    }

    if (!skipHanging) {
        // Normal hanging/gravity logic
        glPushMatrix();
        glTranslatef(0.0f, 1.26f, 0.0f); 
        
        float slantAngle = 5.0f + (jAnimationActive ? 0 : leftArmAngle) * 0.20f;
        if (slantAngle > 35.0f) slantAngle = 35.0f;

        glPushMatrix();
        glLoadMatrixf(cameraViewMatrix);
        glRotatef(charRotation, 0.0f, 1.0f, 0.0f);
        float charM[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, charM);
        glPopMatrix();

        float m[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, m);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                m[i * 4 + j] = charM[i * 4 + j] * weaponScale;
            }
        }
        glLoadMatrixf(m);
        glRotatef(-slantAngle, 0.0f, 0.0f, 1.0f);

        int numChains = 16 + (int)(jAnimChainExtend / 0.32f);
        for (int i = 1; i < numChains; i++) {
            glPushMatrix();
            glTranslatef(0.0f, -(0.16f + (i - 1) * 0.32f), 0.0f);
            if (i % 2 == 0) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
            drawChainLink(quad, 0.045f, 0.10f, 0.12f, 15, 15);
            glPopMatrix();
        }

        glBindTexture(GL_TEXTURE_2D, texWeaponMetal);
        glPushMatrix();
        glTranslatef(0.0f, -(6.76f - 1.26f) - jAnimChainExtend, 0.0f); 
        glScalef(currentHammerScale, currentHammerScale, currentHammerScale);
        gluSphere(quad, 1.5f, 40, 40); 
        // Spikes
        int numLat = 5; int numLon = 8;
        for (int lat = 1; lat < numLat; lat++) {
            float theta = lat * (float)M_PI / numLat;
            for (int lon = 0; lon < numLon; lon++) {
                float phi = lon * 2 * (float)M_PI / numLon;
                float x = sin(theta) * cos(phi); float z = sin(theta) * sin(phi); float y = cos(theta);
                glPushMatrix();
                glTranslatef(x * 1.4375f, y * 1.4375f, z * 1.4375f);
                float angle = acos(z) * 180.0f / (float)M_PI;
                float len = sqrt(x*x + y*y);
                if (len > 0.0001f) glRotatef(angle, -y, x, 0.0f);
                else if (z < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(quad, 0.15f, 0.0f, 0.6875f, 10, 5);
                glPopMatrix();
            }
        }
        glPopMatrix();
        glPopMatrix();
    } else {
        // Centrifugal force logic: Chain extends straight from handle
        int numChains = 16 + (int)(jAnimChainExtend / 0.32f);
        for (int i = 1; i < numChains; i++) {
            glPushMatrix();
            glTranslatef(0.0f, 1.1f + i * 0.32f, 0.0f);
            if (i % 2 == 0) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
            drawChainLink(quad, 0.045f, 0.10f, 0.12f, 15, 15);
            glPopMatrix();
        }

        glBindTexture(GL_TEXTURE_2D, texWeaponMetal);
        glPushMatrix();
        glTranslatef(0.0f, 6.76f + jAnimChainExtend, 0.0f); 
        glScalef(currentHammerScale, currentHammerScale, currentHammerScale);
        gluSphere(quad, 1.5f, 40, 40); 
        // Spikes
        int numLat = 5; int numLon = 8;
        for (int lat = 1; lat < numLat; lat++) {
            float theta = lat * (float)M_PI / numLat;
            for (int lon = 0; lon < numLon; lon++) {
                float phi = lon * 2 * (float)M_PI / numLon;
                float x = sin(theta) * cos(phi); float z = sin(theta) * sin(phi); float y = cos(theta);
                glPushMatrix();
                glTranslatef(x * 1.4375f, y * 1.4375f, z * 1.4375f);
                float angle = acos(z) * 180.0f / (float)M_PI;
                float len = sqrt(x*x + y*y);
                if (len > 0.0001f) glRotatef(angle, -y, x, 0.0f);
                else if (z < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(quad, 0.15f, 0.0f, 0.6875f, 10, 5);
                glPopMatrix();
            }
        }
        glPopMatrix();
    }

    glPopMatrix(); // End scale push
    gluDeleteQuadric(quad);
}

//================================
// Weapon 2: Fan drawing logic (ported from weapon2.cpp)
//================================

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void drawFanCylinder(float baseRadius, float topRadius, float height, int slices, int stacks) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricDrawStyle(q, GLU_FILL);
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricTexture(q, GL_TRUE);
    gluCylinder(q, baseRadius, topRadius, height, slices, stacks);
    glPushMatrix(); glRotatef(180,1,0,0); gluDisk(q,0,baseRadius,slices,1); glPopMatrix();
    glPushMatrix(); glTranslatef(0,0,height); gluDisk(q,0,topRadius,slices,1); glPopMatrix();
    gluDeleteQuadric(q);
}

void drawFanRib(float length, float width, float thickness) {
    glPushMatrix();
    glScalef(width, length, thickness);
    glTranslatef(0.0f, 0.5f, 0.0f);
    glBegin(GL_QUADS);
    glNormal3f( 0,0, 1); glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f, 0.5f); glTexCoord2f(1,0); glVertex3f( 0.5f,-0.5f, 0.5f); glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f, 0.5f); glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f, 0.5f);
    glNormal3f( 0,0,-1); glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f,-0.5f); glTexCoord2f(1,1); glVertex3f(-0.5f, 0.5f,-0.5f); glTexCoord2f(0,1); glVertex3f( 0.5f, 0.5f,-0.5f); glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f,-0.5f);
    glNormal3f( 0,1, 0); glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f,-0.5f); glTexCoord2f(0,0); glVertex3f(-0.5f, 0.5f, 0.5f); glTexCoord2f(1,0); glVertex3f( 0.5f, 0.5f, 0.5f); glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f,-0.5f);
    glNormal3f( 0,-1,0); glTexCoord2f(1,1); glVertex3f(-0.5f,-0.5f,-0.5f); glTexCoord2f(0,1); glVertex3f( 0.5f,-0.5f,-0.5f); glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f, 0.5f); glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f, 0.5f);
    glNormal3f( 1,0, 0); glTexCoord2f(1,0); glVertex3f( 0.5f,-0.5f,-0.5f); glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f,-0.5f); glTexCoord2f(0,1); glVertex3f( 0.5f, 0.5f, 0.5f); glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f, 0.5f);
    glNormal3f(-1,0, 0); glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f,-0.5f); glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f, 0.5f); glTexCoord2f(1,1); glVertex3f(-0.5f, 0.5f, 0.5f); glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f,-0.5f);
    glEnd();
    glPopMatrix();
}

void drawFanGuard(float length, float thickness, float bL, float bR, float tL, float tR) {
    glPushMatrix();
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);  glTexCoord2f(0,0); glVertex3f(bL,0,thickness/2);  glTexCoord2f(1,0); glVertex3f(bR,0,thickness/2);  glTexCoord2f(1,1); glVertex3f(tR,length,thickness/2);  glTexCoord2f(0,1); glVertex3f(tL,length,thickness/2);
    glNormal3f(0,0,-1); glTexCoord2f(1,0); glVertex3f(bL,0,-thickness/2); glTexCoord2f(1,1); glVertex3f(tL,length,-thickness/2); glTexCoord2f(0,1); glVertex3f(tR,length,-thickness/2); glTexCoord2f(0,0); glVertex3f(bR,0,-thickness/2);
    glNormal3f(0,1,0);  glTexCoord2f(0,1); glVertex3f(tL,length,-thickness/2); glTexCoord2f(0,0); glVertex3f(tL,length,thickness/2);  glTexCoord2f(1,0); glVertex3f(tR,length,thickness/2);  glTexCoord2f(1,1); glVertex3f(tR,length,-thickness/2);
    glNormal3f(0,-1,0); glTexCoord2f(1,1); glVertex3f(bL,0,-thickness/2); glTexCoord2f(0,1); glVertex3f(bR,0,-thickness/2); glTexCoord2f(0,0); glVertex3f(bR,0,thickness/2);  glTexCoord2f(1,0); glVertex3f(bL,0,thickness/2);
    glNormal3f(1,0,0);  glTexCoord2f(1,0); glVertex3f(bR,0,-thickness/2); glTexCoord2f(1,1); glVertex3f(tR,length,-thickness/2); glTexCoord2f(0,1); glVertex3f(tR,length,thickness/2);  glTexCoord2f(0,0); glVertex3f(bR,0,thickness/2);
    glNormal3f(-1,0,0); glTexCoord2f(0,0); glVertex3f(bL,0,-thickness/2); glTexCoord2f(1,0); glVertex3f(bL,0,thickness/2);  glTexCoord2f(1,1); glVertex3f(tL,length,thickness/2);  glTexCoord2f(0,1); glVertex3f(tL,length,-thickness/2);
    glEnd();
    glPopMatrix();
}

void drawFanLeaf(int numFolds, float innerRadius, float outerRadius, float startAngle, float endAngle, float stackDepth) {
    float spreadAngle  = endAngle - startAngle;
    float closedFactor = 1.0f - (spreadAngle / 140.0f);
    int segPerGap = 10, total = numFolds * segPerGap;
    float step = (endAngle - startAngle) / total;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= total; i++) {
        float angle = startAngle + i * step;
        float rad = angle * PI / 180.0f;
        float t = (float)i / total;
        float baseZ = (stackDepth/2.0f) - stackDepth * t;
        int gIdx = (i == total) ? numFolds-1 : i / segPerGap;
        float lPos = (i == total) ? 1.0f : (float)(i % segPerGap) / segPerGap;
        float sign = (gIdx % 2 == 0) ? -1.0f : 1.0f;
        float tri = 1.0f - fabs(lPos * 2.0f - 1.0f);
        float fd = 0.14f * tri * closedFactor;
        float tx = -sin(rad), ty = cos(rad);
        float xI = innerRadius*cos(rad) + tx*fd*sign*(innerRadius/outerRadius);
        float yI = innerRadius*sin(rad) + ty*fd*sign*(innerRadius/outerRadius);
        float xO = outerRadius*cos(rad) + tx*fd*sign;
        float yO = outerRadius*sin(rad) + ty*fd*sign;
        float ozf = 0.2f*tri*sign*(1.0f-closedFactor);
        float zO = baseZ + ozf, zI = baseZ + ozf*(innerRadius/outerRadius);
        float px=tx,py=ty,pz=sign*0.5f,pl=sqrt(px*px+py*py+pz*pz);
        glNormal3f(px/pl,py/pl,pz/pl);
        float sh = 0.8f + tri*sign*0.15f; 
        glColor3f(sh, sh, sh); 
        float uI = ((innerRadius*cos(rad)/outerRadius)*0.5f+0.5f)*(1-closedFactor)+t*closedFactor;
        float vI = (innerRadius*sin(rad)/outerRadius)*(1-closedFactor)+(innerRadius/outerRadius)*closedFactor;
        float uO = ((outerRadius*cos(rad)/outerRadius)*0.5f+0.5f)*(1-closedFactor)+t*closedFactor;
        float vO = (outerRadius*sin(rad)/outerRadius)*(1-closedFactor)+1.0f*closedFactor;
        glTexCoord2f(uI,vI); glVertex3f(xI,yI,zI);
        glTexCoord2f(uO,vO); glVertex3f(xO,yO,zO);
    }
    glEnd();
}

void drawFan(float spreadAngle) {
    float ribCount = 21;
    float minAngle = 90.0f - (spreadAngle / 2.0f);
    float maxAngle = 90.0f + (spreadAngle / 2.0f);
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float stackDepth = 0.35f;

    glPushMatrix();
    // Scale down to fit character hand
    glScalef(0.12f, 0.12f, 0.12f);
    if (!isShadowPass && !isWireframe) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texFanWood);
    }

    // Pivot pin
    glPushMatrix();
    glColor3fv(white);
    glTranslatef(0,0,-(stackDepth/2.0f+0.04f));
    drawFanCylinder(0.12f,0.12f,stackDepth+0.08f,30,5);
    glPopMatrix();

    // Inner ribs
    float ribLen=2.05f, ribW=0.08f, ribThk=0.015f;
    float angleStep = spreadAngle / (ribCount-1);
    for (int i=1;i<(int)ribCount-1;i++) {
        float ang = minAngle + i*angleStep;
        glPushMatrix();
        glRotatef(ang-90.0f,0,0,1);
        glColor3fv(white);
        float zOff = (stackDepth/2.0f) - (stackDepth*i/(ribCount-1));
        glTranslatef(0,0,zOff);
        drawFanRib(ribLen,ribW,ribThk);
        glPopMatrix();
    }

    // Outer guards
    float gLen=5.05f,gBW=0.4f,gTW=0.35f,gThk=0.04f;
    glPushMatrix(); glRotatef(minAngle-90.0f,0,0,1); glTranslatef(0,0,stackDepth/2.0f+gThk/2.0f); glColor3fv(white); drawFanGuard(gLen,gThk,-gBW/2,gBW/2,-gTW/2,gTW/2); glPopMatrix();
    glPushMatrix(); glRotatef(maxAngle-90.0f,0,0,1); glTranslatef(0,0,-(stackDepth/2.0f+gThk/2.0f)); glColor3fv(white); drawFanGuard(gLen,gThk,-gBW/2,gBW/2,-gTW/2,gTW/2); glPopMatrix();

    // Fan leaf
    glBindTexture(GL_TEXTURE_2D, texFan);
    glColor3fv(white);
    drawFanLeaf((int)ribCount-1, 2.0f, 5.0f, minAngle, maxAngle, stackDepth);

    glPopMatrix();
}

//================================
// Animation update
//================================

void updateAnimation()
{
    DWORD currentTime = GetTickCount();
    if (lastTime == 0) lastTime = currentTime;
    float dt = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    float speed = 0.5f;
    for(int i = 0; i < 5; i++) {
        if (rightFingerActive[i]) rightFingerProgress[i] += speed * dt;
        else                      rightFingerProgress[i] -= speed * dt;
        if (rightFingerProgress[i] > 1.0f) rightFingerProgress[i] = 1.0f;
        if (rightFingerProgress[i] < 0.0f) rightFingerProgress[i] = 0.0f;

        if (leftFingerActive[i]) leftFingerProgress[i] += speed * dt;
        else                     leftFingerProgress[i] -= speed * dt;
        if (leftFingerProgress[i] > 1.0f) leftFingerProgress[i] = 1.0f;
        if (leftFingerProgress[i] < 0.0f) leftFingerProgress[i] = 0.0f;
    }

    float handRotSpeed = 180.0f; // degrees per second
    if (leftHandRotated) leftHandRotAngle += handRotSpeed * dt;
    else                 leftHandRotAngle -= handRotSpeed * dt;
    if (leftHandRotAngle > 60.0f) leftHandRotAngle = 60.0f;
    if (leftHandRotAngle < 0.0f) leftHandRotAngle = 0.0f;

    if (rightHandRotated) rightHandRotAngle += handRotSpeed * dt;
    else                  rightHandRotAngle -= handRotSpeed * dt;
    if (rightHandRotAngle > 60.0f) rightHandRotAngle = 60.0f;
    if (rightHandRotAngle < 0.0f) rightHandRotAngle = 0.0f;

    if (attackAnimation) {
        attackAngle += 2.0f;
        rightArmAngle = attackAngle;
        if (attackAngle > 90) {
            attackAnimation = false;
            rightArmAngle   = 0;
        }
    }

    // WASD Movement and Leg Animation
    float moveSpeed = 2.5f;
    float rotSpeed = 100.0f; // degrees per second
    bool moving = false;

    if (keys['A']) charRotation += rotSpeed * dt;
    if (keys['D']) charRotation -= rotSpeed * dt;

    float rad = charRotation * 3.14159265f / 180.0f;
    float dx = sin(rad) * moveSpeed * dt;
    float dz = cos(rad) * moveSpeed * dt;

    if (keys['W']) { charX += dx; charZ += dz; moving = true; }
    if (keys['S']) { charX -= dx; charZ -= dz; moving = true; }

    if (moving) {
        walkPhase += dt * 12.0f; // speed of leg oscillation
        leftLegAngle  = sin(walkPhase) * 35.0f;
        rightLegAngle = -sin(walkPhase) * 35.0f;
        walkArmSwing  = sin(walkPhase) * 10.0f; // subtle arm swing
    } else {
        // Smoothly reset legs and arm swing to neutral
        float resetSpeed = 8.0f;
        leftLegAngle  -= leftLegAngle * resetSpeed * dt;
        rightLegAngle -= rightLegAngle * resetSpeed * dt;
        walkArmSwing  -= walkArmSwing * resetSpeed * dt;

        if (fabs(leftLegAngle) < 0.1f)  leftLegAngle = 0;
        if (fabs(rightLegAngle) < 0.1f) rightLegAngle = 0;
        if (fabs(walkArmSwing) < 0.1f)  walkArmSwing = 0;
    }

    // Fan spread animation
    float fanSpeed = 180.0f; // degrees per second
    if (fanSpreadAngle < fanTargetAngle) {
        fanSpreadAngle += fanSpeed * dt;
        if (fanSpreadAngle > fanTargetAngle) fanSpreadAngle = fanTargetAngle;
    } else if (fanSpreadAngle > fanTargetAngle) {
        fanSpreadAngle -= fanSpeed * dt;
        if (fanSpreadAngle < fanTargetAngle) fanSpreadAngle = fanTargetAngle;
    }

        // K animation update
    if (kAnimationActive) {
        float kSpeed = 1.0f; // total animation takes 2 seconds (0 to 2)
        kAnimProgress += kSpeed * dt;
        
        if (kAnimProgress <= 1.0f) {
            // Move forward (outward) up to 20 units
            kAnimDistance = kAnimProgress * 20.0f; 
        } else if (kAnimProgress <= 2.0f) {
            // Return back to hand
            kAnimDistance = (2.0f - kAnimProgress) * 20.0f;
        } else {
            // Finish animation, restore state
            kAnimationActive = false;
            kAnimDistance = 0.0f;
            weapon1_status = preK_weapon1_status;
            weapon2_status = preK_weapon2_status;
            for(int i=0; i<5; i++) {
                leftFingerActive[i] = preK_leftFingerActive[i];
                leftFingerProgress[i] = preK_leftFingerActive[i] ? 1.0f : 0.0f; // Instant restore
            }
            fanTargetAngle = preK_fanTargetAngle;
        }
        
        // Spin the fan rapidly
        kAnimSpin += 1440.0f * dt; 
    }

    // J animation update
    if (jAnimationActive) {
        float jTotalDuration = 3.0f; // Total spin + strike duration
        jAnimProgress += dt;
        
        float spinPhase = 2.0f; // Time for 5 loops
        float strikePhase = 0.5f; // Time for extension
        float impactWait = 0.5f; // Wait at ground

        if (jAnimProgress <= spinPhase) {
            // Phase 1: 360-degree spin loop (4.5 times) to end at OVERHEAD position
            float spinProgress = jAnimProgress / spinPhase;
            leftArmAngle = -(spinProgress * 1620.0f); // Ends at -1620 (UP)
            jAnimChainExtend = 0.0f;
            jAnimHammerScale = 1.0f;
        } 
        else if (jAnimProgress <= spinPhase + strikePhase) {
            // Phase 2: Extension and Strike - Slam from UP to DOWN
            float strikeProgress = (jAnimProgress - spinPhase) / strikePhase;
            // From -1620 (UP) to -1800 (DOWN)
            leftArmAngle = -1620.0f - strikeProgress * 135.0f; 
            jAnimChainExtend = strikeProgress * 13.0f; 
            jAnimHammerScale = 1.0f + strikeProgress * 1.5f; 
        }
        else if (jAnimProgress <= spinPhase + strikePhase + impactWait) {
            // Phase 3: Impact!
            if (screenShakeTimer <= 0.0f && jAnimProgress < spinPhase + strikePhase + 0.1f) {
                screenShakeTimer = 0.5f; // Trigger shake on impact
            }
        }
        else {
            // Reset
            jAnimationActive = false;
            leftArmAngle = preJ_leftArmAngle;
            for(int i=0; i<5; i++) {
                leftFingerActive[i] = preJ_leftFingerActive[i];
                leftFingerProgress[i] = preJ_leftFingerProgress[i];
            }
            jAnimChainExtend = 0.0f;
            jAnimHammerScale = 1.0f;
        }
    }

    // Update screen shake
    if (screenShakeTimer > 0.0f) {
        screenShakeTimer -= dt;
        if (screenShakeTimer < 0.0f) screenShakeTimer = 0.0f;
    }
}

//================================
// Window procedure
//================================

LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        if (wParam < 256) keys[wParam] = true;
        switch (wParam)
        {
        case VK_ESCAPE: PostQuitMessage(0); break;

        // Reset
        case VK_SPACE: resetAll(); break;

        // Camera orbit / height / zoom
        case VK_LEFT:  if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) cameraAngle -= 0.05f; break;
        case VK_RIGHT: if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) cameraAngle += 0.05f; break;
        case VK_UP:    if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) cameraHeight += 0.3f; break;
        case VK_DOWN:
            if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                cameraHeight -= 0.3f;
                if (cameraHeight < 0.5f) cameraHeight = 0.5f;
            }
            break;
        case VK_ADD:    case VK_OEM_PLUS:  cameraDistance -= 0.3f; break;
        case VK_SUBTRACT: case VK_OEM_MINUS: cameraDistance += 0.3f; break;

        // NEW Arm rotation (ZX = left, CV = right)
        case 'Z': leftArmAngle += 5; if (leftArmAngle > 120) leftArmAngle = 120; break;
        case 'X': leftArmAngle -= 5; if (leftArmAngle < 0)   leftArmAngle = 0;   break;
        case 'C': rightArmAngle += 5; if (rightArmAngle > 120) rightArmAngle = 120; break;
        case 'V': rightArmAngle -= 5; if (rightArmAngle < 0)   rightArmAngle = 0;   break;

        case VK_F1:
            if (weapon2_status) weapon2_status = false;
            weapon1_status = !weapon1_status;
            for(int i=0; i<5; i++) leftFingerActive[i] = (weapon1_status || weapon2_status);
            break;

        case VK_F2:
            if (weapon1_status) weapon1_status = false;
            weapon2_status = !weapon2_status;
            for(int i=0; i<5; i++) leftFingerActive[i] = (weapon1_status || weapon2_status);
            if (weapon2_status) fanTargetAngle = 0.0f; 
            break;

        case '5':
        case 'O':
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                rightFingerActive[0] = !rightFingerActive[0];
            } else if (weapon2_status) {
                // Toggle fan open/close if it is equipped
                fanTargetAngle = (fanTargetAngle > 70.0f) ? 0.0f : 140.0f;
            }
            break;

        // Hand / Palm Rotation Toggle
        case 'F': leftHandRotated = !leftHandRotated; break;
        case 'G': rightHandRotated = !rightHandRotated; break;

        // Individual Finger toggles (Numpad, direct keys) - Disabled when weapon is equipped
        case VK_NUMPAD0: case VK_NUMPAD1: case VK_NUMPAD2: case VK_NUMPAD3: case VK_NUMPAD4:
        case VK_NUMPAD5: case VK_NUMPAD6: case VK_NUMPAD7: case VK_NUMPAD8: case VK_NUMPAD9:
            if (!(weapon1_status || weapon2_status)) {
                if (wParam <= VK_NUMPAD4)
                    leftFingerActive[wParam - VK_NUMPAD0] = !leftFingerActive[wParam - VK_NUMPAD0];
                else
                    rightFingerActive[wParam - VK_NUMPAD5] = !rightFingerActive[wParam - VK_NUMPAD5];
            }
            break;

        // Fist toggle  (1 = right, 2 = left, and Shift combinations)
        case '1':
        case 'N': 
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                leftFingerActive[1] = !leftFingerActive[1];
            } else {
                if (kAnimationActive) break; // blocked during K animation
                if (weapon1_status || weapon2_status) break;
                bool newState = !leftFingerActive[0];
                for (int i = 0; i < 5; i++) leftFingerActive[i] = newState;
            }
            break;
            
        case '2':
        case 'M': 
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                leftFingerActive[2] = !leftFingerActive[2];
            } else {
                bool newState = !rightFingerActive[0];
                for (int i = 0; i < 5; i++) rightFingerActive[i] = newState;
            }
            break;

        case 'K':
            if (!kAnimationActive && weapon2_status && fanTargetAngle > 0.0f) {
                kAnimationActive = true;
                kAnimProgress = 0.0f;
                kAnimDistance = 0.0f;
                kAnimSpin = 0.0f;
                
                // Store previous states
                preK_weapon1_status = weapon1_status;
                preK_weapon2_status = weapon2_status;
                for(int i=0; i<5; i++) {
                    preK_leftFingerActive[i] = leftFingerActive[i];
                    preK_leftFingerProgress[i] = leftFingerProgress[i];
                    leftFingerActive[i] = false;    // The hands open
                    leftFingerProgress[i] = 0.0f;   // Instant open
                }
                preK_fanTargetAngle = fanTargetAngle;
                
                weapon1_status = false;    // Hide meteor hammer temporarily
            }
            break;

        case 'P':
            isNight = !isNight;
            break;

        case 'I':
            isWireframe = !isWireframe;
            break;

        case 'J':
            if (!jAnimationActive && !kAnimationActive && weapon1_status) {
                jAnimationActive = true;
                jAnimProgress = 0.0f;
                // Save state
                preJ_leftArmAngle = leftArmAngle;
                for(int i=0; i<5; i++) {
                    preJ_leftFingerActive[i] = leftFingerActive[i];
                    preJ_leftFingerProgress[i] = leftFingerProgress[i];
                    leftFingerActive[i] = true; // Grip weapon tight
                }
            }
            break;
        }
        break;

    case WM_KEYUP:
        if (wParam < 256) keys[wParam] = false;
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//================================
// OpenGL init
//================================

bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.cAlphaBits  = 8; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.cStencilBits = 0;
    pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iLayerType  = PFD_MAIN_PLANE;
    pfd.iPixelType  = PFD_TYPE_RGBA;
    pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion    = 1;
    int n = ChoosePixelFormat(hdc, &pfd);
    return SetPixelFormat(hdc, n, &pfd) ? true : false;
}

void initOpenGL()
{
    glClearColor(0.45f, 0.72f, 0.95f, 1.0f); // sky blue
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0 / 600.0, 0.1f, 200.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
}

void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    float lx = lightX, ly = lightY, lz = lightZ;
    GLfloat lightPos[] = { lx, ly, lz, 1.0f };
    
    GLfloat ambient[4], diffuse[4], specular[4];

    if (!isNight) {
        // Day values
        ambient[0] = 0.35f; ambient[1] = 0.35f; ambient[2] = 0.30f; ambient[3] = 1.0f;
        diffuse[0] = 1.0f;  diffuse[1] = 0.95f; diffuse[2] = 0.80f; diffuse[3] = 1.0f;
        specular[0] = 1.0f; specular[1] = 1.0f;  specular[2] = 0.90f; specular[3] = 1.0f;
    } else {
        // Night values (Dimmer, bluer)
        ambient[0] = 0.10f; ambient[1] = 0.10f; ambient[2] = 0.18f; ambient[3] = 1.0f;
        diffuse[0] = 0.25f; diffuse[1] = 0.25f; diffuse[2] = 0.40f; diffuse[3] = 1.0f;
        specular[0] = 0.20f; specular[1] = 0.20f; specular[2] = 0.30f; specular[3] = 1.0f;
    }

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
}

//================================================================
//  BACKGROUND  (from background.cpp)
//================================================================

void drawGround()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glColor3f(1.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f,  0.0f); glVertex3f(-60, 0, -60);
    glTexCoord2f( 0.0f, 20.0f); glVertex3f(-60, 0,  60);
    glTexCoord2f(20.0f, 20.0f); glVertex3f( 60, 0,  60);
    glTexCoord2f(20.0f,  0.0f); glVertex3f( 60, 0, -60);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawCloud(float cx, float cy, float cz, float scale)
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    float offsets[][4] = {
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 1.2f, 0.3f, 0.0f, 0.8f },
        {-1.2f, 0.3f, 0.0f, 0.8f },
        { 0.6f, 0.7f, 0.0f, 0.7f },
        {-0.6f, 0.7f, 0.0f, 0.7f },
        { 0.0f, 0.9f, 0.0f, 0.6f },
        { 1.8f, 0.0f, 0.0f, 0.5f },
        {-1.8f, 0.0f, 0.0f, 0.5f },
    };
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        glTranslatef(cx + offsets[i][0]*scale, cy + offsets[i][1]*scale, cz);
        gluSphere(q, offsets[i][3]*scale, 16, 16);
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

void drawSun(float sx, float sy, float sz)
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 0.95f, 0.3f);
    glPushMatrix(); glTranslatef(sx, sy, sz); gluSphere(q, 1.5f, 32, 32); glPopMatrix();
    glColor3f(1.0f, 0.85f, 0.2f);
    glPushMatrix(); glTranslatef(sx, sy, sz); gluSphere(q, 1.9f, 32, 32); glPopMatrix();
    gluDeleteQuadric(q);
    glEnable(GL_LIGHTING);
}

void drawMoon(float mx, float my, float mz)
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Main moon body (pale silver)
    glColor3f(0.9f, 0.9f, 0.95f);
    glPushMatrix(); glTranslatef(mx, my, mz); gluSphere(q, 1.5f, 32, 32); glPopMatrix();

    // Subtle glow (very pale blue)
    glColor3f(0.7f, 0.7f, 0.9f);
    glPushMatrix(); glTranslatef(mx, my, mz); gluSphere(q, 1.7f, 32, 32); glPopMatrix();

    gluDeleteQuadric(q);
    glEnable(GL_LIGHTING);
}

void drawBackground()
{
    GLfloat lightPos[] = { lightX, lightY, lightZ, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    if (!isNight) {
        drawSun(lightX, lightY, lightZ);
    } else {
        drawMoon(lightX, lightY, lightZ);
    }

    drawCloud(-18.0f, 14.0f, -40.0f, 1.5f);
    drawCloud(-25.0f, 16.0f, -35.0f, 1.2f);
    drawCloud( 20.0f, 15.0f, -38.0f, 1.8f);
    drawCloud( 28.0f, 13.0f, -32.0f, 1.1f);
    drawCloud(-10.0f, 12.0f, -28.0f, 1.4f);
    drawCloud( 12.0f, 11.0f, -25.0f, 1.6f);
    drawCloud(-20.0f, 10.0f, -18.0f, 1.0f);
    drawCloud( 22.0f, 12.0f, -20.0f, 1.3f);
    drawCloud(  0.0f, 17.0f, -50.0f, 2.0f);
    drawCloud(-32.0f, 14.0f, -45.0f, 1.7f);

    drawGround();
}

//================================================================
//  LEGS  (from legs.cpp)
//================================================================

void drawProceduralLegPart(float length, float topRx, float topRy,
                            float bottomRx, float bottomRy,
                            int slices, int stacks,
                            float bulgeFactor, float bulgePos)
{
    if (isShadowPass) {
        slices = (slices > 10) ? 10 : slices;
        stacks = (stacks > 8) ? 8 : stacks;
    }

    for (int i = 0; i < stacks; ++i) {
        float t1 = (float)i / stacks;
        float t2 = (float)(i + 1) / stacks;
        float z1 = t1 * length, z2 = t2 * length;

        float rx1 = topRx*(1-t1)+bottomRx*t1;
        float ry1 = topRy*(1-t1)+bottomRy*t1;
        float b1  = sin(t1*3.14159265f)*bulgeFactor*exp(-pow(t1-bulgePos,2)*10.0f);
        rx1+=b1; ry1+=b1;

        float rx2 = topRx*(1-t2)+bottomRx*t2;
        float ry2 = topRy*(1-t2)+bottomRy*t2;
        float b2  = sin(t2*3.14159265f)*bulgeFactor*exp(-pow(t2-bulgePos,2)*10.0f);
        rx2+=b2; ry2+=b2;

        if (isShadowPass) {
            glColor3f(0.15f, 0.15f, 0.15f);
            if (isNight) glColor3f(0.01f, 0.01f, 0.02f);
        }

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float theta = (float)j/slices * 2.0f*3.14159265f;
            float s     = (float)j/slices;
            float cosT  = cos(theta), sinT = sin(theta);

            float nx1=0, ny1=0, nz1=0, nx2=0, ny2=0, nz2=0;
            if (!isShadowPass) {
                nx1=ry1*cosT; ny1=rx1*sinT; nz1=(topRx-bottomRx)/length*((rx1+ry1)*0.5f);
                float len1=sqrt(nx1*nx1+ny1*ny1+nz1*nz1);
                if(len1>0){nx1/=len1;ny1/=len1;nz1/=len1;}

                nx2=ry2*cosT; ny2=rx2*sinT; nz2=(topRx-bottomRx)/length*((rx2+ry2)*0.5f);
                float len2=sqrt(nx2*nx2+ny2*ny2+nz2*nz2);
                if(len2>0){nx2/=len2;ny2/=len2;nz2/=len2;}
            }

            if (!isShadowPass) {
                glNormal3f(nx2,ny2,nz2);
                glTexCoord2f(s*2.0f, t2*2.0f);
            }
            glVertex3f(rx2*cosT, ry2*sinT, z2);

            if (!isShadowPass) {
                glNormal3f(nx1,ny1,nz1);
                glTexCoord2f(s*2.0f, t1*2.0f);
            }
            glVertex3f(rx1*cosT, ry1*sinT, z1);
        }
        glEnd();
    }
}

void drawRoundTrim(float r)
{
    // Front
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,1,0); glVertex3f(0,r,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(cos(a)*r*0.9f,r,sin(a)*r*1.4f); }
    glEnd();
    // Back
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,-1,0); glVertex3f(0,-r,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(-cos(a)*r*0.9f,-r,sin(a)*r*1.4f); }
    glEnd();
    // Right
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(1,0,0); glVertex3f(r,0,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(r,-cos(a)*r*0.9f,sin(a)*r*1.4f); }
    glEnd();
    // Left
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(-1,0,0); glVertex3f(-r,0,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(-r,cos(a)*r*0.9f,sin(a)*r*1.4f); }
    glEnd();
}

void drawLegBase(bool isLeft)
{
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);

    glPushMatrix();
    glRotatef(90, 1, 0, 0);  // leg points downward in Y-up space

    if (!isLeft) {
        // Right leg logic
        if (!isShadowPass) {
            glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
        }
        drawProceduralLegPart(0.75f, 0.19f,0.20f, 0.12f,0.12f, 60,40, 0.015f,0.5f);
        glTranslatef(0,0,0.75f);
        gluSphere(quad, 0.095f, 40,40);
        drawProceduralLegPart(0.12f, 0.12f,0.12f, 0.13f,0.13f, 60,20, 0.005f,0.5f);
        glTranslatef(0,0,0.12f);
        if (!isShadowPass) {
            glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
        }
        drawProceduralLegPart(0.06f, 0.14f,0.14f, 0.13f,0.13f, 60,20, 0.0f,0.5f);
        drawRoundTrim(0.14f);
        glTranslatef(0,0,0.06f);
        if (!isShadowPass) {
            glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texFabric);
        }
        drawProceduralLegPart(0.65f, 0.12f,0.12f, 0.07f,0.07f, 60,50, 0.015f,0.2f);
        glTranslatef(0,0,0.65f);
    } else {
        // Left leg: high sock (mid thigh)
        if (!isShadowPass) {
            glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
        }
        drawProceduralLegPart(0.45f, 0.19f,0.20f, 0.15f,0.15f, 60,40, 0.012f,0.5f);
        glTranslatef(0,0,0.45f);
        if (!isShadowPass) {
            glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
        }
        drawProceduralLegPart(0.06f, 0.16f,0.16f, 0.14f,0.14f, 60,20, 0.0f,0.5f);
        drawRoundTrim(0.16f);
        glTranslatef(0,0,0.06f);
        if (!isShadowPass) {
            glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texFabric);
        }
        drawProceduralLegPart(0.24f, 0.14f,0.14f, 0.12f,0.12f, 60,30, 0.012f,0.5f);
        glTranslatef(0,0,0.24f);
        gluSphere(quad, 0.095f, 40,40);
        drawProceduralLegPart(0.12f, 0.12f,0.12f, 0.13f,0.13f, 60,20, 0.005f,0.5f);
        glTranslatef(0,0,0.12f);
        if (!isShadowPass) {
            drawProceduralLegPart(0.06f, 0.14f,0.14f, 0.13f,0.13f, 60,20, 0.0f,0.5f);
        } else {
            drawProceduralLegPart(0.06f, 0.14f,0.14f, 0.13f,0.13f, 60,20, 0.0f,0.5f);
        }
        glTranslatef(0,0,0.06f);
        drawProceduralLegPart(0.65f, 0.12f,0.12f, 0.07f,0.07f, 60,50, 0.015f,0.2f);
        glTranslatef(0,0,0.65f);
    }

    // Boot inner ankle
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texWhiteLeather);
    drawProceduralLegPart(0.12f, 0.08f,0.08f, 0.085f,0.085f, 60,20, 0.0f,0.0f);

    // Boot flap top
    glPushMatrix();
    glTranslatef(0,0,-0.08f);
    drawProceduralLegPart(0.10f, 0.11f,0.11f, 0.085f,0.085f, 60,20, 0.0f,0.5f);
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
    gluCylinder(quad, 0.12f, 0.12f, 0.02f, 60,20);
    glTranslatef(0,0,0.02f);
    gluCylinder(quad, 0.12f, 0.09f, 0.05f, 60,20);
    glPopMatrix();

    // Gold balls (both sides)
    glPushMatrix(); glTranslatef( 0.095f,0,0.04f); gluSphere(quad,0.045f,40,40); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.095f,0,0.04f); gluSphere(quad,0.045f,40,40); glPopMatrix();

    glTranslatef(0,0,0.13f); // ankle joint

    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texWhiteLeather);
    gluSphere(quad, 0.09f, 40,40);

    // Boot foot group
    glPushMatrix();
    glTranslatef(0,0.08f,0.06f);
    glRotatef(20.0f,1,0,0);

    glPushMatrix();
    glScalef(0.12f,0.20f,0.12f);
    gluSphere(quad,1.0f,60,60);
    glPopMatrix();

    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texDarkLeather);
    glPushMatrix();
    glTranslatef(0,0.04f,0.07f);
    glScalef(0.09f,0.14f,0.035f);
    gluSphere(quad,1.0f,30,30);
    glPopMatrix();
    glPopMatrix();

    // Heel (Positioned at back curve, lengthened to prevent burial)
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
    glPushMatrix();
    // Move slightly back (-Y) and set starting depth
    glTranslatef(0, -0.025f, 0.02f); 
    gluCylinder(quad, 0.050, 0.03, 0.175f, 40, 40);
    glTranslatef(0,0,0.175f);
    gluDisk(quad, 0.0, 0.04, 40, 1);
    glPopMatrix();

    gluDeleteQuadric(quad);
    glPopMatrix();
}

void drawLeftLeg()
{
    glPushMatrix();
    glTranslatef(-0.16f, 1.53f, 0.0f);
    glRotatef(leftLegAngle, 1, 0, 0);
    drawLegBase(true);
    glPopMatrix();
}

void drawRightLeg()
{
    glPushMatrix();
    glTranslatef(0.16f, 1.53f, 0.0f);
    glRotatef(rightLegAngle, 1, 0, 0);
    drawLegBase(false);
    glPopMatrix();
}

//================================================================
//  ARMS  (from arms.cpp)
//================================================================

void drawProceduralArmPart(float length,
    float topRx, float topRy, float bottomRx, float bottomRy,
    int slices, int stacks, float bulgeFactor, float bulgePos,
    float sweepStart = 0.0f, float sweepEnd = 2.0f*3.14159265f,
    float pleatDepth = 0.0f, int pleatCount = 0)
{
    if (isShadowPass) {
        slices = (slices > 10) ? 10 : slices;
        stacks = (stacks > 8) ? 8 : stacks;
    }
    for (int i = 0; i < stacks; ++i) {
        float t1 = (float)i/stacks, t2 = (float)(i+1)/stacks;
        float z1 = t1*length, z2 = t2*length;

        float rx1=topRx*(1-t1)+bottomRx*t1, ry1=topRy*(1-t1)+bottomRy*t1;
        float b1=sin(t1*3.14159265f)*bulgeFactor*exp(-pow(t1-bulgePos,2)*10.0f);
        rx1+=b1; ry1+=b1;

        float rx2=topRx*(1-t2)+bottomRx*t2, ry2=topRy*(1-t2)+bottomRy*t2;
        float b2=sin(t2*3.14159265f)*bulgeFactor*exp(-pow(t2-bulgePos,2)*10.0f);
        rx2+=b2; ry2+=b2;

        if (isShadowPass) {
            glColor3f(0.15f, 0.15f, 0.15f);
            if (isNight) glColor3f(0.01f, 0.01f, 0.02f);
        }

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float ts = (float)j/slices;
            float theta = sweepStart*(1-ts)+sweepEnd*ts;
            float cosT=cos(theta), sinT=sin(theta);
            float u=(float)j/slices;

            float pM=(pleatCount>0)?cos(theta*pleatCount):0.0f;
            float ro1=pM*pleatDepth*t1, ro2=pM*pleatDepth*t2;
            float rx1p=rx1+ro1, ry1p=ry1+ro1;
            float rx2p=rx2+ro2, ry2p=ry2+ro2;

            float shade=1.0f;
            if(pleatCount>0){
                float pS=0.75f+0.25f*(pM*0.5f+0.5f);
                shade=1.0f*(1-t1)+pS*t1;
            }

            float avgR1=(rx1+ry1)*0.5f;
            float nx1=ry1*cosT, ny1=rx1*sinT, nz1=(topRx-bottomRx)/length*avgR1;
            float l1=sqrt(nx1*nx1+ny1*ny1+nz1*nz1);
            if(l1>0){nx1/=l1;ny1/=l1;nz1/=l1;}

            float avgR2=(rx2+ry2)*0.5f;
            float nx2=ry2*cosT, ny2=rx2*sinT, nz2=(topRx-bottomRx)/length*avgR2;
            float l2=sqrt(nx2*nx2+ny2*ny2+nz2*nz2);
            if(l2>0){nx2/=l2;ny2/=l2;nz2/=l2;}

            glColor3f(shade,shade,shade);
            if (!isShadowPass) {
                glNormal3f(nx2,ny2,nz2); glTexCoord2f(u,t2);
            }
            glVertex3f(rx2p*cosT, ry2p*sinT, z2);
            if (!isShadowPass) {
                glNormal3f(nx1,ny1,nz1); glTexCoord2f(u,t1);
            }
            glVertex3f(rx1p*cosT, ry1p*sinT, z1);
        }
        glEnd();
        glColor3f(1,1,1);
    }
}

void drawProceduralArmBase(bool isLeft, float* fingerProgress, int part)
{
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    if (!isShadowPass) {
        glBindTexture(GL_TEXTURE_2D, texSkin);
        glColor3f(1,1,1);
    }

    glPushMatrix();

    // Small Shoulder Assembly (2 cylinders + 1 sphere)
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, texSkin);
    // Position the assembly at the arm rotation center (origin)
    glTranslatef(0.0f, 0.0f, 0.0f); 
    
    if (part == 0) {
        // 2. Torso Connector Cylinder (Static part)
        glPushMatrix();
        glRotatef(isLeft ? 90.0f : -90.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(quad, 0.12f, 0.12f, 0.22f, 32, 2);
        glPopMatrix();
    } else {
        // 1. Central Ball Joint (Rotating part)
        gluSphere(quad, 0.12f, 32, 32);

        // 3. Arm Connector Cylinder (Rotating part)
        glPushMatrix();
        gluCylinder(quad, 0.12f, 0.12f, 0.22f, 32, 2);
        glPopMatrix();
    }

    glPopMatrix();

    if (part == 0) {
        glPopMatrix();
        gluDeleteQuadric(quad);
        return;
    }

    // Shift the arm mesh down to make the shoulder connector cylinder visible
    glTranslatef(0.0f, 0.0f, 0.18f); 
    // Apply outward tilt to the WHOLE arm (from joint down)
    glRotatef(isLeft?-10.0f:10.0f, 0, 1, 0);

    // ---- Upper arm clothing ----
    glDisable(GL_TEXTURE_2D);

    if (!isShadowPass && !isWireframe) {
        glBindTexture(GL_TEXTURE_2D, texFabric);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1,1,1);
    }
    glPushMatrix();
    drawProceduralArmPart(0.53f,0.13f,0.13f,0.10f,0.10f,40,20,0.015f,0.5f);
    glPopMatrix();

    // 2. Pauldron
    glPushMatrix();
    glTranslatef(isLeft?-0.14f:0.14f, 0.0f, 0.05f);
    glRotatef(isLeft?-3.0f:3.0f, 0,1,0);
    glRotatef(0.0f, 1,0,0);
    float peakZ=0.25f, peakX=isLeft?-0.06f:0.06f, width=0.13f;
    glColor3f(0.2f,0.22f,0.3f);
    glBegin(GL_TRIANGLES);
    float nx_p=isLeft?-1.0f:1.0f;
    glNormal3f(nx_p,0,0);
    glVertex3f(0,0,-0.08f); glVertex3f(0,-width,0.1f); glVertex3f(peakX,0,peakZ);
    glVertex3f(0,0,-0.08f); glVertex3f(peakX,0,peakZ); glVertex3f(0,width,0.1f);
    glVertex3f(0,-width,0.1f); glVertex3f(0,0,0.35f); glVertex3f(peakX,0,peakZ);
    glVertex3f(0,width,0.1f); glVertex3f(peakX,0,peakZ); glVertex3f(0,0,0.35f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, texGold); glColor3f(1,1,1);
    float ox=isLeft?-0.002f:0.002f;
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(ox,0,-0.08f); glVertex3f(ox,-width,0.1f);
    glVertex3f(ox,0,0.35f);  glVertex3f(ox,width,0.1f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(ox,0,-0.08f); glVertex3f(peakX+ox,0,peakZ); glVertex3f(ox,0,0.35f);
    glEnd();
    glPopMatrix();

    // 3. Two Gold bicep bands
    glBindTexture(GL_TEXTURE_2D, texGold); glColor3f(1,1,1);
    for(int i=0; i<2; i++) {
        glPushMatrix();
        glTranslatef(0, 0, 0.08f + i*0.06f);
        gluCylinder(quad, 0.136f, 0.136f, 0.035f, 30, 1);
        glPopMatrix();
    }

    // 4. White flared sleeve
    glPushMatrix();
    glTranslatef(0,0,0.35f);

    if (!isShadowPass) {
        glDisable(GL_TEXTURE_2D);
    }
    glColor3f(0.25f,0.15f,0.05f);
    GLUquadric* qBand=gluNewQuadric();
    gluQuadricNormals(qBand,GLU_SMOOTH);

    glPushMatrix();
    float bRTop=0.120f, bThickTop=0.028f; // Increased radius to prevent clipping
    gluDisk(qBand,0.08f,bRTop,30,1);
    gluCylinder(qBand,bRTop,bRTop,bThickTop,30,1);
    glTranslatef(0,0,bThickTop); gluDisk(qBand,0.08f,bRTop,30,1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0,0,0.16f);
    float bRBot=0.127f, bThickBot=0.012f; // Increased radius to prevent clipping
    gluDisk(qBand,0.08f,bRBot,30,1);
    gluCylinder(qBand,bRBot,bRBot,bThickBot,30,1);
    glTranslatef(0,0,bThickBot); gluDisk(qBand,0.08f,bRBot,30,1);
    glPopMatrix();
    gluDeleteQuadric(qBand);

    glTranslatef(0,0,0.005f);
    if (!isShadowPass && !isWireframe) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texWhiteSleeve);
    }
    glColor3f(1,1,1);
    drawProceduralArmPart(0.42f,0.115f,0.115f,0.21f,0.21f,60,30,0.02f,0.5f,
                          0.0f,2.0f*3.14159265f,0.02f,8);
    if (!isShadowPass) {
        glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();

    if (!isShadowPass && !isWireframe) {
        glEnable(GL_TEXTURE_2D);
    }
    glColor3f(1,1,1);

    // Upper arm skin
    drawProceduralArmPart(0.53f,0.125f,0.125f,0.095f,0.095f,40,40,0.015f,0.5f);

    // Elbow joint
    glTranslatef(0,0,0.53f);
    glBindTexture(GL_TEXTURE_2D, texFabric); glColor3f(1,1,1);
    gluSphere(quad,0.11f,40,40);
    glBindTexture(GL_TEXTURE_2D, texSkin);

    // Forearm
    glRotatef(-5,1,0,0);
    float wristRx= 0.065f, wristRy= 0.045f;
    drawProceduralArmPart(0.53f,0.10f,0.10f,wristRx,wristRy,40,40,0.015f,0.3f);

    // Forearm sleeve
    glBindTexture(GL_TEXTURE_2D, texFabric); glColor3f(1,1,1);
    drawProceduralArmPart(0.53f,0.105f,0.105f,wristRx+0.003f,wristRy+0.003f,40,20,0.015f,0.3f);

    // Gold wrist rings
    glPushMatrix();
    glTranslatef(0,0,0.46f);
    glBindTexture(GL_TEXTURE_2D, texGold); glColor3f(1,1,1);
    GLUquadric* qR=gluNewQuadric(); gluQuadricTexture(qR,GL_TRUE);
    for(int r=0;r<3;r++){
        glPushMatrix(); glTranslatef(0,0,r*0.015f);
        glScalef(1.0f,0.85f,1.0f); gluCylinder(qR,0.075f,0.075f,0.010f,30,1);
        glPopMatrix();
    }
    gluDeleteQuadric(qR);
    glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, texSkin);

    // Wrist joint
    glTranslatef(0,0,0.53f);
    glRotatef(isLeft?90.0f:-90.0f,0,0,1);

    // Dynamic wrist tilt when opening the fan
    if (isLeft && weapon2_status) {
        float tilt = (fanSpreadAngle / 140.0f) * 60.0f;
        glRotatef(tilt, 0, 0, 1);
    }

    // Toggleable Hand/Palm rotation (60 degree twist)
    if (isLeft) {
        glRotatef(leftHandRotAngle, 0, 0, 1);
    } else {
        glRotatef(-rightHandRotAngle, 0, 0, 1);
    }

    // Hand palm
    float knuckleRx=0.06f, knuckleRy=0.025f, handLength=0.16f;
    glRotatef(isLeft?5:-5,0,1,0);
    glRotatef(-5,1,0,0);
    drawProceduralArmPart(handLength,wristRx,wristRy,knuckleRx,knuckleRy,30,20,0.01f,0.5f);

    glPushMatrix();
    glTranslatef(0,0,handLength);
    glScalef(1.0f,knuckleRy/knuckleRx,0.35f);
    gluSphere(quad,knuckleRx,30,30);
    glPopMatrix();

    // Thumb
    glPushMatrix();
    glTranslatef(isLeft?knuckleRx*0.85f:-knuckleRx*0.85f,-knuckleRy*0.8f,handLength*0.35f);
    glRotatef(isLeft?35.0f:-35.0f,0,1,0);
    glRotatef(10.0f,1,0,0);
    float thumbProg = fingerProgress[0];
    glRotatef(isLeft?(-thumbProg*65.0f):(thumbProg*65.0f),0,1,0);
    glRotatef(thumbProg*15.0f,1,0,0);
    glPushMatrix(); gluSphere(quad,0.016f,16,16); glPopMatrix();
    drawProceduralArmPart(0.065f,0.016f,0.016f,0.014f,0.014f,16,16,0.003f,0.5f);
    glTranslatef(0,0,0.065f); gluSphere(quad,0.014f,16,16);
    glRotatef(isLeft?(-thumbProg*60.0f):(thumbProg*60.0f),0,1,0);
    glRotatef(thumbProg*5.0f,1,0,0);
    drawProceduralArmPart(0.05f,0.014f,0.014f,0.011f,0.011f,16,16,0.002f,0.5f);
    glTranslatef(0,0,0.05f); gluSphere(quad,0.011f,16,16);
    glPopMatrix();

    // Fingers
    glTranslatef(0,0,handLength);
    float fingerSpace=0.028f;
    float fingerPos[]   ={1.5f*fingerSpace,0.5f*fingerSpace,-0.5f*fingerSpace,-1.5f*fingerSpace};
    float fingerLen[]   ={0.075f,0.085f,0.08f,0.06f};
    float fingerAng[]   ={6.0f,0.0f,-4.0f,-10.0f};
    float fingerZOff[]  ={0.007f,0.016f,0.01f,0.0f};
    for(int i=0;i<4;i++){
        float currFingerProg = fingerProgress[i+1];
        glPushMatrix();
        glTranslatef(isLeft?fingerPos[i]:-fingerPos[i],0,fingerZOff[i]);
        float rx=0.014f,ry=0.012f;
        glPushMatrix(); gluSphere(quad,rx,16,16); glPopMatrix();
        glRotatef(isLeft?fingerAng[i]:-fingerAng[i],0,1,0);

        // Standard tight fist curling
        float j1Base = 12.0f;
        float j1Fist = 80.0f;
        glRotatef(j1Base + currFingerProg*j1Fist, 1,0,0);
        drawProceduralArmPart(fingerLen[i],rx,ry,rx*0.9f,ry*0.9f,16,16,0.003f,0.5f);
        glTranslatef(0,0,fingerLen[i]); gluSphere(quad,rx*0.9f,16,16);

        float j2Base = 25.0f;
        float j2Fist = 75.0f;
        glRotatef(j2Base + currFingerProg*j2Fist, 1,0,0);
        drawProceduralArmPart(fingerLen[i]*0.7f,rx*0.9f,ry*0.9f,rx*0.75f,ry*0.75f,16,16,0.002f,0.5f);
        glTranslatef(0,0,fingerLen[i]*0.7f); gluSphere(quad,rx*0.75f,16,16);

        float j3Base = 20.0f;
        float j3Fist = 70.0f;
        glRotatef(j3Base + currFingerProg*j3Fist, 1,0,0);
        drawProceduralArmPart(fingerLen[i]*0.5f,rx*0.75f,ry*0.75f,rx*0.6f,ry*0.6f,16,16,0.001f,0.5f);
        glTranslatef(0,0,fingerLen[i]*0.5f); gluSphere(quad,rx*0.6f,16,16);
        glPopMatrix();
    }

    // ---- Weapon 1: Meteor Hammer ----
    // Held on LEFT arm (appears on screen RIGHT when character faces camera)
    if (isLeft && weapon1_status) {
        glPushMatrix();

        // NOTE: We are currently at the KNUCKLE origin (after glTranslatef(0,0,handLength)).
        // To reach MID-PALM (wrist z=0.08), we go BACK by -0.08 in wrist Z
        // (-0.08 from knuckle = wrist + 0.08 = mid palm position, inside the fist)
        // Y = -0.04 shifts handle toward palm face where fingers curl (grip sweet spot)
        glTranslatef(0.0f, -0.032f, -0.032f);

        // Rotate -90deg around wrist Z so weapon +Y -> wrist +X -> world forward (+Z)
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
        
        // Align weapon main axis (Y) with arm axis (Z) so it points forward from hand
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        if (!isShadowPass) {
            drawMeteorHammer();
        } else {
            drawMeteorHammer();
        }
        glPopMatrix();
    }

    // ---- Weapon 2: Fan (starts folded) ----
    // Held on LEFT arm (appears on screen RIGHT when character faces camera)
    if (isLeft && weapon2_status) {
        glPushMatrix();
        // Counter-rotate the weapon so it doesn't follow the hand tilt
        float tilt = (fanSpreadAngle / 140.0f) * 60.0f;
        glRotatef(-tilt, 0, 0, 1);

        // Translating to palm SWEET SPOT, just like Weapon 1
        glTranslatef(0.0f, -0.032f, -0.032f);
        
        // Rotate -90deg around Z so Fan points forward like Meteor Hammer
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
        
        if (kAnimationActive) {
            // Unwind rotations to align perfectly with Character Root space for forward flight
            glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
            glRotatef(tilt, 0.0f, 0.0f, 1.0f);
            glRotatef(5.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(-5.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(-tilt, 0.0f, 0.0f, 1.0f);
            glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
            glRotatef(5.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(10.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(leftArmAngle - walkArmSwing, 1.0f, 0.0f, 0.0f);

            // Move purely forward (+Z in character space)
            glTranslatef(0.0f, 0.0f, kAnimDistance);
            
            // Lay the fan flat horizontally to ground (spin left-right)
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            
            // Make the fan spin like a disc
            glRotatef(kAnimSpin, 0.0f, 0.0f, 1.0f);

            // Scale fan to 3x size smoothly over the first 0.3s, shrink at end
            float kScale = 1.0f;
            if (kAnimProgress < 0.3f) {
                kScale = 1.0f + (kAnimProgress / 0.3f) * 2.0f;
            } else if (kAnimProgress > 1.7f) {
                kScale = 1.0f + ((2.0f - kAnimProgress) / 0.3f) * 2.0f;
            } else {
                kScale = 3.0f;
            }
            glScalef(kScale, kScale, kScale);
        }

        if (!isShadowPass) {
            drawFan(fanSpreadAngle);
        } else {
            drawFan(fanSpreadAngle);
        }
        glPopMatrix();
    }

    gluDeleteQuadric(quad);
    glPopMatrix();
}

// Arms are placed left/right of the upper body (at shoulder height).
// The body mesh top (shoulders) sits at world Y ≈ charY + 1.35.
// We offset X by ±0.50 from centre so they clear the wide torso.
void drawLeftArm()
{
    glPushMatrix();
    glTranslatef(-0.58f, 1.24f, 0.04f); // Set origin to ball joint center
    
    // 1. Draw Static shoulder part (connector) — not affected by arm rotation
    glPushMatrix();
    glRotatef(90, 1, 0, 0); 
    drawProceduralArmBase(true, leftFingerProgress, 0); // part 0 = static
    glPopMatrix();

    // 2. Apply arm rotation and draw rotating part (now centered on ball joint)
    glRotatef(-(leftArmAngle - walkArmSwing), 1, 0, 0); 
    glRotatef(90, 1, 0, 0);
    drawProceduralArmBase(true, leftFingerProgress, 1); // part 1 = rotating
    glPopMatrix();
}

void drawRightArm()
{
    glPushMatrix();
    glTranslatef(0.58f, 1.24f, 0.04f); // Set origin to ball joint center

    // 1. Draw Static shoulder part (connector) — not affected by arm rotation
    glPushMatrix();
    glRotatef(90, 1, 0, 0);
    drawProceduralArmBase(false, rightFingerProgress, 0); // part 0 = static
    glPopMatrix();

    // 2. Apply arm rotation and draw rotating part (now centered on ball joint)
    glRotatef(-(rightArmAngle + walkArmSwing), 1, 0, 0);
    glRotatef(90, 1, 0, 0);
    drawProceduralArmBase(false, rightFingerProgress, 1); // part 1 = rotating
    glPopMatrix();
}

//================================================================
//  BODY  (from body.cpp) — procedural torso + dress mesh
//================================================================

// Cubic Catmull-Rom interpolation
float spline(float t, float p0, float p1, float p2, float p3) {
    float t2=t*t, t3=t2*t;
    return 0.5f*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t2+(-p0+3*p1-3*p2+p3)*t3);
}

struct CrossSection { float z,rx,ry,yOffset; };
#define SEC_COUNT 10
CrossSection sections[SEC_COUNT] = {
    {-0.2f, 0.42f,0.26f, 0.0f},
    { 0.0f, 0.45f,0.28f, 0.0f},
    { 0.3f, 0.47f,0.32f,-0.06f},
    { 0.6f, 0.31f,0.21f, 0.02f},
    { 0.9f, 0.34f,0.24f, 0.08f},
    { 1.15f,0.40f,0.26f, 0.10f},
    { 1.35f,0.46f,0.21f, 0.03f},
    { 1.42f,0.22f,0.18f, 0.00f},
    { 1.58f,0.13f,0.13f,-0.04f},
    { 1.80f,0.13f,0.13f,-0.04f}
};

void getTorsoBaseRadius(float z,float&rx,float&ry,float&yOfs) {
    if(z<=sections[1].z){rx=sections[1].rx;ry=sections[1].ry;yOfs=sections[1].yOffset;return;}
    if(z>=sections[SEC_COUNT-2].z){rx=sections[SEC_COUNT-2].rx;ry=sections[SEC_COUNT-2].ry;yOfs=sections[SEC_COUNT-2].yOffset;return;}
    int i=1;
    for(;i<SEC_COUNT-2;i++) if(z<=sections[i+1].z) break;
    float t=(z-sections[i].z)/(sections[i+1].z-sections[i].z);
    rx=spline(t,sections[i-1].rx,sections[i].rx,sections[i+1].rx,sections[i+2].rx);
    ry=spline(t,sections[i-1].ry,sections[i].ry,sections[i+1].ry,sections[i+2].ry);
    yOfs=spline(t,sections[i-1].yOffset,sections[i].yOffset,sections[i+1].yOffset,sections[i+2].yOffset);
}

void getTorsoVertex(float z,float theta,float&px,float&py,float&pz) {
    float rx,ry,yOfs; getTorsoBaseRadius(z,rx,ry,yOfs);
    float bump=0;
    float maxBump=0.12f,spreadZ=0.02f,spreadTheta=0.12f;
    while(theta<0)theta+=2*3.14159f; while(theta>2*3.14159f)theta-=2*3.14159f;
    float breastZ=1.1f,tL=3.14159f*0.35f,tR=3.14159f*0.65f;
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(theta-tL,2)/spreadTheta));
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(theta-tR,2)/spreadTheta));
    bump-=0.06f*exp(-(pow(z-breastZ,2)/0.03f+pow(theta-3.14159f*0.5f,2)/0.01f));
    float gZ=0.25f,gL=3.14159f*1.35f,gR=3.14159f*1.65f;
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(theta-gL,2)/0.15f));
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(theta-gR,2)/0.15f));
    bump+=0.03f*exp(-(pow(z-0.55f,2)/0.04f+pow(theta-3.14159f*0.5f,2)/0.2f));
    rx+=bump; ry+=bump;
    px=rx*cos(theta); py=ry*sin(theta)+yOfs; pz=z;
}

void getTorsoNormal(float z,float theta,float&nx,float&ny,float&nz) {
    float eZ=0.01f,eT=0.05f;
    float p1[3],p2[3],p3[3],p4[3];
    getTorsoVertex(z+eZ,theta,p1[0],p1[1],p1[2]);
    getTorsoVertex(z-eZ,theta,p2[0],p2[1],p2[2]);
    getTorsoVertex(z,theta+eT,p3[0],p3[1],p3[2]);
    getTorsoVertex(z,theta-eT,p4[0],p4[1],p4[2]);
    float tx1=p1[0]-p2[0],ty1=p1[1]-p2[1],tz1=p1[2]-p2[2];
    float tx2=p3[0]-p4[0],ty2=p3[1]-p4[1],tz2=p3[2]-p4[2];
    nx=ty2*tz1-tz2*ty1; ny=tz2*tx1-tx2*tz1; nz=tx2*ty1-ty2*tx1;
    float l=sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
}

void getDressTorsoVertex(float z,float theta,float offset,float&px,float&py,float&pz) {
    float rx,ry,yOfs; getTorsoBaseRadius(z,rx,ry,yOfs);
    float bump=0,maxBump=0.12f,spreadZ=0.02f,spreadTheta=0.12f;
    float nt=theta; while(nt<0)nt+=2*3.14159f; while(nt>2*3.14159f)nt-=2*3.14159f;
    float breastZ=1.1f,tL=3.14159f*0.35f,tR=3.14159f*0.65f;
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(nt-tL,2)/spreadTheta));
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(nt-tR,2)/spreadTheta));
    bump-=0.06f*exp(-(pow(z-breastZ,2)/0.03f+pow(nt-3.14159f*0.5f,2)/0.01f));
    float gZ=0.25f,gL=3.14159f*1.35f,gR=3.14159f*1.65f;
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(nt-gL,2)/0.15f));
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(nt-gR,2)/0.15f));
    bump+=0.03f*exp(-(pow(z-0.55f,2)/0.04f+pow(nt-3.14159f*0.5f,2)/0.2f));
    rx+=bump+offset; ry+=bump+offset;
    px=rx*cos(theta); py=ry*sin(theta)+yOfs; pz=z;
}

void getDressTorsoNormal(float z,float theta,float offset,float&nx,float&ny,float&nz) {
    float eZ=0.01f,eT=0.05f;
    float p1[3],p2[3],p3[3],p4[3];
    getDressTorsoVertex(z+eZ,theta,offset,p1[0],p1[1],p1[2]);
    getDressTorsoVertex(z-eZ,theta,offset,p2[0],p2[1],p2[2]);
    getDressTorsoVertex(z,theta+eT,offset,p3[0],p3[1],p3[2]);
    getDressTorsoVertex(z,theta-eT,offset,p4[0],p4[1],p4[2]);
    float tx1=p1[0]-p2[0],ty1=p1[1]-p2[1],tz1=p1[2]-p2[2];
    float tx2=p3[0]-p4[0],ty2=p3[1]-p4[1],tz2=p3[2]-p4[2];
    nx=ty2*tz1-tz2*ty1; ny=tz2*tx1-tx2*tz1; nz=tx2*ty1-ty2*tx1;
    float l=sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
}

void drawDressMesh()
{
    glDisable(GL_TEXTURE_2D);
    int slices=60;

    // 1. White top bodice
    int topStacks=15;
    for(int i=0;i<topStacks;i++){
        float f1=(float)i/topStacks, f2=(float)(i+1)/topStacks;
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float corsetTop=0.95f-0.20f*exp(-pow(dipTh,2)/0.15f);
            float topZ1=corsetTop-0.05f;
            float zTopMax=1.05f+0.18f*exp(-pow(fabs(dipTh)-0.5f,2)/0.08f)
                              +0.05f*exp(-pow(dipTh,2)/0.05f)
                              +0.10f*exp(-pow(fabs(dipTh)-3.14159f,2)/1.0f);
            if(topZ1>zTopMax-0.02f)topZ1=zTopMax-0.02f;
            float z1=topZ1+f1*(zTopMax-topZ1), z2=topZ1+f2*(zTopMax-topZ1);
            float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
            getDressTorsoVertex(z2,theta,0.015f,px2,py2,pz2);
            getDressTorsoNormal(z2,theta,0.015f,nx2,ny2,nz2);
            getDressTorsoVertex(z1,theta,0.015f,px1,py1,pz1);
            getDressTorsoNormal(z1,theta,0.015f,nx1,ny1,nz1);
            float shade=0.95f+0.1f*cos(theta); if(shade>1)shade=1;
            auto applyGrad=[&](float f){
                float t=f*1.5f; if(t>1)t=1;
                glColor3f((0.40f+t*0.60f)*shade,(0.65f+t*0.35f)*shade,shade);
            };
            applyGrad(f2); glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
            applyGrad(f1); glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    }

    // 2. Dark blue corset
    int corsetStacks=12;
    for(int i=0;i<corsetStacks;i++){
        float f1=(float)i/corsetStacks,f2=(float)(i+1)/corsetStacks;
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float corsetTop=0.95f-0.20f*exp(-pow(dipTh,2)/0.15f);
            float corsetBot=0.52f+0.12f*exp(-pow(dipTh,2)/0.15f);
            float z1=corsetBot+f1*(corsetTop-corsetBot),z2=corsetBot+f2*(corsetTop-corsetBot);
            float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
            getDressTorsoVertex(z2,theta,0.025f,px2,py2,pz2);
            getDressTorsoNormal(z2,theta,0.025f,nx2,ny2,nz2);
            getDressTorsoVertex(z1,theta,0.025f,px1,py1,pz1);
            getDressTorsoNormal(z1,theta,0.025f,nx1,ny1,nz1);
            float dS=0.5f+0.5f*f1, cS=0.85f+0.15f*cos(theta*2),shade=dS*cS;
            glColor3f(0.04f*shade,0.15f*shade,0.45f*shade);
            glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
            glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    }

    // 3. Gold irregular belts
    auto drawIrregularBelt=[&](bool isTop){
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float borderZ=isTop?(0.95f-0.20f*exp(-pow(dipTh,2)/0.15f))
                               :(0.52f+0.12f*exp(-pow(dipTh,2)/0.15f));
            float z1=borderZ-0.015f,z2=borderZ+0.015f;
            float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
            getDressTorsoVertex(z2,theta,0.035f,px2,py2,pz2);
            getDressTorsoNormal(z2,theta,0.035f,nx2,ny2,nz2);
            getDressTorsoVertex(z1,theta,0.035f,px1,py1,pz1);
            getDressTorsoNormal(z1,theta,0.035f,nx1,ny1,nz1);
            float shade=0.85f+0.15f*cos(theta);
            glColor3f(0.9f*shade,0.75f*shade,0.2f*shade);
            glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
            glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    };
    drawIrregularBelt(false); drawIrregularBelt(true);

    // 3b. Centre emblem
    glColor3f(0.9f,0.75f,0.2f);
    float cZ=0.72f,cTheta=3.14159f*0.5f;
    float pTop[3],pBot[3],pL[3],pR[3],pC[3];
    getDressTorsoVertex(0.88f,cTheta,0.04f,pTop[0],pTop[1],pTop[2]);
    getDressTorsoVertex(0.53f,cTheta,0.04f,pBot[0],pBot[1],pBot[2]);
    getDressTorsoVertex(cZ,cTheta+0.10f,0.04f,pL[0],pL[1],pL[2]);
    getDressTorsoVertex(cZ,cTheta-0.10f,0.04f,pR[0],pR[1],pR[2]);
    getDressTorsoVertex(cZ,cTheta,0.08f,pC[0],pC[1],pC[2]);
    glBegin(GL_TRIANGLES);
    auto drawFacet=[&](float*p1,float*p2,float*p3){
        float u[3]={p2[0]-p1[0],p2[1]-p1[1],p2[2]-p1[2]};
        float v[3]={p3[0]-p1[0],p3[1]-p1[1],p3[2]-p1[2]};
        float nx=u[1]*v[2]-u[2]*v[1],ny=u[2]*v[0]-u[0]*v[2],nz=u[0]*v[1]-u[1]*v[0];
        float l=sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
        glNormal3f(nx,ny,nz); glVertex3fv(p1); glVertex3fv(p2); glVertex3fv(p3);
    };
    drawFacet(pTop,pR,pC); drawFacet(pTop,pC,pL);
    drawFacet(pBot,pC,pR); drawFacet(pBot,pL,pC);
    glEnd();
    float pTB[3],pBB[3],pLB[3],pRB[3];
    getDressTorsoVertex(0.88f,cTheta,0.025f,pTB[0],pTB[1],pTB[2]);
    getDressTorsoVertex(0.53f,cTheta,0.025f,pBB[0],pBB[1],pBB[2]);
    getDressTorsoVertex(cZ,cTheta+0.10f,0.025f,pLB[0],pLB[1],pLB[2]);
    getDressTorsoVertex(cZ,cTheta-0.10f,0.025f,pRB[0],pRB[1],pRB[2]);
    glBegin(GL_QUADS);
    glVertex3fv(pTop);glVertex3fv(pR);glVertex3fv(pRB);glVertex3fv(pTB);
    glVertex3fv(pR);glVertex3fv(pBot);glVertex3fv(pBB);glVertex3fv(pRB);
    glVertex3fv(pBot);glVertex3fv(pL);glVertex3fv(pLB);glVertex3fv(pBB);
    glVertex3fv(pL);glVertex3fv(pTop);glVertex3fv(pTB);glVertex3fv(pLB);
    glEnd();

    // Filigree curves
    auto drawThickCurve=[&](float sZ,float sT,float eZ,float eT,float cZ2,float cT,float thick){
        glBegin(GL_QUAD_STRIP);
        int steps=15;
        for(int i=0;i<=steps;i++){
            float t=(float)i/steps,mt=1-t;
            float bz=mt*mt*sZ+2*mt*t*cZ2+t*t*eZ;
            float bt=mt*mt*sT+2*mt*t*cT+t*t*eT;
            float dz=2*mt*(cZ2-sZ)+2*t*(eZ-cZ2);
            float dt2=2*mt*(cT-sT)+2*t*(eT-cT);
            float l=sqrt(dz*dz+dt2*dt2); if(l<0.0001f)l=1;
            float pZ=-dt2/l*thick,pT=dz/l*thick;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(bz+pZ,cTheta+bt+pT,0.04f,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(bz-pZ,cTheta+bt-pT,0.04f,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(bz,cTheta+bt,0.04f,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p1); glVertex3fv(p2);
        }
        glEnd();
    };
    int sides[]={-1,1};
    for(int i=0;i<2;i++){
        float dir=(float)sides[i];
        drawThickCurve(0.57f,0.03f*dir,0.68f,0.35f*dir,0.50f,0.25f*dir,0.012f);
        drawThickCurve(0.68f,0.35f*dir,0.75f,0.18f*dir,0.85f,0.35f*dir,0.012f);
        drawThickCurve(0.75f,0.18f*dir,0.69f,0.25f*dir,0.65f,0.15f*dir,0.010f);
        drawThickCurve(0.68f,0.35f*dir,0.93f,0.60f*dir,0.80f,0.45f*dir,0.010f);
        drawThickCurve(0.62f,0.25f*dir,0.53f,0.60f*dir,0.56f,0.35f*dir,0.010f);
        if(sides[i]==-1){
            drawThickCurve(0.55f,-0.35f,0.45f,-0.28f,0.45f,-0.35f,0.008f);
            drawThickCurve(0.45f,-0.28f,0.58f,-0.25f,0.55f,-0.25f,0.008f);
        }
    }

    // 4. High collar
    glColor3f(0.04f,0.15f,0.45f);
    glBegin(GL_QUAD_STRIP);
    for(int j=0;j<=slices;j++){
        float theta=(float)j/slices*2*3.14159f;
        float dipTh=theta-3.14159f*0.5f;
        if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
        float zBot=1.40f;
        if(fabs(dipTh)<0.25f)zBot-=0.07f*(0.25f-fabs(dipTh))/0.25f;
        float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
        getDressTorsoVertex(1.46f,theta,0.015f,px2,py2,pz2);
        getDressTorsoNormal(1.46f,theta,0.015f,nx2,ny2,nz2);
        getDressTorsoVertex(zBot,theta,0.015f,px1,py1,pz1);
        getDressTorsoNormal(zBot,theta,0.015f,nx1,ny1,nz1);
        glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
        glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
    }
    glEnd();

    auto drawTopCurve=[&](float sZ,float sT,float eZ,float eT,float cZ2,float cT,float thick,float offset){
        glBegin(GL_QUAD_STRIP);
        int steps=15;
        for(int i=0;i<=steps;i++){
            float t=(float)i/steps,mt=1-t;
            float bz=mt*mt*sZ+2*mt*t*cZ2+t*t*eZ;
            float bt=mt*mt*sT+2*mt*t*cT+t*t*eT;
            float dz=2*mt*(cZ2-sZ)+2*t*(eZ-cZ2);
            float dt2=2*mt*(cT-sT)+2*t*(eT-cT);
            float l=sqrt(dz*dz+dt2*dt2); if(l<0.0001f)l=1;
            float pZ=-dt2/l*thick,pT=dz/l*thick;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(bz+pZ,cTheta+bt+pT,offset,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(bz-pZ,cTheta+bt-pT,offset,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(bz,cTheta+bt,offset,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p1); glVertex3fv(p2);
        }
        glEnd();
    };

    // Dark blue straps
    glColor3f(0.04f,0.15f,0.45f);
    for(int i=0;i<2;i++){
        float dir=(float)sides[i];
        drawTopCurve(1.23f+0.03f,0.08f*dir,1.15f,0.65f*dir,1.18f,0.40f*dir,0.06f,0.02f);
    }

    // Collar rings
    glColor3f(0.85f,0.7f,0.15f);
    auto drawCollarRings=[&](){
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float zC=1.40f;
            if(fabs(dipTh)<0.25f)zC-=0.07f*(0.25f-fabs(dipTh))/0.25f;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(zC+0.008f,theta,0.02f,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(zC-0.008f,theta,0.02f,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(zC,theta,0.02f,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p2); glVertex3fv(p1);
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(1.46f+0.008f,theta,0.02f,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(1.46f-0.008f,theta,0.02f,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(1.46f,theta,0.02f,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p2); glVertex3fv(p1);
        }
        glEnd();
    };
    drawCollarRings();

    // Chest diamond  + straps gold trim
    for(int i=0;i<2;i++){
        float dir=(float)sides[i];
        drawTopCurve(1.33f,0.0f,1.25f,0.15f*dir,1.29f,0.08f*dir,0.012f,0.025f);
        drawTopCurve(1.25f,0.15f*dir,1.15f,0.0f,1.20f,0.08f*dir,0.012f,0.025f);
        drawTopCurve(1.23f+0.03f,0.08f*dir+0.04f*dir,1.15f,0.65f*dir+0.04f*dir,
                     1.18f,0.40f*dir+0.04f*dir,0.01f,0.021f);
    }

    // Blue gems
    glColor3f(0.3f,0.5f,0.95f);
    float g1Z=1.27f,g1Th=0.04f,g1H=0.04f;
    glBegin(GL_POLYGON);
    float gn[3]; getDressTorsoNormal(g1Z,cTheta,0.035f,gn[0],gn[1],gn[2]);
    float g1T[3],g1B[3],g1L[3],g1R[3];
    getDressTorsoVertex(g1Z+g1H,cTheta,0.04f,g1T[0],g1T[1],g1T[2]);
    getDressTorsoVertex(g1Z-g1H,cTheta,0.04f,g1B[0],g1B[1],g1B[2]);
    getDressTorsoVertex(g1Z,cTheta+g1Th,0.04f,g1L[0],g1L[1],g1L[2]);
    getDressTorsoVertex(g1Z,cTheta-g1Th,0.04f,g1R[0],g1R[1],g1R[2]);
    glNormal3fv(gn); glVertex3fv(g1T); glVertex3fv(g1R); glVertex3fv(g1B); glVertex3fv(g1L);
    glEnd();
    float g2Z=1.19f,g2Th=0.025f,g2H=0.025f;
    glBegin(GL_POLYGON);
    float g2T[3],g2B[3],g2L[3],g2R[3];
    getDressTorsoVertex(g2Z+g2H,cTheta,0.04f,g2T[0],g2T[1],g2T[2]);
    getDressTorsoVertex(g2Z-g2H,cTheta,0.04f,g2B[0],g2B[1],g2B[2]);
    getDressTorsoVertex(g2Z,cTheta+g2Th,0.04f,g2L[0],g2L[1],g2L[2]);
    getDressTorsoVertex(g2Z,cTheta-g2Th,0.04f,g2R[0],g2R[1],g2R[2]);
    glNormal3fv(gn); glVertex3fv(g2T); glVertex3fv(g2R); glVertex3fv(g2B); glVertex3fv(g2L);
    glEnd();

    // 5. Skirt layers
    struct Layer{float zMin,flareMax,color[3];int folds;float foldDepth;};
    Layer layers[4]={
        {-1.0f,0.28f,{0.1f,0.3f,0.8f},8,0.15f},
        {-0.7f,0.22f,{0.6f,0.75f,0.95f},7,0.12f},
        {-0.4f,0.15f,{1.0f,1.0f,1.0f},6,0.08f},
        {-0.1f,0.08f,{0.05f,0.15f,0.35f},5,0.04f}
    };
    for(int l=0;l<4;l++){
        glColor3fv(layers[l].color);
        int skirtStacks=20;
        float zSkirtMax=0.55f,zSkirtMin=layers[l].zMin,flare=layers[l].flareMax;
        for(int i=0;i<skirtStacks;i++){
            float f1=(float)i/skirtStacks,f2=(float)(i+1)/skirtStacks;
            glBegin(GL_QUAD_STRIP);
            for(int j=0;j<=slices;j++){
                float theta=(float)j/slices*2*3.14159f;
                float frontDist=sin(theta);
                float lm=0.8f-frontDist*0.2f;
                if(l==3){float cy=theta/(2*3.14159f)*6,fr=cy-floor(cy),tw=fabs(fr*2-1);lm=0.6f+0.35f*tw;}
                float actZMin=zSkirtMax-(zSkirtMax-zSkirtMin)*lm;
                if(actZMin>zSkirtMax)actZMin=zSkirtMax;
                float actZ1=zSkirtMax-f1*(zSkirtMax-actZMin);
                float actZ2=zSkirtMax-f2*(zSkirtMax-actZMin);
                float fl1=f1*f1*flare,fl2=f2*f2*flare;
                float fo1=sin(theta*layers[l].folds)*layers[l].foldDepth*f1;
                float fo2=sin(theta*layers[l].folds)*layers[l].foldDepth*f2;
                float bx1,by1,bz1,bx2,by2,bz2;
                getDressTorsoVertex(actZ1,theta,0.025f+l*0.003f,bx1,by1,bz1);
                getDressTorsoVertex(actZ2,theta,0.025f+l*0.003f,bx2,by2,bz2);
                float px1=bx1+cos(theta)*(fl1+fo1),py1=by1+sin(theta)*(fl1+fo1);
                float px2=bx2+cos(theta)*(fl2+fo2),py2=by2+sin(theta)*(fl2+fo2);
                float vx=px2-px1,vy=py2-py1,vz=actZ2-actZ1;
                float tx=-sin(theta),ty=cos(theta);
                float nx=ty*vz-0*vy,ny=0*vx-tx*vz,nz2=tx*vy-ty*vx;
                float nl=sqrt(nx*nx+ny*ny+nz2*nz2);if(nl>0.001f){nx/=nl;ny/=nl;nz2/=nl;}
                glNormal3f(nx,ny,nz2); glVertex3f(px2,py2,actZ2);
                glNormal3f(nx,ny,nz2); glVertex3f(px1,py1,actZ1);
            }
            glEnd();
        }
    }

    // 6. Golden skirt accents
    glColor3f(0.85f,0.7f,0.15f);
    for(int i=0;i<6;i++){
        float theta=i/6.0f*2*3.14159f;
        float zSkirtMax=0.55f,zSkirtMin=layers[3].zMin;
        float actZMin=zSkirtMax-(zSkirtMax-zSkirtMin)*0.95f;
        float bx,by,bz2;
        getDressTorsoVertex(actZMin,theta,0.025f+3*0.003f+0.015f,bx,by,bz2);
        float fv=layers[3].flareMax,fov=sin(theta*layers[3].folds)*layers[3].foldDepth;
        float cx=bx+cos(theta)*(fv+fov),cy2=by+sin(theta)*(fv+fov),cz=actZMin;
        float nx2=cos(theta),ny2=sin(theta),nz3=0.3f;
        float nl=sqrt(nx2*nx2+ny2*ny2+nz3*nz3);nx2/=nl;ny2/=nl;nz3/=nl;
        float tx=-sin(theta),ty=cos(theta);
        float w=0.035f,len=0.18f,slZ=0.05f;
        glBegin(GL_POLYGON);
        glNormal3f(nx2,ny2,nz3);
        glVertex3f(cx-tx*w,cy2-ty*w,cz);
        glVertex3f(cx+tx*w,cy2+ty*w,cz-slZ);
        glVertex3f(cx+tx*w,cy2+ty*w,cz-len);
        glVertex3f(cx-tx*w,cy2-ty*w,cz-len+slZ);
        glEnd();
        glColor3f(1.0f,0.9f,0.4f);
        glBegin(GL_POLYGON);
        glNormal3f(nx2,ny2,nz3);
        float iC=cx+nx2*0.005f,iCY=cy2+ny2*0.005f;
        glVertex3f(iC,iCY,cz-len*0.2f);
        glVertex3f(iC+tx*w*0.5f,iCY+ty*w*0.5f,cz-len*0.4f);
        glVertex3f(iC,iCY,cz-len*0.6f);
        glVertex3f(iC-tx*w*0.5f,iCY-ty*w*0.5f,cz-len*0.4f);
        glEnd();
        glColor3f(0.85f,0.7f,0.15f);
    }

    glEnable(GL_TEXTURE_2D);
}

void drawBodyMesh()
{
    int stacks = isShadowPass ? 12 : 60;
    int slices = isShadowPass ? 12 : 60;
    float zMin=sections[1].z, zMax=sections[SEC_COUNT-2].z;
    if (!isShadowPass && !isWireframe) {
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
        glEnable(GL_TEXTURE_2D);
    }
    for(int i=0;i<stacks;i++){
        float f1=(float)i/stacks,f2=(float)(i+1)/stacks;
        float z1=zMin+f1*(zMax-zMin),z2=zMin+f2*(zMax-zMin);
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f,s=(float)j/slices*2;
            float px2,py2,pz2,nx2,ny2,nz2;
            getTorsoVertex(z2,theta,px2,py2,pz2); getTorsoNormal(z2,theta,nx2,ny2,nz2);
            if (!isShadowPass) {
                glNormal3f(nx2,ny2,nz2); glTexCoord2f(s,f2*2); 
            }
            glVertex3f(px2,py2,pz2);

            float px1,py1,pz1,nx1,ny1,nz1;
            getTorsoVertex(z1,theta,px1,py1,pz1); getTorsoNormal(z1,theta,nx1,ny1,nz1);
            if (!isShadowPass) {
                glNormal3f(nx1,ny1,nz1); glTexCoord2f(s,f1*2); 
            }
            glVertex3f(px1,py1,pz1);
        }
        glEnd();
    }
    // Neck cap
    glBegin(GL_POLYGON);
    glNormal3f(0,0,1);
    for(int j=0;j<=slices;j++){
        float theta=(float)j/slices*2*3.14159f;
        float px,py,pz; getTorsoVertex(zMax,theta,px,py,pz);
        glTexCoord2f((px/0.15f+1)/2,(py/0.15f+1)/2); glVertex3f(px,py,pz);
    }
    glEnd();
    // Pelvis cap
    glBegin(GL_POLYGON);
    glNormal3f(0,0,-1);
    for(int j=slices;j>=0;j--){
        float theta=(float)j/slices*2*3.14159f;
        float px,py,pz; getTorsoVertex(zMin,theta,px,py,pz);
        glTexCoord2f((px/0.45f+1)/2,(py/0.28f+1)/2); glVertex3f(px,py,pz);
    }
    glEnd();
}

//================================================================
//  SHADOWS (Planar Squish Method)
//================================================================

void drawCharacter(); // Forward declaration

void drawPlanarShadow()
{
    glPushMatrix();
    
    // Lift slightly to avoid z-buffer fighting
    glTranslatef(0.0f, 0.01f, 0.0f);

    // Made the shadow longer by moving artificial light closer to original
    float sLightX = lightX * 0.9f;
    float sLightY = lightY * 1.4f;
    float sLightZ = lightZ * 0.9f;

    // Squish matrix for light onto y=0 plane
    GLfloat matrix[16] = {
        sLightY,  0.0f, 0.0f, 0.0f,           // Column 0
        -sLightX, 0.0f, -sLightZ, -1.0f,      // Column 1
        0.0f,     0.0f, sLightY,  0.0f,       // Column 2
        0.0f,     0.0f, 0.0f,     sLightY     // Column 3
    };

    glMultMatrixf(matrix);

    // Shadow state: Force pure black unconditionally using the lighting pipeline
    glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
    
    glEnable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_COLOR_MATERIAL); // Critical: Ignore all glColor3f calls inside the character meshes
    glDisable(GL_TEXTURE_2D);

    // Calculate shadow color based on isNight
    GLfloat shadowColor[4];
    if (isNight) {
        shadowColor[0] = 0.01f; shadowColor[1] = 0.01f; shadowColor[2] = 0.04f; shadowColor[3] = 1.0f; 
    } else {
        // Slightly lighter grey for daytime
        shadowColor[0] = 0.12f; shadowColor[1] = 0.12f; shadowColor[2] = 0.12f; shadowColor[3] = 1.0f;
    }

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, shadowColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, shadowColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, shadowColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, shadowColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, shadowColor);

    isShadowPass = true;
    drawCharacter(); 
    isShadowPass = false;

    // Restore all states seamlessly
    glPopAttrib();
    glPopMatrix();
}

//================================================================
//  CHARACTER assembly  (body uses Z-up so we rotate -90° on X)
//================================================================

// charY is the world-Y level of the character's hip joint.
// Leg total length ≈ 1.25 units (from drawLegBase, origin Y=1.53 to foot).
// We lift the entire character group so feet rest on Y = 0.
#define CHAR_Y 1.92f

void drawHeadAndHair() {
    glPushMatrix();
    // 1. Position at the manually adjusted neck location
    glTranslatef(0.0f, 0.2f, 1.85f); 
    
    // --- RENDER HEAD MESH ---
    glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
    glDisable(GL_BLEND); 
    if (!isShadowPass) setupLightingHead();
    
    glPushMatrix();
    // Orient head mesh: Face camera (Torso Z-up -> Head.obj Y-up)
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); 
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f); 

    // Proportional scaling for adult aesthetic
    glScalef(0.135f, 0.162f, 0.145f);
    
    // Internal base model offset (centers the mesh)
    glTranslatef(0.0f, -0.2f, -0.5f);

    if (!isWireframe && !isShadowPass) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texHeadObj);
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glDisable(GL_TEXTURE_2D);
        // During shadows, we use the shadow color set by drawPlanarShadow()
    }
    
    drawHeadOBJGeometry();
    glPopMatrix();
    glPopAttrib();

    // --- RENDER HAIR SYSTEM ---
    // Removed isShadowPass guard to allow hair to cast shadows
    glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
    if (!isShadowPass) setupLightingHair();
    
    glPushMatrix();
    // PULLED DOWN: Seating the hair firmly on the forehead mesh (Z-UP)
    // Adjusting offset to pull the procedural skull (internal Z=0.38) onto the head
    glTranslatef(0.0f, -0.15f, -0.15f); 

    // Orientation: Face the same way as the mesh head
    // Changed to 0 degrees to align hair front with face front
    glRotatef(0.0f, 0.0f, 0.0f, 1.0f); 
    
    glScalef(0.80f, 0.85f, 0.85f);
    
    glDisable(GL_TEXTURE_2D);
    if (isWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    
    drawHairModel();
    glPopMatrix();
    glPopAttrib();

    glPopMatrix();
}

void drawCharacter()
{
    glPushMatrix();
    glTranslatef(charX, CHAR_Y, charZ);  // Move character in world
    glRotatef(charRotation, 0.0f, 1.0f, 0.0f); // Rotate character

    if (isShadowPass) {
        // SHADOW OPTIMIZATION: 
        // Use the real limbs and body for the correct animated shape, 
        // but completely skip the complex dress mesh to eliminate lagging.
        
        // ---- Body/Dress Proxy ----
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(180.0f, 0.0f, 0.0f, 1.0f); 
        drawBodyMesh(); // Torso
        drawHeadAndHair(); // Head shadow proxy
        
        // Super-fast cone to represent the dress skirt
        GLUquadric* q = gluNewQuadric();
        glTranslatef(0.0f, 0.0f, -0.4f);
        gluCylinder(q, 0.3f, 0.8f, 1.0f, 12, 1);
        gluDeleteQuadric(q);
        glPopMatrix();

        // ---- Legs ----
        glTranslatef(0.0f, -1.53f, 0.0f);   
        drawLeftLeg();
        drawRightLeg();

        // ---- Arms ----
        glTranslatef(0.0f, 1.53f, 0.0f);    
        drawLeftArm();
        drawRightArm();
    } else {
        // ---- Body (Z-up model, rotate so Z becomes world Y) ----
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(180.0f, 0.0f, 0.0f, 1.0f); // Reverse body direction to face camera
        if (isWireframe) {
            drawBodyMesh();
            drawDressMesh();
            drawHeadAndHair();
        } else if (cachedBodyList == 0) {
            cachedBodyList = glGenLists(1);
            glNewList(cachedBodyList, GL_COMPILE);
            drawBodyMesh();
            drawDressMesh();
            glEndList();
            glCallList(cachedBodyList);
            drawHeadAndHair();
        } else {
            glCallList(cachedBodyList);
            drawHeadAndHair();
        }
        glPopMatrix();

        // ---- Legs (Y-up, origin at hip Y=1.53, extend downward) ----
        glTranslatef(0.0f, -1.53f, 0.0f);   // re-origin to hip joint
        drawLeftLeg();
        drawRightLeg();

        // ---- Arms (Y-up, origin at shoulder; shoulder ≈ body Z=1.35 above hip) ----
        glTranslatef(0.0f, 1.53f, 0.0f);    // undo the leg offset
        drawLeftArm();
        drawRightArm();
    }

    glPopMatrix();
}

//================================================================
//  DISPLAY
//================================================================

void display()
{
    if (isWireframe) {
        glClearColor(0.04f, 0.04f, 0.07f, 1.0f); // near-black for wireframe contrast
    } else if (!isNight) {
        glClearColor(0.45f, 0.72f, 0.95f, 1.0f); // sky blue
    } else {
        glClearColor(0.02f, 0.02f, 0.12f, 1.0f); // dark navy
    }

    setupLighting(); // Update light intensities
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    updateCamera();
    
    float shakeX = 0, shakeY = 0, shakeZ = 0;
    if (screenShakeTimer > 0.0f) {
        shakeX = ((rand() % 100) / 50.0f - 1.0f) * 0.15f;
        shakeY = ((rand() % 100) / 50.0f - 1.0f) * 0.15f;
        shakeZ = ((rand() % 100) / 50.0f - 1.0f) * 0.15f;
    }

    gluLookAt(cameraX + charX + shakeX, cameraY + shakeY, cameraZ + charZ + shakeZ,
              charX + shakeX, 1.6 + shakeY, charZ + shakeZ,   // look at character neck/head height
              0, 1, 0);

    // Save camera orientation for world-aligned weapons
    glGetFloatv(GL_MODELVIEW_MATRIX, cameraViewMatrix);

    if (!isWireframe) {
        // Normal mode: draw sky, shadow, and textured character
        drawBackground();
        drawPlanarShadow();
        drawCharacter();
    } else {
        // ---- Wireframe / X-ray Mode (press I to toggle) ----
        // Background is already cleared to near-black via glClearColor above.
        // All glEnable(GL_TEXTURE_2D) calls in draw functions are guarded with
        // !isWireframe, so textures stay OFF throughout the character draw.
        // A pure emission material forces every fragment to the same bright cyan
        // regardless of internal glColor3f() calls (GL_COLOR_MATERIAL is off).

        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT | GL_CURRENT_BIT
                   | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);

        // Disable everything that could colour or modulate the output
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_BLEND);        // fully opaque — easiest to see

        // Wireframe polygon edges only
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.3f);

        // Force ALL fragments to a single bright colour via emission.
        // With GL_LIGHTING on, every light off, and only emission non-zero,
        // the rasterised colour = emission regardless of vertex colours.
        glEnable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
        GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat emit[4] = { 0.35f, 0.95f, 1.0f, 1.0f }; // vivid cyan, fully opaque
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  zero);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  zero);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emit);

        // Draw character as wireframe on the dark background
        drawCharacter();

        glPopAttrib();
    }
}

//================================================================
//  WinMain
//================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpfnWndProc   = WindowProcedure;
    wc.lpszClassName = WINDOW_TITLE;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc)) return false;

    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                             NULL, NULL, wc.hInstance, NULL);

    HDC hdc = GetDC(hWnd);
    initPixelFormat(hdc);
    HGLRC hglrc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, hglrc)) return false;

    initOpenGL();
    setupLighting();

    // Load all textures
    texGrass        = loadBMP("grass.bmp");
    texSkin         = loadBMP("skin.bmp");
    texFabric       = loadBMP("fabric.bmp");
    texGold         = loadBMP("gold.bmp");
    texWhiteSleeve  = loadBMP("white_sleeve.bmp");
    texWhiteLeather = loadBMP("white_leather.bmp");
    texDarkLeather  = loadBMP("dark_leather.bmp");
    texWeaponMetal  = loadBMP("metal.bmp");
    texWeaponWood   = loadBMP("wood.bmp");
    texWeaponChain  = loadBMP("chain.bmp");
    texFanWood      = loadBMP("fan_wood.bmp");
    texFan          = loadBMP("fan.bmp");
    texHeadObj      = loadBMP("head.bmp");

    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        updateAnimation();
        display();
        SwapBuffers(hdc);
    }

    UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return true;
}
//--------------------------------------------------------------------