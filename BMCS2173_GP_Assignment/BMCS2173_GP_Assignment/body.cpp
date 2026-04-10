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

#define SEC_COUNT 8
CrossSection sections[SEC_COUNT] = {
    {-0.2f,  0.42f, 0.26f,  0.0f},   // Extra section for spline margin
    {0.0f,   0.45f, 0.28f,  0.0f},   // Hips / Crotch
    {0.3f,   0.47f, 0.32f, -0.06f},  // High hips / glutes
    {0.6f,   0.31f, 0.21f,  0.02f},	 // Waist
    {0.9f,   0.34f, 0.24f,  0.08f},  // Underbust
    {1.15f,  0.40f, 0.26f,  0.10f},  // Chest Level
    {1.35f,  0.46f, 0.21f,  0.03f},  // Shoulders
    {1.5f,   0.15f, 0.15f, -0.01f}   // Neck Base
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

    // 1. White Top Bodice (from z = 0.9 to z = 1.35)
    glColor3f(1.0f, 1.0f, 1.0f); 
    int topStacks = 15;
    float zTopMin = 0.9f;
    float zTopMax = 1.35f;
    for (int i = 0; i < topStacks; i++) {
        float f1 = (float)i / topStacks;
        float f2 = (float)(i + 1) / topStacks;
        float z1 = zTopMin + f1 * (zTopMax - zTopMin);
        float z2 = zTopMin + f2 * (zTopMax - zTopMin);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float px1, py1, pz1, nx1, ny1, nz1;
            float px2, py2, pz2, nx2, ny2, nz2;
            getDressTorsoVertex(z2, theta, 0.015f, px2, py2, pz2);
            getDressTorsoNormal(z2, theta, 0.015f, nx2, ny2, nz2);
            glNormal3f(nx2, ny2, nz2); glVertex3f(px2, py2, pz2);

            getDressTorsoVertex(z1, theta, 0.015f, px1, py1, pz1);
            getDressTorsoNormal(z1, theta, 0.015f, nx1, ny1, nz1);
            glNormal3f(nx1, ny1, nz1); glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }

    // 2. Dark Blue Corset/Midriff (z = 0.55 to 0.9)
    glColor3f(0.05f, 0.15f, 0.35f); 
    int corsetStacks = 10;
    float zCorMin = 0.55f;
    float zCorMax = 0.9f;
    for (int i = 0; i < corsetStacks; i++) {
        float f1 = (float)i / corsetStacks;
        float f2 = (float)(i + 1) / corsetStacks;
        float z1 = zCorMin + f1 * (zCorMax - zCorMin);
        float z2 = zCorMin + f2 * (zCorMax - zCorMin);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float px1, py1, pz1, nx1, ny1, nz1;
            float px2, py2, pz2, nx2, ny2, nz2;
            getDressTorsoVertex(z2, theta, 0.025f, px2, py2, pz2); 
            getDressTorsoNormal(z2, theta, 0.025f, nx2, ny2, nz2);
            glNormal3f(nx2, ny2, nz2); glVertex3f(px2, py2, pz2);

            getDressTorsoVertex(z1, theta, 0.025f, px1, py1, pz1);
            getDressTorsoNormal(z1, theta, 0.025f, nx1, ny1, nz1);
            glNormal3f(nx1, ny1, nz1); glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }

    // 3. Gold Belts & Trims
    glColor3f(0.85f, 0.7f, 0.15f);
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz, nx, ny, nz;
        getDressTorsoVertex(0.58f, theta, 0.035f, px, py, pz);
        getDressTorsoNormal(0.58f, theta, 0.035f, nx, ny, nz);
        glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
        getDressTorsoVertex(0.53f, theta, 0.035f, px, py, pz);
        getDressTorsoNormal(0.53f, theta, 0.035f, nx, ny, nz);
        glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
    }
    glEnd();
    
    // Diamond center piece on the belt
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 1.0f, 0.0f); // Roughly facing front
    float cx, cy, cz;
    getDressTorsoVertex(0.555f, 3.14159f * 0.5f, 0.045f, cx, cy, cz); 
    glVertex3f(cx, cy - 0.08f, cz);
    glVertex3f(cx + 0.08f, cy, cz);
    glVertex3f(cx, cy + 0.12f, cz);
    glVertex3f(cx - 0.08f, cy, cz);
    glEnd();

    // Upper belt trim around z = 0.9
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz, nx, ny, nz;
        getDressTorsoVertex(0.92f, theta, 0.03f, px, py, pz);
        getDressTorsoNormal(0.92f, theta, 0.03f, nx, ny, nz);
        glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
        getDressTorsoVertex(0.88f, theta, 0.03f, px, py, pz);
        getDressTorsoNormal(0.88f, theta, 0.03f, nx, ny, nz);
        glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
    }
    glEnd();

    // Neck Halter Gold Ring
    glColor3f(0.85f, 0.7f, 0.15f);
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz, nx, ny, nz;
        getDressTorsoVertex(1.37f, theta, 0.025f, px, py, pz);
        getDressTorsoNormal(1.37f, theta, 0.025f, nx, ny, nz);
        glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
        getDressTorsoVertex(1.33f, theta, 0.025f, px, py, pz);
        getDressTorsoNormal(1.33f, theta, 0.025f, nx, ny, nz);
        glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
    }
    glEnd();

    // Chest blue trim piece (Halter neck detail)
    glColor3f(0.05f, 0.15f, 0.35f);
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        if(theta > 3.1415f*0.2f && theta < 3.1415f*0.8f) { // Front part of neck
            float px, py, pz, nx, ny, nz;
            getDressTorsoVertex(1.36f, theta, 0.02f, px, py, pz);
            getDressTorsoNormal(1.36f, theta, 0.02f, nx, ny, nz);
            glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
            getDressTorsoVertex(1.24f, theta, 0.02f, px, py, pz);
            getDressTorsoNormal(1.24f, theta, 0.02f, nx, ny, nz);
            glNormal3f(nx, ny, nz); glVertex3f(px, py, pz);
        }
    }
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
                if (l == 3) lengthMultiplier = 0.9f - frontDist * 0.1f; 

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
    
    glEnable(GL_TEXTURE_2D);
}

void drawBodyMesh() {
    int stacks = 60;
    int slices = 60;

    float zMin = sections[1].z;
    float zMax = sections[SEC_COUNT - 2].z;

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
    drawBodyMesh();
    drawDressMesh();
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
