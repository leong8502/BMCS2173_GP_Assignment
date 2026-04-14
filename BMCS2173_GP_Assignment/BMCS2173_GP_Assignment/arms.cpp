
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma warning(disable:4996)
#pragma comment (lib, "OpenGL32.lib")

#define WINDOW_TITLE "Arms Part"

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

bool leftFistActive = false;
bool rightFistActive = false;
float leftFistProgress = 0.0f;
float rightFistProgress = 0.0f;
DWORD lastTime = 0;

void updateAnimation() {
	DWORD currentTime = GetTickCount();
	if (lastTime == 0) lastTime = currentTime;
	float dt = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;

	float speed = 0.5f; // Takes 2 seconds to complete
	
	if (rightFistActive) { rightFistProgress += speed * dt; }
	else { rightFistProgress -= speed * dt; }
	if (rightFistProgress > 1.0f) rightFistProgress = 1.0f;
	if (rightFistProgress < 0.0f) rightFistProgress = 0.0f;

	if (leftFistActive) { leftFistProgress += speed * dt; }
	else { leftFistProgress -= speed * dt; }
	if (leftFistProgress > 1.0f) leftFistProgress = 1.0f;
	if (leftFistProgress < 0.0f) leftFistProgress = 0.0f;
}

//--------------------------------
// Texture
//--------------------------------

GLuint texSkin;
GLuint texFabric;
GLuint texGold;
GLuint texWhiteSleeve;

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

		case 'O':
			rightArmAngle += 5;
			break;

		case 'P':
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

		case '1':
			rightFistActive = !rightFistActive;
			break;
			
		case '2':
			leftFistActive = !leftFistActive;
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

void drawProceduralArmPart(float length, float topRx, float topRy, float bottomRx, float bottomRy, int slices, int stacks, float bulgeFactor, float bulgePos, float sweepStart = 0.0f, float sweepEnd = 2.0f * 3.14159265f, float pleatDepth = 0.0f, int pleatCount = 0) {
	for (int i = 0; i < stacks; ++i) {
		float t1 = (float)i / stacks;
		float t2 = (float)(i + 1) / stacks;

		float z1 = t1 * length;
		float z2 = t2 * length;

		// smooth muscle bulge
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
			float t_sweep = (float)j / slices;
			float theta = sweepStart * (1.0f - t_sweep) + sweepEnd * t_sweep;
			float cosT = cos(theta);
			float sinT = sin(theta);

			float u = (float)j / slices;
			float v1 = t1;
			float v2 = t2;

			float pMod = (pleatCount > 0) ? cos(theta * pleatCount) : 0.0f;
			float r_offset1 = pMod * pleatDepth * t1;
			float r_offset2 = pMod * pleatDepth * t2;

			float rx1p = rx1 + r_offset1;
			float ry1p = ry1 + r_offset1;
			float rx2p = rx2 + r_offset2;
			float ry2p = ry2 + r_offset2;

			float shade = 1.0f;
			if (pleatCount > 0) {
				// Shading also scales with t to avoid harsh edges at top
				float pShade = 0.75f + 0.25f * (pMod * 0.5f + 0.5f);
				shade = 1.0f * (1.0f - t1) + pShade * t1; 
			}

			// Elliptical normal calculation (using base radii for smooth light)
			float avgR1 = (rx1 + ry1) * 0.5f;
			float nx1 = ry1 * cosT; float ny1 = rx1 * sinT; float nz1 = (topRx - bottomRx) / length * avgR1;
			float len1 = sqrt(nx1 * nx1 + ny1 * ny1 + nz1 * nz1);
			if (len1 > 0) { nx1 /= len1; ny1 /= len1; nz1 /= len1; }

			float avgR2 = (rx2 + ry2) * 0.5f;
			float nx2 = ry2 * cosT; float ny2 = rx2 * sinT; float nz2 = (topRx - bottomRx) / length * avgR2;
			float len2 = sqrt(nx2 * nx2 + ny2 * ny2 + nz2 * nz2);
			if (len2 > 0) { nx2 /= len2; ny2 /= len2; nz2 /= len2; }

			glColor3f(shade, shade, shade);
			glNormal3f(nx2, ny2, nz2);
			glTexCoord2f(u, v2);
			glVertex3f(rx2p * cosT, ry2p * sinT, z2);

			glNormal3f(nx1, ny1, nz1);
			glTexCoord2f(u, v1);
			glVertex3f(rx1p * cosT, ry1p * sinT, z1);
		}
		glEnd();
		glColor3f(1.0f, 1.0f, 1.0f); // Reset color for other parts
	}
}

