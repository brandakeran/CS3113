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
#include "SatCollision.h"
#include <math.h>
#include <vector>
#include <string>
#include <ctime>
#include <stdlib.h>
#include <iostream>
#include <limits>
#include <SDL_mixer.h>


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define ORTHOX 17.75
#define ORTHOY 10.0

enum GameMode{STATE_MAIN_MENU, STATE_GAME, STATE_PAUSE_MENU, STATE_GAME_OVER};


GLuint triangle;
GLuint square;
GLuint pentagon;
GLuint hexagon;


float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}

class Node {
public:
	Node(std::pair<float , float> pos) : coord(pos) {
	};
	std::pair<float, float> coord;
	bool visited = false;
	float cost = std::numeric_limits<float>::infinity();
	float distance;
	Node* cameFrom = nullptr;
};

/*
void reorganize(std::vector<Node*> &heap) {
	int i = heap.size() - 1;
	while (i > 1) {
		if ((heap[i]->cost + heap[i]->distance) < (heap[(int)(i / 2)]->cost + heap[(int)(i / 2)]->distance)) {
			Node* temp = heap[(int)(i / 2)];
			heap[(int)(i / 2)] = heap[i];
			heap[i] = temp;
		}
		i--;
	}
}
Node* getMinNode(std::vector<Node*> &heap) {
	Node* current = heap[1];
	heap[1] = heap[heap.size() - 1];
	heap.erase(heap.begin() + heap.size() - 1);
	int i = 1;
	while (i < heap.size()) {
		if ((2 * i)+1 < heap.size() && (heap[(2 * i)]->cost + heap[(i * 2)]->distance) < (heap[(2 * i) + 1]->cost + heap[(i * 2) + 1]->distance)) {
			if ((heap[i]->cost + heap[i]->distance) < (heap[(2 * i)]->cost + heap[(i * 2)]->distance)) {
				Node* temp = heap[(i * 2)];
				heap[(i * 2)] = heap[i];
				heap[i] = temp;
			}
		}
		else if ((2 * i) + 1 < heap.size() && (heap[(2 * i)]->cost + heap[(i * 2)]->distance) > (heap[(2 * i) + 1]->cost + heap[(i * 2) + 1]->distance) ){
			if ((heap[i]->cost + heap[i]->distance) < (heap[(2 * i) + 1]->cost + heap[(i * 2) + 1]->distance)) {
				Node* temp = heap[(i * 2) + 1];
				heap[(i * 2) + 1] = heap[i];
				heap[i] = temp;
			}
		}
		i++;
	}
	return current;
}
*/

