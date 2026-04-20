// night_background.cpp - Standalone viewer for the night background scene
// Run independently like weapon1.cpp / weapon2.cpp
// Controls:
//   Arrow Keys  - Orbit camera
//   + / -       - Zoom in/out
//   N           - Toggle Night / Day mode
//   Space       - Reset camera
//   Escape      - Quit

#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")

#define WINDOW_TITLE "Night Background Scene"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------------------------------------------
// Local state (no extern - fully standalone)
// -------------------------------------------------------
static bool  isNight   = true;          // Start in night mode
static float lightX    = 12.0f;
static float lightY    = 18.0f;
static float lightZ    = -30.0f;

static GLuint texGrass   = 0;
static GLuint texFanWood = 0;

static float cameraAngle    = 0.0f;
static float cameraHeight   = 5.0f;
static float cameraDistance = 30.0f;
static float cameraX, cameraY, cameraZ;

static bool isWireframe = false;

// -------------------------------------------------------
// Math helpers
// -------------------------------------------------------
static const float PI     = 3.1415926535f;
static const float TWO_PI = 6.283185307f;

static float hf(int s) {
    s = (s ^ 61) ^ (s >> 16); s += (s << 3); s ^= (s >> 4);
    s *= 0x27d4eb2d; s ^= (s >> 15);
    return (float)(s & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}
static float rr(int s, float lo, float hi) { return lo + hf(s) * (hi - lo); }

// -------------------------------------------------------
// Texture loader
// -------------------------------------------------------
static GLuint loadBMPTexture(const char* filename)
{
    GLuint texture = 0;
    FILE* file;
    if (fopen_s(&file, filename, "rb") != 0) return 0;

    unsigned char header[54];
    fread(header, 1, 54, file);

    int width     = *(int*)&header[18];
    int height    = *(int*)&header[22];
    int imageSize = *(int*)&header[34];
    if (imageSize == 0) imageSize = width * height * 3;

    unsigned char* data = new unsigned char[imageSize];
    fread(data, 1, imageSize, file);
    fclose(file);

    // BMP is BGR; swap to RGB
    for (int i = 0; i < imageSize; i += 3) {
        unsigned char tmp = data[i];
        data[i]     = data[i + 2];
        data[i + 2] = tmp;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return texture;
}

// -------------------------------------------------------
// Drawing: shared primitives
// -------------------------------------------------------
static void drawCube(float size)
{
    float h = size * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f( 0, 0, 1); glVertex3f(-h,-h, h); glVertex3f( h,-h, h); glVertex3f( h, h, h); glVertex3f(-h, h, h);
    glNormal3f( 0, 0,-1); glVertex3f(-h,-h,-h); glVertex3f(-h, h,-h); glVertex3f( h, h,-h); glVertex3f( h,-h,-h);
    glNormal3f( 0, 1, 0); glVertex3f(-h, h,-h); glVertex3f(-h, h, h); glVertex3f( h, h, h); glVertex3f( h, h,-h);
    glNormal3f( 0,-1, 0); glVertex3f(-h,-h,-h); glVertex3f( h,-h,-h); glVertex3f( h,-h, h); glVertex3f(-h,-h, h);
    glNormal3f(-1, 0, 0); glVertex3f(-h,-h,-h); glVertex3f(-h,-h, h); glVertex3f(-h, h, h); glVertex3f(-h, h,-h);
    glNormal3f( 1, 0, 0); glVertex3f( h,-h,-h); glVertex3f( h, h,-h); glVertex3f( h, h, h); glVertex3f( h,-h, h);
    glEnd();
}

// -------------------------------------------------------
// Drawing: background components
// -------------------------------------------------------
static void drawGround()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    if (isNight) glColor3f(0.6f, 0.6f, 0.7f);
    else         glColor3f(1.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f,  0.0f); glVertex3f(-150.0f, 0.0f, -150.0f);
    glTexCoord2f( 0.0f, 50.0f); glVertex3f(-150.0f, 0.0f,  150.0f);
    glTexCoord2f(50.0f, 50.0f); glVertex3f( 150.0f, 0.0f,  150.0f);
    glTexCoord2f(50.0f,  0.0f); glVertex3f( 150.0f, 0.0f, -150.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void drawCloud(float cx, float cy, float cz, float scale)
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

static void drawSun(float sx, float sy, float sz)
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

static void drawGiantMoon()
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int i = 0; i < 5; i++) {
        float r     = 45.0f + i * 4.0f;
        float alpha = 0.5f / (float)(i + 1);
        glColor4f(1.0f, 0.98f, 0.9f, alpha);
        glPushMatrix();
        glTranslatef(0.0f, -10.0f, -120.0f);
        gluDisk(q, 0.0f, r, 64, 1);
        glPopMatrix();
    }
    glDisable(GL_BLEND);
    gluDeleteQuadric(q);
    glEnable(GL_LIGHTING);
}

static void drawRiver()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUAD_STRIP);
    for (float x = -150.0f; x <= 150.0f; x += 5.0f) {
        float curveOffset = sinf(x * 0.08f) * 6.0f;
        glColor4f(0.05f, 0.15f, 0.3f, 0.85f);
        glVertex3f(x, 0.05f, -40.0f + curveOffset);
        glColor4f(0.1f, 0.3f, 0.5f, 0.85f);
        glVertex3f(x, 0.05f, -25.0f + curveOffset);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void drawDistantMountains()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.01f, 0.01f, 0.04f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-150.0f, 0.0f, -100.0f); glVertex3f(-80.0f, 15.0f, -100.0f); glVertex3f(-20.0f, 0.0f, -100.0f);
    glVertex3f( -50.0f, 0.0f, -105.0f); glVertex3f( 10.0f, 20.0f, -105.0f); glVertex3f( 60.0f, 0.0f, -105.0f);
    glVertex3f(  30.0f, 0.0f, -100.0f); glVertex3f( 90.0f, 12.0f, -100.0f); glVertex3f(150.0f, 0.0f, -100.0f);
    glEnd();
    glEnable(GL_LIGHTING);
}

