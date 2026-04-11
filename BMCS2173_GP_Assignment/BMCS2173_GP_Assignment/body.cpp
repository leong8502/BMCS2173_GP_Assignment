#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma warning(disable:4996)
#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "GLU32.lib")

#define WINDOW_TITLE "Body Part"

//--------------------------------
// Camera variables
//--------------------------------

float cameraAngle = 0.0f;
float cameraHeight = 1.0f;
float cameraDistance = 4.0f;
float cameraX, cameraY, cameraZ;

//--------------------------------
// Texture Variables
//--------------------------------

GLuint texSkin;
GLuint texFabric;
GLuint texGold;
GLuint texWhiteLeather;
GLuint texDarkLeather;

GLuint loadBMP(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;

    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) { fclose(file); return 0; }
    if (header[0] != 'B' || header[1] != 'M') { fclose(file); return 0; }

    int width = *(int*)&(header[18]);
    int height = *(int*)&(header[22]);
    int imageSize = *(int*)&(header[34]);
    if (imageSize == 0) imageSize = width * height * 3;

    unsigned char* data = new unsigned char[imageSize];
    fread(data, 1, imageSize, file);
    fclose(file);

    // Swap BGR to RGB
    for (int i = 0; i < imageSize; i += 3) {
        unsigned char tmp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = tmp;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return tex;
}

void updateCamera()
{
    cameraX = sin(cameraAngle) * cameraDistance;
    cameraZ = cos(cameraAngle) * cameraDistance;
    cameraY = cameraHeight;
}

LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            break;

        case VK_LEFT:
            cameraAngle -= 0.05f;
            break;
        case VK_RIGHT:
            cameraAngle += 0.05f;
            break;
        case VK_UP:
            cameraHeight += 0.3f;
            break;
        case VK_DOWN:
            cameraHeight -= 0.3f;
            break;
        case VK_ADD:
        case VK_OEM_PLUS:
            cameraDistance -= 0.3f;
            break;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            cameraDistance += 0.3f;
            break;
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.cAlphaBits = 8;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 0;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;

    int n = ChoosePixelFormat(hdc, &pfd);
    if (SetPixelFormat(hdc, n, &pfd)) return true;
    else return false;
}

void initOpenGL()
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // background color
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0 / 600.0, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
}

void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat ambientLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

    GLfloat lightPosition[] = { 3.0f, 5.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}

// Cubic Catmull-Rom interpolation for smooth body curves
float spline(float t, float p0, float p1, float p2, float p3) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * (
        (2.0f * p1) +
        (-p0 + p2) * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
}

// Body Keyframes
struct CrossSection {
    float z;
    float rx;
    float ry;
    float yOffset;
};

#define SEC_COUNT 10
CrossSection sections[SEC_COUNT] = {
    {-0.2f,  0.42f, 0.26f,  0.0f},   // Extra section for spline margin
    {0.0f,   0.45f, 0.28f,  0.0f},   // Hips / Crotch
    {0.3f,   0.47f, 0.32f, -0.06f},  // High hips / glutes
    {0.6f,   0.31f, 0.21f,  0.02f},	 // Waist
    {0.9f,   0.34f, 0.24f,  0.08f},  // Underbust
    {1.15f,  0.40f, 0.26f,  0.10f},  // Chest Level
    {1.35f,  0.46f, 0.21f,  0.03f},  // Shoulders
    {1.42f,  0.22f, 0.18f,  0.00f},  // Trapezius / Neck Base (Smooth transition to body)
    {1.58f,  0.13f, 0.13f, -0.04f},  // Upper Neck Straight Column
    {1.80f,  0.13f, 0.13f, -0.04f}   // Extra margin
};

void getTorsoBaseRadius(float z, float& rx, float& ry, float& yOfs) {
    // Add clamp
    if (z <= sections[1].z) {
        rx = sections[1].rx; ry = sections[1].ry; yOfs = sections[1].yOffset;
        return;
    }
    if (z >= sections[SEC_COUNT - 2].z) {
        rx = sections[SEC_COUNT - 2].rx; ry = sections[SEC_COUNT - 2].ry; yOfs = sections[SEC_COUNT - 2].yOffset;
        return;
    }

    int i = 1;
    for (; i < SEC_COUNT - 2; i++) {
        if (z <= sections[i + 1].z) break;
    }

    float t = (z - sections[i].z) / (sections[i + 1].z - sections[i].z);

    rx = spline(t, sections[i - 1].rx, sections[i].rx, sections[i + 1].rx, sections[i + 2].rx);
    ry = spline(t, sections[i - 1].ry, sections[i].ry, sections[i + 1].ry, sections[i + 2].ry);
    yOfs = spline(t, sections[i - 1].yOffset, sections[i].yOffset, sections[i + 1].yOffset, sections[i + 2].yOffset);
}

