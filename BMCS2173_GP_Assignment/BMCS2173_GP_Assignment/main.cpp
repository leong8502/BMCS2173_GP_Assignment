
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>

#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "GLU32.lib")
#pragma warning(disable:4996)

#define WINDOW_TITLE "Main Character"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//================================
// Camera variables
//================================

float cameraAngle = 0.0f;
float cameraHeight = 2.0f;
float cameraDistance = 8.0f;
float cameraX, cameraY, cameraZ;

//================================
// Character joint / animation variables
//================================

float leftArmAngle  = 0.0f;
float rightArmAngle = 0.0f;
float leftLegAngle  = 0.0f;
float rightLegAngle = 0.0f;

bool  attackAnimation = false;
float attackAngle     = 0.0f;

bool  leftFistActive   = false;
bool  rightFistActive  = false;
float leftFistProgress  = 0.0f;
float rightFistProgress = 0.0f;

// Movement and key state
float charX = 0.0f;
float charZ = 0.0f;
float charRotation = 0.0f;
float walkPhase = 0.0f;
float walkArmSwing = 0.0f;
bool keys[256];

bool weapon1_status = false; // Meteor Hammer equipped
bool weapon2_status = false; // Fan equipped
float fanSpreadAngle  = 0.0f;  // start folded
float fanTargetAngle  = 0.0f;

DWORD lastTime = 0;

//================================
// Textures
//================================

GLuint texGrass;        // background ground
GLuint texSkin;         // body / arm skin
GLuint texFabric;       // dark-blue fabric
GLuint texGold;         // gold trim
GLuint texWhiteSleeve;  // arm white sleeve
GLuint texWhiteLeather; // boot white leather
GLuint texDarkLeather;  // boot sole

GLuint texWeaponMetal;  // weapon metal
GLuint texWeaponWood;   // weapon wood
GLuint texWeaponChain;  // weapon chain

GLuint texFanWood;      // fan wood
GLuint texFan;          // fan fabric

// Body uses a display-list cache to avoid repeating heavy procedural math
GLuint cachedBodyList = 0;

//================================
// Utility: load a BMP texture
//================================

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

    // BMP stores BGR; flip to RGB while respecting 4-byte row padding
    int rowSize = ((width * 3) + 3) & ~3;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = y * rowSize + x * 3;
            unsigned char tmp = data[i];
            data[i]     = data[i + 2];
            data[i + 2] = tmp;
        }
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

//================================
// Camera helpers
//================================

void updateCamera()
{
    cameraX = sin(cameraAngle) * cameraDistance;
    cameraZ = cos(cameraAngle) * cameraDistance;
    cameraY = cameraHeight;
}

void resetAll()
{
    cameraAngle    = 0.0f;
    cameraHeight   = 2.0f;
    cameraDistance = 8.0f;
    cameraX = 0.0f; cameraY = 2.0f; cameraZ = 8.0f;

    leftArmAngle  = 0.0f; rightArmAngle = 0.0f;
    leftLegAngle  = 0.0f; rightLegAngle = 0.0f;

    attackAnimation = false; attackAngle = 0.0f;
    leftFistActive  = false; rightFistActive = false;
    leftFistProgress = 0.0f; rightFistProgress = 0.0f;

    charX = 0.0f; charZ = 0.0f; charRotation = 0.0f;
    walkPhase = 0.0f; walkArmSwing = 0.0f;
    for (int i = 0; i < 256; i++) keys[i] = false;

    weapon1_status = false;
    weapon2_status = false;
    leftFistActive = false;
    rightFistActive = false;
    fanTargetAngle = 0.0f;

    updateCamera();
}

//================================
// Weapon 1: Meteor Hammer logic
//================================

void getPathPoint(float t, float r, float L, float& px, float& py, float& nx, float& ny, float& traveled) {
    float arcLen = (float)M_PI * r;
    float totalLen = 2.0f * arcLen + 2.0f * L;
    float dist = t * totalLen;
    if (dist <= L) {
        px = -L / 2.0f + dist; py = r; nx = 0.0f; ny = 1.0f; traveled = dist;
    } else if (dist <= L + arcLen) {
        float a = (dist - L) / r; float angle = (float)M_PI / 2.0f - a;
        px = L / 2.0f + r * cos(angle); py = r * sin(angle); nx = cos(angle); ny = sin(angle); traveled = dist;
    } else if (dist <= 2.0f * L + arcLen) {
        float d = dist - (L + arcLen); px = L / 2.0f - d; py = -r; nx = 0.0f; ny = -1.0f; traveled = dist;
    } else {
        float d = dist - (2.0f * L + arcLen); float a = d / r; float angle = -(float)M_PI / 2.0f - a;
        px = -L / 2.0f + r * cos(angle); py = r * sin(angle); nx = cos(angle); ny = sin(angle); traveled = dist;
    }
}

void drawChainLink(GLUquadric* quad, float innerRadius, float outerRadius, float L, int nsides, int rings) {
    float arcLen = (float)M_PI * outerRadius;
    float totalLen = 2.0f * arcLen + 2.0f * L;
    for (int i = 0; i < rings; i++) {
        float t0 = (float)i / rings; float t1 = (float)(i + 1) / rings;
        float px0, py0, nx0, ny0, dist0_path; getPathPoint(t0, outerRadius, L, px0, py0, nx0, ny0, dist0_path);
        float px1, py1, nx1, ny1, dist1_path; getPathPoint(t1, outerRadius, L, px1, py1, nx1, ny1, dist1_path);
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= nsides; j++) {
            float phi = (float)j * 2.0f * (float)M_PI / nsides;
            float cosPhi = cos(phi); float sinPhi = sin(phi);
            float n0x = nx0 * cosPhi; float n0y = ny0 * cosPhi; float n0z = sinPhi;
            float v0x = px0 + innerRadius * n0x; float v0y = py0 + innerRadius * n0y; float v0z = innerRadius * n0z;
            float tx0 = (float)j / nsides; float ty0 = dist0_path / totalLen * 4.0f;
            glNormal3f(n0x, n0y, n0z); glTexCoord2f(tx0, ty0); glVertex3f(v0x, v0y, v0z);
            float n1x = nx1 * cosPhi; float n1y = ny1 * cosPhi; float n1z = sinPhi;
            float v1x = px1 + innerRadius * n1x; float v1y = py1 + innerRadius * n1y; float v1z = innerRadius * n1z;
            float ty1 = dist1_path / totalLen * 4.0f;
            glNormal3f(n1x, n1y, n1z); glTexCoord2f(tx0, ty1); glVertex3f(v1x, v1y, v1z);
        }
        glEnd();
    }
}

void drawMeteorHammer() {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH); gluQuadricTexture(quad, GL_TRUE);
    
    // Increase scale for better visibility and presence
    glPushMatrix();
    glScalef(0.18f, 0.18f, 0.18f);

    // 1. Wooden Handle
    glBindTexture(GL_TEXTURE_2D, texWeaponWood);
    glPushMatrix();
    glTranslatef(0.0f, -0.9f, 0.0f); // Centering handle in hand
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quad, 0.15f, 0.15f, 1.8f, 20, 20); 
    glPopMatrix();

    // 2. Metal parts
    glBindTexture(GL_TEXTURE_2D, texWeaponMetal);
    // Pommel / Top Rim
    for(int s_idx = 0; s_idx < 2; s_idx++) {
        int s = (s_idx == 0) ? -1 : 1;
        glPushMatrix();
        glTranslatef(0.0f, s == -1 ? -0.9f : 0.9f, 0.0f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        drawChainLink(quad, 0.05f, 0.16f, 0.0f, 15, 15);
        glPushMatrix(); glTranslatef(0, 0, -0.02f); gluDisk(quad, 0, 0.17f, 20, 1); glPopMatrix();
        glPopMatrix();
    }

    // 3. Chains
    glBindTexture(GL_TEXTURE_2D, texWeaponChain);
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        glTranslatef(0.0f, 1.1f + i * 0.32f, 0.0f);
        if (i % 2 == 0) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        drawChainLink(quad, 0.045f, 0.10f, 0.12f, 15, 15);
        glPopMatrix();
    }

    // 4. Meteor Sphere
    glBindTexture(GL_TEXTURE_2D, texWeaponMetal);
    glPushMatrix();
    glTranslatef(0.0f, 4.2f, 0.0f); 
    gluSphere(quad, 1.2f, 40, 40); 
    // Spikes (Cones)
    int numLat = 5; int numLon = 8;
    for (int lat = 1; lat < numLat; lat++) {
        float theta = lat * (float)M_PI / numLat;
        for (int lon = 0; lon < numLon; lon++) {
            float phi = lon * 2 * (float)M_PI / numLon;
            float x = sin(theta) * cos(phi); float z = sin(theta) * sin(phi); float y = cos(theta);
            glPushMatrix();
            glTranslatef(x * 1.15f, y * 1.15f, z * 1.15f);
            float angle = acos(z) * 180.0f / (float)M_PI;
            float len = sqrt(x*x + y*y);
            if (len > 0.0001f) glRotatef(angle, -y, x, 0.0f);
            else if (z < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(quad, 0.12f, 0.0f, 0.55f, 10, 5);
            glPopMatrix();
        }
    }
    glPopMatrix();

    glPopMatrix();
    gluDeleteQuadric(quad);
}

//================================
// Weapon 2: Fan drawing logic (ported from weapon2.cpp)
//================================

#ifndef PI
#define PI 3.14159265358979323846f
#endif

void drawFanCylinder(float baseRadius, float topRadius, float height, int slices, int stacks) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricDrawStyle(q, GLU_FILL);
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricTexture(q, GL_TRUE);
    gluCylinder(q, baseRadius, topRadius, height, slices, stacks);
    glPushMatrix(); glRotatef(180,1,0,0); gluDisk(q,0,baseRadius,slices,1); glPopMatrix();
    glPushMatrix(); glTranslatef(0,0,height); gluDisk(q,0,topRadius,slices,1); glPopMatrix();
    gluDeleteQuadric(q);
}

