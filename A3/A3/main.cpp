/*
 *	Includes
 */
#include <assimp/cimport.h>		// C importer
#include <assimp/scene.h>		// Collects data
#include <assimp/postprocess.h> // Various extra operations
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iomanip>				// setprecision
#include <iostream>
#include <math.h>
#include <mmsystem.h>
#include <sstream>
#include <stdio.h>
#include <vector>				// STL dynamic memory
#include <windows.h>

#include "Antons_maths_funcs.h" // Anton's maths functions
#include "Camera.h"
#include "Distance.h"
#include "Mesh.h"
#include "RigidBody.h"
#include "Shader_Functions.h"
#include "text.h"
#include "time.h"

using namespace std;

/*
 *	Globally defined variables and constants
 */
#define BUFFER_OFFSET(i) ((char *)NULL + (i))  // Macro for indexing vertex buffer

#define NUM_MESHES   2
#define NUM_SHADERS	 3
#define NUM_TEXTURES 1

bool firstMouse = true;
bool keys[1024];
Camera camera(vec3(2.0f, 2.0f, 2.0f));
enum Meshes { CUBE_MESH, POINT_MESH };
enum Shaders { SKYBOX, PARTICLE_SHADER, BASIC_TEXTURE_SHADER };
enum Textures { OBJECT_TEXTURE };
GLfloat cameraSpeed = 0.005f;
GLfloat deltaTime = 1.0f / 60.0f;
GLfloat lastX = 400, lastY = 300;
GLuint shaderProgramID[NUM_SHADERS];
int screenWidth = 1000;
int screenHeight = 800;
int stringIDs[3];
Mesh lineMesh, pointMesh, triangleMesh;
vec3 closestPoint = vec3(0.0f, 0.0f, 0.0f);
vec3 point1 = vec3(-0.5f, -0.5f, 0.0f);
vec3 point2 = vec3(0.5f, -0.5f, 0.0f);
vec3 point3 = vec3(0.0f, 0.5f, 0.0f);
vec3 point0 = vec3(0.5f, 0.5f, 0.0f);

// | Resource Locations
const char * meshFiles[NUM_MESHES] = { "../Meshes/cube.dae", "../Meshes/particle_reduced.dae" };
const char * skyboxTextureFiles[6] = { "../Textures/DSposx.png", "../Textures/DSnegx.png", "../Textures/DSposy.png", "../Textures/DSnegy.png", "../Textures/DSposz.png", "../Textures/DSnegz.png"};
const char * textureFiles[NUM_TEXTURES] = { "../Textures/plane.jpg" };

const char * vertexShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxVertexShader.txt", "../Shaders/ParticleVertexShader.txt", "../Shaders/BasicTextureVertexShader.txt" };
const char * fragmentShaderNames[NUM_SHADERS] = { "../Shaders/SkyboxFragmentShader.txt", "../Shaders/ParticleFragmentShader.txt", "../Shaders/BasicTextureFragmentShader.txt" };

string frf(const float &f)
{
	ostringstream ss;
	ss << setfill(' ') << std::setw(6) << fixed << setprecision(3) << f;
	string s(ss.str());
	return s;
}

void draw_text()
{
	ostringstream distanceOSS, pointOSS, closestOSS;
	string distanceString, pointString, closestString;
	//distanceOSS << "Distance: " << fixed << setprecision(3) << rigidBody.mass;
	pointOSS << "Point Location ( " << fixed << setprecision(3) << point0.v[0] << ", " << point0.v[1] << ", " << point0.v[2] << " )";
	closestOSS << "Closest Point ( " << fixed << setprecision(3) << closestPoint.v[0] << ", " << closestPoint.v[1] << ", " << closestPoint.v[2] << " )";
	
	//distanceString = distanceOSS.str();
	pointString = pointOSS.str();
	closestString = closestOSS.str();
	
	//update_text(stringIDs[0], distanceString.c_str());
	update_text(stringIDs[1], pointString.c_str());
	update_text(stringIDs[2], closestString.c_str());

	draw_texts();
}

