#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable:4996)
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")

#define WINDOW_TITLE "Head Part 3 - Hair Model"

const float PI = 3.1415926535f;
const float TWO_PI = 6.283185307f;

float cameraAngle = 0.0f;
float cameraHeight = 0.55f;
float cameraDistance = 3.8f;
float cameraX, cameraY, cameraZ;

// ----------------------------------------------------------------
// Math helpers
// ----------------------------------------------------------------
struct Vec3 { float x, y, z; };

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

// Deterministic hash‑based random
float hf(int s) {
    s = (s ^ 61) ^ (s >> 16); s += (s << 3); s ^= (s >> 4);
    s *= 0x27d4eb2d; s ^= (s >> 15);
    return (float)(s & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}
float rr(int s, float lo, float hi) { return lo + hf(s) * (hi - lo); }

// ----------------------------------------------------------------
// Camera & window
// ----------------------------------------------------------------
void updateCamera() {
    cameraX = sinf(cameraAngle) * cameraDistance;
    cameraZ = cosf(cameraAngle) * cameraDistance;
    cameraY = cameraHeight;
}

LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE: PostQuitMessage(0); break;
        case VK_LEFT: cameraAngle -= 0.06f; break;
        case VK_RIGHT: cameraAngle += 0.06f; break;
        case VK_UP: cameraHeight += 0.18f; break;
        case VK_DOWN: cameraHeight -= 0.18f; break;
        case VK_ADD: case VK_OEM_PLUS: cameraDistance -= 0.2f; break;
        case VK_SUBTRACT: case VK_OEM_MINUS: cameraDistance += 0.2f; break;
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool initPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24;
    pfd.cAlphaBits = 8; pfd.iLayerType = PFD_MAIN_PLANE;
    int fmt = ChoosePixelFormat(hdc, &pfd);
    if (!fmt) return false;
    return SetPixelFormat(hdc, fmt, &pfd) == TRUE;
}

void initOpenGL() {
    glClearColor(0.10f, 0.10f, 0.12f, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 900.0 / 700.0, 0.1, 100);
    glMatrixMode(GL_MODELVIEW);
}

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

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
// Skull shape (ellipsoid the hair is built around)
// In this coordinate system: X=right, Y=forward(face), Z=up
// ----------------------------------------------------------------
const float SR_X = 0.48f;   // skull radius side
const float SR_Y = 0.50f;   // skull radius front-back
const float SR_Z = 0.56f;   // skull radius up
const float SC_Z = 0.42f;   // skull center Z (vertical)

// Get point on skull. theta=azimuth (0=right, PI/2=front, PI=left, 3PI/2=back), phi=polar from top
Vec3 skullPt(float theta, float phi) {
    return v3(SR_X * sinf(phi) * cosf(theta),
        SR_Y * sinf(phi) * sinf(theta),
        SC_Z + SR_Z * cosf(phi));
}

Vec3 skullNrm(float theta, float phi) {
    Vec3 p = skullPt(theta, phi);
    return v3norm(v3(p.x / (SR_X * SR_X), p.y / (SR_Y * SR_Y), (p.z - SC_Z) / (SR_Z * SR_Z)));
}

// ----------------------------------------------------------------
// Emit geometry
// ----------------------------------------------------------------
void emitQ(Vec3 p0, Vec3 n0, Vec3 p1, Vec3 n1, Vec3 p2, Vec3 n2, Vec3 p3, Vec3 n3) {
    glBegin(GL_QUADS);
    glNormal3f(n0.x, n0.y, n0.z); glVertex3f(p0.x, p0.y, p0.z);
    glNormal3f(n1.x, n1.y, n1.z); glVertex3f(p1.x, p1.y, p1.z);
    glNormal3f(n2.x, n2.y, n2.z); glVertex3f(p2.x, p2.y, p2.z);
    glNormal3f(n3.x, n3.y, n3.z); glVertex3f(p3.x, p3.y, p3.z);
    glEnd();
}