void drawFanRib(float length, float width, float thickness) {
    glPushMatrix();
    glScalef(width, length, thickness);
    glTranslatef(0.0f, 0.5f, 0.0f);
    glBegin(GL_QUADS);
    glNormal3f( 0,0, 1); glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f, 0.5f); glTexCoord2f(1,0); glVertex3f( 0.5f,-0.5f, 0.5f); glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f, 0.5f); glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f, 0.5f);
    glNormal3f( 0,0,-1); glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f,-0.5f); glTexCoord2f(1,1); glVertex3f(-0.5f, 0.5f,-0.5f); glTexCoord2f(0,1); glVertex3f( 0.5f, 0.5f,-0.5f); glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f,-0.5f);
    glNormal3f( 0,1, 0); glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f,-0.5f); glTexCoord2f(0,0); glVertex3f(-0.5f, 0.5f, 0.5f); glTexCoord2f(1,0); glVertex3f( 0.5f, 0.5f, 0.5f); glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f,-0.5f);
    glNormal3f( 0,-1,0); glTexCoord2f(1,1); glVertex3f(-0.5f,-0.5f,-0.5f); glTexCoord2f(0,1); glVertex3f( 0.5f,-0.5f,-0.5f); glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f, 0.5f); glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f, 0.5f);
    glNormal3f( 1,0, 0); glTexCoord2f(1,0); glVertex3f( 0.5f,-0.5f,-0.5f); glTexCoord2f(1,1); glVertex3f( 0.5f, 0.5f,-0.5f); glTexCoord2f(0,1); glVertex3f( 0.5f, 0.5f, 0.5f); glTexCoord2f(0,0); glVertex3f( 0.5f,-0.5f, 0.5f);
    glNormal3f(-1,0, 0); glTexCoord2f(0,0); glVertex3f(-0.5f,-0.5f,-0.5f); glTexCoord2f(1,0); glVertex3f(-0.5f,-0.5f, 0.5f); glTexCoord2f(1,1); glVertex3f(-0.5f, 0.5f, 0.5f); glTexCoord2f(0,1); glVertex3f(-0.5f, 0.5f,-0.5f);
    glEnd();
    glPopMatrix();
}

void drawFanGuard(float length, float thickness, float bL, float bR, float tL, float tR) {
    glPushMatrix();
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);  glTexCoord2f(0,0); glVertex3f(bL,0,thickness/2);  glTexCoord2f(1,0); glVertex3f(bR,0,thickness/2);  glTexCoord2f(1,1); glVertex3f(tR,length,thickness/2);  glTexCoord2f(0,1); glVertex3f(tL,length,thickness/2);
    glNormal3f(0,0,-1); glTexCoord2f(1,0); glVertex3f(bL,0,-thickness/2); glTexCoord2f(1,1); glVertex3f(tL,length,-thickness/2); glTexCoord2f(0,1); glVertex3f(tR,length,-thickness/2); glTexCoord2f(0,0); glVertex3f(bR,0,-thickness/2);
    glNormal3f(0,1,0);  glTexCoord2f(0,1); glVertex3f(tL,length,-thickness/2); glTexCoord2f(0,0); glVertex3f(tL,length,thickness/2);  glTexCoord2f(1,0); glVertex3f(tR,length,thickness/2);  glTexCoord2f(1,1); glVertex3f(tR,length,-thickness/2);
    glNormal3f(0,-1,0); glTexCoord2f(1,1); glVertex3f(bL,0,-thickness/2); glTexCoord2f(0,1); glVertex3f(bR,0,-thickness/2); glTexCoord2f(0,0); glVertex3f(bR,0,thickness/2);  glTexCoord2f(1,0); glVertex3f(bL,0,thickness/2);
    glNormal3f(1,0,0);  glTexCoord2f(1,0); glVertex3f(bR,0,-thickness/2); glTexCoord2f(1,1); glVertex3f(tR,length,-thickness/2); glTexCoord2f(0,1); glVertex3f(tR,length,thickness/2);  glTexCoord2f(0,0); glVertex3f(bR,0,thickness/2);
    glNormal3f(-1,0,0); glTexCoord2f(0,0); glVertex3f(bL,0,-thickness/2); glTexCoord2f(1,0); glVertex3f(bL,0,thickness/2);  glTexCoord2f(1,1); glVertex3f(tL,length,thickness/2);  glTexCoord2f(0,1); glVertex3f(tL,length,-thickness/2);
    glEnd();
    glPopMatrix();
}

void drawFanLeaf(int numFolds, float innerRadius, float outerRadius, float startAngle, float endAngle, float stackDepth) {
    float spreadAngle  = endAngle - startAngle;
    float closedFactor = 1.0f - (spreadAngle / 140.0f);
    int segPerGap = 10, total = numFolds * segPerGap;
    float step = (endAngle - startAngle) / total;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= total; i++) {
        float angle = startAngle + i * step;
        float rad = angle * PI / 180.0f;
        float t = (float)i / total;
        float baseZ = (stackDepth/2.0f) - stackDepth * t;
        int gIdx = (i == total) ? numFolds-1 : i / segPerGap;
        float lPos = (i == total) ? 1.0f : (float)(i % segPerGap) / segPerGap;
        float sign = (gIdx % 2 == 0) ? -1.0f : 1.0f;
        float tri = 1.0f - fabs(lPos * 2.0f - 1.0f);
        float fd = 0.14f * tri * closedFactor;
        float tx = -sin(rad), ty = cos(rad);
        float xI = innerRadius*cos(rad) + tx*fd*sign*(innerRadius/outerRadius);
        float yI = innerRadius*sin(rad) + ty*fd*sign*(innerRadius/outerRadius);
        float xO = outerRadius*cos(rad) + tx*fd*sign;
        float yO = outerRadius*sin(rad) + ty*fd*sign;
        float ozf = 0.2f*tri*sign*(1.0f-closedFactor);
        float zO = baseZ + ozf, zI = baseZ + ozf*(innerRadius/outerRadius);
        float px=tx,py=ty,pz=sign*0.5f,pl=sqrt(px*px+py*py+pz*pz);
        glNormal3f(px/pl,py/pl,pz/pl);
        float sh = 0.8f + tri*sign*0.15f; 
        glColor3f(sh, sh, sh); 
        float uI = ((innerRadius*cos(rad)/outerRadius)*0.5f+0.5f)*(1-closedFactor)+t*closedFactor;
        float vI = (innerRadius*sin(rad)/outerRadius)*(1-closedFactor)+(innerRadius/outerRadius)*closedFactor;
        float uO = ((outerRadius*cos(rad)/outerRadius)*0.5f+0.5f)*(1-closedFactor)+t*closedFactor;
        float vO = (outerRadius*sin(rad)/outerRadius)*(1-closedFactor)+1.0f*closedFactor;
        glTexCoord2f(uI,vI); glVertex3f(xI,yI,zI);
        glTexCoord2f(uO,vO); glVertex3f(xO,yO,zO);
    }
    glEnd();
}

void drawFan(float spreadAngle) {
    float ribCount = 21;
    float minAngle = 90.0f - (spreadAngle / 2.0f);
    float maxAngle = 90.0f + (spreadAngle / 2.0f);
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float stackDepth = 0.35f;

    glPushMatrix();
    // Scale down to fit character hand
    glScalef(0.12f, 0.12f, 0.12f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texFanWood);

    // Pivot pin
    glPushMatrix();
    glColor3fv(white);
    glTranslatef(0,0,-(stackDepth/2.0f+0.04f));
    drawFanCylinder(0.12f,0.12f,stackDepth+0.08f,30,5);
    glPopMatrix();

    // Inner ribs
    float ribLen=2.05f, ribW=0.08f, ribThk=0.015f;
    float angleStep = spreadAngle / (ribCount-1);
    for (int i=1;i<(int)ribCount-1;i++) {
        float ang = minAngle + i*angleStep;
        glPushMatrix();
        glRotatef(ang-90.0f,0,0,1);
        glColor3fv(white);
        float zOff = (stackDepth/2.0f) - (stackDepth*i/(ribCount-1));
        glTranslatef(0,0,zOff);
        drawFanRib(ribLen,ribW,ribThk);
        glPopMatrix();
    }

    // Outer guards
    float gLen=5.05f,gBW=0.4f,gTW=0.35f,gThk=0.04f;
    glPushMatrix(); glRotatef(minAngle-90.0f,0,0,1); glTranslatef(0,0,stackDepth/2.0f+gThk/2.0f); glColor3fv(white); drawFanGuard(gLen,gThk,-gBW/2,gBW/2,-gTW/2,gTW/2); glPopMatrix();
    glPushMatrix(); glRotatef(maxAngle-90.0f,0,0,1); glTranslatef(0,0,-(stackDepth/2.0f+gThk/2.0f)); glColor3fv(white); drawFanGuard(gLen,gThk,-gBW/2,gBW/2,-gTW/2,gTW/2); glPopMatrix();

    // Fan leaf
    glBindTexture(GL_TEXTURE_2D, texFan);
    glColor3fv(white);
    drawFanLeaf((int)ribCount-1, 2.0f, 5.0f, minAngle, maxAngle, stackDepth);

    glPopMatrix();
}

//================================
// Animation update
//================================

void updateAnimation()
{
    DWORD currentTime = GetTickCount();
    if (lastTime == 0) lastTime = currentTime;
    float dt = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;

    float speed = 0.5f;
    if (rightFistActive) rightFistProgress += speed * dt;
    else                 rightFistProgress -= speed * dt;
    if (rightFistProgress > 1.0f) rightFistProgress = 1.0f;
    if (rightFistProgress < 0.0f) rightFistProgress = 0.0f;

    if (leftFistActive) leftFistProgress += speed * dt;
    else                leftFistProgress -= speed * dt;
    if (leftFistProgress > 1.0f) leftFistProgress = 1.0f;
    if (leftFistProgress < 0.0f) leftFistProgress = 0.0f;

    if (attackAnimation) {
        attackAngle += 2.0f;
        rightArmAngle = attackAngle;
        if (attackAngle > 90) {
            attackAnimation = false;
            rightArmAngle   = 0;
        }
    }

    // WASD Movement and Leg Animation
    float moveSpeed = 2.5f;
    float rotSpeed = 100.0f; // degrees per second
    bool moving = false;

    if (keys['A']) charRotation += rotSpeed * dt;
    if (keys['D']) charRotation -= rotSpeed * dt;

    float rad = charRotation * 3.14159265f / 180.0f;
    float dx = sin(rad) * moveSpeed * dt;
    float dz = cos(rad) * moveSpeed * dt;

    if (keys['W']) { charX += dx; charZ += dz; moving = true; }
    if (keys['S']) { charX -= dx; charZ -= dz; moving = true; }

    if (moving) {
        walkPhase += dt * 12.0f; // speed of leg oscillation
        leftLegAngle  = sin(walkPhase) * 35.0f;
        rightLegAngle = -sin(walkPhase) * 35.0f;
        walkArmSwing  = sin(walkPhase) * 10.0f; // subtle arm swing
    } else {
        // Smoothly reset legs and arm swing to neutral
        float resetSpeed = 8.0f;
        leftLegAngle  -= leftLegAngle * resetSpeed * dt;
        rightLegAngle -= rightLegAngle * resetSpeed * dt;
        walkArmSwing  -= walkArmSwing * resetSpeed * dt;

        if (fabs(leftLegAngle) < 0.1f)  leftLegAngle = 0;
        if (fabs(rightLegAngle) < 0.1f) rightLegAngle = 0;
        if (fabs(walkArmSwing) < 0.1f)  walkArmSwing = 0;
    }

    // Fan spread animation
    float fanSpeed = 180.0f; // degrees per second
    if (fanSpreadAngle < fanTargetAngle) {
        fanSpreadAngle += fanSpeed * dt;
        if (fanSpreadAngle > fanTargetAngle) fanSpreadAngle = fanTargetAngle;
    } else if (fanSpreadAngle > fanTargetAngle) {
        fanSpreadAngle -= fanSpeed * dt;
        if (fanSpreadAngle < fanTargetAngle) fanSpreadAngle = fanTargetAngle;
    }
}

