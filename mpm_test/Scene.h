#pragma once
#include "config.h"
#include "shader.h"
#include "mesh.h"
#include "camera.h"

class Scene {
public:
	unsigned int VAO, VBO, EBO;
	std::vector<glm::vec3> particles;
	std::vector<Mesh> meshes;

	void draw(Shader& shader, Camera& camera);
	void setup();
	void update();
};

void Scene::draw(Shader& shader, Camera& camera) {
	shader.use();

	glBindVertexArray(VAO);
	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);
	shader.setMat4("view", camera.GetViewMatrix());
	shader.setMat4("projection", projection);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(glm::vec3), &particles[0], GL_DYNAMIC_DRAW);

	glPointSize(3.0f);
	
	glDrawArrays(GL_POINTS, 0, particles.size());
	
	for (auto m : meshes) {
		m.Draw(shader);
	}
}

void Scene::setup(){
	// create buffers/arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(glm::vec3), &particles[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec3), (void*)0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Scene::update(){
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(glm::vec3), &particles[0], GL_DYNAMIC_DRAW);
}
