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

#define SEC_COUNT 8
CrossSection sections[SEC_COUNT] = {
    {-0.3f, 0.18f, 0.16f,  0.0f},   // under chin margin
    {-0.1f, 0.28f, 0.25f,  0.02f},  // jaw/chin
    { 0.1f, 0.34f, 0.30f,  0.04f},  // mouth/lower cheeks
    { 0.3f, 0.38f, 0.34f,  0.05f},  // cheeks/nose base
    { 0.5f, 0.38f, 0.36f,  0.04f},  // eyes level
    { 0.7f, 0.36f, 0.38f,  0.02f},  // forehead
    { 0.9f, 0.24f, 0.27f,  0.00f},  // crown
    { 1.1f, 0.10f, 0.10f,  0.0f}    // margin
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
}

void getHeadVertex(float z, float theta, float &px, float &py, float &pz) {
    float rx, ry, yOfs;
    getHeadBaseRadius(z, rx, ry, yOfs);
    
    // Normalize theta to 0..2PI
    while(theta < 0) theta += 2.0f*3.14159f;
    while(theta > 2.0f*3.14159f) theta -= 2.0f*3.14159f;

    // Face is at theta = PI/2 (Y axis goes forward, X axis goes right)
    float bump = 0.0f;
    
    // Only apply face details if in the front half to keep the back perfectly smooth
    if (theta > 0.0f && theta < 3.14159f) {
        // Nose - made smaller and less stretched
        float noseZ = 0.32f;
        float noseBump = 0.08f * exp(-(pow(z - noseZ, 2) / 0.008f + pow(theta - 3.14159f*0.5f, 2) / 0.015f));
        bump += noseBump;

        // Eye sockets (indentations) - sharper and clearer
        float eyeZ = 0.45f;
        float eyeIndent = -0.06f * exp(-(pow(z - eyeZ, 2) / 0.01f + pow(theta - 3.14159f*0.38f, 2) / 0.015f));
        eyeIndent += -0.06f * exp(-(pow(z - eyeZ, 2) / 0.01f + pow(theta - 3.14159f*0.62f, 2) / 0.015f));
        bump += eyeIndent;

        // Chin bump
        float chinZ = -0.05f;
        float chinBump = 0.04f * exp(-(pow(z - chinZ, 2) / 0.01f + pow(theta - 3.14159f*0.5f, 2) / 0.04f));
        bump += chinBump;
        
        // Eyebrow ridge
        float ridgeZ = 0.52f;
        float ridgeBump = 0.02f * exp(-(pow(z - ridgeZ, 2) / 0.008f + pow(theta - 3.14159f*0.5f, 2) / 0.08f));
        bump += ridgeBump;
        
        // Under lip indentation
        float underLipZ = 0.10f;
        bump -= 0.02f * exp(-(pow(z - underLipZ, 2) / 0.005f + pow(theta - 3.14159f*0.5f, 2) / 0.03f));

        // Mouth bump (lips)
        float mouthZ = 0.16f;
        bump += 0.02f * exp(-(pow(z - mouthZ, 2) / 0.003f + pow(theta - 3.14159f*0.5f, 2) / 0.02f));
    }

    // Ear bumps (theta = 0 and PI)
    float earZ = 0.35f;
    float earBump = 0.07f * exp(-(pow(z - earZ, 2) / 0.015f + pow(theta - 0.0f, 2) / 0.03f));
    earBump += 0.07f * exp(-(pow(z - earZ, 2) / 0.015f + pow(theta - 3.14159f, 2) / 0.03f));
    bump += earBump;

    rx += bump;
    ry += bump;
    
    px = rx * cos(theta);
    py = ry * sin(theta) + yOfs;
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
    
    float len = sqrt(nx*nx + ny*ny + nz*nz);
    if(len > 0) { nx /= len; ny /= len; nz /= len; }
}

void drawHeadMesh() {
    int stacks = 80;
    int slices = 80;
    
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

	glPushMatrix();
    // Rotate to make Z up, since we modeled with Z as up
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0f, 0.0f, 0.5f); // Move up a bit for centering
    drawHeadMesh();
    // drawHair(); // Optional, from main.cpp. 
	glPopMatrix();
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