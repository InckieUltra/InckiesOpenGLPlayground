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
#include <ctime>
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
int n_particles = 1024;
int n_grid = 64;
float width = 0.3f;
float dx = 2.0f * width / n_grid;
float dt = 2e-4;

float p_rho = 1;
float p_vol = (dx * 0.5) * (dx * 0.5);
float p_mass = p_vol * p_rho;
float gravity = 9.8;
int bound = 3;
float E = 400;
vector<glm::vec3> x;
vector<glm::vec3> v(n_particles, glm::vec3(0,-2,0));
vector<glm::mat3> C(n_particles, glm::mat3(0.0f));
vector<float> J(n_particles, 1.0f);
vector< vector<glm::vec3>> grid_v(n_grid, vector<glm::vec3>(n_grid, glm::vec3(0.0f)));
vector< vector<float>> grid_m(n_grid, vector<float>(n_grid, 0));

const int THREAD_NUM = 12;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

float lastX = 400, lastY = 300;

void initData() {
	srand(time(0));
	for (int i = 0; i < n_particles; i++) {
		x.push_back(glm::vec3(( rand()%100 )*1.0f/500-0.1, (rand() % 100)*1.0f / 500 - 0.1, 0));
	}
}

vector<thread> threads;
int TASK_NUM = 100;
int AVG_NUM = TASK_NUM / THREAD_NUM;

void P2G(int p) {
	glm::vec3 Xp = x[p] / dx;
	glm::vec3 base = glm::vec3(floor(Xp.x - 0.5f), floor(Xp.y - 0.5f), 0.0f);
	glm::vec3 fx = Xp - base;
	glm::mat3x2 w = glm::mat3x2(0.5 * (1.5 - fx.x) * (1.5 - fx.x), 0.5 * (1.5 - fx.y) * (1.5 - fx.y),
		0.75 - (fx.x - 1) * (fx.x - 1), 0.75 - (fx.y - 1) * (fx.y - 1),
		0.5 * (fx.x - 0.5)* (fx.x - 0.5), 0.5 * (fx.y - 0.5)* (fx.y - 0.5));

	float stress = -dt * 4 * E * p_vol * (J[p] - 1) / (dx * dx);
	glm::mat3 affine = glm::mat3(stress, 0, 0,
		0, stress, 0,
		0, 0, 0) + p_mass * C[p];
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			glm::vec3 offset = glm::vec3(i, j, 0);
			glm::vec3 dpos = (offset - fx) * dx;
			float weight = w[i][0] * w[j][1];
			glm::vec3 _index = base + offset + glm::vec3(n_grid / 2, n_grid / 2, 0);
			if (_index.x < 0 || _index.x >= n_grid || _index.y < 0 || _index.y >= n_grid)
				cout << x[p].x << ' ' << base.x << ' ' << _index.x << ' ' << _index.y << endl;
			grid_v[_index.x][_index.y] += weight * (p_mass * v[p] + affine * dpos);
			grid_m[_index.x][_index.y] += weight * p_mass;
		}
}

void update_grid_field(int j) {
	for (int i = 0; i < n_grid; i++) {
		if (grid_m[i][j] > 0)
			grid_v[i][j] /= grid_m[i][j];
		grid_v[i][j].y -= dt * gravity;
		if (i < bound && grid_v[i][j].x < 0)
			grid_v[i][j].x = 0;
		if (i > n_grid - bound && grid_v[i][j].x > 0)
			grid_v[i][j].x = 0;
		if (j < bound && grid_v[i][j].y < 0)
			grid_v[i][j].y = 0;
		if (j > n_grid - bound && grid_v[i][j].y > 0)
			grid_v[i][j].y = 0;
	}
}

