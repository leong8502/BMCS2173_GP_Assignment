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

#pragma warning(disable:4996)
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")

#define WINDOW_TITLE "Combined Head and Hair Model"

const float PI = 3.1415926535f;
const float TWO_PI = 6.283185307f;

float cameraAngle = 0.0f;
float cameraHeight = 0.5f;
float cameraDistance = 3.8f;
float cameraX, cameraY, cameraZ;

GLuint texSkin;
GLuint texHeadObj;

GLuint loadBMP(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) return 0;
	
	unsigned char header[54];
	if (fread(header, 1, 54, file) != 54) { fclose(file); return 0; }
	if (header[0] != 'B' || header[1] != 'M') { fclose(file); return 0; }
	
	int width  = *(int*)&(header[18]);
	int height = *(int*)&(header[22]);
	int imageSize = *(int*)&(header[34]);
	if (imageSize == 0) imageSize = width * height * 3;
	
	unsigned char* data = new unsigned char[imageSize];
	fread(data, 1, imageSize, file);
	fclose(file);
	
	// Swap BGR to RGB
	for (int i = 0; i < imageSize; i += 3) {
		unsigned char tmp = data[i];
		data[i] = data[i+2];
		data[i+2] = tmp;
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
		case VK_LEFT: cameraAngle -= 0.05f; break;
		case VK_RIGHT: cameraAngle += 0.05f; break;
		case VK_UP: cameraHeight += 0.2f; break;
		case VK_DOWN: cameraHeight -= 0.2f; break;
		case VK_ADD: case VK_OEM_PLUS: cameraDistance -= 0.2f; break;
		case VK_SUBTRACT: case VK_OEM_MINUS: cameraDistance += 0.2f; break;
		}
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool initPixelFormat(HDC hdc) {
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

void initOpenGL() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0 / 600.0, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------------
// Math helpers (from head3.cpp)
// ----------------------------------------------------------------
struct Vec3 { float x, y, z; };
struct Vec2 { float u, v; };

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

// Cubic Catmull-Rom interpolation for smooth curves (from head4.cpp)
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

// ----------------------------------------------------------------
// Lighting Configs
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
    
    // Advanced material properties
    GLfloat specColor[] = { 0.15f, 0.15f, 0.15f, 1.0f }; // Softer specular reflection
    glMaterialfv(GL_FRONT, GL_SPECULAR, specColor);
    glMateriali(GL_FRONT, GL_SHININESS, 16); // Soft skin, less plastic
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
// Head Model (from head4.cpp)
// ----------------------------------------------------------------
struct CrossSection {
    float z;
    float rx;
    float ry;
    float yOffset;
};

#define SEC_COUNT 11
CrossSection sections[SEC_COUNT] = {
    {-0.12f, 0.05f, 0.05f,  0.08f},  // dummy
    { 0.00f, 0.08f, 0.10f,  0.08f},  // chin base (slightly wider)
    { 0.15f, 0.24f, 0.23f,  0.06f},  // lower jaw
    { 0.30f, 0.38f, 0.37f,  0.03f},  // jaw angle / cheeks
    { 0.45f, 0.46f, 0.46f,  0.01f},  // eye level (widest)
    { 0.55f, 0.45f, 0.45f,  0.00f},  // upper face
    { 0.65f, 0.41f, 0.41f,  0.00f},  // forehead
    { 0.75f, 0.34f, 0.34f,  0.00f},  // upper head
    { 0.83f, 0.25f, 0.25f,  0.00f},  // top curve
    { 0.88f, 0.13f, 0.13f,  0.00f},  // near top
    { 0.90f, 0.000f, 0.000f, 0.00f}  // top end
};

void getHeadBaseRadius(float z, float &rx, float &ry, float &yOfs) {
	if(z <= sections[1].z) {
		rx = sections[1].rx; ry = sections[1].ry; yOfs = sections[1].yOffset;
		return;
	}
	if(z >= sections[SEC_COUNT-2].z) {
		rx = sections[SEC_COUNT-2].rx; ry = sections[SEC_COUNT-2].ry; yOfs = sections[SEC_COUNT-2].yOffset;
		return;
	}

    int i = 1;
    for (; i < SEC_COUNT - 2; i++) {
        if (z <= sections[i + 1].z) break;
    }
    
    float t = (z - sections[i].z) / (sections[i+1].z - sections[i].z);
    
    rx = spline(t, sections[i-1].rx, sections[i].rx, sections[i+1].rx, sections[i+2].rx);
    ry = spline(t, sections[i-1].ry, sections[i].ry, sections[i+1].ry, sections[i+2].ry);
    yOfs = spline(t, sections[i-1].yOffset, sections[i].yOffset, sections[i+1].yOffset, sections[i+2].yOffset);

    // Smaller overall head scale
    rx *= 0.85f;
    ry *= 0.85f;
}

void getHeadVertex(float z, float theta, float &px, float &py, float &pz) {
    float rx, ry, yOfs;
    getHeadBaseRadius(z, rx, ry, yOfs);
    
    while(theta < 0) theta += 2.0f*3.14159f;
    while(theta > 2.0f*3.14159f) theta -= 2.0f*3.14159f;

    float bump = 0.0f;
    float frontSide = sinf(theta);
    float sideFactor = fabsf(cosf(theta));

    if (z < 0.45f) {
        float faceFade = (0.45f - z) / 0.45f;
        // Jaw narrowing - softer than original but still anime V-shape
        float frontSharp = faceFade * 0.25f * expf(-powf(theta - 3.14159f*0.5f, 2.0f) / 0.30f);
        rx *= (1.0f - frontSharp);
    }

    if (frontSide < 0.0f) {
        float backFactor = -frontSide;
        float zHeight = (z - sections[0].z) / (sections[SEC_COUNT-1].z - sections[0].z);
        // Cranium expansion for rounder back-of-head
        float expand = 0.14f * backFactor * expf(-powf(zHeight - 0.6f, 2.0f) / 0.11f);
        rx += expand;
        ry += expand;
    }

    if (theta > 0.0f && theta < 3.14159f) {
        float noseBridgeZ = 0.38f;
        float noseBridge = 0.020f * expf(-(powf(z - noseBridgeZ, 2.0f) / 0.010f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.006f));
        float noseTipZ = 0.32f;
        float noseTip = 0.035f * expf(-(powf(z - noseTipZ, 2.0f) / 0.002f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.010f));
        bump += noseBridge + noseTip;

        float eyeZ = 0.39f;
        float eyeIndent = -0.085f * expf(-(powf(z - eyeZ, 2.0f) / 0.024f + powf(theta - 3.14159f*0.35f, 2.0f) / 0.038f));
        eyeIndent += -0.085f * expf(-(powf(z - eyeZ, 2.0f) / 0.024f + powf(theta - 3.14159f*0.65f, 2.0f) / 0.038f));
        
        float irisArea = 0.015f * expf(-(powf(z - eyeZ, 2.0f) / 0.008f + powf(theta - 3.14159f*0.35f, 2.0f) / 0.015f));
        irisArea += 0.015f * expf(-(powf(z - eyeZ, 2.0f) / 0.008f + powf(theta - 3.14159f*0.65f, 2.0f) / 0.015f));
        bump += eyeIndent + irisArea;

        float chinZ = 0.05f;
        float chinBump = 0.025f * expf(-(powf(z - chinZ, 2.0f) / 0.008f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.030f));
        bump += chinBump;
        
        float upperLipZ = 0.208f;
        float lowerLipZ = 0.192f;
        float upperLip = 0.012f * expf(-(powf(z - upperLipZ, 2.0f) / 0.0015f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.10f));
        float lowerLip = 0.006f * expf(-(powf(z - lowerLipZ, 2.0f) / 0.0004f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.08f));
        float mouthIndent = -0.004f * expf(-(powf(z - 0.198f, 2.0f) / 0.0002f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.12f));
        
        float cornerThetaL = 3.14159f * 0.5f + 0.22f;
        float cornerThetaR = 3.14159f * 0.5f - 0.22f;
        float lipCorner = -0.005f * expf(-(powf(z - 0.197f, 2.0f) / 0.001f + powf(theta - cornerThetaL, 2.0f) / 0.010f));
        lipCorner += -0.005f * expf(-(powf(z - 0.197f, 2.0f) / 0.001f + powf(theta - cornerThetaR, 2.0f) / 0.010f));
        bump += upperLip + lowerLip + mouthIndent + lipCorner;

        float nostrilZ = 0.31f;
        float nostrilL = -0.008f * expf(-(powf(z - nostrilZ, 2.0f) / 0.001f + powf(theta - (3.14159f*0.5f + 0.04f), 2.0f) / 0.006f));
        float nostrilR = -0.008f * expf(-(powf(z - nostrilZ, 2.0f) / 0.001f + powf(theta - (3.14159f*0.5f - 0.04f), 2.0f) / 0.006f));
        bump += nostrilL + nostrilR;
        
        float creaseZ = 0.43f;
        float creaseBump = 0.006f * expf(-(powf(z - creaseZ, 2.0f) / 0.0005f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.15f));
        bump += creaseBump;

        float cheekZ = 0.38f;
        float cheekBump = 0.015f * expf(-(powf(z - cheekZ, 2.0f) / 0.03f + powf(theta - 3.14159f*0.3f, 2.0f) / 0.05f));
        cheekBump += 0.015f * expf(-(powf(z - cheekZ, 2.0f) / 0.03f + powf(theta - 3.14159f*0.7f, 2.0f) / 0.05f));
        bump += cheekBump;
    }

    bump *= 0.85f;

    rx += bump;
    ry += bump;
    
    px = rx * cosf(theta);
    py = ry * sinf(theta) + yOfs;
    pz = z;
}

void getHeadNormal(float z, float theta, float &nx, float &ny, float &nz) {
    float epsZ = 0.01f;
    float epsT = 0.05f;                 
    
    float px1, py1, pz1;
    float px2, py2, pz2;
    float px3, py3, pz3;
    float px4, py4, pz4;
    
    getHeadVertex(z + epsZ, theta, px1, py1, pz1);
    getHeadVertex(z - epsZ, theta, px2, py2, pz2);
    getHeadVertex(z, theta + epsT, px3, py3, pz3);
    getHeadVertex(z, theta - epsT, px4, py4, pz4);
    
    float tx1 = px1 - px2, ty1 = py1 - py2, tz1 = pz1 - pz2;
    float tx2 = px3 - px4, ty2 = py3 - py4, tz2 = pz3 - pz4;
    
    nx = ty2 * tz1 - tz2 * ty1;
    ny = tz2 * tx1 - tx2 * tz1;
    nz = tx2 * ty1 - ty2 * tx1;
    
    float len = sqrtf(nx*nx + ny*ny + nz*nz);
    if(len > 0) { nx /= len; ny /= len; nz /= len; }
}

void bindOBJToDisplayList(const char* filename, GLuint& displayList) {
    std::vector<Vec3> temp_vertices;
    std::vector<Vec2> temp_uvs;
    std::vector<Vec3> temp_normals;
    
    std::ifstream in(filename, std::ios::in);
    if (!in) {
        printf("Cannot open %s\n", filename);
        return;
    }
    
    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);
    
    std::string line;
    while (std::getline(in, line)) {
        if (line.length() < 2) continue;
        if (line[0] == 'v' && line[1] == ' ') {
            std::istringstream s(line.substr(2));
            Vec3 v; s >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            std::istringstream s(line.substr(3));
            Vec2 uv; s >> uv.u >> uv.v;
            temp_uvs.push_back(uv);
        } else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
            std::istringstream s(line.substr(3));
            Vec3 n; s >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        } else if (line[0] == 'f' && line[1] == ' ') {
            std::istringstream s(line.substr(2));
            std::string token;
            std::vector<int> vIdx, uvIdx, nIdx;
            while (s >> token) {
                int v=0, vt=0, vn=0;
                if (sscanf(token.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
                    vIdx.push_back(v); uvIdx.push_back(vt); nIdx.push_back(vn);
                } else if (sscanf(token.c_str(), "%d//%d", &v, &vn) == 2) {
                    vIdx.push_back(v); nIdx.push_back(vn);
                } else if (sscanf(token.c_str(), "%d/%d", &v, &vt) == 2) {
                    vIdx.push_back(v); uvIdx.push_back(vt);
                } else if (sscanf(token.c_str(), "%d", &v) == 1) {
                    vIdx.push_back(v);
                }
            }
            
            if (vIdx.size() == 3) glBegin(GL_TRIANGLES);
            else if (vIdx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
            for (size_t i = 0; i < vIdx.size(); ++i) {
                if (i < nIdx.size() && nIdx[i] > 0 && (nIdx[i]-1) < temp_normals.size()) {
                    Vec3& n = temp_normals[nIdx[i]-1];
                    glNormal3f(n.x, n.y, n.z);
                }
                if (i < uvIdx.size() && uvIdx[i] > 0 && (uvIdx[i]-1) < temp_uvs.size()) {
                    Vec2& uv = temp_uvs[uvIdx[i]-1];
                    // v coordinate inversion is sometimes necessary for obj
                    glTexCoord2f(uv.u, uv.v); 
                }
                if (i < vIdx.size() && vIdx[i] > 0 && (vIdx[i]-1) < temp_vertices.size()) {
                    Vec3& v = temp_vertices[vIdx[i]-1];
                    glVertex3f(v.x, v.y, v.z);
                }
            }
            glEnd();
        }
    }
    glEndList();
}

void drawHeadMesh() {
    int stacks = 128;
    int slices = 128;
    
    float zMin = sections[1].z;
    float zMax = sections[SEC_COUNT-2].z;
    
    glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texSkin);

    for(int i = 0; i < stacks; i++) {
        float f1 = (float)i / stacks;
        float f2 = (float)(i+1) / stacks;
        
        float z1 = zMin + f1 * (zMax - zMin);
        float z2 = zMin + f2 * (zMax - zMin);
        
        glBegin(GL_QUAD_STRIP);
        for(int j = 0; j <= slices; j++) {
            float theta = (float)j / slices * 2.0f * 3.14159f;
            float s = (float)j / slices * 2.0f;
            
            float px2, py2, pz2;
            getHeadVertex(z2, theta, px2, py2, pz2);
            float nx2, ny2, nz2;
            getHeadNormal(z2, theta, nx2, ny2, nz2);
            
            glNormal3f(nx2, ny2, nz2);
            glTexCoord2f(s, f2 * 2.0f);
            float blush2L = expf(-(powf(z2 - 0.25f, 2.0f)/0.005f + powf(theta - 1.0f, 2.0f)/0.05f));
            float blush2R = expf(-(powf(z2 - 0.25f, 2.0f)/0.005f + powf(theta - (3.14159f - 1.0f), 2.0f)/0.05f));
            float b2 = clampf(blush2L + blush2R, 0.0f, 1.0f);
            glColor3f(1.0f, 1.0f - 0.35f * b2, 1.0f - 0.35f * b2);
            glVertex3f(px2, py2, pz2);
            
            float px1, py1, pz1;
            getHeadVertex(z1, theta, px1, py1, pz1);
            float nx1, ny1, nz1;
            getHeadNormal(z1, theta, nx1, ny1, nz1);
            
            glNormal3f(nx1, ny1, nz1);
            glTexCoord2f(s, f1 * 2.0f);
            float blush1L = expf(-(powf(z1 - 0.25f, 2.0f)/0.005f + powf(theta - 1.0f, 2.0f)/0.05f));
            float blush1R = expf(-(powf(z1 - 0.25f, 2.0f)/0.005f + powf(theta - (3.14159f - 1.0f), 2.0f)/0.05f));
            float b1 = clampf(blush1L + blush1R, 0.0f, 1.0f);
            glColor3f(1.0f, 1.0f - 0.35f * b1, 1.0f - 0.35f * b1);
            glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, 1.0f);
    for(int j = 0; j <= slices; j++) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz;
        getHeadVertex(zMax, theta, px, py, pz);
        glTexCoord2f((px/0.3f + 1.0f)/2.0f, (py/0.3f + 1.0f)/2.0f);
        glVertex3f(px, py, pz);
    }
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.0f, -1.0f);
    for(int j = slices; j >= 0; j--) {
        float theta = (float)j / slices * 2.0f * 3.14159f;
        float px, py, pz;
        getHeadVertex(zMin, theta, px, py, pz);
        glTexCoord2f((px/0.3f + 1.0f)/2.0f, (py/0.3f + 1.0f)/2.0f);
        glVertex3f(px, py, pz);
    }
    glEnd();
}

