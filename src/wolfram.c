#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "stb/stb_image_write.h"

#include "options.h"

#define WIN_WIDTH 640
#define WIN_HEIGHT 480
#define TITLE "wolfram"

uint8_t PIXEL_OFF  = 0x00;
uint8_t PIXEL_HALF = 0x45;
uint8_t PIXEL_ON   = 0xff;

void print_version(void);
void get_next_generation(uint8_t* dst, const uint8_t* src, size_t w);
void save_image();

float map(float x, float min_i, float max_i, float min_o, float max_o) {
	float range_i = max_i - min_i;
	float range_o = max_o - min_o;
	/* offset, scale, re-offset */
	return (x - min_i) * (range_o / range_i) + min_o;
}

struct Options options = {0};

int main(int argc, char* argv[]) {
	enum ParseStatus ps = parse_args(&options, argc, argv);
	if (ps == PARSE_HELP) {
		fprintf(stderr, help_text);
		exit(0);
	}
	if (ps != PARSE_OK) {
		exit(2);
	}

	if (options.invert) {
		PIXEL_OFF  = 0xff - PIXEL_OFF;
		PIXEL_HALF = 0xff - PIXEL_HALF;
		PIXEL_ON   = 0xff - PIXEL_ON;
	}

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
	uint8_t* display_buffer = malloc(WIN_WIDTH * WIN_HEIGHT * 3);
	memset(display_buffer, PIXEL_OFF,  WIN_WIDTH * WIN_HEIGHT * 3);
	memset(display_buffer + (WIN_WIDTH * 3 / 2), PIXEL_ON, 3);

	for (size_t i = 0; i < WIN_HEIGHT - 1; ++i) {
		uint8_t* current = display_buffer + ((WIN_WIDTH * 3) * i);
		uint8_t* next = current + (WIN_WIDTH * 3);

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
		GL_TEXTURE_2D, 0, GL_RGB,
		WIN_WIDTH, WIN_HEIGHT,
		0, GL_RGB, GL_UNSIGNED_BYTE, display_buffer
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

void get_next_generation_standard(uint8_t* dst, const uint8_t* src, size_t w) {
	size_t last_pixel_index = (w - 1) * 3;
	for (size_t i = 0; i < w; ++i) {
		size_t pixel_index = i * 3;
		size_t left_pixel  = pixel_index - 3;
		size_t right_pixel = pixel_index + 3;

		/* wrap around edges */
		if (pixel_index == 0) {
			left_pixel = last_pixel_index;
		} else if (pixel_index == last_pixel_index) {
			right_pixel = 0;
		}

		/* convert to rule index */
		char rule_index = 0;
		if (src[left_pixel] == PIXEL_ON) {
			rule_index |= 4;
		}
		if (src[pixel_index] == PIXEL_ON) {
			rule_index |= 2;
		}
		if (src[right_pixel] == PIXEL_ON) {
			rule_index |= 1;
		}

		bool fill_pixel = ((options.rules[0] >> rule_index) & 1) == 1;
		if (fill_pixel) {
			memset(dst + pixel_index, PIXEL_ON, 3);
		}
	}
}

void get_next_generation_split(uint8_t* dst, const uint8_t* src, size_t w) {
	size_t last_pixel_index = (w - 1) * 3;
	for (size_t i = 0; i < w; ++i) {
		size_t pixel_index = i * 3;
		size_t left_pixel  = pixel_index - 3;
		size_t right_pixel = pixel_index + 3;

		/* wrap around edges */
		if (pixel_index == 0) {
			left_pixel = last_pixel_index;
		} else if (pixel_index == last_pixel_index) {
			right_pixel = 0;
		}

		/* convert to rule index */
		for (int channel = 0; channel < 3; ++channel) {
			uint8_t rule_index = 0;
			if (src[left_pixel + channel] == PIXEL_ON) {
				rule_index |= 4;
			}
			if (src[pixel_index + channel] == PIXEL_ON) {
				rule_index |= 2;
			}
			if (src[right_pixel + channel] == PIXEL_ON) {
				rule_index |= 1;
			}

			bool fill_pixel = (options.rules[channel] >> rule_index) & 1;
			dst[pixel_index + channel] = fill_pixel ? PIXEL_ON : PIXEL_OFF;
		}

	}
}

void get_next_generation_directional(uint8_t* dst, const uint8_t* src, size_t w) {
	size_t last_pixel_index = (w - 1) * 3;
	for (size_t i = 0; i < w; ++i) {
		size_t pixel_index = i * 3;
		size_t left_pixel  = pixel_index - 3;
		size_t right_pixel = pixel_index + 3;

		/* wrap around edges */
		if (pixel_index == 0) {
			left_pixel = last_pixel_index;
		} else if (pixel_index == last_pixel_index) {
			right_pixel = 0;
		}

		bool pixel_set = false;
		bool left_set  = false;
		bool right_set = false;

		for (int channel = 0; channel < 3; ++channel) {
			if (src[left_pixel + channel] != PIXEL_OFF) {
				left_set = true;
			}
			if (src[pixel_index + channel] != PIXEL_OFF) {
				pixel_set = true;
			}
			if (src[right_pixel + channel] != PIXEL_OFF) {
				right_set = true;
			}
		}

		/* convert to rule index */
		uint8_t rule_index = 0;
		if (left_set) {
			rule_index |= 4;
		}
		if (pixel_set) {
			rule_index |= 2;
		}
		if (right_set) {
			rule_index |= 1;
		}

		bool fill_pixel = ((options.rules[0] >> rule_index) & 1) == 1;
		if (fill_pixel) {
			dst[pixel_index    ] = left_set  ? PIXEL_ON : PIXEL_HALF;
			dst[pixel_index + 1] = pixel_set ? PIXEL_ON : PIXEL_HALF;
			dst[pixel_index + 2] = right_set ? PIXEL_ON : PIXEL_HALF;
		}
	}
}

void get_next_generation(uint8_t* dst, const uint8_t* src, size_t w) {
	switch (options.mode) {
		default:
		case MODE_STANDARD: {
			get_next_generation_standard(dst, src, w);
			break;
		}
		case MODE_SPLIT: {
			get_next_generation_split(dst, src, w);
			break;
		}
		case MODE_DIRECTIONAL: {
			get_next_generation_directional(dst, src, w);
			break;
		}
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
