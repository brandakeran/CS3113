#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	ShaderProgram programTextured;
	programTextured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint sun = LoadTexture(RESOURCE_FOLDER"sun.png");
	GLuint cloud = LoadTexture(RESOURCE_FOLDER"cloud.png");
	GLuint plane = LoadTexture(RESOURCE_FOLDER"planeRed1.png");
	
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix; 
	Matrix sunMatrix;
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0, 2.0, -1.0f, 1.0f);
	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.60f, 0.82f, 1.0f, 0.0f);

	float lastFrameTicks = 0.0f;
	float shift = 0;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		modelMatrix.Identity();
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		
		program.SetColor(1.0f, 0.0f, 0.0f, 0.0f);
		float vertices1[] = { 0.5f, -0.5f, 0.0f, 0.25f, -0.5f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(program.positionAttribute);
		
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float vertices2[] = { 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -1.5f, -0.5f, -1.5f, 0.5f, -1.5f, 0.5f, -0.5f};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		program.SetColor(0.0f, 1.0f, 0.0f, 0.0f);
		float vertices3[] = { 5.0f, -1.5f, -5.0f, -1.5f, -5.0f, -2.0f, -5.0f, -2.0f, 5.0f, -2.0f, 5.0f, -1.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		glUseProgram(programTextured.programID);
		programTextured.SetModelMatrix(modelMatrix);
		programTextured.SetProjectionMatrix(projectionMatrix);
		programTextured.SetViewMatrix(viewMatrix);
		glBindTexture(GL_TEXTURE_2D, sun);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float vertices4[] = { -2.5f, 1.0f, -1.5f, 1.0f, -1.5f, 2.0f, -2.5f, 1.0f, -1.5f, 2.0f, -2.5f, 2.0f };
		glVertexAttribPointer(programTextured.positionAttribute, 2, GL_FLOAT, false, 0, vertices4);
		glEnableVertexAttribArray(programTextured.positionAttribute);
		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(programTextured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(programTextured.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(programTextured.texCoordAttribute);
		glDisableVertexAttribArray(programTextured.positionAttribute);

		modelMatrix.Identity();
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		shift += elapsed;
		if (shift == 4.0f) {
			shift = -4.0f;
		}
		modelMatrix.Translate(shift, 1.5f, 0.0f);
		modelMatrix.Scale(0.5f, 0.5f, 0.0f);
		programTextured.SetModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, plane);
		float vertices7[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
		glVertexAttribPointer(programTextured.positionAttribute, 2, GL_FLOAT, false, 0, vertices7);
		glEnableVertexAttribArray(programTextured.positionAttribute);
		float texCoords4[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(programTextured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords4);
		glEnableVertexAttribArray(programTextured.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(programTextured.texCoordAttribute);
		glDisableVertexAttribArray(programTextured.positionAttribute);

		modelMatrix.Identity();
		modelMatrix.Translate(0.0f, 1.0f, 0.0f);
		programTextured.SetModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, cloud);
		float vertices5[] = { -0.5f, -0.25f, 0.5f, -0.25f, 0.5f, 0.25f, -0.5f, -0.25f, 0.5f, 0.25f, -0.5f, 0.25f };
		glVertexAttribPointer(programTextured.positionAttribute, 2, GL_FLOAT, false, 0, vertices5);
		glEnableVertexAttribArray(programTextured.positionAttribute);
		float texCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(programTextured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(programTextured.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(programTextured.texCoordAttribute);
		glDisableVertexAttribArray(programTextured.positionAttribute);
		
		modelMatrix.Identity();

		modelMatrix.Translate(2.0f, 1.5f, 0.0f);
		programTextured.SetModelMatrix(modelMatrix);
		float vertices6[] = { -0.5f, -0.25f, 0.5f, -0.25f, 0.5f, 0.25f, -0.5f, -0.25f, 0.5f, 0.25f, -0.5f, 0.25f };
		glVertexAttribPointer(programTextured.positionAttribute, 2, GL_FLOAT, false, 0, vertices6);
		glEnableVertexAttribArray(programTextured.positionAttribute);
		float texCoords3[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(programTextured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords3);
		glEnableVertexAttribArray(programTextured.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(programTextured.texCoordAttribute);
		glDisableVertexAttribArray(programTextured.positionAttribute);

		glDisable(GL_BLEND);
		
		SDL_GL_SwapWindow(displayWindow);
	}

	

	SDL_Quit();
	return 0;
}
