
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma comment (lib, "OpenGL32.lib")
#pragma warning(disable:4996)

#define WINDOW_TITLE "Background"

//--------------------------------
// Camera variables
//--------------------------------

float cameraAngle = 0.0f;
float cameraHeight = 2.0f;
float cameraDistance = 8.0f;

float cameraX, cameraY, cameraZ;

//--------------------------------
// Texture
//--------------------------------

GLuint textureID;
GLuint texGrass;


void updateCamera()
{
	cameraX = sin(cameraAngle) * cameraDistance;
	cameraZ = cos(cameraAngle) * cameraDistance;
	cameraY = cameraHeight;
}

void resetAll() {

	//--------------------------------
	// Camera variables
	//--------------------------------

	cameraAngle = 0.0f;
	cameraHeight = 2.0f;
	cameraDistance = 8.0f;

	cameraX = 0.0f;
	cameraY = 2.0f;
	cameraZ = 8.0f;

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

			//--------------------------------
			// Reset Function
			//--------------------------------

		case VK_SPACE:
			resetAll();
			break;

			//--------------------------------
			// Camera Viewport
			//--------------------------------

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
			if (cameraHeight < 0.5f) cameraHeight = 0.5f; // Clamp: no going underground
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

	// choose pixel format returns the number most similar pixel format available
	int n = ChoosePixelFormat(hdc, &pfd);

	// set pixel format returns whether it sucessfully set the pixel format
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
	glClearColor(0.45f, 0.72f, 0.95f, 1.0f); // Sky blue background

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

// -----------------------------------------------
// Draw the flat green grassland with grass texture
// -----------------------------------------------
void drawGround()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texGrass);
	glColor3f(1.0f, 1.0f, 1.0f); // Let texture show at full color
	glNormal3f(0.0f, 1.0f, 0.0f);

	// Tile the texture across the large quad (repeat 20x)
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); glVertex3f(-60, 0, -60);
	glTexCoord2f( 0.0f, 20.0f); glVertex3f(-60, 0,  60);
	glTexCoord2f(20.0f, 20.0f); glVertex3f( 60, 0,  60);
	glTexCoord2f(20.0f,  0.0f); glVertex3f( 60, 0, -60);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

// -----------------------------------------------
// Draw the grass field with scattered tufts
// -----------------------------------------------
void drawGrass()
{
	// Deterministic pseudo-random scatter without stdlib
	// Using a fixed pattern with varying positions
	float posX[] = { -4.5f, -3.1f, -5.8f, -1.2f, -6.3f, -2.7f, -0.4f, -5.0f, -3.9f, -1.8f,
	                  0.6f,  2.3f,  4.7f,  1.1f,  3.5f,  5.2f,  0.9f,  2.8f,  4.0f,  5.9f,
	                 -4.2f, -2.0f, -0.8f,  1.6f,  3.2f,  5.5f, -5.5f, -3.6f,  0.3f,  4.4f,
	                 -6.0f,  6.1f, -1.5f,  2.6f, -4.8f,  3.9f,  1.3f, -3.3f,  5.7f, -0.2f };
	float posZ[] = { -4.0f, -5.3f, -2.1f, -3.8f, -0.9f, -6.1f, -5.0f, -1.5f, -3.2f, -4.7f,
	                 -5.5f, -4.2f, -3.0f, -6.0f, -1.8f, -2.6f, -0.5f, -5.8f, -4.5f, -1.1f,
	                  1.2f,  2.5f,  4.0f,  1.7f,  3.4f,  0.8f,  5.1f,  2.9f,  4.6f,  1.5f,
	                  3.7f,  0.4f,  5.5f,  2.1f,  4.9f,  1.0f,  3.3f,  5.8f,  0.7f,  4.2f };
	float heights[] = { 0.30f,0.25f,0.35f,0.28f,0.22f,0.32f,0.26f,0.33f,0.20f,0.29f,
	                    0.31f,0.24f,0.27f,0.36f,0.21f,0.34f,0.23f,0.30f,0.25f,0.28f,
	                    0.32f,0.26f,0.35f,0.22f,0.29f,0.33f,0.24f,0.27f,0.31f,0.20f,
	                    0.28f,0.34f,0.25f,0.30f,0.23f,0.36f,0.27f,0.21f,0.33f,0.26f };
	// Alternate between bright and dark green shades
	float gr[] = { 0.25f,0.32f,0.20f,0.35f,0.28f,0.18f,0.30f,0.22f,0.33f,0.26f,
	               0.21f,0.29f,0.36f,0.24f,0.31f,0.19f,0.27f,0.34f,0.23f,0.28f,
	               0.30f,0.25f,0.32f,0.20f,0.35f,0.28f,0.18f,0.30f,0.22f,0.33f,
	               0.26f,0.21f,0.29f,0.36f,0.24f,0.31f,0.19f,0.27f,0.34f,0.23f };
}