void getTorsoVertex(float z, float theta, float& px, float& py, float& pz) {
    float rx, ry, yOfs;
    getTorsoBaseRadius(z, rx, ry, yOfs);

    // Add breast bumps
    // 2D Gaussian bumps symmetrically placed
    float bump = 0.0f;
    float maxBump = 0.12f;
    float spreadZ = 0.02f;
    float spreadTheta = 0.12f;

    // Normalize theta to 0..2PI
    while (theta < 0) theta += 2.0f * 3.14159f;
    while (theta > 2.0f * 3.14159f) theta -= 2.0f * 3.14159f;

    float breastZ = 1.1f;
    float thetaLeft = 3.14159f * 0.35f; // Front-left
    float thetaRight = 3.14159f * 0.65f; // Front-right

    // Left breast
    float distSqL = pow(z - breastZ, 2) / spreadZ + pow(theta - thetaLeft, 2) / spreadTheta;
    bump += maxBump * exp(-distSqL);

    // Right breast
    float distSqR = pow(z - breastZ, 2) / spreadZ + pow(theta - thetaRight, 2) / spreadTheta;
    bump += maxBump * exp(-distSqR);

    // Cleavage indentation (definition between breasts)
    float cleavageDepth = 0.06f; // Increased from 0.04f
    float distSqCleavage = pow(z - breastZ, 2) / 0.03f + pow(theta - 3.14159f * 0.5f, 2) / 0.01f; // Sharpened
    bump -= cleavageDepth * exp(-distSqCleavage);

    // Glutes bump for back (theta around 1.35PI and 1.65PI)
    float gluteMaxBump = 0.005f; // Reduced significantly to make buttocks smaller
    float gluteZ = 0.25f;
    float thetaGluteL = 3.14159f * 1.35f;
    float thetaGluteR = 3.14159f * 1.65f;
    float gluteDistSqL = pow(z - gluteZ, 2) / 0.03f + pow(theta - thetaGluteL, 2) / 0.15f;
    float gluteDistSqR = pow(z - gluteZ, 2) / 0.03f + pow(theta - thetaGluteR, 2) / 0.15f;
    bump += gluteMaxBump * exp(-gluteDistSqL);
    bump += gluteMaxBump * exp(-gluteDistSqR);

    // Belly slight roundness
    float bellyBump = 0.03f * exp(-(pow(z - 0.55f, 2) / 0.04f + pow(theta - 3.14159f * 0.5f, 2) / 0.2f));
    bump += bellyBump;

    // Scale radius by bump
    rx += bump;
    ry += bump;

    px = rx * cos(theta);
    py = ry * sin(theta) + yOfs;
    pz = z;
}

void getTorsoNormal(float z, float theta, float& nx, float& ny, float& nz) {
    // We compute normal via central differences to account for all bumps automatically
    float epsZ = 0.01f;
    float epsT = 0.05f;

    float px1, py1, pz1;
    float px2, py2, pz2;
    float px3, py3, pz3;
    float px4, py4, pz4;

    getTorsoVertex(z + epsZ, theta, px1, py1, pz1);
    getTorsoVertex(z - epsZ, theta, px2, py2, pz2);
    getTorsoVertex(z, theta + epsT, px3, py3, pz3);
    getTorsoVertex(z, theta - epsT, px4, py4, pz4);

    // Tangents
    float tx1 = px1 - px2, ty1 = py1 - py2, tz1 = pz1 - pz2;
    float tx2 = px3 - px4, ty2 = py3 - py4, tz2 = pz3 - pz4;

    // Cross Product (tx2 x tx1 because of the parameterization direction)
    nx = ty2 * tz1 - tz2 * ty1;
    ny = tz2 * tx1 - tx2 * tz1;
    nz = tx2 * ty1 - ty2 * tx1;

    float len = sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0) { nx /= len; ny /= len; nz /= len; }
}

void getDressTorsoVertex(float z, float theta, float offset, float& px, float& py, float& pz) {
    float rx, ry, yOfs;
    getTorsoBaseRadius(z, rx, ry, yOfs);

    float bump = 0.0f;
    float maxBump = 0.12f;
    float spreadZ = 0.02f;
    float spreadTheta = 0.12f;

    float normTheta = theta;
    while (normTheta < 0) normTheta += 2.0f * 3.14159f;
    while (normTheta > 2.0f * 3.14159f) normTheta -= 2.0f * 3.14159f;

    float breastZ = 1.1f;
    float thetaLeft = 3.14159f * 0.35f;
    float thetaRight = 3.14159f * 0.65f;
    float distSqL = pow(z - breastZ, 2) / spreadZ + pow(normTheta - thetaLeft, 2) / spreadTheta;
    bump += maxBump * exp(-distSqL);
    float distSqR = pow(z - breastZ, 2) / spreadZ + pow(normTheta - thetaRight, 2) / spreadTheta;
    bump += maxBump * exp(-distSqR);
    float cleavageDepth = 0.06f; 
    float distSqCleavage = pow(z - breastZ, 2) / 0.03f + pow(normTheta - 3.14159f * 0.5f, 2) / 0.01f;
    bump -= cleavageDepth * exp(-distSqCleavage);
    float gluteMaxBump = 0.005f; 
    float gluteZ = 0.25f;
    float thetaGluteL = 3.14159f * 1.35f;
    float thetaGluteR = 3.14159f * 1.65f;
    float gluteDistSqL = pow(z - gluteZ, 2) / 0.03f + pow(normTheta - thetaGluteL, 2) / 0.15f;
    float gluteDistSqR = pow(z - gluteZ, 2) / 0.03f + pow(normTheta - thetaGluteR, 2) / 0.15f;
    bump += gluteMaxBump * exp(-gluteDistSqL);
    bump += gluteMaxBump * exp(-gluteDistSqR);
    float bellyBump = 0.03f * exp(-(pow(z - 0.55f, 2) / 0.04f + pow(normTheta - 3.14159f * 0.5f, 2) / 0.2f));
    bump += bellyBump;

    rx += bump + offset;
    ry += bump + offset;

    px = rx * cos(theta);
    py = ry * sin(theta) + yOfs;
    pz = z;
}

