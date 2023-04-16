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

// data
bool launched = false, rotating = false;
float dtdt = 0.025f;
float dtV = -dtdt / 2, dtX = 0, dtW = -dtdt / 2, dtR = 0, dtT = 0;
glm::vec3 velocity = glm::vec3(-4.0f, 1.0f, 0.0f);
glm::vec3 position = glm::vec3(0.0f, -1.0f, 0.0f);
glm::vec3 constantG = glm::vec3(0.0f, -9.8f, 0.0f);
glm::vec3 omega = glm::vec3(0.000001f, 0.000001f, 0.000001f);
glm::mat4 rotation = glm::mat4(1.0f);
float restitution = 0.5f;

glm::mat4 I_ref = glm::mat4(0.0f);
float m = 1;
float mass = 0;
Model *theModel;
Model ourModel("");

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

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

void drawRotatingBox(Shader ourShader,glm::vec3 cubePositions[],float timeValue) {
	for (unsigned int i = 0; i < 10; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, cubePositions[i]);
		float angle = 20.0f * i;
		if (i % 3 == 0)
			model = glm::rotate(model, timeValue, glm::vec3(1.0f, 0.3f, 0.5f));
		else
			model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		ourShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

void drawFloorAndWall(Shader ourShader) {
	
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, -4.0f, -4.0f));
		model = glm::rotate(model, glm::radians(90.0f),glm::vec3(1,0,0));
		ourShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0f, 0 , -4.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
		ourShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 6);
}

glm::vec4 J;
glm::mat4 I;

struct CollisionHelper {
	glm::vec3 center;
	int size;
	bool isCollision;
}c[2];

void CollisionDetection(int li, int ui) {

	for (int i = li; i < ui; i++) {
		glm::vec3 _ri = ourModel.meshes[0].vertices[i * 10].Position;
		glm::vec4 Rri = rotation * glm::vec4(_ri.x, _ri.y, _ri.z, 1.0f);
		glm::vec3 xi = position + glm::vec3(Rri.x, Rri.y, Rri.z);

		if (xi.y <= -2.0f) {
			c[0].isCollision = true;
			c[0].size++;
			c[0].center += xi - position;
		}
		if (xi.x <= -3.0f) {
			c[1].isCollision = true;
			c[1].size++;
			c[1].center += xi - position;
		}
	}
}