static void drawPalaceRoof(float w, float h, float d)
{
    glColor3f(0.45f, 0.08f, 0.08f);
    float fx = w * 1.3f, fz = d * 1.3f;
    glBegin(GL_TRIANGLES);
    glVertex3f( 0.0f, h,  0.0f); glVertex3f(-fx, 0.0f,  fz); glVertex3f( fx, 0.0f,  fz);
    glVertex3f( 0.0f, h,  0.0f); glVertex3f( fx, 0.0f, -fz); glVertex3f(-fx, 0.0f, -fz);
    glVertex3f( 0.0f, h,  0.0f); glVertex3f(-fx, 0.0f, -fz); glVertex3f(-fx, 0.0f,  fz);
    glVertex3f( 0.0f, h,  0.0f); glVertex3f( fx, 0.0f,  fz); glVertex3f( fx, 0.0f, -fz);
    glEnd();
}

static void drawHighFidLantern(float x, float y, float z)
{
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(x, y, z);

    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texFanWood);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, 0.07f, 0.07f, 3.8f, 8, 1);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    glTranslatef(0.0f, 3.8f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.25f);
    glScalef(0.12f, 0.12f, 0.6f);
    drawCube(1.0f);
    glPopMatrix();

    glTranslatef(0.0f, -0.4f, 0.55f);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.85f, 0.3f);
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -0.3f);
    gluCylinder(q, 0.35f, 0.35f, 0.6f, 6, 1);
    glPopMatrix();

    glColor3f(0.6f, 0.1f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, -0.3f, 0.0f); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); gluDisk(q, 0.0f, 0.38f, 6, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f,  0.3f, 0.0f); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); gluDisk(q, 0.0f, 0.38f, 6, 1);
    glPopMatrix();

    glTranslatef(0.0f, 0.35f, 0.0f);
    drawPalaceRoof(0.35f, 0.25f, 0.35f);

    glColor3f(0.8f, 0.05f, 0.05f);
    float tw = 0.45f;
    float tx[] = {-tw, tw, tw, -tw}, tz[] = {tw, tw, -tw, -tw};
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(tx[i], -0.2f, tz[i]);
        glScalef(0.05f, 0.8f, 0.02f);
        drawCube(1.0f);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    gluDeleteQuadric(q);
    glPopMatrix();
}

