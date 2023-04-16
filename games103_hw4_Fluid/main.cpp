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
struct node {
	glm::vec3 position;
	glm::vec2 uv;
};
vector<node> vertices;
vector<int> triangles;
const int siz = 60;
float rate = 0.005f;
float gamma = 0.004f;
float damping = 0.99f;
vector< vector<float>> 	h(siz, vector<float>(siz, 0));
vector< vector<float>> 	old_h(siz, vector<float>(siz, 0));
vector< vector<float>>  new_h(siz, vector<float>(siz, 0.0f));
vector< vector<float>>	low_h(siz, vector<float>(siz, 999999));
vector< vector<float>>	vh(siz, vector<float>(siz, 0));
vector< vector<float>>	b(siz, vector<float>(siz, 0));

vector< vector<bool>>	cg_mask(siz, vector<bool>(siz, 0));
vector< vector<float>>	cg_p(siz, vector<float>(siz, 0));
vector< vector<float>>	cg_r(siz, vector<float>(siz, 0));
vector< vector<float>>	cg_Ap(siz, vector<float>(siz, 0));
bool 	tag = true;

glm::vec3	cube_v = glm::vec3(0.0f);
glm::vec3 	cube_w = glm::vec3(0.0f);

node bvertices[8];
int btriangles[36] = { 0,1,2, 1,2,3, 4,5,6, 5,6,7, 0,2,4, 2,4,6, 1,3,5, 3,5,7, 0,1,4, 1,4,5, 2,3,6, 3,6,7 };
glm::vec3 bPos[2] = { glm::vec3(0,0,0), glm::vec3(0,0,-1) };

int op[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };

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

void initBlocks() {
	bvertices[0].position = glm::vec3(-0.25f, 0.25f, -0.25f);
	bvertices[1].position = glm::vec3(-0.25f, 0.25f, 0.25f);
	bvertices[2].position = glm::vec3(0.25f, 0.25f, -0.25f);
	bvertices[3].position = glm::vec3(0.25f, 0.25f, 0.25f); 
	bvertices[4].position = glm::vec3(-0.25f, -0.25f, -0.25f);
	bvertices[5].position = glm::vec3(-0.25f, -0.25f, 0.25f); 
	bvertices[6].position = glm::vec3(0.25f, -0.25f, -0.25f); 
	bvertices[7].position = glm::vec3(0.25f, -0.25f, 0.25f);
}

void initWaterSurface() {

	node temp;

	for (int i = 0; i < siz; i++)
		for (int j = 0; j < siz; j++)
		{
			temp.position = glm::vec3(i * 0.1f - siz * 0.05f, 0.0f, j * 0.1f - siz * 0.05f);
			vertices.push_back(temp);
		}

	int index = 0;
	for (int i = 0; i < siz - 1; i++)
		for (int j = 0; j < siz - 1; j++)
		{
			triangles.push_back((i + 0) * siz + (j + 0));
			triangles.push_back((i + 0) * siz + (j + 1));
			triangles.push_back((i + 1) * siz + (j + 1));
			triangles.push_back((i + 0) * siz + (j + 0));
			triangles.push_back((i + 1) * siz + (j + 1));
			triangles.push_back((i + 1) * siz + (j + 0));
			index++;
		}

	for (int i = 0; i < siz; i++)
		for (int j = 0; j < siz; j++)
		{
			low_h[i][j] = 99999;
			old_h[i][j] = 0;
			vh[i][j] = 0;
		}
}

bool inBoundry(int x, int y) {
	return (x >= 0 && x < siz) && (y >= 0 && y < siz);
}

void A_Times(vector< vector<float>> *xx, vector< vector<float>> *Axx, int li, int ui, int lj, int uj)
{
	vector< vector<float>> x = *xx;
	vector< vector<float>>::iterator it1 = Axx->begin();
	vector<float>::iterator it2;
	for (int i = li; i <= ui; i++) {
		for (int j = lj; j <= uj; j++) {
			it2 = (*(it1+i)).begin();
			it2 += j;
			if (i >= 0 && j >= 0 && i < siz && j < siz && cg_mask[i][j])
			{
				*it2 = 0;
				if (i != 0)		*it2 -= x[i - 1][j] - x[i][j];
				if (i != siz - 1)	*it2 -= x[i + 1][j] - x[i][j];
				if (j != 0)		*it2 -= x[i][j - 1] - x[i][j];
				if (j != siz - 1)	*it2 -= x[i][j + 1] - x[i][j];
			}
		}
	}
}

