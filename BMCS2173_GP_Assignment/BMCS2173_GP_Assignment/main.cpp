
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>

#pragma comment (lib, "OpenGL32.lib")

#define WINDOW_TITLE "Main Character"

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
// Texture
//--------------------------------

GLuint textureID;

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

void drawHair()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glColor3f(0.1f, 0.2f, 0.8f); // blue hair

	glTranslatef(0.0f, 1.7f, -0.15f);

	glRotatef(-90, 1, 0, 0);

	gluCylinder(quad, 0.42, 0.42, 0.7, 32, 32);

	glPopMatrix();
}

void drawHead()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glColor3f(1.0f, 0.8f, 0.6f);
	glTranslatef(0.0f, 1.6f, 0.0f);

	gluSphere(quad, 0.5, 32, 32);

	glPopMatrix();
}

void drawBody()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	if (outfitColor == 1)
		glColor3f(0.3f, 0.3f, 1.0f); // blue

	if (outfitColor == 2)
		glColor3f(1.0f, 0.3f, 0.3f); // red

	if (outfitColor == 3)
		glColor3f(0.3f, 1.0f, 0.3f); // green

	glTranslatef(0.0f, 0.8f, 0.0f);

	glRotatef(-90, 1, 0, 0);

	glBindTexture(GL_TEXTURE_2D, textureID); //must before cylinder

	gluCylinder(quad, 0.5, 0.5, 1.2, 32, 32);

	glPopMatrix();
}

void drawLeftArm()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glTranslatef(-0.7f, 1.3f, 0.0f);

	glRotatef(leftArmAngle, 1, 0, 0);

	glRotatef(90, 1, 0, 0);

	glColor3f(0.9f, 0.8f, 0.7f);

	gluCylinder(quad, 0.15, 0.15, 0.8, 32, 32);

	glTranslatef(0.0f, 0.0f, 0.8f);
	glColor3f(1.0f, 0.8f, 0.6f);
	gluSphere(quad, 0.15, 16, 16);

	glPopMatrix();
}

void drawRightArm()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glTranslatef(0.7f, 1.3f, 0.0f);

	glRotatef(rightArmAngle, 1, 0, 0);

	glRotatef(90, 1, 0, 0);

	glColor3f(0.9f, 0.8f, 0.7f);

	gluCylinder(quad, 0.15, 0.15, 0.8, 32, 32);

	glTranslatef(0.0f, 0.0f, 0.8f);
	glColor3f(1.0f, 0.8f, 0.6f);
	gluSphere(quad, 0.15, 16, 16);

	glPopMatrix();
}

void drawLeftLeg()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glTranslatef(-0.25f, 0.8f, 0.0f);

	glRotatef(leftLegAngle, 1, 0, 0);

	glRotatef(90, 1, 0, 0);

	if (outfitColor == 1)
		glColor3f(0.3f, 0.3f, 1.0f); // blue

	if (outfitColor == 2)
		glColor3f(1.0f, 0.3f, 0.3f); // red

	if (outfitColor == 3)
		glColor3f(0.3f, 1.0f, 0.3f); // green

	gluCylinder(quad, 0.18, 0.18, 0.8, 32, 32);

	glTranslatef(0.0f, 0.0f, 0.8f);
	glColor3f(0.1f, 0.1f, 0.4f);
	gluSphere(quad, 0.2, 16, 16);

	glPopMatrix();
}

void drawRightLeg()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glTranslatef(0.25f, 0.8f, 0.0f);

	glRotatef(rightLegAngle, 1, 0, 0);

	glRotatef(90, 1, 0, 0);

	if (outfitColor == 1)
		glColor3f(0.3f, 0.3f, 1.0f); // blue

	if (outfitColor == 2)
		glColor3f(1.0f, 0.3f, 0.3f); // red

	if (outfitColor == 3)
		glColor3f(0.3f, 1.0f, 0.3f); // green

	gluCylinder(quad, 0.18, 0.18, 0.8, 32, 32);

	glTranslatef(0.0f, 0.0f, 0.8f);
	glColor3f(0.1f, 0.1f, 0.4f);
	gluSphere(quad, 0.2, 16, 16);

	glPopMatrix();
}

void drawDress()
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();

	glTranslatef(0.0f, 0.7f, 0.0f);

	glRotatef(-90, 1, 0, 0);

	glColor3f(0.3f, 0.3f, 1.0f);

	gluCylinder(quad, 0.7, 0.4, 0.7, 32, 32);

	glPopMatrix();
}

void drawCharacter()
{
	glPushMatrix();

	drawHair();
	drawHead();
	drawBody();
	drawDress();
	drawLeftArm();
	drawRightArm();
	drawLeftLeg();
	drawRightLeg();

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

void updateAnimation()
{

	if (attackAnimation)
	{
		attackAngle += 2.0f;

		rightArmAngle = +attackAngle;

		if (attackAngle > 90)
		{
			attackAnimation = false;
			rightArmAngle = 0;
		}
	}
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

	updateAnimation();

	drawGround();
	drawCharacter();

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