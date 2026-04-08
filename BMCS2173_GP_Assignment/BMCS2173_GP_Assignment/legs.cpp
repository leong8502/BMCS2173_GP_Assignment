
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma warning(disable:4996)
#pragma comment (lib, "OpenGL32.lib")

#define WINDOW_TITLE "Leg Parts"

//--------------------------------
// Camera variables
//--------------------------------

float cameraAngle = 0.0f;
float cameraHeight = 2.0f;
float cameraDistance = 8.0f;

float cameraX, cameraY, cameraZ;

//--------------------------------
// Character joint variables
//--------------------------------

float leftArmAngle = 0.0f;
float rightArmAngle = 0.0f;

float leftLegAngle = 0.0f;
float rightLegAngle = 0.0f;

//--------------------------------
// Animation variables
//--------------------------------

bool attackAnimation = false;
float attackAngle = 0.0f;

//--------------------------------
// Customization
//--------------------------------

int outfitColor = 1;

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

	//--------------------------------
	// Character joint variables
	//--------------------------------

	leftArmAngle = 0.0f;
	rightArmAngle = 0.0f;

	leftLegAngle = 0.0f;
	rightLegAngle = 0.0f;

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
		// Customization for outfit
		//--------------------------------

		case '1':
			outfitColor = 1;
			break;

		case '2':
			outfitColor = 2;
			break;

		case '3':
			outfitColor = 3;
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
			break;

		case VK_ADD: // press + / =
		case VK_OEM_PLUS:
			cameraDistance -= 0.3f;
			break;

		case VK_SUBTRACT: // press - / _
		case VK_OEM_MINUS:
			cameraDistance += 0.3f;
			break;

		//--------------------------------
		// Character arm movement
		//--------------------------------

		case 'Q':
			leftArmAngle += 5;
			break;

		case 'W':
			leftArmAngle -= 5;
			break;

		case 'E':
			rightArmAngle += 5;
			break;

		case 'R':
			rightArmAngle -= 5;
			break;

		case 'Z':
			leftLegAngle += 5;
			break;

		case 'X':
			leftLegAngle -= 5;
			break;

		case 'C':
			rightLegAngle += 5;
			break;

		case 'V':
			rightLegAngle -= 5;
			break;

		//--------------------------------
		// Character attack
		//--------------------------------

		case 'F':
			attackAnimation = true;
			attackAngle = 0;
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

void drawProceduralLegPart(float length, float topRx, float topRy, float bottomRx, float bottomRy, int slices, int stacks, float bulgeFactor, float bulgePos) {
	for (int i = 0; i < stacks; ++i) {
		float t1 = (float)i / stacks;
		float t2 = (float)(i + 1) / stacks;
		float z1 = t1 * length;
		float z2 = t2 * length;
		
		float rx1 = topRx * (1.0f - t1) + bottomRx * t1;
		float ry1 = topRy * (1.0f - t1) + bottomRy * t1;
		float bulge1 = sin(t1 * 3.14159265f) * bulgeFactor * exp(-pow(t1 - bulgePos, 2) * 10.0f);
		rx1 += bulge1; ry1 += bulge1;
		
		float rx2 = topRx * (1.0f - t2) + bottomRx * t2;
		float ry2 = topRy * (1.0f - t2) + bottomRy * t2;
		float bulge2 = sin(t2 * 3.14159265f) * bulgeFactor * exp(-pow(t2 - bulgePos, 2) * 10.0f);
		rx2 += bulge2; ry2 += bulge2;

		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= slices; ++j) {
			float theta = (float)j / slices * 2.0f * 3.14159265f;
			float s = (float)j / slices;
			float cosT = cos(theta);
			float sinT = sin(theta);
			
			float nx1 = ry1 * cosT; float ny1 = rx1 * sinT; float nz1 = (topRx - bottomRx) / length * ((rx1+ry1)*0.5f);
			float len1 = sqrt(nx1 * nx1 + ny1 * ny1 + nz1 * nz1);
			if (len1 > 0) { nx1 /= len1; ny1 /= len1; nz1 /= len1; }
			
			float nx2 = ry2 * cosT; float ny2 = rx2 * sinT; float nz2 = (topRx - bottomRx) / length * ((rx2+ry2)*0.5f);
			float len2 = sqrt(nx2 * nx2 + ny2 * ny2 + nz2 * nz2);
			if (len2 > 0) { nx2 /= len2; ny2 /= len2; nz2 /= len2; }
			
			glNormal3f(nx2, ny2, nz2);
			glTexCoord2f(s * 2.0f, t2 * 2.0f); // Tiling factor of 2
			glVertex3f(rx2 * cosT, ry2 * sinT, z2);
			
			glNormal3f(nx1, ny1, nz1);
			glTexCoord2f(s * 2.0f, t1 * 2.0f);
			glVertex3f(rx1 * cosT, ry1 * sinT, z1);
		}
		glEnd();
	}
}

