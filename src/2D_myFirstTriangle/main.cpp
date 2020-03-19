#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if __cplusplus < 201103L
#pragma message("WARNING: the compiler may not be C++11 compliant. __cplusplus version is : " STRING(__cplusplus))
#endif

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <chrono>
#include <sstream>
#include <cmath>

#ifdef _DEBUG
#include <windows.h> //This is for debug output only
#endif

#include <GL/glew.h>
#include <SDL.h>
#include <glm\glm.hpp>

#include "GameObject.h"
#include "MeshGenerator.h"
#include "Camera.h"

using namespace std;
using namespace std::chrono;
using glm::vec2; //vec3 and mat4 are included in Transform so they're included here too

const GLsizei WIN_DIM_X = 1400;
const GLsizei WIN_DIM_Y = 900;
std::string exeName;
SDL_Window *win; //pointer to the SDL_Window
SDL_GLContext context; //the SDL_GLContext
int frameCount = 0;
std::string frameLine = "";
bool done = false;

Camera cam(vec3(0.0,0.0,-0.8));
int camMode = 1; //Can be 1, 2 or 3. Changed by pressing '1', '2' or '3' on the keyboard
mat4 proj = glm::perspective(61.2f, WIN_DIM_X / (float)WIN_DIM_Y, 0.1f, 10.f);
GLfloat light[] = { 0, 1, 1, 1 }; //Directional light (like a sun)

GLuint theProgram;

//In attribute locations
GLint positionLocation; 
GLint colorLocation;
GLint normalLocation;

//Uniform locations
GLint modelMatLocation;
GLint modelAllRotationsLocation;
GLint viewMatLocation;
GLint projectionMatLocation;
GLint lightLocation;

//It's more optimised to have all of your data in one buffer
GLuint bufferID; //Holds data like: {obj1.verts, obj1.indices, obj2.verts, obj2.indices, ...}

#pragma region Game Related Variables

//Game
//These four constants can be tweaked to change the gameplay
const int FIRST_TO = 5; //First to ... Wins!
bool hasWon = false;
bool spacePressedToReset = false;
//Used to make the game flash when won
int colorChanger = 0;
bool colorOne = true;

//Players
bool isPlayerIGoingUp[2] = { false, false };
bool isPlayerIGoingDown[2] = { false, false };
const int PLAYERS_UP_KEY[2] = { SDLK_w, SDLK_UP };
const int PLAYERS_DOWN_KEY[2] = { SDLK_s, SDLK_DOWN };
const float PLAYER_SPEED = 0.007f;
char scores[2] = { 0, 0 };
const vec3 NO_SCORE_COLOR = vec3(1, 1, 1) * 0.4f;
const float PADDLE_HEIGHT = 0.1f;

//Ball
glm::vec2 ballVel; //Velocity
float ballRotSpeed = 4.f; //Can't be a const as the value is stored as is, but it's also used to store the direction, ie ballRotSpeed *= -1 is used.
const float ballInitSpeed = 0.004f;
const float ballDeltaSpeed = 0.028f; //This is a currently an exponential increase to the speed
float ballTimeSinceSpawn = 0.f;
const float BALL_FREEZE_TIME = 250.f;

//Bounds
const float BOUND_TOP = 0.9f;
const float BOUND_BOTTOM = -BOUND_TOP;
const float BOUND_LEFT = -0.975f;
const float BOUND_RIGHT = -BOUND_LEFT;
const float HORIZONTAL_BOUND_THICKNESS = 0.5f;  //Top & Bottom Thickness as a scaler of 0.1 sqaure
const float VERTICAL_BOUND_THICKNESS = 0.125f; //Left & Right Thickness as a scaler of 0.1 sqaure

//Entities Indicies - Not using enum class to get implicit conversion to int
class EEntityIndex {
public:
	enum {
		MiddleLine = 0,
		Player1 = 1,
		Player2 = 2,
		Ball = 3,
		TopBoarder = 4,
		BottomBoarder = 5,
		LeftBoarder = 6,
		RightBoarder = 7,
		Floor = 8,
		Rabbit = 9,
		ScoreStartPlayer1 = 10,
		//Indicies past this depend on FIRST_TO const
	};
};

