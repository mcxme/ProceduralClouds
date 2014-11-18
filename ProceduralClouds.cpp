#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>
#include <iostream>
#include <assert.h>
#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <common/shader.hpp>
#include <common/controls.hpp>
#include "project\Sphere.h"
using namespace glm;
#define PI 3.14159265


// Constants
float FRONT_THRESHOLD_DISTANCE = 0.99f;
float BACK_THRESHOLD_DISTANCE = 0.01f;
const int NUMBER_OF_SPHERES = 1000000;
int SPHERE_NUMBER_OF_VERTICES = 25;
const bool ROTATE_LIGHT = true;
float MORPHING_SPEED_PER_SECOND = 0.1f;
float FLOATING_SPEED_PER_SECOND = 2.0f;
const int WINDOW_WIDTH = 1024;
const int WINDOW_HIGHT = 768;
const int SPHERES_PER_CLOUD = 1;
const float SPAWN_CLOUD_LIKELYHOOD = 0.35f * SPHERES_PER_CLOUD / NUMBER_OF_SPHERES;
const vec3 INITIAL_EXPANSION_DIRECTION = vec3(1.0f, 0.0f, 1.0f); // leave y coordinate blank to avoid a tilted plane
const int MIN_FRAME_RATE = 32;
const bool DRAW_SKYDOME = true;
const bool DRAW_CLOUDS = true;
const float START_DAYTIME = 10.0f;
const float SUNRISE_HOUR = 6.0f;
const float SUNSET_HOUR = 21.0f;
const float DAY_TIME_UPDATE_PER_SECOND = 0.1f;
float MIN_CLOUD_CREATION_DISTANCE_SECONDS = 0.15f;
const float NONSE_FACTOR = 100;
const float MOVING_SPEED = 100.0f; // per second
const float MOUSE_SPEED = 0.1f;
const float SKYDOME_RADIUS = 300.0f;
const float CLOUD_PLANE_HIGHT = SKYDOME_RADIUS * 0.3f;
const float CLOUD_PLANE_RADIUS = sqrt(SKYDOME_RADIUS * SKYDOME_RADIUS - CLOUD_PLANE_HIGHT * CLOUD_PLANE_HIGHT);
const vec3 CLOUD_PLANE_CENTER = vec3(0, CLOUD_PLANE_HIGHT, 0);
float MAX_CLOUD_SCALE = 10.0f;
float MIN_CLOUD_SCALE = 4.0f;


// various attributes
GLFWwindow* window;
vec3 lightDirection(1, -0.5f, -1);
float lightIntensity = 0;
float rotateAngle = 0;
vec3 cameraPosition(0, 0, 0);
float cameraAngle = 0;
glm::mat4 Projection;
std::vector<Sphere> spheres;
Sphere skydome(SKYDOME_RADIUS, 100, 100);
vec3 expansionDirection = INITIAL_EXPANSION_DIRECTION;
std::vector<vec3> offsets;
std::vector<GLfloat> nonses;
std::vector<GLfloat> scales;
std::vector<vec3> thresholdNormals;
std::vector<vec3> frontPoints;
std::vector<vec3> backPoints;
float currentFrameRate = 0;
float daytime = START_DAYTIME;
float horizontalAngle = PI;
float verticalAngle = 0.0f;
vec3 cameraDirectionVector;
mat4 MVP;

// handles
GLuint sphereProgramID;
GLuint skydomeProgramID;
GLuint cameraLocation;
GLuint cameraDirection;
GLuint expansionDirectionLocation;
GLuint MatrixID;
GLuint SkydomeMatrixID;
GLuint sphereCenterLocation;
GLuint morphingSpeedLocation;
GLuint timeLocation;
GLuint lightDirectionLocation;
GLuint skydomelightIntensityLocation;
GLuint VertexArrayID;
GLuint vertexbuffer;
GLuint indexbuffer;
GLuint normalbuffer;
GLuint textureCoordsBuffer;
GLuint offsetsBuffer;
GLuint nonseBuffer;
GLuint scaleBuffer;
GLuint skydomeVertexbuffer;
GLuint skydomeIndexbuffer;
GLuint skyDomeNormalbuffer;
GLuint skydomeTextureCoordsBuffer;
GLuint skydomeLightDirectionLocation;
GLuint skydomeCameraDirectionLocation;
GLuint skydomecameraPositionLocation;
GLuint thresholdNormalsBuffer;
GLuint frontPointsBuffer;
GLuint backPointsBuffer;
GLuint lightIntensityLocation;