void drawRoundTrim(float r) {
	// Front
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, r, 0.0f); 
	for(int i = 0; i <= 10; ++i) {
		float angle = (float)i / 10.0f * 3.14159f; 
		glVertex3f(cos(angle) * r * 0.9f, r, sin(angle) * r * 1.4f);
	}
	glEnd();

	// Back
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f, -1.0f, 0.0f);
	glVertex3f(0.0f, -r, 0.0f); 
	for(int i = 0; i <= 10; ++i) {
		float angle = (float)i / 10.0f * 3.14159f;
		glVertex3f(-cos(angle) * r * 0.9f, -r, sin(angle) * r * 1.4f);
	}
	glEnd();

	// Right
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(1.0f, 0.0f, 0.0f);
	glVertex3f(r, 0.0f, 0.0f); 
	for(int i = 0; i <= 10; ++i) {
		float angle = (float)i / 10.0f * 3.14159f;
		glVertex3f(r, -cos(angle) * r * 0.9f, sin(angle) * r * 1.4f);
	}
	glEnd();

	// Left
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glVertex3f(-r, 0.0f, 0.0f); 
	for(int i = 0; i <= 10; ++i) {
		float angle = (float)i / 10.0f * 3.14159f;
		glVertex3f(-r, cos(angle) * r * 0.9f, sin(angle) * r * 1.4f);
	}
	glEnd();
}