//Create Entities
const float DEFAULT_Z_POS = 0.2f; //A default value that a lot of objects use for their z pos
GameObject entities[EEntityIndex::ScoreStartPlayer1 + FIRST_TO * 4] = {
	GameObject(MeshGenerator::MakeCube(vec3(0.5, 0.5, 0.5)), Transformation(vec3(0,   0,   DEFAULT_Z_POS), vec3(.1, 10 - HORIZONTAL_BOUND_THICKNESS * 2, .1))), //Middle Line

	GameObject(MeshGenerator::MakeCube(vec3(1,0,0)), Transformation(vec3(-.85, 0, 0.2), vec3(.2, 1.2, .2))), //Player 1
	GameObject(MeshGenerator::MakeCube(vec3(0,0,1)), Transformation(vec3(.85,  0, 0.2), vec3(.2, 1.2, .2))), //Player 2
	GameObject(MeshGenerator::MakeHueSphere(3), Transformation(vec3(0, 0, DEFAULT_Z_POS), vec3(0.4, 0.4, .4), vec3(0, 0, 5.f))), //Ball

	GameObject(MeshGenerator::MakeCube(vec3(0.0, 0.0, 0.0)), Transformation(vec3(0,  BOUND_TOP + HORIZONTAL_BOUND_THICKNESS / 10.f,   DEFAULT_Z_POS), vec3(10, HORIZONTAL_BOUND_THICKNESS, .1))), //Top 
	GameObject(MeshGenerator::MakeCube(vec3(0.0, 0.0, 0.0)), Transformation(vec3(0, BOUND_BOTTOM - HORIZONTAL_BOUND_THICKNESS / 10.f, DEFAULT_Z_POS), vec3(10, HORIZONTAL_BOUND_THICKNESS, .1))), //Bottom
	GameObject(MeshGenerator::MakeCube(vec3(0.0, 0.0, 0.0)), Transformation(vec3(BOUND_LEFT - VERTICAL_BOUND_THICKNESS / 10.f,  0,  DEFAULT_Z_POS), vec3(VERTICAL_BOUND_THICKNESS, 10, .1))), //Left
	GameObject(MeshGenerator::MakeCube(vec3(0.0, 0.0, 0.0)), Transformation(vec3(BOUND_RIGHT + VERTICAL_BOUND_THICKNESS / 10.f, 0,  DEFAULT_Z_POS), vec3(VERTICAL_BOUND_THICKNESS, 10, .1))), //Right
	GameObject(MeshGenerator::MakeCube(vec3(0,0.4,0)), Transformation(vec3(0, 0, 0.4), vec3(15, 15, 0.2))), //Floor
	GameObject() //Rabbit init in main

	//Triangle Score markers will be generated here
};
int numEntities = sizeof(entities) / sizeof(*entities);

#pragma endregion




