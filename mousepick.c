#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "mousepick.h"
#include "math.h"

#define RAY_DEBUG 1

//this function relies upon uptodate modelview and mouse coords so best to call function immediately after any view change or mouse handling

//Method 1. use depth buffer via glReadPixels to identify depth value of a point in 3d space when provided with x, y, cursor position - i.e a change of depth at x a y will indicate if geometry is beneath the cursor. 
//To be useful would then need a further function to identify if the point is within the boundying box of a specific object or within a defined radius.

GLint handleMousePicking(GLint mouse_x, GLint mouse_y, GLint viewport[4], GLfloat viewMatrix[16], GLfloat modelMatrix[16], GLfloat perspectiveMatrix[16], GLfloat worldCoords[3]){
	
	GLfloat winX, winY;
	GLfloat x, y, z;
	GLfloat depth[2];
	
	//winY is flipped because OpenGL needs window coords to start 0,0 at lower left
	winX = (GLfloat) mouse_x;
	winY = (GLfloat) viewport[3] - mouse_y; 
	if(RAY_DEBUG){
		fprintf(stdout,"winX %f winY %f \n", winX, winY);
	}

	//get z depth of object under x,y - depth is a array, it is the first [0] value that is useful
	glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
	
	//modelview matrix - i.e view * model 
	GLfloat modelViewMatrix[16];
	multiMatrix(viewMatrix, modelMatrix, modelViewMatrix);
	
	if(WorldSpaceCoords(winX, winY, depth[0], modelViewMatrix, perspectiveMatrix, viewport, &x, &y, &z)){
		if(RAY_DEBUG){
			fprintf(stdout,"depth 0 %f \n", depth[0]);
			fprintf(stdout,"coord 0 %f %f %f\n", x, y, z);
		}
	}else{
		if(RAY_DEBUG){
			fprintf(stdout,"WorldSpaceCoords error\n");
		}
	}	
	
	worldCoords[0] = x;
	worldCoords[1] = y;
	worldCoords[2] = z;
}

GLboolean WorldSpaceCoords(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat modelViewMatrix[16], GLfloat projectionMatrix[16], GLint viewport[4],
GLfloat *x, GLfloat *y, GLfloat *z){
	
	GLfloat matrix[16];
	GLfloat inverseMatrix[16];
	GLfloat in[4];
	GLfloat out[4];
	
	//this order generates correct figures
	multiMatrix(projectionMatrix, modelViewMatrix, matrix);
	invertMatrix(matrix, inverseMatrix);
	
	in[0] = winX;
	in[1] = winY;
	in[2] = winZ;
	in[3] = 1.0;
	
	//map x and y from window coords
	in[0] = (in[0] - viewport[0]) / viewport[2];
	in[1] = (in[1] - viewport[1]) / viewport[3];
	
	//map to range -1 to 1
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;
	
	//this function can return an error BUT not sure under what conditions
	multiplyVectorByMatrix(in, inverseMatrix, out);
	if(out[3] == 0.0) return GL_FALSE;
	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	
	*x = out[0];
	*y = out[1];
	*z = out[2];
	
	return GL_TRUE; 
}

//Method 2. Project a ray from camera position into the scene along -z axis. The equation of the ray can be used to test intersection. Probably the better method but not as easy to use. 