void drawLegBase(bool isLeft) {
	GLUquadric* quad = gluNewQuadric();
	gluQuadricNormals(quad, GLU_SMOOTH);
	gluQuadricTexture(quad, GL_TRUE); // Enable texture coordinates for quadrics
	
	glPushMatrix();
	glRotatef(90, 1, 0, 0); 
	
	if (!isLeft) {
		// Right leg: short sock (below knee)
		
		// Thigh (Skin) 0.0 to 0.60
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texSkin);
		drawProceduralLegPart(0.60f, 0.15f, 0.16f, 0.09f, 0.09f, 60, 40, 0.01f, 0.5f);
		glTranslatef(0.0f, 0.0f, 0.60f);
		
		// Knee Joint (Skin)
		gluSphere(quad, 0.075f, 40, 40);
		
		// Upper Calf (Skin) 0.60 to 0.70
		drawProceduralLegPart(0.10f, 0.09f, 0.09f, 0.105f, 0.105f, 60, 20, 0.005f, 0.5f);
		glTranslatef(0.0f, 0.0f, 0.10f);
		
		// Gold Trim (Below knee) 0.70 to 0.75
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texGold);
		drawProceduralLegPart(0.05f, 0.11f, 0.11f, 0.10f, 0.10f, 60, 20, 0.0f, 0.5f);
		drawRoundTrim(0.11f);
		glTranslatef(0.0f, 0.0f, 0.05f);
		
		// Lower Sock (Calf) 0.75 to 1.25
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texFabric);
		drawProceduralLegPart(0.50f, 0.09f, 0.09f, 0.05f, 0.05f, 60, 50, 0.01f, 0.2f);
		glTranslatef(0.0f, 0.0f, 0.50f);	
	} else {
		// Left leg: high sock (mid thigh)
		
		// Thigh (Skin) 0.0 to 0.35
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texSkin);
		drawProceduralLegPart(0.35f, 0.15f, 0.16f, 0.12f, 0.12f, 60, 40, 0.01f, 0.5f);
		glTranslatef(0.0f, 0.0f, 0.35f);
		
		// Gold Trim (Upper thigh) 0.35 to 0.40
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texGold);
		drawProceduralLegPart(0.05f, 0.125f, 0.125f, 0.11f, 0.11f, 60, 20, 0.0f, 0.5f);
		drawRoundTrim(0.125f);
		glTranslatef(0.0f, 0.0f, 0.05f);
		
		// Upper Sock (Above knee) 0.40 to 0.60
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texFabric);
		drawProceduralLegPart(0.20f, 0.11f, 0.11f, 0.09f, 0.09f, 60, 30, 0.01f, 0.5f);
		glTranslatef(0.0f, 0.0f, 0.20f);
		
		// Knee Joint (Covered by sock)
		gluSphere(quad, 0.075f, 40, 40);
		
		// Lower Sock (Calf) Split into matched segments for identical proportions
		// 0.60 to 0.70
		drawProceduralLegPart(0.10f, 0.09f, 0.09f, 0.105f, 0.105f, 60, 20, 0.005f, 0.5f);
		glTranslatef(0.0f, 0.0f, 0.10f);
		
		// 0.70 to 0.75
		drawProceduralLegPart(0.05f, 0.11f, 0.11f, 0.10f, 0.10f, 60, 20, 0.0f, 0.5f);
		glTranslatef(0.0f, 0.0f, 0.05f);
		
		// 0.75 to 1.25
		drawProceduralLegPart(0.50f, 0.09f, 0.09f, 0.05f, 0.05f, 60, 50, 0.01f, 0.2f);
		glTranslatef(0.0f, 0.0f, 0.50f);
	}
	
	// Boot Inner Ankle (White)
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texWhiteLeather);
	drawProceduralLegPart(0.10f, 0.055f, 0.055f, 0.058f, 0.058f, 60, 20, 0.0f, 0.0f);
	
	// Boot Flap Top (White)
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -0.06f);
	drawProceduralLegPart(0.08f, 0.09f, 0.09f, 0.065f, 0.065f, 60, 20, 0.0f, 0.5f);
	
	// Gold Rim on Boot
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texGold);
	gluCylinder(quad, 0.095f, 0.095f, 0.015f, 60, 20);
	glTranslatef(0.0f, 0.0f, 0.015f);
	gluCylinder(quad, 0.095f, 0.07f, 0.04f, 60, 20);
	glPopMatrix();
	
	// Gold Balls (Both sides)
	glPushMatrix();
	glTranslatef(0.075f, 0.0f, 0.03f);
	gluSphere(quad, 0.03f, 40, 40);
	glPopMatrix();
	
	glPushMatrix();
	glTranslatef(-0.075f, 0.0f, 0.03f);
	gluSphere(quad, 0.03f, 40, 40);
	glPopMatrix();
	
	glTranslatef(0.0f, 0.0f, 0.10f); // moves to ankle joint
	
	// Ankle Joint
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texWhiteLeather);
	gluSphere(quad, 0.065f, 40, 40); 
	
	// Boot Foot Group
	glPushMatrix();
	glTranslatef(0.0f, 0.06f, 0.04f); 
	glRotatef(20.0f, 1, 0, 0); 
	
	// Upper foot shape (Perfectly rounded stretched sphere)
	glPushMatrix();
	glScalef(0.075f, 0.14f, 0.075f);
	gluSphere(quad, 1.0f, 60, 60);
	glPopMatrix();

	// Front Sole
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texDarkLeather);
	glPushMatrix();
	glTranslatef(0.0f, 0.03f, 0.055f); 
	glScalef(0.072f, 0.11f, 0.025f); 
	gluSphere(quad, 1.0f, 30, 30);
	glPopMatrix();
	
	glPopMatrix(); // End Boot Foot Group
	
	// Heel block (Gold instead of dark leather)
	glColor3f(1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, texGold);
	glPushMatrix();
	glTranslatef(0.0f, -0.02f, 0.0f); 
	gluCylinder(quad, 0.035, 0.025, 0.13f, 40, 40); 
	glTranslatef(0.0f, 0.0f, 0.13f);
	gluDisk(quad, 0.0, 0.025, 40, 1);
	glPopMatrix();
	
	gluDeleteQuadric(quad);
	glPopMatrix();
}

void drawLeftLeg()
{
	glPushMatrix();
	// Adjusted initial translation to float on ground based on new length
	glTranslatef(-0.16f, 1.53f, 0.0f);
	glRotatef(leftLegAngle, 1, 0, 0);
	drawLegBase(true);
	glPopMatrix();
}

void drawRightLeg()
{
	glPushMatrix();
	// Adjusted initial translation to float on ground based on new length
	glTranslatef(0.16f, 1.53f, 0.0f);
	glRotatef(rightLegAngle, 1, 0, 0);
	drawLegBase(false);
	glPopMatrix();
}

void drawGround()
{
	glDisable(GL_LIGHTING);

	glColor3f(0.6f, 0.6f, 0.6f);

	glBegin(GL_QUADS);

	glVertex3f(-10, 0, -10);
	glVertex3f(-10, 0, 10);
	glVertex3f(10, 0, 10);
	glVertex3f(10, 0, -10);

	glEnd();

	glEnable(GL_LIGHTING);
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

	GLfloat spotDirection[] = { 0.0f, -1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDirection);

	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f);
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

	GLfloat lightPosition[] = { 3.0f, 5.0f, 3.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	//drawGround();
	
	drawLeftLeg();
	drawRightLeg();

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
	
	texSkin = loadBMP("skin.bmp");
	texFabric = loadBMP("fabric.bmp");
	texGold = loadBMP("gold.bmp");
	texWhiteLeather = loadBMP("white_leather.bmp");
	texDarkLeather = loadBMP("dark_leather.bmp");

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