void drawEyes() {
    float rx, ry, yOfs;
    float eyeZ = 0.25f; // Lowered further to fit perfectly above the nose into the eye sockets
    getHeadBaseRadius(eyeZ, rx, ry, yOfs);
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    for (int i = 0; i < 2; i++) {
        // Spread the eyes wider to sit in the large geometric sockets
        float thetaBase = (i == 0) ? 0.28f * 3.14159f : 0.72f * 3.14159f;
        
        glPushMatrix();
        float ex = rx * cosf(thetaBase);
        float ey = ry * sinf(thetaBase) + yOfs;
        
        // Push the depth out to 1.03 (from 0.95) to prevent clipping into the forehead/mesh
        glTranslatef(ex, ey * 1.03f, eyeZ); 
        glRotatef(thetaBase * 180.0f / 3.14159f - 90.0f, 0, 0, 1);
        
        // Mirror the right eye so the eyelashes flair outwards correctly instead of creating a unibrow
        if (i == 1) glScalef(-1.0f, 1.0f, 1.0f);
        
        glColor3f(0.95f, 0.95f, 0.98f);
        glBegin(GL_TRIANGLE_STRIP);
        for(int j=0; j<=20; j++) {
            float a = (j/20.0f) * 3.14159f;
            float vx = 0.10f * cosf(a);
            float vz = 0.055f * sinf(a);
            glVertex3f(vx, 0.005f, vz * 0.95f);
            glVertex3f(vx * 1.05f, 0.005f, vz * 1.05f + 0.003f);
        }
        glEnd();
        
        glBegin(GL_TRIANGLES);
        for(int j=0; j<4; j++) {
            float a = 3.14159f + (j+1.5f)*3.14159f/7.0f;
            float vx = 0.075f * cosf(a);
            float vz = 0.035f * sinf(a);
            glVertex3f(vx, 0.005f, vz);
            glVertex3f(vx + 0.003f, 0.005f, vz - 0.006f);
            glVertex3f(vx - 0.003f, 0.005f, vz - 0.006f);
        }
        glEnd();

        float w = 0.078f;
        float h = 0.042f;
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.0f, 0.1f, 0.3f); // core pupil base
        glVertex3f(0, 0.010f, 0); 
        for (int j = 0; j <= 40; j++) {
            float a = j * 2.0f * 3.14159f / 40.0f;
            float vx = w * cosf(a);
            float vz = h * sinf(a);
            float dist = sqrtf(vx*vx + vz*vz) / h;
            float yGrad = (vz / h); // goes from -1 to 1
            if (dist < 0.25f) {
                glColor3f(0.0f, 0.05f, 0.2f); // core dark
            } else {
                // Lower is bright blue/cyan, upper is dark navy
                float bR = 0.1f + 0.1f * yGrad;
                float bG = 0.5f - 0.3f * yGrad;
                float bB = 1.0f - 0.4f * yGrad;
                
                if (dist > 0.82f) { // Outer iris rim darker
                    bR *= 0.4f; bG *= 0.4f; bB *= 0.4f;
                }
                glColor3f(bR, bG, bB);
            }
            glVertex3f(vx, 0.010f, vz);
        }
        glEnd();
        
        glColor3f(0.01f, 0.005f, 0.04f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0, 0.011f, 0);
        for(int j=0; j<=20; j++) {
            float a = j * 2.0f * 3.14159f / 20.0f;
            glVertex3f(0.022f * cosf(a), 0.011f, 0.018f * sinf(a));
        }
        glEnd();
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.15f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-0.015f, 0.012f, 0.020f);
        for(int j=0; j<=16; j++){
            float a = j * 2.0f * 3.14159f / 16.0f;
            glVertex3f(-0.015f+0.020f*cosf(a), 0.012f, 0.020f+0.020f*sinf(a));
        }
        glEnd();
        glDisable(GL_BLEND);

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-0.018f, 0.014f, 0.022f);
        for(int j=0; j<=16; j++){
            float a = j * 2.0f * 3.14159f / 16.0f;
            glVertex3f(-0.018f + 0.008f*cosf(a), 0.014f, 0.022f + 0.008f*sinf(a));
        }
        glEnd();
        
        float glints[3][3] = {
            {0.022f, -0.012f, 0.015f},  // x, z, radius (main glint)
            {-0.025f, 0.010f, 0.008f},  // side highlight
            {0.010f, -0.028f, 0.006f}   // bottom tiny accent
        };
        for(int g=0; g<3; g++){
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex3f(glints[g][0], 0.014f, glints[g][1]);
            for(int j=0; j<=16; j++){
                float a = j * 2.0f * 3.14159f / 16.0f;
                float r = glints[g][2];
                glVertex3f(glints[g][0]+r*cosf(a), 0.014f, glints[g][1]+r*sinf(a));
            }
            glEnd();
        }

        // Add eyelashes and eyeliner
        glPushMatrix();
        glColor3f(0.05f, 0.02f, 0.06f); // dark purplish black for lashes
        // Sweeping Lash spikes
        glBegin(GL_TRIANGLES);
        for(int l=0; l<7; l++) {
            float f = (float)l/6.0f; // 0 to 1
            float startA = (0.15f + 0.85f * f) * 3.14159f;
            float bx = 0.10f * cosf(startA);
            float bz = 0.055f * sinf(startA);
            
            float lashLen = 0.035f + 0.040f * powf(1.0f - f, 2.0f); // Longer at outer edge (f near 0)
            float lashSweep = 0.045f * (1.0f - f); // Sweeps outwards
            
            glVertex3f(bx, 0.015f, bz);
            glVertex3f(bx + 0.008f, 0.015f, bz - 0.004f);
            glVertex3f(bx + lashSweep, 0.020f, bz + lashLen);
        }
        glEnd();
        
        // Eyeliner thick band
        glBegin(GL_QUAD_STRIP);
        for(int j=0; j<=20; j++) {
            float a = (j/20.0f) * 3.14159f;
            float vx = 0.10f * cosf(a);
            float vz = 0.055f * sinf(a);
            float thick = 0.006f + 0.014f * powf(1.0f - (float)j/20.0f, 2.0f); // Thicker line on outer edge
            glVertex3f(vx * 0.98f, 0.012f, vz * 0.98f);
            glVertex3f(vx * 1.15f, 0.014f, vz + thick);
        }
        glEnd();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.10f); 
        glRotatef(i == 0 ? -5.0f : 5.0f, 0, 1, 0); 
        glColor3f(0.04f, 0.04f, 0.05f);
        int bSteps = 16;
        float bWidth = 0.22f; 
        glBegin(GL_QUAD_STRIP);
        for(int j=0; j<=bSteps; j++){
            float f = (float)j/bSteps;
            float bx = -0.11f + f * bWidth;
            float arch = 0.025f * (1.0f - powf(2.0f * f - 1.0f, 2.0f));
            float thick = 0.005f * (1.0f - 0.4f * fabsf(2.0f * f - 1.0f));
            glVertex3f(bx, 0.01f, arch);
            glVertex3f(bx, 0.01f, arch + thick);
        }
        glEnd();
        glPopMatrix();
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
}