void sort(std::vector<Node*> &queue) {
	bool sorted = false;
	int i = queue.size() - 1;
	while (!sorted && i > 0) {
		if (queue[i]->cost + queue[i]->distance < queue[i - 1]->cost + queue[i - 1]->distance) {
			Node* temp = queue[i - 1];
			queue[i - 1] = queue[i];
			queue[i] = temp;
		}
		else {
			sorted = true;
		}
		i--;
	}
}
class Map {
public:
	Map(){}
	Map(int h, int w) : mapHeight(h), mapWidth(w) {
		for (int i = 0; i < h; i++) {
			std::vector<int> row = {};
			for (int j = 0; j < w; j++) {
				row.push_back(rand() % 2);
			}
			mapData.push_back(row);
		}
	}
	void Draw(ShaderProgram &program, GLuint tiles) {
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.SetOrthoProjection(-ORTHOX, ORTHOX, -ORTHOY, ORTHOY, -1.0f, 1.0f);
		viewMatrix.Translate(-ORTHOX, ORTHOY, 0.0);

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
		viewMatrix.Identity();
	}
	void finalize() {
		for (int i = 0; i < mapHeight; i++) {
			for (int j = 0; j < mapWidth; j++) {
				if (mapData[i][j] == 0) {
					continue;
				}

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
					0, 0,
					0, 1,
					1, 1,

					0, 0,
					1, 1,
					1, 0
					}
				);

				tilecount += 1;
			}
		}
	}
	void generate(std::vector<std::vector<int>> previous) {
		for (int i = 0; i < previous.size(); i++) {
			for (int j = 0; j < previous[i].size(); j++) {
				int neighbors = 0;
				for (int m = i - 1; m <= i + 1; m++) {
					for (int n = j - 1; n <= j + 1; n++) {
						if (m >= 0 && m < previous.size()) {
							if (n >= 0 && n < previous[i].size()) {
								if (n != i && n != j) {
									if (previous[m][n] == 1) {
										neighbors += 1;
									}
								}
							}
						}
					}
				}
				if (mapData[i][j] == 1) {
					if (neighbors < 3){ //|| neighbors > 3) {
						mapData[i][j] = 0;
					}
				}
				else {
					if (neighbors == 4) {
						mapData[i][j] = 1;
					}
				}
			}
		}
	}
	void node(){
		mapNodes = {};
		for (int i = 0; i < mapHeight; i++) {
			std::vector<Node> row = {};
			for (int j = 0; j < mapWidth; j++) {
				Node node = Node(std::make_pair(j, i));
				row.push_back(node);
			}
			mapNodes.push_back(row);
		}
	}
	std::vector<Node*> findPath(int originx, int originy, int targetx, int targety) {
		node();
		std::vector<Node*> queue;
		queue.push_back(&mapNodes[originy][originx]);
		std::vector<Node*> path = {};
		mapNodes[originy][originx].visited = true;
		mapNodes[originy][originx].cost = 0;
		bool reached = false;
		while (!reached) {
			Node* current = queue[0];
			queue.erase(queue.begin());
			if (current == &mapNodes[targety][targetx]) {
				path.push_back(current);
				reached = true;
			}
			else {
				for (int i = current->coord.second - 1; i <= current->coord.second + 1; i++) {
					for (int j = current->coord.first - 1; j <= current->coord.first + 1; j++) {
						if (!(i == current->coord.second && j == current->coord.first)) {
							if (i >= 0 && i < mapHeight && j >= 0 && j < mapWidth && mapData[i][j] == 0 && (mapNodes[i][j].visited == false || current->cost + 1 < mapNodes[i][j].cost)) {
								mapNodes[i][j].cost = current->cost + 1;
								mapNodes[i][j].distance = abs(targetx - j) + abs(targety - i);
								mapNodes[i][j].visited = true;
								mapNodes[i][j].cameFrom = current;
								queue.push_back(&mapNodes[i][j]);
								sort(queue);
							}
						}
					}
				}
			}
		}
		bool completed = false;
		while (!completed) {
			if(path[path.size() - 1] == &mapNodes[originy][originx]){
				completed = true;
			}
			else {
				Node* next = path[path.size() - 1]->cameFrom;
				if (next == &mapNodes[originy][originx]) {
					completed = true;
				}
				path.push_back(next);
			}
		}
		return path;
	}

	int mapHeight;
	int mapWidth;
	int tilecount = 0;
	float tilesize = 1.0;
	std::vector<std::vector<int>> mapData = {};
	std::vector<std::vector<Node>> mapNodes = {};
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
};

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

	void Draw(ShaderProgram &program, float x, float y, float rotation, float alpha) {
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.SetOrthoProjection(-ORTHOX, ORTHOX, -ORTHOY, ORTHOY, -1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, textureID);


		GLfloat texCoords[] = { u, v + height, u + width, v, u, v, u + width, v, u, v + height, u + width, v + height };

		float aspect = width / height;
		float verticies[] = {
			-0.5f* size * aspect, -0.5f*size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f* size * aspect, 0.5f* size,
			0.5f* size * aspect, 0.5f* size,
			-0.5f* size * aspect, -0.5f* size,
			0.5f* size * aspect, -0.5f* size };

		modelMatrix.Identity();
		modelMatrix.Translate(x, y, 0.0f);
		modelMatrix.Rotate(rotation);
		glUseProgram(program.programID);
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		program.SetAlpha(alpha);


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
void world(std::pair<float, float> vertex, std::vector<std::pair<float, float>> &worldCoord, Vector3 position, float rotation) {
	float x1 = vertex.first * cos(rotation) - vertex.second * sin(rotation) + position.x;
	float y1 = vertex.first * sin(rotation) + vertex.second * cos(rotation) + position.y;
	worldCoord.push_back(std::make_pair(x1, y1));
}
bool collision(std::vector<std::pair<float, float>> checker, std::vector<std::pair<float, float>> checkee, Vector3 checkerposition, float checkerrotation, Vector3 checkeeposition, float checkeerotation, std::pair<float, float> &penetration) {
	std::vector<std::pair<float, float>> checkerCoord;
	std::vector<std::pair<float, float>> checkeeCoord;

	for (int i = 0; i < checker.size(); i++) {
		world(checker[i], checkerCoord, checkerposition, checkerrotation);
	}
	for (int i = 0; i < checkee.size(); i++) {
		world(checkee[i], checkeeCoord, checkeeposition, checkeerotation);
	}
	return CheckSATCollision(checkerCoord, checkeeCoord, penetration);
}
bool tileCollision(std::vector<std::pair<float, float>> checker, std::vector<std::pair<float, float>> checkee, std::pair<float, float> &penetration) {
	return CheckSATCollision(checker, checkee, penetration);
}
class Entity {
public:
	Entity() {}
	Entity(Vector3 position, Vector3 velocity, Vector3 scale, float rotation) : position(position), velocity(velocity), scale(scale), rotation(rotation) {
	}
	void Draw(ShaderProgram &program, float elapsed) {
		if (active) {
			sprite.Draw(program, position.x, position.y, rotation, alpha);
		}
		if (!player && fading) {
			alpha = lerp(alpha, 0.0, 3.0*elapsed);
			sprite.Draw(program, position.x, position.y, rotation, alpha);
		}
	}
	void Update(float elapsed, std::vector<Entity> &enemies, Entity playerEnt, Map &map, float tilesize) {
		if (active) {
			position.x += (velocity.x * elapsed);
			position.y += (velocity.y * elapsed);

			if (sides == 3) {
				sprite = SheetSprite(triangle, 0.0, 0.0, 1.0, 1.0, 1.0);
				verticies = { std::make_pair(0, 0.433), std::make_pair(-0.5, -0.433), std::make_pair(0.5, -0.433) };
				if (!player) {
					fading = true;
					active = false;
				}
			}
			else if (sides == 4) {
				sprite = SheetSprite(square, 0.0, 0.0, 1.0, 1.0, 1.0);
				verticies = { std::make_pair(0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, -0.5), std::make_pair(0.5, -0.5) };
			}
			else if (sides == 5) {
				sprite = SheetSprite(pentagon, 0.0, 0.0, 1.0, 1.0, 1.0);
				verticies = { std::make_pair(0, 0.5), std::make_pair(0.475, 0.154), std::make_pair(0.294, -0.404), std::make_pair(-0.294, -0.404), std::make_pair(-0.475, 0.154) };
			}
			else if (sides == 6) {
				sprite = SheetSprite(hexagon, 0.0, 0.0, 1.0, 1.0, 1.0);
				verticies = { std::make_pair(0.5, 0), std::make_pair(0.433, 0.25), std::make_pair(-0.433, 0.25), std::make_pair(-0.5, 0.0), std::make_pair(-0.433, -0.25), std::make_pair(0.433, -0.25) };
			}
			if (position.x < -17.0) {
				position.x += (-17 - position.x);
			}
			else if (position.x > 17.0) {
				position.x += (17 - position.x);
			}
			if (position.y < -9.5) {
				position.y += (-9.5 - position.y);
			}
			else if (position.y > 9.5) {
				position.y += (9.5 - position.y);
			}
			if (player) {
				float diffX = mouseX - position.x / 2;
				float diffY = mouseY - position.y / 2;
				float result = atan(diffX / diffY);
				if ((diffY < 0)) {
					result += 3.1415926;
				}
				rotation = -result;

				for (int i = 0; i < enemies.size(); i++) {
					if (enemies[i].active) {
						if (collision(verticies, enemies[i].verticies, position, rotation, enemies[i].position, enemies[i].rotation, penetration)) {
							active = false;
						}
					}
				}
			}
			else {

				int tileX = (int)((position.x + ORTHOX) / tilesize);
				int tileY = 20 - ((int)((position.y + ORTHOY) / tilesize)) - 1;
				int playerX = (int)((playerEnt.position.x + ORTHOX) / tilesize);
				int playerY = 20 - ((int)((playerEnt.position.y + ORTHOY) / tilesize)) - 1;

				std::vector<Node*> path = map.findPath(tileX, tileY, playerX, playerY);

				if (path.size() > 1) {
					Node* current = path[path.size() - 2];
					float x = (current->coord.first * tilesize - ORTHOX) + 0.5;
					float y = ((20 - current->coord.second - 1)*tilesize - ORTHOY) + 0.5;

					float diffX = x - position.x / 2;
					float diffY = y - position.y / 2;
					float result = atan(diffX / diffY);
					if ((diffY < 0)) {
						result += 3.1415926;
					}
					rotation = -result;

					float speed = 4.0f;

					velocity.x = speed * (x - position.x);
					velocity.y = speed * (y - position.y);
				}
			}
			std::vector<std::pair<float, float>> worldCoord;
			for (int i = 0; i < verticies.size(); i++) {
				world(verticies[i], worldCoord, position, rotation);
			}
			for (int i = 0; i < worldCoord.size(); i++) {
				int mapX = (int)((worldCoord[i].first + ORTHOX) / tilesize);
				int mapY = 20 - ((int)((worldCoord[i].second + ORTHOY) / tilesize)) - 1;

				float x = mapX * tilesize - ORTHOX;
				float y = (20 - mapY - 1)*tilesize - ORTHOY;

				if (mapX > 0 && mapX < map.mapWidth && mapY > 0 && mapY < map.mapHeight) {
					if (map.mapData[(mapY)][mapX] == 1) {
						std::vector<std::pair<float, float>> tileCoord = { std::make_pair(x, y), std::make_pair(x + tilesize, y), std::make_pair(x + tilesize, y + tilesize), std::make_pair(x, y + tilesize) };
						if (tileCollision(worldCoord, tileCoord, penetration)) {
							position.x += penetration.first;
							position.y += penetration.second;
						}

					}
				}
			}
		}
	}

	Vector3 position = Vector3(0.0, 0.0, 0.0);
	Vector3 velocity = Vector3(0.0, 0.0, 0.0);
	Vector3 acceleration = Vector3(0.0, 0.0, 0.0);
	Vector3 scale = Vector3(0.0, 0.0, 0.0);
	SheetSprite sprite;
	std::vector<std::pair<float, float>> verticies;
	float mouseX;
	float mouseY;
	float rotation;
	std::pair<float, float> penetration;
	int sides = 3;
	bool player = false;
	bool active = true;
	bool fading = false;
	float alpha = 1.0;
};
class BulletEntity {
public:
	BulletEntity() {}
	BulletEntity(Vector3 position, Vector3 velocity, Vector3 scale, float rotation) : position(position), velocity(velocity), scale(scale), rotation(rotation) {
	}
	void Draw(ShaderProgram &program) {
		if (active) {
			sprite.Draw(program, position.x, position.y, rotation, alpha);
		}
	}
	void Update(float elapsed, std::vector<Entity> &enemies, std::vector<std::vector<int>> map, float tilesize) {
		if (active) {
			position.x += (velocity.x * elapsed) * cos(rotation + (3.1415926 / 2));
			position.y += (velocity.y * elapsed) * sin(rotation + (3.1415926 / 2));

			for (int i = 0; i < enemies.size(); i++) {
				if (enemies[i].active) {
					if (collision(verticies, enemies[i].verticies, position, rotation, enemies[i].position, enemies[i].rotation, penetration)) {
						enemies[i].sides -= 1;
						active = false;
					}
				}
			}

			int mapX = (int)((position.x + 17.75) / tilesize);
			int mapY = (int)((position.y + 10.0) / tilesize);
			if (position.y > 9.0) {
				std::cout << "hi";
			}
			if (mapX > 0 && mapX < 35 && mapY > 0 && mapY < 20.0) {
				if (map[(20 - mapY - 1)][mapX ] == 1) {
					active = false;
				}
			}
		}
	}

	Vector3 position = Vector3(0.0, 0.0, 0.0);
	Vector3 velocity = Vector3(0.0, 0.0, 0.0);
	Vector3 scale = Vector3(0.0, 0.0, 0.0);
	SheetSprite sprite;
	std::vector<std::pair<float, float>> verticies = { std::make_pair(0.083, 0.5), std::make_pair(-0.083, 0.5), std::make_pair(-0.083, -0.5), std::make_pair(0.083, -0.5) };
	float rotation;
	std::pair<float, float> penetration;
	float alpha = 1.0;
	bool active = true;
};

class Message {
public:
	Message(std::string message, Vector3 position) : message(message), position(position) {}

	void Render(GLuint font, ShaderProgram &program){
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);

		for (int i = 0; i < message.length(); i++) {
			int ascii = int(message[i]);

			// I do not know why the letters at the edge of the spritesheet have the white edges when rendered. I used the 
			// same sheet as in my HW 3, but in that one, everthing was fine
			float u = (float)(((int)ascii) % 16) / 16.0f;
			float v = (float)(((int)ascii) / 16) / 16.0f;
			float spriteHeight = 1.0 / 16.0;
			float spriteWidth = 1.0 / 16.0;

			modelMatrix.Identity();
			modelMatrix.Translate((position.x - message.size()/2) + i, position.y, 0.0f);
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
	std::string message;
	Vector3 position;
};
class Menu {
public:
	Menu() {};

	void Render(GLuint font, ShaderProgram program) {
		for (int i = 0; i < messages.size(); i++) {
			messages[i].Render(font, program);
		}
	}
	void onClick(GameMode &mode, bool &quit, bool &newgame) {
		for (int i = 0; i < messages.size(); i++) {
			if (mouseX > messages[i].position.x - messages[i].message.size() / 2 && mouseX < messages[i].position.x + messages[i].message.size() / 2 && mouseY > messages[i].position.y - 0.5 && mouseY < messages[i].position.y + 0.5) {
				if (mode == STATE_MAIN_MENU) {
					if (messages[i].message == "Play") {
						mode = STATE_GAME;
						newgame = true;
					}
					if (messages[i].message == "Quit") {
						quit = true;
					}
				}
				else if (mode == STATE_PAUSE_MENU) {
					if (messages[i].message == "Resume") {
						mode = STATE_GAME;
					}
					if (messages[i].message == "Main Menu") {
						mode = STATE_MAIN_MENU;
					}
				}
				else if (mode == STATE_GAME_OVER) {
					if (messages[i].message == "Main Menu") {
						mode = STATE_MAIN_MENU;
					}
				}
			}
		}
	}

	std::vector<Message> messages;
	float mouseX;
	float mouseY;
};
class GameState {
public:
	GameState() {}
	void ProcessInput(){
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_W]) {
			player.velocity.y = 4;
		}
		else if (keys[SDL_SCANCODE_S]) {
			player.velocity.y = -4;
		}
		else {
			player.velocity.y = 0;
		}

		if (keys[SDL_SCANCODE_A]) {
			player.velocity.x = -4;
		}
		else if (keys[SDL_SCANCODE_D]) {
			player.velocity.x = 4;
		}
		else {
			player.velocity.x = 0;
		}

		if (keys[SDL_SCANCODE_F]) {
			std::cout << "hi";
		}
	}
	void SetupGame(bool &newgame){

		triangle = LoadTexture(RESOURCE_FOLDER"Triangle-icon.png");
		square = LoadTexture(RESOURCE_FOLDER"Square-icon.png");
		pentagon = LoadTexture(RESOURCE_FOLDER"Pentagon-icon.png");
		hexagon = LoadTexture(RESOURCE_FOLDER"Hexagon-icon.png");
		rock = LoadTexture(RESOURCE_FOLDER"rock.png");
		bulletSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
		font = LoadTexture(RESOURCE_FOLDER"font1.png");

		mainMenu = Menu();
		mainMenu.messages.push_back(Message("POLY-GUNS", Vector3(0.0, 8.0, 0.0)));
		mainMenu.messages.push_back(Message("Play", Vector3(0.0, -3.0, 0.0)));
		mainMenu.messages.push_back(Message("Quit", Vector3(0.0, -5.0, 0.0)));

		pauseMenu = Menu();
		pauseMenu.messages.push_back(Message("PAUSED", Vector3(0.0, 8.0, 0.0)));
		pauseMenu.messages.push_back(Message("Resume", Vector3(0.0, -3.0, 0.0)));
		pauseMenu.messages.push_back(Message("Main Menu", Vector3(0.0, -5.0, 0.0)));

		overMenu = Menu();
		overMenu.messages.push_back(Message("GAME OVER", Vector3(0.0, 8.0, 0.0)));
		overMenu.messages.push_back(Message("Main Menu", Vector3(0.0, -3.0, 0.0)));

		map = Map(20, 35);
		for (int i = 0; i < 5; i++) {
			map.generate(map.mapData);
		}
		map.finalize();

		player = Entity(Vector3(0.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), 0.0);
		bool placed = false;
		while (!placed) {
			float x = (rand() % 15)*pow(-1.0, rand() % 2 + 1);
			float y = (rand() % 10)*pow(-1.0, rand() % 2 + 1);

			int mapX = (int)((x + ORTHOX) / map.tilesize);
			int mapY = 20 - ((int)((y + ORTHOY) / map.tilesize)) - 1;

			if (map.mapData[mapY][mapX] == 0) {
				player.position = Vector3(x, y, 0.0);
				placed = true;
			}
		}
		player.sprite = SheetSprite(triangle, 0.0, 0.0, 1.0, 1.0, 1.0);
		player.player = true;

		enemies = {};
		for (int i = 0; i < rand() % 5 + 5; i++) {
			float x = (rand() % 15)*pow(-1.0, rand() % 2 + 1);
			float y = (rand() % 9)*pow(-1.0, rand() % 2 + 1);

			int mapX = (int)((x + ORTHOX) / map.tilesize);
			int mapY = 20 - ((int)((y + ORTHOY) / map.tilesize)) - 1;

			if (map.mapData[mapY][mapX] == 0 && abs(player.position.x - x) + abs(player.position.y - y) > 6.0) {
				Entity enemy = Entity(Vector3(x, y, 0.0), Vector3(0.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), 0.0);
				enemy.sprite = SheetSprite(hexagon, 0.0, 0.0, 1.0, 1.0, 1.0);
				enemy.sides = rand() % 3 + 4;
				if (enemy.sides == 4) {
					enemy.sprite = SheetSprite(square, 0.0, 0.0, 1.0, 1.0, 1.0);
					enemy.verticies = { std::make_pair(0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, -0.5), std::make_pair(0.5, -0.5) };
				}
				else if (enemy.sides == 5) {
					enemy.sprite = SheetSprite(pentagon, 0.0, 0.0, 1.0, 1.0, 1.0);
					enemy.verticies = { std::make_pair(0, 0.5), std::make_pair(0.475, 0.154), std::make_pair(0.294, -0.404), std::make_pair(-0.294, -0.404), std::make_pair(-0.475, 0.154) };
				}
				else if (enemy.sides == 6) {
					enemy.sprite = SheetSprite(hexagon, 0.0, 0.0, 1.0, 1.0, 1.0);
					enemy.verticies = { std::make_pair(0.5, 0), std::make_pair(0.433, 0.25), std::make_pair(-0.433, 0.25), std::make_pair(-0.5, 0.0), std::make_pair(-0.433, -0.25), std::make_pair(0.433, -0.25) };
				}
				enemies.push_back(enemy);
			}
			else {
				i--;
			}
		}
		bullets = {};
		newgame = false;
	}
	void RenderGame(ShaderProgram &program, float elapsed) {
		map.Draw(program, rock);
		for (int i = 0; i < enemies.size(); i++) {
			enemies[i].Draw(program, elapsed);
		}
		player.Draw(program, elapsed);
		player.Update(elapsed, enemies, player, map, map.tilesize);

		for (int i = 0; i < bullets.size(); i++) {
			bullets[i].Draw(program);
		}

	}
	void UpdateGame(float elapsed, GameMode &mode) {
		if (enemySpawnTime > 1.0) {
			for (int i = 0; i < rand() % 3; i++) {
				float x = (rand() % 15)*pow(-1.0, rand() % 2 + 1);
				float y = (rand() % 9)*pow(-1.0, rand() % 2 + 1);

				int mapX = (int)((x + ORTHOX) / map.tilesize);
				int mapY = 20 - ((int)((y + ORTHOY) / map.tilesize)) - 1;

				if (map.mapData[mapY][mapX] == 0 && abs(player.position.x - x) + abs(player.position.y - y) > 6.0) {
					Entity enemy = Entity(Vector3(x, y, 0.0), Vector3(0.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), 0.0);
					enemy.sprite = SheetSprite(hexagon, 0.0, 0.0, 1.0, 1.0, 1.0);
					enemy.sides = rand() % 3 + 4;
					if (enemy.sides == 4) {
						enemy.sprite = SheetSprite(square, 0.0, 0.0, 1.0, 1.0, 1.0);
						enemy.verticies = { std::make_pair(0.5, 0.5), std::make_pair(-0.5, 0.5), std::make_pair(-0.5, -0.5), std::make_pair(0.5, -0.5) };
					}
					else if (enemy.sides == 5) {
						enemy.sprite = SheetSprite(pentagon, 0.0, 0.0, 1.0, 1.0, 1.0);
						enemy.verticies = { std::make_pair(0, 0.5), std::make_pair(0.475, 0.154), std::make_pair(0.294, -0.404), std::make_pair(-0.294, -0.404), std::make_pair(-0.475, 0.154) };
					}
					else if (enemy.sides == 6) {
						enemy.sprite = SheetSprite(hexagon, 0.0, 0.0, 1.0, 1.0, 1.0);
						enemy.verticies = { std::make_pair(0.5, 0), std::make_pair(0.433, 0.25), std::make_pair(-0.433, 0.25), std::make_pair(-0.5, 0.0), std::make_pair(-0.433, -0.25), std::make_pair(0.433, -0.25) };
					}
					enemies.push_back(enemy);
				}
				else {
					i--;
				}
			}
			enemySpawnTime = 0.0;
		}
		else {
			enemySpawnTime += elapsed;
		}

		for (int i = 0; i < enemies.size(); i++) {
			enemies[i].Update(elapsed, enemies, player, map, map.tilesize);
		}
		player.Update(elapsed, enemies, player, map, map.tilesize);

		for (int i = 0; i < bullets.size(); i++) {
			bullets[i].Update(elapsed, enemies, map.mapData, map.tilesize);
		}
		ProcessInput();

		if (player.active == false) {
			mode = STATE_GAME_OVER;
		}
	}

	Entity player;
	std::vector<Entity> enemies;
	std::vector<BulletEntity> bullets;
	Map map;
	float enemySpawnTime;

	GLuint rock;
	GLuint bulletSheet;
	GLuint font;

	Menu mainMenu;
	Menu pauseMenu;
	Menu overMenu;
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
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured_fade.glsl");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	projectionMatrix.SetOrthoProjection(-ORTHOX, ORTHOY, -ORTHOX, ORTHOY, -1.0f, 1.0f);
	viewMatrix.Translate(-ORTHOX, -ORTHOY, 0.0);
	glUseProgram(program.programID);

	std::srand((int)std::time(0));
	float lastFrameTicks = 0.0f;

	Mix_OpenAudio(44140, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Chunk *shootSound;
	shootSound = Mix_LoadWAV("shoot.wav");

	Mix_Music *music;
	music = Mix_LoadMUS("background.mp3");
	Mix_VolumeMusic(50);
	Mix_PlayMusic(music, -1);

	GameMode mode = STATE_MAIN_MENU;
	GameState game = GameState();
	bool newgame = false;

	game.SetupGame(newgame);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_BLEND);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_MOUSEMOTION) {
				float x = ((((float)event.motion.x / 1280)*ORTHOX) - (0.5)*ORTHOX);
				float y = ((((float)(720 - event.motion.y) / 720) * ORTHOY) - (0.5)*ORTHOY);

				game.player.mouseX = x;
				game.player.mouseY = y;
				game.mainMenu.mouseX = x;
				game.mainMenu.mouseY = y * 2; 
				game.pauseMenu.mouseX = x;
				game.pauseMenu.mouseY = y * 2;
				game.overMenu.mouseX = x;
				game.overMenu.mouseY = y * 2;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && mode == STATE_GAME) {
					mode = STATE_PAUSE_MENU;
				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					if (mode == STATE_MAIN_MENU) {
						game.mainMenu.onClick(mode, done, newgame);
					}
					else if (mode == STATE_GAME) {
						if (game.player.active) {
							BulletEntity bullet = BulletEntity(Vector3(game.player.position.x, game.player.position.y, game.player.position.z), Vector3(10.0f, 10.0f, 0.0f), Vector3(((9.0f / 54.0f) / 2)*0.5, (0.5)*0.5, 0.0f), game.player.rotation);
							bullet.sprite = SheetSprite(game.bulletSheet, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.5);
							game.bullets.push_back(bullet);
						}
						Mix_PlayChannel(-1, shootSound, 0);
					}
					else if (mode == STATE_PAUSE_MENU) {
						game.pauseMenu.onClick(mode, done, newgame);
					}
					else if (mode == STATE_GAME_OVER) {
						game.overMenu.onClick(mode, done, newgame);
					}
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

		switch (mode) {
		case STATE_MAIN_MENU:
			game.mainMenu.Render(game.font, program);
			break;
		case STATE_GAME:
			if (newgame) {
				game.SetupGame(newgame);
			}
			game.RenderGame(program, elapsed);
			game.UpdateGame(elapsed, mode);
			break;
		case STATE_PAUSE_MENU:
			game.pauseMenu.Render(game.font, program);
			break;
		case STATE_GAME_OVER:
			game.overMenu.Render(game.font, program);
			break;
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	glDisable(GL_BLEND);

	SDL_Quit();
	return 0;
}