//================================
// Window procedure
//================================

LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        if (wParam < 256) keys[wParam] = true;
        switch (wParam)
        {
        case VK_ESCAPE: PostQuitMessage(0); break;

        // Reset
        case VK_SPACE: resetAll(); break;

        // Camera orbit / height / zoom
        case VK_LEFT:      cameraAngle   -= 0.05f; break;
        case VK_RIGHT:     cameraAngle   += 0.05f; break;
        case VK_UP:        cameraHeight  += 0.3f;  break;
        case VK_DOWN:
            cameraHeight -= 0.3f;
            if (cameraHeight < 0.5f) cameraHeight = 0.5f;
            break;
        case VK_ADD:    case VK_OEM_PLUS:  cameraDistance -= 0.3f; break;
        case VK_SUBTRACT: case VK_OEM_MINUS: cameraDistance += 0.3f; break;

        // NEW Arm rotation (ZX = left, CV = right)
        case 'Z': leftArmAngle += 5; if (leftArmAngle > 120) leftArmAngle = 120; break;
        case 'X': leftArmAngle -= 5; if (leftArmAngle < 0)   leftArmAngle = 0;   break;
        case 'C': rightArmAngle += 5; if (rightArmAngle > 120) rightArmAngle = 120; break;
        case 'V': rightArmAngle -= 5; if (rightArmAngle < 0)   rightArmAngle = 0;   break;

        case VK_F1:
            if (weapon2_status) weapon2_status = false;
            weapon1_status = !weapon1_status;
            leftFistActive = (weapon1_status || weapon2_status);
            break;

        case VK_F2:
            if (weapon1_status) weapon1_status = false;
            weapon2_status = !weapon2_status;
            leftFistActive = (weapon1_status || weapon2_status);
            if (weapon2_status) fanTargetAngle = 0.0f; 
            break;

        case '5':
            // Toggle fan open/close if it is equipped
            if (weapon2_status) {
                fanTargetAngle = (fanTargetAngle > 70.0f) ? 0.0f : 140.0f;
            }
            break;

        // Attack animation
        case 'F': attackAnimation = true; attackAngle = 0; break;

        // Fist toggle  (1 = right, 2 = left)
        case '2': rightFistActive = !rightFistActive; break;
        case '1': 
            if (weapon1_status || weapon2_status) break;
            leftFistActive  = !leftFistActive;  
            break;
        }
        break;

    case WM_KEYUP:
        if (wParam < 256) keys[wParam] = false;
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//================================
// OpenGL init
//================================

bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.cAlphaBits  = 8; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.cStencilBits = 0;
    pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iLayerType  = PFD_MAIN_PLANE;
    pfd.iPixelType  = PFD_TYPE_RGBA;
    pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion    = 1;
    int n = ChoosePixelFormat(hdc, &pfd);
    return SetPixelFormat(hdc, n, &pfd) ? true : false;
}

void initOpenGL()
{
    glClearColor(0.45f, 0.72f, 0.95f, 1.0f); // sky blue
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0 / 600.0, 0.1f, 200.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
}

void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    float sunX = 12.0f, sunY = 18.0f, sunZ = -30.0f;
    GLfloat lightPos[]  = { sunX, sunY, sunZ, 1.0f };
    GLfloat ambient[]   = { 0.35f, 0.35f, 0.30f, 1.0f };
    GLfloat diffuse[]   = { 1.0f,  0.95f, 0.80f, 1.0f };
    GLfloat specular[]  = { 1.0f,  1.0f,  0.90f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
}

//================================================================
//  BACKGROUND  (from background.cpp)
//================================================================

void drawGround()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glColor3f(1.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f,  0.0f); glVertex3f(-60, 0, -60);
    glTexCoord2f( 0.0f, 20.0f); glVertex3f(-60, 0,  60);
    glTexCoord2f(20.0f, 20.0f); glVertex3f( 60, 0,  60);
    glTexCoord2f(20.0f,  0.0f); glVertex3f( 60, 0, -60);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawCloud(float cx, float cy, float cz, float scale)
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    float offsets[][4] = {
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 1.2f, 0.3f, 0.0f, 0.8f },
        {-1.2f, 0.3f, 0.0f, 0.8f },
        { 0.6f, 0.7f, 0.0f, 0.7f },
        {-0.6f, 0.7f, 0.0f, 0.7f },
        { 0.0f, 0.9f, 0.0f, 0.6f },
        { 1.8f, 0.0f, 0.0f, 0.5f },
        {-1.8f, 0.0f, 0.0f, 0.5f },
    };
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        glTranslatef(cx + offsets[i][0]*scale, cy + offsets[i][1]*scale, cz);
        gluSphere(q, offsets[i][3]*scale, 16, 16);
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

void drawSun(float sx, float sy, float sz)
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 0.95f, 0.3f);
    glPushMatrix(); glTranslatef(sx, sy, sz); gluSphere(q, 1.5f, 32, 32); glPopMatrix();
    glColor3f(1.0f, 0.85f, 0.2f);
    glPushMatrix(); glTranslatef(sx, sy, sz); gluSphere(q, 1.9f, 32, 32); glPopMatrix();
    gluDeleteQuadric(q);
    glEnable(GL_LIGHTING);
}

void drawBackground()
{
    float sunX = 12.0f, sunY = 18.0f, sunZ = -30.0f;
    GLfloat lightPos[] = { sunX, sunY, sunZ, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    drawSun(sunX, sunY, sunZ);

    drawCloud(-18.0f, 14.0f, -40.0f, 1.5f);
    drawCloud(-25.0f, 16.0f, -35.0f, 1.2f);
    drawCloud( 20.0f, 15.0f, -38.0f, 1.8f);
    drawCloud( 28.0f, 13.0f, -32.0f, 1.1f);
    drawCloud(-10.0f, 12.0f, -28.0f, 1.4f);
    drawCloud( 12.0f, 11.0f, -25.0f, 1.6f);
    drawCloud(-20.0f, 10.0f, -18.0f, 1.0f);
    drawCloud( 22.0f, 12.0f, -20.0f, 1.3f);
    drawCloud(  0.0f, 17.0f, -50.0f, 2.0f);
    drawCloud(-32.0f, 14.0f, -45.0f, 1.7f);

    drawGround();
}

//================================================================
//  LEGS  (from legs.cpp)
//================================================================

void drawProceduralLegPart(float length, float topRx, float topRy,
                            float bottomRx, float bottomRy,
                            int slices, int stacks,
                            float bulgeFactor, float bulgePos)
{
    for (int i = 0; i < stacks; ++i) {
        float t1 = (float)i / stacks;
        float t2 = (float)(i + 1) / stacks;
        float z1 = t1 * length, z2 = t2 * length;

        float rx1 = topRx*(1-t1)+bottomRx*t1;
        float ry1 = topRy*(1-t1)+bottomRy*t1;
        float b1  = sin(t1*3.14159265f)*bulgeFactor*exp(-pow(t1-bulgePos,2)*10.0f);
        rx1+=b1; ry1+=b1;

        float rx2 = topRx*(1-t2)+bottomRx*t2;
        float ry2 = topRy*(1-t2)+bottomRy*t2;
        float b2  = sin(t2*3.14159265f)*bulgeFactor*exp(-pow(t2-bulgePos,2)*10.0f);
        rx2+=b2; ry2+=b2;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float theta = (float)j/slices * 2.0f*3.14159265f;
            float s     = (float)j/slices;
            float cosT  = cos(theta), sinT = sin(theta);

            float nx1=ry1*cosT, ny1=rx1*sinT, nz1=(topRx-bottomRx)/length*((rx1+ry1)*0.5f);
            float len1=sqrt(nx1*nx1+ny1*ny1+nz1*nz1);
            if(len1>0){nx1/=len1;ny1/=len1;nz1/=len1;}

            float nx2=ry2*cosT, ny2=rx2*sinT, nz2=(topRx-bottomRx)/length*((rx2+ry2)*0.5f);
            float len2=sqrt(nx2*nx2+ny2*ny2+nz2*nz2);
            if(len2>0){nx2/=len2;ny2/=len2;nz2/=len2;}

            glNormal3f(nx2,ny2,nz2);
            glTexCoord2f(s*2.0f, t2*2.0f);
            glVertex3f(rx2*cosT, ry2*sinT, z2);

            glNormal3f(nx1,ny1,nz1);
            glTexCoord2f(s*2.0f, t1*2.0f);
            glVertex3f(rx1*cosT, ry1*sinT, z1);
        }
        glEnd();
    }
}

void drawRoundTrim(float r)
{
    // Front
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,1,0); glVertex3f(0,r,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(cos(a)*r*0.9f,r,sin(a)*r*1.4f); }
    glEnd();
    // Back
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,-1,0); glVertex3f(0,-r,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(-cos(a)*r*0.9f,-r,sin(a)*r*1.4f); }
    glEnd();
    // Right
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(1,0,0); glVertex3f(r,0,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(r,-cos(a)*r*0.9f,sin(a)*r*1.4f); }
    glEnd();
    // Left
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(-1,0,0); glVertex3f(-r,0,0);
    for(int i=0;i<=10;++i){ float a=(float)i/10*3.14159f; glVertex3f(-r,cos(a)*r*0.9f,sin(a)*r*1.4f); }
    glEnd();
}

