#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 modelLance1;
glm::mat4 modelLance2;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
//glm::mat3 lightDirMatrix;

glm::mat4 lightRotation;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

GLint pointLightPositionLoc1;
GLint pointLightColorLoc1;
GLint pointLightPositionLoc2;
GLint pointLightColorLoc2;
//GLuint lightDirMatrixLoc;
//fog
GLint fogDensityLoc;
GLint is_lightLoc;

// camera
//pos -1.00754 m  -2.29122 m  0.707724 m
//trg -1.00754 m  -1.35013 m  0 m
gps::Camera myCamera(
    glm::vec3(-1.00754f, 0.707724f, 2.29122f), //0 0 3
    glm::vec3(-1.00754f, 0.0f, 1.35013f), //0 0 -10
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

double lastX = 400, lastY = 100;
bool firstMouse = true;
float yaw = -90, pitch = 0;
float sensitivity = 0.1f;

float fov = 45.0f;
bool first_render = true;
float startTime;
float currentTime;

GLboolean pressedKeys[1024];

// models
gps::Model3D scene;
gps::Model3D lance1;
gps::Model3D lance2;
gps::Model3D lightCube;

GLfloat angle;
GLfloat light_angle = 0.0f;
GLfloat lance_angle = 0.0f;
GLfloat fogDensity = 0.0f;
GLfloat is_light = 0.0f;


//point light
glm::vec3 pointLightPosition1;
glm::vec3 pointLightColor1;
glm::vec3 pointLightPosition2;
glm::vec3 pointLightColor2;

// shaders
gps::Shader myBasicShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

//shadow
GLuint shadowMapFBO;
GLuint depthMapTexture;
bool showDepthMap;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

//skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;


GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    glfwGetFramebufferSize(window, &width, &height);

    myBasicShader.useShaderProgram();
    projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 1000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glViewport(0, 0, width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;  

    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}
      
	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }
    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_F]) {
        fogDensity += 0.002f;
        if (fogDensity >= 0.3f)
            fogDensity = 0.3f; 
        myBasicShader.useShaderProgram();
        glUniform1fv(fogDensityLoc, 1, &fogDensity);
    }
    if (pressedKeys[GLFW_KEY_G]) {
        fogDensity -= 0.002f;
        if (fogDensity <= 0.0f)
            fogDensity = 0.0f;
        myBasicShader.useShaderProgram();
        glUniform1fv(fogDensityLoc, 1, &fogDensity);
    }

    if (pressedKeys[GLFW_KEY_J]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //solid
    }
    if (pressedKeys[GLFW_KEY_K]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe
    }
    if (pressedKeys[GLFW_KEY_L]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); //polygonal
    }

    if (pressedKeys[GLFW_KEY_O]) {
        is_light = 1.0f;
    }
    if (pressedKeys[GLFW_KEY_P]) {
        is_light = 0.0f;
    }

    if (pressedKeys[GLFW_KEY_N]) {
        light_angle += 2.0f;
    }
    if (pressedKeys[GLFW_KEY_M]) {
        light_angle -= 2.0f;
    }

}

