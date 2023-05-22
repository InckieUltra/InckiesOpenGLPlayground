#pragma once
#include"config.h"
#include"opengl_render.h"


void sim(){
	init_window();

	vector<Vertex> vs;
	Vertex v;
	v.Position.x = -0.5f;
	v.Position.y = -0.5f;
	v.Position.z = -0.5f;
	vs.push_back(v); v.Position.x = 0.5f; vs.push_back(v); v.Position.y = 0.5f; vs.push_back(v);
	vector<unsigned int> is;
	is.push_back(0); is.push_back(1); is.push_back(2);
	vector<Texture> ts;
	Mesh mesh(vs, is, ts);

	Scene scene;
	scene.meshes.push_back(mesh);
	for (int i = 0; i < 100; i++) {
		scene.particles.push_back(glm::vec3(rand() % 100 * 1.0f, rand() % 100 * 1.0f, rand() % 100 * 1.0f / 40));
	}
	Shader shader(
		"D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\mpm_test\\0shader.vs",
		"D:\\OpenGLPlayground\\InckiesOpenGLPlayground\\mpm_test\\0shader.fs"
	);
	scene.setup();
	int cnt = 0;
	while (1) {
		if (cnt > 50) {
			scene.particles.clear();
			for (int i = 0; i < 100; i++) {
				scene.particles.push_back(glm::vec3(rand() % 100 * 1.0f / 20 - 2.5, rand() % 100 * 1.0f / 20 - 2.5, rand() % 100 * 1.0f / 40));
			}
			cnt = 0;
		}
		cnt++;
		scene.update();
		processInput(window);
		draw_frame(scene,shader);
	}
}
