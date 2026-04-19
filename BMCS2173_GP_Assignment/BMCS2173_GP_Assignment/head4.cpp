#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma warning(disable:4996)
#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "GLU32.lib")

#define WINDOW_TITLE "Head Part"

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

void updateCamera()
{
	cameraX = sinf(cameraAngle) * cameraDistance;
	cameraZ = cosf(cameraAngle) * cameraDistance;
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
	glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2); // Rim light for high fidelity
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glShadeModel(GL_SMOOTH);

	GLfloat ambientLight[] = { 0.20f, 0.20f, 0.20f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

	GLfloat lightPos0[] = { 4.0f, 5.0f, 5.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

    GLfloat lightPos1[] = { -4.0f, 2.0f, -1.0f, 1.0f };
    GLfloat lightDiff1[] = { 0.3f, 0.3f, 0.4f, 1.0f }; // Soft cool fill
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiff1);
    
    GLfloat lightPos2[] = { 0.0f, 5.0f, -5.0f, 1.0f }; // Rim light from behind
    GLfloat lightDiff2[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, lightPos2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, lightDiff2);
    
    // Advanced material properties
    GLfloat specColor[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specColor);
    glMateriali(GL_FRONT, GL_SHININESS, 64); // Sharper highlights
}

// Cubic Catmull-Rom interpolation for smooth curves
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

// Head Keyframes (from bottom to top)
struct CrossSection {
    float z;
    float rx;
    float ry;
    float yOffset;
};