void drawLips() {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING); 
    
    glDisable(GL_LIGHTING);
    glColor3f(0.01f, 0.01f, 0.02f);
    int sSteps = 48;
    float slitWidth = 0.16f; // Smaller mouth
    glBegin(GL_QUAD_STRIP);
    for(int j=0; j<=sSteps; j++) {
        float f = (float)j/sSteps;
        float theta = 3.14159f * 0.5f + (-1.0f + 2.0f * f) * slitWidth;
        float z = 0.198f + 0.015f * powf(fabsf(-1.0f + 2.0f * f), 2.0f); // More pronounced smile curve
        float px, py, pz, nx, ny, nz;
        getHeadVertex(z, theta, px, py, pz);
        getHeadNormal(z, theta, nx, ny, nz);
        glVertex3f(px + nx * 0.0050f, py + ny * 0.0050f, pz + nz * 0.0050f);
        glVertex3f(px + nx * 0.0065f, py + ny * 0.0065f, pz + nz * 0.0065f);
    }
    glEnd();

    glEnable(GL_LIGHTING);
    glColor3f(0.95f, 0.65f, 0.6f); 
    GLfloat lipSpec[] = { 0.5f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, lipSpec);
    glMateriali(GL_FRONT, GL_SHININESS, 32);

    struct LipPart { float zCenter, zHalf, tMax; bool isUpper; };
    LipPart parts[2] = {
        {0.208f, 0.010f, 0.14f, true},
        {0.192f, 0.006f, 0.12f, false}
    };

    for (int p = 0; p < 2; p++) {
        int lSlices = 32;
        int lStacks = 8;
        for (int i = 0; i < lStacks; i++) {
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= lSlices; j++) {
                float fTheta = (-1.0f + 2.0f * j / lSlices);
                float theta = 3.14159f * 0.5f + fTheta * parts[p].tMax;
                float factor = sqrtf(max(0.0f, 1.0f - powf(fTheta, 2.0f)));
                float zBot = parts[p].zCenter - parts[p].zHalf * factor;
                float zTop = parts[p].zCenter + parts[p].zHalf * factor;
                
                // Add smile curve to lips
                float smileCurve = 0.015f * powf(fTheta, 2.0f);
                zBot += smileCurve;
                zTop += smileCurve;
                
                if (parts[p].isUpper) {
                    float bowDip = 0.005f * expf(-powf(theta - 3.14159f*0.5f, 2.0f)/0.0015f);
                    zTop -= bowDip;
                }
                float curP1 = (float)i/lStacks;
                float curP2 = (float)(i+1)/lStacks;
                float curZ1 = zBot + curP1 * (zTop - zBot);
                float curZ2 = zBot + curP2 * (zTop - zBot);
                float px1, py1, pz1, nx1, ny1, nz1, px2, py2, pz2, nx2, ny2, nz2;
                getHeadVertex(curZ1, theta, px1, py1, pz1);
                getHeadNormal(curZ1, theta, nx1, ny1, nz1);
                getHeadVertex(curZ2, theta, px2, py2, pz2);
                getHeadNormal(curZ2, theta, nx2, ny2, nz2);
                glNormal3f(nx1, ny1, nz1);
                glVertex3f(px1+nx1*0.0035f, py1+ny1*0.0035f, pz1+nz1*0.0035f);
                glNormal3f(nx2, ny2, nz2);
                glVertex3f(px2+nx2*0.0035f, py2+ny2*0.0035f, pz2+nz2*0.0035f);
            }
            glEnd();
        }
    }
    GLfloat defSpec[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, defSpec);
    glMateriali(GL_FRONT, GL_SHININESS, 64);
    glEnable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------------
