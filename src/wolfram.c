#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#define WIN_WIDTH 512
#define WIN_HEIGHT 512
#define WIN_TITLE "wolfram"

unsigned char* display_buffer;

void print_version(void);

float map(float x, float min_i, float max_i, float min_o, float max_o) {
	float range_i = max_i - min_i;
	float range_o = max_o - min_o;
	/* offset, scale, re-offset */
	return (x - min_i) * (range_o / range_i) + min_o;
}

int main(void) {
	glfwInit();
	/* 4.3 required for debug context */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(
		WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL
	);
	if (window == NULL) {
		const char* msg = NULL;
		glfwGetError(&msg);
		fprintf(stderr, "error: glfwCreateWindow: %s\n", msg);
		return 1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		fprintf(stderr, "error: gladLoadGL\n");
		glfwTerminate();
		return 2;
	}

	/* vertex array init *************************************************/
	GLfloat vertices[] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0};
	GLubyte indices[] = {0, 1, 2, 0, 2, 3};

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0
	);

	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
		GL_STATIC_DRAW
	);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* shader program ini ************************************************/
	const GLuint vshader_id = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vshader_string =
	"#version 330 core\n"
	"layout (location = 0) in vec2 a_position;\n"
	"out vec2 f_tex_coords;\n"
	"uniform mat4 mvp_matrix;\n"
	"void main () {\n"
	"	f_tex_coords = a_position;\n"
	"	gl_Position = mvp_matrix * vec4(a_position, 0.0, 1.0);\n"
	"}\n";
	glShaderSource(vshader_id, 1, &vshader_string, NULL);
	glCompileShader(vshader_id);

	const GLuint fshader_id = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fshader_string =
	"#version 330 core\n"
	"in vec2 f_tex_coords;\n"
	"out vec4 FragColor;\n"
	"uniform sampler2D texture0;\n"
	"void main () {\n"
	"	FragColor = texture(texture0, f_tex_coords);\n"
	"}\n";
	glShaderSource(fshader_id, 1, &fshader_string, NULL);
	glCompileShader(fshader_id);

	const GLuint shader_program = glCreateProgram();
	glAttachShader(shader_program, vshader_id);
	glAttachShader(shader_program, fshader_id);
	glLinkProgram(shader_program);

	glDetachShader(shader_program, fshader_id);
	glDetachShader(shader_program, vshader_id);
	glDeleteShader(fshader_id);
	glDeleteShader(vshader_id);

	glUseProgram(shader_program);

	/* shader program uniforms *******************************************/
	GLfloat mvp_matrix[] = {
		 2.0,  0.0, 0.0, 0.0,
		 0.0,  2.0, 0.0, 0.0,
		 0.0,  0.0, 1.0, 0.0,
		-1.0, -1.0, 0.0, 1.0,
	};

	const GLint loc = glGetUniformLocation(shader_program, "mvp_matrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)(mvp_matrix));

	/* display buffer/texture init ***************************************/
	display_buffer = malloc(WIN_WIDTH * WIN_HEIGHT * 4);
	memset(display_buffer, 0, WIN_WIDTH * WIN_HEIGHT * 4);

	for (size_t y = 0; y < WIN_HEIGHT; ++y) {
		size_t offset = y * WIN_WIDTH;
		for (size_t x = 0; x < WIN_WIDTH; ++x) {
			size_t index = (x + offset) * 4;
			display_buffer[index] = map(x, 0, WIN_WIDTH, 0, 255);
			display_buffer[index+1] = map(y, 0, WIN_HEIGHT, 0, 255);
		}
	}

	GLuint display_texture;
	glGenTextures(1, &display_texture);
	glBindTexture(GL_TEXTURE_2D, display_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(
		GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA,
		WIN_WIDTH, WIN_HEIGHT,
		0, GL_RGBA, GL_UNSIGNED_BYTE, display_buffer
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	print_version();

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(shader_program);
		glBindTexture(GL_TEXTURE_2D, display_texture);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);

		glfwSwapBuffers(window);
	}

	glDeleteProgram(shader_program);
	glfwTerminate();

	return 0;
}

void print_version(void) {
	printf("OpenGL %s\n", glGetString(GL_VERSION));

	int major;
	int minor;
	int rev;
	glfwGetVersion(&major, &minor, &rev);
	printf("GLFW %i.%i", major, minor);
	if (rev != 0) {
		printf("revision %i", rev);
	}
	printf("\n");
}
