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
#include <vector>
#include <string>
#include <random>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

enum GameMode{STATE_MAIN_MENU, STATE_GAME, STATE_GAME_OVER, STATE_GAME_WIN};

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

class SheetSprite {
public: 
	SheetSprite(){}
	SheetSprite(unsigned int textureID1, float u1, float v1, float w, float h, float s) {
		textureID = textureID1;
		u = u1;
		v = v1;
		width = w;
		height = h;
		size = s;
	}

	void Draw(ShaderProgram &program, float x, float y) {
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, textureID);

		
		GLfloat texCoords[] = { u, v + height, u + width, v, u, v, u + width, v, u, v + height, u + width, v + height };

		float aspect = width / height;
		float verticies[] = {
			-0.5f* size * aspect, -0.5f*size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f* size * aspect, 0.5f* size,
			0.5f* size * aspect, 0.5f* size,
			-0.5f* size * aspect, -0.5f* size,
			0.5f* size * aspect, -0.5f* size};

		modelMatrix.Identity();
		modelMatrix.Translate(x, y, 0.0f);
		glUseProgram(program.programID);
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verticies);
		glEnableVertexAttribArray(program.positionAttribute);
glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
glEnableVertexAttribArray(program.texCoordAttribute);
glDrawArrays(GL_TRIANGLES, 0, 6);
glDisableVertexAttribArray(program.texCoordAttribute);
glDisableVertexAttribArray(program.positionAttribute);
modelMatrix.Identity();

	}

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};
class Vector3 {
public:
	Vector3(float x1, float y1, float z1) {
		x = x1, y = y1, z = z1;
	}
	float x;
	float y;
	float z;

};
class Entity {
public:
	Entity(){}
	Entity(Vector3 position, Vector3 velocity, Vector3 size, bool control) : position(position), velocity(velocity), size(size) {
		controlled = control;
	}
	void Draw(ShaderProgram &program) {
		sprite.Draw(program, position.x, position.y);
	}
	void Update(float elapsed) {
		position.x += (velocity.x * elapsed);
		position.y += (velocity.y * elapsed);
	}
	void Shoot(float direction, std::vector<Entity> &bullets, unsigned int textureID) {
		
		Entity bullet = Entity(Vector3(position.x, position.y + 2 * size.y * direction, position.z), Vector3(0.0f, 10.0f*direction, 0.0f), Vector3(((9.0f / 54.0f) / 2)*0.5, (0.5)*0.5, 0.0f), false);
		if (direction = 1.0) {
			bullet.sprite = SheetSprite(textureID, 858.0f / 1024.0f, 230.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.5);
		}
		else {
			bullet.sprite = SheetSprite(textureID, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.5);
		}
		bullets.push_back(bullet);
	}
	Vector3 position = Vector3(0.0, 0.0, 0.0);
	Vector3 velocity = Vector3(0.0, 0.0, 0.0);
	Vector3 size = Vector3(0.0, 0.0, 0.0);
	SheetSprite sprite;
	bool controlled;
	bool active = true;
};

bool collision(Entity checker, Entity checkee) {
	if ((checker.position.x + checker.size.x) > (checkee.position.x - checkee.size.x) && (checker.position.x - checker.size.x) < (checkee.position.x + checkee.size.x) && (checker.position.y + checker.size.y) > (checkee.position.y - checkee.size.y) && (checker.position.y - checker.size.y) < (checkee.position.y + checkee.size.y)) {
		return true;
	}
	return false;
}

class Menu {
public:
	Menu() {};
	void MainDisplay(std::string mmessage) {
		mainMessage = mmessage;
	}
	void OtherDisplay(std::string omessage) {
		message = omessage;
	}
	void Render(GLuint font, ShaderProgram program) {
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);

		float start = 0.0f - mainMessage.length();

