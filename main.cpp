#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <windows.h>
#include <thread>

#include "shader.h"
#include "emu.h"

#define H 32
#define W 64

typedef struct EmuArgs {
	emu* emulator;
	GLFWwindow* window;
	unsigned short delay;
}emuArgs;

void emuThread(emuArgs* args) {
	std::ofstream ofile;
	ofile.open("debug.csv");
	ofile << "Cycle,Opcode,0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F,SD,DY,SP,I,PC" << std::endl;

	int keycodes[16] = {GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3,
						GLFW_KEY_KP_4, GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7,
						GLFW_KEY_KP_8, GLFW_KEY_KP_9, GLFW_KEY_A, GLFW_KEY_B,
						GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F};

	unsigned short keys = 0;
	long long start, end;
	while (!glfwWindowShouldClose(args->window)) {
		start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		keys = 0;
		for (unsigned char i = 0; i < 16; i++) {
			if (glfwGetKey(args->window, keycodes[i]) == GLFW_PRESS) {
				keys |= (1 << i);
			}
		}

		if (glfwGetKey(args->window, GLFW_KEY_R) == GLFW_PRESS) {
			args->emulator->reset();
		}

		args->emulator->tick(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), keys, ofile);
		do {
			end = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		} while (end - start < args->delay);
	}

	ofile.close();
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

static void sizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

int main(void) {
	srand(time(0));
	glfwInit();

	/* read ROM */
	char buffer[4096];
	unsigned short buflen;
	unsigned short entry;
	unsigned short delay;

	std::string filename;
	std::ifstream f;

	while (!f.is_open()) {
		printf(">");
		std::cin >> filename;

		f.open(filename.c_str(), std::ios::binary);
		if (!f.is_open()) {
			printf("ERROR: cannot open file %s\n", filename.c_str());
		}
	}

	f.read(buffer, 4096);
	buflen = f.gcount();
	f.close();

	printf("Read %d/4096 bytes from file %s\n", buflen, filename.c_str());

	printf("Specify an entry point address (this will be 512 in most cases)\n>");
	std::cin >> entry;
	printf("Input a delay value (0 for as fast as possible, 2000-4000 is sufficent for most games)\n>");
	std::cin >> delay;

	/* create window */
	GLFWwindow* window = glfwCreateWindow(1280, 640, "CHIP8", NULL, NULL);
	if (!window) {
		std::cout << "ERROR: failed to create GLFW window" << std::endl;
		glfwTerminate();
		return 1;
	}

	glfwSetKeyCallback(window, keyCallback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetFramebufferSizeCallback(window, sizeCallback);

	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	/* create shaders */
	GLuint vShader = createShaderFromFile(GL_VERTEX_SHADER, "shaders/shader.vert");
	GLuint fShader = createShaderFromFile(GL_FRAGMENT_SHADER, "shaders/shader.frag");

	const GLuint shaderList[] = { vShader, fShader };
	GLuint shaderProgram = createShaderProgram(shaderList, 2);

	glDeleteShader(vShader);
	glDeleteShader(fShader);

	/* define texture dimensions and VBO/VAO */
	float vertices[] = {
		// positions                  // texture coords
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,   // top right
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom right
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom left
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f    // top left 
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), NULL); //position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); //texture
	glEnableVertexAttribArray(1);

	/* texture settings */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* main loop variables */
	unsigned char display[32 * 64];
	memset(display, 0, 32 * 64);

	emu* emulator = new emu(display, entry);
	emulator->load((unsigned char*)buffer, buflen, entry);

	emuArgs* args = new emuArgs;

	args->emulator = emulator;
	args->window = window;
	args->delay = delay;

	_beginthread((void(*)(void*)) &emuThread, 0, args); 

	/* main loop */
	while (!glfwWindowShouldClose(window)) {
		/* render texture */
		glTexImage2D(GL_TEXTURE_2D, 0, 1, W, H, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, display);

		/* draw triangles */
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

		/* this code should go last */
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	delete emulator;
	return 0;
}
//dedicated to Zedd