// method definitions
float lightIntensityFromTime(float hours);
vec3 lightDirectionFromTime(float hours);
void sendUniformsToCloudShader(float passedTime);
void sendUniformsToSkydomeShader();

void sendUniformsToCloudShader(float passedTime)
{
	glUniform3f(cameraLocation, cameraPosition.x, cameraPosition.y, cameraPosition.z); GLERROR;
	glUniform3f(cameraDirection, cameraDirectionVector.x, cameraDirectionVector.y, cameraDirectionVector.z); GLERROR;
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]); GLERROR;
	glUniform1f(morphingSpeedLocation, MORPHING_SPEED_PER_SECOND); GLERROR;
	glUniform3f(sphereCenterLocation, spheres.at(0).center.x, spheres.at(0).center.y, spheres.at(0).center.z); GLERROR;
	glUniform1f(timeLocation, passedTime * MORPHING_SPEED_PER_SECOND); GLERROR;
	glUniform3f(expansionDirectionLocation, expansionDirection.x, expansionDirection.y, expansionDirection.z); GLERROR;
	glUniform3f(lightDirectionLocation, lightDirection.x, lightDirection.y, lightDirection.z); GLERROR;
	glUniform1f(lightIntensityLocation, lightIntensity); GLERROR;
}

float calculateDistanceToCamera(int index)
{
	vec3 offset = offsets.at(index);
	return abs(length(cameraPosition - offset));
}

void sendUniformsToSkydomeShader()
{
	glUniform3f(skydomecameraPositionLocation, cameraPosition.x, cameraPosition.y, cameraPosition.z); GLERROR;
	glUniform3f(skydomeCameraDirectionLocation, cameraDirectionVector.x, cameraDirectionVector.y, cameraDirectionVector.z); GLERROR;
	glUniform3f(skydomeLightDirectionLocation, lightDirection.x, lightDirection.y, lightDirection.z); GLERROR;
	glUniform1f(skydomelightIntensityLocation, lightIntensity); GLERROR;
	glUniformMatrix4fv(SkydomeMatrixID, 1, GL_FALSE, &MVP[0][0]); GLERROR;
}

void updateLight(float passedTime)
{
	if (ROTATE_LIGHT)
	{
		daytime += DAY_TIME_UPDATE_PER_SECOND * passedTime;
		if (daytime >= 24.0f) daytime -= 24.0f;
		lightIntensity = lightIntensityFromTime(daytime);
		lightDirection = lightDirectionFromTime(daytime);
	}
}

vec3 lightDirectionFromTime(float hours)
{
	vec3 direction(0);
	if (hours < SUNRISE_HOUR || hours > SUNSET_HOUR)
	{
		direction = vec3(0, 1, 0);
	}
	else
	{
		float highest = SUNRISE_HOUR + 0.5f * (SUNSET_HOUR - SUNRISE_HOUR);
		float percentage = lightIntensity * 0.5f;
		if (hours > highest) percentage = 0.5f + 0.5f * (1 - lightIntensity);

		// x,y,z in [0,1]
		float y = sin(PI * percentage);
		float x = 2.0f * percentage - 1.0f;
		float z = 0.0f;
		direction = vec3(x, y, z);
		direction = normalize(direction);
	}

	return direction;
}

float lightIntensityFromTime(float hours)
{
	float intensity = 0;
	if (hours < SUNRISE_HOUR || hours > SUNSET_HOUR)
	{
		intensity = 0.0f;
	}
	else
	{
		float highest = SUNRISE_HOUR + 0.5 * (SUNSET_HOUR - SUNRISE_HOUR);
		if (hours < highest)
		{
			intensity = (1 / (highest - SUNRISE_HOUR)) * (hours - SUNRISE_HOUR);
		}
		else
		{
			intensity = (1 / (highest - SUNSET_HOUR)) * (hours - SUNSET_HOUR);
		}
	}

	return intensity;
}

