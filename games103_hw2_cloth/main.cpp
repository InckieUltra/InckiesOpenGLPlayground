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

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float dtdt = 0.03f, damp = 0.95f;
vector<glm::vec3> velocity;
glm::vec3 spherePos;
float sphereR = 2.5f;
const int Y_SEGMENTS = 10;
const int X_SEGMENTS = 10;
const GLfloat  PI = 3.14159265358979323846f;
const int numOfVertices = 13;

Camera camera = Camera();
Camera_Movement cameraMovement;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// data
struct node {
	glm::vec3 position;
	glm::vec2 uv;
};
vector<node> vertices;
vector<unsigned int> indices;
vector<int> E;
vector<float> L;

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
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		spherePos += glm::vec3(0, 0, -10)*deltaTime;
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		spherePos += glm::vec3(0, 0, 10)*deltaTime;
	}
	glfwSetCursorPosCallback(window, mouse_callback);
}

int  Quick_Sort_Partition(int *a, int l, int r)
{
	int pivot_0, pivot_1, i, j;
	pivot_0 = a[l * 2 + 0];
	pivot_1 = a[l * 2 + 1];
	i = l;
	j = r + 1;
	while (true)
	{
		do ++i; while (i <= r && (a[i * 2] < pivot_0 || a[i * 2] == pivot_0 && a[i * 2 + 1] <= pivot_1));
		do --j; while (a[j * 2] > pivot_0 || a[j * 2] == pivot_0 && a[j * 2 + 1] > pivot_1);
		if (i >= j)	break;
		swap(a[i * 2], a[j * 2]);
		swap(a[i * 2 + 1], a[j * 2 + 1]);
	}
	swap(a[l * 2 + 0], a[j * 2 + 0]);
	swap(a[l * 2 + 1], a[j * 2 + 1]);
	return j;
}

void Quick_Sort(int *a, int l, int r)
{
	int j;
	if (l < r)
	{
		j = Quick_Sort_Partition(a, l, r);
		Quick_Sort(a, l, j - 1);
		Quick_Sort(a, j + 1, r);
	}
}

void init_cloth() {
	//Resize the mesh.
	int n = numOfVertices;
	for (int j = 0; j < n; j++)
		for (int i = 0; i < n; i++)
		{
			node temp;
			temp.position = glm::vec3(5 - 10.0f*i / (n - 1), 0, 5 - 10.0f*j / (n - 1));
			temp.uv = glm::vec2(i / (n - 1.0f), j / (n - 1.0f));
			vertices.push_back(temp);
		}
	for (int j = 0; j < n - 1; j++)
		for (int i = 0; i < n - 1; i++)
		{
			indices.push_back(j * n + i);
			indices.push_back(j * n + i + 1);
			indices.push_back((j + 1)*n + i + 1);
			indices.push_back(j * n + i);
			indices.push_back((j + 1)*n + i + 1);
			indices.push_back((j + 1)*n + i);
		}

	//Construct the original edge list
	vector<int> _E;
	for (int i = 0; i < indices.size(); i += 3)
	{
		_E.push_back(indices[i + 0]);
		_E.push_back(indices[i + 1]);
		_E.push_back(indices[i + 1]);
		_E.push_back(indices[i + 2]);
		_E.push_back(indices[i + 2]);
		_E.push_back(indices[i + 0]);
	}
	//Reorder the original edge list
	for (int i = 0; i < _E.size(); i += 2)
		if (_E[i] > _E[i + 1])
			swap(_E[i],_E[i+1]);
	//Sort the original edge list using quicksort
	Quick_Sort(&_E[0], 0, _E.size() / 2 - 1);
	
	int e_number = 0;
	for (int i = 0; i < _E.size(); i += 2)
		if (i == 0 || _E[i + 0] != _E[i - 2] || _E[i + 1] != _E[i - 1])
			e_number++;

	for (int i = 0; i < _E.size(); i += 2)
		if (i == 0 || _E[i + 0] != _E[i - 2] || _E[i + 1] != _E[i - 1])
		{
			E.push_back(_E[i + 0]);
			E.push_back(_E[i + 1]);
		}
	
	for (int e = 0; e < E.size() / 2; e++)
	{
		int i = E[e * 2 + 0];
		int j = E[e * 2 + 1];
		L.push_back(glm::length(vertices[i].position - vertices[j].position));
	}
}

