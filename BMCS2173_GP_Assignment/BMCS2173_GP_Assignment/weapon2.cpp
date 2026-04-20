#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma comment (lib, "OpenGL32.lib")

#define WINDOW_TITLE "Fan"

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
bool isWireframe = false;

//--------------------------------
// Folding Animation
//--------------------------------

bool isFanFolded = false;
float currentSpreadAngle = 140.0f;
float targetSpreadAngle = 140.0f;

//--------------------------------
// Texture
//--------------------------------

GLuint textureWoodID;
GLuint textureFanID;

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

	isFanFolded = false;
	currentSpreadAngle = 140.0f;
	targetSpreadAngle = 140.0f;

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

		case '5': // Key 5
			isFanFolded = !isFanFolded;
			targetSpreadAngle = isFanFolded ? 0.0f : 140.0f;
			break;

		case 'I':
		case 'i':
			isWireframe = !isWireframe;
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

GLuint loadBMP(const char* filename)
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
	// We must respect 4-byte row padding so we don't accidentally swap padding with pixel colors
	int rowSize = ((width * 3) + 3) & ~3;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int i = y * rowSize + x * 3;
			unsigned char temp = data[i];
			data[i] = data[i + 2];
			data[i + 2] = temp;
		}
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	delete[] data;
	return texture;
}

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void drawCylinder(float baseRadius, float topRadius, float height, int slices, int stacks) {
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluQuadricTexture(quadric, GL_TRUE);

    gluCylinder(quadric, baseRadius, topRadius, height, slices, stacks);

    glPushMatrix();
    glRotatef(180, 1.0f, 0.0f, 0.0f);
    gluDisk(quadric, 0.0f, baseRadius, slices, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, height);
    gluDisk(quadric, 0.0f, topRadius, slices, 1);
    glPopMatrix();

    gluDeleteQuadric(quadric);
}