// Hair Model (from head3.cpp)
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

void emitT(Vec3 p0, Vec3 n0, Vec3 p1, Vec3 n1, Vec3 p2, Vec3 n2) {
    glBegin(GL_TRIANGLES);
    glNormal3f(n0.x, n0.y, n0.z); glVertex3f(p0.x, p0.y, p0.z);
    glNormal3f(n1.x, n1.y, n1.z); glVertex3f(p1.x, p1.y, p1.z);
    glNormal3f(n2.x, n2.y, n2.z); glVertex3f(p2.x, p2.y, p2.z);
    glEnd();
}

void setHairColor(float shade, float tint) {
    float r = (0.10f + 0.10f * tint) * shade;
    float g = (0.35f + 0.20f * tint) * shade;
    float b = (0.85f + 0.15f * tint) * shade;
    glColor3f(r, g, b);
}

void setHairColorBright(float shade, float tint) {
    float r = (0.25f + 0.15f * tint) * shade;
    float g = (0.55f + 0.20f * tint) * shade;
    float b = (1.0f  + 0.0f  * tint) * shade;
    glColor3f(r, g, b);
}

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
        int r0 = ri - 1 < 0 ? 0 : ri - 1;
        int r1 = ri;
        int r2 = ri + 1 >= hp.numRings ? hp.numRings - 1 : ri + 1;
        int r3 = ri + 2 >= hp.numRings ? hp.numRings - 1 : ri + 2;

        float oOut = catRom(rt, hp.rings[r0].offsetOut, hp.rings[r1].offsetOut, hp.rings[r2].offsetOut, hp.rings[r3].offsetOut);
        float oZ = catRom(rt, hp.rings[r0].offsetZ, hp.rings[r1].offsetZ, hp.rings[r2].offsetZ, hp.rings[r3].offsetZ);
        float wMul = catRom(rt, hp.rings[r0].widthMul, hp.rings[r1].widthMul, hp.rings[r2].widthMul, hp.rings[r3].widthMul);
        float bulge = catRom(rt, hp.rings[r0].bulge, hp.rings[r1].bulge, hp.rings[r2].bulge, hp.rings[r3].bulge);

        for (int c = 0; c < cols; ++c) {
            float u = (float)c / (float)wSegs;

            float theta = hp.thetaStart + (hp.thetaEnd - hp.thetaStart) * u;
            float centerTheta = (hp.thetaStart + hp.thetaEnd) * 0.5f;
            theta = centerTheta + (theta - centerTheta) * wMul;
            theta += hp.thetaSweep * v * v; // Dynamic sweep outwards over length

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

        if (hp.bright) setHairColorBright(shade, hp.colorTint);
        else setHairColor(shade, hp.colorTint);

        for (int c = 0; c < wSegs; ++c) {
            int i00 = r * cols + c;
            int i10 = r * cols + c + 1;
            int i01 = (r + 1) * cols + c;
            int i11 = (r + 1) * cols + c + 1;

            emitQ(pts[i00], nrm[i00], pts[i10], nrm[i10], pts[i11], nrm[i11], pts[i01], nrm[i01]);
            Vec3 fn00 = v3scale(nrm[i00], -1);
            Vec3 fn10 = v3scale(nrm[i10], -1);
            Vec3 fn11 = v3scale(nrm[i11], -1);
            Vec3 fn01 = v3scale(nrm[i01], -1);
            float bshade = shade * 0.40f;
            if (hp.bright) setHairColorBright(bshade, hp.colorTint);
            else setHairColor(bshade, hp.colorTint);
            emitQ(pts[i01], fn01, pts[i11], fn11, pts[i10], fn10, pts[i00], fn00);

            if (hp.bright) setHairColorBright(shade, hp.colorTint);
            else setHairColor(shade, hp.colorTint);
        }
    }

    delete[] pts;
    delete[] nrm;
}