#define SEC_COUNT 10
CrossSection sections[SEC_COUNT] = {
    {-0.15f, 0.10f, 0.09f,  0.00f},  // neck top (slightly thicker)
    { 0.00f, 0.14f, 0.11f,  0.02f},  // jaw base (smoother transition)
    { 0.15f, 0.28f, 0.26f,  0.08f},  // lower jaw
    { 0.35f, 0.38f, 0.34f,  0.10f},  // cheeks
    { 0.52f, 0.40f, 0.38f,  0.08f},  // eye level
    { 0.68f, 0.38f, 0.37f,  0.05f},  // temples
    { 0.82f, 0.35f, 0.35f,  0.02f},  // forehead
    { 0.92f, 0.28f, 0.30f,  0.01f},  // crown start
    { 0.98f, 0.12f, 0.15f,  0.00f},  // top curve
    { 1.05f, 0.00f, 0.00f,  0.00f}   // spline end
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
    
    // Normalize theta to 0..2PI
    while(theta < 0) theta += 2.0f*3.14159f;
    while(theta > 2.0f*3.14159f) theta -= 2.0f*3.14159f;

    // Face is at theta = PI/2 (Y axis goes forward, X axis goes right)
    float bump = 0.0f;
    
    // Front and Back half logic
    float frontSide = sinf(theta); // > 0 is front
    float sideFactor = fabsf(cosf(theta)); // 1.0 at sides (0, PI), 0.0 at center (PI/2, 3PI/2)

    // 1. V-Jaw Pinch (Front lower half)
    if (z < 0.5f && frontSide > 0.0f) {
        float jawDepth = (0.5f - z) / 0.8f;
        float pinch = jawDepth * (1.0f - fabsf(theta - 3.14159f*0.5f) / (3.14159f*0.5f));
        // Reduced pinch for a rounder triangle jaw
        rx *= (1.0f - 0.22f * pinch);
        ry *= (1.0f - 0.08f * pinch);
    }

    // 2. Cranium Expansion (Back half)
    if (frontSide < 0.0f) {
        float backFactor = -frontSide; // 1.0 at the direct back
        float zHeight = (z - sections[0].z) / (sections[SEC_COUNT-1].z - sections[0].z);
        float expand = 0.12f * backFactor * expf(-powf(zHeight - 0.6f, 2.0f) / 0.1f);
        rx += expand;
        ry += expand;
    }

    // 3. Facial Features (Bumps)
    if (theta > 0.0f && theta < 3.14159f) {
        // Precise anime nose (integrated bridge and tip)
        float noseBridgeZ = 0.38f;
        float noseBridge = 0.035f * expf(-(powf(z - noseBridgeZ, 2.0f) / 0.010f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.006f));
        float noseTipZ = 0.32f;
        float noseTip = 0.050f * expf(-(powf(z - noseTipZ, 2.0f) / 0.002f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.010f));
        bump += noseBridge + noseTip;

        // Deepened anime eye sockets
        float eyeZ = 0.48f;
        float eyeIndent = -0.085f * expf(-(powf(z - eyeZ, 2.0f) / 0.024f + powf(theta - 3.14159f*0.35f, 2.0f) / 0.038f));
        eyeIndent += -0.085f * expf(-(powf(z - eyeZ, 2.0f) / 0.024f + powf(theta - 3.14159f*0.65f, 2.0f) / 0.038f));
        
        // Flatten eye landing area slightly
        float irisArea = 0.015f * expf(-(powf(z - eyeZ, 2.0f) / 0.008f + powf(theta - 3.14159f*0.35f, 2.0f) / 0.015f));
        irisArea += 0.015f * expf(-(powf(z - eyeZ, 2.0f) / 0.008f + powf(theta - 3.14159f*0.65f, 2.0f) / 0.015f));
        
        bump += eyeIndent + irisArea;

        // Sharp chin refinement
        float chinZ = 0.05f;
        float chinBump = 0.025f * expf(-(powf(z - chinZ, 2.0f) / 0.008f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.030f));
        bump += chinBump;
        
        // Detailed Lip System (with lip corners) - Thick Top, Thin Bottom
        float upperLipZ = 0.208f;
        float lowerLipZ = 0.192f;
        float upperLip = 0.012f * expf(-(powf(z - upperLipZ, 2.0f) / 0.0015f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.10f));
        float lowerLip = 0.006f * expf(-(powf(z - lowerLipZ, 2.0f) / 0.0004f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.08f));
        float mouthIndent = -0.004f * expf(-(powf(z - 0.198f, 2.0f) / 0.0002f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.12f));
        
        // Lip corners (micro-shadows) - Widened
        float cornerThetaL = 3.14159f * 0.5f + 0.22f;
        float cornerThetaR = 3.14159f * 0.5f - 0.22f;
        float lipCorner = -0.005f * expf(-(powf(z - 0.197f, 2.0f) / 0.001f + powf(theta - cornerThetaL, 2.0f) / 0.010f));
        lipCorner += -0.005f * expf(-(powf(z - 0.197f, 2.0f) / 0.001f + powf(theta - cornerThetaR, 2.0f) / 0.010f));
        
        bump += upperLip + lowerLip + mouthIndent + lipCorner;

        // Nostrils (micro-indents)
        float nostrilZ = 0.31f;
        float nostrilL = -0.008f * expf(-(powf(z - nostrilZ, 2.0f) / 0.001f + powf(theta - (3.14159f*0.5f + 0.04f), 2.0f) / 0.006f));
        float nostrilR = -0.008f * expf(-(powf(z - nostrilZ, 2.0f) / 0.001f + powf(theta - (3.14159f*0.5f - 0.04f), 2.0f) / 0.006f));
        bump += nostrilL + nostrilR;
        
        // Eye lid creases (orbitals)
        float creaseZ = 0.54f;
        float creaseBump = 0.006f * expf(-(powf(z - creaseZ, 2.0f) / 0.0005f + powf(theta - 3.14159f*0.5f, 2.0f) / 0.15f));
        bump += creaseBump;

        // Cheekbone highlights
        float cheekZ = 0.38f;
        float cheekBump = 0.015f * expf(-(powf(z - cheekZ, 2.0f) / 0.03f + powf(theta - 3.14159f*0.3f, 2.0f) / 0.05f));
        cheekBump += 0.015f * expf(-(powf(z - cheekZ, 2.0f) / 0.03f + powf(theta - 3.14159f*0.7f, 2.0f) / 0.05f));
        bump += cheekBump;
    }

    /* 
    // Ear bumps removed as per user request
    float earZ = 0.35f;
    float earBump = 0.07f * expf(-(powf(z - earZ, 2.0f) / 0.015f + powf(theta - 0.0f, 2.0f) / 0.03f));
    earBump += 0.07f * expf(-(powf(z - earZ, 2.0f) / 0.015f + powf(theta - 3.14159f, 2.0f) / 0.03f));
    bump += earBump;
    */

    // Scale facial features bumps proportionally with the head
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
    
    // Tangents
    float tx1 = px1 - px2, ty1 = py1 - py2, tz1 = pz1 - pz2;
    float tx2 = px3 - px4, ty2 = py3 - py4, tz2 = pz3 - pz4;
    
    // Cross Product (tx2 x tx1 because of the parameterization direction)
    nx = ty2 * tz1 - tz2 * ty1;
    ny = tz2 * tx1 - tx2 * tz1;
    nz = tx2 * ty1 - ty2 * tx1;
    
    float len = sqrtf(nx*nx + ny*ny + nz*nz);
    if(len > 0) { nx /= len; ny /= len; nz /= len; }
}