GLint getMouseRay(GLint mouse_x, GLint mouse_y, GLint viewport[4], GLfloat cameraPos[3], GLfloat viewMatrix[16], GLfloat modelMatrix[16], GLfloat perspectiveMatrix[16], GLfloat ray[3]){

	//first challenge is getting from screen space (viewport coords to normalised to device space)
	GLfloat winX, winY;
	GLfloat x, y, z;
	GLfloat RayEyeStart[4], RayEyeEnd[4];
	GLfloat RayWorldStart[4], RayWorldEnd[4];
	GLfloat RayWorldDirection[4];

	//winY is flipped because OpenGL needs coord to start 0,0 at lower left
	winX = (GLfloat) mouse_x;
	winY = (GLfloat) viewport[3] - mouse_y; 
	if(RAY_DEBUG){
		fprintf(stdout,"%f %f \n", winX, winY);
	}
	
	//think of this as preparation for RayEyeStart
	GLfloat in[4];
	in[0] = winX;
	in[1] = winY;
	in[2] = -1.0; // manually set down the -z axis
	in[3] = 1.0;
	
	//map x and y from window coords
	in[0] = (in[0] - viewport[0]) / viewport[2];
	in[1] = (in[1] - viewport[1]) / viewport[3];
	
	//map x and y to range -1 to 1 - i.e NDC coords 
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;

	if(RAY_DEBUG){
		fprintf(stdout,"in %f %f %f %f \n", in[0], in[1], in[2], in[3]);
	}
	
	//think of this as preparation for RayEyeEnd, the only difference is that [2] is 1.0f rather than -1.0f
	GLfloat in_end[4];
	in_end[0] = in[0];
	in_end[1] = in[1];
	in_end[2] = 1.0f;
	in_end[3] = in[3];
	
	//EYE SPACE or 'camera space' - inverse projectionMatrix * vec4
	//these ray points are only calculated for the purpose of getting to the world ray 
	
	//inverse projection matrix
	GLfloat inverseProjectionMatrix[16];
	invertMatrix(perspectiveMatrix, inverseProjectionMatrix);
	
	//RayEyeStart - Start of the ray in eye space
	multiplyVectorByMatrix(in, inverseProjectionMatrix, RayEyeStart);
	RayEyeStart[0] /= RayEyeStart[3]; RayEyeStart[1] /= RayEyeStart[3]; RayEyeStart[2] /= RayEyeStart[3]; RayEyeStart[3] /= RayEyeStart[3];
	if(RAY_DEBUG){
		fprintf(stdout,"RayEyeStart %f %f %f %f \n", RayEyeStart[0], RayEyeStart[1], RayEyeStart[2], RayEyeStart[3]);
	}
	
	//RayEyeEnd - End of the ray in eye space
	multiplyVectorByMatrix(in_end, inverseProjectionMatrix, RayEyeEnd);
	RayEyeEnd[0] /= RayEyeEnd[3]; RayEyeEnd[1] /= RayEyeEnd[3]; RayEyeEnd[2] /= RayEyeEnd[3]; RayEyeEnd[3] /= RayEyeEnd[3];
	if(RAY_DEBUG){
		fprintf(stdout,"RayEyeStart %f %f %f %f \n", RayEyeEnd[0], RayEyeEnd[1], RayEyeEnd[2], RayEyeEnd[3]);
	}
	
	//WORLD SPACE - multiply ray by inverse view matrix means the ray goes from camera space to world space
	
	//inverse modelview matrix
	GLfloat inverseViewMatrix[16];
	GLfloat modelViewMatrix[16];
	multiMatrix(viewMatrix, modelMatrix, modelViewMatrix);
	invertMatrix(modelViewMatrix, inverseViewMatrix);
	
	//RayWorldStart - Start of ray in world space
	multiplyVectorByMatrix(RayEyeStart, inverseViewMatrix, RayWorldStart);
	RayWorldStart[0] /= RayWorldStart[3]; RayWorldStart[1] /= RayWorldStart[3]; RayWorldStart[2] /= RayWorldStart[3]; RayWorldStart[3] /= RayWorldStart[3];
	if(RAY_DEBUG){
		fprintf(stdout,"RayWorldStart %f %f %f %f \n", RayWorldStart[0], RayWorldStart[1], RayWorldStart[2], RayWorldStart[3]);
	}
	
	//RayWorldEnd - End of ray in world space
	multiplyVectorByMatrix(RayEyeEnd, inverseViewMatrix, RayWorldEnd);
	RayWorldEnd[0] /= RayWorldEnd[3]; RayWorldEnd[1] /= RayWorldEnd[3]; RayWorldEnd[2] /= RayWorldEnd[3]; RayWorldEnd[3] /= RayWorldEnd[3];
	if(RAY_DEBUG){
		fprintf(stdout,"RayWorldEnd %f %f %f %f \n", RayWorldEnd[0], RayWorldEnd[1], RayWorldEnd[2], RayWorldEnd[3]);
	}
	
	//from here onwards we don't need the 4th vector component
	
	//RayDirection - this is simple vector subtraction of end - start to return a 3d vector describing the direction of the ray in world space
	RayWorldDirection[0] = RayWorldEnd[0] - RayWorldStart[0];
	RayWorldDirection[1] = RayWorldEnd[1] - RayWorldStart[1];
	RayWorldDirection[2] = RayWorldEnd[2] - RayWorldStart[2];
	
	//normalise
	GLfloat l;
	l = (GLfloat) sqrt(RayWorldDirection[0] * RayWorldDirection[0] + RayWorldDirection[1] * RayWorldDirection[1] + RayWorldDirection[2] * RayWorldDirection[2]);
	RayWorldDirection[0] /= l; RayWorldDirection[1] /= l; RayWorldDirection[2] /= l;
	
	if(RAY_DEBUG){
		fprintf(stdout,"out direction %f %f %f %f \n", RayWorldDirection[0], RayWorldDirection[1], RayWorldDirection[2], RayWorldDirection[3]);
	}
	
	//output ray direction
	ray[0] = RayWorldDirection[0];
	ray[1] = RayWorldDirection[1];
	ray[2] = RayWorldDirection[2];	
}