void drawLegBase(bool isLeft)
{
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluQuadricTexture(quad, GL_TRUE);

    glPushMatrix();
    glRotatef(90, 1, 0, 0);  // leg points downward in Y-up space

    if (!isLeft) {
        // Right leg: short sock (below knee)
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
        // Right leg: lower calf
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
        drawProceduralLegPart(0.75f, 0.19f,0.20f, 0.12f,0.12f, 60,40, 0.015f,0.5f);
        glTranslatef(0,0,0.75f);
        gluSphere(quad, 0.095f, 40,40);
        drawProceduralLegPart(0.12f, 0.12f,0.12f, 0.13f,0.13f, 60,20, 0.005f,0.5f);
        glTranslatef(0,0,0.12f);
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
        drawProceduralLegPart(0.06f, 0.14f,0.14f, 0.13f,0.13f, 60,20, 0.0f,0.5f);
        drawRoundTrim(0.14f);
        glTranslatef(0,0,0.06f);
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texFabric);
        drawProceduralLegPart(0.65f, 0.12f,0.12f, 0.07f,0.07f, 60,50, 0.015f,0.2f);
        glTranslatef(0,0,0.65f);
    } else {
        // Left leg: high sock (mid thigh)
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
        drawProceduralLegPart(0.45f, 0.19f,0.20f, 0.15f,0.15f, 60,40, 0.012f,0.5f);
        glTranslatef(0,0,0.45f);
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
        drawProceduralLegPart(0.06f, 0.16f,0.16f, 0.14f,0.14f, 60,20, 0.0f,0.5f);
        drawRoundTrim(0.16f);
        glTranslatef(0,0,0.06f);
        glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texFabric);
        drawProceduralLegPart(0.24f, 0.14f,0.14f, 0.12f,0.12f, 60,30, 0.012f,0.5f);
        glTranslatef(0,0,0.24f);
        gluSphere(quad, 0.095f, 40,40);
        drawProceduralLegPart(0.12f, 0.12f,0.12f, 0.13f,0.13f, 60,20, 0.005f,0.5f);
        glTranslatef(0,0,0.12f);
        drawProceduralLegPart(0.06f, 0.14f,0.14f, 0.13f,0.13f, 60,20, 0.0f,0.5f);
        glTranslatef(0,0,0.06f);
        drawProceduralLegPart(0.65f, 0.12f,0.12f, 0.07f,0.07f, 60,50, 0.015f,0.2f);
        glTranslatef(0,0,0.65f);
    }

    // Boot inner ankle
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texWhiteLeather);
    drawProceduralLegPart(0.12f, 0.08f,0.08f, 0.085f,0.085f, 60,20, 0.0f,0.0f);

    // Boot flap top
    glPushMatrix();
    glTranslatef(0,0,-0.08f);
    drawProceduralLegPart(0.10f, 0.11f,0.11f, 0.085f,0.085f, 60,20, 0.0f,0.5f);
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
    gluCylinder(quad, 0.12f, 0.12f, 0.02f, 60,20);
    glTranslatef(0,0,0.02f);
    gluCylinder(quad, 0.12f, 0.09f, 0.05f, 60,20);
    glPopMatrix();

    // Gold balls (both sides)
    glPushMatrix(); glTranslatef( 0.095f,0,0.04f); gluSphere(quad,0.045f,40,40); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.095f,0,0.04f); gluSphere(quad,0.045f,40,40); glPopMatrix();

    glTranslatef(0,0,0.13f); // ankle joint

    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texWhiteLeather);
    gluSphere(quad, 0.09f, 40,40);

    // Boot foot group
    glPushMatrix();
    glTranslatef(0,0.08f,0.06f);
    glRotatef(20.0f,1,0,0);

    glPushMatrix();
    glScalef(0.12f,0.20f,0.12f);
    gluSphere(quad,1.0f,60,60);
    glPopMatrix();

    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texDarkLeather);
    glPushMatrix();
    glTranslatef(0,0.04f,0.07f);
    glScalef(0.09f,0.14f,0.035f);
    gluSphere(quad,1.0f,30,30);
    glPopMatrix();
    glPopMatrix();

    // Heel (Positioned at back curve, lengthened to prevent burial)
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texGold);
    glPushMatrix();
    // Move slightly back (-Y) and set starting depth
    glTranslatef(0, -0.025f, 0.02f); 
    gluCylinder(quad, 0.050, 0.03, 0.175f, 40, 40);
    glTranslatef(0,0,0.175f);
    gluDisk(quad, 0.0, 0.04, 40, 1);
    glPopMatrix();

    gluDeleteQuadric(quad);
    glPopMatrix();
}

void drawLeftLeg()
{
    glPushMatrix();
    glTranslatef(-0.16f, 1.53f, 0.0f);
    glRotatef(leftLegAngle, 1, 0, 0);
    drawLegBase(true);
    glPopMatrix();
}

void drawRightLeg()
{
    glPushMatrix();
    glTranslatef(0.16f, 1.53f, 0.0f);
    glRotatef(rightLegAngle, 1, 0, 0);
    drawLegBase(false);
    glPopMatrix();
}

//================================================================
//  ARMS  (from arms.cpp)
//================================================================

void drawProceduralArmPart(float length,
    float topRx, float topRy, float bottomRx, float bottomRy,
    int slices, int stacks, float bulgeFactor, float bulgePos,
    float sweepStart = 0.0f, float sweepEnd = 2.0f*3.14159265f,
    float pleatDepth = 0.0f, int pleatCount = 0)
{
    for (int i = 0; i < stacks; ++i) {
        float t1 = (float)i/stacks, t2 = (float)(i+1)/stacks;
        float z1 = t1*length, z2 = t2*length;

        float rx1=topRx*(1-t1)+bottomRx*t1, ry1=topRy*(1-t1)+bottomRy*t1;
        float b1=sin(t1*3.14159265f)*bulgeFactor*exp(-pow(t1-bulgePos,2)*10.0f);
        rx1+=b1; ry1+=b1;

        float rx2=topRx*(1-t2)+bottomRx*t2, ry2=topRy*(1-t2)+bottomRy*t2;
        float b2=sin(t2*3.14159265f)*bulgeFactor*exp(-pow(t2-bulgePos,2)*10.0f);
        rx2+=b2; ry2+=b2;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float ts = (float)j/slices;
            float theta = sweepStart*(1-ts)+sweepEnd*ts;
            float cosT=cos(theta), sinT=sin(theta);
            float u=(float)j/slices;

            float pM=(pleatCount>0)?cos(theta*pleatCount):0.0f;
            float ro1=pM*pleatDepth*t1, ro2=pM*pleatDepth*t2;
            float rx1p=rx1+ro1, ry1p=ry1+ro1;
            float rx2p=rx2+ro2, ry2p=ry2+ro2;

            float shade=1.0f;
            if(pleatCount>0){
                float pS=0.75f+0.25f*(pM*0.5f+0.5f);
                shade=1.0f*(1-t1)+pS*t1;
            }

            float avgR1=(rx1+ry1)*0.5f;
            float nx1=ry1*cosT, ny1=rx1*sinT, nz1=(topRx-bottomRx)/length*avgR1;
            float l1=sqrt(nx1*nx1+ny1*ny1+nz1*nz1);
            if(l1>0){nx1/=l1;ny1/=l1;nz1/=l1;}

            float avgR2=(rx2+ry2)*0.5f;
            float nx2=ry2*cosT, ny2=rx2*sinT, nz2=(topRx-bottomRx)/length*avgR2;
            float l2=sqrt(nx2*nx2+ny2*ny2+nz2*nz2);
            if(l2>0){nx2/=l2;ny2/=l2;nz2/=l2;}

            glColor3f(shade,shade,shade);
            glNormal3f(nx2,ny2,nz2); glTexCoord2f(u,t2); glVertex3f(rx2p*cosT, ry2p*sinT, z2);
            glNormal3f(nx1,ny1,nz1); glTexCoord2f(u,t1); glVertex3f(rx1p*cosT, ry1p*sinT, z1);
        }
        glEnd();
        glColor3f(1,1,1);
    }
}