void emitT(Vec3 p0, Vec3 n0, Vec3 p1, Vec3 n1, Vec3 p2, Vec3 n2) {
    glBegin(GL_TRIANGLES);
    glNormal3f(n0.x, n0.y, n0.z); glVertex3f(p0.x, p0.y, p0.z);
    glNormal3f(n1.x, n1.y, n1.z); glVertex3f(p1.x, p1.y, p1.z);
    glNormal3f(n2.x, n2.y, n2.z); glVertex3f(p2.x, p2.y, p2.z);
    glEnd();
}

// ----------------------------------------------------------------
// Hair color helpers
// ----------------------------------------------------------------
void setHairColor(float shade, float tint) {
    // Base: vivid anime blue
    float r = (0.05f + 0.15f * tint) * shade;
    float g = (0.22f + 0.22f * tint) * shade;
    float b = (0.62f + 0.22f * tint) * shade;
    glColor3f(r, g, b);
}

void setHairColorBright(float shade, float tint) {
    float r = (0.18f + 0.25f * tint) * shade;
    float g = (0.42f + 0.20f * tint) * shade;
    float b = (0.80f + 0.15f * tint) * shade;
    glColor3f(r, g, b);
}

// ----------------------------------------------------------------
// HAIR PANEL: A wide, curved panel that conforms to the skull
// then flows outward. This is the core geometry primitive.
//
// Each panel is defined by:
//   - angular range on the skull (thetaStart..thetaEnd)
//   - phi on skull where it starts
//   - a set of "rings" that define the panel at different distances
//     from the head. The first rings conform to the skull, later
//     rings flow downward.
//   - widthSegs x lengthSegs subdivision
// ----------------------------------------------------------------

struct PanelRing {
    float offsetOut;    // outward from skull surface
    float offsetZ;      // vertical offset (down = negative)
    float widthMul;     // width multiplier (taper)
    float bulge;        // extra outward bulge for volume
};

struct HairPanel {
    float thetaStart, thetaEnd;  // angular range on skull
    float phiStart;              // where on skull this panel begins
    float phiConform;            // how far along skull the hair conforms before flowing
    PanelRing rings[10];
    int numRings;
    int widthSegs;               // across the panel
    int lengthSegs;              // along the panel
    float colorTint;             // 0..1 color variation
    float colorShade;            // brightness multiplier
    int bright;                  // use bright palette?
};

