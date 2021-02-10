/*
*	Final Project (Wooden Spoon)
*	12/13/19
*   Jeremy Kansas
*/


#include <iostream>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL2/SOIL2.h>

using namespace std;



/*
*	FUNCTION DECLARATIONS
*/

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mode);

static GLuint CompileShader(const string& source, GLuint shaderType);
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader);

void initiateCamera();
void TransformCamera();



/*
 *	GLOBAL VARIABLE DECLARATIONS/INITIALIZATIONS
 */

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;
GLfloat fov = 45.0f;

// Define Camera Attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f); // Move 3 units back in z towards screen
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f); // What the camera points to
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target); // direction z
glm::vec3 worldUp = glm::vec3(0.0, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));// right vector x
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight)); // up vector y
glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // 1 unit away from lense

// Pitch and Yaw
GLfloat radius = 3.0f, rawYaw = 0.0f, rawPitch = 0.0f, degYaw, degPitch;

// light source positions and colors
glm::vec3 lightPosition(1.0f, 1.0f, 0.0f);
glm::vec3 lightPosition2(0.0f, -1.0f, 0.0f);
glm::vec3 lightColor(1.0f, 1.0f, 0.0f);
glm::vec3 lightColor2(0.2f, 0.01f, 0.1f);

// user input related
bool keys[1024], mouseButtons[3]; // Boolean array for key and mouse button states
bool isPanning = false, isOrbiting = false; // camera movement state
bool perspective = true;
GLfloat lastX, lastY, xChange, yChange; 
bool firstMouseMove = true; // detect first mouse movement because there will be no lastX/lastY then



/*
*		MAIN FUNCTION
*/