void drawProceduralArmBase(bool isLeft, float fistProgress, int part)
{
    GLUquadric* quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);

    glBindTexture(GL_TEXTURE_2D, texSkin);
    glColor3f(1,1,1);

    glPushMatrix();

    // Small Shoulder Assembly (2 cylinders + 1 sphere)
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, texSkin);
    // Position the assembly at the arm rotation center (origin)
    glTranslatef(0.0f, 0.0f, 0.0f); 
    
    if (part == 0) {
        // 2. Torso Connector Cylinder (Static part)
        glPushMatrix();
        glRotatef(isLeft ? 90.0f : -90.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(quad, 0.12f, 0.12f, 0.22f, 32, 2);
        glPopMatrix();
    } else {
        // 1. Central Ball Joint (Rotating part)
        gluSphere(quad, 0.12f, 32, 32);

        // 3. Arm Connector Cylinder (Rotating part)
        glPushMatrix();
        gluCylinder(quad, 0.12f, 0.12f, 0.22f, 32, 2);
        glPopMatrix();
    }

    glPopMatrix();

    if (part == 0) {
        glPopMatrix();
        gluDeleteQuadric(quad);
        return;
    }

    // Shift the arm mesh down to make the shoulder connector cylinder visible
    glTranslatef(0.0f, 0.0f, 0.18f); 
    // Apply outward tilt to the WHOLE arm (from joint down)
    glRotatef(isLeft?-10.0f:10.0f, 0, 1, 0);

    // ---- Upper arm clothing ----
    glDisable(GL_TEXTURE_2D);

    // 1. Dark blue sleeve
    glBindTexture(GL_TEXTURE_2D, texFabric);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);
    glPushMatrix();
    drawProceduralArmPart(0.53f,0.13f,0.13f,0.10f,0.10f,40,20,0.015f,0.5f);
    glPopMatrix();

    // 2. Pauldron
    glPushMatrix();
    glTranslatef(isLeft?-0.115f:0.115f, 0.0f, 0.05f);
    glRotatef(isLeft?-15.0f:15.0f, 0,1,0);
    glRotatef(10.0f, 1,0,0);
    float peakZ=0.25f, peakX=isLeft?-0.06f:0.06f, width=0.13f;
    glColor3f(0.2f,0.22f,0.3f);
    glBegin(GL_TRIANGLES);
    float nx_p=isLeft?-1.0f:1.0f;
    glNormal3f(nx_p,0,0);
    glVertex3f(0,0,-0.08f); glVertex3f(0,-width,0.1f); glVertex3f(peakX,0,peakZ);
    glVertex3f(0,0,-0.08f); glVertex3f(peakX,0,peakZ); glVertex3f(0,width,0.1f);
    glVertex3f(0,-width,0.1f); glVertex3f(0,0,0.35f); glVertex3f(peakX,0,peakZ);
    glVertex3f(0,width,0.1f); glVertex3f(peakX,0,peakZ); glVertex3f(0,0,0.35f);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, texGold); glColor3f(1,1,1);
    float ox=isLeft?-0.002f:0.002f;
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(ox,0,-0.08f); glVertex3f(ox,-width,0.1f);
    glVertex3f(ox,0,0.35f);  glVertex3f(ox,width,0.1f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(ox,0,-0.08f); glVertex3f(peakX+ox,0,peakZ); glVertex3f(ox,0,0.35f);
    glEnd();
    glPopMatrix();

    // 3. Two Gold bicep bands
    glBindTexture(GL_TEXTURE_2D, texGold); glColor3f(1,1,1);
    for(int i=0; i<2; i++) {
        glPushMatrix();
        glTranslatef(0, 0, 0.08f + i*0.06f);
        gluCylinder(quad, 0.136f, 0.136f, 0.035f, 30, 1);
        glPopMatrix();
    }

    // 4. White flared sleeve
    glPushMatrix();
    glTranslatef(0,0,0.35f);

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.25f,0.15f,0.05f);
    GLUquadric* qBand=gluNewQuadric();
    gluQuadricNormals(qBand,GLU_SMOOTH);

    glPushMatrix();
    float bRTop=0.120f, bThickTop=0.028f; // Increased radius to prevent clipping
    gluDisk(qBand,0.08f,bRTop,30,1);
    gluCylinder(qBand,bRTop,bRTop,bThickTop,30,1);
    glTranslatef(0,0,bThickTop); gluDisk(qBand,0.08f,bRTop,30,1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0,0,0.16f);
    float bRBot=0.127f, bThickBot=0.012f; // Increased radius to prevent clipping
    gluDisk(qBand,0.08f,bRBot,30,1);
    gluCylinder(qBand,bRBot,bRBot,bThickBot,30,1);
    glTranslatef(0,0,bThickBot); gluDisk(qBand,0.08f,bRBot,30,1);
    glPopMatrix();
    gluDeleteQuadric(qBand);

    glTranslatef(0,0,0.005f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWhiteSleeve); glColor3f(1,1,1);
    drawProceduralArmPart(0.42f,0.115f,0.115f,0.21f,0.21f,60,30,0.02f,0.5f,
                          0.0f,2.0f*3.14159265f,0.02f,8);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);

    // Upper arm skin
    drawProceduralArmPart(0.53f,0.125f,0.125f,0.095f,0.095f,40,40,0.015f,0.5f);

    // Elbow joint
    glTranslatef(0,0,0.53f);
    glBindTexture(GL_TEXTURE_2D, texFabric); glColor3f(1,1,1);
    gluSphere(quad,0.11f,40,40);
    glBindTexture(GL_TEXTURE_2D, texSkin);

    // Forearm
    glRotatef(-5,1,0,0);
    float wristRx= 0.065f, wristRy= 0.045f;
    drawProceduralArmPart(0.53f,0.10f,0.10f,wristRx,wristRy,40,40,0.015f,0.3f);

    // Forearm sleeve
    glBindTexture(GL_TEXTURE_2D, texFabric); glColor3f(1,1,1);
    drawProceduralArmPart(0.53f,0.105f,0.105f,wristRx+0.003f,wristRy+0.003f,40,20,0.015f,0.3f);

    // Gold wrist rings
    glPushMatrix();
    glTranslatef(0,0,0.46f);
    glBindTexture(GL_TEXTURE_2D, texGold); glColor3f(1,1,1);
    GLUquadric* qR=gluNewQuadric(); gluQuadricTexture(qR,GL_TRUE);
    for(int r=0;r<3;r++){
        glPushMatrix(); glTranslatef(0,0,r*0.015f);
        glScalef(1.0f,0.85f,1.0f); gluCylinder(qR,0.075f,0.075f,0.010f,30,1);
        glPopMatrix();
    }
    gluDeleteQuadric(qR);
    glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, texSkin);

    // Wrist joint
    glTranslatef(0,0,0.53f);
    glRotatef(isLeft?90.0f:-90.0f,0,0,1);

    // Dynamic wrist tilt when opening the fan
    if (isLeft && weapon2_status) {
        float tilt = (fanSpreadAngle / 140.0f) * 60.0f;
        glRotatef(tilt, 0, 0, 1);
    }

    // Hand palm
    float knuckleRx=0.06f, knuckleRy=0.025f, handLength=0.16f;
    glRotatef(isLeft?5:-5,0,1,0);
    glRotatef(-5,1,0,0);
    drawProceduralArmPart(handLength,wristRx,wristRy,knuckleRx,knuckleRy,30,20,0.01f,0.5f);

    glPushMatrix();
    glTranslatef(0,0,handLength);
    glScalef(1.0f,knuckleRy/knuckleRx,0.35f);
    gluSphere(quad,knuckleRx,30,30);
    glPopMatrix();

    // Thumb
    glPushMatrix();
    glTranslatef(isLeft?knuckleRx*0.85f:-knuckleRx*0.85f,-knuckleRy*0.8f,handLength*0.35f);
    glRotatef(isLeft?35.0f:-35.0f,0,1,0);
    glRotatef(10.0f,1,0,0);
    glRotatef(isLeft?(-fistProgress*65.0f):(fistProgress*65.0f),0,1,0);
    glRotatef(fistProgress*15.0f,1,0,0);
    glPushMatrix(); gluSphere(quad,0.016f,16,16); glPopMatrix();
    drawProceduralArmPart(0.065f,0.016f,0.016f,0.014f,0.014f,16,16,0.003f,0.5f);
    glTranslatef(0,0,0.065f); gluSphere(quad,0.014f,16,16);
    glRotatef(isLeft?(-fistProgress*60.0f):(fistProgress*60.0f),0,1,0);
    glRotatef(fistProgress*5.0f,1,0,0);
    drawProceduralArmPart(0.05f,0.014f,0.014f,0.011f,0.011f,16,16,0.002f,0.5f);
    glTranslatef(0,0,0.05f); gluSphere(quad,0.011f,16,16);
    glPopMatrix();

    // Fingers
    glTranslatef(0,0,handLength);
    float fingerSpace=0.028f;
    float fingerPos[]   ={1.5f*fingerSpace,0.5f*fingerSpace,-0.5f*fingerSpace,-1.5f*fingerSpace};
    float fingerLen[]   ={0.075f,0.085f,0.08f,0.06f};
    float fingerAng[]   ={6.0f,0.0f,-4.0f,-10.0f};
    float fingerZOff[]  ={0.007f,0.016f,0.01f,0.0f};
    for(int i=0;i<4;i++){
        glPushMatrix();
        glTranslatef(isLeft?fingerPos[i]:-fingerPos[i],0,fingerZOff[i]);
        float rx=0.014f,ry=0.012f;
        glPushMatrix(); gluSphere(quad,rx,16,16); glPopMatrix();
        glRotatef(isLeft?fingerAng[i]:-fingerAng[i],0,1,0);

        // Standard tight fist curling
        float j1Base = 12.0f;
        float j1Fist = 80.0f;
        glRotatef(j1Base + fistProgress*j1Fist, 1,0,0);
        drawProceduralArmPart(fingerLen[i],rx,ry,rx*0.9f,ry*0.9f,16,16,0.003f,0.5f);
        glTranslatef(0,0,fingerLen[i]); gluSphere(quad,rx*0.9f,16,16);

        float j2Base = 25.0f;
        float j2Fist = 75.0f;
        glRotatef(j2Base + fistProgress*j2Fist, 1,0,0);
        drawProceduralArmPart(fingerLen[i]*0.7f,rx*0.9f,ry*0.9f,rx*0.75f,ry*0.75f,16,16,0.002f,0.5f);
        glTranslatef(0,0,fingerLen[i]*0.7f); gluSphere(quad,rx*0.75f,16,16);

        float j3Base = 20.0f;
        float j3Fist = 70.0f;
        glRotatef(j3Base + fistProgress*j3Fist, 1,0,0);
        drawProceduralArmPart(fingerLen[i]*0.5f,rx*0.75f,ry*0.75f,rx*0.6f,ry*0.6f,16,16,0.001f,0.5f);
        glTranslatef(0,0,fingerLen[i]*0.5f); gluSphere(quad,rx*0.6f,16,16);
        glPopMatrix();
    }

    // ---- Weapon 1: Meteor Hammer ----
    // Held on LEFT arm (appears on screen RIGHT when character faces camera)
    if (isLeft && weapon1_status) {
        glPushMatrix();

        // NOTE: We are currently at the KNUCKLE origin (after glTranslatef(0,0,handLength)).
        // To reach MID-PALM (wrist z=0.08), we go BACK by -0.08 in wrist Z
        // (-0.08 from knuckle = wrist + 0.08 = mid palm position, inside the fist)
        // Y = -0.04 shifts handle toward palm face where fingers curl (grip sweet spot)
        glTranslatef(0.0f, -0.032f, -0.032f);

        // Rotate -90deg around wrist Z so weapon +Y -> wrist +X -> world forward (+Z)
        // For left arm: wrist +X = world forward => ball faces forward toward camera
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);

        drawMeteorHammer();
        glPopMatrix();
    }

    // ---- Weapon 2: Fan (starts folded) ----
    // Held on LEFT arm (appears on screen RIGHT when character faces camera)
    if (isLeft && weapon2_status) {
        glPushMatrix();
        // Counter-rotate the weapon so it doesn't follow the hand tilt
        float tilt = (fanSpreadAngle / 140.0f) * 60.0f;
        glRotatef(-tilt, 0, 0, 1);

        // Translating to palm SWEET SPOT, just like Weapon 1
        glTranslatef(0.0f, -0.032f, -0.032f);
        
        // Rotate -90deg around Z so Fan points forward like Meteor Hammer
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
        
        drawFan(fanSpreadAngle);
        glPopMatrix();
    }

    gluDeleteQuadric(quad);
    glPopMatrix();
}

// Arms are placed left/right of the upper body (at shoulder height).
// The body mesh top (shoulders) sits at world Y ≈ charY + 1.35.
// We offset X by ±0.50 from centre so they clear the wide torso.
void drawLeftArm()
{
    glPushMatrix();
    glTranslatef(-0.58f, 1.24f, 0.04f); // Set origin to ball joint center
    
    // 1. Draw Static shoulder part (connector) — not affected by arm rotation
    glPushMatrix();
    glRotatef(90, 1, 0, 0); 
    drawProceduralArmBase(true, leftFistProgress, 0); // part 0 = static
    glPopMatrix();

    // 2. Apply arm rotation and draw rotating part (now centered on ball joint)
    glRotatef(-(leftArmAngle - walkArmSwing), 1, 0, 0); 
    glRotatef(90, 1, 0, 0);
    drawProceduralArmBase(true, leftFistProgress, 1); // part 1 = rotating
    glPopMatrix();
}

void drawRightArm()
{
    glPushMatrix();
    glTranslatef(0.58f, 1.24f, 0.04f); // Set origin to ball joint center

    // 1. Draw Static shoulder part (connector) — not affected by arm rotation
    glPushMatrix();
    glRotatef(90, 1, 0, 0);
    drawProceduralArmBase(false, rightFistProgress, 0); // part 0 = static
    glPopMatrix();

    // 2. Apply arm rotation and draw rotating part (now centered on ball joint)
    glRotatef(-(rightArmAngle + walkArmSwing), 1, 0, 0);
    glRotatef(90, 1, 0, 0);
    drawProceduralArmBase(false, rightFistProgress, 1); // part 1 = rotating
    glPopMatrix();
}

