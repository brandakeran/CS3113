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

class Box {
public:
	Box(float x, float y, float h, float w) {
		posX = x;
		posY = y;
		height = h;
		width = w;
	};
	float const getPosX() {
		return posX;
	};
	float const getPosY() {
		return posY;
	};
	float const getHeight() {
		return height;
	};
	float const getWidth() {
		return width;
	};

	void changeX(float amount) {
		posX += amount;
		if (posX > 17.75 - width) {
			posX = 17.75 - width;
		}
		else if (posX < -17.75 + width) {
			posX = -17.75 + width;
		}
	}
	void changeY(float amount) {
		posY += amount;
		if (posY > 7.0 - height) {
			posY = 7.0 - height;
		}
		else if (posY < -7.0 + height) {
			posY = -7.0 + height;
		}
	}
	void reset() {
		posX = 0.0f;
		posY = 0.0f;
	}
private:
	float posX = 0;
	float posY = 0;
	float height = 0;
	float width = 0;
};

SDL_Window* displayWindow;

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 1280, 720);

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	ShaderProgram programTextured;
	programTextured.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint characters = LoadTexture(RESOURCE_FOLDER"font1.png");

	float lastFrameTicks = 0.0f;
	float directionX = 1.0f;
	float directionY = 1.0f;

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix; 
	projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);
	glUseProgram(program.programID);

	Box paddle1 = Box(12.5f, 0.0f, 1.0f, 0.25f);
	Box paddle2 = Box(-12.5f, 0.0f, 1.0f, 0.25f);
	Box ball = Box(0.0f, 0.0f, 0.25f, 0.25f);

	int leftScore = 0;
	int rightScore = 0;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		modelMatrix.Identity();
		modelMatrix.Translate(0.0f, 7.25f, 0.0f);
		program.SetModelMatrix(modelMatrix);
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float verticesBarrier1[] = { -12.5f, 0.25f, -12.5, -0.25, 12.5f, 0.25f, 12.5f, 0.25f, -12.5f, -0.25f, 12.5f, -0.25f  };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verticesBarrier1);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		modelMatrix.Identity();
		
		modelMatrix.Translate(0.0f, -7.25f, 0.0f);
		program.SetModelMatrix(modelMatrix);
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float verticesBarrier2[] = { -12.5f, 0.25f, -12.5, -0.25, 12.5f, 0.25f, 12.5f, 0.25f, -12.5f, -0.25f, 12.5f, -0.25f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verticesBarrier2);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		modelMatrix.Identity();

		// Right Player Movement
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_UP]){
			paddle1.changeY(elapsed * 7);
		}
		else if (keys[SDL_SCANCODE_DOWN]) {
			paddle1.changeY(elapsed * -7);
		}

		program.SetModelMatrix(modelMatrix);
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float x = paddle1.getPosX();
		float y = paddle1.getPosY();
		float h = paddle1.getHeight();
		float w = paddle1.getWidth();
		float vertices1[] = { x+w, y+h, x-w, y+h, x-w, y-h, x-w, y-h, x+w, y-h, x+w, y+h};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		
		// Left Player Movement
		if (keys[SDL_SCANCODE_W]) {
			paddle2.changeY(elapsed * 7);
		}
		else if (keys[SDL_SCANCODE_S]) {
			paddle2.changeY(elapsed * -7);
		}
		
		program.SetModelMatrix(modelMatrix);
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float x2 = paddle2.getPosX();
		float y2 = paddle2.getPosY();
		float h2 = paddle2.getHeight();
		float w2 = paddle2.getWidth();
		float vertices2[] = { x2+w2, y2+h2, x2-w2, y2+h2, x2-w2, y2-h2, x2-w2, y2-h2, x2+w2, y2-h2, x2+w2, y2+h2 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		// Ball Movement
		program.SetModelMatrix(modelMatrix);
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float x3 = ball.getPosX();
		float y3 = ball.getPosY();
		float h3 = ball.getHeight();
		float w3 = ball.getWidth();

		if ((x3 - w3 < x2 + w2 && x3 + w3 > x2 - w2 && y3 - h3 < y2 + h2 && y3 + h3 > y2 - h2) || (x3 + w3 > x - w && x3 - w3 < x + w && y3 - h3 < y + h && y3 + h3 > y - h)) {
			directionX = directionX * -1.0;
		}
		else if (x3 >= 12.5 - w3) {
			std::cout << "Left Player Scores!" << std::endl;
			leftScore = leftScore + 1;
			ball.reset();
		}
		else if (x3 <= -12.5 + w3) {
			std::cout << "Right Player Scores!" << std::endl;
			rightScore = rightScore + 1;
			ball.reset();
		}
		if (y3 >= 7.0 - h3 || y3 <= -7.0 + h3) {
			directionY = directionY * -1.0;
		}
		ball.changeX(cos(45)*elapsed * 12 * directionX);
		ball.changeY(sin(45)*elapsed * 12 * directionY);
		float vertices3[] = { x3 + w3, y3 + h3, x3 - w3, y3 + h3, x3 - w3, y3 - h3, x3 - w3, y3 - h3, x3 + w3, y3 - h3, x3 + w3, y3 + h3 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		// Displaying Score

		//char scoreL = (char)leftScore;
		if (leftScore > 9) {
			leftScore = 9;
		}
			int asciiL = leftScore + 48;

			float u = (float)(((int)asciiL) % 16) / 16.0f;
			float v = (float)(((int)asciiL) / 16) / 16.0f;

			float spriteHeight = 1.0 / 16.0;
			float spriteWidth = 1.0 / 16.0;


			modelMatrix.Identity();
			modelMatrix.Translate(-2.0f, 6.0f, 0.0f);
			glUseProgram(programTextured.programID);
			programTextured.SetModelMatrix(modelMatrix);
			programTextured.SetProjectionMatrix(projectionMatrix);
			programTextured.SetViewMatrix(viewMatrix);
			glBindTexture(GL_TEXTURE_2D, characters);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			float vertices4[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
			glVertexAttribPointer(programTextured.positionAttribute, 2, GL_FLOAT, false, 0, vertices4);
			glEnableVertexAttribArray(programTextured.positionAttribute);
			float texCoords[] = { u, v + spriteHeight, u + spriteHeight, v, u, v, u + spriteWidth, v, u, v + spriteHeight, u + spriteHeight, v + spriteWidth };
			glVertexAttribPointer(programTextured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(programTextured.texCoordAttribute);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(programTextured.texCoordAttribute);
			glDisableVertexAttribArray(programTextured.positionAttribute);
			modelMatrix.Identity();
		

		if (rightScore > 9) {
			rightScore = 9;
		}
			int asciiR = rightScore + 48;

			u = (float)(((int)asciiR) % 16) / 16.0f;
			v = (float)(((int)asciiR) / 16) / 16.0f;

			spriteHeight = 1.0 / 16.0;
			spriteWidth = 1.0 / 16.0;


			modelMatrix.Identity();
			modelMatrix.Translate(2.0f, 6.0f, 0.0f);
			programTextured.SetModelMatrix(modelMatrix);
			programTextured.SetProjectionMatrix(projectionMatrix);
			programTextured.SetViewMatrix(viewMatrix);
			glBindTexture(GL_TEXTURE_2D, characters);


			float vertices5[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
			glVertexAttribPointer(programTextured.positionAttribute, 2, GL_FLOAT, false, 0, vertices5);
			glEnableVertexAttribArray(programTextured.positionAttribute);
			float texCoords1[] = { u, v + spriteHeight, u + spriteHeight, v, u, v, u + spriteWidth, v, u, v + spriteHeight, u + spriteHeight, v + spriteWidth };
			glVertexAttribPointer(programTextured.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords1);
			glEnableVertexAttribArray(programTextured.texCoordAttribute);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(programTextured.texCoordAttribute);
			glDisableVertexAttribArray(programTextured.positionAttribute);
			modelMatrix.Identity();
		
		glDisable(GL_BLEND);


		SDL_GL_SwapWindow(displayWindow);
	}

	

	SDL_Quit();
	return 0;
}