void updateCamera(float passedTime)
{

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, (double)WINDOW_WIDTH / 2, (double)WINDOW_HIGHT / 2);

	// Compute new orientation
	horizontalAngle += MOUSE_SPEED * passedTime * float(WINDOW_WIDTH / 2 - xpos);
	verticalAngle += MOUSE_SPEED * passedTime * float(WINDOW_HIGHT / 2 - ypos);

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
		);

	// Right vector
	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
		);

	// Up vector : perpendicular to both direction and right
	glm::vec3 up = glm::cross(right, direction);

	// Move forward
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
		cameraPosition += direction * passedTime * MOVING_SPEED;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
		cameraPosition -= direction * passedTime * MOVING_SPEED;
	}
	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
		cameraPosition += right * passedTime * MOVING_SPEED;
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
		cameraPosition -= right * passedTime * MOVING_SPEED;
	}

	// calculate threshold for cloud rendering with new camera position
	// threshold equation: normal equation with normal towards camera and point on plain
	for (size_t i = 0; i < offsets.size(); i++)
	{
		thresholdNormals.at(i) = cameraPosition - offsets.at(i);
		thresholdNormals.at(i) = normalize(thresholdNormals.at(i));
		frontPoints.at(i) = FRONT_THRESHOLD_DISTANCE * thresholdNormals.at(i) * spheres.at(0).radius * scales.at(i) + offsets.at(i);
		backPoints.at(i) = BACK_THRESHOLD_DISTANCE * thresholdNormals.at(i) * spheres.at(0).radius * scales.at(i) + offsets.at(i);
	}

	vec3 lookAtPosition = cameraPosition + direction;
	cameraDirectionVector = normalize(cameraPosition - lookAtPosition);

	// Camera matrix
	glm::mat4 View = glm::lookAt(
		cameraPosition,           // Camera is here
		lookAtPosition, // and looks here : at the same position, plus "direction"
		up                  // Head is up (set to 0,-1,0 to look upside-down)
		);
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

	// change data structures to draw far clouds first
	// linear sorting assumption: camera updates happen more frequently then clouds can flip around
	// data structures: first element is most far away from camera
	float lastDistance = calculateDistanceToCamera(0);
	for (size_t i = 1; i < offsets.size(); i++)
	{
		float currentDistance = calculateDistanceToCamera(i);
		if (lastDistance < currentDistance)
		{
			// swap data
			vec3 offsetTmp = offsets.at(i);
			offsets.at(i) = offsets.at(i - 1);
			offsets.at(i - 1) = offsetTmp;
			float nonseTmp = nonses.at(i);
			nonses.at(i) = nonses.at(i - 1);
			nonses.at(i - 1) = nonseTmp;
			float scaleTmp = scales.at(i);
			scales.at(i) = scales.at(i - 1);
			scales.at(i - 1) = scaleTmp;
			vec3 frontPointTmp = frontPoints.at(i);
			frontPoints.at(i) = frontPoints.at(i - 1);
			frontPoints.at(i - 1) = frontPointTmp;
			vec3 backPointsTmp = backPoints.at(i);
			backPoints.at(i) = backPoints.at(i - 1);
			backPoints.at(i - 1) = backPointsTmp;
			vec3 thresholdNormalTmp = thresholdNormals.at(i);
			thresholdNormals.at(i) = thresholdNormals.at(i - 1);
			thresholdNormals.at(i - 1) = thresholdNormalTmp;
		}

		lastDistance = currentDistance;
	}
}

void addSphere(vec3 offset)
{
	// add a sphere on the outline of the circular cloud plane
	float xRand2D = -0.5f + static_cast <float> (rand()) / static_cast <float> (RAND_MAX); // [-0.5, 0.5]
	float yRand2D = -0.5f + static_cast <float> (rand()) / static_cast <float> (RAND_MAX); // [-0.5, 0.5]
	// create the plane for the clouds
	vec2 planarExpansionDirection = vec2(expansionDirection.x, expansionDirection.z); // the y coordinate points upward and is thus only the hight
	planarExpansionDirection = normalize(planarExpansionDirection);
	// randomize the start position
	planarExpansionDirection.x += xRand2D;
	planarExpansionDirection.y += yRand2D;
	planarExpansionDirection = normalize(planarExpansionDirection);
	// scale the plane up to the size of the sky (+10% to make clouds float into the scene)
	planarExpansionDirection = -planarExpansionDirection * CLOUD_PLANE_RADIUS * 1.1f;

	float rX = planarExpansionDirection.x;
	float rY = CLOUD_PLANE_HIGHT;
	float rZ = planarExpansionDirection.y;
	offsets.insert(offsets.begin(), vec3(rX, rY, rZ));

	nonses.insert(nonses.begin(), NONSE_FACTOR * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)); // [0,NONSE_FACTOR]
	scales.insert(scales.begin(), MIN_CLOUD_SCALE + (MAX_CLOUD_SCALE - MIN_CLOUD_SCALE) * static_cast <float> (rand()) / static_cast <float> (RAND_MAX)); // [min,max]
	thresholdNormals.insert(thresholdNormals.begin(), vec3(0)); // add standard value which will be updated
	backPoints.insert(backPoints.begin(), vec3(0)); // add standard value which will be updated
	frontPoints.insert(frontPoints.begin(), vec3(0)); // add standard value which will be updated
}

