#pragma once

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <limits>
#include <math.h>
#include <cmath>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

#include <iostream>
// error macro
#define GLERROR do { \
     GLint err;                          \
	 	 	 	      while ((err = glGetError()) != GL_NO_ERROR) \
					  				  			  {\
		/*__debugbreak();*/ \
		GLubyte const* str = gluErrorString(err);       \
		std::cout << __FILE__ << ":" << __LINE__ << " : GLERROR: " << str << std::endl; \
		_sleep(1000);\
				  			  } \
} while (0)

#define M_PI 3.14159265359
#define M_PI_2 1.57079632679

class Sphere
{
public:
	Sphere(float radius, unsigned int rings, unsigned int sectors, vec3 center);
	Sphere(float radius, unsigned int rings, unsigned int sectors);
	~Sphere();
	void draw(GLfloat x, GLfloat y, GLfloat z);
	void render(GLFWwindow* window);
//protected:
	std::vector<vec3> vertices;
	std::vector<vec3> normals;
	std::vector<vec2> texcoords;
	std::vector<GLuint> indices;
	vec3 center;
	float radius;
private:
	void setup(float radius, unsigned int rings, unsigned int sectors, vec3 center);
};