void Initialise() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}
	cout << "SDL initialised OK!\n";
}
void CreateNewWindow() {
	//get executable name, and use as window title
	int beginIdxWindows = exeName.rfind("\\"); //find last occurrence of a backslash
	int beginIdxLinux = exeName.rfind("/"); //find last occurrence of a forward slash
	int beginIdx = max(beginIdxWindows, beginIdxLinux);
	std::string exeNameEnd = exeName.substr(beginIdx + 1);
	const char* exeNameCStr = exeNameEnd.c_str();

	//create window
	win = SDL_CreateWindow(exeNameCStr, 500, 100, (int)WIN_DIM_X, (int)WIN_DIM_Y, SDL_WINDOW_OPENGL); //same height and width makes the window square ...

	//error handling
	if (win == nullptr){
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "SDL CreatedWindow OK!\n";
}
void SetGLAttributes() {
	const int MAJOR = 3;
	const int MINOR = 1;
	cout << "Built for OpenGL Version " << MAJOR << "." << MINOR << endl;
																		 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, MAJOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, MINOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //core profile
	cout << "Set OpenGL context to versicreate remote branchon " << MAJOR << "." << MINOR << " OK!\n";
}
void CreateContext() {
	SetGLAttributes();

	context = SDL_GL_CreateContext(win);
	if (context == nullptr) {
		SDL_DestroyWindow(win);
		std::cout << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		SDL_Quit();

		exit(1);
	}
	cout << "Created OpenGL context OK!\n";
}
void InitGlew() {
	GLenum rev;
	glewExperimental = GL_TRUE;
	rev = glewInit();
	if (GLEW_OK != rev) {
		std::cout << "GLEW Error: " << glewGetErrorString(rev) << std::endl;
		SDL_Quit();
		exit(1);
	}
	else
		cout << "GLEW Init OK!\n";
}

string ReadInShaderCode(const char* fileName) {
	ifstream meInput(fileName);
	if (!meInput.good()) {
		cout << "File failed to load..." << fileName;
		exit(1);
	}
	return string(
		istreambuf_iterator<char>(meInput),
		istreambuf_iterator<char>());
}
GLuint CreateShader(GLenum eShaderType, const std::string &strShaderFile) {
	GLuint shader = glCreateShader(eShaderType);
	//error check
	const char* strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE){
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (eShaderType){
			case GL_VERTEX_SHADER:   strShaderType = "vertex";   break;
			case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
			case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return shader;
}
GLuint CreateProgram(const std::vector<GLuint> &shaderList) {
	GLuint program = glCreateProgram();

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(program, shaderList[iLoop]);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE){
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	return program;
}
void InitializeProgram() {
	std::vector<GLuint> shaderList;
	string shaderCode = ReadInShaderCode("VertexShader.glsl");
	shaderList.push_back(CreateShader(GL_VERTEX_SHADER, shaderCode.c_str()));
	shaderCode = ReadInShaderCode("FragmentShader.glsl");
	shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, shaderCode.c_str()));

	theProgram = CreateProgram(shaderList);
	if (theProgram == 0) {
		cout << "GLSL program creation error." << std::endl;
		SDL_Quit();
		exit(1);
	}
	else
		cout << "GLSL program creation OK! GLUint is: " << theProgram << std::endl;

	//Ins
	positionLocation = glGetAttribLocation(theProgram, "position");
	colorLocation = glGetAttribLocation(theProgram, "colorIn");
	normalLocation = glGetAttribLocation(theProgram, "normal");
	
	//Uniforms
	modelMatLocation = glGetUniformLocation(theProgram, "model");
	modelAllRotationsLocation = glGetUniformLocation(theProgram, "rotationMat");
	viewMatLocation = glGetUniformLocation(theProgram, "view");
	projectionMatLocation = glGetUniformLocation(theProgram, "proj");
	lightLocation = glGetUniformLocation(theProgram, "light");

	//Clean up shaders
	for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
}