void drawBackHair() {
    const int numPanels = 10;
    for (int i = 0; i < numPanels; ++i) {
        float centerAngle = PI * 1.5f + PI * 0.85f * ((float)i / (numPanels - 1) - 0.5f);
        float halfWidth = PI / (float)numPanels * 1.5f;

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
        float sideSign = (side == 0) ? 1.0f : -1.0f;
        const int numPanels = 8;
        for (int i = 0; i < numPanels; ++i) {
            float baseAngle = (side == 0) ? (PI * 0.0f) : (PI * 1.0f);
            float spread = PI * 0.40f * ((float)i / (numPanels - 1) - 0.5f);
            float centerAngle = baseAngle + spread;
            float halfWidth = PI / (float)(numPanels) * 0.8f;

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
            hp.rings[1] = { 0.06f + dv, -0.06f,  1.0f,  0.05f };
            hp.rings[2] = { 0.10f + dv, -0.22f,  0.92f, 0.04f };
            hp.rings[3] = { 0.05f + dv, -0.45f,  0.82f, 0.03f };
            hp.rings[4] = { 0.12f + dv, -0.72f,  0.68f, 0.02f };
            hp.rings[5] = { 0.05f + dv, -totalDrop * 0.85f, 0.42f, 0.01f };
            hp.rings[6] = { 0.10f + dv, -totalDrop,         0.18f, 0.00f };

            drawPanel(hp);
        }
    }
}

