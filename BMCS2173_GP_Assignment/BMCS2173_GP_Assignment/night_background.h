#ifndef NIGHT_BACKGROUND_H
#define NIGHT_BACKGROUND_H

#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

// External globals from main.cpp
extern bool isNight;
extern bool isWarmClothing;
extern float lightX, lightY, lightZ;
extern GLuint texGrass;
extern GLuint texFanWood;

namespace night_bg {
    // Main entry point for background rendering
    void drawBackground();
    
    // Individual components (if needed publicly)
    void drawGround();
    void drawCloud(float cx, float cy, float cz, float scale);
    void drawSun(float sx, float sy, float sz);
}

#endif // NIGHT_BACKGROUND_H