void InitializeVertexBuffer() {
	glGenBuffers(1, &bufferID);
	glBindBuffer(GL_ARRAY_BUFFER, bufferID);

	int totalBufferSize = 0;
	for (int i = 0; i < numEntities; i++)
		totalBufferSize += entities[i].m.VertexAndIndexBufferSize();
	glBufferData(GL_ARRAY_BUFFER, totalBufferSize, 0, GL_STATIC_DRAW);

	int currentOffset = 0;
	for (int i = 0; i < numEntities; i++) {
		glBufferSubData(GL_ARRAY_BUFFER, currentOffset, entities[i].m.VertexBufferSize(), entities[i].m.verts);
		currentOffset += entities[i].m.VertexBufferSize();
		glBufferSubData(GL_ARRAY_BUFFER, currentOffset, entities[i].m.IndexBufferSize(), entities[i].m.indices);
		currentOffset += entities[i].m.IndexBufferSize();
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0); //Clean up
	cout << "bufferID created OK! GLUint is: " << bufferID << std::endl;
}
void LoadAssets() {
	InitializeProgram(); //create GLSL Shaders, link into a GLSL program, and get IDs of attributes and variables
	InitializeVertexBuffer(); //load data into a vertex buffer
	cout << "Loaded Assets OK!\n";
}
void SetupVertexArrayObject() {
	int currentOffset = 0;
	for (int i = 0; i < numEntities; i++) {
		glGenVertexArrays(1, &entities[i].m.vertexArray); //create a Vertex Array Object
		glBindVertexArray(entities[i].m.vertexArray); //make the just created vertexArrayObject the active one
			glBindBuffer(GL_ARRAY_BUFFER, bufferID); //bind vertexDataBufferObject
			glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation
			glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)currentOffset); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
			glEnableVertexAttribArray(colorLocation);
			glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(currentOffset + sizeof(vec3)));
			glEnableVertexAttribArray(normalLocation);
			glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(currentOffset + sizeof(vec3)*2));
			currentOffset += entities[i].m.VertexAndIndexBufferSize();
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
	}
	cout << "entities[i].m.vertexArray[0 to " << numEntities-1 << "] were created correctly!" << endl;

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it
	//cleanup
	glDisableVertexAttribArray(positionLocation); //disable vertex attribute at index positionLocation
	glDisableVertexAttribArray(colorLocation);
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer
}

void ResetBall() {
	entities[EEntityIndex::Ball].transform.position = vec3(0,0, DEFAULT_Z_POS);
	srand((unsigned int)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() / 50); //Set up the random seed
	
	float randomAngleDeg;
	
	const float UP = 90.f;
	const float DOWN = 270.f;
	const float ANGLE_TO_AVOID = 20.f; //Avoid from 90 and 270. 
	const int PRECISION = 10000;

	//Pick left or right side
	if ((rand() % 2) == 0)
	{
		//If ANGLE_TO_AVOID == 20. Range: 290 to 70(+360)
		const float RANGE_START = DOWN + ANGLE_TO_AVOID;
		const float RANGE_END = 360 + UP - ANGLE_TO_AVOID;
		const float RANGE_SIZE = RANGE_END - RANGE_START;
		randomAngleDeg = float(int(RANGE_START + RANGE_SIZE * (rand() % PRECISION) / (float)PRECISION) % 360);
	}
	else
	{
		//If ANGLE_TO_AVOID == 20. Range: 110 to 250
		const float RANGE_START = UP + ANGLE_TO_AVOID;
		const float RANGE_END = DOWN - ANGLE_TO_AVOID;
		const float RANGE_SIZE = RANGE_END - RANGE_START;
		randomAngleDeg = float(RANGE_START + RANGE_SIZE * (rand() % PRECISION) / (float)PRECISION);
	}

	float randomAngleRad = glm::radians(randomAngleDeg);
	ballVel = vec2(cos(randomAngleRad), sin(randomAngleRad)) * ballInitSpeed;
	ballTimeSinceSpawn = 0; //Start it frozen again
}
void ResetGame() {
	scores[0] = scores[1] = 0; //Reset

	//Reset the scoring trianlges
	glBindBuffer(GL_ARRAY_BUFFER, bufferID);

	int offset = 0; //Offset into the buffer
	for (int i = 0; i < EEntityIndex::ScoreStartPlayer1; i++) //Loop through all of the data before the triangles to get the offset
		offset += entities[i].m.VertexAndIndexBufferSize(); 

	for (int i = EEntityIndex::ScoreStartPlayer1; i < EEntityIndex::ScoreStartPlayer1 + FIRST_TO*4; i++) { //Loop through all of the triangle score markers
		entities[i].transform.rotation.z = 0; //Reset the rotation
		for (int j = 0; j <= 2; j++)
			entities[i].m.verts[j].col = NO_SCORE_COLOR;
		glBufferSubData(GL_ARRAY_BUFFER, offset, entities[i].m.VertexBufferSize(), entities[i].m.verts); //Update the colour back to grey
		offset += entities[i].m.VertexAndIndexBufferSize(); //Iterate to the next offset position
	}

	//Reset the player's position:
	for (int i = 0; i <= 1; i++) 
		entities[EEntityIndex::Player1+i].transform.position.y = 0;

	ResetBall();
}
void InitGameLogic() {
	//Setup camera
	cam.pos.y = -1;
	cam.LookAt(0.f, 0.f, 0.2f);

	//Place the triangle score counters in the correct place
	for (int i = 0; i < FIRST_TO * 4; i++) {
		entities[i + EEntityIndex::ScoreStartPlayer1] = GameObject(
			MeshGenerator::MakeTriangle(NO_SCORE_COLOR),
			Transformation(
				vec3((-0.950 + (i % (FIRST_TO * 2) / 2)*0.075)*(i >= (FIRST_TO * 2) ? -1 : 1), 
					  +0.95 * (i % 2 == 0 ? 1 : -1), 
					  DEFAULT_Z_POS), 
				vec3(0.3, 0.3, 1)
			)
		);
		entities[i + EEntityIndex::ScoreStartPlayer1].isGUI = true;
	}

	ResetBall();
}