void drawHeadMesh() {
    // High-fidelity density
    int stacks = 128;
    int slices = 128;
    
    float zMin = sections[0].z;
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
            float s = (float)j / slices * 2.0f; // Tile texture wrapping around
            
            float px2, py2, pz2;
            getHeadVertex(z2, theta, px2, py2, pz2);
            float nx2, ny2, nz2;
            getHeadNormal(z2, theta, nx2, ny2, nz2);
            
            glNormal3f(nx2, ny2, nz2);
            glTexCoord2f(s, f2 * 2.0f);
            glVertex3f(px2, py2, pz2);
            
            float px1, py1, pz1;
            getHeadVertex(z1, theta, px1, py1, pz1);
            float nx1, ny1, nz1;
            getHeadNormal(z1, theta, nx1, ny1, nz1);
            
            glNormal3f(nx1, ny1, nz1);
            glTexCoord2f(s, f1 * 2.0f);
            glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }
    
    // Draw top cap
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
    
    // Draw bottom cap
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
    float eyeZ = 0.52f; 
    getHeadBaseRadius(eyeZ, rx, ry, yOfs);
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    for (int i = 0; i < 2; i++) {
        float thetaBase = (i == 0) ? 0.38f * 3.14159f : 0.62f * 3.14159f;
        
        glPushMatrix();
        float ex = rx * cosf(thetaBase);
        float ey = ry * sinf(thetaBase) + yOfs;
        glTranslatef(ex, ey * 0.95f, eyeZ); 
        glRotatef(thetaBase * 180.0f / 3.14159f - 90.0f, 0, 0, 1);
        
        // --- 1. Almond Shape Eyeliner (Flattened Model) ---
        glColor3f(0.06f, 0.05f, 0.08f);
        glBegin(GL_TRIANGLE_STRIP);
        for(int j=0; j<=20; j++) {
            float a = (j/20.0f) * 3.14159f;
            // Widened horizontal (vx: 0.10) and flattened vertical (vz: 0.055)
            float vx = 0.10f * cosf(a);
            float vz = 0.055f * sinf(a);
            glVertex3f(vx, 0.005f, vz * 0.95f);
            glVertex3f(vx * 1.05f, 0.005f, vz * 1.05f + 0.003f);
        }
        glEnd();
        
        // --- 2. Lower Lashes ---
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

        // --- 3. Iris (Flattened/Widened) ---
        float w = 0.078f;
        float h = 0.042f;
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.05f, 0.0f, 0.15f);
        glVertex3f(0, 0.010f, 0); 
        for (int j = 0; j <= 40; j++) {
            float a = j * 2.0f * 3.14159f / 40.0f;
            float vx = w * cosf(a);
            float vz = h * sinf(a);
            float dist = sqrtf(vx*vx + vz*vz) / h;
            if (dist < 0.3f) glColor3f(0.15f, 0.35f, 0.9f);
            else if (dist < 0.7f) glColor3f(0.45f, 0.15f, 0.85f);
            else if (dist < 0.9f) glColor3f(0.2f, 0.02f, 0.4f);
            else glColor3f(0.5f, 0.4f, 1.0f);
            glVertex3f(vx, 0.010f, vz);
        }
        glEnd();
        
        // Pupil (Flattened)
        glColor3f(0.01f, 0.005f, 0.04f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0, 0.011f, 0);
        for(int j=0; j<=20; j++) {
            float a = j * 2.0f * 3.14159f / 20.0f;
            glVertex3f(0.022f * cosf(a), 0.011f, 0.018f * sinf(a));
        }
        glEnd();
        
        // --- 4. Glints ---
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
        
        float glints[2][2] = {{0.030f, -0.015f}, {-0.012f, -0.035f}};
        for(int g=0; g<2; g++){
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(glints[g][0], 0.014f, glints[g][1]);
            for(int j=0; j<=8; j++){
                float a = j * 2.0f * 3.14159f / 8.0f;
                glVertex3f(glints[g][0]+0.004f*cosf(a), 0.014f, glints[g][1]+0.004f*sinf(a));
            }
            glEnd();
        }

        // --- 5. Eyebrows (Reduced Gap) ---
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.13f); // Moved down towards eyes (0.21 -> 0.13)
        glRotatef(i == 0 ? -5 : 5, 0, 1, 0); 
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
    
    // 1. Mouth Slit Line
    glDisable(GL_LIGHTING);
    glColor3f(0.01f, 0.01f, 0.02f);
    int sSteps = 48;
    float slitWidth = 0.28f;
    glBegin(GL_QUAD_STRIP);
    for(int j=0; j<=sSteps; j++) {
        float f = (float)j/sSteps;
        float theta = 3.14159f * 0.5f + (-1.0f + 2.0f * f) * slitWidth;
        float z = 0.198f + 0.008f * powf(fabsf(-1.0f + 2.0f * f), 2.0f); 
        float px, py, pz, nx, ny, nz;
        getHeadVertex(z, theta, px, py, pz);
        getHeadNormal(z, theta, nx, ny, nz);
        glVertex3f(px + nx * 0.0050f, py + ny * 0.0050f, pz + nz * 0.0050f);
        glVertex3f(px + nx * 0.0065f, py + ny * 0.0065f, pz + nz * 0.0065f);
    }
    glEnd();

    // 2. Peach Lips
    glEnable(GL_LIGHTING);
    glColor3f(1.0f, 0.72f, 0.76f); 
    GLfloat lipSpec[] = { 0.5f, 0.4f, 0.4f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, lipSpec);
    glMateriali(GL_FRONT, GL_SHININESS, 32);

    struct LipPart { float zCenter, zHalf, tMax; bool isUpper; };
    LipPart parts[2] = {
        {0.208f, 0.010f, 0.22f, true},  // Upper (Thicker)
        {0.192f, 0.006f, 0.19f, false}  // Lower (Thinner)
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

void drawHair()
{
	GLUquadric* quad = gluNewQuadric();
	glPushMatrix();
	glColor3f(0.1f, 0.2f, 0.8f); // blue hair
	glBindTexture(GL_TEXTURE_2D, 0); // Disable texture or map color
	
	// Position at the crown (Z=0.9 in model space, which is top since it is rotated -90 in display)
	glTranslatef(0.0f, 0.0f, 0.65f);
	
	gluCylinder(quad, 0.38, 0.38, 0.3, 32, 32);
	
	// Caps for hair
	glPushMatrix();
	glRotatef(180, 1, 0, 0);
	gluDisk(quad, 0, 0.38, 32, 32);
	glPopMatrix();

	glTranslatef(0.0f, 0.0f, 0.3f);
	gluDisk(quad, 0, 0.38, 32, 32);
	
	glPopMatrix();
	gluDeleteQuadric(quad);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	updateCamera();
	gluLookAt(cameraX, cameraY, cameraZ, 0, 0.7, 0, 0, 1, 0);

	GLfloat lightPosition[] = { 3.0f, 5.0f, 3.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    static GLuint modelList = 0;
    if (modelList == 0) {
        modelList = glGenLists(1);
        glNewList(modelList, GL_COMPILE);
	    glPushMatrix();
        // Rotate to make Z up, since we modeled with Z as up
	    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	    glTranslatef(0.0f, 0.0f, 0.2f); // Centering adjusted for shorter model
        drawHeadMesh();
        drawEyes();
        drawLips();
        // drawHair(); 
	    glPopMatrix();
        glEndList();
    }

    glCallList(modelList);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
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