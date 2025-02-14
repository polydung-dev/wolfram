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

void print_version();
void print_rule_info(struct Options* options);
void get_next_generation(
	uint8_t* dst, const uint8_t* src, size_t w, struct Options* options
);
void save_image(struct Options* options);

float map(float x, float min_i, float max_i, float min_o, float max_o) {
	float range_i = max_i - min_i;
	float range_o = max_o - min_o;
	/* offset, scale, re-offset */
	return (x - min_i) * (range_o / range_i) + min_o;
}

int main(int argc, char* argv[]) {
	struct Options options = {0};
	enum ParseStatus ps = parse_args(&options, argc, argv);
	if (ps == PARSE_HELP) {
		fprintf(stderr, help_text);
		return 0;
	}
	if (ps != PARSE_OK) {
		return 2;
	}

	if (options.mode == MODE_LIST_RULES) {
		print_rule_info(&options);

		return 0;
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

		get_next_generation(next, current, WIN_WIDTH, &options);
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
			save_image(&options);
		}
	}

	free(display_buffer);

	glDeleteTextures(1, &display_texture);
	glDeleteProgram(shader_program);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void get_next_generation_standard(
	uint8_t* dst, const uint8_t* src, size_t w, struct Options* options
) {
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

		bool fill_pixel = ((options->rules[0] >> rule_index) & 1) == 1;
		if (fill_pixel) {
			memset(dst + pixel_index, PIXEL_ON, 3);
		}
	}
}

void get_next_generation_split(
	uint8_t* dst, const uint8_t* src, size_t w, struct Options* options
) {
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

			bool fill_pixel = \
				(options->rules[channel] >> rule_index) & 1;
			dst[pixel_index + channel] = \
				fill_pixel ? PIXEL_ON : PIXEL_OFF;
		}

	}
}

void get_next_generation_directional(
	uint8_t* dst, const uint8_t* src, size_t w, struct Options* options
) {
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

		bool fill_pixel = ((options->rules[0] >> rule_index) & 1) == 1;
		if (fill_pixel) {
			dst[pixel_index  ] = left_set  ? PIXEL_ON : PIXEL_HALF;
			dst[pixel_index+1] = pixel_set ? PIXEL_ON : PIXEL_HALF;
			dst[pixel_index+2] = right_set ? PIXEL_ON : PIXEL_HALF;
		}
	}
}

void get_next_generation(
	uint8_t* dst, const uint8_t* src, size_t w, struct Options* options
) {
	switch (options->mode) {
		default:
		case MODE_STANDARD: {
			get_next_generation_standard(dst, src, w, options);
			break;
		}
		case MODE_SPLIT: {
			get_next_generation_split(dst, src, w, options);
			break;
		}
		case MODE_DIRECTIONAL: {
			get_next_generation_directional(dst, src, w, options);
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

void print_rule_(uint8_t r) {
	printf("%4i ", r);
	for (int i = 7; i >= 0; --i) {
		(i & 4) ? printf("x") : printf("o");
		(i & 2) ? printf("x") : printf("o");
		(i & 1) ? printf("x") : printf("o");
		printf(" ");
	}
	printf("\n");

	printf("     ");
	for (int i = 7; i >= 0; --i) {
		printf(" ");
		((r >> i) & 1) ? printf("x") : printf("o");
		printf(" ");
		printf(" ");
	}
	printf("\n");
}

uint8_t get_mirror_rule(uint8_t r) {
	uint8_t nr = r & 0xa5; /* 0b10100101 already mirrored */

	/* swap 3 and 6 */
	nr |= ((r >> 3) & 1) << 6;
	nr |= ((r >> 6) & 1) << 3;

	/* swap 4 and 1 */
	nr |= ((r >> 4) & 1) << 1;
	nr |= ((r >> 1) & 1) << 4;

	return nr;
}

uint8_t get_complement_rule(uint8_t r) {
	// return ~r;
	uint8_t nr = 0;

	for (int i = 0; i < 8; ++i) {
		nr |= ((r >> i) & 1) << (7 - i);
	}

	return ~nr;
}

void print_rule_info(struct Options* options) {
	uint8_t r = options->rules[0];
	uint8_t m = get_mirror_rule(r);

	printf("Rule\n");
	print_rule_(r);
	printf("\n");

	printf("Mirror\n");
	print_rule_(m);
	printf("\n");

	printf("Complement\n");
	print_rule_(get_complement_rule(r));
	printf("\n");

	printf("Mirror Complement\n");
	print_rule_(get_complement_rule(m));
	printf("\n");
}

char* byte_to_str(uint8_t n) {
	char* s = malloc(4);

	s[0] = '0' +  n / 100;
	s[1] = '0' + (n /  10) % 10;
	s[2] = '0' +  n        % 10;
	s[3] = 0;

	return s;
}

char* make_filename(struct Options* options) {
	char* name_buffer = malloc(128);
	char* p = name_buffer;

	const char* mode_string = modestr(options->mode);
	size_t mode_string_sz = strlen(mode_string);

	int rule_count = 1;
	if (options->mode == MODE_SPLIT) { rule_count = 3; }

	memcpy(p, "rule-", 5);
	p += 5;

	for (int i = 0; i < rule_count; ++i) {
		uint8_t n = options->rules[i];
		char* str = malloc(4);

		str[0] = '0' +  n / 100;
		str[1] = '0' + (n /  10) % 10;
		str[2] = '0' +  n        % 10;

		memcpy(p, str, 3);
		p += 3;
		*p = '-';
		++p;

		free(str);
	}

	memcpy(p, mode_string, mode_string_sz);
	p += mode_string_sz;

	/* standard mode is inverted by default, uninverted by flag */
	if (options->invert ^ (options->mode == MODE_STANDARD)) {
		memcpy(p, "-i", 2);
		p += 2;
	}

	memcpy(p, ".png", 5);
	return name_buffer;
}

void save_image(struct Options* options) {
	uint8_t* pixels = malloc(WIN_WIDTH * WIN_HEIGHT * 3);
	char* filename = make_filename(options);

	glReadBuffer(GL_FRONT);
	glReadPixels(
		0, 0, WIN_WIDTH, WIN_HEIGHT,
		GL_RGB, GL_UNSIGNED_BYTE, pixels
	);
	stbi_flip_vertically_on_write(1);
	stbi_write_png(
		filename, WIN_WIDTH, WIN_HEIGHT, 3, pixels, WIN_WIDTH * 3
	);

	free(filename);
	free(pixels);
}