void CheckPlayerKeyPresses(SDL_Keycode k, bool isDown) {
	//isDown is if it's a keydown event, rather than a key up event
	for (int i = 0; i < 2; i++) {
		if (k == PLAYERS_UP_KEY[i])
			isPlayerIGoingDown[i] = isDown;
		if (k == PLAYERS_DOWN_KEY[i])
			isPlayerIGoingUp[i] = isDown;
	}
}
void CheckBallPaddleCollision(GameObject paddle, bool left) {
	GameObject ball = entities[EEntityIndex::Ball]; //Shorter to type

	//Create some aliases for readability (free compared to storing in variables which isn't)
	#define Scale			0.1f
	#define BallLeft		ball.transform.Left(Scale)
	#define BallRight		ball.transform.Right(Scale)
	#define BallTop			ball.transform.Top(Scale)
	#define BallBottom		ball.transform.Bottom(Scale)
	#define BallY			ball.transform.position.y //Ball center in y-axis
	#define PaddleRight		paddle.transform.Right(Scale)
	#define PaddleLeft		paddle.transform.Left(Scale)
	#define PaddleTop		paddle.transform.Top(Scale)
	#define PaddleBottom	paddle.transform.Bottom(Scale)
	#define PaddleY			paddle.transform.position.y //Paddle center in y-axis

	//Check for collision (bounds check)
	if (BallLeft < PaddleRight && BallRight > PaddleLeft&& BallBottom < PaddleTop && BallTop > PaddleBottom) {

		//If not already going in the correct direction / if not already collided
		if ((left && ballVel.x < 0) || (!left && ballVel.x > 0)) {
			ballVel.x *= -1; //Flip the direction
			ballRotSpeed *= -1;
			ballVel *= 1 + ballDeltaSpeed;

			//This is Pong Physics, NOT real physics!
			//Did it hit the top half?
			bool hitTopHalf = BallY > PaddleY;

			//Percent along the stick, if in the middle, then =0, else at the ends, =1, enterpolated between
			//if hitTopHalf: 
			//    = (BallCenter-PaddleCenter)/(PaddleTop-PaddleCenter)
			//else 
			//    = (BallCenter-PaddleCenter)/(PaddleBottom-PaddleCenter)
			float percentAlong = (BallY - PaddleY) / ((hitTopHalf ? PaddleTop : PaddleBottom) - PaddleY);

			//True to pong tradition, if it hits the end, it goes off at 45 degrees
			float stickAngleDeg = glm::clamp(45 * percentAlong, 0.f, 45.f); 

			float outAngleRad;
			if (left)
				outAngleRad = glm::radians(0 + (hitTopHalf ? stickAngleDeg : -stickAngleDeg));
			else
				outAngleRad = glm::radians(180 + (hitTopHalf ? -stickAngleDeg : stickAngleDeg));
			ballVel = vec2(cos(outAngleRad), sin(outAngleRad)) * glm::length(ballVel);
		}
	}
}
void PlayerWins(char playerID) {
	cout << "\n\nPlayer " << (playerID == 0 ? "red" : "blue") << " wins!!!!!!!!!!!!!!!!\n\n" << endl;
	cout << " >>>> Press SPACE to reset the game! <<<< \n" << endl;
	hasWon = true;
}
void PointScored(char playerID) {
	scores[playerID] += 1;
	cout << "\nPlayer " << (playerID == 0 ? "red" : "blue") << " scored! The scores are red: " << 
		(int)scores[0] << " blue: " << (int)scores[1] << endl;

	glBindBuffer(GL_ARRAY_BUFFER, bufferID);

	//Visually update the score:
	for (int i = -2; i <= -1; i++) { //Do top and bottom one
		int id = EEntityIndex::ScoreStartPlayer1 + scores[playerID] * 2 + i + 2 * FIRST_TO * playerID;
		entities[id].transform.rotation.z = 180; //Flip the triangle
		vec3 col = entities[EEntityIndex::Player1 + playerID].m.verts[0].col; //Set the point to the colour of the player's top-left vertex

		for (int j = 0; j <= 2; j++) { //Loop through each vertex in the triangle
			entities[id].m.verts[j].col = col; //Update the colour
			//Update the buffer with the new colour
			int offset = 0;
			for (int k = 0; k < id; k++)
				offset += entities[k].m.VertexAndIndexBufferSize();
			glBufferSubData(GL_ARRAY_BUFFER, offset, entities[id].m.VertexBufferSize(), entities[id].m.verts);
		}
	}

	if (scores[playerID] == FIRST_TO) {
		PlayerWins(playerID);
		return;
	}

	//Reset the game:
	ResetBall();
}

