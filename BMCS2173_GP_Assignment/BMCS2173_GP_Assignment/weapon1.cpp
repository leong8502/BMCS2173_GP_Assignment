#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "GLU32.lib")

#define WINDOW_TITLE "Meteor Hammer (Weapon 1)"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//--------------------------------
// Camera variables
//--------------------------------

float cameraAngle = 0.0f;
float cameraHeight = 0.0f;
float cameraDistance = 10.0f;

float cameraX, cameraY, cameraZ;

//--------------------------------
// Animation variables
//--------------------------------

float spinAngle = 0.0f;

//--------------------------------
// Texture
//--------------------------------

GLuint textureMetalID;
GLuint textureWoodID;
GLuint textureChainID;

void updateCamera()
{
	cameraX = sin(cameraAngle) * cameraDistance;
	cameraZ = cos(cameraAngle) * cameraDistance;
	cameraY = cameraHeight;
}

void resetAll() {
	cameraAngle = 0.0f;
	cameraHeight = 0.0f;
	cameraDistance = 10.0f;

	updateCamera();
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

		case VK_SPACE:
			resetAll();
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

		case VK_ADD: // press + / =
		case VK_OEM_PLUS:
			cameraDistance -= 0.3f;
			break;

		case VK_SUBTRACT: // press - / _
		case VK_OEM_MINUS:
			cameraDistance += 0.3f;
			break;
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
//--------------------------------------------------------------------

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

	if (SetPixelFormat(hdc, n, &pfd))
	{
		return true;
	}
	else
	{
		return false;
	}
}
//--------------------------------------------------------------------

void initOpenGL()
{
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // background color
	glEnable(GL_DEPTH_TEST); // enable depth for 3D

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(
		45.0f,   // field of view
		800.0 / 600.0, // aspect ratio
		0.1f,
		100.0f
	);

	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_TEXTURE_2D);
}

void setupLighting()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// Make the material shiny so the metal looks heavy
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glShadeModel(GL_SMOOTH);

	GLfloat ambientLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

	GLfloat diffuseLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	
	GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	GLfloat lightPosition[] = { 5.0f, 5.0f, 5.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
}

GLuint loadBMPTexture(const char* filename)
{
	GLuint texture = 0;
	FILE* file;
	if (fopen_s(&file, filename, "rb") != 0) return 0;

	unsigned char header[54];
	fread(header, 1, 54, file);

	int width = *(int*)&header[18];
	int height = *(int*)&header[22];
	int imageSize = *(int*)&header[34];
	if (imageSize == 0) imageSize = width * height * 3;

	unsigned char* data = new unsigned char[imageSize];
	fread(data, 1, imageSize, file);
	fclose(file);

	// Windows BMP stores BGR, OpenGL expects RGB unless formatted
	for (int i = 0; i < imageSize; i += 3)
	{
		unsigned char temp = data[i];
		data[i] = data[i + 2];
		data[i + 2] = temp;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	delete[] data;
	return texture;
}

void getPathPoint(float t, float r, float L, float& px, float& py, float& nx, float& ny, float& traveled) {
	float arcLen = (float)M_PI * r;
	float totalLen = 2.0f * arcLen + 2.0f * L;
	float dist = t * totalLen;

	if (dist <= L) {
		px = -L / 2.0f + dist;
		py = r;
		nx = 0.0f; ny = 1.0f;
		traveled = dist;
	}
	else if (dist <= L + arcLen) {
		float a = (dist - L) / r;
		float angle = (float)M_PI / 2.0f - a;
		px = L / 2.0f + r * cos(angle);
		py = r * sin(angle);
		nx = cos(angle); ny = sin(angle);
		traveled = dist;
	}
	else if (dist <= 2.0f * L + arcLen) {
		float d = dist - (L + arcLen);
		px = L / 2.0f - d;
		py = -r;
		nx = 0.0f; ny = -1.0f;
		traveled = dist;
	}
	else {
		float d = dist - (2.0f * L + arcLen);
		float a = d / r;
		float angle = -(float)M_PI / 2.0f - a;
		px = -L / 2.0f + r * cos(angle);
		py = r * sin(angle);
		nx = cos(angle); ny = sin(angle);
		traveled = dist;
	}
}

void drawChainLink(float innerRadius, float outerRadius, float L, int nsides, int rings) {
	float arcLen = (float)M_PI * outerRadius;
	float totalLen = 2.0f * arcLen + 2.0f * L;

	for (int i = 0; i < rings; i++) {
		float t0 = (float)i / rings;
		float t1 = (float)(i + 1) / rings;

		float px0, py0, nx0, ny0, dist0_path;
		getPathPoint(t0, outerRadius, L, px0, py0, nx0, ny0, dist0_path);

		float px1, py1, nx1, ny1, dist1_path;
		getPathPoint(t1, outerRadius, L, px1, py1, nx1, ny1, dist1_path);

		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= nsides; j++) {
			float phi = (float)j * 2.0f * (float)M_PI / nsides;
			float cosPhi = cos(phi);
			float sinPhi = sin(phi);

			float n0x = nx0 * cosPhi;
			float n0y = ny0 * cosPhi;
			float n0z = sinPhi;

			float v0x = px0 + innerRadius * n0x;
			float v0y = py0 + innerRadius * n0y;
			float v0z = innerRadius * n0z;

			float tx0 = (float)j / nsides;
			float ty0 = dist0_path / totalLen * 4.0f;

			glNormal3f(n0x, n0y, n0z);
			glTexCoord2f(tx0, ty0);
			glVertex3f(v0x, v0y, v0z);

			float n1x = nx1 * cosPhi;
			float n1y = ny1 * cosPhi;
			float n1z = sinPhi;

			float v1x = px1 + innerRadius * n1x;
			float v1y = py1 + innerRadius * n1y;
			float v1z = innerRadius * n1z;
			
			float ty1 = dist1_path / totalLen * 4.0f;

			glNormal3f(n1x, n1y, n1z);
			glTexCoord2f(tx0, ty1);
			glVertex3f(v1x, v1y, v1z);
		}
		glEnd();
	}
}