void drawProceduralArmBase(bool isLeft, float fistProgress) {
    // Note: glRotatef sets the starting angle. 
    // Hand will be constructed pointing down +Z in this local space.
	glBindTexture(GL_TEXTURE_2D, texSkin);
	glColor3f(1.0f, 1.0f, 1.0f); 

	// Upper arm
	glPushMatrix();
	
	// Cap the shoulder to prevent hollow body view
	glPushMatrix();
	glBegin(GL_POLYGON);
	glNormal3f(0.0f, 0.0f, -1.0f);
	for (int j = 0; j <= 30; ++j) {
		float theta = (float)j / 30 * 2.0f * 3.14159265f;
		glTexCoord2f(0.5f + 0.5f*cos(theta), 0.5f + 0.5f*sin(theta));
		glVertex3f(0.10f * cos(theta), 0.10f * sin(theta), 0.0f);
	}
	glEnd();
	glPopMatrix();

	// ------------------------------------------
	// START CLOTHING ADDITIONS (UPPER ARM)
	// ------------------------------------------
	glDisable(GL_TEXTURE_2D);

	// 1. Dark Blue Upper Sleeve Cover (slightly larger than arm)
	glBindTexture(GL_TEXTURE_2D, texFabric);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f); 
	glPushMatrix();
	glRotatef(isLeft ? 5.0f : -5.0f, 0, 1, 0);
	drawProceduralArmPart(0.53f, 0.105f, 0.105f, 0.08f, 0.08f, 40, 20, 0.015f, 0.5f);
	glPopMatrix();

	// 2. Shoulder Shield (Pauldron)
	glPushMatrix();
	glTranslatef(isLeft ? -0.115f : 0.115f, 0.0f, 0.05f);
	glRotatef(isLeft ? -15.0f : 15.0f, 0, 1, 0); // Outward spread
	glRotatef(10.0f, 1, 0, 0);

	float peakZ = 0.25f;
	float peakX = isLeft ? -0.06f : 0.06f; 
	float width = 0.13f;

	glColor3f(0.2f, 0.22f, 0.3f); // Slate dark blue/grey
	glBegin(GL_TRIANGLES);
	// Outer side (away from arm)
	float nx = isLeft ? -1.0f : 1.0f;
	glNormal3f(nx, 0.0f, 0.0f);

	// Top half
	glVertex3f(0.0f, 0.0f, -0.08f); 
	glVertex3f(0.0f, -width, 0.1f);
	glVertex3f(peakX, 0.0f, peakZ);

	glVertex3f(0.0f, 0.0f, -0.08f); 
	glVertex3f(peakX, 0.0f, peakZ);
	glVertex3f(0.0f, width, 0.1f);

	// Bottom half
	glVertex3f(0.0f, -width, 0.1f);
	glVertex3f(0.0f, 0.0f, 0.35f); 
	glVertex3f(peakX, 0.0f, peakZ);

	glVertex3f(0.0f, width, 0.1f);
	glVertex3f(peakX, 0.0f, peakZ);
	glVertex3f(0.0f, 0.0f, 0.35f);
	glEnd();

	// Gold trim for Pauldron
	glBindTexture(GL_TEXTURE_2D, texGold);
	glColor3f(1.0f, 1.0f, 1.0f);
	float ox = isLeft ? -0.002f : 0.002f;
	glLineWidth(3.0f);
	glBegin(GL_LINE_LOOP);
	glVertex3f(ox, 0.0f, -0.08f); 
	glVertex3f(ox, -width, 0.1f);
	glVertex3f(ox, 0.0f, 0.35f);
	glVertex3f(ox, width, 0.1f);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(ox, 0.0f, -0.08f);
	glVertex3f(peakX + ox, 0.0f, peakZ);
	glVertex3f(ox, 0.0f, 0.35f);
	glEnd();
	glPopMatrix();

	// 3. Gold Bicep Band
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.105f); // Positioned consistently
	glRotatef(isLeft ? 5.0f : -5.0f, 0, 1, 0);
	glTranslatef(isLeft ? 0.01f : -0.01f, 0.0f, 0.0f);
	
	glBindTexture(GL_TEXTURE_2D, texGold);
	glColor3f(1.0f, 1.0f, 1.0f);
	GLUquadric* q = gluNewQuadric();
	gluQuadricTexture(q, GL_TRUE); // Enable texture for cylinder
	gluCylinder(q, 0.110f, 0.105f, 0.045f, 30, 1);

	gluDeleteQuadric(q);
	glPopMatrix();
	
	// 4. White Flared Sleeve
	glPushMatrix();
	glRotatef(isLeft ? 5.0f : -5.0f, 0, 1, 0);
	glTranslatef(0.0f, 0.0f, 0.35f); 
	
	// Dual Brown Bands (Top and Transition)
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.25f, 0.15f, 0.05f); // Dark Brown
	GLUquadric* qBand = gluNewQuadric();
	gluQuadricNormals(qBand, GLU_SMOOTH);
	
	// 1. TOP BAND (at sleeve origin, visible over blue fabric)
	glPushMatrix();
	float bRTop = 0.115f; 
	float bThickTop = 0.028f;
	gluDisk(qBand, 0.08f, bRTop, 30, 1); 
	gluCylinder(qBand, bRTop, bRTop, bThickTop, 30, 1); 
	glTranslatef(0.0f, 0.0f, bThickTop);
	gluDisk(qBand, 0.08f, bRTop, 30, 1); 
	glPopMatrix();

	// 2. BOTTOM BAND (at the blue fabric edge/transition)
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.16f); 
	float bRBot = 0.122f; 
	float bThickBot = 0.012f;
	gluDisk(qBand, 0.08f, bRBot, 30, 1); 
	gluCylinder(qBand, bRBot, bRBot, bThickBot, 30, 1); 
	glTranslatef(0.0f, 0.0f, bThickBot);
	gluDisk(qBand, 0.08f, bRBot, 30, 1); 
	glPopMatrix();

	gluDeleteQuadric(qBand);

	// White Flared Sleeve with Pleats
	glTranslatef(0.0f, 0.0f, 0.005f); // Minor offset to prevent Z-fighting with the band disk
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texWhiteSleeve);
	glColor3f(1.0f, 1.0f, 1.0f); 
	drawProceduralArmPart(0.42f, 0.09f, 0.09f, 0.17f, 0.17f, 60, 30, 0.02f, 0.5f, 0.0f, 2.0f * 3.14159265f, 0.015f, 8); 
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	// ------------------------------------------
	// END CLOTHING ADDITIONS (UPPER ARM)
	// ------------------------------------------

	// Rotate slightly outwards depending on left or right
	glRotatef(isLeft ? 5.0f : -5.0f, 0, 1, 0);
	drawProceduralArmPart(0.53f, 0.10f, 0.10f, 0.075f, 0.075f, 40, 40, 0.015f, 0.5f);

	// Elbow joint
	glTranslatef(0.0f, 0.0f, 0.53f);
	glBindTexture(GL_TEXTURE_2D, texFabric);
	glColor3f(1.0f, 1.0f, 1.0f);
	GLUquadric* quad = gluNewQuadric();
	gluQuadricTexture(quad, GL_TRUE);
	gluQuadricNormals(quad, GLU_SMOOTH);
	gluSphere(quad, 0.078f, 40, 40); 
	glBindTexture(GL_TEXTURE_2D, texSkin);

	// Forearm 
	// Smoothly transitions from circular elbow to flattened wrist
	glRotatef(-5, 1, 0, 0); // slight inner bend at elbow
	float wristRx = 0.045f;
	float wristRy = 0.030f;
	drawProceduralArmPart(0.53f, 0.075f, 0.075f, wristRx, wristRy, 40, 40, 0.012f, 0.3f); 

	// ------------------------------------------
	// START CLOTHING ADDITIONS (FOREARM)
	// ------------------------------------------
	
	// Forearm dark blue sleeve
	glBindTexture(GL_TEXTURE_2D, texFabric);
	glColor3f(1.0f, 1.0f, 1.0f);
	drawProceduralArmPart(0.53f, 0.078f, 0.078f, wristRx + 0.003f, wristRy + 0.003f, 40, 20, 0.012f, 0.3f);

	// Gold rings at wrist
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.46f); // Near end of forearm 
	glBindTexture(GL_TEXTURE_2D, texGold);
	glColor3f(1.0f, 1.0f, 1.0f);
	GLUquadric* qRings = gluNewQuadric();
	gluQuadricTexture(qRings, GL_TRUE);
	
	for(int r = 0; r < 3; r++) {
		glPushMatrix();
		glTranslatef(0.0f, 0.0f, r * 0.015f); 
		// Exact fit - forearm sleeve at this point is approx 0.052
		glScalef(1.0f, 0.82f, 1.0f); 
		gluCylinder(qRings, 0.053f, 0.053f, 0.010f, 30, 1);
		glPopMatrix();
	}
	gluDeleteQuadric(qRings);
	glPopMatrix();
	
	glBindTexture(GL_TEXTURE_2D, texSkin);
	// ------------------------------------------
	// END CLOTHING ADDITIONS (FOREARM)
	// ------------------------------------------

	// Wrist Joint 
	glTranslatef(0.0f, 0.0f, 0.53f);

	// Rotate hand so palms face inside
	glRotatef(isLeft ? 90.0f : -90.0f, 0, 0, 1);

	// Hand palm
	float knuckleRx = 0.06f; 
	float knuckleRy = 0.025f;
	float handLength = 0.16f;
	
	// bend the hand slightly up for a natural resting pose
	glRotatef(isLeft ? 5 : -5, 0, 1, 0);
	glRotatef(-5, 1, 0, 0);
	drawProceduralArmPart(handLength, wristRx, wristRy, knuckleRx, knuckleRy, 30, 20, 0.01f, 0.5f);

	// ROUNDED Palm Base (smooths out the flat stump seamlessly)
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, handLength);
	// Scale X to 1 (matches knuckleRx), Scale Y to match knuckleRy, Scale Z to 0.35 to round it gently
	glScalef(1.0f, knuckleRy / knuckleRx, 0.35f); 
	gluSphere(quad, knuckleRx, 30, 30);
	glPopMatrix();

	// Thumb 
	glPushMatrix();
	// Base position: shift closer to hand on X, more palm-side (-Y), closer to wrist (Z)
	glTranslatef(isLeft ? knuckleRx * 0.85f : -knuckleRx * 0.85f, -knuckleRy * 0.8f, handLength * 0.35f);
	
	// 1. Initial base orientation (open hand)
	glRotatef(isLeft ? 35.0f : -35.0f, 0, 1, 0); 
	glRotatef(10.0f, 1, 0, 0); // Minimal pitch to angle away from palm
	
	// 2. Base Clench: Sweeps sideways across the palm
	glRotatef(isLeft ? (-fistProgress * 65.0f) : (fistProgress * 65.0f), 0, 1, 0); 
	glRotatef(fistProgress * 15.0f, 1, 0, 0); // pitch away from palm to add space
	
	// Thumb base knuckle
	glPushMatrix();
	gluSphere(quad, 0.016f, 16, 16);
	glPopMatrix();

	drawProceduralArmPart(0.065f, 0.016f, 0.016f, 0.014f, 0.014f, 16, 16, 0.003f, 0.5f);
	
	glTranslatef(0.0f, 0.0f, 0.065f);
	gluSphere(quad, 0.014f, 16, 16);
	
	// 3. Second Knuckle Clench: folds sideways across the other fingers
	glRotatef(isLeft ? (-fistProgress * 60.0f) : (fistProgress * 60.0f), 0, 1, 0);
	glRotatef(fistProgress * 5.0f, 1, 0, 0); 
	
	drawProceduralArmPart(0.05f, 0.014f, 0.014f, 0.011f, 0.011f, 16, 16, 0.002f, 0.5f);
	
	glTranslatef(0.0f, 0.0f, 0.05f);
	gluSphere(quad, 0.011f, 16, 16);
	glPopMatrix();

	// Fingers 
	glTranslatef(0.0f, 0.0f, handLength);
	
	float fingerSpace = 0.028f;
	float fingerPos[] = { 1.5f * fingerSpace, 0.5f * fingerSpace, -0.5f * fingerSpace, -1.5f * fingerSpace };
	float fingerLength[] = { 0.075f, 0.085f, 0.08f, 0.06f }; // Further scaled-down finger sections
	float fingerAngles[] = { 6.0f, 0.0f, -4.0f, -10.0f }; // Fanning
	float fingerZOff[] = { 0.007f, 0.016f, 0.01f, 0.0f }; // Scaled arched Z sink offsets
	
	for (int i = 0; i < 4; i++) {
	    glPushMatrix();
	    glTranslatef(isLeft ? fingerPos[i] : -fingerPos[i], 0.0f, fingerZOff[i]);
	    
	    float rx = 0.014f; float ry = 0.012f;
	    
	    // Base knuckle
	    glPushMatrix();
	    gluSphere(quad, rx, 16, 16);
	    glPopMatrix();

	    glRotatef(isLeft ? fingerAngles[i] : -fingerAngles[i], 0, 1, 0); 
	    glRotatef(12.0f + fistProgress * 80.0f, 1, 0, 0); // relaxed curl blending to full clench
	    
	    drawProceduralArmPart(fingerLength[i], rx, ry, rx*0.9f, ry*0.9f, 16, 16, 0.003f, 0.5f);
	    
	    glTranslatef(0.0f, 0.0f, fingerLength[i]);
	    gluSphere(quad, rx*0.9f, 16, 16);

	    glRotatef(25.0f + fistProgress * 75.0f, 1, 0, 0); // curl 2
	    drawProceduralArmPart(fingerLength[i] * 0.7f, rx*0.9f, ry*0.9f, rx*0.75f, ry*0.75f, 16, 16, 0.002f, 0.5f);
	    
		glTranslatef(0.0f, 0.0f, fingerLength[i]*0.7f);
		gluSphere(quad, rx*0.75f, 16, 16);

		glRotatef(20.0f + fistProgress * 70.0f, 1, 0, 0); // tip curl
		drawProceduralArmPart(fingerLength[i] * 0.5f, rx*0.75f, ry*0.75f, rx*0.6f, ry*0.6f, 16, 16, 0.001f, 0.5f);

	    glTranslatef(0.0f, 0.0f, fingerLength[i]*0.5f);
	    gluSphere(quad, rx*0.6f, 16, 16);
	    glPopMatrix();
	}
	
	gluDeleteQuadric(quad);
	glPopMatrix();
}

