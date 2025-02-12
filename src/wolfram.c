#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "stb/stb_image_write.h"

#define WIN_WIDTH 640
#define WIN_HEIGHT 480
#define TITLE "wolfram"

#define PIXEL_SET   0xffffffff
#define PIXEL_CLEAR 0x000000ff

uint8_t rule = 0;

void print_version(void);
void get_next_generation(uint32_t* dst, const uint32_t* src, size_t w);
void save_image();

float map(float x, float min_i, float max_i, float min_o, float max_o) {
	float range_i = max_i - min_i;
	float range_o = max_o - min_o;
	/* offset, scale, re-offset */
	return (x - min_i) * (range_o / range_i) + min_o;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("usage: %s <rule>\n", argv[0]);
		exit(1);
	}

	long r = strtol(argv[1], NULL, 10);
	if (r < 0 || r > 255) {
		printf("Please enter a rule number between 0 and 255\n");
		exit(1);
	}

	rule = (uint8_t)r;

	glfwInit();
	/* 4.3 required for debug context */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(
		WIN_WIDTH, WIN_HEIGHT, TITLE, NULL, NULL
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
		 0.0, -2.0, 0.0, 0.0,
		 0.0,  0.0, 1.0, 0.0,
		-1.0,  1.0, 0.0, 1.0,
	};

	const GLint loc = glGetUniformLocation(shader_program, "mvp_matrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)(mvp_matrix));

	/* display buffer/texture init ***************************************/
	uint32_t* display_buffer = malloc(WIN_WIDTH * WIN_HEIGHT * 4);
	for (int i = 0; i < WIN_WIDTH * WIN_HEIGHT; ++i) {
		display_buffer[i] = PIXEL_SET;
	}
	display_buffer[WIN_WIDTH / 2] = PIXEL_CLEAR;

	for (size_t i = 0; i < WIN_HEIGHT - 1; ++i) {
		uint32_t* current = display_buffer + (WIN_WIDTH * i);
		uint32_t* next = current + WIN_WIDTH;

		get_next_generation(next, current, WIN_WIDTH);
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
		GL_TEXTURE_2D, 0, GL_RGBA8,
		WIN_WIDTH, WIN_HEIGHT,
		0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, display_buffer
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	print_version();

	int x = 1;

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(shader_program);
		glBindTexture(GL_TEXTURE_2D, display_texture);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);

		glfwSwapBuffers(window);

		if (x != 0) {
			x = 0;
			save_image();
		}
	}

	free(display_buffer);

	glDeleteProgram(shader_program);
	glfwTerminate();

	return 0;
}

void get_next_generation(uint32_t* dst, const uint32_t* src, size_t w) {
	for (size_t i = 0; i < w; ++i) {
		size_t left = i - 1;
		size_t right = i + 1;

		/* wrap around edges */
		if (i == 0) {
			left = w - 1;
		} else if (i == w - 1) {
			right = 0;
		}

		/* convert to rule index */
		char ri = 0;
		if (src[left] == PIXEL_CLEAR) {
			ri |= 4;
		}
		if (src[i] == PIXEL_CLEAR) {
			ri |= 2;
		}
		if (src[right] == PIXEL_CLEAR) {
			ri |= 1;
		}

		dst[i] = ((rule >> ri) & 1) ? PIXEL_CLEAR : PIXEL_SET;
	}
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

void save_image() {
	uint8_t* pixels = malloc(WIN_WIDTH * WIN_HEIGHT * 4);
	glReadBuffer(GL_FRONT);
	glReadPixels(
		0, 0, WIN_WIDTH, WIN_HEIGHT,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels
	);
	stbi_flip_vertically_on_write(1);
	stbi_write_png(
		"out.png", WIN_WIDTH, WIN_HEIGHT, 4, pixels, WIN_WIDTH * 4
	);

	free(pixels);
}