void init_text()
{
	stringIDs[0] = add_text("Distance: ", -0.95f, 0.95f, 20.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	stringIDs[1] = add_text("Point Location (,,)", -0.95f, 0.9f, 20.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	stringIDs[2] = add_text("Closest Point (,,)", -0.95f, 0.85f, 20.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void display() 
{
	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST);	// Enable depth-testing
	glDepthFunc(GL_LESS);		// Depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f/255.0f, 0.0f/255.0f, 0.0f/255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw skybox first
	mat4 view = look_at(camera.Position, vec3(0.0f, 0.0f, 0.0f), camera.Up); //camera.GetViewMatrix();
	mat4 projection = perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
	mat4 triangle_model = identity_mat4();
	vec4 triangle_colour = vec4(1.0f, 0.2f, 0.2f, 0.8f);

	triangleMesh.drawMesh(view, projection, triangle_model, triangle_colour);

	GLfloat line_vertices[] = {
		point0.v[0], point0.v[1], point0.v[2],
		closestPoint.v[0], closestPoint.v[1], closestPoint.v[2]
	};

	lineMesh = Mesh(&shaderProgramID[PARTICLE_SHADER]);
	lineMesh.generateObjectBufferMesh(line_vertices, 2);

	mat4 line_model = identity_mat4();
	vec4 line_colour = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	lineMesh.drawLine(view, projection, line_model, line_colour);
	
	mat4 point_model = identity_mat4();
	point_model = scale(point_model, vec3(0.02f, 0.02f, 0.02f));
	point_model = translate(point_model, point0);
	vec4 point_colour = vec4(0.2f, 1.0f, 0.2f, 0.8f);
	pointMesh.drawMesh(view, projection, point_model, point_colour);

	mat4 closest_model = identity_mat4();
	closest_model = scale(closest_model, vec3(0.02f, 0.02f, 0.02f));
	closest_model = translate(closest_model, closestPoint);
	vec4 closest_colour = vec4(0.2f, 0.2f, 1.0f, 0.8f);
	pointMesh.drawMesh(view, projection, closest_model, closest_colour);

	//skyboxMesh.drawSkybox(view, projection);

	//mat4 objectModel = identity_mat4();
	//objectModel = rigidBody.rotation * objectModel;
	//objectModel = translate(objectModel, rigidBody.position);

	//objectMesh.drawMesh(view, projection, objectModel);

	

	draw_text();
	
	glutSwapBuffers();
}

void processInput()
{
	if (keys[GLUT_KEY_UP])
		camera.ProcessKeyboard(FORWARD, cameraSpeed);
	if(keys[GLUT_KEY_DOWN])
		camera.ProcessKeyboard(BACKWARD, cameraSpeed);
	if (keys[GLUT_KEY_LEFT])
		camera.ProcessKeyboard(LEFT, cameraSpeed);
	if (keys[GLUT_KEY_RIGHT])
		camera.ProcessKeyboard(RIGHT, cameraSpeed);
	if (keys['q'])
		point0 += vec3(-0.001f, 0.0f, 0.0f);
	if (keys['w'])
		point0 += vec3(0.001f, 0.0f, 0.0f);
	if (keys['a'])
		point0 += vec3(0.0f, -0.001f, 0.0f);
	if (keys['s'])
		point0 += vec3(0.0f, 0.001f, 0.0f);
	if (keys['z'])
		point0 += vec3(0.0f, 0.0f, -0.001f);
	if (keys['x'])
		point0 += vec3(0.0f, 0.0f, 0.001f);
	if (keys[(char)27])
		exit(0);
}

void updateScene()
{
	processInput();
	closestPoint = closestPointOnTriangleVoronoi(point0, point1, point2, point3);
	//updateRigidBody();
	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	if (!init_text_rendering("../Textures/freemono.png", "../Textures/freemono.meta", screenWidth, screenHeight))
	{
		fprintf(stderr, "ERROR init text rendering\n");
		exit(1);
	}
	init_text();

	// Compile the shaders
	for (int i = 0; i < NUM_SHADERS; i++)
	{
		shaderProgramID[i] = CompileShaders(vertexShaderNames[i], fragmentShaderNames[i]);
	}

	//skyboxMesh = Mesh(&shaderProgramID[SKYBOX]);
	//skyboxMesh.setupSkybox(skyboxTextureFiles);

	//objectMesh = Mesh(&shaderProgramID[BASIC_TEXTURE_SHADER]);
	//objectMesh.generateObjectBufferMesh(meshFiles[OBJECT_MESH]);
	//objectMesh.loadTexture(textureFiles[OBJECT_TEXTURE]);

	//rigidBody = RigidBody(objectMesh.vertex_count, objectMesh.vertex_positions);

	GLfloat pointVertex[] = {
		0.0f, 0.0f, 0.0f
	};

	GLfloat triangleVertices[] = {
		point1.v[0], point1.v[1], point1.v[2],
		point2.v[0], point2.v[1], point2.v[2],
		point3.v[0], point3.v[1], point3.v[2]
	};

	triangleMesh = Mesh(&shaderProgramID[PARTICLE_SHADER]);
	triangleMesh.generateObjectBufferMesh(triangleVertices, 3);

	pointMesh = Mesh(&shaderProgramID[PARTICLE_SHADER]);
	pointMesh.generateObjectBufferMesh(meshFiles[POINT_MESH]);
	//pointMesh.generateObjectBufferMesh(pointVertex, 1);
}

/*
 *	User Input Functions
 */
#pragma region USER_INPUT_FUNCTIONS
void pressNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = true;
	
}

void releaseNormalKeys(unsigned char key, int x, int y)
{
	keys[key] = false;
}

void pressSpecialKeys(int key, int x, int y)
{
	keys[key] = true;
}

void releaseSpecialKeys(int key, int x, int y)
{
	keys[key] = false;
}

void mouseClick(int button, int state, int x, int y)
{}

void processMouse(int x, int y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	GLfloat xoffset = x - lastX;
	GLfloat yoffset = lastY - y;

	lastX = x;
	lastY = y;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void mouseWheel(int button, int dir, int x, int y)
{}
#pragma endregion

/*
 *	Main
 */
int main(int argc, char** argv) 
{
	srand(time(NULL));

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - screenWidth) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - screenHeight) / 4);
	glutCreateWindow("Distance and Contact");

	// Glut display and update functions
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);

	// User input functions
	glutKeyboardFunc(pressNormalKeys);
	glutKeyboardUpFunc(releaseNormalKeys);
	glutSpecialFunc(pressSpecialKeys);
	glutSpecialUpFunc(releaseSpecialKeys);
	glutMouseFunc(mouseClick);
	glutPassiveMotionFunc(processMouse);
	glutMouseWheelFunc(mouseWheel);


	glewExperimental = GL_TRUE; //for non-lab machines, this line gives better modern GL support
	
	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	// Set up meshes and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}