void addCloud(int numberOfSpheres)
{
	vec3 offset(0);
	for (size_t i = 0; i < numberOfSpheres; i++)
	{
		addSphere(offset);
	}
}

void sceneSetup()
{
	// add cloud template
	spheres.push_back(Sphere(1, SPHERE_NUMBER_OF_VERTICES, SPHERE_NUMBER_OF_VERTICES));

	// add first cloud
	addCloud(SPHERES_PER_CLOUD);
}

void openGlSetup()
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);


	// Create and compile our GLSL program from the shaders
	sphereProgramID = LoadShaders("Sphere.vertexshader", "Sphere.fragmentshader");
	GLERROR;
	skydomeProgramID = LoadShaders("Skydome.vertexshader", "Skydome.fragmentshader");
	GLERROR;

	// register uniforms
	// -- for the clouds
	cameraLocation = glGetUniformLocation(sphereProgramID, "cameraPosition");
	cameraDirection = glGetUniformLocation(sphereProgramID, "cameraDirection");
	expansionDirectionLocation = glGetUniformLocation(sphereProgramID, "expansionDirection");
	MatrixID = glGetUniformLocation(sphereProgramID, "MVP");
	sphereCenterLocation = glGetUniformLocation(sphereProgramID, "sphereCenter");
	morphingSpeedLocation = glGetUniformLocation(sphereProgramID, "morphingSpeed");
	timeLocation = glGetUniformLocation(sphereProgramID, "time");
	lightDirectionLocation = glGetUniformLocation(sphereProgramID, "lightDirection");
	lightIntensityLocation = glGetUniformLocation(sphereProgramID, "lightIntensity");

	// -- for the skydome
	SkydomeMatrixID = glGetUniformLocation(skydomeProgramID, "skyMVP");
	skydomelightIntensityLocation = glGetUniformLocation(skydomeProgramID, "lightIntensity");
	skydomeLightDirectionLocation = glGetUniformLocation(skydomeProgramID, "skydomeLightDirection");
	skydomeCameraDirectionLocation = glGetUniformLocation(skydomeProgramID, "cameraDirection");
	skydomecameraPositionLocation = glGetUniformLocation(skydomeProgramID, "cameraPosition");
	GLERROR;


	// OpenGL preferences
	glDisable(GL_CULL_FACE); // culling
	glEnable(GL_DEPTH_TEST); // depth testing
	glEnable(GL_BLEND); // Enable blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 10000 units
	Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 10000.0f);

}

bool initWindow()
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return false;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HIGHT, "Procedural Clouds", NULL, NULL);
	if (window == NULL){
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return false;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	return true;
}

int nbFrames = 0; double lastTime = 0;
void setWindowFPS(GLFWwindow* win)
{
	// Measure speed
	double currentTime = glfwGetTime();
	nbFrames++;

	if (currentTime - lastTime >= 1.0){ // If last cout was more than 1 sec ago
		char title[256];
		title[255] = '\0';
		currentFrameRate = (float)nbFrames / (currentTime - lastTime);
		sprintf(title,
			"Procedural Clouds [FPS: %3.2f], day time: %3.2f, position (%3.1f, %3.1f, %3.1f), #clouds = %d",
			currentFrameRate,
			daytime,
			cameraPosition.x, cameraPosition.y, cameraPosition.z,
			offsets.size());

		glfwSetWindowTitle(win, title);

		nbFrames = 0;
		lastTime += 1.0;
	}
}

