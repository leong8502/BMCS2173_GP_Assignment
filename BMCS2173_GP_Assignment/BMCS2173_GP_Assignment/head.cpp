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

void drawBunnyEars() {
    float rOuter = 0.95f, gOuter = 0.95f, bOuter = 0.95f; 
    float rInner = 0.85f, gInner = 0.45f, bInner = 0.55f; 

    // Draw dual ears
    for (int e = -1; e <= 1; e += 2) {
        float tiltSide = e * 20.0f; 
        glPushMatrix();
        // Crown root attachment
        glTranslatef(e * 0.12f, -0.05f, 1.0f); 
        glRotatef(tiltSide, 0, 1, 0);
        glRotatef(-15.0f, 1, 0, 0); 
        
        int stacks = 15;
        int slices = 12;
        float height = 0.7f;
        
        for (int i = 0; i < stacks; i++) {
            float f1 = (float)i / stacks;
            float f2 = (float)(i + 1) / stacks;
            
            float z1 = f1 * height;
            float z2 = f2 * height;
            
            // Ear bows outwards then comes to a point
            float w1 = 0.12f * sin(f1 * 3.14159f);
            float w2 = 0.12f * sin(f2 * 3.14159f);
            
            // Bend backwards
            float yb1 = -0.1f * f1 * f1;
            float yb2 = -0.1f * f2 * f2;
            
            // Ear depth (thickness)
            float d1 = 0.02f * sin(f1 * 3.14159f);
            float d2 = 0.02f * sin(f2 * 3.14159f);
            
            glBegin(GL_QUAD_STRIP);
            for(int j=0; j<=slices; j++) {
                float th = (float)j/slices * 2.0f * 3.14159f;
                // Oval shape
                float px1 = w1 * cos(th);
                float py1 = yb1 + d1 * sin(th);
                float px2 = w2 * cos(th);
                float py2 = yb2 + d2 * sin(th);
                
                // Color: Inner part (front face) is pink, outer part is white
                if(sin(th) > 0.5f) glColor3f(rInner, gInner, bInner);
                else glColor3f(rOuter, gOuter, bOuter);
                
                glNormal3f(cos(th), sin(th), 0.0f);
                glVertex3f(px2, py2, z2);
                glNormal3f(cos(th), sin(th), 0.0f);
                glVertex3f(px1, py1, z1);
            }
            glEnd();
        }
        glPopMatrix();
    }
}

void drawHairOrnaments() {
    // Huge thick gold arcs over the forehead
    glColor3f(0.85f, 0.7f, 0.15f);
    
    for(int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        float px, py, pz;
        getHeadVertex(0.80f, 3.14159f*0.5f + side*0.3f, px, py, pz);
        glTranslatef(px, py + 0.05f, pz + 0.05f);
        
        glRotatef(side * 30.0f, 0, 1, 0);
        glRotatef(30.0f, 1, 0, 0);
        
        // Draw elegant curve horn
        int pieces = 10;
        glBegin(GL_QUAD_STRIP);
        for(int i=0; i<=pieces; i++) {
            float t = (float)i/pieces;
            float rad = 0.03f * (1.0f - t);
            float hX = side * t * 0.15f;
            float hY = -t * t * 0.1f;
            float hZ = t * 0.1f;
            
            for(int j=0; j<=6; j++) {
                float th = (float)j/6 * 2.0f * 3.14159f;
                float nx = cos(th), ny = sin(th);
                glNormal3f(nx, ny, 0.0f);
                glVertex3f(hX + nx*rad, hY + ny*rad, hZ);
            }
        }
        glEnd();
        
        // Embedded blue gem
        glColor3f(0.0f, 0.8f, 1.0f); // Cyan
        glTranslatef(0.0f, 0.0f, -0.02f);
        glScalef(1.0f, 0.3f, 1.5f);
        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.05, 16, 16);
        gluDeleteQuadric(quad);
        glColor3f(0.85f, 0.7f, 0.15f);
        glPopMatrix();
    }
}

void crossProduct3(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1]*b[2] - a[2]*b[1];
    out[1] = a[2]*b[0] - a[0]*b[2];
    out[2] = a[0]*b[1] - a[1]*b[0];
}

void normalize3(float v[3]) {
    float len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if(len > 0.0001f) { v[0]/=len; v[1]/=len; v[2]/=len; }
}

float bezier3(float t, float p0, float p1, float p2, float p3) {
    float mt = 1.0f - t;
    return mt*mt*mt*p0 + 3.0f*mt*mt*t*p1 + 3.0f*mt*t*t*p2 + t*t*t*p3;
}