void drawMeteorHammer() {
	GLUquadricObj* quadric = gluNewQuadric();
	gluQuadricNormals(quadric, GLU_SMOOTH);
	gluQuadricTexture(quadric, GL_TRUE);

	glBindTexture(GL_TEXTURE_2D, textureWoodID);
	
	// 1. Draw Wooden Handle
	glPushMatrix();
	glTranslatef(0.0f, -4.5f, 0.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	gluCylinder(quadric, 0.15f, 0.15f, 1.8f, 50, 50); 
	glPopMatrix();

	// Switch back to metal for chains, sphere, and edges
	glBindTexture(GL_TEXTURE_2D, textureMetalID);

	// Metal pommel outer ring (handle bottom edge)
	glPushMatrix();
	glTranslatef(0.0f, -4.5f, 0.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	drawChainLink(0.05f, 0.16f, 0.0f, 20, 20); // L=0 draws a circular torus
	glPopMatrix();

	// Solid plate covering the entire bottom inside the ring
	glPushMatrix();
	glTranslatef(0.0f, -4.52f, 0.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	gluDisk(quadric, 0.0f, 0.17f, 30, 10);
	glPopMatrix();

	// Metal top rim
	glPushMatrix();
	glTranslatef(0.0f, -2.7f, 0.0f); 
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	drawChainLink(0.05f, 0.16f, 0.0f, 20, 20);
	glPopMatrix();

	// Solid plate covering the entire top inside the rim
	glPushMatrix();
	glTranslatef(0.0f, -2.68f, 0.0f); 
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	gluDisk(quadric, 0.0f, 0.17f, 30, 10);
	glPopMatrix();

	// 2. Draw Chains (Oval links)
	glBindTexture(GL_TEXTURE_2D, textureChainID);

	// Link from handle at y = -2.55, to sphere at y = 1.0
	for (int i = 0; i < 12; i++) {
		glPushMatrix();
		glTranslatef(0.0f, -2.55f + i * 0.32f, 0.0f);
		
		// Alternate ring rotation to interlock perfectly
		if (i % 2 == 0) {
			glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
		}
		
		// Rotate chains to stand up vertically along the Y axis
		glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
		
		drawChainLink(0.045f, 0.10f, 0.12f, 30, 40); // 1200 polygons per ring
		glPopMatrix();
	}

	// Switch back to metal for the actual meteor sphere and cones
	glBindTexture(GL_TEXTURE_2D, textureMetalID);

	// 3. Draw Sphere
	glPushMatrix();
	glTranslatef(0.0f, 1.5f, 0.0f); 
	gluSphere(quadric, 1.2f, 100, 100); 

	// 4. Draw Cones
	int numLat = 7; // Reduced cones
	int numLon = 12; // Reduced cones
	for (int lat = 1; lat < numLat; lat++) { 
		float theta = lat * M_PI / numLat;
		// Skip placing cones near the bottom pole so they don't overlap the chains
		if (lat >= numLat - 1) continue;

		for (int lon = 0; lon < numLon; lon++) {
			float phi = lon * 2 * M_PI / numLon;

			float x = sin(theta) * cos(phi);
			float z = sin(theta) * sin(phi);
			float y = cos(theta);

			glPushMatrix();
			glTranslatef(x * 1.15f, y * 1.15f, z * 1.15f);

			float angle = acos(z) * 180.0f / M_PI;
			float len = sqrt(x*x + y*y);
			if (len > 0.0001f) {
				glRotatef(angle, -y, x, 0.0f);
			} else {
				if (z < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
			}

			// Higher and thicker cones
			gluCylinder(quadric, 0.12f, 0.0f, 0.55f, 20, 10); 
			glPopMatrix();
		}
	}
	
	// Top Pole cone
	glPushMatrix();
	glTranslatef(0.0f, 1.15f, 0.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
	gluCylinder(quadric, 0.12f, 0.0f, 0.55f, 20, 10);
	glPopMatrix();
	
	// Removed the bottom pole cone completely

	glPopMatrix();

	gluDeleteQuadric(quadric);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	updateCamera();

	gluLookAt(
		cameraX, cameraY, cameraZ,
		0, 0, 0,
		0, 1, 0
	);

	drawMeteorHammer();
}
//--------------------------------------------------------------------

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
	textureMetalID = loadBMPTexture("metal.bmp");
	textureWoodID = loadBMPTexture("wood.bmp");
	textureChainID = loadBMPTexture("chain.bmp");

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

		// Update spin logic slightly for animation showcasing high fidelity
		spinAngle += 0.1f;
		if (spinAngle > 360.0f) spinAngle -= 360.0f;

		display();
		SwapBuffers(hdc);
	}

	UnregisterClass(WINDOW_TITLE, wc.hInstance);
	return true;
}