		for (int i = 0; i < mainMessage.length(); i++) {
			int ascii = int(mainMessage[i]);

			float u = (float)(((int)ascii) % 16) / 16.0f;
			float v = (float)(((int)ascii) / 16) / 16.0f;

			float spriteHeight = 1.0 / 16.0;
			float spriteWidth = 1.0 / 16.0;



			modelMatrix.Identity();
			modelMatrix.Translate(start + i, 8.0f, 0.0f);
			glUseProgram(program.programID);
			program.SetModelMatrix(modelMatrix);
			program.SetProjectionMatrix(projectionMatrix);
			program.SetViewMatrix(viewMatrix);
			glBindTexture(GL_TEXTURE_2D, font);

			float vertices[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
			glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
			glEnableVertexAttribArray(program.positionAttribute);
			float texCoords[] = { u, v + spriteHeight, u + spriteHeight, v, u, v, u + spriteWidth, v, u, v + spriteHeight, u + spriteHeight, v + spriteWidth };
			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(program.texCoordAttribute);
			glDisableVertexAttribArray(program.positionAttribute);
			modelMatrix.Identity();

		}

		float start2 = 0.0f - message.length() / 2;

		for (int i = 0; i < message.length(); i++) {
			int ascii = int(message[i]);

			float u = (float)(((int)ascii) % 16) / 16.0f;
			float v = (float)(((int)ascii) / 16) / 16.0f;

			float spriteHeight = 1.0 / 16.0;
			float spriteWidth = 1.0 / 16.0;



			modelMatrix.Identity();
			modelMatrix.Translate(start2 + i, -7.0f, 0.0f);
			glUseProgram(program.programID);
			program.SetModelMatrix(modelMatrix);
			program.SetProjectionMatrix(projectionMatrix);
			program.SetViewMatrix(viewMatrix);
			glBindTexture(GL_TEXTURE_2D, font);

			float vertices[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
			glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
			glEnableVertexAttribArray(program.positionAttribute);
			float texCoords[] = { u, v + spriteHeight, u + spriteHeight, v, u, v, u + spriteWidth, v, u, v + spriteHeight, u + spriteHeight, v + spriteWidth };
			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(program.texCoordAttribute);
			glDisableVertexAttribArray(program.positionAttribute);
			modelMatrix.Identity();
		}
	}
	std::string mainMessage;
	std::string message;
};
class GameState {
public: 
	GameState(){}
	void RenderGame(ShaderProgram &textureID){
		player.Draw(textureID);
		for (int i = 0; i < entities.size(); i++) {
			for (int j = 0; j < entities[i].size(); j++) {
				if (entities[i][j].active)
					entities[i][j].Draw(textureID);
			}
		}
		for (int b = 0; b < bullets.size(); b++) {
			bullets[b].Draw(textureID);
		}
	}
	void UpdateGame(GameMode &mode, float& lastFrameTicks, float& playerShootTime, float& enemyShootTime, GLuint& ships, float elapsed){

		playerShootTime += elapsed;



		for (int i = 0; i < entities.size(); i++) {
			for (int j = 0; j < entities[i].size(); j++) {
				entities[i][j].Update(elapsed);
				if (entities[i][j].position.x <= -15.0f) {
					for (int h = 0; h < entities[i].size(); h++) {
						entities[i][h].position.x += abs(15.0f + entities[i][j].position.x); // adjusts how much it went over by
						entities[i][h].velocity.x *= -1.0f;
						entities[i][h].position.y -= 1.5f;
					}
				}
				else if (entities[i][j].position.x >= 15.0f) {
					for (int h = 0; h < entities[i].size(); h++) {
						entities[i][h].position.x -= abs(entities[i][j].position.x - 15.0f);
						entities[i][h].velocity.x *= -1.0f;
						entities[i][h].position.y -= 1.5f;
					}
				}
				if (i == 0) {
					if (entities[i][j].active == true) {
						if (enemyShootTime >= 1.0 && rand() % 10 == 1) {
							entities[i][j].Shoot(-1.0f, bullets, ships);
							enemyShootTime = 0.0;
						}
					}
				}
				else if (entities[i - 1][j].active == false && entities[i][j].active == true) {
					if (enemyShootTime >= 1.0 && rand() % 10 == 1) {
						entities[i][j].Shoot(-1.0f, bullets, ships);
						enemyShootTime = 0.0;
					}
				}
			}
		}
		enemyShootTime += elapsed;

		bool bulletHit = false;
		for (int b = 0; b < bullets.size(); b++) {
			bullets[b].Update(elapsed);
			if (collision(bullets[b], player)) {
				mode = STATE_GAME_OVER;
			}
			for (int i = 0; i < entities.size(); i++) {
				for (int j = 0; j < entities[i].size(); j++) {
					if (entities[i][j].active == true && collision(bullets[b], entities[i][j])) {
						entities[i][j].active = false;
						bullets.erase(bullets.begin() + b);
						bulletHit = true;
					}
					if (bulletHit) {
						break;
					}
				}
				if (bulletHit) {
					break;
				}
			}
		}

		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) {
			player.velocity.x = -4.0f;
			player.Update(elapsed);
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			player.velocity.x = 4.0f;
			player.Update(elapsed);
		}
		else {
			player.velocity.x = 0.0f;
			player.Update(elapsed);
		}
	}
   	bool CheckWin() {
		int i = 0;
		int j = 0;
		while (i < entities.size()) {
			while (j < entities[i].size()) {
				if (entities[i][j].active == true) {
					return false;
				}
				j++;
			}
			i++;
			j = 0;
		}
		return true;
	}
	Entity player;
	std::vector<std::vector<Entity>> entities;
	std::vector<Entity> bullets;
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
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint ships = LoadTexture(RESOURCE_FOLDER"sheet.png");
	GLuint font = LoadTexture(RESOURCE_FOLDER "font1.png");