void Strain_Limiting(float dt)
{
	vector<glm::vec3> sum_x;
	vector<int> sum_n;
	for (int i = 0; i < vertices.size(); i++) {
		sum_x.push_back(glm::vec3(0, 0, 0));
		sum_n.push_back(0);
	}
	//Apply PBD here.
	for (int e = 0; e < E.size() / 2; e++)
	{
		int i = E[e * 2 + 0];
		int j = E[e * 2 + 1];
		sum_x[i] += (vertices[i].position + vertices[j].position + L[e] * glm::normalize(vertices[i].position - vertices[j].position)) / 2.0f;
		sum_x[j] += (vertices[i].position + vertices[j].position - L[e] * glm::normalize(vertices[i].position - vertices[j].position)) / 2.0f;
		sum_n[i]++;
		sum_n[j]++;
	}
	for (int i = 0; i < vertices.size(); i++) {
		if (i == 0 || i == numOfVertices-1)
			continue;
		velocity[i] += ((0.2f*vertices[i].position + sum_x[i]) / (0.2f + sum_n[i]) - vertices[i].position) / dt;
		vertices[i].position = (0.2f*vertices[i].position + sum_x[i]) / (0.2f + sum_n[i]);
	}
	//...
}

void Collision_Handling(float dt)
{
	//For every vertex, detect collision and apply impulse if needed.
	for (int i = 0; i < vertices.size(); i++) {
		if (i == 0 || i == numOfVertices-1)
			continue;
		glm::vec3 distance = vertices[i].position - spherePos;
		glm::vec3 targetPos = spherePos + sphereR * glm::normalize(distance);
		if (glm::length(distance) <= sphereR) {
			velocity[i] += (targetPos - vertices[i].position) / dt;
			vertices[i].position = targetPos + 0.1f * glm::normalize(distance);
		}
	}
	//...
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
	Shader ourShader("D:\\OpenGLPlayground\\games103_hw2_cloth\\3.3.shader.vs", "D:\\OpenGLPlayground\\games103_hw2_cloth\\3.3.shader.fs"); // you can name your shader files however you like
	Shader anotherShader("D:\\OpenGLPlayground\\games103_hw2_cloth\\3.3.shader.vs", "D:\\OpenGLPlayground\\games103_hw2_cloth\\3.3.bunny.fs.txt");

	//Init
	init_cloth();

	std::vector<float> sphereVertices;
	std::vector<int> sphereIndices;


	// 生成球的顶点
	for (int y = 0; y <= Y_SEGMENTS; y++)
	{
		for (int x = 0; x <= X_SEGMENTS; x++)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI) * sphereR;
			float yPos = std::cos(ySegment * PI) * sphereR;
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI) * sphereR;


			sphereVertices.push_back(xPos);
			sphereVertices.push_back(yPos);
			sphereVertices.push_back(zPos);
			sphereVertices.push_back(0.0f); sphereVertices.push_back(0.0f);
		}
	}

	// 生成球的;三角形面索引
	for (int i = 0; i < Y_SEGMENTS; i++)
	{
		for (int j = 0; j < X_SEGMENTS; j++)
		{

			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);

			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
			sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
			sphereIndices.push_back(i * (X_SEGMENTS + 1) + j + 1);
		}
	}

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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

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
	unsigned char *data = stbi_load("D:\\OpenGLPlayground\\games103_hw2_cloth\\images.jpg", &width, &height, &nrChannels, 0);
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

	for (int i = 0; i < vertices.size(); i++) {
		velocity.push_back(glm::vec3(0, 0, 0));
	}
	glm::vec3 G = glm::vec3(0, -9.8f, 0);
	float dt = 0;
	camera.ProcessKeyboard(BACKWARD, 10);
	camera.ProcessKeyboard(DOWN, 1);
	spherePos = glm::vec3(0, -7.0f, 4.0f);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window,deltaTime);

		dt += deltaTime;
		//if (dt > dtdt) {
			for (int i = 0; i < vertices.size(); i++)
			{
				if (i == 0 || i == numOfVertices-1)	continue;
				//Initial Setup
				velocity[i] += G * dt/2.0f;
				velocity[i] *= damp;
				vertices[i].position += velocity[i] * dt;
				//...
			}

			for (int l = 0; l < 7; l++)
				Strain_Limiting(dt);

			Collision_Handling(dt);

			dt = 0;
		//}

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ourShader.use();
		glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(VAO);

		// camera/view transformation
		ourShader.setMat4("view", camera.GetViewMatrix());

		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(node), &vertices[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glm::mat4 model = glm::mat4(1.0f);
		ourShader.setMat4("model", model);

		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

		anotherShader.use();
		model = glm::translate(model, spherePos);
		anotherShader.setMat4("model", model);
		anotherShader.setMat4("view", camera.GetViewMatrix());
		anotherShader.setMat4("projection", projection);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// 将顶点数据绑定至当前默认的缓冲中
		glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(int), &sphereIndices[0], GL_STATIC_DRAW);
		glBindTexture(GL_TEXTURE_2D, NULL);

		glDrawElements(GL_TRIANGLES, X_SEGMENTS*Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}