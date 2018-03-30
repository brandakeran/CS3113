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
#include "FlareMap.h"
#include <vector>
#include <math.h>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

//Global Variables
#define GRAVITY -4.0


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
	SheetSprite() {}
	SheetSprite(unsigned int textureID1, float u1, float v1, float w, float h, float s) {
		textureID = textureID1;
		u = u1;
		v = v1;
		width = w;
		height = h;
		size = s;
	}

	void Draw(ShaderProgram &program, float x, float y, Matrix &viewMatrix) {
		Matrix projectionMatrix;
		Matrix modelMatrix;
		projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, textureID);


		GLfloat texCoords[] = { u, v + height, u + width, v, u, v, u + width, v, u, v + height, u + width, v + height };

		float aspect = 1.0;
		float verticies[] = {
			-0.5f* size * aspect, -0.5f*size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f* size * aspect, 0.5f* size,
			0.5f* size * aspect, 0.5f* size,
			-0.5f* size * aspect, -0.5f* size,
			0.5f* size * aspect, -0.5f* size };

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

float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}

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
	Entity() {}
	Entity(Vector3 position, Vector3 velocity, Vector3 size, bool control) : position(position), velocity(velocity), size(size) {
		controlled = control;
	}
	void Draw(ShaderProgram &program, Matrix &viewMatrix) {
		if (active) {
			sprite.Draw(program, position.x, position.y, viewMatrix);
		}
	}
	void Update(float elapsed, FlareMap &map, Entity &other) {
		if (active) {
			float friction_x = 2.0f;
			float friction_y = 0.0f;
			velocity.x = lerp(velocity.x, 0.0f, elapsed * friction_x);
			velocity.y = lerp(velocity.y, 0.0f, elapsed * friction_y);
			velocity.x += acceleration.x * elapsed;
			velocity.y += acceleration.y * elapsed;
			position.x += (velocity.x * elapsed);
			position.y += (velocity.y * elapsed);

			int LeftTile = abs((int)(position.x - size.x));
			int RightTile = abs((int)(position.x + size.x));
			int BottomTile = abs((int)(position.y - size.y));
			int TopTile = abs((int)(position.y + size.y));

			if (map.mapData[abs((int)(position.y))][LeftTile] != 0) {
				float shift = abs(position.x - size.x) - abs((int)(position.x - size.x));
				position.x += shift;
				velocity.x = 0;
			}
			if (map.mapData[abs((int)(position.y))][RightTile] != 0) {
				float shift = abs(position.x + size.x) - abs((int)(position.x + size.x));
				position.x -= shift;
				velocity.x = 0;
			}
			if (map.mapData[BottomTile][abs((int)(position.x))] != 0) {
				float shift = abs(position.y - size.y) - abs((int)(position.y - size.y));
				position.y += shift;
				velocity.y = 0;
				bottomCollide = true;
				if (!controlled) {
					velocity.y = 1.0;
				}
			}
			else {
				bottomCollide = false;
			}
			if (map.mapData[TopTile][abs((int)(position.x))] != 0) {
				float shift = abs(position.y + size.y) - abs((int)(position.y + size.y));
				position.y -= shift;
				velocity.y = 0;
			}

			if (position.x < 1.0) {
				position.x = 1.0;
				acceleration.x = 0;
				velocity.x = 0;
			}
			else if(position.x > 49.0) {
				position.x = 49.0;
				acceleration.x = 0;
				velocity.x = 0;
			}
			else if (position.y < -19.0) {
				active = false;
			}
			if ((position.x + size.x) > (other.position.x - other.size.x) && (position.x - size.x) < (other.position.x + other.size.x) && (position.y + size.y) > (other.position.y - other.size.y) && (position.y - size.y) < (other.position.y + other.size.y)) {
				if (other.controlled == false) {
					other.active = false;
				}
			}
		}
	}

	Vector3 position = Vector3(0.0, 0.0, 0.0);
	Vector3 velocity = Vector3(0.0, 0.0, 0.0);
	Vector3 acceleration = Vector3(0.0, GRAVITY, 0.0);
	Vector3 size = Vector3(0.0, 0.0, 0.0);
	SheetSprite sprite;
	bool controlled;
	bool bottomCollide = false;
	bool active = true;
};