void CollisionHandling(int cid, glm::vec3 normal) {
	if (!c[cid].isCollision)
		return;
	restitution -= 0.0000005f;
	glm::vec3 collideCenter = c[cid].center / (float)c[cid].size;
	glm::vec3 xi = position + collideCenter;
	if ((xi.y <= -2.0f && cid == 0) || (xi.x <= -3.0f && cid == 1)) {
		glm::vec3 vi = velocity + glm::vec3(omega.x*collideCenter.x, omega.y*collideCenter.y, omega.z*collideCenter.z);
		glm::vec3 vn = glm::dot(vi, normal) * normal;
		glm::vec3 vt = vi - vn;
		if (glm::dot(vn, normal) < 0) {
			float vtValue = max(1 - (0.5*(1 + restitution)*glm::length(vn)) / glm::length(vt), (double)0);
			vn = -restitution * vn;
			vt = vt * vtValue;
			//cout << (vt + vn).x << ' ' << (vt + vn).y << ' ' << (vt + vn).z << endl;
		}
		glm::vec3 dvi = (vn + vt) - vi;
		//cout << "dvi" << ' ' << dvi.x << ' ' << dvi.y << ' ' << dvi.z << endl;
		glm::mat4 Rriprime = glm::mat4(0, -collideCenter.z, collideCenter.y, 0,
			collideCenter.z, 0, -collideCenter.x, 0,
			-collideCenter.y, collideCenter.x, 0, 0,
			0, 0, 0, 1);
		I = rotation * I_ref * glm::transpose(rotation);
		glm::mat4 K = glm::mat4(1.0f / mass) - Rriprime * glm::inverse(I) * Rriprime;
		glm::mat4 inverse_K = glm::inverse(K);
		float *vs = glm::value_ptr(inverse_K);

		J += glm::inverse(K) * glm::vec4(dvi.x, dvi.y, dvi.z, 1.0f);
		//cout << J.x << ' ' << J.y << ' ' << J.z << endl;

		velocity += 1.0f / mass * glm::vec3(J.x, J.y, J.z);
		glm::vec3 RriCrossJ = glm::cross(collideCenter, glm::vec3(J.x, J.y, J.z));
		glm::vec4 domega = glm::inverse(I) * glm::vec4(RriCrossJ.x, RriCrossJ.y, RriCrossJ.z, 1.0f);
		omega += glm::vec3(domega.x, domega.y, domega.z);
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
	Shader ourShader("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\HelloWorld\\3.3.shader.vs", "D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\HelloWorld\\3.3.shader.fs"); // you can name your shader files however you like
	Shader bunnyShader("D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\HelloWorld\\3.3.shader.vs", "D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\games103_hw1_rigidbody\\3.3.bunny.fs.txt");


	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
	-2.0f, -2.0f, -2.0f,  0.0f, 0.0f,
	 2.0f, -2.0f, -2.0f,  1.0f, 0.0f,
	 2.0f,  2.0f, -2.0f,  1.0f, 1.0f,
	 2.0f,  2.0f, -2.0f,  1.0f, 1.0f,
	-2.0f,  2.0f, -2.0f,  0.0f, 1.0f,
	-2.0f, -2.0f, -2.0f,  0.0f, 0.0f
	};

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//配置EBO
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0*sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
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
	unsigned char *data = stbi_load("C:\\Users\\NH55\\Pictures\\marble.png", &width, &height, &nrChannels, 0);
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

	// load models
	// -----------
	Model oourModel("C:/Users/NH55/Downloads/stanford_bunny_obj_mtl_jpg-master/colorbunny.obj");
	theModel = &ourModel;
	ourModel = oourModel;

	int cnt = 0,maxn=0,minn=0x3f3f3f3f;
	for (int i = 0; i < ourModel.meshes.size(); i++) {
		cnt += ourModel.meshes[i].vertices.size();
		maxn = std::max(maxn, (int)ourModel.meshes[i].vertices.size());
		minn = std::min(minn, (int)ourModel.meshes[i].vertices.size());
	}
	cout << ourModel.meshes.size() <<' '<<minn<<' '<<maxn<<' '<<cnt/ourModel.meshes.size()<<' '<< cnt << endl;

	rotation = glm::rotate(rotation, glm::radians(60.0f), glm::vec3(1, 1, 1));

	for (int i = 0; i < ourModel.meshes[0].vertices.size(); i += 10)
	{
		mass += m;
		glm::vec3 _vertice = ourModel.meshes[0].vertices[i].Position;
		float diag = m * glm::length(_vertice);
		I_ref += glm::mat4(diag - m * _vertice.x * _vertice.x, -m * _vertice.x * _vertice.y, -m * _vertice.x * _vertice.z, 0,
			-m * _vertice.y * _vertice.x, diag - m * _vertice.y * _vertice.y, -m * _vertice.y * _vertice.z, 0,
			-m * _vertice.z * _vertice.x, -m * _vertice.z * _vertice.y, diag - m * _vertice.z * _vertice.z, 0,
			0, 0, 0, 0);
	}
	I_ref += glm::mat4(0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window,deltaTime);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// render the triangle
		ourShader.use();
		glActiveTexture(GL_TEXTURE0); // 在绑定纹理之前先激活纹理单元
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(VAO);

		// camera/view transformation
		ourShader.setMat4("view", camera.GetViewMatrix());
		bunnyShader.setMat4("view", camera.GetViewMatrix());

		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);
		bunnyShader.setMat4("projection", projection);

		//drawRotatingBox(ourShader,cubePositions,timeValue);
		drawFloorAndWall(ourShader);

		if (glfwGetKey(window, GLFW_KEY_1)) {
			launched = true;
			rotating = true;
		}
		if (glfwGetKey(window, GLFW_KEY_2)) {
			velocity = glm::vec3(-4, 2, 0);
			omega = glm::vec3(0.000001f, 0.000001f, 0.001f);
			restitution = 0.5f;
		}
		if (glfwGetKey(window, GLFW_KEY_R)) {
			launched = rotating = false;
			dtV = -dtdt / 2, dtX = 0, dtW = -dtdt / 2, dtR = 0, dtT = 0;
			velocity = glm::vec3(-4.0f, 1.0f, 0.0f);
			position = glm::vec3(0.0f, 0.0f, 0.0f);
			omega = glm::vec3(0.000001f, 0.000001f, 0.001f);
			rotation = glm::mat4(1.0f);
			rotation = glm::rotate(rotation, glm::radians(60.0f), glm::vec3(1, 1, 1));
			restitution= 0.5f;
		}

		//cout << glfwGetTime() - curT << endl;
		
		if (launched) {
			dtT += dtdt;
			if (dtT >= dtdt) {
				c[0].size = c[1].size = 0;
				c[0].center = c[1].center = glm::vec3(0.0f);
				c[0].isCollision = c[1].isCollision = false;
				J = glm::vec4(0.0f);

				vector<thread> threads;
				int THREAD_NUM = 6;
				threads.reserve(static_cast<size_t>(THREAD_NUM));
				int TASK_NUM = ourModel.meshes[0].vertices.size()/10;
				int AVG_NUM = TASK_NUM / THREAD_NUM;

				for (int i = 0; i < THREAD_NUM - 1; ++i) {
					threads.emplace_back([i, AVG_NUM]() {
						CollisionDetection(i*AVG_NUM, (i+1)*AVG_NUM);
						});
				}
				threads.emplace_back([&]() {
					CollisionDetection((THREAD_NUM-1)*AVG_NUM, TASK_NUM);
					});
				for (auto &t : threads) {
					t.join();
				}

				CollisionHandling(0, glm::vec3(0, 1, 0));
				CollisionHandling(1, glm::vec3(1, 0, 0));

				dtT = 0;
			}

			//Update State
			dtV += dtdt; dtX += dtdt;
			if (dtX >= dtdt) {
				position += dtdt * velocity;
				dtX = 0;
			}
			if (dtV >= dtdt) {
				velocity += dtdt * constantG;
				velocity *= 0.996;
				dtV = -0.5f * dtdt;
			}

			dtW += dtdt; dtR += dtdt;
			if (dtR >= dtdt) {
				glm::mat4 newrotation = glm::mat4(1.0f);
				newrotation = glm::rotate(newrotation, dtdt * glm::length(omega), glm::vec3(omega.x,omega.y,omega.z));
				rotation = newrotation * rotation;
				dtR = 0;
			}
			if (dtW >= dtdt) {
				omega *= 0.95f;
				dtW = -0.5f * dtdt;
			}
		}

		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -4.0f)); // translate it down so it's at the center of the scene
		model = glm::translate(model, position);
		model = model * rotation;


		bunnyShader.setMat4("model", model);
		ourModel.Draw(bunnyShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}