int main(void)
{

	/*
	 *	Initializations - GLEW, GLFW, camera, callbacks, etc.
	 */

	int width = 640, height = 480;
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Main Window", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	initiateCamera();

	// Set input callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
		cout << "Error!" << endl;

	glEnable(GL_DEPTH_TEST);



	/*
	 *	Define primitives with arrays - vertices, indices, etc.
	 */

	GLfloat lampVertices[] = {
		-0.5, -0.5, 0.0,
		-0.5, 0.5, 0.0,
		0.5, -0.5, 0.0,
		0.5, 0.5, 0.0
	};

	// vertices for square shape (planes used to make spoon handle)
	GLfloat vertices[] = {

		// Triangle 1
		-0.5, -0.5, 0.0, // index 0
		0.0, 0.0, // UV (bl)
		0.0f, 0.0f, 1.0f, // normal pos Z

		-0.5, 0.5, 0.0, // index 1
		0.0, 1.0, // UV (br)
		0.0f, 0.0f, 1.0f, // normal pos Z

		0.5, -0.5, 0.0,  // index 2	
		1.0, 0.0, // UV (tl)
		0.0f, 0.0f, 1.0f, // normal pos Z

		// Triangle 2	
		0.5, 0.5, 0.0,  // index 3	
		1.0, 1.0, // UV (tr)
		0.0f, 0.0f, 1.0f, // normal pos Z
	};

	// indices for above square
	GLubyte indices[] = {
		0, 1, 2,
		1, 2, 3
	};

	// seperate set of vertices for *isoceles* triangle shape (for spoon head)
	GLfloat triangleVertices[] = {

		-0.5, -0.5, 0.0, // index 0
		0.0, 0.0, // UV (bl)
		0.0f, 0.0f, 1.0f, // normal ?

		-0.5, 0.5, 0.0, // index 1
		0.0, 1.0, // UV (br)
		0.0f, 0.0f, 1.0f, // normal ?

		0.5, 0.0, 0.0,  // index 2	
		1.0, 0.0, // UV (top)
		0.0f, 0.0f, 1.0f, // normal?  
	};

	GLubyte triangleIndices[] = {
		0, 1, 2
	};

	// Plane Transforms (translate and rotate) for creating cube out of squares 
	glm::vec3 planePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.5f),
		glm::vec3(0.5f,  0.0f,  0.0f),
		glm::vec3(0.0f,  0.0f,  -0.5f),
		glm::vec3(-0.5f, 0.0f,  0.0f),
		glm::vec3(0.0f, 0.5f,  0.0f),
		glm::vec3(0.0f, -0.5f,  0.0f)
	};
	glm::float32 planeRotations[] = {
		0.0f, 90.0f, 180.0f, -90.0f, -90.f, 90.f
	};

	
	/*
	 *	Generate buffer objects and bind to VAOs
	 */

	GLuint squareVBO, squareEBO, squareVAO, triangleVBO, triangleEBO, triangleVAO;
	GLuint lampVBO, lampEBO, lampVAO;

	glGenBuffers(1, &squareVBO); // Create VBO
	glGenBuffers(1, &squareEBO); // Create EBO
	glGenBuffers(1, &triangleVBO); // Create VBO
	glGenBuffers(1, &triangleEBO); // Create EBO
	glGenBuffers(1, &lampVBO); // Create VBO
	glGenBuffers(1, &lampEBO); // Create EBO

	glGenVertexArrays(1, &squareVAO); // Create VOA
	glGenVertexArrays(1, &triangleVAO); // Create VOA
	glGenVertexArrays(1, &lampVAO); // Create VOA

	// square plane
	glBindVertexArray(squareVAO);

		glBindBuffer(GL_ARRAY_BUFFER, squareVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareEBO); // Select EBO
	
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

		 // Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Unbind VAO


	// triangle plane
	glBindVertexArray(triangleVAO);

		glBindBuffer(GL_ARRAY_BUFFER, triangleVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleEBO); // Select EBO

		glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangleIndices), triangleIndices, GL_STATIC_DRAW); // Load indices 

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

	glBindVertexArray(0);


	// lamp (light source cue)
	glBindVertexArray(lampVAO);

		glBindBuffer(GL_ARRAY_BUFFER, lampVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO); // Select EBO

		glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // Load indices 

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

	glBindVertexArray(0);


	// Load texture map and generate textures
	int woodTexWidth, woodTexHeight;
	unsigned char* woodImage = SOIL_load_image("wood.jpg", &woodTexWidth, &woodTexHeight, 0, SOIL_LOAD_RGB); // load image
	GLuint woodTexture;
	glGenTextures(1, &woodTexture);// Generate texture id
	glBindTexture(GL_TEXTURE_2D, woodTexture); // Activate texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, woodTexWidth, woodTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, woodImage); // Generate texture
	glGenerateMipmap(GL_TEXTURE_2D); // Texture resolution managment
	SOIL_free_image_data(woodImage); // Free image from memory
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind or close texture object

	
	/*
	 *	Shader source code
	 */
	
	// Vertex shader for spoon
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"layout(location = 1) in vec2 texCoord;"
		"layout(location = 2) in vec3 normal;"
		"out vec3 oColor;"
		"out vec2 oTexCoord;"
		"out vec3 oNormal;"
		"out vec3 fragPos;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"oTexCoord = texCoord;"
		"oNormal = mat3(transpose(inverse(model))) * normal;"
		"fragPos = vec3(model * vec4(vPosition, 1.0f));"
		"}\n";

	// Fragment shader for spoon
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec2 oTexCoord;"
		"in vec3 oNormal;"
		"in vec3 fragPos;"
		"out vec4 fragColor;"
		"uniform sampler2D myTexture;"
		"uniform vec3 objectColor;"
		"uniform vec3 lightColor;"
		"uniform vec3 lightPos;"
		"uniform vec3 lightColor2;"
		"uniform vec3 lightPos2;"
		"uniform vec3 viewPos;"
		"void main()\n"
		"{\n"
		"// ambient \n"
		"float ambientStrength = 0.2f;"
		"vec3 ambient = ambientStrength * lightColor;"
		"vec3 ambient2 = ambientStrength * lightColor2;"
		"// diffuse \n"
		"vec3 norm = normalize(oNormal);"
		"vec3 lightDir = normalize(lightPos - fragPos);"
		"vec3 lightDir2 = normalize(lightPos2 - fragPos);"
		"float diff = max(dot(norm, lightDir), 0.0);"
		"float diff2 = max(dot(norm, lightDir2), 0.0);"
		"vec3 diffuse = diff * lightColor;"
		"vec3 diffuse2 = diff2 * lightColor2;"
		"// specularity \n"
		"float specularStrength = 5.5f;"
		"float specularStrength2 = 1.5f;"
		"vec3 viewDir = normalize(viewPos - fragPos);"
		"vec3 reflectDir = reflect(-lightDir, norm);"
		"vec3 reflectDir2 = reflect(-lightDir2, norm);"
		"float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);"
		"float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), 32);"
		"vec3 specular = specularStrength * spec * lightColor;"
		"vec3 specular2 = specularStrength2 * spec2 * lightColor2;"
		"// lighting result\n"
		"vec3 result = (ambient + diffuse + specular + ambient2 + diffuse2 + specular2) * objectColor;"
		"fragColor = texture(myTexture, oTexCoord) * vec4(result, 1.0f);"
		"}\n";

	// vertex shader for lamp
	string lampVertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"}\n";

	// fragment shader for lamp
	string lampFragmentShaderSource =
		"#version 330 core\n"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"fragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);"
		"}\n";

	// Create Shader Programs
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
	GLuint lampShaderProgram = CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource);

		
	/*
	 *	MAIN LOOP
	 */

	while (!glfwWindowShouldClose(window))
	{
		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear screen

		// Define lookAt and projection matrices
		viewMatrix = glm::lookAt(cameraPosition, target, worldUp);

		if (perspective)
			projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
		else
			projectionMatrix = glm::ortho(-2.0, 2.0, -2.0, 2.0, -10.0, 10.0);
		
		
		// Get matrix's uniform location and set matrix
		GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

		// get light and object color, and light and view position location
		GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
		GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
		GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
		GLint lightColorLoc2 = glGetUniformLocation(shaderProgram, "lightColor2");
		GLint lightPosLoc2 = glGetUniformLocation(shaderProgram, "lightPos2");
		GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
		
		glUseProgram(shaderProgram); // set shader program to use

		// assign light and object colors
		glUniform3f(objectColorLoc, 0.46f, 0.46f, 0.25f);  // (color of wood texture roughly?) 
		
		// setup lights
		glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z);
		glUniform3f(lightColorLoc, lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(lightPosLoc2, lightPosition2.x, lightPosition2.y, lightPosition2.z);
		glUniform3f(lightColorLoc2, lightColor2.x, lightColor2.y, lightColor2.z); 
		
		// specify view pos (for specular lighting)
		glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glBindTexture(GL_TEXTURE_2D, woodTexture); // Apply wood texture (Auto detected by Uniform Sampler)
		
		
		/*
		 *	Draw Spoon Handle
		 */

		glBindVertexArray(squareVAO);

		GLfloat handleWidth = .2, handleHeight = .1, handleLength = 2.1;
		glm::vec3 handleTransform = glm::vec3(glm::vec3(handleWidth, handleHeight, handleLength));

		// Transform planes to form rect solid
		for (GLuint i = 0; i < 6; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions[i] * handleTransform);
			modelMatrix = glm::scale(modelMatrix, handleTransform);
			modelMatrix = glm::rotate(modelMatrix, glm::radians(planeRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
			if (i >= 4)
				modelMatrix = glm::rotate(modelMatrix, glm::radians(planeRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f));
						
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
		}

		glBindVertexArray(0);
		
		
		/*
		 *	Draw spoon head
		 */
		
		// calculate points, angles, etc around the spoon head
		//(N refers to # of sides in the N-gon representing the circle)
		const int N = 20;
		const GLfloat anglePerN = glm::radians(360.0 / N);
		const GLfloat interiorAngle = (N - 2) * glm::radians(180.0 / N);
		const GLfloat inradius = 0.5f;
		const float lengthPerSide = 2 * inradius * tanf(glm::radians(180.0 / N));

		GLfloat rotations[N];
		glm::vec3 translations[N];

		// calculate rotation and translation for each wedge piece of the spoon
		for (int i = 0; i < N; i++)
		{
			GLfloat x, z;
			x = cosf(anglePerN * i) * inradius/2.0;
			z = sinf(anglePerN * i) * inradius/2.0;
			translations[i] = glm::vec3(x, 0.0f, z);
			rotations[i] = i * anglePerN + glm::pi<float>();
		}
					
		// location of the center point of the head
		glm::vec3 headCenter = glm::vec3(0, 0.0f, handleLength / 2.0 + inradius);

		// scale value for wedge triangles
		glm::vec3 scale = glm::vec3(inradius + .03, lengthPerSide + 0.03, 1.0);

		GLfloat concavityAngle = glm::radians(25.0f); // 15

		// vertical offset to fix triangle position after rotating for spoon head concavity
		GLfloat concavityOffset = sinf(concavityAngle) * inradius / 2.0f;

		// loop to draw each triangular wedge of the spoon head N-gon
		for (int i = 0; i < N; i++)
		{
			// outside plane
			glBindVertexArray(squareVAO);
										
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, headCenter + translations[i] * glm::vec3(1.98, 0.0, 1.98));
				
				modelMatrix = glm::rotate(modelMatrix, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
				
				// fix so every other side doesn't have its normal in wrong direction
				if (i%2==1) modelMatrix = glm::rotate(modelMatrix, glm::radians(180.f), glm::vec3(0.0f, 1.0f, 0.0f));

				modelMatrix = glm::rotate(modelMatrix, interiorAngle * i, glm::vec3(0.0f, 1.0f, 0.0f)); 
				
				modelMatrix = glm::scale(modelMatrix, glm::vec3(lengthPerSide, handleHeight + 0.02, 1.1f));

				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

			glBindVertexArray(0);

			// top and bottom triangles
			glBindVertexArray(triangleVAO);

				// top
				modelMatrix = glm::mat4(1.0f); // reset to identity matrix
				
				modelMatrix = glm::translate(modelMatrix, headCenter + translations[i]);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, handleHeight / 2.0 - concavityOffset, 0.f));

				modelMatrix = glm::rotate(modelMatrix, glm::radians(90.f) + glm::radians(180.f), glm::vec3(1.0f, 0.0f, 0.0f)); // flip to horizontal
				modelMatrix = glm::rotate(modelMatrix, rotations[i] , glm::vec3(0.0f, 0.0f, -1.0f)); // point at center
				modelMatrix = glm::rotate(modelMatrix, concavityAngle, glm::vec3(0.0f, 1.0f, 0.0f)); // angle down for spoon concavity
				
				modelMatrix = glm::scale(modelMatrix, scale);

				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, nullptr);

				// bottom
				modelMatrix = glm::mat4(1.0); //reset to identity matrix
				
				modelMatrix = glm::translate(modelMatrix, headCenter + translations[i]);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, -handleHeight/2.0 - concavityOffset - 0.02, 0.f));

				modelMatrix = glm::rotate(modelMatrix, glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)); // flip to horizontal
				modelMatrix = glm::rotate(modelMatrix, rotations[i], glm::vec3(0.0f, 0.0f, 1.0f)); // point at center
				modelMatrix = glm::rotate(modelMatrix, -concavityAngle, glm::vec3(0.0f, 1.0f, 0.0f)); // angle down for spoon concavity

				modelMatrix = glm::scale(modelMatrix, scale);

				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, nullptr);

			glBindVertexArray(0); 
		}

		glUseProgram(0);

		
		/*
		 *	Draw Light Source Cue
		 */
		
		glUseProgram(lampShaderProgram);
		glBindVertexArray(lampVAO);

			GLint lampModelLoc = glGetUniformLocation(lampShaderProgram, "model");
			GLint lampViewLoc = glGetUniformLocation(lampShaderProgram, "view");
			GLint lampProjLoc = glGetUniformLocation(lampShaderProgram, "projection");
			glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
			glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
			
			// Transform planes to form light cube
			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] / glm::vec3(8.0f) + lightPosition);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(planeRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(.125));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, glm::radians(planeRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f));
			
				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
			}

			for (GLuint i = 0; i < 6; i++)
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, planePositions[i] / glm::vec3(8.0f) + lightPosition2);
				modelMatrix = glm::rotate(modelMatrix, glm::radians(planeRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(.125));
				if (i >= 4)
					modelMatrix = glm::rotate(modelMatrix, glm::radians(planeRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f));

				glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
			}

		glBindVertexArray(0);
		glUseProgram(0);


		/* Swap front and back buffers */
		glfwSwapBuffers(window);
		/* Poll for and process events */
		glfwPollEvents();
		// Poll Camera Transformations
		TransformCamera();
	}

	
	//Clear GPU resources
	glDeleteVertexArrays(1, &squareVAO);
	glDeleteBuffers(1, &squareVBO);
	glDeleteBuffers(1, &squareEBO);
	glDeleteVertexArrays(1, &triangleVAO);
	glDeleteBuffers(1, &triangleVBO);
	glDeleteBuffers(1, &triangleEBO);
	
	glfwTerminate();
	return 0;
}