int scnt = 0;
float Dot(vector< vector<bool>> *maskk, vector< vector<float>> *xx, vector< vector<float>> *yy, int li, int ui, int lj, int uj)
{
	vector< vector<bool>> mask = *maskk;
	vector< vector<float>> x = *xx, y = *yy;
	float ret = 0;
	for (int i = li; i <= ui; i++)
		for (int j = lj; j <= uj; j++)
			if (i >= 0 && j >= 0 && i < siz && j < siz && mask[i][j])
			{
				ret += x[i][j] * y[i][j];
			}
	return ret;
}

void Conjugate_Gradient(int li, int ui, int lj, int uj)
{
	//Solve the Laplacian problem by CG.
	A_Times(&vh, &cg_r, li, ui, lj, uj);

	for (int i = li; i <= ui; i++)
		for (int j = lj; j <= uj; j++)
			if (i >= 0 && j >= 0 && i < siz && j < siz && cg_mask[i][j])
			{
				cg_p[i][j] = cg_r[i][j] = b[i][j] - cg_r[i][j];
			}

	float rk_norm = Dot(&cg_mask, &cg_r, &cg_r, li, ui, lj, uj);

	for (int k = 0; k < 128; k++)
	{
		if (rk_norm < 1e-10f)	break;
		A_Times(&cg_p, &cg_Ap, li, ui, lj, uj);
		float alpha = rk_norm / Dot(&cg_mask, &cg_p, &cg_Ap, li, ui, lj, uj);

		for (int i = li; i <= ui; i++)
			for (int j = lj; j <= uj; j++)
				if (i >= 0 && j >= 0 && i < siz && j < siz && cg_mask[i][j])
				{
					vh[i][j] += alpha * cg_p[i][j];
					cg_r[i][j] -= alpha * cg_Ap[i][j];
				}
		float _rk_norm = Dot(&cg_mask, &cg_r, &cg_r, li, ui, lj, uj);
		float beta = _rk_norm / rk_norm;
		rk_norm = _rk_norm;

		for (int i = li; i <= ui; i++)
			for (int j = lj; j <= uj; j++)
				if (i >= 0 && j >= 0 && i < siz && j < siz && cg_mask[i][j])
				{
					cg_p[i][j] = cg_r[i][j] + beta * cg_p[i][j];
				}
	}

}

void Shallow_Wave(int i)
{
	//Step 1:
	//TODO: Compute new_h based on the shallow wave model.
	for (int j = 0; j < siz; j++) {
		new_h[i][j] = h[i][j] + (h[i][j] - old_h[i][j]) * damping;
		for (int k = 0; k < 4; k++) {
			int newi = i + op[k][0], newj = j + op[k][1];
			if (inBoundry(newi, newj)) {
				new_h[i][j] += (h[newi][newj] - h[i][j]) * rate;
			}
		}
	}

	//Step 2: Block->Water coupling
	//TODO: for block 1, calculate low_h.
	//TODO: then set up b and cg_mask for conjugate gradient.
	//TODO: Solve the Poisson equation to obtain vh (virtual height).

	//TODO: for block 2, calculate low_h.
	//TODO: then set up b and cg_mask for conjugate gradient.
	//TODO: Solve the Poisson equation to obtain vh (virtual height).
}

void Shallow_Wave_2(int i) {
	//TODO: Diminish vh.
	for (int j = 0; j < siz; j++) {
		vh[i][j] *= gamma;
		//cout << vh[i][j]<<endl;
	}
}

void Shallow_Wave_3(int i) {
	// TODO: Update new_h by vh.
		for (int j = 0; j < siz; j++) {
			if (vh[i][j] != vh[i][j] ) {
				//cout << vh[i][j] << endl;
				continue;
			}
			for (int k = 0; k < 4; k++) {
				int newi = i + op[k][0], newj = j + op[k][1];
				if (inBoundry(newi, newj)) {
					if (vh[newi][newj] != vh[newi][newj]  ) {
						//cout << vh[newi][newj] << endl;
						continue;
					}
					new_h[i][j] += (vh[newi][newj] - vh[i][j]) * rate;
				}
			}
		}
	//Step 3
	//TODO: old_h <- h; h <- new_h;

	for (int j = 0; j < siz; j++) {
		if (new_h[i][j] > 3 || new_h[i][j] < -3)
			continue;
		old_h[i][j] = h[i][j];
		h[i][j] = new_h[i][j];
	}

	//Step 4: Water->Block coupling.
	//More TODO here.

	for (int j = 0; j < siz; j++) {
		vertices[i*siz + j].position.y = h[i][j];
	}
}