void drawRib(float length, float width, float thickness) {
    glPushMatrix();
    glScalef(width, length, thickness);
    glTranslatef(0.0f, 0.5f, 0.0f);
    
    glBegin(GL_QUADS);
    glNormal3f( 0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f,  0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
    
    glNormal3f( 0.0f, 0.0f,-1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f, -0.5f);
    
    glNormal3f( 0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f,  0.5f,  0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);
    
    glNormal3f( 0.0f,-1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f, -0.5f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
    
    glNormal3f( 1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f,  0.5f,  0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);
    
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
    glEnd();

    glPopMatrix();
}

void drawGuard(float length, float thickness, float bL, float bR, float tL, float tR) {
    glPushMatrix();
    
    glBegin(GL_QUADS);
    glNormal3f( 0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(bL, 0.0f,  thickness/2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bR, 0.0f,  thickness/2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(tR,    length, thickness/2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(tL,    length, thickness/2);
    
    glNormal3f( 0.0f, 0.0f,-1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bL, 0.0f, -thickness/2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(tL,    length, -thickness/2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(tR,    length, -thickness/2);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(bR, 0.0f, -thickness/2);
    
    glNormal3f( 0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(tL,    length, -thickness/2);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(tL,    length,  thickness/2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(tR,    length,  thickness/2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(tR,    length, -thickness/2);
    
    glNormal3f( 0.0f,-1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(bL, 0.0f, -thickness/2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(bR, 0.0f, -thickness/2);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(bR, 0.0f,  thickness/2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bL, 0.0f,  thickness/2);
    
    // Right face approx normal
    glNormal3f( 1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bR, 0.0f, -thickness/2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(tR,    length, -thickness/2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(tR,    length,  thickness/2);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(bR, 0.0f,  thickness/2);
    
    // Left Face approx normal
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(bL, 0.0f, -thickness/2);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(bL, 0.0f,  thickness/2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(tL,    length,  thickness/2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(tL,    length, -thickness/2);
    glEnd();

    glPopMatrix();
}

void drawFanLeaf(int numFolds, float innerRadius, float outerRadius, float startAngle, float endAngle, float stackDepth) {
    glBegin(GL_TRIANGLE_STRIP);

    int segmentsPerGap = 10;
    int totalSegments = numFolds * segmentsPerGap;
    float angleStep = (endAngle - startAngle) / totalSegments;

    float spreadAngle = endAngle - startAngle;
    float closedFactor = 1.0f - (spreadAngle / 140.0f); // 0 when open, 1 when fully closed

    for (int i = 0; i <= totalSegments; i++) {
        float angle = startAngle + i * angleStep;
        float rad = angle * PI / 180.0f;

        float t = (float)i / totalSegments; 
        float baseZ = (stackDepth / 2.0f) - (stackDepth * t);

        int gapIndex = i / segmentsPerGap;
        if (gapIndex >= numFolds) gapIndex = numFolds - 1;

        float localPos = (float)(i % segmentsPerGap) / segmentsPerGap;
        if (i == totalSegments) {
            gapIndex = numFolds - 1;
            localPos = 1.0f;
        }

        float sign = (gapIndex % 2 == 0) ? -1.0f : 1.0f;
        // Triangle wave: 0 at ribs (localPos 0 or 1), 1 at midpoint
        float triangleWave = 1.0f - fabs(localPos * 2.0f - 1.0f); 

        // Max stick-out distance perpendicular to the folding direction
        float rawFoldDist = 0.14f * triangleWave * closedFactor; 
        
        // Tangent vector for paper jutting sideway
        float tx = -sin(rad);
        float ty = cos(rad);

        float xInner = innerRadius * cos(rad) + tx * rawFoldDist * sign * (innerRadius / outerRadius);
        float yInner = innerRadius * sin(rad) + ty * rawFoldDist * sign * (innerRadius / outerRadius);

        float xOuter = outerRadius * cos(rad) + tx * rawFoldDist * sign;
        float yOuter = outerRadius * sin(rad) + ty * rawFoldDist * sign;

        // When open, the paper has naturally slight z accordion effect
        float openZFold = 0.2f * triangleWave * sign * (1.0f - closedFactor);
        float zOffset = baseZ + openZFold;
        float zInnerOffset = baseZ + openZFold * (innerRadius / outerRadius);

        float px = tx;
        float py = ty;
        float pz = sign * 0.5f; 
        float len = sqrt(px*px + py*py + pz*pz);

        glNormal3f(px/len, py/len, pz/len);

        float pleatShade = 0.8f + (triangleWave * sign * 0.15f);
        glColor3f(pleatShade, pleatShade, pleatShade);

        // Blend between Cartesian (open) and Polar (closed) mapping
        // so that the folded fan preserves texture details!
        float uInnerCartesian = (innerRadius * cos(rad) / outerRadius) * 0.5f + 0.5f;
        float vInnerCartesian = (innerRadius * sin(rad) / outerRadius);
        float uOuterCartesian = (outerRadius * cos(rad) / outerRadius) * 0.5f + 0.5f;
        float vOuterCartesian = (outerRadius * sin(rad) / outerRadius);

        float uPolar = t; // 0.0 to 1.0 sweeping across
        float vInnerPolar = innerRadius / outerRadius;
        float vOuterPolar = 1.0f;

        float uInnerCoord = uInnerCartesian * (1.0f - closedFactor) + uPolar * closedFactor;
        float vInnerCoord = vInnerCartesian * (1.0f - closedFactor) + vInnerPolar * closedFactor;

        float uOuterCoord = uOuterCartesian * (1.0f - closedFactor) + uPolar * closedFactor;
        float vOuterCoord = vOuterCartesian * (1.0f - closedFactor) + vOuterPolar * closedFactor;

        glTexCoord2f(uInnerCoord, vInnerCoord);
        glVertex3f(xInner, yInner, zInnerOffset);

        glTexCoord2f(uOuterCoord, vOuterCoord);
        glVertex3f(xOuter, yOuter, zOffset);
    }

    glEnd();
}

void drawFan() {
    float ribCount = 21;
    float spreadAngle = currentSpreadAngle;
    float minAngle = 90.0f - (spreadAngle / 2.0f); 
    float maxAngle = 90.0f + (spreadAngle / 2.0f);
    
    // Setup Colors
    GLfloat colorWood[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Base white so BMP colors are not darkened
    GLfloat colorPink[] = { 0.9f, 0.6f, 0.7f, 1.0f };  // Pink

    glPushMatrix();

    // Bind texture for wood components
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureWoodID);

    float stackDepth = 0.35f;

    // 1. Draw Pivot
    glPushMatrix();
    glColor3fv(colorWood);
    glTranslatef(0.0f, 0.0f, -(stackDepth / 2.0f + 0.04f));
    drawCylinder(0.12f, 0.12f, stackDepth + 0.08f, 30, 5);
    glPopMatrix();

    // 2. Draw Inner Ribs
    float ribLength = 2.05f; // Stops precisely at the inner radius of the fan leaf
    float ribWidth = 0.08f;
    float ribThickness = 0.015f;
    
    float angleStep = spreadAngle / (ribCount - 1);
    
    for (int i = 1; i < ribCount - 1; i++) {
        float angle = minAngle + i * angleStep;
        
        glPushMatrix();
        // Subtract 90 degrees to align the +Y facing ribs to the +X standard math circle
        glRotatef(angle - 90.0f, 0.0f, 0.0f, 1.0f); 
        glColor3fv(colorWood);
        
        // Z stacking from front (right guard) to back (left guard)
        float zOffset = (stackDepth / 2.0f) - (stackDepth * i / (ribCount - 1));
        glTranslatef(0.0f, 0.0f, zOffset);
        
        drawRib(ribLength, ribWidth, ribThickness);
        glPopMatrix();
    }
    
    // 3. Draw Outer Guards (Left and Right edges)
    float guardLength = 5.05f; // Slightly longer than 5.0 to cap the leaf tip perfectly
    float guardBottomWidth = 0.4f;
    float guardTopWidth = 0.35f;
    float guardThickness = 0.04f;
    
    // Right Guard (Min Angle, Right Edge) -> Front side
    glPushMatrix();
    glRotatef(minAngle - 90.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(0.0f, 0.0f, stackDepth / 2.0f + guardThickness / 2.0f); 
    glColor3fv(colorWood);
    drawGuard(guardLength, guardThickness, -guardBottomWidth/2, guardBottomWidth/2, -guardTopWidth/2, guardTopWidth/2);
    glPopMatrix();
    
    // Left Guard (Max Angle, Left Edge) -> Back side
    glPushMatrix();
    glRotatef(maxAngle - 90.0f, 0.0f, 0.0f, 1.0f);
    glTranslatef(0.0f, 0.0f, -(stackDepth / 2.0f + guardThickness / 2.0f)); 
    glColor3fv(colorWood);
    drawGuard(guardLength, guardThickness, -guardBottomWidth/2, guardBottomWidth/2, -guardTopWidth/2, guardTopWidth/2);
    glPopMatrix();
    
    // 4. Draw Fan Leaf (Textured Material)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureFanID);
    
    glPushMatrix();
    glColor3fv(colorWood); // colorWood is now [1.0, 1.0, 1.0] (White), ensuring no darkening of the BMP
    
    float innerRadius = 2.0f;
    float outerRadius = 5.0f;
    
    drawFanLeaf(ribCount - 1, innerRadius, outerRadius, minAngle, maxAngle, stackDepth);
    
    glPopMatrix();
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();

	if (isWireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	updateCamera();

	gluLookAt(
		cameraX, cameraY, cameraZ,
		0, 0, 0,
		0, 1, 0
	);

	GLfloat lightPosition[] = { 3.0f, 5.0f, 5.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glPushMatrix();
    glTranslatef(0.0f, -2.0f, 0.0f); // Center fan in view
    drawFan();
    glPopMatrix();

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
	//setupLighting();
	textureWoodID = loadBMP("Textures/fan_wood.bmp");
	textureFanID = loadBMP("Textures/fan.bmp");

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

		// Animation frame update (slowed down for visibility, approx 0.5f per frame)
		if (currentSpreadAngle < targetSpreadAngle) {
			currentSpreadAngle += 0.4f;
			if (currentSpreadAngle > targetSpreadAngle) currentSpreadAngle = targetSpreadAngle;
		} else if (currentSpreadAngle > targetSpreadAngle) {
			currentSpreadAngle -= 0.4f;
			if (currentSpreadAngle < targetSpreadAngle) currentSpreadAngle = targetSpreadAngle;
		}

		display();

		SwapBuffers(hdc);
	}

	UnregisterClass(WINDOW_TITLE, wc.hInstance);

	return true;
}
//--------------------------------------------------------------------