void drawPanel(const HairPanel& hp) {
    int wSegs = hp.widthSegs;
    int lSegs = hp.lengthSegs;

    // Build grid of points
    int cols = wSegs + 1;
    int rows = lSegs + 1;
    Vec3* pts = new Vec3[rows * cols];
    Vec3* nrm = new Vec3[rows * cols];

    for (int r = 0; r < rows; ++r) {
        float v = (float)r / (float)lSegs;  // 0 at root, 1 at tip

        // Interpolate ring parameters using Catmull-Rom
        float globalR = v * (float)(hp.numRings - 1);
        int ri = (int)globalR;
        if (ri >= hp.numRings - 1) ri = hp.numRings - 2;
        float rt = globalR - (float)ri;
        int r0 = ri - 1 < 0 ? 0 : ri - 1;
        int r1 = ri;
        int r2 = ri + 1 >= hp.numRings ? hp.numRings - 1 : ri + 1;
        int r3 = ri + 2 >= hp.numRings ? hp.numRings - 1 : ri + 2;

        float oOut = catRom(rt, hp.rings[r0].offsetOut, hp.rings[r1].offsetOut, hp.rings[r2].offsetOut, hp.rings[r3].offsetOut);
        float oZ = catRom(rt, hp.rings[r0].offsetZ, hp.rings[r1].offsetZ, hp.rings[r2].offsetZ, hp.rings[r3].offsetZ);
        float wMul = catRom(rt, hp.rings[r0].widthMul, hp.rings[r1].widthMul, hp.rings[r2].widthMul, hp.rings[r3].widthMul);
        float bulge = catRom(rt, hp.rings[r0].bulge, hp.rings[r1].bulge, hp.rings[r2].bulge, hp.rings[r3].bulge);

        for (int c = 0; c < cols; ++c) {
            float u = (float)c / (float)wSegs;  // 0..1 across width

            // Angular position on skull
            float theta = hp.thetaStart + (hp.thetaEnd - hp.thetaStart) * u;
            // The width narrows by wMul toward the tip
            float centerTheta = (hp.thetaStart + hp.thetaEnd) * 0.5f;
            theta = centerTheta + (theta - centerTheta) * wMul;

            // Phi: the panel conforms to skull up to phiConform, then extends
            float phi = hp.phiStart + hp.phiConform * clampf(v * 1.5f, 0, 1);

            // Get skull surface point and normal at this location
            Vec3 sp = skullPt(theta, phi);
            Vec3 sn = skullNrm(theta, phi);

            // The point starts on the skull then moves outward and downward
            float edgeFade = 1.0f - 2.0f * fabsf(u - 0.5f);
            float bulgeFactor = bulge * edgeFade * edgeFade;  // more bulge in center

            Vec3 p = v3add(sp, v3scale(sn, oOut + bulgeFactor));
            p.z += oZ;  // vertical displacement

            pts[r * cols + c] = p;
        }
    }

    // Compute normals via cross products of neighbors
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

    // Emit quads
    for (int r = 0; r < lSegs; ++r) {
        float v = (float)r / (float)lSegs;
        float shade = hp.colorShade * (1.0f - 0.30f * v);  // darken toward tips

        if (hp.bright) setHairColorBright(shade, hp.colorTint);
        else setHairColor(shade, hp.colorTint);

        for (int c = 0; c < wSegs; ++c) {
            int i00 = r * cols + c;
            int i10 = r * cols + c + 1;
            int i01 = (r + 1) * cols + c;
            int i11 = (r + 1) * cols + c + 1;

            // Front face
            emitQ(pts[i00], nrm[i00], pts[i10], nrm[i10], pts[i11], nrm[i11], pts[i01], nrm[i01]);
            // Back face (flipped normal)
            Vec3 fn00 = v3scale(nrm[i00], -1);
            Vec3 fn10 = v3scale(nrm[i10], -1);
            Vec3 fn11 = v3scale(nrm[i11], -1);
            Vec3 fn01 = v3scale(nrm[i01], -1);
            float bshade = shade * 0.65f;
            if (hp.bright) setHairColorBright(bshade, hp.colorTint);
            else setHairColor(bshade, hp.colorTint);
            emitQ(pts[i01], fn01, pts[i11], fn11, pts[i10], fn10, pts[i00], fn00);

            // Restore front face color
            if (hp.bright) setHairColorBright(shade, hp.colorTint);
            else setHairColor(shade, hp.colorTint);
        }
    }

    delete[] pts;
    delete[] nrm;
}

// ----------------------------------------------------------------
// Build the full hair
// ----------------------------------------------------------------