static void drawStonePath()
{
    glDisable(GL_LIGHTING);
    for (float z = 20.0f; z > -150.0f; z -= 5.0f) {
        float shade    = 0.28f + (float)((int)fabsf(z) % 3) * 0.03f;
        float slabW    = 2.8f;
        float slabL    = 5.05f;
        float y        = 0.12f;
        glBegin(GL_QUADS);
        glColor3f(shade, shade, shade + 0.05f);
        glVertex3f(-slabW, y, z);
        glVertex3f( slabW, y, z);
        glVertex3f( slabW, y, z - slabL);
        glVertex3f(-slabW, y, z - slabL);
        glEnd();
        glColor3f(shade - 0.05f, shade - 0.05f, shade - 0.02f);
        glBegin(GL_LINES);
        glVertex3f(-slabW, y + 0.01f, z); glVertex3f(slabW, y + 0.01f, z);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    for (int i = 0; i < 12; i++) {
        float zBase = 15.0f - i * 8.5f;
        float xL    = -4.5f - rr(i * 31, 0.0f, 2.0f);
        float zL    = zBase + rr(i * 37, -3.0f, 3.0f);
        if (!(zL < -22.0f && zL > -44.0f)) {
            if (hf(i * 41) > 0.15f) drawHighFidLantern(xL, 0.0f, zL);
        }
        float xR = 4.5f + rr(i * 43, 0.0f, 2.0f);
        float zR = zBase + rr(i * 47, -3.0f, 3.0f);
        if (!(zR < -22.0f && zR > -44.0f)) {
            if (hf(i * 53) > 0.15f) drawHighFidLantern(xR, 0.0f, zR);
        }
    }
}

static void drawWaterDetails()
{
    GLUquadric* q = gluNewQuadric();
    glDisable(GL_LIGHTING);
    float lpx[] = {-15.0f, 12.0f, -22.0f, 18.0f, -8.0f, 25.0f, -45.0f, 30.0f};
    float lpz[] = {  5.0f,-12.0f,  28.0f, -5.0f, 35.0f, 12.0f,  10.0f, 42.0f};
    for (int i = 0; i < 8; i++) {
        glPushMatrix();
        glTranslatef(lpx[i], 0.04f, lpz[i]);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.02f, 0.1f, 0.02f);
        gluDisk(q, 0.0f, 1.5f, 16, 1);
        glPopMatrix();
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glVertex3f(-200.0f, 0.05f,   0.0f); glVertex3f( 200.0f, 0.05f,   0.0f);
    glColor4f(0.1f, 0.2f, 0.5f, 0.2f);
    glVertex3f( 200.0f, 0.05f, -90.0f); glVertex3f(-200.0f, 0.05f, -90.0f);
    glEnd();
    glDisable(GL_BLEND);
    gluDeleteQuadric(q);
    glEnable(GL_LIGHTING);
}

static void drawExtravagantRoof(float w, float h, float d, bool doubleEave)
{
    glPushMatrix();
    if (doubleEave) {
        glPushMatrix();
        glScalef(1.2f, 0.4f, 1.2f);
        drawPalaceRoof(w, h, d);
        glPopMatrix();
        glTranslatef(0.0f, h * 0.4f, 0.0f);
    }
    drawPalaceRoof(w, h, d);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.85f, 0.3f);
    float fx = w * 1.35f, fz = d * 1.35f;
    float cx[] = {-fx, fx, fx, -fx}, cz[] = {fz, fz, -fz, -fz};
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(cx[i], 0.0f, cz[i]);
        GLUquadric* q = gluNewQuadric();
        gluSphere(q, 0.25f, 8, 8);
        gluDeleteQuadric(q);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

static void drawPalaceBuilding(int floors, float baseScale, int style)
{
    GLUquadric* q = gluNewQuadric();
    for (int tier = 0; tier < floors; tier++) {
        float tierScale = 1.0f - (float)tier * (0.8f / (float)floors);
        float h  = 4.0f;
        float y  = (float)tier * h;
        glPushMatrix();
        glTranslatef(0.0f, y, 0.0f);
        glScalef(baseScale * tierScale, 1.0f, baseScale * tierScale);
        if      (style == 1) glColor3f(0.15f, 0.35f, 0.2f);
        else if (style == 2) glColor3f(0.15f, 0.2f,  0.35f);
        else                 glColor3f(0.7f,  0.1f,  0.05f);
        float pw = 4.0f, pd = 3.5f;
        float px[] = {-pw, pw, pw, -pw}, pz[] = {-pd, -pd, pd, pd};
        for (int i = 0; i < 4; i++) {
            glPushMatrix(); glTranslatef(px[i], 0.0f, pz[i]); glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, 0.28f, 0.28f, h, 16, 1); glPopMatrix();
        }
        glDisable(GL_LIGHTING);
        if      (style == 1) glColor3f(0.8f, 1.0f, 0.5f);
        else if (style == 2) glColor3f(0.6f, 0.8f, 1.0f);
        else                 glColor3f(1.0f, 0.7f, 0.2f);
        glPushMatrix(); glScalef(pw*1.75f, h*0.85f, pd*1.75f); glTranslatef(0.0f, 0.5f, 0.0f); drawCube(0.95f); glPopMatrix();
        glEnable(GL_LIGHTING);
        glTranslatef(0.0f, h, 0.0f);
        if      (style == 1) drawPalaceRoof(pw * 1.5f, 3.5f, pd * 1.5f);
        else if (style == 2) drawExtravagantRoof(pw * 1.8f, 2.5f, pd * 1.8f, false);
        else                 drawExtravagantRoof(pw * 1.6f, 3.0f, pd * 1.6f, (tier == 0 || tier == floors - 1));
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

static void drawGuanghanPalace()
{
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -75.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    glPushMatrix(); glScalef(25.0f, 1.0f, 20.0f); drawCube(1.0f); glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    drawPalaceBuilding(5, 1.8f, 0);
    glPopMatrix();
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); glTranslatef((float)i * 18.0f, 0.5f,  5.0f); drawPalaceBuilding(3, 1.2f, 0); glPopMatrix();
        glPushMatrix(); glTranslatef((float)i * 32.0f, 0.5f,-10.0f); drawPalaceBuilding(2, 1.0f, 0); glPopMatrix();
    }
    glPopMatrix();
}