void HandleInput() {
	int x, y;
	SDL_GetMouseState(&x, &y);
	float theta = glm::radians(180.f * (x / (float)WIN_DIM_X)); //Ranges between Pi(180deg) and 2Pi(360deg)
	switch (camMode) {
	//Side View
	case 1:
		if (entities[EEntityIndex::Rabbit].transform.position.y > 0) //If I moved it for option3, move it back
			entities[EEntityIndex::Rabbit].transform.position.y *= -1;
		cam.pos.x = cos(theta) * 1.2f;
		cam.pos.y = sin(theta) * 1.2f;
		cam.pos.z = -0.8f + 0.8f * (y / (float)WIN_DIM_Y);
		break;
	//Top Down View
	case 2:
		if (entities[EEntityIndex::Rabbit].transform.position.y > 0) //If I moved it for option3, move it back
			entities[EEntityIndex::Rabbit].transform.position.y *= -1;
		cam.pos.x = cos(theta) * 0.1f;
		cam.pos.y = sin(theta) * 0.1f;
		cam.pos.z = -1.2f;
		break;
	//Rabbit View
	case 3:
		//The rabbit has to be moved so that it's not weird for the player's controls else they'd be controlling it from a different angle
		if (entities[EEntityIndex::Rabbit].transform.position.y < 0) //Move for option 3
			entities[EEntityIndex::Rabbit].transform.position.y *= -1;
		cam.pos = entities[EEntityIndex::Rabbit].transform.position;
		cam.pos.z -= .7f;
		cam.pos.y -= 0.2f;
		break;
	}

	//Angle light according to camera
	light[0] = cam.pos.x / 0.5f;

	SDL_Event event; //somewhere to store an event

	while (SDL_PollEvent(&event)) { //loop until SDL_PollEvent returns 0 (meaning no more events)
		switch (event.type){
		case SDL_QUIT:
			done = true;
			break;
		case SDL_KEYDOWN:
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			if (!event.key.repeat) {
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					done = true; break;
				case SDLK_SPACE: 
					if (hasWon) {
						spacePressedToReset = true;
						cout << "\n\nResetting the Game!\n\n";
					}
					break;
				case SDLK_1: camMode = 1; break;
				case SDLK_2: camMode = 2; break;
				case SDLK_3: camMode = 3; break;
				}
				CheckPlayerKeyPresses(event.key.keysym.sym, true);
			}
			break;
		case SDL_KEYUP:
			if (!event.key.repeat)
				CheckPlayerKeyPresses(event.key.keysym.sym, false);
			break;
		}
	}
}
void UpdateSimulation(double simLength) { //update simulation with an amount of time to simulate for (in seconds)
	if (hasWon) {
		if (spacePressedToReset) {
			hasWon = false;
			spacePressedToReset = false;
			ResetGame();
		} else 
			return;
	}

	//Rotate the model to look at the ball
	vec2 modelToBall(entities[EEntityIndex::Ball].transform.position - entities[EEntityIndex::Rabbit].transform.position);
	float modelToBallTheta = glm::atan(modelToBall.y / modelToBall.x);
	const float ANGLE_RABBIT_FORWARD = 220.f;
	entities[EEntityIndex::Rabbit].transform.rotation.z = ANGLE_RABBIT_FORWARD + glm::degrees(modelToBallTheta) + (modelToBall.x >= 0 ? 0 : 180);
	//If on cam mode 3 track the ball
	if (camMode == 3)
		cam.LookAt(entities[EEntityIndex::Ball].transform.position);
	else
		cam.LookAt(0.f, 0.f, 0.2f);


	//Make the ball Freeze for a bit before the game starts:
	simLength /= 3; //Makes for nicer numbers to use

	//Move the ball
	if (ballTimeSinceSpawn != -1) //If the ball isn't already in play
		ballTimeSinceSpawn += (float)simLength; //Accumulate the time in play
	if (ballTimeSinceSpawn > BALL_FREEZE_TIME) //If the ball has been frozen enough, play:
		ballTimeSinceSpawn = -1; //Say that the ball is in play
	if (ballTimeSinceSpawn == -1) {  //If the ball is in play:
		entities[EEntityIndex::Ball].transform.position += vec3(ballVel, 0) * (float)simLength;
		entities[EEntityIndex::Ball].transform.rotation.z += ballRotSpeed;
	}


	//Move the players
	for (int i = 0; i < 2; i++) { //For each player
		//Move the player
		if (isPlayerIGoingUp[i] && entities[EEntityIndex::Player1+i].transform.Top(PADDLE_HEIGHT) < BOUND_TOP)
			entities[EEntityIndex::Player1+i].transform.position.y += PLAYER_SPEED * (float)simLength;
		if (isPlayerIGoingDown[i] && entities[EEntityIndex::Player1+i].transform.Bottom(PADDLE_HEIGHT) > BOUND_BOTTOM)
			entities[EEntityIndex::Player1+i].transform.position.y -= PLAYER_SPEED * (float)simLength;
	}
	
	//Check if hit the ceiling or floor
	if ((entities[EEntityIndex::Ball].transform.Top(PADDLE_HEIGHT) > BOUND_TOP && ballVel.y > 0) || 
		(entities[EEntityIndex::Ball].transform.Bottom(PADDLE_HEIGHT) < BOUND_BOTTOM && ballVel.y < 0))
	{
		ballVel.y *= -1;
	}

	//Bound the ball off the paddle
	CheckBallPaddleCollision(entities[EEntityIndex::Player1], true);
	CheckBallPaddleCollision(entities[EEntityIndex::Player2], false);

	//Goal scored on Left, +1 to Blue
	if (entities[EEntityIndex::Ball].transform.position.x < -1)
		PointScored(1);
	if (entities[EEntityIndex::Ball].transform.position.x > +1)
		PointScored(0);
}