// BACK HAIR: Large overlapping panels covering the entire back
// Tips curve slightly upward at the end
void drawBackHair() {
    const int numPanels = 10;
    for (int i = 0; i < numPanels; ++i) {
        float centerAngle = PI * 1.5f + PI * 0.85f * ((float)i / (numPanels - 1) - 0.5f);
        float halfWidth = PI / (float)numPanels * 1.5f; // overlapping

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.12f + rr(i * 31, 0, 0.03f);
        hp.phiConform = PI * 0.40f + rr(i * 37, -0.05f, 0.05f);
        hp.widthSegs = 4;
        hp.lengthSegs = 10;
        hp.colorTint = rr(i * 41, 0.0f, 0.8f);
        hp.colorShade = 0.85f + rr(i * 43, 0, 0.15f);
        hp.bright = 0;

        float depthVar = rr(i * 47, 0, 0.02f);
        float lengthVar = rr(i * 53, -0.15f, 0.20f);
        float totalDrop = 1.4f + lengthVar;

        hp.numRings = 8;
        hp.rings[0] = { 0.04f + depthVar, 0.0f,    1.0f,  0.02f };
        hp.rings[1] = { 0.05f + depthVar, -0.05f,   1.0f,  0.04f };
        hp.rings[2] = { 0.06f + depthVar, -0.18f,   0.95f, 0.05f };
        hp.rings[3] = { 0.08f + depthVar, -0.38f,   0.88f, 0.04f };
        hp.rings[4] = { 0.10f + depthVar, -0.60f,   0.78f, 0.03f };
        hp.rings[5] = { 0.10f + depthVar, -0.85f,   0.65f, 0.02f };
        hp.rings[6] = { 0.08f + depthVar, -totalDrop * 0.88f, 0.40f, 0.01f };
        hp.rings[7] = { 0.05f + depthVar, -totalDrop + 0.08f, 0.20f, 0.00f };  // tip curves UP

        drawPanel(hp);
    }
}

// SIDE HAIR: Panels flowing down alongside the face
void drawSideHair() {
    for (int side = 0; side < 2; ++side) {
        float sideSign = (side == 0) ? 1.0f : -1.0f;
        const int numPanels = 8;

        for (int i = 0; i < numPanels; ++i) {
            float baseAngle = (side == 0) ? (PI * 0.0f) : (PI * 1.0f);
            float spread = PI * 0.55f * ((float)i / (numPanels - 1) - 0.5f);
            float centerAngle = baseAngle + spread;
            float halfWidth = PI / (float)(numPanels) * 1.2f;

            HairPanel hp;
            hp.thetaStart = centerAngle - halfWidth;
            hp.thetaEnd = centerAngle + halfWidth;
            hp.phiStart = PI * 0.04f + rr(i * 59 + side * 500, 0, 0.04f);
            hp.phiConform = PI * 0.38f + rr(i * 61 + side * 500, -0.04f, 0.04f);
            hp.widthSegs = 5;
            hp.lengthSegs = 14;
            hp.colorTint = rr(i * 67 + side * 500, 0.1f, 0.9f);
            hp.colorShade = 0.90f + rr(i * 71 + side * 500, 0, 0.10f);
            hp.bright = 0;

            float dv = rr(i * 73 + side * 500, 0, 0.02f);
            float totalDrop = 1.25f + rr(i * 79 + side * 500, -0.15f, 0.25f);

            hp.numRings = 7;
            hp.rings[0] = { 0.04f + dv, 0.0f,   1.0f,  0.03f };
            hp.rings[1] = { 0.05f + dv, -0.06f,  1.0f,  0.05f };
            hp.rings[2] = { 0.07f + dv, -0.22f,  0.92f, 0.04f };
            hp.rings[3] = { 0.09f + dv, -0.45f,  0.82f, 0.03f };
            hp.rings[4] = { 0.09f + dv, -0.72f,  0.68f, 0.02f };
            hp.rings[5] = { 0.07f + dv, -totalDrop * 0.85f, 0.42f, 0.01f };
            hp.rings[6] = { 0.04f + dv, -totalDrop,         0.18f, 0.00f };

            drawPanel(hp);
        }
    }
}

