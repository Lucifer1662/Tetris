#pragma once

#include <GL\glew.h>
#include <string>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
using std::cout;
using std::endl;
using std::vector;
using std::string;

GLuint CreateShader(GLenum type, string source);

unsigned int CreateProgram(const string& vsCode, const string& fsCode);
unsigned int CreateProgram(const string& vsCode, const string& gsCode, const string& fsCode);
unsigned int CreateProgram(const string & computeCode);


struct _Program {
	unsigned int program;
	_Program(int typeId, bool load);
	~_Program();
};

typedef std::shared_ptr<_Program> Program;


GLuint CreateShader(GLenum type, string source) {

	GLuint shader = glCreateShader(type);
	const GLchar* chars = source.c_str();
	glShaderSource(shader, 1, &chars, NULL);
	glCompileShader(shader);
	GLint isCompiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		//The maxLength includes the NULL character
		GLchar* infoLog = new GLchar[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
		//We don't need the shader anymore.
		glDeleteShader(shader);
		cout << maxLength << endl;
		cout << "-----------------------------------------" << endl;
		cout << infoLog << endl;
		cout << "-----------------------------------------" << endl;
		cout << source << endl;
		cout << "-----------------------------------------" << endl;
		return 0;
	}

	return shader;
}

unsigned int CreateProgram(const string& vsCode, const string& fsCode) {
	unsigned int program = glCreateProgram();

	

	unsigned int vs = CreateShader(GL_VERTEX_SHADER, vsCode);
	glAttachShader(program, vs);


	unsigned int fs = CreateShader(GL_FRAGMENT_SHADER, fsCode);
	glAttachShader(program, fs);


	glLinkProgram(program);

	int InfoLogLength;
	GLint Result = GL_FALSE;
	// Check the program
	glGetProgramiv(program, GL_LINK_STATUS, &Result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(program, vs);
	glDetachShader(program, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	glUseProgram(program);
	return program;
}

unsigned int CreateProgram(const string & computeCode)
{
	unsigned int program = glCreateProgram();
	

	unsigned int cs = CreateShader(GL_COMPUTE_SHADER, computeCode);
	glAttachShader(program, cs);



	glLinkProgram(program);

	int InfoLogLength;
	GLint Result = GL_FALSE;
	// Check the program
	glGetProgramiv(program, GL_LINK_STATUS, &Result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(program, cs);
	glDeleteShader(cs);
	glUseProgram(program);
	return program;
}

unsigned int CreateProgram(const string & vsCode, const string & gsCode, const string & fsCode)
{
	unsigned int program = glCreateProgram();
	glUseProgram(program);

	unsigned int vs = CreateShader(GL_VERTEX_SHADER, vsCode);
	glAttachShader(program, vs);

	unsigned int gs = CreateShader(GL_GEOMETRY_SHADER, gsCode);
	glAttachShader(program, gs);

	unsigned int fs = CreateShader(GL_FRAGMENT_SHADER, fsCode);
	glAttachShader(program, fs);


	glLinkProgram(program);

	int InfoLogLength;
	GLint Result = GL_FALSE;
	// Check the program
	glGetProgramiv(program, GL_LINK_STATUS, &Result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(program, vs);
	glDetachShader(program, gs);
	glDetachShader(program, fs);
	glDeleteShader(vs);
	glDeleteShader(gs);
	glDeleteShader(fs);
	return program;
}