//================================================================
//  BODY  (from body.cpp) — procedural torso + dress mesh
//================================================================

// Cubic Catmull-Rom interpolation
float spline(float t, float p0, float p1, float p2, float p3) {
    float t2=t*t, t3=t2*t;
    return 0.5f*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t2+(-p0+3*p1-3*p2+p3)*t3);
}

struct CrossSection { float z,rx,ry,yOffset; };
#define SEC_COUNT 10
CrossSection sections[SEC_COUNT] = {
    {-0.2f, 0.42f,0.26f, 0.0f},
    { 0.0f, 0.45f,0.28f, 0.0f},
    { 0.3f, 0.47f,0.32f,-0.06f},
    { 0.6f, 0.31f,0.21f, 0.02f},
    { 0.9f, 0.34f,0.24f, 0.08f},
    { 1.15f,0.40f,0.26f, 0.10f},
    { 1.35f,0.46f,0.21f, 0.03f},
    { 1.42f,0.22f,0.18f, 0.00f},
    { 1.58f,0.13f,0.13f,-0.04f},
    { 1.80f,0.13f,0.13f,-0.04f}
};

void getTorsoBaseRadius(float z,float&rx,float&ry,float&yOfs) {
    if(z<=sections[1].z){rx=sections[1].rx;ry=sections[1].ry;yOfs=sections[1].yOffset;return;}
    if(z>=sections[SEC_COUNT-2].z){rx=sections[SEC_COUNT-2].rx;ry=sections[SEC_COUNT-2].ry;yOfs=sections[SEC_COUNT-2].yOffset;return;}
    int i=1;
    for(;i<SEC_COUNT-2;i++) if(z<=sections[i+1].z) break;
    float t=(z-sections[i].z)/(sections[i+1].z-sections[i].z);
    rx=spline(t,sections[i-1].rx,sections[i].rx,sections[i+1].rx,sections[i+2].rx);
    ry=spline(t,sections[i-1].ry,sections[i].ry,sections[i+1].ry,sections[i+2].ry);
    yOfs=spline(t,sections[i-1].yOffset,sections[i].yOffset,sections[i+1].yOffset,sections[i+2].yOffset);
}

void getTorsoVertex(float z,float theta,float&px,float&py,float&pz) {
    float rx,ry,yOfs; getTorsoBaseRadius(z,rx,ry,yOfs);
    float bump=0;
    float maxBump=0.12f,spreadZ=0.02f,spreadTheta=0.12f;
    while(theta<0)theta+=2*3.14159f; while(theta>2*3.14159f)theta-=2*3.14159f;
    float breastZ=1.1f,tL=3.14159f*0.35f,tR=3.14159f*0.65f;
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(theta-tL,2)/spreadTheta));
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(theta-tR,2)/spreadTheta));
    bump-=0.06f*exp(-(pow(z-breastZ,2)/0.03f+pow(theta-3.14159f*0.5f,2)/0.01f));
    float gZ=0.25f,gL=3.14159f*1.35f,gR=3.14159f*1.65f;
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(theta-gL,2)/0.15f));
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(theta-gR,2)/0.15f));
    bump+=0.03f*exp(-(pow(z-0.55f,2)/0.04f+pow(theta-3.14159f*0.5f,2)/0.2f));
    rx+=bump; ry+=bump;
    px=rx*cos(theta); py=ry*sin(theta)+yOfs; pz=z;
}

void getTorsoNormal(float z,float theta,float&nx,float&ny,float&nz) {
    float eZ=0.01f,eT=0.05f;
    float p1[3],p2[3],p3[3],p4[3];
    getTorsoVertex(z+eZ,theta,p1[0],p1[1],p1[2]);
    getTorsoVertex(z-eZ,theta,p2[0],p2[1],p2[2]);
    getTorsoVertex(z,theta+eT,p3[0],p3[1],p3[2]);
    getTorsoVertex(z,theta-eT,p4[0],p4[1],p4[2]);
    float tx1=p1[0]-p2[0],ty1=p1[1]-p2[1],tz1=p1[2]-p2[2];
    float tx2=p3[0]-p4[0],ty2=p3[1]-p4[1],tz2=p3[2]-p4[2];
    nx=ty2*tz1-tz2*ty1; ny=tz2*tx1-tx2*tz1; nz=tx2*ty1-ty2*tx1;
    float l=sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
}

void getDressTorsoVertex(float z,float theta,float offset,float&px,float&py,float&pz) {
    float rx,ry,yOfs; getTorsoBaseRadius(z,rx,ry,yOfs);
    float bump=0,maxBump=0.12f,spreadZ=0.02f,spreadTheta=0.12f;
    float nt=theta; while(nt<0)nt+=2*3.14159f; while(nt>2*3.14159f)nt-=2*3.14159f;
    float breastZ=1.1f,tL=3.14159f*0.35f,tR=3.14159f*0.65f;
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(nt-tL,2)/spreadTheta));
    bump+=maxBump*exp(-(pow(z-breastZ,2)/spreadZ+pow(nt-tR,2)/spreadTheta));
    bump-=0.06f*exp(-(pow(z-breastZ,2)/0.03f+pow(nt-3.14159f*0.5f,2)/0.01f));
    float gZ=0.25f,gL=3.14159f*1.35f,gR=3.14159f*1.65f;
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(nt-gL,2)/0.15f));
    bump+=0.005f*exp(-(pow(z-gZ,2)/0.03f+pow(nt-gR,2)/0.15f));
    bump+=0.03f*exp(-(pow(z-0.55f,2)/0.04f+pow(nt-3.14159f*0.5f,2)/0.2f));
    rx+=bump+offset; ry+=bump+offset;
    px=rx*cos(theta); py=ry*sin(theta)+yOfs; pz=z;
}

void getDressTorsoNormal(float z,float theta,float offset,float&nx,float&ny,float&nz) {
    float eZ=0.01f,eT=0.05f;
    float p1[3],p2[3],p3[3],p4[3];
    getDressTorsoVertex(z+eZ,theta,offset,p1[0],p1[1],p1[2]);
    getDressTorsoVertex(z-eZ,theta,offset,p2[0],p2[1],p2[2]);
    getDressTorsoVertex(z,theta+eT,offset,p3[0],p3[1],p3[2]);
    getDressTorsoVertex(z,theta-eT,offset,p4[0],p4[1],p4[2]);
    float tx1=p1[0]-p2[0],ty1=p1[1]-p2[1],tz1=p1[2]-p2[2];
    float tx2=p3[0]-p4[0],ty2=p3[1]-p4[1],tz2=p3[2]-p4[2];
    nx=ty2*tz1-tz2*ty1; ny=tz2*tx1-tx2*tz1; nz=tx2*ty1-ty2*tx1;
    float l=sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
}