// FRONT BANGS: Short swept bangs
void drawBangs() {
    // Left bangs
    const int numLeft = 4;
    for (int i = 0; i < numLeft; ++i) {
        float t = (float)i / (numLeft - 1);
        float centerAngle = PI * 0.5f + PI * 0.10f * (t - 0.5f) - PI * 0.06f;
        float halfWidth = PI * 0.06f + PI * 0.02f * t;

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.02f + rr(i * 83, 0, 0.02f);
        hp.phiConform = PI * 0.22f + rr(i * 89, -0.02f, 0.02f);
        hp.widthSegs = 3;
        hp.lengthSegs = 6;
        hp.colorTint = rr(i * 97, 0.1f, 0.6f);
        hp.colorShade = 0.85f + rr(i * 101, 0, 0.10f);
        hp.bright = 0;

        float bangLen = 0.18f + 0.12f * t + rr(i * 103, -0.04f, 0.05f);

        hp.numRings = 4;
        hp.rings[0] = { 0.05f, 0.0f,   1.0f,  0.02f };
        hp.rings[1] = { 0.07f, -0.04f,  1.0f,  0.03f };
        hp.rings[2] = { 0.08f, -bangLen * 0.6f,  0.75f, 0.02f };
        hp.rings[3] = { 0.05f, -bangLen,          0.30f, 0.00f };

        drawPanel(hp);
    }

    // Right bangs
    const int numRight = 4;
    for (int i = 0; i < numRight; ++i) {
        float t = (float)i / (numRight - 1);
        float centerAngle = PI * 0.5f + PI * 0.10f * (t - 0.5f) + PI * 0.06f;
        float halfWidth = PI * 0.06f + PI * 0.02f * t;

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.02f + rr(i * 107, 0, 0.02f);
        hp.phiConform = PI * 0.22f + rr(i * 109, -0.02f, 0.02f);
        hp.widthSegs = 3;
        hp.lengthSegs = 6;
        hp.colorTint = rr(i * 113 + 1000, 0.1f, 0.6f);
        hp.colorShade = 0.85f + rr(i * 127 + 1000, 0, 0.10f);
        hp.bright = 0;

        float bangLen = 0.18f + 0.12f * t + rr(i * 131, -0.04f, 0.05f);

        hp.numRings = 4;
        hp.rings[0] = { 0.05f, 0.0f,   1.0f,  0.02f };
        hp.rings[1] = { 0.07f, -0.04f,  1.0f,  0.03f };
        hp.rings[2] = { 0.08f, -bangLen * 0.6f,  0.75f, 0.02f };
        hp.rings[3] = { 0.05f, -bangLen,          0.30f, 0.00f };

        drawPanel(hp);
    }
}

// TOP HAIR: Wide panels covering the crown of the head
void drawTopHair() {
    const int numPanels = 8;
    for (int i = 0; i < numPanels; ++i) {
        float centerAngle = TWO_PI * ((float)i / numPanels);
        float halfWidth = PI / (float)numPanels * 1.6f;

        if (centerAngle > PI * 0.3f && centerAngle < PI * 0.7f) continue;

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.01f;
        hp.phiConform = PI * 0.22f;
        hp.widthSegs = 4;
        hp.lengthSegs = 6;
        hp.colorTint = rr(i * 137, 0.3f, 0.7f);
        hp.colorShade = 0.90f + rr(i * 139, 0, 0.10f);
        hp.bright = 0;

        hp.numRings = 4;
        float topLen = 0.25f + rr(i * 141, -0.10f, 0.15f);
        hp.rings[0] = { 0.04f, 0.02f,  1.0f,  0.01f };
        hp.rings[1] = { 0.05f, 0.0f,   1.0f,  0.03f };
        hp.rings[2] = { 0.06f, -topLen * 0.35f,  0.95f, 0.03f };
        hp.rings[3] = { 0.05f, -topLen,          0.70f, 0.00f };

        drawPanel(hp);
    }
}