static void drawCuteRabbit()
{
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glScalef(1.3f, 1.3f, 1.3f);
    glTranslatef(0.0f, 0.8f, 0.0f);
    glColor3f(0.95f, 0.95f, 0.98f);
    glPushMatrix(); glTranslatef(0.0f,-0.4f, 0.0f); glScalef(1.0f, 1.3f, 0.9f); gluSphere(q, 0.5f, 32, 32); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.4f, 0.0f); glScalef(1.2f, 0.95f, 1.1f); gluSphere(q, 0.55f, 32, 32); glPopMatrix();
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef((float)i * 0.25f, 0.9f, 0.0f);
        glRotatef((float)i * 10.0f, 0.0f, 0.0f, 1.0f); glRotatef(-5.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.95f, 0.95f, 0.98f);
        glPushMatrix(); glTranslatef(0.0f, 0.4f, 0.0f); glScalef(0.3f, 1.2f, 0.15f); gluSphere(q, 0.5f, 16, 16); glPopMatrix();
        glColor3f(0.9f, 0.4f, 0.5f);
        glPushMatrix(); glTranslatef(0.0f, 0.4f, 0.08f); glScalef(0.18f, 1.0f, 0.05f); gluSphere(q, 0.5f, 16, 16); glPopMatrix();
        glPopMatrix();
    }
    glColor3f(0.1f, 0.1f, 0.1f);
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); glTranslatef((float)i*0.28f, 0.45f, 0.52f); glRotatef((float)i*-15.0f, 0.0f,0.0f,1.0f); glScalef(0.15f,0.05f,0.05f); gluSphere(q,0.5f,16,16); glPopMatrix();
    }
    glColor3f(1.0f, 0.5f, 0.5f);
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); glTranslatef((float)i*0.45f, 0.35f, 0.48f); glScalef(0.12f,0.08f,0.05f); gluSphere(q,0.5f,16,16); glPopMatrix();
    }
    glColor3f(0.8f, 0.2f, 0.2f);
    glPushMatrix(); glTranslatef(0.0f,0.25f,0.58f); glRotatef(20.0f,1.0f,0.0f,0.0f); glScalef(0.12f,0.1f,0.05f); gluSphere(q,0.5f,16,16); glPopMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix(); glTranslatef(0.0f,0.29f,0.60f); glScalef(0.08f,0.05f,0.02f); drawCube(1.0f); glPopMatrix();
    glColor3f(1.0f, 0.6f, 0.7f);
    glPushMatrix(); glTranslatef(0.0f,0.35f,0.61f); glScalef(0.06f,0.04f,0.04f); gluSphere(q,0.5f,16,16); glPopMatrix();
    glColor3f(0.95f, 0.95f, 0.98f);
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); glTranslatef((float)i*0.35f,-0.2f,0.35f); glRotatef((float)i*-25.0f,0.0f,0.0f,1.0f); glRotatef(-30.0f,1.0f,0.0f,0.0f); glScalef(0.15f,0.4f,0.15f); gluSphere(q,0.5f,16,16); glPopMatrix();
    }
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); glTranslatef((float)i*0.25f,-0.95f,0.2f); glScalef(0.18f,0.1f,0.35f); gluSphere(q,0.5f,16,16); glPopMatrix();
    }
    glPushMatrix(); glTranslatef(0.0f,-0.6f,-0.45f); glScalef(0.25f,0.25f,0.25f); gluSphere(q,0.5f,16,16); glPopMatrix();
    glColor3f(0.8f, 0.0f, 0.1f);
    glPushMatrix(); glTranslatef(0.0f,-0.05f,0.45f); glScalef(0.12f,0.12f,0.12f); gluSphere(q,0.5f,16,16); glPopMatrix();
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); glTranslatef((float)i*0.16f,-0.05f,0.42f); glRotatef((float)i*20.0f,0.0f,0.0f,1.0f); glScalef(0.2f,0.12f,0.05f); gluSphere(q,0.5f,16,16); glPopMatrix();
    }
    glPopMatrix();
    gluDeleteQuadric(q);
}