float bezier3Deriv(float t, float p0, float p1, float p2, float p3) {
    float mt = 1.0f - t;
    return 3.0f*mt*mt*(p1 - p0) + 6.0f*mt*t*(p2 - p1) + 3.0f*t*t*(p3 - p2);
}

void getFrame(const float tng[3], float right[3], float up[3]) {
    float ref[3] = {0.0f, 0.0f, 1.0f};
    if (fabs(tng[2]) > 0.9f) { ref[0] = 1.0f; ref[2] = 0.0f; } 
    crossProduct3(tng, ref, right);
    normalize3(right);
    crossProduct3(right, tng, up);
    normalize3(up);
}

// Sweeps a 4-sided stylized diamond tube along a 3D Bezier curve
void drawTubularHairStrand(float p0[3], float p1[3], float p2[3], float p3[3], float radiusStart, float radiusEnd, float rTop, float gTop, float bTop, float rBot, float gBot, float bBot) {
    int steps = 14;
    int sides = 4; // Diamond cross section gives a sharp anime edge reflection
    
    for (int i = 0; i < steps; ++i) {
        float t1 = (float)i / steps;
        float t2 = (float)(i + 1) / steps;
        
        float pos1[3] = {bezier3(t1, p0[0], p1[0], p2[0], p3[0]),
                         bezier3(t1, p0[1], p1[1], p2[1], p3[1]),
                         bezier3(t1, p0[2], p1[2], p2[2], p3[2])};
        
        float pos2[3] = {bezier3(t2, p0[0], p1[0], p2[0], p3[0]),
                         bezier3(t2, p0[1], p1[1], p2[1], p3[1]),
                         bezier3(t2, p0[2], p1[2], p2[2], p3[2])};
                         
        float tng1[3] = {bezier3Deriv(t1, p0[0], p1[0], p2[0], p3[0]),
                         bezier3Deriv(t1, p0[1], p1[1], p2[1], p3[1]),
                         bezier3Deriv(t1, p0[2], p1[2], p2[2], p3[2])};
        normalize3(tng1);

        float tng2[3] = {bezier3Deriv(t2, p0[0], p1[0], p2[0], p3[0]),
                         bezier3Deriv(t2, p0[1], p1[1], p2[1], p3[1]),
                         bezier3Deriv(t2, p0[2], p1[2], p2[2], p3[2])};
        normalize3(tng2);
        
        float right1[3], up1[3], right2[3], up2[3];
        getFrame(tng1, right1, up1);
        getFrame(tng2, right2, up2);
        
        float currentRad1 = radiusStart + t1 * (radiusEnd - radiusStart);
        float currentRad2 = radiusStart + t2 * (radiusEnd - radiusStart);
        
        // Soft ambient occlusion shadowing if deep down
        float occ1 = (t1 > 0.8f) ? (1.0f - (t1 - 0.8f) * 2.0f) : 1.0f;
        float occ2 = (t2 > 0.8f) ? (1.0f - (t2 - 0.8f) * 2.0f) : 1.0f;
        
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= sides; j++) {
            float angle = (float)j / sides * 2.0f * 3.14159f;
            float cosA = cos(angle);
            float sinA = sin(angle);
            
            // Step 2 Vertex
            float nx2 = cosA * right2[0] + sinA * up2[0];
            float ny2 = cosA * right2[1] + sinA * up2[1];
            float nz2 = cosA * right2[2] + sinA * up2[2];
            glNormal3f(nx2, ny2, nz2);
            float curR2 = rTop + t2 * (rBot - rTop);
            float curG2 = gTop + t2 * (gBot - gTop);
            float curB2 = bTop + t2 * (bBot - bTop);
            glColor3f(curR2 * occ2, curG2 * occ2, curB2 * occ2);
            glVertex3f(pos2[0] + nx2*currentRad2, pos2[1] + ny2*currentRad2, pos2[2] + nz2*currentRad2);
            
            // Step 1 Vertex
            float nx1 = cosA * right1[0] + sinA * up1[0];
            float ny1 = cosA * right1[1] + sinA * up1[1];
            float nz1 = cosA * right1[2] + sinA * up1[2];
            glNormal3f(nx1, ny1, nz1);
            float curR1 = rTop + t1 * (rBot - rTop);
            float curG1 = gTop + t1 * (gBot - gTop);
            float curB1 = bTop + t1 * (bBot - bTop);
            glColor3f(curR1 * occ1, curG1 * occ1, curB1 * occ1);
            glVertex3f(pos1[0] + nx1*currentRad1, pos1[1] + ny1*currentRad1, pos1[2] + nz1*currentRad1);
        }
        glEnd();
    }
}