void drawLeftArm()
{
	glPushMatrix();
	glTranslatef(-0.7f, 1.3f, 0.0f);
	glRotatef(leftArmAngle, 1, 0, 0);
	glRotatef(90, 1, 0, 0); // arm points down generally
	
	drawProceduralArmBase(true, leftFistProgress);

	glPopMatrix();
}

void drawRightArm()
{
	glPushMatrix();
	glTranslatef(0.7f, 1.3f, 0.0f);
	glRotatef(rightArmAngle, 1, 0, 0);
	glRotatef(90, 1, 0, 0); // arm points down generally

	drawProceduralArmBase(false, rightFistProgress);

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

GLuint loadBMP(const char* filename)
{
	GLuint texture = 0;
	FILE* file = fopen(filename, "rb");
	if (!file) {
		printf("Failed to open %s\n", filename);
		return 0;
	}
	
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
	
	// BMP uses BGR, OpenGL uses RGB. Perform swap manually.
	for (int i = 0; i < imageSize; i += 3) {
		unsigned char tmp = data[i];
		data[i] = data[i + 2];
		data[i + 2] = tmp;
	}
	
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	// Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	
	delete[] data;
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
	drawLeftArm();
	drawRightArm();

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
	texSkin = loadBMP("skin.bmp");
	texFabric = loadBMP("fabric.bmp");
	texGold = loadBMP("gold.bmp");
	texWhiteSleeve = loadBMP("white_sleeve.bmp");

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

		updateAnimation();
		display();

		SwapBuffers(hdc);
	}

	UnregisterClass(WINDOW_TITLE, wc.hInstance);

	return true;
}
//--------------------------------------------------------------------