	float lastFrameTicks = 0.0f;
	float playerShootTime = 0.0;
	float enemyShootTime = 0.0;


	glUseProgram(program.programID);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	
	GameMode mode = STATE_MAIN_MENU;
	Menu mainMenu;
	mainMenu.MainDisplay("SPACE INVADERS");
	mainMenu.OtherDisplay("Press Space To Play");
	Menu winMenu;
	winMenu.MainDisplay("You win!");
	Menu loseMenu;
	loseMenu.MainDisplay("You lost!");
	GameState game;
	game.player = Entity(Vector3(0.0f, -8.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3((99.0f / 75.0f) / 2, 0.5, 0.0f), true);
	game.player.sprite = SheetSprite(ships, 224.0f / 1024.0f, 832.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 1.0);

	game.entities;
	for (int i = 0; i < 5; i++) {
		std::vector<Entity> row;
		for (int j = 0; j < 8; j++) {
			Entity entity = Entity(Vector3((-5.0f + 1.5f*j), (0.0f + 1.5f*i), 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3((93.0f / 84) / 2, 0.5, 0.0f), false);
			entity.sprite = SheetSprite(ships, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 1.0);
			row.push_back(entity);
		}
		game.entities.push_back(row);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && mode == STATE_MAIN_MENU) {
					mode = STATE_GAME;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && playerShootTime >= 0.25){
					game.player.Shoot(1.0f, game.bullets, ships);
					playerShootTime = 0.0;
				}
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		switch (mode) {
		case STATE_MAIN_MENU:
			mainMenu.Render(font, program);
			break;
		case STATE_GAME:
			game.RenderGame(program);
			game.UpdateGame(mode, lastFrameTicks, playerShootTime, enemyShootTime, ships, elapsed);
			if (game.CheckWin()) {
				mode = STATE_GAME_WIN;
			}
			break;
		case STATE_GAME_OVER:
			loseMenu.Render(font, program);
			break;
		case STATE_GAME_WIN:
			winMenu.Render(font, program);
			break;
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	glDisable(GL_BLEND);

	SDL_Quit();
	return 0;
}