void G2P(int p) {
	glm::vec3 Xp = x[p] / dx;
	glm::vec3 base = glm::vec3(floor(Xp.x - 0.5f), floor(Xp.y - 0.5f), 0.0f);
	glm::vec3 fx = Xp - base;
	glm::mat3x2 w = glm::mat3x2(0.5 * (1.5 - fx.x) * (1.5 - fx.x), 0.5 * (1.5 - fx.y) * (1.5 - fx.y),
		0.75 - (fx.x - 1) * (fx.x - 1), 0.75 - (fx.y - 1) * (fx.y - 1),
		0.5 * (fx.x - 0.5)* (fx.x - 0.5), 0.5 * (fx.y - 0.5)* (fx.y - 0.5));
	glm::vec3 new_v = glm::vec3(0.0f);
	glm::mat3 new_C = glm::mat3(0.0f);
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			glm::vec3 offset = glm::vec3(i, j, 0);
			glm::vec3 dpos = (offset - fx) * dx;
			float weight = w[i][0] * w[j][1];
			glm::vec3 _index = base + offset + glm::vec3(n_grid / 2, n_grid / 2, 0);
			if (_index.x < 0 || _index.x >= n_grid || _index.y < 0 || _index.y >= n_grid)
				cout << x[p].x << ' ' << base.x << ' ' << _index.x << ' ' << _index.y << endl;
			glm::vec3 g_v = grid_v[_index.x][_index.y];
			new_v += weight * g_v;
			new_C += 4 * weight * glm::outerProduct(g_v, dpos) / (dx*dx);
		}
	v[p] = new_v;
	if (v[p].x != v[p].x)
		v[p].x = 0;
	if (v[p].y != v[p].y)
		v[p].y = 0;
	x[p] += dt * v[p];
	if (x[p].x <= -width+0.02f) {
		x[p].x = -width + 0.02f;
		v[p].x = 0.2;
	}
	if (x[p].x >= width-0.02f) {
		x[p].x = width - 0.02f;
		v[p].x = -0.2;
	}
	if (x[p].y <= -width+0.02f) {
		x[p].y = -width + 0.02f;
		v[p].y = 0.2;
	}
	if (x[p].y >= width-0.02f) {
		x[p].y = width - 0.02f;
		v[p].y = -0.2;
	}
	J[p] *= 1 + dt * (new_C[0][0] + new_C[1][1] + new_C[2][2]);
	C[p] = new_C;
}

void substep() {
	threads.reserve(static_cast<size_t>(THREAD_NUM));
	int AVG_NUM;

	for (int i = 0; i < n_grid; i++) {
		memset(&grid_v[i][0], 0, sizeof(glm::vec3) * n_grid);
		memset(&grid_m[i][0], 0, sizeof(float) * n_grid);
	}

	// P2G
	threads.clear();
	TASK_NUM = n_particles;
	AVG_NUM = TASK_NUM / THREAD_NUM;
	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				P2G(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			P2G(j);
		}
		});
	for (auto &t : threads) {
		t.join();
	}

	// update grid field
	threads.clear();
	TASK_NUM = n_grid;
	AVG_NUM = TASK_NUM / THREAD_NUM;
	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				update_grid_field(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			update_grid_field(j);
		}
		});
	for (auto &t : threads) {
		t.join();
	}

	// G2P
	threads.clear();
	TASK_NUM = n_particles;
	AVG_NUM = TASK_NUM / THREAD_NUM;
	for (int i = 0; i < THREAD_NUM - 1; ++i) {
		threads.emplace_back([i, AVG_NUM]() {
			for (int j = i * AVG_NUM; j < (i + 1) * AVG_NUM; j++) {
				G2P(j);
			}
			});
	}
	threads.emplace_back([&]() {
		for (int j = (THREAD_NUM - 1) * AVG_NUM; j < TASK_NUM; j++) {
			G2P(j);
		}
		});
	for (auto &t : threads) {
		t.join();
	}
}

void MPMsimulate() {
	for (int i = 0; i < 1; i++) {
		substep();
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
	Shader ourShader("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\mpm88\\3.3.shader.vs", "D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\mpm88\\3.3.shader.fs"); // you can name your shader files however you like


	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	initData();

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, x.size() * sizeof(glm::vec3), &x[0], GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	while (!glfwWindowShouldClose(window))
	{
		MPMsimulate();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float timeValue = glfwGetTime();

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, x.size() * sizeof(glm::vec3), &x[0], GL_DYNAMIC_DRAW);
		ourShader.use();

		glPointSize(2.0f);
		glDrawArrays(GL_POINTS, 0, x.size() * sizeof(glm::vec3));

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}