static void drawMoonTown()
{
    struct Building { float x, z, s; int f; int style; };
    Building town[] = {
        {-22.0f, 12.0f, 0.80f, 2, 0}, { 22.0f, 15.0f, 0.90f, 2, 1},
        {-38.0f, -5.0f, 0.70f, 1, 2}, { 38.0f,-10.0f, 1.10f, 2, 0},
        {-18.0f, 42.0f, 0.60f, 1, 1}, { 18.0f, 38.0f, 0.75f, 2, 2},
        {-45.0f, 20.0f, 1.20f, 3, 0}, { 45.0f, 15.0f, 1.00f, 2, 1},
        {-18.0f,-55.0f, 0.50f, 1, 2}, { 25.0f,-60.0f, 0.60f, 1, 0},
        {-55.0f, 45.0f, 1.50f, 3, 1}, { 55.0f, 40.0f, 1.30f, 4, 2},
        {-30.0f, 60.0f, 1.00f, 2, 0}, { 35.0f, 55.0f, 0.80f, 2, 1},
        {-10.0f, 75.0f, 0.70f, 1, 2}, { 12.0f, 70.0f, 0.65f, 3, 0}
    };
    for (int i = 0; i < 16; i++) {
        glPushMatrix();
        glTranslatef(town[i].x, -0.2f, town[i].z);
        glRotatef((town[i].x > 0) ? -90.0f : 90.0f, 0.0f, 1.0f, 0.0f);
        glRotatef((i % 3 == 0) ? 10.0f : 0.0f, 0.0f, 1.0f, 0.0f);
        glScalef(town[i].s, town[i].s, town[i].s);
        drawPalaceBuilding(town[i].f, 1.0f, town[i].style);
        glPopMatrix();
    }
    for (int i = 0; i < 8; i++) {
        float zPosL = 12.0f - (float)i * 14.0f;
        if (!(zPosL < -20.0f && zPosL > -45.0f)) {
            glPushMatrix();
            glTranslatef(-15.0f, -0.2f, zPosL);
            glRotatef(-90.0f + rr(i*17, -5.0f, 5.0f), 0.0f, 1.0f, 0.0f);
            float s = 0.6f + rr(i*19, 0.0f, 0.2f);
            glScalef(s, s, s);
            drawPalaceBuilding((i % 2) + 2, 1.0f, (i % 3));
            glPopMatrix();
            if (hf(i*101) > 0.75f) {
                glPushMatrix(); glTranslatef(-10.0f, 0.0f, zPosL + 2.0f); glRotatef(120.0f, 0.0f, 1.0f, 0.0f); drawCuteRabbit(); glPopMatrix();
            }
        }
        float zPosR = 8.0f - (float)i * 14.0f;
        if (!(zPosR < -20.0f && zPosR > -45.0f)) {
            glPushMatrix();
            glTranslatef(15.0f, -0.2f, zPosR);
            glRotatef(90.0f + rr(i*23, -5.0f, 5.0f), 0.0f, 1.0f, 0.0f);
            float sR = 0.6f + rr(i*29, 0.0f, 0.2f);
            glScalef(sR, sR, sR);
            drawPalaceBuilding(((i+1) % 2) + 2, 1.0f, ((i+1) % 3));
            glPopMatrix();
            if (hf(i*103) > 0.75f) {
                glPushMatrix(); glTranslatef(10.0f, 0.0f, zPosR - 2.0f); glRotatef(-120.0f, 0.0f, 1.0f, 0.0f); drawCuteRabbit(); glPopMatrix();
            }
        }
    }
}