void drawDistinctHair()
{
    glDisable(GL_TEXTURE_2D);

    // --- BASE SCALP ---
    glColor3f(0.12f, 0.20f, 0.75f);
    GLUquadric* quad = gluNewQuadric();
    glPushMatrix();
    glTranslatef(0.0f, 0.02f, 0.90f);
    glScalef(0.24f, 0.27f, 0.15f);
    gluSphere(quad, 1.0, 32, 16);
    glPopMatrix();
    gluDeleteQuadric(quad);
    
    // Smooth root and tip gradient colors
    float rt = 0.10f, gt = 0.15f, bt = 0.75f;
    float rb = 0.45f, gb = 0.55f, bb = 0.95f;
    
    // LAYER 1: 50 Back Hair Tubular Strands (spreading outwards)
    for (int i = 0; i < 50; i++) {
        float f = (float)i / 49.0f;
        float theta = 0.2f - f * 3.54f; // Scatter from left shoulder to right shoulder
        
        float px, py, pz;
        getHeadVertex(0.95f, theta, px, py, pz);
        
        float randCurveX = sin(i * 14.3f) * 0.25f;
        float randCurveY = cos(i * 9.7f) * 0.25f;
        float randLength = sin(i * 22.1f) * 0.3f;
        float randWidth  = fabs(sin(i * 5.5f)) * 0.03f;
        
        float length = 1.7f + randLength;
        float p0[3] = {px, py, pz};
        
        // Ploy outwards from the scalp significantly to create spreading volume 
        float pullX = cos(theta);
        float pullY = sin(theta);
        float p1[3] = {px + pullX * 0.6f, py + pullY * 0.6f, pz - 0.2f};
        
        float endZ = pz - length;
        float endX = px + pullX * 0.8f + randCurveX;
        float endY = py + pullY * 0.8f + randCurveY;
        
        // Midway point adds the s-curve
        float p2[3] = {endX * 0.6f - randCurveX * 0.5f, endY * 0.6f - randCurveY * 0.5f, endZ + 0.5f};
        float p3[3] = {endX, endY, endZ};
        
        float radStart = 0.04f + randWidth;
        float radEnd = 0.005f; 
        
        drawTubularHairStrand(p0, p1, p2, p3, radStart, radEnd, rt, gt, bt, rb, gb, bb);
    }
    
    // LAYER 2: Separate Bangs / Locks Framing the Face
    for(int side = -1; side <= 1; side += 2) {
        float theta = 3.14159f*0.5f + side * 0.65f;
        float px, py, pz;
        getHeadVertex(0.85f, theta, px, py, pz);
        
        float p0[3] = {px, py, pz};
        float p1[3] = {px + side*0.3f, py + 0.15f, pz - 0.1f};
        float p2[3] = {px + side*0.35f, py + 0.05f, pz - 0.6f};
        float p3[3] = {px + side*0.15f, py - 0.10f, pz - 0.8f}; // Sweeping curl towards shoulder
        
        drawTubularHairStrand(p0, p1, p2, p3, 0.06f, 0.005f, rt, gt, bt, rb, gb, bb);
    }
    
    // LAYER 3: 7 Distinct Forward Bang Strands
    for (int i = 0; i < 7; i++) {
        float f = (float)i / 6.0f;
        float theta = 3.14159f*0.5f - 0.35f + f * 0.7f;
        
        float px, py, pz;
        getHeadVertex(0.88f, theta, px, py, pz);
        
        float p0[3] = {px, py, pz};
        float p1[3] = {px + cos(theta)*0.15f, py + sin(theta)*0.15f, pz - 0.05f};
        
        float curlX = sin(f*15.0f) * 0.05f;
        float p2[3] = {px + cos(theta)*0.20f, py + sin(theta)*0.20f, pz - 0.20f};
        float p3[3] = {px + cos(theta)*0.10f + curlX, py + sin(theta)*0.10f, pz - 0.38f + fabs(curlX)*0.5f}; 
        
        drawTubularHairStrand(p0, p1, p2, p3, 0.035f, 0.002f, rt, gt, bt, rb, gb, bb);
    }
    
    drawBunnyEars();
    drawHairOrnaments();
    
    glEnable(GL_TEXTURE_2D);
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
	glTranslatef(0.0f, 0.0f, 0.0f); // Centering handled by zooming out
    
    // Enable seamless compilation via OpenGL Display List
    static GLuint headCachedList = 0;
    if (headCachedList == 0) {
        headCachedList = glGenLists(1);
        glNewList(headCachedList, GL_COMPILE);
        drawHeadMesh();
        drawDistinctHair(); 
        glEndList();
    }
    glCallList(headCachedList);
    
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