void drawDressMesh()
{
    glDisable(GL_TEXTURE_2D);
    int slices=60;

    // 1. White top bodice
    int topStacks=15;
    for(int i=0;i<topStacks;i++){
        float f1=(float)i/topStacks, f2=(float)(i+1)/topStacks;
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float corsetTop=0.95f-0.20f*exp(-pow(dipTh,2)/0.15f);
            float topZ1=corsetTop-0.05f;
            float zTopMax=1.05f+0.18f*exp(-pow(fabs(dipTh)-0.5f,2)/0.08f)
                              +0.05f*exp(-pow(dipTh,2)/0.05f)
                              +0.10f*exp(-pow(fabs(dipTh)-3.14159f,2)/1.0f);
            if(topZ1>zTopMax-0.02f)topZ1=zTopMax-0.02f;
            float z1=topZ1+f1*(zTopMax-topZ1), z2=topZ1+f2*(zTopMax-topZ1);
            float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
            getDressTorsoVertex(z2,theta,0.015f,px2,py2,pz2);
            getDressTorsoNormal(z2,theta,0.015f,nx2,ny2,nz2);
            getDressTorsoVertex(z1,theta,0.015f,px1,py1,pz1);
            getDressTorsoNormal(z1,theta,0.015f,nx1,ny1,nz1);
            float shade=0.95f+0.1f*cos(theta); if(shade>1)shade=1;
            auto applyGrad=[&](float f){
                float t=f*1.5f; if(t>1)t=1;
                glColor3f((0.40f+t*0.60f)*shade,(0.65f+t*0.35f)*shade,shade);
            };
            applyGrad(f2); glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
            applyGrad(f1); glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    }

    // 2. Dark blue corset
    int corsetStacks=12;
    for(int i=0;i<corsetStacks;i++){
        float f1=(float)i/corsetStacks,f2=(float)(i+1)/corsetStacks;
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float corsetTop=0.95f-0.20f*exp(-pow(dipTh,2)/0.15f);
            float corsetBot=0.52f+0.12f*exp(-pow(dipTh,2)/0.15f);
            float z1=corsetBot+f1*(corsetTop-corsetBot),z2=corsetBot+f2*(corsetTop-corsetBot);
            float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
            getDressTorsoVertex(z2,theta,0.025f,px2,py2,pz2);
            getDressTorsoNormal(z2,theta,0.025f,nx2,ny2,nz2);
            getDressTorsoVertex(z1,theta,0.025f,px1,py1,pz1);
            getDressTorsoNormal(z1,theta,0.025f,nx1,ny1,nz1);
            float dS=0.5f+0.5f*f1, cS=0.85f+0.15f*cos(theta*2),shade=dS*cS;
            glColor3f(0.04f*shade,0.15f*shade,0.45f*shade);
            glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
            glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    }

    // 3. Gold irregular belts
    auto drawIrregularBelt=[&](bool isTop){
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float borderZ=isTop?(0.95f-0.20f*exp(-pow(dipTh,2)/0.15f))
                               :(0.52f+0.12f*exp(-pow(dipTh,2)/0.15f));
            float z1=borderZ-0.015f,z2=borderZ+0.015f;
            float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
            getDressTorsoVertex(z2,theta,0.035f,px2,py2,pz2);
            getDressTorsoNormal(z2,theta,0.035f,nx2,ny2,nz2);
            getDressTorsoVertex(z1,theta,0.035f,px1,py1,pz1);
            getDressTorsoNormal(z1,theta,0.035f,nx1,ny1,nz1);
            float shade=0.85f+0.15f*cos(theta);
            glColor3f(0.9f*shade,0.75f*shade,0.2f*shade);
            glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
            glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    };
    drawIrregularBelt(false); drawIrregularBelt(true);

    // 3b. Centre emblem
    glColor3f(0.9f,0.75f,0.2f);
    float cZ=0.72f,cTheta=3.14159f*0.5f;
    float pTop[3],pBot[3],pL[3],pR[3],pC[3];
    getDressTorsoVertex(0.88f,cTheta,0.04f,pTop[0],pTop[1],pTop[2]);
    getDressTorsoVertex(0.53f,cTheta,0.04f,pBot[0],pBot[1],pBot[2]);
    getDressTorsoVertex(cZ,cTheta+0.10f,0.04f,pL[0],pL[1],pL[2]);
    getDressTorsoVertex(cZ,cTheta-0.10f,0.04f,pR[0],pR[1],pR[2]);
    getDressTorsoVertex(cZ,cTheta,0.08f,pC[0],pC[1],pC[2]);
    glBegin(GL_TRIANGLES);
    auto drawFacet=[&](float*p1,float*p2,float*p3){
        float u[3]={p2[0]-p1[0],p2[1]-p1[1],p2[2]-p1[2]};
        float v[3]={p3[0]-p1[0],p3[1]-p1[1],p3[2]-p1[2]};
        float nx=u[1]*v[2]-u[2]*v[1],ny=u[2]*v[0]-u[0]*v[2],nz=u[0]*v[1]-u[1]*v[0];
        float l=sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
        glNormal3f(nx,ny,nz); glVertex3fv(p1); glVertex3fv(p2); glVertex3fv(p3);
    };
    drawFacet(pTop,pR,pC); drawFacet(pTop,pC,pL);
    drawFacet(pBot,pC,pR); drawFacet(pBot,pL,pC);
    glEnd();
    float pTB[3],pBB[3],pLB[3],pRB[3];
    getDressTorsoVertex(0.88f,cTheta,0.025f,pTB[0],pTB[1],pTB[2]);
    getDressTorsoVertex(0.53f,cTheta,0.025f,pBB[0],pBB[1],pBB[2]);
    getDressTorsoVertex(cZ,cTheta+0.10f,0.025f,pLB[0],pLB[1],pLB[2]);
    getDressTorsoVertex(cZ,cTheta-0.10f,0.025f,pRB[0],pRB[1],pRB[2]);
    glBegin(GL_QUADS);
    glVertex3fv(pTop);glVertex3fv(pR);glVertex3fv(pRB);glVertex3fv(pTB);
    glVertex3fv(pR);glVertex3fv(pBot);glVertex3fv(pBB);glVertex3fv(pRB);
    glVertex3fv(pBot);glVertex3fv(pL);glVertex3fv(pLB);glVertex3fv(pBB);
    glVertex3fv(pL);glVertex3fv(pTop);glVertex3fv(pTB);glVertex3fv(pLB);
    glEnd();

    // Filigree curves
    auto drawThickCurve=[&](float sZ,float sT,float eZ,float eT,float cZ2,float cT,float thick){
        glBegin(GL_QUAD_STRIP);
        int steps=15;
        for(int i=0;i<=steps;i++){
            float t=(float)i/steps,mt=1-t;
            float bz=mt*mt*sZ+2*mt*t*cZ2+t*t*eZ;
            float bt=mt*mt*sT+2*mt*t*cT+t*t*eT;
            float dz=2*mt*(cZ2-sZ)+2*t*(eZ-cZ2);
            float dt2=2*mt*(cT-sT)+2*t*(eT-cT);
            float l=sqrt(dz*dz+dt2*dt2); if(l<0.0001f)l=1;
            float pZ=-dt2/l*thick,pT=dz/l*thick;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(bz+pZ,cTheta+bt+pT,0.04f,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(bz-pZ,cTheta+bt-pT,0.04f,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(bz,cTheta+bt,0.04f,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p1); glVertex3fv(p2);
        }
        glEnd();
    };
    int sides[]={-1,1};
    for(int i=0;i<2;i++){
        float dir=(float)sides[i];
        drawThickCurve(0.57f,0.03f*dir,0.68f,0.35f*dir,0.50f,0.25f*dir,0.012f);
        drawThickCurve(0.68f,0.35f*dir,0.75f,0.18f*dir,0.85f,0.35f*dir,0.012f);
        drawThickCurve(0.75f,0.18f*dir,0.69f,0.25f*dir,0.65f,0.15f*dir,0.010f);
        drawThickCurve(0.68f,0.35f*dir,0.93f,0.60f*dir,0.80f,0.45f*dir,0.010f);
        drawThickCurve(0.62f,0.25f*dir,0.53f,0.60f*dir,0.56f,0.35f*dir,0.010f);
        if(sides[i]==-1){
            drawThickCurve(0.55f,-0.35f,0.45f,-0.28f,0.45f,-0.35f,0.008f);
            drawThickCurve(0.45f,-0.28f,0.58f,-0.25f,0.55f,-0.25f,0.008f);
        }
    }

    // 4. High collar
    glColor3f(0.04f,0.15f,0.45f);
    glBegin(GL_QUAD_STRIP);
    for(int j=0;j<=slices;j++){
        float theta=(float)j/slices*2*3.14159f;
        float dipTh=theta-3.14159f*0.5f;
        if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
        float zBot=1.40f;
        if(fabs(dipTh)<0.25f)zBot-=0.07f*(0.25f-fabs(dipTh))/0.25f;
        float px1,py1,pz1,nx1,ny1,nz1,px2,py2,pz2,nx2,ny2,nz2;
        getDressTorsoVertex(1.46f,theta,0.015f,px2,py2,pz2);
        getDressTorsoNormal(1.46f,theta,0.015f,nx2,ny2,nz2);
        getDressTorsoVertex(zBot,theta,0.015f,px1,py1,pz1);
        getDressTorsoNormal(zBot,theta,0.015f,nx1,ny1,nz1);
        glNormal3f(nx2,ny2,nz2); glVertex3f(px2,py2,pz2);
        glNormal3f(nx1,ny1,nz1); glVertex3f(px1,py1,pz1);
    }
    glEnd();

    auto drawTopCurve=[&](float sZ,float sT,float eZ,float eT,float cZ2,float cT,float thick,float offset){
        glBegin(GL_QUAD_STRIP);
        int steps=15;
        for(int i=0;i<=steps;i++){
            float t=(float)i/steps,mt=1-t;
            float bz=mt*mt*sZ+2*mt*t*cZ2+t*t*eZ;
            float bt=mt*mt*sT+2*mt*t*cT+t*t*eT;
            float dz=2*mt*(cZ2-sZ)+2*t*(eZ-cZ2);
            float dt2=2*mt*(cT-sT)+2*t*(eT-cT);
            float l=sqrt(dz*dz+dt2*dt2); if(l<0.0001f)l=1;
            float pZ=-dt2/l*thick,pT=dz/l*thick;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(bz+pZ,cTheta+bt+pT,offset,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(bz-pZ,cTheta+bt-pT,offset,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(bz,cTheta+bt,offset,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p1); glVertex3fv(p2);
        }
        glEnd();
    };

    // Dark blue straps
    glColor3f(0.04f,0.15f,0.45f);
    for(int i=0;i<2;i++){
        float dir=(float)sides[i];
        drawTopCurve(1.23f+0.03f,0.08f*dir,1.15f,0.65f*dir,1.18f,0.40f*dir,0.06f,0.02f);
    }

    // Collar rings
    glColor3f(0.85f,0.7f,0.15f);
    auto drawCollarRings=[&](){
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float dipTh=theta-3.14159f*0.5f;
            if(dipTh<-3.14159f)dipTh+=2*3.14159f; if(dipTh>3.14159f)dipTh-=2*3.14159f;
            float zC=1.40f;
            if(fabs(dipTh)<0.25f)zC-=0.07f*(0.25f-fabs(dipTh))/0.25f;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(zC+0.008f,theta,0.02f,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(zC-0.008f,theta,0.02f,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(zC,theta,0.02f,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p2); glVertex3fv(p1);
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f;
            float p1[3],p2[3],n[3];
            getDressTorsoVertex(1.46f+0.008f,theta,0.02f,p1[0],p1[1],p1[2]);
            getDressTorsoVertex(1.46f-0.008f,theta,0.02f,p2[0],p2[1],p2[2]);
            getDressTorsoNormal(1.46f,theta,0.02f,n[0],n[1],n[2]);
            glNormal3fv(n); glVertex3fv(p2); glVertex3fv(p1);
        }
        glEnd();
    };
    drawCollarRings();

    // Chest diamond  + straps gold trim
    for(int i=0;i<2;i++){
        float dir=(float)sides[i];
        drawTopCurve(1.33f,0.0f,1.25f,0.15f*dir,1.29f,0.08f*dir,0.012f,0.025f);
        drawTopCurve(1.25f,0.15f*dir,1.15f,0.0f,1.20f,0.08f*dir,0.012f,0.025f);
        drawTopCurve(1.23f+0.03f,0.08f*dir+0.04f*dir,1.15f,0.65f*dir+0.04f*dir,
                     1.18f,0.40f*dir+0.04f*dir,0.01f,0.021f);
    }

    // Blue gems
    glColor3f(0.3f,0.5f,0.95f);
    float g1Z=1.27f,g1Th=0.04f,g1H=0.04f;
    glBegin(GL_POLYGON);
    float gn[3]; getDressTorsoNormal(g1Z,cTheta,0.035f,gn[0],gn[1],gn[2]);
    float g1T[3],g1B[3],g1L[3],g1R[3];
    getDressTorsoVertex(g1Z+g1H,cTheta,0.04f,g1T[0],g1T[1],g1T[2]);
    getDressTorsoVertex(g1Z-g1H,cTheta,0.04f,g1B[0],g1B[1],g1B[2]);
    getDressTorsoVertex(g1Z,cTheta+g1Th,0.04f,g1L[0],g1L[1],g1L[2]);
    getDressTorsoVertex(g1Z,cTheta-g1Th,0.04f,g1R[0],g1R[1],g1R[2]);
    glNormal3fv(gn); glVertex3fv(g1T); glVertex3fv(g1R); glVertex3fv(g1B); glVertex3fv(g1L);
    glEnd();
    float g2Z=1.19f,g2Th=0.025f,g2H=0.025f;
    glBegin(GL_POLYGON);
    float g2T[3],g2B[3],g2L[3],g2R[3];
    getDressTorsoVertex(g2Z+g2H,cTheta,0.04f,g2T[0],g2T[1],g2T[2]);
    getDressTorsoVertex(g2Z-g2H,cTheta,0.04f,g2B[0],g2B[1],g2B[2]);
    getDressTorsoVertex(g2Z,cTheta+g2Th,0.04f,g2L[0],g2L[1],g2L[2]);
    getDressTorsoVertex(g2Z,cTheta-g2Th,0.04f,g2R[0],g2R[1],g2R[2]);
    glNormal3fv(gn); glVertex3fv(g2T); glVertex3fv(g2R); glVertex3fv(g2B); glVertex3fv(g2L);
    glEnd();

    // 5. Skirt layers
    struct Layer{float zMin,flareMax,color[3];int folds;float foldDepth;};
    Layer layers[4]={
        {-1.0f,0.28f,{0.1f,0.3f,0.8f},8,0.15f},
        {-0.7f,0.22f,{0.6f,0.75f,0.95f},7,0.12f},
        {-0.4f,0.15f,{1.0f,1.0f,1.0f},6,0.08f},
        {-0.1f,0.08f,{0.05f,0.15f,0.35f},5,0.04f}
    };
    for(int l=0;l<4;l++){
        glColor3fv(layers[l].color);
        int skirtStacks=20;
        float zSkirtMax=0.55f,zSkirtMin=layers[l].zMin,flare=layers[l].flareMax;
        for(int i=0;i<skirtStacks;i++){
            float f1=(float)i/skirtStacks,f2=(float)(i+1)/skirtStacks;
            glBegin(GL_QUAD_STRIP);
            for(int j=0;j<=slices;j++){
                float theta=(float)j/slices*2*3.14159f;
                float frontDist=sin(theta);
                float lm=0.8f-frontDist*0.2f;
                if(l==3){float cy=theta/(2*3.14159f)*6,fr=cy-floor(cy),tw=fabs(fr*2-1);lm=0.6f+0.35f*tw;}
                float actZMin=zSkirtMax-(zSkirtMax-zSkirtMin)*lm;
                if(actZMin>zSkirtMax)actZMin=zSkirtMax;
                float actZ1=zSkirtMax-f1*(zSkirtMax-actZMin);
                float actZ2=zSkirtMax-f2*(zSkirtMax-actZMin);
                float fl1=f1*f1*flare,fl2=f2*f2*flare;
                float fo1=sin(theta*layers[l].folds)*layers[l].foldDepth*f1;
                float fo2=sin(theta*layers[l].folds)*layers[l].foldDepth*f2;
                float bx1,by1,bz1,bx2,by2,bz2;
                getDressTorsoVertex(actZ1,theta,0.025f+l*0.003f,bx1,by1,bz1);
                getDressTorsoVertex(actZ2,theta,0.025f+l*0.003f,bx2,by2,bz2);
                float px1=bx1+cos(theta)*(fl1+fo1),py1=by1+sin(theta)*(fl1+fo1);
                float px2=bx2+cos(theta)*(fl2+fo2),py2=by2+sin(theta)*(fl2+fo2);
                float vx=px2-px1,vy=py2-py1,vz=actZ2-actZ1;
                float tx=-sin(theta),ty=cos(theta);
                float nx=ty*vz-0*vy,ny=0*vx-tx*vz,nz2=tx*vy-ty*vx;
                float nl=sqrt(nx*nx+ny*ny+nz2*nz2);if(nl>0.001f){nx/=nl;ny/=nl;nz2/=nl;}
                glNormal3f(nx,ny,nz2); glVertex3f(px2,py2,actZ2);
                glNormal3f(nx,ny,nz2); glVertex3f(px1,py1,actZ1);
            }
            glEnd();
        }
    }

    // 6. Golden skirt accents
    glColor3f(0.85f,0.7f,0.15f);
    for(int i=0;i<6;i++){
        float theta=i/6.0f*2*3.14159f;
        float zSkirtMax=0.55f,zSkirtMin=layers[3].zMin;
        float actZMin=zSkirtMax-(zSkirtMax-zSkirtMin)*0.95f;
        float bx,by,bz2;
        getDressTorsoVertex(actZMin,theta,0.025f+3*0.003f+0.015f,bx,by,bz2);
        float fv=layers[3].flareMax,fov=sin(theta*layers[3].folds)*layers[3].foldDepth;
        float cx=bx+cos(theta)*(fv+fov),cy2=by+sin(theta)*(fv+fov),cz=actZMin;
        float nx2=cos(theta),ny2=sin(theta),nz3=0.3f;
        float nl=sqrt(nx2*nx2+ny2*ny2+nz3*nz3);nx2/=nl;ny2/=nl;nz3/=nl;
        float tx=-sin(theta),ty=cos(theta);
        float w=0.035f,len=0.18f,slZ=0.05f;
        glBegin(GL_POLYGON);
        glNormal3f(nx2,ny2,nz3);
        glVertex3f(cx-tx*w,cy2-ty*w,cz);
        glVertex3f(cx+tx*w,cy2+ty*w,cz-slZ);
        glVertex3f(cx+tx*w,cy2+ty*w,cz-len);
        glVertex3f(cx-tx*w,cy2-ty*w,cz-len+slZ);
        glEnd();
        glColor3f(1.0f,0.9f,0.4f);
        glBegin(GL_POLYGON);
        glNormal3f(nx2,ny2,nz3);
        float iC=cx+nx2*0.005f,iCY=cy2+ny2*0.005f;
        glVertex3f(iC,iCY,cz-len*0.2f);
        glVertex3f(iC+tx*w*0.5f,iCY+ty*w*0.5f,cz-len*0.4f);
        glVertex3f(iC,iCY,cz-len*0.6f);
        glVertex3f(iC-tx*w*0.5f,iCY-ty*w*0.5f,cz-len*0.4f);
        glEnd();
        glColor3f(0.85f,0.7f,0.15f);
    }

    glEnable(GL_TEXTURE_2D);
}

void drawBodyMesh()
{
    int stacks=60,slices=60;
    float zMin=sections[1].z, zMax=sections[SEC_COUNT-2].z;
    glColor3f(1,1,1); glBindTexture(GL_TEXTURE_2D, texSkin);
    for(int i=0;i<stacks;i++){
        float f1=(float)i/stacks,f2=(float)(i+1)/stacks;
        float z1=zMin+f1*(zMax-zMin),z2=zMin+f2*(zMax-zMin);
        glBegin(GL_QUAD_STRIP);
        for(int j=0;j<=slices;j++){
            float theta=(float)j/slices*2*3.14159f,s=(float)j/slices*2;
            float px2,py2,pz2,nx2,ny2,nz2;
            getTorsoVertex(z2,theta,px2,py2,pz2); getTorsoNormal(z2,theta,nx2,ny2,nz2);
            glNormal3f(nx2,ny2,nz2); glTexCoord2f(s,f2*2); glVertex3f(px2,py2,pz2);
            float px1,py1,pz1,nx1,ny1,nz1;
            getTorsoVertex(z1,theta,px1,py1,pz1); getTorsoNormal(z1,theta,nx1,ny1,nz1);
            glNormal3f(nx1,ny1,nz1); glTexCoord2f(s,f1*2); glVertex3f(px1,py1,pz1);
        }
        glEnd();
    }
    // Neck cap
    glBegin(GL_POLYGON);
    glNormal3f(0,0,1);
    for(int j=0;j<=slices;j++){
        float theta=(float)j/slices*2*3.14159f;
        float px,py,pz; getTorsoVertex(zMax,theta,px,py,pz);
        glTexCoord2f((px/0.15f+1)/2,(py/0.15f+1)/2); glVertex3f(px,py,pz);
    }
    glEnd();
    // Pelvis cap
    glBegin(GL_POLYGON);
    glNormal3f(0,0,-1);
    for(int j=slices;j>=0;j--){
        float theta=(float)j/slices*2*3.14159f;
        float px,py,pz; getTorsoVertex(zMin,theta,px,py,pz);
        glTexCoord2f((px/0.45f+1)/2,(py/0.28f+1)/2); glVertex3f(px,py,pz);
    }
    glEnd();
}

//================================================================
//  CHARACTER assembly  (body uses Z-up so we rotate -90° on X)
//================================================================

// charY is the world-Y level of the character's hip joint.
// Leg total length ≈ 1.25 units (from drawLegBase, origin Y=1.53 to foot).
// We lift the entire character group so feet rest on Y = 0.
#define CHAR_Y 1.92f

void drawCharacter()
{
    glPushMatrix();
    glTranslatef(charX, CHAR_Y, charZ);  // Move character in world
    glRotatef(charRotation, 0.0f, 1.0f, 0.0f); // Rotate character

    // ---- Body (Z-up model, rotate so Z becomes world Y) ----
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f); // Reverse body direction to face camera
    if (cachedBodyList == 0) {
        cachedBodyList = glGenLists(1);
        glNewList(cachedBodyList, GL_COMPILE);
        drawBodyMesh();
        drawDressMesh();
        glEndList();
    }
    glCallList(cachedBodyList);
    glPopMatrix();

    // ---- Legs (Y-up, origin at hip Y=1.53, extend downward) ----
    // Hip is at body Z=0 → world Y=0 inside the group → subtract 1.53 to
    // shift the leg origin to hip level (world Y = CHAR_Y - 1.53... but
    // the legs' drawLeftLeg() already "assumes" their origin is at y=1.53
    // above the floor, so we just call them directly; the CHAR_Y lift handles it).
    glTranslatef(0.0f, -1.53f, 0.0f);   // re-origin to hip joint
    drawLeftLeg();
    drawRightLeg();

    // ---- Arms (Y-up, origin at shoulder; shoulder ≈ body Z=1.35 above hip) ----
    // Inside the group (after CHAR_Y lift), shoulder world-Y = 0 + 1.35.
    // drawLeftArm/drawRightArm() use glTranslatef(±0.50, 1.35, 0) internally.
    glTranslatef(0.0f, 1.53f, 0.0f);    // undo the leg offset
    drawLeftArm();
    drawRightArm();

    glPopMatrix();
}

//================================================================
//  DISPLAY
//================================================================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    updateCamera();
    gluLookAt(cameraX + charX, cameraY, cameraZ + charZ,
              charX, 1.2, charZ,   // look at character chest height
              0, 1, 0);

    drawBackground();
    drawCharacter();
}

//================================================================
//  WinMain
//================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpfnWndProc   = WindowProcedure;
    wc.lpszClassName = WINDOW_TITLE;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
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

    // Load all textures
    texGrass        = loadBMP("grass.bmp");
    texSkin         = loadBMP("skin.bmp");
    texFabric       = loadBMP("fabric.bmp");
    texGold         = loadBMP("gold.bmp");
    texWhiteSleeve  = loadBMP("white_sleeve.bmp");
    texWhiteLeather = loadBMP("white_leather.bmp");
    texDarkLeather  = loadBMP("dark_leather.bmp");
    texWeaponMetal  = loadBMP("metal.bmp");
    texWeaponWood   = loadBMP("wood.bmp");
    texWeaponChain  = loadBMP("chain.bmp");
    texFanWood      = loadBMP("fan_wood.bmp");
    texFan          = loadBMP("fan.bmp");

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