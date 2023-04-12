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
float dt = 0.003f;
float mass = 1;
float stiffness_0 = 2000.0f;
float stiffness_1 = 500.0f;
float damp = 0.999f;
glm::vec3 G = glm::vec3(0, -9.8f, 0);

vector<int>	Tet;
int tet_number;			//The number of tetrahedra

vector<glm::vec3> 	Force;
vector<glm::vec3> 	V;
vector<glm::vec3> 	X;
vector<float> A_ref;
int number;				//The number of vertices

vector<glm::mat4> inv_Dm;

//For Laplacian smoothing.
vector<glm::vec3>   V_sum;
vector<int>		V_num;

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
vector<unsigned int> triangles;
vector<glm::vec3> velocity;

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

glm::mat4 buildEdgeMatrix(int tet) {
	glm::vec3 x0 = X[Tet[tet * 4 + 0]],
		x1 = X[Tet[tet * 4 + 1]],
		x2 = X[Tet[tet * 4 + 2]],
		x3 = X[Tet[tet * 4 + 3]];

	glm::mat4 res = glm::mat4{
		x1.x - x0.x, x2.x - x0.x, x3.x - x0.x, 0,
		x1.y - x0.y, x2.y - x0.y, x3.y - x0.y, 0,
		x1.z - x0.z, x2.z - x0.z, x3.z - x0.z, 0,
		0, 0, 0, 1
	};

	return res;
}

void initHouse() {
	// FILO IO: Read the house model from files.
		// The model is from Jonathan Schewchuk's Stellar lib.
	{
		ifstream infile;
		infile.open("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw3_FEM\\house2.ele");

		int temp;
		infile >> temp;
		tet_number = temp;
		infile >> temp; infile >> temp; infile >> temp;

		for (int i = 0; i < tet_number; i++) {
			infile >> temp; Tet.push_back(temp - 1);
			infile >> temp; Tet.push_back(temp - 1);
			infile >> temp; Tet.push_back(temp - 1);
			infile >> temp; Tet.push_back(temp - 1);
			infile >> temp;
		}

		infile.close();
	}
	{
		ifstream infile;
		infile.open("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw3_FEM\\house2.node");

		int temp;
		infile >> temp;
		number = temp;
		infile >> temp; infile >> temp; infile >> temp; infile >> temp;
		float tempx, tempy, tempz;

		for (int i = 0; i < number; i++)
		{
			infile >> tempx >> tempy >> tempz;

			X.push_back(glm::vec3(tempx, tempy, tempz));

			infile >> tempx; infile >> tempx;
		}

		infile.close();

		//Centralize the model.
		glm::vec3 center = glm::vec3(0.0f);
		for (int i = 0; i < number; i++)		center += X[i];
		center = center / (float)number;
		for (int i = 0; i < number; i++)
		{
			X[i] -= center;
			float temp = X[i].y;
			X[i].y = X[i].z;
			X[i].z = temp;
		}
	}
	/*tet_number=1;
	Tet = new int[tet_number*4];
	Tet[0]=0;
	Tet[1]=1;
	Tet[2]=2;
	Tet[3]=3;

	number=4;
	X = new Vector3[number];
	V = new Vector3[number];
	Force = new Vector3[number];
	X[0]= new Vector3(0, 0, 0);
	X[1]= new Vector3(1, 0, 0);
	X[2]= new Vector3(0, 1, 0);
	X[3]= new Vector3(0, 0, 1);*/


	//Create triangle mesh.
	node temp; glm::vec3 tempos;
	temp.uv = glm::vec2(0.0f);
	glm::vec3 zeroVelocity = glm::vec3(0.0f);
	for (int i = 0; i < number; i++) {
		velocity.push_back(zeroVelocity);
	}
	for (int tet = 0; tet < tet_number; tet++)
	{
		temp.position = X[Tet[tet * 4 + 0]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 2]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 1]];
		vertices.push_back(temp);

		temp.position = X[Tet[tet * 4 + 0]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 3]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 2]];
		vertices.push_back(temp);

		temp.position = X[Tet[tet * 4 + 0]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 1]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 3]];
		vertices.push_back(temp);

		temp.position = X[Tet[tet * 4 + 1]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 2]];
		vertices.push_back(temp);
		temp.position = X[Tet[tet * 4 + 3]];
		vertices.push_back(temp);
	}

	for (int t = 0; t < tet_number * 4; t++)
	{
		triangles.push_back(t * 3 + 0);
		triangles.push_back(t * 3 + 1);
		triangles.push_back(t * 3 + 2);
	}

	//TODO: Need to allocate and assign inv_Dm
	for (int i = 0; i < tet_number; i++) {
		inv_Dm.push_back(glm::inverse(buildEdgeMatrix(i)));
	}
}