void PreRender() {
	glViewport(0, 0, WIN_DIM_X, WIN_DIM_Y); //set viewpoint
	if (hasWon) {
		if (colorChanger++ > 150) { 
			colorOne = !colorOne;
			colorChanger = 0;
		}
	}
	else
		colorOne = true;

	if (colorOne)
		glClearColor(0.2f, 0.0f, 0.4f, 1.0f); 
	else {
		vec3 c;
		if (scores[0] == FIRST_TO) //Red Won
			c = entities[EEntityIndex::Player1].m.verts[0].col;
		else //Blue Won
			c = entities[EEntityIndex::Player2].m.verts[0].col;
		c *= 0.6f; //Make it a darker colour than the player
		glClearColor(c.r, c.g, c.b, 1);
	}
		
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the window (technical the scissor box bounds)
}
void Render() {
	glUseProgram(theProgram); //installs the program object specified by program as part of current rendering state
	int indexStartOffset = 0;

	for (int i = 0; i < numEntities; i++) {
		glBindVertexArray(entities[i].m.vertexArray);
			glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, &entities[i].transform.GetTransformMatrix()[0][0]);
			mat4 rotMat = entities[i].transform.GetRotationMatrix();
			glUniformMatrix4fv(modelAllRotationsLocation, 1, GL_FALSE, &rotMat[0][0]);
			glUniform4fv(lightLocation, 1, light);
			if (!entities[i].isGUI) { //3D Geometry
				glUniformMatrix4fv(viewMatLocation, 1, GL_FALSE, &cam.GetViewMatrix()[0][0]);
				glUniformMatrix4fv(projectionMatLocation, 1, GL_FALSE, &proj[0][0]);
			} else { //If 2D GUI, Apply Identity matrices to both (dont wrap data)
				glUniformMatrix4fv(viewMatLocation, 1, GL_FALSE, &mat4()[0][0]);
				glUniformMatrix4fv(projectionMatLocation, 1, GL_FALSE, &mat4()[0][0]);
			}
	
			indexStartOffset += entities[i].m.VertexBufferSize();
			if (entities[i].shouldRender)
				glDrawElements(GL_TRIANGLES, entities[i].m.numIndices, GL_UNSIGNED_SHORT, (void*)indexStartOffset);
			indexStartOffset += entities[i].m.IndexBufferSize();
	}
	glBindVertexArray(0);
	glUseProgram(0); //clean up
}
void PostRender() {
	SDL_GL_SwapWindow(win);; //present the frame buffer to the display (swapBuffers)
	frameLine += "Frame: " + std::to_string(frameCount++);
	cout << "\r" << frameLine << std::flush;
	frameLine = "";
}