class GameState {
public:
	GameState() {}
	void RenderGame(ShaderProgram &textureID, Matrix &viewMatrix) {
		player.Draw(textureID, viewMatrix);
		gold.Draw(textureID, viewMatrix);
	}
	void UpdateGame(FlareMap &map, float elapsed) {
		player.Update(elapsed, map, gold);
		gold.Update(elapsed, map, player);

		//Player Movement
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) {
			player.acceleration.x = -5;
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			player.acceleration.x = 5;
		}
		else {
			player.acceleration.x = 0;
		}
	}
	Entity player;
	Entity gold;
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
	GLuint tiles = LoadTexture(RESOURCE_FOLDER"dirt-tiles.png");
	GLuint character = LoadTexture(RESOURCE_FOLDER"characters_1.png");
	GLuint coin = LoadTexture(RESOURCE_FOLDER"coinGold.png");

	float lastFrameTicks = 0.0f;

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix; 
	projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);
	viewMatrix.Translate(-17.75, 10.0, 0.0);
	glUseProgram(program.programID);

	FlareMap map;
	map.Load("tilemap.txt");

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	float tilesize = 1.0;
	int tilecount = 0;

	for (int i = 0; i < map.mapHeight; i++) {
		for (int j = 0; j < map.mapWidth; j++) {
				if (map.mapData[i][j] == 0) {
					continue;
				}

				float u = (float)(((int)map.mapData[i][j]) % 24) / 24.0;
				float v = (float)(((int)map.mapData[i][j]) / 24) / 16.0;

				float spriteWidth = 1.0f / 24.0f;
				float spriteHeight = 1.0f / 16.0f;

				vertexData.insert(vertexData.end(), {
					tilesize * j, -tilesize * i,
					tilesize * j, (-tilesize * i) - tilesize,
					(tilesize * j) + tilesize, (-tilesize * i) - tilesize,

					tilesize * j, -tilesize * i,
					(tilesize * j) + tilesize, (-tilesize * i) - tilesize,
					(tilesize * j) + tilesize, -tilesize * i
					}
				);

				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + spriteHeight,
					u + spriteWidth, v + spriteHeight,

					u, v,
					u + spriteWidth, v + spriteHeight,
					u + spriteWidth, v
					}
				);

				tilecount += 1;
		}
	}

	GameState game;

	game.player = Entity(Vector3(2.0f, -15.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.5f, 0.5f, 0.0f), true);
	game.player.sprite = SheetSprite((unsigned int)character, (float)((28 % 12) / 12.0), (float)((28 / 12) / 8.0), 1.0/12.0, 1.0/8.0, 1.0f);
	game.gold = Entity(Vector3(29.5f, -11.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.5f, 0.5f, 0.0f), false);
	game.gold.sprite = SheetSprite((unsigned int)coin, 0.0, 0.0, 1.0, 1.0, 1.0f);

	glClearColor(0.60f, 0.82f, 1.0f, 0.0f);

	glEnable(GL_BLEND);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && game.player.bottomCollide == true) {
					game.player.velocity.y = 4.0;
				}
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		if (game.player.position.x > 17.75 && game.player.position.x < 32.25) {
			viewMatrix.Translate(-(game.player.velocity.x * elapsed), 0.0, 0.0);
		}

		//Draw Tilemap
		modelMatrix.Identity();
		glUseProgram(program.programID);
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		glBindTexture(GL_TEXTURE_2D, tiles);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program.positionAttribute);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, tilecount * 6);
		glDisableVertexAttribArray(program.texCoordAttribute);
		glDisableVertexAttribArray(program.positionAttribute);
		modelMatrix.Identity();

		game.RenderGame(program, viewMatrix);
		game.UpdateGame(map, elapsed);


		SDL_GL_SwapWindow(displayWindow);
	}

	glDisable(GL_BLEND);

	SDL_Quit();
	return 0;
}