vector<glm::vec3> force;

void applyElasticForce(int t) {
	for (int tet = t; tet < t+1000 && tet < tet_number; tet++) {
		//TODO: Deformation Gradient
		glm::mat4 F = buildEdgeMatrix(tet);
		F = F * inv_Dm[tet];
		//TODO: Green Strain
		glm::mat4 G = 0.5f * (glm::transpose(F)*F - glm::mat4(1.0f));
		//TODO: Second PK Stress
		glm::mat4 S = 2 * stiffness_1 * G + stiffness_0 * (G[0][0] + G[1][1] + G[2][2])*glm::mat4(1.0f);
		//TODO: Elastic Force
		glm::mat4 tempf = -1.0f / (glm::determinant(inv_Dm[tet]) * 6) * F * S * glm::transpose(inv_Dm[tet]);
		force[Tet[tet * 4 + 1]] += glm::vec3(tempf[0][0], tempf[1][0], tempf[2][0]);
		force[Tet[tet * 4 + 2]] += glm::vec3(tempf[0][1], tempf[1][1], tempf[2][1]);
		force[Tet[tet * 4 + 3]] += glm::vec3(tempf[0][2], tempf[1][2], tempf[2][2]);
		force[Tet[tet * 4 + 0]] -= glm::vec3(tempf[0][0], tempf[1][0], tempf[2][0])
			+ glm::vec3(tempf[0][1], tempf[1][1], tempf[2][1])
			+ glm::vec3(tempf[0][2], tempf[1][2], tempf[2][2]);
	}
}

void _Update(GLFWwindow *window, float dT) {
	// Jump up.
	if (glfwGetKey(window, GLFW_KEY_1))
	{
		for (int i = 0; i < number; i++) {
			velocity[i].y += 0.2f;
		}
	}

	force.clear();
	for (int i = 0; i < number; i++) {
		velocity[i] *= 0.998f;
		force.push_back(G);
	}

	for (int tet = 0; tet < tet_number; tet+=1000)
	{
		thread workwork(applyElasticForce, tet);
		workwork.join();
	}

	for (int i = 0; i < number; i++) {
		velocity[i] += force[i] * dT;
		X[i] += velocity[i] * dT;
		if (X[i].y <= -5.20457f) {
			X[i].y = -5.20457f;
			if(velocity[i].y < 0)
				velocity[i].y *= -0.5f;
		}
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
	initHouse();

	vector<node> gV; vector<int> indices;
	node temp;
	temp.position = glm::vec3(-100, -5.2046f, -100); temp.uv = glm::vec2(-1, -1);
	gV.push_back(temp);
	temp.position = glm::vec3(100, -5.2046f, -100); temp.uv = glm::vec2(1, -1);
	gV.push_back(temp);
	temp.position = glm::vec3(-100, -5.2046f, 100); temp.uv = glm::vec2(-1, 1);
	gV.push_back(temp);
	temp.position = glm::vec3(100, -5.2046f, 100); temp.uv = glm::vec2(1, 1);
	gV.push_back(temp);
	indices.push_back(0); indices.push_back(1); indices.push_back(2);
	indices.push_back(1); indices.push_back(2); indices.push_back(3);

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
		dtt += deltaTime;
		if (dtt >= 4 * dt) {
			for(int i=0;i<4;i++)
				_Update(window, dt);
			dtt = 0;
		}

		int cnt = 0;
		for (int tet = 0; tet < tet_number; tet++)
		{
			vertices[cnt++].position = X[Tet[tet * 4 + 0]];
			vertices[cnt++].position = X[Tet[tet * 4 + 2]];
			vertices[cnt++].position = X[Tet[tet * 4 + 1]];

			vertices[cnt++].position = X[Tet[tet * 4 + 0]];
			vertices[cnt++].position = X[Tet[tet * 4 + 3]];
			vertices[cnt++].position = X[Tet[tet * 4 + 2]];

			vertices[cnt++].position = X[Tet[tet * 4 + 0]];
			vertices[cnt++].position = X[Tet[tet * 4 + 1]];
			vertices[cnt++].position = X[Tet[tet * 4 + 3]];

			vertices[cnt++].position = X[Tet[tet * 4 + 1]];
			vertices[cnt++].position = X[Tet[tet * 4 + 2]];
			vertices[cnt++].position = X[Tet[tet * 4 + 3]];
		}

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