// ACCENT WISPS: Thin panels for extra detail (back only)
void drawAccentStrands() {
    const int numStrands = 12;
    for (int i = 0; i < numStrands; ++i) {
        float angle = PI * 1.0f + PI * 1.0f * hf(i * 149);  // back half only
        float halfWidth = PI * 0.03f + rr(i * 151, 0, 0.015f);

        HairPanel hp;
        hp.thetaStart = angle - halfWidth;
        hp.thetaEnd = angle + halfWidth;
        hp.phiStart = PI * 0.04f + rr(i * 157, 0, 0.04f);
        hp.phiConform = PI * 0.35f + rr(i * 163, -0.04f, 0.04f);
        hp.widthSegs = 2;
        hp.lengthSegs = 8;
        hp.colorTint = rr(i * 167, 0, 1);
        hp.colorShade = 0.80f + rr(i * 173, 0, 0.20f);
        hp.bright = (hf(i * 179) > 0.6f) ? 1 : 0;

        float strandLen = 0.8f + rr(i * 181, -0.2f, 0.5f);
        float dv = rr(i * 191, 0, 0.02f);

        hp.numRings = 5;
        hp.rings[0] = { 0.055f + dv, 0.0f,   1.0f,  0.01f };
        hp.rings[1] = { 0.065f + dv, -0.06f,  1.0f,  0.02f };
        hp.rings[2] = { 0.075f + dv, -0.30f,  0.75f, 0.01f };
        hp.rings[3] = { 0.06f + dv,  -strandLen * 0.8f, 0.40f, 0.00f };
        hp.rings[4] = { 0.04f + dv,  -strandLen + 0.06f, 0.15f, 0.00f };  // slight upward tip

        drawPanel(hp);
    }
}

// INNER SHELL: A continuous hair shell hugging the skull
// Covers any gaps between panels
void drawInnerShell() {
    const int stacks = 20;
    const int slices = 32;
    const float shellOut = 0.035f;

    for (int i = 0; i < stacks; ++i) {
        float phi0 = PI * 0.01f + PI * 0.52f * ((float)i / stacks);
        float phi1 = PI * 0.01f + PI * 0.52f * ((float)(i + 1) / stacks);

        for (int j = 0; j < slices; ++j) {
            float th0 = TWO_PI * ((float)j / slices);
            float th1 = TWO_PI * ((float)(j + 1) / slices);

            Vec3 n00 = skullNrm(th0, phi0);
            Vec3 n10 = skullNrm(th0, phi1);
            Vec3 n11 = skullNrm(th1, phi1);
            Vec3 n01 = skullNrm(th1, phi0);

            Vec3 p00 = v3add(skullPt(th0, phi0), v3scale(n00, shellOut));
            Vec3 p10 = v3add(skullPt(th0, phi1), v3scale(n10, shellOut));
            Vec3 p11 = v3add(skullPt(th1, phi1), v3scale(n11, shellOut));
            Vec3 p01 = v3add(skullPt(th1, phi0), v3scale(n01, shellOut));

            float shade = 0.75f + 0.12f * cosf(phi0 * 2);
            setHairColor(shade, 0.3f + 0.2f * cosf(th0 * 2));
            emitQ(p00, n00, p10, n10, p11, n11, p01, n01);
        }
    }
}

// LONG SIDE FRINGE: Extra-long strands framing the face
void drawSideFringe() {
    for (int side = 0; side < 2; ++side) {
        float baseAngle = (side == 0) ? (PI * 0.22f) : (PI * 0.78f);
        const int numPanels = 4;

        for (int i = 0; i < numPanels; ++i) {
            float offset = PI * 0.06f * ((float)i / (numPanels - 1) - 0.5f);
            float centerAngle = baseAngle + offset;
            float halfWidth = PI * 0.04f + rr(i * 193 + side * 700, 0, 0.02f);

            HairPanel hp;
            hp.thetaStart = centerAngle - halfWidth;
            hp.thetaEnd = centerAngle + halfWidth;
            hp.phiStart = PI * 0.04f + rr(i * 197 + side * 700, 0, 0.02f);
            hp.phiConform = PI * 0.32f + rr(i * 199 + side * 700, -0.03f, 0.03f);
            hp.widthSegs = 4;
            hp.lengthSegs = 14;
            hp.colorTint = rr(i * 211 + side * 700, 0.3f, 0.8f);
            hp.colorShade = 0.92f + rr(i * 223 + side * 700, 0, 0.08f);
            hp.bright = 1;

            float fringeLen = 1.1f + rr(i * 227 + side * 700, -0.1f, 0.2f);
            float dv = rr(i * 229 + side * 700, 0, 0.015f);

            hp.numRings = 7;
            hp.rings[0] = { 0.05f + dv, 0.0f,   1.0f,  0.03f };
            hp.rings[1] = { 0.07f + dv, -0.04f,  1.0f,  0.05f };
            hp.rings[2] = { 0.09f + dv, -0.16f,  0.92f, 0.04f };
            hp.rings[3] = { 0.10f + dv, -0.35f,  0.80f, 0.03f };
            hp.rings[4] = { 0.09f + dv, -0.60f,  0.62f, 0.02f };
            hp.rings[5] = { 0.07f + dv, -fringeLen * 0.85f, 0.38f, 0.01f };
            hp.rings[6] = { 0.04f + dv, -fringeLen,         0.15f, 0.00f };

            drawPanel(hp);
        }
    }
}