void getDressTorsoNormal(float z, float theta, float offset, float& nx, float& ny, float& nz) {
    float epsZ = 0.01f;
    float epsT = 0.05f;
    float px1, py1, pz1, px2, py2, pz2, px3, py3, pz3, px4, py4, pz4;
    getDressTorsoVertex(z + epsZ, theta, offset, px1, py1, pz1);
    getDressTorsoVertex(z - epsZ, theta, offset, px2, py2, pz2);
    getDressTorsoVertex(z, theta + epsT, offset, px3, py3, pz3);
    getDressTorsoVertex(z, theta - epsT, offset, px4, py4, pz4);
    float tx1 = px1 - px2, ty1 = py1 - py2, tz1 = pz1 - pz2;
    float tx2 = px3 - px4, ty2 = py3 - py4, tz2 = pz3 - pz4;
    nx = ty2 * tz1 - tz2 * ty1;
    ny = tz2 * tx1 - tx2 * tz1;
    nz = tx2 * ty1 - ty2 * tx1;
    float len = sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0) { nx /= len; ny /= len; nz /= len; }
}

void drawDressMesh() {
    glDisable(GL_TEXTURE_2D);

    int slices = 60;

    // 1. White Top Bodice (Scooped top exposing shoulders & cleavage)
    int topStacks = 15;
    for (int i = 0; i < topStacks; i++) {
        float f1 = (float)i / topStacks;
        float f2 = (float)(i + 1) / topStacks;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float dipTh = theta - 3.14159f * 0.5f;
            if (dipTh < -3.14159f) dipTh += 2.0f * 3.14159f;
            if (dipTh > 3.14159f) dipTh -= 2.0f * 3.14159f;
            
            // Corset top edge dips down heavily in front
            float corsetTop = 0.95f - 0.20f * exp(-pow(dipTh, 2) / 0.15f);
            float topZ1 = corsetTop - 0.05f; // Bodice goes slightly below the corset edge

            // Top boundary of white fabric - M shape exposing shoulders
            float zTopMax = 1.05f 
                + 0.18f * exp(-pow(fabs(dipTh) - 0.5f, 2) / 0.08f) 
                + 0.05f * exp(-pow(dipTh, 2) / 0.05f) 
                + 0.10f * exp(-pow(fabs(dipTh) - 3.14159f, 2) / 1.0f);

            // Avoid inverted fabric if bottom Z crosses top Z
            if(topZ1 > zTopMax - 0.02f) topZ1 = zTopMax - 0.02f;

            float z1 = topZ1 + f1 * (zTopMax - topZ1);
            float z2 = topZ1 + f2 * (zTopMax - topZ1);

            float px1, py1, pz1, nx1, ny1, nz1;
            float px2, py2, pz2, nx2, ny2, nz2;
            getDressTorsoVertex(z2, theta, 0.015f, px2, py2, pz2);
            getDressTorsoNormal(z2, theta, 0.015f, nx2, ny2, nz2);
            getDressTorsoVertex(z1, theta, 0.015f, px1, py1, pz1);
            getDressTorsoNormal(z1, theta, 0.015f, nx1, ny1, nz1);

            // Brighten up the shade slightly to avoid grayness
            float shade = 0.95f + 0.1f * cos(theta);
            if (shade > 1.0f) shade = 1.0f;
            
            // Gradient:
            // From middle to top: pure white
            // Bottom: saturated light blue
            auto applyGradientColor = [&](float f) {
                float t = f * 1.5f; // Push white downwards
                if (t > 1.0f) t = 1.0f;
                
                float r = 0.40f + t * (1.0f - 0.40f);
                float g = 0.65f + t * (1.0f - 0.65f);
                float b = 1.0f; // Keep B=1.0 for vibrant blue
                glColor3f(r * shade, g * shade, b * shade);
            };

            applyGradientColor(f2);
            glNormal3f(nx2, ny2, nz2); glVertex3f(px2, py2, pz2);
            
            applyGradientColor(f1);
            glNormal3f(nx1, ny1, nz1); glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }

    // 2. Dark Blue Corset/Midriff (Irregular curves with dynamic shading)
    int corsetStacks = 12;
    for (int i = 0; i < corsetStacks; i++) {
        float f1 = (float)i / corsetStacks;
        float f2 = (float)(i + 1) / corsetStacks;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float dipTh = theta - 3.14159f * 0.5f;
            if (dipTh < -3.14159f) dipTh += 2.0f * 3.14159f;
            if (dipTh > 3.14159f) dipTh -= 2.0f * 3.14159f;
            
            float corsetTop = 0.95f - 0.20f * exp(-pow(dipTh, 2) / 0.15f);
            float corsetBot = 0.52f + 0.12f * exp(-pow(dipTh, 2) / 0.15f); // Front curves up slightly
            
            float z1 = corsetBot + f1 * (corsetTop - corsetBot);
            float z2 = corsetBot + f2 * (corsetTop - corsetBot);

            float px1, py1, pz1, nx1, ny1, nz1;
            float px2, py2, pz2, nx2, ny2, nz2;
            getDressTorsoVertex(z2, theta, 0.025f, px2, py2, pz2); 
            getDressTorsoNormal(z2, theta, 0.025f, nx2, ny2, nz2);
            getDressTorsoVertex(z1, theta, 0.025f, px1, py1, pz1);
            getDressTorsoNormal(z1, theta, 0.025f, nx1, ny1, nz1);

            // Shading variation for non-flat look
            float depthShade = 0.5f + 0.5f * f1; // Darker at the bottom, lighter at the top
            float contourShade = 0.85f + 0.15f * cos(theta * 2.0f); // Enhances 3D roundness
            float shade = depthShade * contourShade;
            glColor3f(0.04f * shade, 0.15f * shade, 0.45f * shade); // Rich blue

            glNormal3f(nx2, ny2, nz2); glVertex3f(px2, py2, pz2);
            glNormal3f(nx1, ny1, nz1); glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }

    // 3. Gold Belts & Trims (Now perfectly tracing the irregular edges!)
    auto drawIrregularBelt = [&](bool isTopBelt) {
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float dipTh = theta - 3.14159f * 0.5f;
            if (dipTh < -3.14159f) dipTh += 2.0f * 3.14159f;
            if (dipTh > 3.14159f) dipTh -= 2.0f * 3.14159f;
            
            float borderZ = isTopBelt ? 
                (0.95f - 0.20f * exp(-pow(dipTh, 2) / 0.15f)) : 
                (0.52f + 0.12f * exp(-pow(dipTh, 2) / 0.15f));
                
            float z1 = borderZ - 0.015f;
            float z2 = borderZ + 0.015f;
            
            float px1, py1, pz1, nx1, ny1, nz1;
            float px2, py2, pz2, nx2, ny2, nz2;
            getDressTorsoVertex(z2, theta, 0.035f, px2, py2, pz2);
            getDressTorsoNormal(z2, theta, 0.035f, nx2, ny2, nz2);
            getDressTorsoVertex(z1, theta, 0.035f, px1, py1, pz1);
            getDressTorsoNormal(z1, theta, 0.035f, nx1, ny1, nz1);

            float shade = 0.85f + 0.15f * cos(theta); // Specular highlight fading
            glColor3f(0.9f * shade, 0.75f * shade, 0.2f * shade);

            glNormal3f(nx2, ny2, nz2); glVertex3f(px2, py2, pz2);
            glNormal3f(nx1, ny1, nz1); glVertex3f(px1, py1, pz1);
        }
        glEnd();
    };

    drawIrregularBelt(false); // Lower irregular belt
    drawIrregularBelt(true);  // Upper irregular belt
    
    // --- Detailed 3D Center Emblem & Filigree ---
    glColor3f(0.9f, 0.75f, 0.2f); // Shiny Gold
    float cZ = 0.72f; 
    float cTheta = 3.14159f * 0.5f; 
    float pTop[3], pBot[3], pL[3], pR[3], pC[3];
    getDressTorsoVertex(0.88f, cTheta, 0.04f, pTop[0], pTop[1], pTop[2]);
    getDressTorsoVertex(0.53f, cTheta, 0.04f, pBot[0], pBot[1], pBot[2]);
    getDressTorsoVertex(cZ, cTheta + 0.10f, 0.04f, pL[0], pL[1], pL[2]);
    getDressTorsoVertex(cZ, cTheta - 0.10f, 0.04f, pR[0], pR[1], pR[2]);
    getDressTorsoVertex(cZ, cTheta, 0.08f, pC[0], pC[1], pC[2]); // Center protrudes significantly for 3D effect

    glBegin(GL_TRIANGLES);
    auto drawFacet = [&](float* p1, float* p2, float* p3) {
        float u[3] = {p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]};
        float v[3] = {p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]};
        float nx = u[1]*v[2] - u[2]*v[1];
        float ny = u[2]*v[0] - u[0]*v[2];
        float nz = u[0]*v[1] - u[1]*v[0];
        float len = sqrt(nx*nx + ny*ny + nz*nz);
        if(len > 0) { nx/=len; ny/=len; nz/=len; }
        glNormal3f(nx, ny, nz);
        glVertex3fv(p1); glVertex3fv(p2); glVertex3fv(p3);
    };
    drawFacet(pTop, pR, pC);
    drawFacet(pTop, pC, pL);
    drawFacet(pBot, pC, pR);
    drawFacet(pBot, pL, pC);
    glEnd();
    
    // Smooth side bevels for the diamond connecting back to the fabric
    float pTopB[3], pBotB[3], pLB[3], pRB[3];
    getDressTorsoVertex(0.88f, cTheta, 0.025f, pTopB[0], pTopB[1], pTopB[2]);
    getDressTorsoVertex(0.53f, cTheta, 0.025f, pBotB[0], pBotB[1], pBotB[2]);
    getDressTorsoVertex(cZ, cTheta + 0.10f, 0.025f, pLB[0], pLB[1], pLB[2]);
    getDressTorsoVertex(cZ, cTheta - 0.10f, 0.025f, pRB[0], pRB[1], pRB[2]);
    
    glBegin(GL_QUADS);
    glVertex3fv(pTop); glVertex3fv(pR); glVertex3fv(pRB); glVertex3fv(pTopB);
    glVertex3fv(pR); glVertex3fv(pBot); glVertex3fv(pBotB); glVertex3fv(pRB);
    glVertex3fv(pBot); glVertex3fv(pL); glVertex3fv(pLB); glVertex3fv(pBotB);
    glVertex3fv(pL); glVertex3fv(pTop); glVertex3fv(pTopB); glVertex3fv(pLB);
    glEnd();

    // Quadratic Bezier function for curved gold filigree mapped to the torso surface
    auto drawThickCurve = [&](float startZ, float startT, float endZ, float endT, float ctrlZ, float ctrlT, float thickness) {
        glBegin(GL_QUAD_STRIP);
        int steps = 15;
        for(int i=0; i<=steps; i++) {
            float t = (float)i / steps;
            float mt = 1.0f - t;
            float bz = mt*mt*startZ + 2.0f*mt*t*ctrlZ + t*t*endZ;
            float bt = mt*mt*startT + 2.0f*mt*t*ctrlT + t*t*endT;
            
            float dz = 2.0f*mt*(ctrlZ - startZ) + 2.0f*t*(endZ - ctrlZ);
            float dt = 2.0f*mt*(ctrlT - startT) + 2.0f*t*(endT - ctrlT);
            float len = sqrt(dz*dz + dt*dt);
            if (len < 0.0001f) len = 1.0f;
            float perpZ = -dt / len * thickness;
            float perpT = dz / len * thickness;
            
            float p1[3], p2[3], n[3];
            getDressTorsoVertex(bz + perpZ, cTheta + bt + perpT, 0.04f, p1[0], p1[1], p1[2]);
            getDressTorsoVertex(bz - perpZ, cTheta + bt - perpT, 0.04f, p2[0], p2[1], p2[2]);
            getDressTorsoNormal(bz, cTheta + bt, 0.04f, n[0], n[1], n[2]);
            
            glNormal3fv(n);
            glVertex3fv(p1);
            glVertex3fv(p2);
        }
        glEnd();
    };

    // Draw the symmetrical curved patterns
    int sides[] = {-1, 1};
    for (int i = 0; i < 2; i++) {
        int side = sides[i];
        float dir = (float)side;
        // Main bottom-up sweep
        drawThickCurve(0.57f, 0.03f*dir, 0.68f, 0.35f*dir, 0.50f, 0.25f*dir, 0.012f);
        // Loop curling inwards
        drawThickCurve(0.68f, 0.35f*dir, 0.75f, 0.18f*dir, 0.85f, 0.35f*dir, 0.012f);
        // Inner finish of the curl
        drawThickCurve(0.75f, 0.18f*dir, 0.69f, 0.25f*dir, 0.65f, 0.15f*dir, 0.010f);
        
        // Connections to main horizontal belts
        // Connections dynamically leading into the irregular belts
        drawThickCurve(0.68f, 0.35f*dir, 0.93f, 0.60f*dir, 0.80f, 0.45f*dir, 0.010f); // top branch
        drawThickCurve(0.62f, 0.25f*dir, 0.53f, 0.60f*dir, 0.56f, 0.35f*dir, 0.010f); // bottom branch
        
        // Hanging loop detail on one side (left side -> side == -1)
        if (side == -1) {
            drawThickCurve(0.55f, -0.35f, 0.45f, -0.28f, 0.45f, -0.35f, 0.008f); // Drops down
            drawThickCurve(0.45f, -0.28f, 0.58f, -0.25f, 0.55f, -0.25f, 0.008f); // Loops back up
        }
    }

    // --- Neck Collar, Blue Straps, and Top Cleavage Diamonds ---
    
    // High Collar (Dark Blue)
    glColor3f(0.04f, 0.15f, 0.45f);
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float dipTh = theta - 3.14159f * 0.5f;
        if (dipTh < -3.14159f) dipTh += 2.0f * 3.14159f;
        if (dipTh > 3.14159f) dipTh -= 2.0f * 3.14159f;

        float zBottom = 1.40f;
        if (fabs(dipTh) < 0.25f) {
            zBottom -= 0.07f * (0.25f - fabs(dipTh)) / 0.25f; // Dipping down in the front to meet the emblem at 1.33f
        }

        float px1, py1, pz1, nx1, ny1, nz1;
        float px2, py2, pz2, nx2, ny2, nz2;
        getDressTorsoVertex(1.46f, theta, 0.015f, px2, py2, pz2);
        getDressTorsoNormal(1.46f, theta, 0.015f, nx2, ny2, nz2);
        getDressTorsoVertex(zBottom, theta, 0.015f, px1, py1, pz1);
        getDressTorsoNormal(zBottom, theta, 0.015f, nx1, ny1, nz1);
        
        glNormal3f(nx2, ny2, nz2); glVertex3f(px2, py2, pz2);
        glNormal3f(nx1, ny1, nz1); glVertex3f(px1, py1, pz1);
    }
    glEnd();

    // Custom variable-offset curve drawer for the chest transitioning straps
    auto drawTopCurve = [&](float startZ, float startT, float endZ, float endT, float ctrlZ, float ctrlT, float thickness, float offset) {
        glBegin(GL_QUAD_STRIP);
        int steps = 15;
        for(int i=0; i<=steps; i++) {
            float t = (float)i / steps;
            float mt = 1.0f - t;
            float bz = mt*mt*startZ + 2.0f*mt*t*ctrlZ + t*t*endZ;
            float bt = mt*mt*startT + 2.0f*mt*t*ctrlT + t*t*endT;
            
            float dz = 2.0f*mt*(ctrlZ - startZ) + 2.0f*t*(endZ - ctrlZ);
            float dt = 2.0f*mt*(ctrlT - startT) + 2.0f*t*(endT - ctrlT);
            float len = sqrt(dz*dz + dt*dt);
            if (len < 0.0001f) len = 1.0f;
            float perpZ = -dt / len * thickness;
            float perpT = dz / len * thickness;
            
            float p1[3], p2[3], n[3];
            getDressTorsoVertex(bz + perpZ, cTheta + bt + perpT, offset, p1[0], p1[1], p1[2]);
            getDressTorsoVertex(bz - perpZ, cTheta + bt - perpT, offset, p2[0], p2[1], p2[2]);
            getDressTorsoNormal(bz, cTheta + bt, offset, n[0], n[1], n[2]);
            
            glNormal3fv(n);
            glVertex3fv(p1);
            glVertex3fv(p2);
        }
        glEnd();
    };

    // Dark Blue Fabric Straps connecting collar to bodice
    glColor3f(0.04f, 0.15f, 0.45f);
    for (int i = 0; i < 2; i++) {
        float dir = sides[i];
        drawTopCurve(1.23f + 0.03f, 0.08f*dir, 1.15f, 0.65f*dir, 1.18f, 0.40f*dir, 0.06f, 0.02f);
    }

    // Gold Trims & Emblem on the Chest
    glColor3f(0.85f, 0.7f, 0.15f);

    // Collar Bottom and Top Gold Rings
    auto drawCollarRings = [&]() {
        // Bottom Ring
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float dipTh = theta - 3.14159f * 0.5f;
            if (dipTh < -3.14159f) dipTh += 2.0f * 3.14159f;
            if (dipTh > 3.14159f) dipTh -= 2.0f * 3.14159f;
            
            float zCenter = 1.40f;
            if (fabs(dipTh) < 0.25f) zCenter -= 0.07f * (0.25f - fabs(dipTh)) / 0.25f; // Meets emblem at 1.33f
            
            float p1[3], p2[3], n[3];
            getDressTorsoVertex(zCenter + 0.008f, theta, 0.02f, p1[0], p1[1], p1[2]);
            getDressTorsoVertex(zCenter - 0.008f, theta, 0.02f, p2[0], p2[1], p2[2]);
            getDressTorsoNormal(zCenter, theta, 0.02f, n[0], n[1], n[2]);
            glNormal3fv(n); glVertex3fv(p2); glVertex3fv(p1);
        }
        glEnd();
        
        // Top Ring
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float zCenter = 1.46f; // Top of the collar
            
            float p1[3], p2[3], n[3];
            getDressTorsoVertex(zCenter + 0.008f, theta, 0.02f, p1[0], p1[1], p1[2]);
            getDressTorsoVertex(zCenter - 0.008f, theta, 0.02f, p2[0], p2[1], p2[2]);
            getDressTorsoNormal(zCenter, theta, 0.02f, n[0], n[1], n[2]);
            glNormal3fv(n); glVertex3fv(p2); glVertex3fv(p1);
        }
        glEnd();
    };
    drawCollarRings();

    // Chest hollow diamond emblem (outer gold frame)
    for (int i = 0; i < 2; i++) {
        float dir = sides[i];
        // Upper edge of diamond
        drawTopCurve(1.33f, 0.0f, 1.25f, 0.15f*dir, 1.29f, 0.08f*dir, 0.012f, 0.025f);
        // Lower edge of diamond
        drawTopCurve(1.25f, 0.15f*dir, 1.15f, 0.0f, 1.20f, 0.08f*dir, 0.012f, 0.025f);
        
        // Gold edge on the dark blue straps
        drawTopCurve(1.23f + 0.03f, 0.08f*dir + 0.04f*dir, 1.15f, 0.65f*dir + 0.04f*dir, 1.18f, 0.40f*dir + 0.04f*dir, 0.01f, 0.021f);
    }

    // Inner blue gems inside the chest emblem
    glColor3f(0.3f, 0.5f, 0.95f); // Bright crystal blue
    // Top diamond gem
    float g1Z = 1.27f, g1Th = 0.04f, g1H = 0.04f;
    glBegin(GL_POLYGON);
    float g1C[3], g1T[3], g1B[3], g1L[3], g1R[3], gn[3];
    getDressTorsoNormal(g1Z, cTheta, 0.035f, gn[0], gn[1], gn[2]);
    getDressTorsoVertex(g1Z + g1H, cTheta, 0.04f, g1T[0], g1T[1], g1T[2]);
    getDressTorsoVertex(g1Z - g1H, cTheta, 0.04f, g1B[0], g1B[1], g1B[2]);
    getDressTorsoVertex(g1Z, cTheta + g1Th, 0.04f, g1L[0], g1L[1], g1L[2]);
    getDressTorsoVertex(g1Z, cTheta - g1Th, 0.04f, g1R[0], g1R[1], g1R[2]);
    glNormal3fv(gn);
    glVertex3fv(g1T); glVertex3fv(g1R); glVertex3fv(g1B); glVertex3fv(g1L);
    glEnd();

    // Bottom tiny diamond gem
    float g2Z = 1.19f, g2Th = 0.025f, g2H = 0.025f;
    glBegin(GL_POLYGON);
    float g2T[3], g2B[3], g2L[3], g2R[3];
    getDressTorsoVertex(g2Z + g2H, cTheta, 0.04f, g2T[0], g2T[1], g2T[2]);
    getDressTorsoVertex(g2Z - g2H, cTheta, 0.04f, g2B[0], g2B[1], g2B[2]);
    getDressTorsoVertex(g2Z, cTheta + g2Th, 0.04f, g2L[0], g2L[1], g2L[2]);
    getDressTorsoVertex(g2Z, cTheta - g2Th, 0.04f, g2R[0], g2R[1], g2R[2]);
    glNormal3fv(gn);
    glVertex3fv(g2T); glVertex3fv(g2R); glVertex3fv(g2B); glVertex3fv(g2L);
    glEnd();

    // 4. Skirt Extensions
    struct Layer {
        float zMin;
        float flareMax;
        float color[3];
        int folds;
        float foldDepth;
    };
    
    // Skirt swings downwards naturally, longer length, less horizontal flare
    Layer layers[4] = {
        {-1.0f, 0.28f, {0.1f, 0.3f, 0.8f}, 8, 0.15f},      
        {-0.7f, 0.22f, {0.6f, 0.75f, 0.95f}, 7, 0.12f},     
        {-0.4f, 0.15f, {1.0f, 1.0f, 1.0f}, 6, 0.08f},     
        {-0.1f, 0.08f, {0.05f, 0.15f, 0.35f}, 5, 0.04f}     
    };
    
    for(int l = 0; l < 4; l++) {
        glColor3fv(layers[l].color);
        int skirtStacks = 20;
        float zSkirtMax = 0.55f;
        float zSkirtMin = layers[l].zMin;
        float flare = layers[l].flareMax;
        
        for (int i = 0; i < skirtStacks; i++) {
            float f1 = (float)i / skirtStacks;
            float f2 = (float)(i + 1) / skirtStacks;
            
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= slices; j++) {
                float theta = (float)j / slices * 2.0f * 3.14159f;
                float frontDist = sin(theta);
                float lengthMultiplier = 0.8f - frontDist * 0.2f; // back=1.0, front=0.6 (covers hips)
                
                // Make the top dark blue layer into 6 jagged overlapping petals
                if (l == 3) {
                    float cycle = theta / (2.0f * 3.14159f) * 6.0f;
                    float frac = cycle - floor(cycle);
                    float triWave = fabs(frac * 2.0f - 1.0f); 
                    lengthMultiplier = 0.6f + 0.35f * triWave;
                }

                float actZMin = zSkirtMax - (zSkirtMax - zSkirtMin) * lengthMultiplier;
                if(actZMin > zSkirtMax) actZMin = zSkirtMax;
                
                float actZ1 = zSkirtMax - f1 * (zSkirtMax - actZMin);
                float actZ2 = zSkirtMax - f2 * (zSkirtMax - actZMin);
                 
                float t1 = f1; 
                float t2 = f2;
                float flare1 = t1 * t1 * flare;
                float flare2 = t2 * t2 * flare;
                float fold1 = sin(theta * layers[l].folds) * layers[l].foldDepth * t1;
                float fold2 = sin(theta * layers[l].folds) * layers[l].foldDepth * t2;
                 
                float bx1, by1, bz1, bx2, by2, bz2;
                // Evaluate base dress shape AT the current Z so the skirt wraps/hugs the hips properly without clipping
                getDressTorsoVertex(actZ1, theta, 0.025f + l * 0.003f, bx1, by1, bz1); 
                getDressTorsoVertex(actZ2, theta, 0.025f + l * 0.003f, bx2, by2, bz2); 

                float px1 = bx1 + cos(theta) * (flare1 + fold1);
                float py1 = by1 + sin(theta) * (flare1 + fold1);
                float px2 = bx2 + cos(theta) * (flare2 + fold2);
                float py2 = by2 + sin(theta) * (flare2 + fold2);
                 
                float vx = px2 - px1, vy = py2 - py1, vz = actZ2 - actZ1;
                float tx = -sin(theta), ty = cos(theta), tz = 0; 
                float nx = ty*vz - tz*vy;
                float ny = tz*vx - tx*vz;
                float nz = tx*vy - ty*vx;
                 
                float nlen = sqrt(nx*nx + ny*ny + nz*nz);
                if(nlen > 0.001f) { nx /= nlen; ny /= nlen; nz /= nlen; }

                glNormal3f(nx, ny, nz);
                glVertex3f(px2, py2, actZ2);
                glNormal3f(nx, ny, nz);
                glVertex3f(px1, py1, actZ1);
            }
            glEnd();
        }
    }

    // 5. Golden Skirt Accents
    // Hang 6 decorative golden slant-ribbons from the tips of the dark blue petals
    glColor3f(0.85f, 0.7f, 0.15f);
    for (int i = 0; i < 6; i++) {
        float theta = i / 6.0f * 2.0f * 3.14159f; // Tips occur exactly at integer cycles
        
        float zSkirtMax = 0.55f;
        float zSkirtMin = layers[3].zMin;
        float actZMin = zSkirtMax - (zSkirtMax - zSkirtMin) * 0.95f;
        
        float bx, by, bz;
        getDressTorsoVertex(actZMin, theta, 0.025f + 3 * 0.003f + 0.015f, bx, by, bz);
        float flareVal = layers[3].flareMax;
        float foldVal = sin(theta * layers[3].folds) * layers[3].foldDepth;
        
        float cx = bx + cos(theta) * (flareVal + foldVal);
        float cy = by + sin(theta) * (flareVal + foldVal);
        float cz = actZMin;
        
        float nx = cos(theta), ny = sin(theta), nz = 0.3f; 
        float nlen = sqrt(nx*nx + ny*ny + nz*nz);
        nx /= nlen; ny /= nlen; nz /= nlen;
        
        float tx = -sin(theta), ty = cos(theta);
        
        float width = 0.035f;
        float length = 0.18f;
        float slantZ = 0.05f;
        
        // Outer gold slant ribbon
        glBegin(GL_POLYGON);
        glNormal3f(nx, ny, nz);
        glVertex3f(cx - tx * width, cy - ty * width, cz);
        glVertex3f(cx + tx * width, cy + ty * width, cz - slantZ);
        glVertex3f(cx + tx * width, cy + ty * width, cz - length);
        glVertex3f(cx - tx * width, cy - ty * width, cz - length + slantZ);
        glEnd();
        
        // Inner bright gold diamond
        glColor3f(1.0f, 0.9f, 0.4f);
        glBegin(GL_POLYGON);
        glNormal3f(nx, ny, nz);
        float inC = cx + nx * 0.005f;
        float inCY = cy + ny * 0.005f;
        glVertex3f(inC, inCY, cz - length*0.2f);
        glVertex3f(inC + tx * width*0.5f, inCY + ty * width*0.5f, cz - length*0.4f);
        glVertex3f(inC, inCY, cz - length*0.6f);
        glVertex3f(inC - tx * width*0.5f, inCY - ty * width*0.5f, cz - length*0.4f);
        glEnd();
        glColor3f(0.85f, 0.7f, 0.15f); // restore
    }
    
    glEnable(GL_TEXTURE_2D);
}