// Define input functions
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// Close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Assign true to Element ASCII if key pressed
	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE) // Assign false to Element ASCII if key released
		keys[key] = false;

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

	// Clamp FOV
	if (fov >= 1.0f && fov <= 55.0f)
		fov -= yoffset * 0.01;

	// Default FOV
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 55.0f)
		fov = 55.0f;

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

	if (firstMouseMove)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}
	// Calculate mouse offset (Easing effect)
	xChange = xpos - lastX;
	yChange = lastY - ypos; // Inverted cam

							// Get current mouse (always starts at 0)
	lastX = xpos;
	lastY = ypos;


	if (isOrbiting)
	{
		// Update raw yaw and pitch with mouse movement
		rawYaw += xChange;
		rawPitch += yChange;

		// Conver yaw and pitch to degrees, and clamp pitch
		degYaw = glm::radians(rawYaw);
		degPitch = glm::clamp(glm::radians(rawPitch), -glm::pi<float>() / 2.f + .1f, glm::pi<float>() / 2.f - .1f);

		// Azimuth Altitude formula
		cameraPosition.x = target.x + radius * cosf(degPitch) * sinf(degYaw);
		cameraPosition.y = target.y + radius * sinf(degPitch);
		cameraPosition.z = target.z + radius * cosf(degPitch) * cosf(degYaw);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mode)
{
	// Assign boolean state to element Button code
	if (action == GLFW_PRESS)
		mouseButtons[button] = true;
	else if (action == GLFW_RELEASE)
		mouseButtons[button] = false;
}


