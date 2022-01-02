#include "shader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glad/glad.h>

GLuint createShaderFromFile(const GLenum shaderType, const char* filename) {
	GLuint shader = glCreateShader(shaderType);
	std::string fileText;

	std::ifstream inp;
	inp.open(filename);
	if (!inp) {
		std::cout << "ERROR: failed to open shader file " << filename << std::endl;
		return 0;
	}

	std::string temp = "";
	while (!inp.eof()) {
		std::getline(inp, temp);
		fileText.append(temp + "\n");
	}

	inp.close();

	const char* s = fileText.c_str();
	glShaderSource(shader, 1, &s, NULL);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		std::cout << "ERROR: failed to compile shader from " << filename << std::endl;
	}

	return shader;
}

GLuint createShaderProgram(const GLuint* shaderArray, const unsigned int num) {
	GLuint shaderProgram = glCreateProgram();

	for (unsigned int i = 0; i < num; i++) {
		glAttachShader(shaderProgram, shaderArray[i]);
	}

	glLinkProgram(shaderProgram);

	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		std::cout << "ERROR: failed to create shader program" << std::endl;
	}

	return shaderProgram;
}