// -----------------------------------------------
// Draw a 3D cloud at (cx, cy, cz) with given scale
// -----------------------------------------------
void drawCloud(float cx, float cy, float cz, float scale)
{
	GLUquadric* q = gluNewQuadric();
	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f); // White cloud

	// Cluster of spheres to form the puff shape
	float offsets[][4] = {
		{ 0.0f, 0.0f,  0.0f, 1.0f },
		{ 1.2f, 0.3f,  0.0f, 0.8f },
		{-1.2f, 0.3f,  0.0f, 0.8f },
		{ 0.6f, 0.7f,  0.0f, 0.7f },
		{-0.6f, 0.7f,  0.0f, 0.7f },
		{ 0.0f, 0.9f,  0.0f, 0.6f },
		{ 1.8f, 0.0f,  0.0f, 0.5f },
		{-1.8f, 0.0f,  0.0f, 0.5f },
	};

	for (int i = 0; i < 8; i++) {
		glPushMatrix();
		glTranslatef(cx + offsets[i][0] * scale,
		             cy + offsets[i][1] * scale,
		             cz);
		gluSphere(q, offsets[i][3] * scale, 16, 16);
		glPopMatrix();
	}
	gluDeleteQuadric(q);
}

// -----------------------------------------------
// Draw the golden Sun sphere and set it as light
// -----------------------------------------------
void drawSun(float sx, float sy, float sz)
{
	GLUquadric* q = gluNewQuadric();

	// The sun itself: emissive bright yellow sphere, no lighting applied
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glColor3f(1.0f, 0.95f, 0.3f); // Bright golden yellow
	glPushMatrix();
	glTranslatef(sx, sy, sz);
	gluSphere(q, 1.5f, 32, 32);
	glPopMatrix();

	// Sun glow halo (slightly larger, semi-transparent)
	glColor3f(1.0f, 0.85f, 0.2f);
	glPushMatrix();
	glTranslatef(sx, sy, sz);
	gluSphere(q, 1.9f, 32, 32);
	glPopMatrix();

	gluDeleteQuadric(q);
	glEnable(GL_LIGHTING);
}

void setupLighting()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glShadeModel(GL_SMOOTH);

	// Sun position - higher in the sky
	float sunX = 12.0f, sunY = 18.0f, sunZ = -30.0f;
	GLfloat lightPosition[] = { sunX, sunY, sunZ, 1.0f }; // point light
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	// Strong warm sunlight - high diffuse and specular
	GLfloat ambientLight[]  = { 0.35f, 0.35f, 0.30f, 1.0f };
	GLfloat diffuseLight[]  = { 1.0f,  0.95f, 0.80f, 1.0f };
	GLfloat specularLight[] = { 1.0f,  1.0f,  0.9f,  1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT,  ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	// No distance attenuation so the whole ground is well lit
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  1.0f);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.0f);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);

	// Use the full spotlight cone (no cutoff)
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
}

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

	// BMP stores BGR, convert to RGB
	for (int i = 0; i < imageSize; i += 3) {
		unsigned char tmp = data[i];
		data[i] = data[i + 2];
		data[i + 2] = tmp;
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

GLuint loadTexture()
{
	GLuint texture;

	unsigned char textureData[] =
	{
		255,0,0, 0,255,0,
		0,0,255, 255,255,0
	};

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		2,
		2,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		textureData
	);

	return texture;
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

	// Update sun (light) position every frame - higher in the sky
	float sunX = 12.0f, sunY = 18.0f, sunZ = -30.0f;
	GLfloat lightPosition[] = { sunX, sunY, sunZ, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	// Draw the sun sphere (emissive, no lighting)
	drawSun(sunX, sunY, sunZ);

	// Clouds spread across the sky in all four quadrants, higher up
	// Far-left quadrant
	drawCloud(-18.0f, 14.0f, -40.0f, 1.5f);
	drawCloud(-25.0f, 16.0f, -35.0f, 1.2f);
	// Far-right quadrant
	drawCloud( 20.0f, 15.0f, -38.0f, 1.8f);
	drawCloud( 28.0f, 13.0f, -32.0f, 1.1f);
	// Centre-left, mid distance
	drawCloud(-10.0f, 12.0f, -28.0f, 1.4f);
	// Centre-right, mid distance
	drawCloud( 12.0f, 11.0f, -25.0f, 1.6f);
	// Left side, near
	drawCloud(-20.0f, 10.0f, -18.0f, 1.0f);
	// Right side, near
	drawCloud( 22.0f, 12.0f, -20.0f, 1.3f);
	// Far background centre
	drawCloud(  0.0f, 17.0f, -50.0f, 2.0f);
	// Extra scatter
	drawCloud(-32.0f, 14.0f, -45.0f, 1.7f);

	// Draw the green grassland
	drawGround();

	// Draw individual grass tufts
	drawGrass();
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

	//--------------------------------
	//	Initialize window for OpenGL
	//--------------------------------

	HDC hdc = GetDC(hWnd);

	//	initialize pixel format for the window
	initPixelFormat(hdc);

	//	get an openGL context
	HGLRC hglrc = wglCreateContext(hdc);

	//	make context current
	if (!wglMakeCurrent(hdc, hglrc)) return false;

	//--------------------------------
	//	setup for Initial, Lighting and Texture
	//--------------------------------
	initOpenGL();
	setupLighting();
	textureID = loadTexture();
	texGrass = loadBMP("grass.bmp");


	//--------------------------------
	//	End initialization
	//--------------------------------

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
//--------------------------------------------------------------------