/*
GLint testHit2 - from cameraPos, which is specified in world space, travel along raydirection in steps and when impact is recieved print value.
hitspot is a world location you want to test against

can use within a game loop or on button click (assuming access to uptpdate matrix and camera position)
Example:

	GLfloat ray[3];
    getMouseRay(mouse_x, mouse_y, viewport, cameraPos, viewMatrix, modelMatrix, perspectiveMatrix, ray);
    
     GLfloat bunny_locations[4][3] = {
		{-10.0, 0.0, -10.0},
		{10.0, 0.0, -10.0},
		{10.0, 0.0, 10.0},
		{-10.0, 0.0, 10.0}
	};
	
	testHit("blue", cameraPos, ray, bunny_locations[0]);
    testHit("red",cameraPos, ray, bunny_locations[1]);
    testHit("green",cameraPos, ray, bunny_locations[2]);
    testHit("orange",cameraPos, ray, bunny_locations[3]);
    
    Then when you mouseover the objects in your scene (such as the blue bunny) you will get:
    
    blue hit -0.039295 -0.343065 0.201419 @ -9.960705 0.343065 -10.201419
    
*/

GLint testHit(GLchar *str, GLfloat cameraPos[3], GLfloat rayDirection[3], GLfloat hitSpot[3]){
	
	//progress along ray direction
	
	GLfloat worldRayPosition[3];
	GLfloat x, y, z;
	
	GLfloat max = 0.5f;
	
	int i = 0;
	while(i < 50){
		
		worldRayPosition[0] = cameraPos[0] + rayDirection[0] * i;
		worldRayPosition[1] = cameraPos[1] + rayDirection[1] * i;
		worldRayPosition[2] = cameraPos[2] + rayDirection[2] * i;	
		
		x = hitSpot[0] - worldRayPosition[0];
		y = hitSpot[1] - worldRayPosition[1];
		z = hitSpot[2] - worldRayPosition[2];
		
		if(x >= -max && x <= max 
		&& y >= -max && y <= max
		&& z >= -max && z <= max){
			
			fprintf(stdout,"%s hit %f %f %f", str, x, y, z);
			fprintf(stdout," @ %f %f %f\n", worldRayPosition[0], worldRayPosition[1], worldRayPosition[2]);
			break;
		}
		
		i++;	
	}
	return 0;
}

/*
GLint testHit2 - this is a more mathematical way of working out if an object is below the mouse. 
from cameraPos, which is specified in world space, and when impact is recieved print value.
hitspot is a world location you want to test against

can use within a game loop or on button click (assuming access to uptpdate matrix and camera position)
Example:

	GLfloat ray[3];
    getMouseRay(mouse_x, mouse_y, viewport, cameraPos, viewMatrix, modelMatrix, perspectiveMatrix, ray);
    
     GLfloat bunny_locations[4][3] = {
		{-10.0, 0.0, -10.0},
		{10.0, 0.0, -10.0},
		{10.0, 0.0, 10.0},
		{-10.0, 0.0, 10.0}
	};
	
	testHit2("blue", cameraPos, ray, bunny_locations[0]);
    testHit2("red",cameraPos, ray, bunny_locations[1]);
    testHit2("green",cameraPos, ray, bunny_locations[2]);
    testHit2("orange",cameraPos, ray, bunny_locations[3]);
    
    Then when you mouseover the objects in your scene (such as the blue bunny) you will get:
    
    blue hit
    missed
    missed
    missed
*/

GLint testHit2(GLchar *str, GLfloat cameraPos[3], GLfloat rayDirection[3], GLfloat hitSpot[3]){
	
	GLfloat worldRayPosition[3];
	GLfloat x, y, z;
	
	GLfloat max = 0.5;
	
	GLfloat distToSpot[3];
	distToSpot[0] = cameraPos[0] - hitSpot[0];
	distToSpot[1] = cameraPos[1] - hitSpot[1];
	distToSpot[2] = cameraPos[2] - hitSpot[2];

	GLfloat b = dot(rayDirection, distToSpot);
	GLfloat c = dot(distToSpot, distToSpot) - (max * max);
	GLfloat b_squared_minus_c = (b * b) - c;

	//mouse not near object
	if(b_squared_minus_c < 0.0f){
		fprintf(stdout,"missed\n");
	}
	
	//object has been 'hit' by ray twice, visualise the ray going through one part of a model and exiting the another side
	if(b_squared_minus_c > 0.0f){
		fprintf(stdout,"%s hit\n", str);
	}
	
	//ray has skimmed the surface of the object - i'll include this a crude 'hit' also
	if(b_squared_minus_c == 0.0f){
		fprintf(stdout,"%s hit\n", str);
	}
	return 0;
}