void drawBodyMesh() {
    int stacks = 60;
    int slices = 60;

    float zMin = sections[1].z;
    float zMax = sections[SEC_COUNT - 2].z; // Draw neatly up to Upper Neck

    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, texSkin);

    for (int i = 0; i < stacks; i++) {
        float f1 = (float)i / stacks;
        float f2 = (float)(i + 1) / stacks;

        float z1 = zMin + f1 * (zMax - zMin);
        float z2 = zMin + f2 * (zMax - zMin);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float s = (float)j / slices * 2.0f; // Tile texture around

            float px2, py2, pz2;
            getTorsoVertex(z2, theta, px2, py2, pz2);
            float nx2, ny2, nz2;
            getTorsoNormal(z2, theta, nx2, ny2, nz2);

            glNormal3f(nx2, ny2, nz2);
            glTexCoord2f(s, f2 * 2.0f);
            glVertex3f(px2, py2, pz2);

            float px1, py1, pz1;
            getTorsoVertex(z1, theta, px1, py1, pz1);
            float nx1, ny1, nz1;
            getTorsoNormal(z1, theta, nx1, ny1, nz1);

            glNormal3f(nx1, ny1, nz1);
            glTexCoord2f(s, f1 * 2.0f);
            glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }

    // Draw top neck cap to avoid hollow body
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, 1.0f);
    for (int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz;
        getTorsoVertex(zMax, theta, px, py, pz);
        glTexCoord2f((px / 0.15f + 1.0f) / 2.0f, (py / 0.15f + 1.0f) / 2.0f);
        glVertex3f(px, py, pz);
    }
    glEnd();

    // Draw bottom pelvis cap
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, -1.0f);
    for (int j = slices; j >= 0; j--) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz;
        getTorsoVertex(zMin, theta, px, py, pz);
        glTexCoord2f((px / 0.45f + 1.0f) / 2.0f, (py / 0.28f + 1.0f) / 2.0f);
        glVertex3f(px, py, pz);
    }
    glEnd();
}

GLuint cachedDisplayList = 0;

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    updateCamera();
    gluLookAt(cameraX, cameraY, cameraZ, 0, 0.7, 0, 0, 1, 0);

    GLfloat lightPosition[] = { 3.0f, 5.0f, 3.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glPushMatrix();
    // Rotate to make Z up, since we modeled with Z as up
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    
    // Optimize: Bake heavy procedural maths into a display list so it runs instantly every frame
    if (cachedDisplayList == 0) {
        cachedDisplayList = glGenLists(1);
        glNewList(cachedDisplayList, GL_COMPILE);
        drawBodyMesh();
        drawDressMesh();
        glEndList();
    }
    
    glCallList(cachedDisplayList);
    
    glPopMatrix();
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = WindowProcedure;
    wc.lpszClassName = WINDOW_TITLE;
    wc.style = CS_HREDRAW | CS_VREDRAW;

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

    texSkin = loadBMP("skin.bmp");

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

        display();
        SwapBuffers(hdc);
    }

    UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return true;
}