// Update Camera Transform State Based On User Input
void TransformCamera()
{

	// Orbit camera
	if (mouseButtons[GLFW_MOUSE_BUTTON_LEFT])
		isOrbiting = true;
	else
		isOrbiting = false;

	// Focus camera
	if (keys[GLFW_KEY_F])
		initiateCamera();

	// switch projection (perspective vs orthographic)
	if (keys[GLFW_KEY_P])
		perspective = true;
	if (keys[GLFW_KEY_O])
		perspective = false;
}

// Define Initial Camera Position and Orientation
void initiateCamera()
{	// Define Camera Attributes
	cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f); // Move 3 units back in z towards screen
	target = glm::vec3(0.0f, 0.0f, 0.0f); // What the camera points to
	cameraDirection = glm::normalize(cameraPosition - target); // direction z
	worldUp = glm::vec3(0.0, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));// right vector x
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight)); // up vector y
	CameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // 1 unit away from lense
}


// Create and Compile Shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	// Create Shader object
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	// Attach source code to Shader object
	glShaderSource(shaderID, 1, &src, nullptr);

	// Compile Shader
	glCompileShader(shaderID);

	// Return ID of Compiled shader
	return shaderID;

}

// Create Program Object
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	// Create program object
	GLuint shaderProgram = glCreateProgram();

	// Attach vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	// Link shaders to create executable
	glLinkProgram(shaderProgram);

	// Delete compiled vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	// Return Shader Program
	return shaderProgram;

}