void drawBangs() {
    // 1. Middle V-shape bangs (covers forehead)
    const int numMid = 8;
    for (int i = 0; i < numMid; ++i) {
        float t = (float)i / (numMid - 1); 
        float centerAngle = PI * 0.5f + PI * 0.14f * (t - 0.5f); // Spread across forehead
        float halfWidth = PI * 0.04f; 

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.01f + rr(i * 83, 0, 0.01f);
        hp.phiConform = PI * 0.22f + rr(i * 89, -0.01f, 0.01f);
        hp.widthSegs = 4;
        hp.lengthSegs = 10;
        hp.colorTint = rr(i * 97, 0.2f, 0.6f);
        hp.colorShade = 0.85f + rr(i * 101, 0, 0.10f);
        hp.bright = 0;
        hp.thetaSweep = 0.0f; // Straight down, no side-sweep

        // V-shape: center strands (t~0.5) are LONGEST, edges are shorter
        float distFromCenter = fabsf(t - 0.5f) * 2.0f;
        float bangLen = 0.45f - 0.15f * distFromCenter; // Center: 0.45, Edges: 0.30

        hp.numRings = 5;
        hp.rings[0] = { 0.05f, 0.0f,   1.0f,  0.02f };
        hp.rings[1] = { 0.07f, -0.04f,  1.0f,  0.03f };
        hp.rings[2] = { 0.09f, -bangLen * 0.4f,  0.90f, 0.02f };
        hp.rings[3] = { 0.07f, -bangLen * 0.75f,  0.55f, 0.01f };
        hp.rings[4] = { 0.03f, -bangLen,          0.15f, 0.00f }; 

        drawPanel(hp);
    }

    // 2. Left Bangs (Sweeping Outwards)
    const int numLeft = 4;
    for (int i = 0; i < numLeft; ++i) {
        float t = (float)i / (numLeft - 1);
        float centerAngle = PI * 0.5f - PI * 0.08f - PI * 0.14f * t; 
        float halfWidth = PI * 0.06f; 

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.02f + rr(i * 103, 0, 0.02f);
        hp.phiConform = PI * 0.20f; 
        hp.widthSegs = 4;
        hp.lengthSegs = 10;
        hp.colorTint = rr(i * 113, 0.1f, 0.6f);
        hp.colorShade = 0.85f + rr(i * 127, 0, 0.10f);
        hp.bright = 0;
        
        hp.thetaSweep = -0.45f - 0.25f * t; 

        float bangLen = 0.15f + 0.10f * t; // Reduced length significantly        
        hp.numRings = 5;
        hp.rings[0] = { 0.05f, 0.0f,   1.0f,  0.02f };
        hp.rings[1] = { 0.07f, -0.04f,  1.0f,  0.03f };
        hp.rings[2] = { 0.10f, -bangLen * 0.4f,  0.90f, 0.02f };
        hp.rings[3] = { 0.08f, -bangLen * 0.8f,  0.70f, 0.01f };
        hp.rings[4] = { 0.04f, -bangLen,          0.20f, 0.00f };

        drawPanel(hp);
    }

    // 3. Right Bangs (Sweeping Outwards)
    const int numRight = 4;
    for (int i = 0; i < numRight; ++i) {
        float t = (float)i / (numRight - 1);
        float centerAngle = PI * 0.5f + PI * 0.08f + PI * 0.14f * t; 
        float halfWidth = PI * 0.06f; 

        HairPanel hp;
        hp.thetaStart = centerAngle - halfWidth;
        hp.thetaEnd = centerAngle + halfWidth;
        hp.phiStart = PI * 0.02f + rr(i * 131, 0, 0.02f);
        hp.phiConform = PI * 0.20f; 
        hp.widthSegs = 4;
        hp.lengthSegs = 10;
        hp.colorTint = rr(i * 139, 0.1f, 0.6f);
        hp.colorShade = 0.85f + rr(i * 149, 0, 0.10f);
        hp.bright = 0;
        
        hp.thetaSweep = 0.45f + 0.25f * t;

        float bangLen = 0.15f + 0.10f * t; // Reduced length significantly
        hp.numRings = 5;
        hp.rings[0] = { 0.05f, 0.0f,   1.0f,  0.02f };
        hp.rings[1] = { 0.07f, -0.04f,  1.0f,  0.03f };
        hp.rings[2] = { 0.10f, -bangLen * 0.4f,  0.90f, 0.02f };
        hp.rings[3] = { 0.08f, -bangLen * 0.8f,  0.70f, 0.01f };
        hp.rings[4] = { 0.04f, -bangLen,          0.20f, 0.00f };

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

void drawAccentStrands() {
    const int numStrands = 12;
    for (int i = 0; i < numStrands; ++i) {
        float angle = PI * 1.0f + PI * 1.0f * hf(i * 149);
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
        hp.rings[4] = { 0.04f + dv,  -strandLen + 0.06f, 0.15f, 0.00f };

        drawPanel(hp);
    }
}

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

void drawSideFringe() {
    for (int side = 0; side < 2; ++side) {
        float baseAngle = (side == 0) ? (PI * 0.12f) : (PI * 0.88f); // Pushed further back from the face
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
        hp.rings[5] = { 0.04f + dv, -totalDrop + 0.06f,  0.15f, 0.00f };

        drawPanel(hp);
    }
}

void drawRabbitEars() {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        // Position ears on the top-sides of the head
        glTranslatef(side * 0.14f, 0.04f, 0.88f);
        glRotatef(side * 18.0f, 0, 1, 0); // Tilt outwards
        glRotatef(-8.0f, 1, 0, 0);        // Tilt slightly back

        const int stacks = 50;
        const int slices = 36;
        float height = 0.70f;  // Shorter rabbit ears
        
        for (int i = 0; i < stacks; i++) {
            float f1 = (float)i / stacks;
            float f2 = (float)(i + 1) / stacks;
            
            float z1 = f1 * height;
            float z2 = f2 * height;
            
            // Custom width profile:
            // NARROW at base, WIDEST at ~60% height, then smooth rounded U at the top
            float peakPos = 0.60f;
            float maxWidth = 0.15f;
            float earWidth1, earWidth2;
            
            if (f1 < peakPos) {
                // Base to peak: gradual expansion (narrow base, wider top)
                float t = f1 / peakPos;
                earWidth1 = maxWidth * (0.35f + 0.65f * powf(t, 0.7f));
            } else {
                // Peak to tip: gentle round-off for U-shape
                float t = (f1 - peakPos) / (1.0f - peakPos);
                earWidth1 = maxWidth * (0.30f + 0.70f * cosf(t * 3.14159f * 0.5f));
            }
            
            if (f2 < peakPos) {
                float t = f2 / peakPos;
                earWidth2 = maxWidth * (0.35f + 0.65f * powf(t, 0.7f));
            } else {
                float t = (f2 - peakPos) / (1.0f - peakPos);
                earWidth2 = maxWidth * (0.30f + 0.70f * cosf(t * 3.14159f * 0.5f));
            }
            
            // Gentle backward curve
            float curve1 = 0.12f * f1 * f1;
            float curve2 = 0.12f * f2 * f2;

            // Thickness profile
            float thick1 = 0.035f * sinf(f1 * 3.14159f * 0.85f + 0.15f);
            float thick2 = 0.035f * sinf(f2 * 3.14159f * 0.85f + 0.15f);

            if (i == stacks - 1) {
                // Rounded dome cap (inverted U) instead of a sharp point
                glBegin(GL_TRIANGLE_FAN);
                glColor3f(1.0f, 1.0f, 1.0f);
                // Center of the dome sits slightly above the last ring
                glNormal3f(0, 0, 1);
                glVertex3f(0, -curve2 - 0.005f, z2 + earWidth2 * 0.4f);
                for (int j = 0; j <= slices; j++) {
                    float th = (float)j / slices * 2.0f * 3.14159f;
                    glColor3f(1.0f, 1.0f, 1.0f);
                    glNormal3f(cosf(th) * 0.3f, sinf(th) * 0.3f, 0.9f);
                    glVertex3f(earWidth1 * cosf(th), thick1 * sinf(th) - curve1, z1);
                }
                glEnd();
            } else {
                glBegin(GL_QUAD_STRIP);
                for (int j = 0; j <= slices; j++) {
                    float th = (float)j / slices * 2.0f * 3.14159f;
                    float cosTh = cosf(th);
                    float sinTh = sinf(th);
                    
                    // Pink intensity: only on the FRONT face (sinTh > threshold)
                    // with thick white borders on left/right edges
                    float faceFactor = max(0.0f, sinTh - 0.50f) / 0.50f;
                    // Height: no pink at very base or very tip
                    float hFact = clampf(f1 * 4.0f, 0, 1) * clampf((1.0f - f1) * 3.0f, 0, 1);
                    float pinkIntensity = faceFactor * hFact;
                    
                    // Warm pink color
                    float cr = 1.0f;
                    float cg = 1.0f - 0.30f * pinkIntensity;
                    float cb = 1.0f - 0.22f * pinkIntensity;
                    glColor3f(cr, cg, cb);

                    // Recess the pink area inward for 3D rim effect
                    float depthMul = 1.0f - 0.55f * pinkIntensity;
                    float rd1 = thick1 * depthMul;
                    float rd2 = thick2 * depthMul;

                    glNormal3f(cosTh, sinTh * (1.0f - 0.7f * pinkIntensity), 0.15f);
                    glVertex3f(earWidth2 * cosTh, rd2 * sinTh - curve2, z2);
                    glVertex3f(earWidth1 * cosTh, rd1 * sinTh - curve1, z1);
                }
                glEnd();
            }
        }
        glPopMatrix();
    }
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void drawHairModel() {
    glDisable(GL_CULL_FACE);
    drawRabbitEars(); // Add the ears to the model
    drawTopHair();
    drawBackHair();
    drawSideHair();   // Dense bulk on the sides
    drawLayerStrands();
    // drawSideFringe(); 
    drawBangs();
    drawAccentStrands();
}

// ----------------------------------------------------------------
// Display & WinMain
// ----------------------------------------------------------------

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    updateCamera();
    gluLookAt(cameraX, cameraY, cameraZ, 0.0, 0.4, 0.0, 0.0, 1.0, 0.0);

    // --- RENDER HEAD ---
    glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
    glDisable(GL_BLEND); // Head is opaque
    setupLightingHead(); // Called BEFORE model transformations
    
    glPushMatrix();
    
    // Position the head in world space relative to the hair's origin
    // Retracting the previous shift so the face isn't completely buried
    glTranslatef(0.0f, 0.55f, -0.15f);  
    
    // Head.obj naturally faces +Z, but the hair and old head faced -Z
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f); 
    
    // Scale down the object linearly
    glScalef(0.145f, 0.145f, 0.145f);
    
    // Optional internal offset to center exactly
    glTranslatef(0.0f, -0.2f, -0.5f);
    
    static GLuint objModelList = 0;
    if (objModelList == 0) {
        bindOBJToDisplayList("Head.obj", objModelList);
    }
    
    // Reset color to white so we don't accidentally tint the skin
    glColor3f(1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, texHeadObj);
    glCallList(objModelList);
    
    glPopMatrix(); // End Head.obj transform
    
    glPopAttrib();

    // --- RENDER HAIR ---
    glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setupLightingHair(); // Set hair lighting in view space
    
    glPushMatrix();
    // Rotate to match head upright vector 
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    
    // Position vertically on the head 
    glTranslatef(0.0f, 0.0f, 0.36f); 
    
    // Scale the entire hair mesh system down to fit the smaller head
    // X is scaled a bit more to ensure it hugs the cheeks nicely
    glScalef(0.80f, 0.85f, 0.85f); 
    
    glDisable(GL_TEXTURE_2D); 
    drawHairModel();
    glPopMatrix();
    glPopAttrib();

    SwapBuffers(wglGetCurrentDC());
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize = sizeof(wc);
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = WindowProcedure;
	wc.lpszClassName = WINDOW_TITLE;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	if (!RegisterClassEx(&wc)) return false;

	HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL, NULL, wc.hInstance, NULL);

	HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return false;
	HGLRC hglrc = wglCreateContext(hdc);
	if (!wglMakeCurrent(hdc, hglrc)) return false;

	initOpenGL();
	
	texSkin = loadBMP("skin.bmp");
	texHeadObj = loadBMP("head.bmp"); // load the converted texture

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (true) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
            display();
        }
	}

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hWnd, hdc);
	UnregisterClass(WINDOW_TITLE, wc.hInstance);
	return true;
}