static void drawBackground()
{
    if (isNight) {
        drawGiantMoon();
        drawDistantMountains();
        drawGround();
        drawRiver();
        drawWaterDetails();
        drawStonePath();
        drawGuanghanPalace();
        drawMoonTown();
    } else {
        GLfloat lightPos[] = { lightX, lightY, lightZ, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        drawSun(lightX, lightY, lightZ);
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
}

// -------------------------------------------------------
// OpenGL / Win32 boilerplate
// -------------------------------------------------------
static void updateCamera()
{
    cameraX = sinf(cameraAngle) * cameraDistance;
    cameraZ = cosf(cameraAngle) * cameraDistance;
    cameraY = cameraHeight;
}

static void resetAll()
{
    cameraAngle    = 0.0f;
    cameraHeight   = 5.0f;
    cameraDistance = 30.0f;
    updateCamera();
}

static bool initPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.cAlphaBits  = 8;
    pfd.cColorBits  = 32;
    pfd.cDepthBits  = 24;
    pfd.cStencilBits = 0;
    pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iLayerType  = PFD_MAIN_PLANE;
    pfd.iPixelType  = PFD_TYPE_RGBA;
    pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion    = 1;
    int n = ChoosePixelFormat(hdc, &pfd);
    return SetPixelFormat(hdc, n, &pfd) ? true : false;
}

static void initOpenGL()
{
    // Night sky colour or daytime blue
    if (isNight) glClearColor(0.01f, 0.01f, 0.06f, 1.0f);
    else         glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 800.0 / 600.0, 0.5f, 600.0f);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
}

static void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    if (isNight) {
        GLfloat amb[] = { 0.15f, 0.15f, 0.25f, 1.0f };
        GLfloat dif[] = { 0.30f, 0.30f, 0.45f, 1.0f };
        GLfloat pos[] = { 0.0f, 20.0f, -50.0f, 1.0f };  // moon position
        glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
        glLightfv(GL_LIGHT0, GL_POSITION, pos);
    } else {
        GLfloat amb[] = { 0.40f, 0.40f, 0.35f, 1.0f };
        GLfloat dif[] = { 1.00f, 0.95f, 0.80f, 1.0f };
        GLfloat pos[] = { lightX, lightY, lightZ, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
        glLightfv(GL_LIGHT0, GL_POSITION, pos);
    }
}

static void display()
{
    // Refresh background clear colour in case isNight toggled
    if (isNight) glClearColor(0.01f, 0.01f, 0.06f, 1.0f);
    else         glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glPolygonMode(GL_FRONT_AND_BACK, isWireframe ? GL_LINE : GL_FILL);

    updateCamera();
    gluLookAt(cameraX, cameraY, cameraZ,
              0.0f,    0.0f,   -10.0f,
              0.0f,    1.0f,    0.0f);

    setupLighting();
    drawBackground();
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
        case 'N': case 'n':
            isNight = !isNight;
            break;
        case 'I': case 'i':
            isWireframe = !isWireframe;
            break;
        case VK_LEFT:
            cameraAngle -= 0.05f;
            break;
        case VK_RIGHT:
            cameraAngle += 0.05f;
            break;
        case VK_UP:
            cameraHeight += 0.5f;
            break;
        case VK_DOWN:
            cameraHeight -= 0.5f;
            break;
        case VK_ADD:
        case VK_OEM_PLUS:
            cameraDistance -= 1.0f;
            if (cameraDistance < 2.0f) cameraDistance = 2.0f;
            break;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            cameraDistance += 1.0f;
            break;
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

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

    HDC   hdc   = GetDC(hWnd);
    initPixelFormat(hdc);
    HGLRC hglrc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, hglrc)) return false;

    initOpenGL();
    resetAll();

    // Load textures (paths relative to executable; adjust if needed)
    texGrass   = loadBMPTexture("Textures/grass.bmp");
    texFanWood = loadBMPTexture("Textures/wood.bmp");

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