// Thin overlay strands (back only, reduced count)
void drawLayerStrands() {
    const int numPanels = 10;
    for (int i = 0; i < numPanels; ++i) {
        float angle = PI * 1.0f + PI * 1.0f * ((float)i / (numPanels - 1));
        float halfWidth = PI * 0.04f + rr(i * 233, 0, 0.02f);

        HairPanel hp;
        hp.thetaStart = angle - halfWidth;
        hp.thetaEnd = angle + halfWidth;
        hp.phiStart = PI * 0.06f + rr(i * 239, 0, 0.04f);
        hp.phiConform = PI * 0.36f + rr(i * 241, -0.04f, 0.04f);
        hp.widthSegs = 2;
        hp.lengthSegs = 8;
        hp.colorTint = rr(i * 251, 0, 1);
        hp.colorShade = 0.82f + rr(i * 257, 0, 0.18f);
        hp.bright = (hf(i * 263) > 0.5f) ? 1 : 0;

        float totalDrop = 1.1f + rr(i * 269, -0.2f, 0.3f);
        float dv = rr(i * 271, 0, 0.015f);

        hp.numRings = 6;
        hp.rings[0] = { 0.05f + dv, 0.0f,   1.0f,  0.015f };
        hp.rings[1] = { 0.06f + dv, -0.05f,  1.0f,  0.025f };
        hp.rings[2] = { 0.07f + dv, -0.22f,  0.85f, 0.02f };
        hp.rings[3] = { 0.08f + dv, -0.50f,  0.65f, 0.01f };
        hp.rings[4] = { 0.06f + dv, -totalDrop * 0.88f, 0.35f, 0.005f };
        hp.rings[5] = { 0.04f + dv, -totalDrop + 0.06f,  0.15f, 0.00f };  // slight upward tip

        drawPanel(hp);
    }
}

// ----------------------------------------------------------------
// Master draw call
// ----------------------------------------------------------------
void drawHairModel() {
    glDisable(GL_CULL_FACE);
    drawTopHair();       // Crown coverage
    drawBackHair();      // Main flowing back hair
    drawLayerStrands();  // Overlay detail
    drawBangs();         // Short front bangs
    drawAccentStrands(); // Detail wisps (back only)
}



// ----------------------------------------------------------------
// Display
// ----------------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    updateCamera();
    gluLookAt(cameraX, cameraY, cameraZ, 0, 0.20f, 0.10f, 0, 1, 0);

    setupLighting();

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    drawHairModel();
    glPopMatrix();

    SwapBuffers(wglGetCurrentDC());
}

// ----------------------------------------------------------------
// WinMain
// ----------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = WindowProcedure;
    wc.lpszClassName = WINDOW_TITLE;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    if (!RegisterClassEx(&wc)) return false;

    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700, NULL, NULL, wc.hInstance, NULL);

    HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return false;
    HGLRC hglrc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, hglrc)) return false;

    initOpenGL();
    setupLighting();

    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        display();
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hWnd, hdc);
    UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return true;
}
