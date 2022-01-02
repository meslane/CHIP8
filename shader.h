#ifndef __SHADER_H__
#define __SHADER_H__

#include <glad/glad.h>

GLuint createShaderFromFile(const GLenum shaderType, const char* filename);
GLuint createShaderProgram(const GLuint* shaderArray, const unsigned int num);

#endif