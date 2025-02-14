#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "stb/stb_image_write.h"

#include "eca.h"
#include "options.h"

static const int window_width  = 640;
static const int window_height = 480;
static const int channel_count = 3;
static const char* title = "elementary cellular automata";

enum StatusCode {
	RV_OK = 0,
	RV_BAD_ARGS,
	RV_GLFW_ERR,
	RV_GLAD_ERR,
	RV_EXIT_ERR
};

void print_rule(uint8_t r);
void print_rule_variants(struct Options* options);
void save_image(struct Options* options);

int main(int argc, char* argv[]) {
	/* argument parsing and function selection ***************************/
	struct Options options = {0};
	enum ParseStatus ps = parse_args(&options, argc, argv);

	if (ps == PARSE_HELP) {
		fprintf(stderr, help_text);
		return RV_OK;
	}

	if (ps != PARSE_OK) {
		return RV_BAD_ARGS;
	}

	if (options.mode == MODE_LIST_RULES) {
		print_rule_variants(&options);

		return RV_OK;
	}

	/* standard display draws black pixels on a white background */
	if (options.mode == MODE_STANDARD) {
		pixel_off  = 0xff - pixel_off;
		pixel_half = 0xff - pixel_half;
		pixel_on   = 0xff - pixel_on;
	}

	/* initialise opengl and create window *******************************/
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(
		window_width, window_height, title, NULL, NULL
	);

	if (window == NULL) {
		const char* msg = NULL;
		glfwGetError(&msg);
		fprintf(stderr, "error: glfwCreateWindow: %s\n", msg);
		return RV_GLFW_ERR;
	}

	/* ensure glfwTerminate is called on exit */
	if (atexit(glfwTerminate) != 0) {
		fprintf(stderr, "error: could not register exit function\n");
		glfwTerminate();
		return RV_EXIT_ERR;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		fprintf(stderr, "error: gladLoadGL\n");
		return RV_GLAD_ERR;
	}

	/* shader program ****************************************************/
	GLuint vshader_id = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vshader_string =
		"#version 330 core\n"
		"layout (location = 0) in vec2 a_position;\n"
		"out vec2 f_tex_coords;\n"
		"uniform mat4 matrix;\n"
		"void main () {\n"
		"	f_tex_coords = a_position;\n"
		"	gl_Position = matrix * vec4(a_position, 0.0, 1.0);\n"
		"}\n";
	glShaderSource(vshader_id, 1, &vshader_string, NULL);
	glCompileShader(vshader_id);

	GLuint fshader_id = glCreateShader(GL_FRAGMENT_SHADER);
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

	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vshader_id);
	glAttachShader(program_id, fshader_id);
	glLinkProgram(program_id);

	glDetachShader(program_id, fshader_id);
	glDetachShader(program_id, vshader_id);
	glDeleteShader(fshader_id);
	glDeleteShader(vshader_id);

	glUseProgram(program_id);
	GLfloat matrix[] = {
		 2.0,  0.0, 0.0, 0.0,
		 0.0, -2.0, 0.0, 0.0,
		 0.0,  0.0, 1.0, 0.0,
		-1.0,  1.0, 0.0, 1.0,
	};

	glUniformMatrix4fv(
		glGetUniformLocation(program_id, "matrix"),
		1, GL_FALSE, (GLfloat*)(matrix)
	);

	/* vertex array ******************************************************/
	GLfloat vertices[] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0};
	GLubyte indices[] = {0, 1, 2, 0, 2, 3};

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0
	);

	GLuint ebo = 0;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
		GL_STATIC_DRAW
	);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* display buffer ****************************************************/
	size_t row_size = window_width * channel_count;
	size_t buffer_size = row_size * window_height;
	uint8_t* display_buffer = malloc(buffer_size);
	memset(display_buffer, pixel_off, buffer_size);

	/* set initial generation */
	eca_init_fn* init_fn = NULL;
	switch (options.initial) {
		default:
		case INIT_STANDARD:  init_fn = eca_initialise;           break;
		case INIT_ALTERNATE: init_fn = eca_initialise_alternate; break;
		case INIT_RANDOM:    init_fn = eca_initialise_random;    break;
	}

	init_fn(display_buffer, window_width, channel_count);

	/* generation all */
	eca_gen_fn* gen_fn = NULL;
	switch (options.mode) {
		default:
		case MODE_STANDARD:    gen_fn = eca_generate;             break;
		case MODE_SPLIT:       gen_fn = eca_generate_split;       break;
		case MODE_DIRECTIONAL: gen_fn = eca_generate_directional; break;
	}

	for (size_t i = 0; i < window_height - 1; ++i) {
		uint8_t* current = display_buffer + ((row_size) * i);
		uint8_t* next = current + (row_size);
		gen_fn(next, current, window_width, options.rules);
	}

	/* display texture ***************************************************/
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
		window_width, window_height,
		0, GL_RGB, GL_UNSIGNED_BYTE, display_buffer
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	/* main loop *********************************************************/
	int x = 1;
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program_id);
		glBindTexture(GL_TEXTURE_2D, display_texture);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);

		glfwSwapBuffers(window);

		if (x != 0) {
			x = 0;
			save_image(&options);
		}
	}

	/* cleanup ***********************************************************/
	free(display_buffer);

	glDeleteTextures(1, &display_texture);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program_id);
	glfwDestroyWindow(window);
	glfwTerminate();

	return RV_OK;
}

void print_rule(uint8_t r) {
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

void print_rule_variants(struct Options* options) {
	uint8_t r = options->rules[0];
	uint8_t m = get_mirror_rule(r);

	printf("Rule\n");
	print_rule(r);
	printf("\n");

	printf("Mirror\n");
	print_rule(m);
	printf("\n");

	printf("Complement\n");
	print_rule(get_complement_rule(r));
	printf("\n");

	printf("Mirror Complement\n");
	print_rule(get_complement_rule(m));
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

	memcpy(p, "rule", 4);
	p += 4;

	int rule_count = 1;
	if (options->mode == MODE_SPLIT) { rule_count = channel_count; }
	for (int i = 0; i < rule_count; ++i) {
		uint8_t n = options->rules[i];
		char* str = malloc(4);

		str[0] = '0' +  n / 100;
		str[1] = '0' + (n /  10) % 10;
		str[2] = '0' +  n        % 10;

		*p++ = '-';
		memcpy(p, str, 3);
		p += 3;

		free(str);
	}

	if (options->mode != MODE_STANDARD) {
		const char* mode_string = modestr(options->mode);
		size_t mode_string_sz = strlen(mode_string);

		*p++ = '-';
		memcpy(p, mode_string, mode_string_sz);
		p += mode_string_sz;
	}

	if (options->initial != INIT_STANDARD) {
		const char* init_string = initstr(options->initial);
		size_t init_string_sz = strlen(init_string);

		*p++ = '-';
		memcpy(p, init_string, init_string_sz);
		p += init_string_sz;
	}

	memcpy(p, ".png", 5);
	return name_buffer;
}

void save_image(struct Options* options) {
	size_t row_size = window_width * channel_count;
	size_t buffer_size = row_size * window_height;

	uint8_t* pixels = malloc(buffer_size);
	char* filename = make_filename(options);

	glReadBuffer(GL_FRONT);
	glReadPixels(
		0, 0, window_width, window_height,
		GL_RGB, GL_UNSIGNED_BYTE, pixels
	);
	stbi_flip_vertically_on_write(1);
	stbi_write_png(
		filename, window_width, window_height, channel_count,
		pixels, row_size
	);

	free(filename);
	free(pixels);
}