float lastCloudCreated = 0;
void updateScene(float passedSeconds, float totalPassedTime)
{
	// move the spheres
	for (size_t i = 0; i < offsets.size(); i++)
	{
		// translate the offset of all spheres
		// - floats at same speed no matter which FPS
		offsets.at(i) += expansionDirection * FLOATING_SPEED_PER_SECOND * passedSeconds;

		// remove if it is out of view (2D cloud offset > radius of cloud plane circle)
		float offsetLength = length(vec2(offsets.at(i).x, offsets.at(i).z)); // y is only hight coordinate
		bool invisible = offsetLength > CLOUD_PLANE_RADIUS * 1.2; // add 20% to avoid removing clouds that have just been spawned
		if (invisible)
		{
			offsets.erase(offsets.begin() + i);
			nonses.erase(nonses.begin() + i);
			scales.erase(scales.begin() + i);
			thresholdNormals.erase(thresholdNormals.begin() + i);
			frontPoints.erase(frontPoints.begin() + i);
			backPoints.erase(backPoints.begin() + i);
			i--;
		}
	}

	// add new spheres
	if (offsets.size() < NUMBER_OF_SPHERES - SPHERES_PER_CLOUD && currentFrameRate > MIN_FRAME_RATE)
	{

		int numberOfAdditionalClouds = (NUMBER_OF_SPHERES - offsets.size()) / SPHERES_PER_CLOUD;
		for (size_t i = 0; i < numberOfAdditionalClouds; i++)
		{
			float randDecision = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			if ((totalPassedTime - lastCloudCreated > MIN_CLOUD_CREATION_DISTANCE_SECONDS && randDecision < SPAWN_CLOUD_LIKELYHOOD)
				|| offsets.size() == 0)
			{
				addCloud(SPHERES_PER_CLOUD);
				lastCloudCreated = totalPassedTime;
			}
		}
	}
}

