#ifndef     GLXGL_MOUSEPICK
#define     GLXGL_MOUSEPICK

#include <GL/gl.h>

//Method 1
GLint handleMousePicking(GLint mouse_x, GLint mouse_y, GLint viewport[4], GLfloat viewMatrix[16], GLfloat modelMatrix[16], GLfloat perspectiveMatrix[16], GLfloat worldCoords[3]);

GLboolean WorldSpaceCoords(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat modelViewMatrix[16], GLfloat projectionMatrix[16], GLint viewport[4],
GLfloat *x, GLfloat *y, GLfloat *z);

//Method 2
GLint getMouseRay(GLint mouse_x, GLint mouse_y, GLint viewport[4], GLfloat cameraPos[3], GLfloat viewMatrix[16], GLfloat modelMatrix[16], GLfloat perspectiveMatrix[16], GLfloat ray[3]);
GLint testHit(GLchar *str, GLfloat cameraPos[3], GLfloat rayDirection[3], GLfloat hitSpot[3]);
GLint testHit2(GLchar *str, GLfloat cameraPos[3], GLfloat rayDirection[3], GLfloat hitSpot[3]);

extern GLfloat worldRay[4];
#endif