void initOpenGLWindow() {
    myWindow.Create(1324, 768, "Project - Village");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetScrollCallback(myWindow.getWindow(), scroll_callback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    //teapot.LoadModel("models/teapot/teapot20segUT.obj");
    scene.LoadModel("models/scene/scene.obj");
    lance1.LoadModel("models/scene/lance1.obj");
    lance2.LoadModel("models/scene/lance2.obj");
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
    depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
}

void initSkyBox() {
    std::vector<const GLchar*> faces;

    faces.push_back("hills/hills_rt.tga");
    faces.push_back("hills/hills_lf.tga");
    faces.push_back("hills/hills_up.tga");
    faces.push_back("hills/hills_dn.tga");
    faces.push_back("hills/hills_bk.tga");
    faces.push_back("hills/hills_ft.tga");
    
    mySkyBox.Load(faces);
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    modelLance1 = glm::rotate(glm::mat4(1.0f), glm::radians(lance_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    //modelLance1 = glm::rotate(modelLance1, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLance2 = glm::rotate(glm::mat4(1.0f), glm::radians(lance_angle), glm::vec3(0.0f, 1.0f, 0.0f));
    //modelLance2 = glm::rotate(modelLance2, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // light direction matrix
    //lightDirMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 100.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(light_angle), glm::vec3(1.0f, 0.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));
    //glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    fogDensity = 0.0f;
    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    glUniform1fv(fogDensityLoc, 1, &fogDensity);

    is_light = 0.0f;
    is_lightLoc = glGetUniformLocation(myBasicShader.shaderProgram, "is_light");
    glUniform1fv(is_lightLoc, 1, &is_light);

    //////////////point light 1
    pointLightColor1 = glm::vec3(1.0f, 1.0f, 0.0f);
    pointLightColorLoc1 = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor1");
    // send light color to shader
    glUniform3fv(pointLightColorLoc1, 1, glm::value_ptr(pointLightColor1));
    //-5.77464 m
    //0.85487 m
    //2.01812 m
    pointLightPosition1 = glm::vec3(-5.77464f, 2.01812f, -0.85487f);
    pointLightPositionLoc1 = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPosition1");
    // send light color to shader
    glUniform3fv(pointLightPositionLoc1, 1, glm::value_ptr(pointLightPosition1));

    //////////////point light 2
    pointLightColor2 = glm::vec3(1.0f, 1.0f, 0.0f);
    pointLightColorLoc2 = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor2");
    // send light color to shader
    glUniform3fv(pointLightColorLoc2, 1, glm::value_ptr(pointLightColor2));
    //-5.77464 m
    //5.8723 m
    //2.01812 m
    pointLightPosition2 = glm::vec3(-5.77464f, 2.01812f, -5.8723f);
    pointLightPositionLoc2 = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPosition2");
    // send light color to shader
    glUniform3fv(pointLightPositionLoc2, 1, glm::value_ptr(pointLightPosition2));

    //////skybox
    skyboxShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 1000.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection));
    
    //////light
    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO

    // generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);

    // create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
        0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix

    const GLfloat near_plane = 1.0f, far_plane = 10.0f;
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
    glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(light_angle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}*/

void renderSceneObject(gps::Shader shader) {
    shader.useShaderProgram();

    //send scene object model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw scene
    scene.Draw(shader);
}

void renderLance1(gps::Shader shader) {
    shader.useShaderProgram();

    //send lance1 model matrix data to shader
    //modelLance1 = glm::translate(modelLance1, glm::vec3(15.7653f, 6.41739f, -16.9036f));
        //  18.9 m
        //  16.7575 m
        //  6.41205 m
    modelLance1 = glm::translate(glm::mat4(1.0f), glm::vec3(18.9f, 6.41205f, -16.7575f));
    modelLance1 = glm::rotate(modelLance1, glm::radians(lance_angle), glm::vec3(0.0f, 0.0f, 1.0f));
    modelLance1 = glm::translate(modelLance1, glm::vec3(-18.9f, -6.41205f, 16.7575f));
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLance1));

    //send lance1 normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw scene
    lance1.Draw(shader);
}

void renderLance2(gps::Shader shader) {
    shader.useShaderProgram();
    //  14.1481 m
    //  16.6224 m
    //  6.25291 m
    //send lance1 model matrix data to shader
    modelLance2 = glm::translate(glm::mat4(1.0f), glm::vec3(14.1481f, 6.25291f, -16.6224f));
    modelLance2 = glm::rotate(modelLance2, glm::radians(lance_angle), glm::vec3(0.0f, 0.0f, 1.0f));
    modelLance2 = glm::translate(modelLance2, glm::vec3(-14.1481f, -6.25291f, 16.6224f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLance2));

    //send lance1 normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw scene
    lance2.Draw(shader);
}

void do_start_animation(int direction) {
    if(direction == 1)
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    else
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    //update view matrix
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    lance_angle += 0.4f;

    //render objects
    renderSceneObject(myBasicShader);
    renderLance1(myBasicShader);
    renderLance2(myBasicShader);

    //send the maxtrix for directional light
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(light_angle), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    //send is_light for turning on or off the point light
    glUniform1fv(is_lightLoc, 1, &is_light);

    //update de projection matrix for scrolling
    projection = glm::perspective(glm::radians(fov),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 100.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    skyboxShader.useShaderProgram();
    //renderSkyBox
    mySkyBox.Draw(skyboxShader, view, projection);

    //for start position of camera
    if (first_render) {
        mouseCallback(myWindow.getWindow(), 400, 100);
        startTime = static_cast<float>(glfwGetTime());
        first_render = false;
    }

    currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - startTime <= 10.0f) {
        do_start_animation(1);
    }
    if (currentTime - startTime > 11.5f && currentTime - startTime <= 16.0f) {
        do_start_animation(0);
    }

}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
    initFBO();
    initSkyBox();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
