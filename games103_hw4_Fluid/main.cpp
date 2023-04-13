#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader_s.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"
#include "model_m.h"
#include <cmath>
#include <thread>

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera = Camera();
Camera_Movement cameraMovement;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// data
const int size = 100;
float rate = 0.005f;
float gamma = 0.004f;
float damping = 0.996f;
vector< vector<float>> 	old_h;
vector< vector<float>>	low_h;
vector< vector<float>>	vh;
vector< vector<float>>	b;

vector< vector<bool>>	cg_mask;
vector< vector<float>>	cg_p;
vector< vector<float>>	cg_r;
vector< vector<float>>	cg_Ap;
bool 	tag = true;

glm::vec3	cube_v = glm::vec3(0.0f);
glm::vec3 	cube_w = glm::vec3(0.0f);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

float lastX = 400, lastY = 300;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // 注意这里是相反的，因为y坐标是从底部往顶部依次增大的
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	camera.ProcessMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow *window,float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	glfwSetCursorPosCallback(window, mouse_callback);
}

void initWaterSurface() {
	Vector3[] X = new Vector3[size*size];

	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
		{
			X[i*size + j].x = i * 0.1f - size * 0.05f;
			X[i*size + j].y = 0;
			X[i*size + j].z = j * 0.1f - size * 0.05f;
		}

	int[] T = new int[(size - 1) * (size - 1) * 6];
	int index = 0;
	for (int i = 0; i < size - 1; i++)
		for (int j = 0; j < size - 1; j++)
		{
			T[index * 6 + 0] = (i + 0)*size + (j + 0);
			T[index * 6 + 1] = (i + 0)*size + (j + 1);
			T[index * 6 + 2] = (i + 1)*size + (j + 1);
			T[index * 6 + 3] = (i + 0)*size + (j + 0);
			T[index * 6 + 4] = (i + 1)*size + (j + 1);
			T[index * 6 + 5] = (i + 1)*size + (j + 0);
			index++;
		}
	mesh.vertices = X;
	mesh.triangles = T;
	mesh.RecalculateNormals();

	low_h = new float[size, size];
	old_h = new float[size, size];
	vh = new float[size, size];
	b = new float[size, size];

	cg_mask = new bool[size, size];
	cg_p = new float[size, size];
	cg_r = new float[size, size];
	cg_Ap = new float[size, size];

	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
		{
			low_h[i, j] = 99999;
			old_h[i, j] = 0;
			vh[i, j] = 0;
		}
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 800, 600);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// build and compile our shader program
	// ------------------------------------
	Shader ourShader("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw2_cloth\\3.3.shader.vs", "D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw3_FEM\\3.3.bunny.fs.txt"); // you can name your shader files however you like
	Shader anotherShader("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw2_cloth\\3.3.shader.vs", "D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw3_FEM\\3.3.shader.fs");

	//Init
	initWaterSurface();

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(node), &vertices[0], GL_DYNAMIC_DRAW);

	// 配置EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), &triangles[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	stbi_set_flip_vertically_on_load(true);
	//texture
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// 为当前绑定的纹理对象设置环绕、过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// 加载并生成纹理
	int width, height, nrChannels;
	unsigned char *data = stbi_load("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw2_cloth\\images.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	ourShader.use();
	ourShader.setInt("ourTexture", 0);

	//Z buffer
	glEnable(GL_DEPTH_TEST);

	//隐藏鼠标
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	float dtt = 0;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window,deltaTime);

		//update
		


		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw floor
		anotherShader.use();
		glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(VAO);
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		anotherShader.setMat4("view", camera.GetViewMatrix());
		anotherShader.setMat4("projection", projection);
		glm::mat4 model = glm::mat4(1.0f);
		anotherShader.setMat4("model", model);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, gV.size() * sizeof(node), &gV[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

		// draw house
		ourShader.use();
		ourShader.setMat4("view", camera.GetViewMatrix());
		ourShader.setMat4("projection", projection);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(node), &vertices[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), &triangles[0], GL_STATIC_DRAW);

		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -30.0f));
		ourShader.setMat4("model", model);

		glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}