void Shallow_Wave_4(int i) {
	for (int j = 0; j < siz; j++) {
		if (h[i][j] >= low_h[i][j]) {
			cg_mask[i][j] = true;
			b[i][j] = (new_h[i][j] - low_h[i][j]) / rate;
		}
		else {
			cg_mask[i][j] = false;
			b[i][j] = 0;
			vh[i][j] = 0;
		}
	}
}

void _Update() {
	scnt++;
	for (int i = 0; i < siz; i++) {
		if (scnt == 8) {
			//std::fill(h[i].begin(), h[i].end(), 0);
			scnt = 0;
		}
	}
	vector<thread> threads;
	int THREAD_NUM = 12;
	threads.reserve(static_cast<size_t>(THREAD_NUM));
	int TASK_NUM = siz;
	int AVG_NUM = TASK_NUM / THREAD_NUM;

	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				Shallow_Wave(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			Shallow_Wave(j);
		}
		});
	for (auto &t : threads) {
		t.join();
	}
	//
	for (int ii = 0; ii < siz; ii++) {
		std::fill(low_h[ii].begin(), low_h[ii].end(), 99999);
	}
	int li = max((int)ceil((bPos[0].x - 0.25f + 0.05f*siz) / 0.1f), 0),
		ui = min((int)floor((bPos[0].x + 0.25f + 0.05f*siz) / 0.1f), siz-1),
		lj = max((int)ceil((bPos[0].z - 0.25f + 0.05f*siz) / 0.1f), 0),
		uj = min((int)floor((bPos[0].z + 0.25f + 0.05f*siz) / 0.1f), siz-1);
	for (int ii = li; ii <= uj; ii++) {
		for (int j = lj; j <= uj; j++) {
			low_h[ii][j] = -0.25f;
		}
	}
	//
	threads.clear();
	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				Shallow_Wave_4(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			Shallow_Wave_4(j);
		}
		});
	for (auto &t : threads) {
		t.join();
	}
	//
	Conjugate_Gradient(li, ui, lj, uj);
	//
	threads.clear();
	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				Shallow_Wave_2(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			Shallow_Wave_2(j);
		}
		});
	for (auto &t : threads) {
		t.join();
	}
	//
	threads.clear();
	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				Shallow_Wave_3(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			Shallow_Wave_3(j);
		}
		});
	for (auto &t : threads) {
		t.join();
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
	Shader ourShader("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw2_cloth\\3.3.shader.vs", "D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw4_Fluid\\3.3.bunny.fs.txt"); // you can name your shader files however you like

	//Init
	initWaterSurface();
	initBlocks();


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

		if (glfwGetKey(window, GLFW_KEY_I)) {
			bPos[0]-=glm::vec3(0,0,1)*deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_K)) {
			bPos[0] += glm::vec3(0, 0, 1)*deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_J)) {
			bPos[0] -= glm::vec3(1, 0, 0)*deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_L)) {
			bPos[0] += glm::vec3(1, 0, 0)*deltaTime;
		}

		//update
		std::srand(currentFrame);
		if (glfwGetKey(window, GLFW_KEY_1)) {
			int ri = std::rand() % siz, rj = std::rand() % siz;
			float dH = 0.05f;
			dH /= 10.0f;
			h[ri][rj] -= dH;
			int cnt = 0;
			for (int i = 0; i < 4; i++) {
				int newi = ri + op[i][0], newj = rj + op[i][1];
				if (inBoundry(newi, newj)) {
					cnt++;
				}
			}
			for (int i = 0; i < 4; i++) {
				int newi = ri + op[i][0], newj = rj + op[i][1];
				if (inBoundry(newi, newj)) {
					h[newi][newj] += dH / cnt;
				}
			}
		}
		
		for (int l = 0; l < 3; l++) {
			_Update();
		}

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ourShader.use();
		glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(VAO);
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 model = glm::mat4(1.0f);

		// draw surface
		ourShader.setMat4("view", camera.GetViewMatrix());
		ourShader.setMat4("projection", projection);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(node), &vertices[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(unsigned int), &triangles[0], GL_STATIC_DRAW);

		model = glm::translate(model, glm::vec3(0.0f, -10.0f, -10.0f));
		ourShader.setMat4("model", model);

		glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);

		// draw blocks
		model = glm::translate(model, bPos[0]);
		ourShader.setMat4("model", model);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(node), &bvertices[0], GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(unsigned int), &btriangles[0], GL_STATIC_DRAW);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		model = glm::translate(model, -bPos[0]+bPos[1]);
		ourShader.setMat4("model", model);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(node), &bvertices[0], GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(unsigned int), &btriangles[0], GL_STATIC_DRAW);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}