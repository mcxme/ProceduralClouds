#include "Sphere.h"


Sphere::Sphere(float radius, unsigned int rings, unsigned int sectors)
{
	setup(radius, rings, sectors, vec3(0, 0, 0));
}

Sphere::Sphere(float radius, unsigned int rings, unsigned int sectors, vec3 center)
{
	setup(radius, rings, sectors, center);
}

Sphere::~Sphere()
{
	vertices.clear();
	normals.clear();
	texcoords.clear();
	indices.clear();
}

void Sphere::setup(float radius, unsigned int rings, unsigned int sectors, vec3 center)
{
	Sphere::radius = radius;

	float const R = 1. / (float)(rings - 1);
	float const S = 1. / (float)(sectors - 1);
	int r, s;

	vertices.resize(rings * sectors);
	normals.resize(rings * sectors);
	texcoords.resize(rings * sectors);
	std::vector<vec3>::iterator v = vertices.begin();
	std::vector<vec3>::iterator n = normals.begin();
	std::vector<vec2>::iterator t = texcoords.begin();

	for (r = 0; r < rings; r++)
	{
		for (s = 0; s < sectors; s++) {
			float const y = sin(-M_PI_2 + M_PI * r * R);
			float const x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
			float const z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);

			*t++ = vec2(s*S, r*R);
			*v++ = vec3(x, y, z) * radius + center;
			*n++ = vec3(x, y, z);
		}
	}

	indices.resize((rings - 1) * (sectors - 1) * 3 * 2);
	std::vector<GLuint>::iterator i = indices.begin();
	for (r = 0; r < rings - 1; r++) for (s = 0; s < sectors - 1; s++) {
		// create 2 triangles: 1 - 2 - 4  &  2 - 3 - 4
		*i++ = r * sectors + s; // 1
		*i++ = r * sectors + (s + 1); // 2
		*i++ = (r + 1) * sectors + s; // 4

		*i++ = r * sectors + (s + 1); // 2
		*i++ = (r + 1) * sectors + (s + 1); // 3
		*i++ = (r + 1) * sectors + s; // 4
	}
}