int main(void)
{
	// setup Window, OpenGL and scene
	if (!initWindow()) return -1;
	sceneSetup();
	openGlSetup();

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, spheres.at(0).vertices.size() * sizeof(vec3), &spheres.at(0).vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &indexbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * spheres.at(0).indices.size(), &spheres.at(0).indices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, spheres.at(0).normals.size() * sizeof(vec3), &spheres.at(0).normals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &textureCoordsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
	glBufferData(GL_ARRAY_BUFFER, spheres.at(0).texcoords.size() * sizeof(vec2), &spheres.at(0).texcoords[0], GL_STATIC_DRAW);

	glGenBuffers(1, &offsetsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, offsetsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * offsets.size(), &offsets[0], GL_STATIC_DRAW);

	glGenBuffers(1, &nonseBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, nonseBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nonses.size(), &nonses[0], GL_STATIC_DRAW);

	glGenBuffers(1, &scaleBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * scales.size(), &scales[0], GL_STATIC_DRAW);

	glGenBuffers(1, &thresholdNormalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, thresholdNormalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * thresholdNormals.size(), &thresholdNormals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &frontPointsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, frontPointsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * frontPoints.size(), &frontPoints[0], GL_STATIC_DRAW);

	glGenBuffers(1, &backPointsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backPointsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * backPoints.size(), &backPoints[0], GL_STATIC_DRAW);

	// ------------------- SKYDOME ----------------------
	glGenBuffers(1, &skydomeVertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, skydomeVertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, skydome.vertices.size() * sizeof(vec3), &skydome.vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &skydomeIndexbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skydomeIndexbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * skydome.indices.size(), &skydome.indices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &skyDomeNormalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, skyDomeNormalbuffer);
	glBufferData(GL_ARRAY_BUFFER, skydome.normals.size() * sizeof(vec3), &skydome.normals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &skydomeTextureCoordsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, skydomeTextureCoordsBuffer);
	glBufferData(GL_ARRAY_BUFFER, skydome.texcoords.size() * sizeof(vec2), &skydome.texcoords[0], GL_STATIC_DRAW);

	double startTime, currentTime;
	float passedTime = 0;
	startTime = (double)clock();

	do{
		// calculate the time
		currentTime = (double)clock();
		float lastFramePassedTime = passedTime;
		passedTime = (currentTime - startTime) / (CLOCKS_PER_SEC);
		float passedTimePerFrame = passedTime - lastFramePassedTime;

		// update
		updateScene(passedTimePerFrame, passedTime); GLERROR;
		updateCamera(passedTimePerFrame); GLERROR;
		updateLight(passedTimePerFrame); GLERROR;
		setWindowFPS(window); GLERROR;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (DRAW_SKYDOME)
		{
			// now use the second program
			glUseProgram(skydomeProgramID);
			GLERROR;

			sendUniformsToSkydomeShader(); GLERROR;

			// 6th attribute buffer : vertices
			glEnableVertexAttribArray(10);
			glBindBuffer(GL_ARRAY_BUFFER, skydomeVertexbuffer);
			glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			GLERROR;

			// 7th attribute buffer : normals
			glEnableVertexAttribArray(11);
			glBindBuffer(GL_ARRAY_BUFFER, skyDomeNormalbuffer);
			glVertexAttribPointer(11, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			// 8th attribute buffer : 2D texture coordinates
			glEnableVertexAttribArray(12);
			glBindBuffer(GL_ARRAY_BUFFER, skydomeTextureCoordsBuffer);
			glVertexAttribPointer(12, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skydomeIndexbuffer);

			glDrawElements(GL_TRIANGLES, skydome.indices.size(), GL_UNSIGNED_INT, NULL);

			glDisableVertexAttribArray(10);
			glDisableVertexAttribArray(11);
			glDisableVertexAttribArray(12);
		}

		if (DRAW_CLOUDS)
		{
			// Use our shader
			glUseProgram(sphereProgramID);
			GLERROR;

			sendUniformsToCloudShader(passedTime); GLERROR;

			// 1rst attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			GLERROR;

			// 2nd attribute buffer : normals
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			GLERROR;

			// 3rd attribute buffer : 2D texture coordinates
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			GLERROR;

			// 4th attribute buffer : 3D offset (instanced for each sphere object)
			glEnableVertexAttribArray(3);
			glBindBuffer(GL_ARRAY_BUFFER, offsetsBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * offsets.size(), &offsets[0], GL_STATIC_DRAW);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glVertexAttribDivisor(3, 1);
			GLERROR;

			// 5th attribute buffer : single random value per sphere (instanced for each sphere object)
			glEnableVertexAttribArray(4);
			glBindBuffer(GL_ARRAY_BUFFER, nonseBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nonses.size(), &nonses[0], GL_STATIC_DRAW);
			glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glVertexAttribDivisor(4, 1);
			GLERROR;

			// 6th attribute buffer : single random value per sphere (instanced for each sphere object)
			glEnableVertexAttribArray(5);
			glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * scales.size(), &scales[0], GL_STATIC_DRAW);
			glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glVertexAttribDivisor(5, 1);
			GLERROR;


			// 7th attribute buffer : single random value per sphere (instanced for each sphere object)
			glEnableVertexAttribArray(6);
			glBindBuffer(GL_ARRAY_BUFFER, thresholdNormalsBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * thresholdNormals.size(), &thresholdNormals[0], GL_STATIC_DRAW);
			glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glVertexAttribDivisor(6, 1);
			GLERROR;

			// 8th attribute buffer : single random value per sphere (instanced for each sphere object)
			glEnableVertexAttribArray(7);
			glBindBuffer(GL_ARRAY_BUFFER, frontPointsBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * frontPoints.size(), &frontPoints[0], GL_STATIC_DRAW);
			glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glVertexAttribDivisor(7, 1);
			GLERROR;

			// 9th attribute buffer : single random value per sphere (instanced for each sphere object)
			glEnableVertexAttribArray(8);
			glBindBuffer(GL_ARRAY_BUFFER, backPointsBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * backPoints.size(), &backPoints[0], GL_STATIC_DRAW);
			glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glVertexAttribDivisor(8, 1);
			GLERROR;

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);

			glDrawElementsInstanced(GL_TRIANGLES, spheres.at(0).indices.size(), GL_UNSIGNED_INT, NULL, offsets.size());
			GLERROR;

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4);
			glDisableVertexAttribArray(5);
			glDisableVertexAttribArray(6);
			glDisableVertexAttribArray(7);
			glDisableVertexAttribArray(8);

		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();


	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &indexbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &textureCoordsBuffer);
	glDeleteBuffers(1, &offsetsBuffer);
	glDeleteBuffers(1, &nonseBuffer);
	glDeleteBuffers(1, &scaleBuffer);
	glDeleteBuffers(1, &skydomeVertexbuffer);
	glDeleteBuffers(1, &skydomeIndexbuffer);
	glDeleteBuffers(1, &skyDomeNormalbuffer);
	glDeleteBuffers(1, &skydomeTextureCoordsBuffer);
	glDeleteBuffers(1, &thresholdNormalsBuffer);
	glDeleteBuffers(1, &frontPointsBuffer);
	glDeleteBuffers(1, &backPointsBuffer);
	glDeleteProgram(sphereProgramID);
	glDeleteProgram(skydomeProgramID);
	glDeleteVertexArrays(1, &VertexArrayID);
	GLERROR;


	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