void CleanUp() {
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	cout << "Cleaning up OK!\n";
}

int main(int argc, char* args[]) {
	//Get exe name and directory
	exeName = args[0];
	string dir;
	int dirC = exeName.length() - 1;
	while (exeName[dirC] != '\\')
		dirC--;
	for (int i = 0; i <= dirC; i++)
		dir += exeName[i];

	//Setup
	Initialise();
	CreateNewWindow();
	CreateContext();
	InitGlew();

	//Get mesh from given directory
	entities[EEntityIndex::Rabbit] = GameObject(MeshGenerator::LoadObj((dir + string("bunny.obj")).c_str(), vec3(0.6, 0.25, 0.05)), 
												Transformation(vec3(0.0, -1.2, 0.0), vec3(0.2, 0.2, 0.2), vec3(-90, 0, -90)));

	glViewport(0, 0, WIN_DIM_X, WIN_DIM_Y); //should check what the actual window res is?

	//Culling to make it run more efficiently
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	SDL_GL_SwapWindow(win); //force a swap, to make the trace clearer

	InitGameLogic();
	LoadAssets(); //- create shaders - load vertex data
	SetupVertexArrayObject();

	cout << "\nGame Launched!\n\n";

	milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	milliseconds oldTimeMS = ms;
	while (!done) { //loop until done flag is set)
		long long deltaTime = ms.count() - oldTimeMS.count();
		cout << "   delta time: " << deltaTime << "ms";
		oldTimeMS = ms;

		HandleInput();
		UpdateSimulation((double)deltaTime); // http://headerphile.blogspot.co.uk/2014/07/part-9-no-more-delays.html

		PreRender();
		Render();
		PostRender();

		ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
	}

	//Keep window open
	int a;
	cin >> a;

	//cleanup and exit
	CleanUp();
	SDL_Quit();

	return 0;
}
