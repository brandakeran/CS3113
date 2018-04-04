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
#include <math.h>
#include "SatCollision.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif



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
	Entity(Vector3 position, Vector3 velocity, Vector3 scale, float rotation) : position(position), velocity(velocity), scale(scale), rotation(rotation){
	}
	void Draw(ShaderProgram &program) {
		Matrix modelMatrix;

		modelMatrix.Scale(scale.x, scale.y, scale.z);
		modelMatrix.Rotate(rotation);
		modelMatrix.Translate(position.x, position.y, 0.0);

		program.SetModelMatrix(modelMatrix);
			
		program.SetColor(1.0f, 1.0f, 1.0f, 0.0f);
		float vertices2[] = { 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		modelMatrix.Identity();
	}
	void Update(float elapsed) {
		position.x += velocity.x * elapsed;
		position.y += velocity.y * elapsed;
	}

	Vector3 position = Vector3(0.0, 0.0, 0.0);
	Vector3 velocity = Vector3(0.0, 0.0, 0.0);
	Vector3 scale = Vector3(0.0, 0.0, 0.0);
	float rotation;
};

//Caculation for matrix multiplication written out in Scale, Rotate, Translate order, then multiplied by x,y vector
void world(Entity rec, float x, float y, std::vector<std::pair<float, float>> &points) {
	float x1 = x * rec.scale.x * cos(rec.rotation) - (y) * rec.scale.x * (sin(rec.rotation)) + rec.scale.x * cos(rec.rotation) * rec.position.x - rec.scale.x * (sin(rec.rotation)) * rec.position.y;
	float y1 = x * rec.scale.y * sin(rec.rotation) + (y) * rec.scale.y * cos(rec.rotation) + rec.scale.y * sin(rec.rotation) * rec.position.x + rec.scale.y * cos(rec.rotation) * rec.position.y;
	points.push_back(std::make_pair(x1, y1)); 
}

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

	float lastFrameTicks = 0.0f;

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix; 
	projectionMatrix.SetOrthoProjection(-17.75, 17.75, -10.0, 10.0, -1.0f, 1.0f);
	glUseProgram(program.programID);

	Entity rec1 = Entity(Vector3(-5.0, 0.0, 0.0), Vector3(1.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), 45.0);
	Entity rec2 = Entity(Vector3(5.0, 0.0, 0.0), Vector3(-1.0, 0.0, 0.0), Vector3(2.0, 2.0, 2.0), -20.0);
	Entity rec3 = Entity(Vector3(0.0, 4.0, 0.0), Vector3(0.0, -1.0, 0.0), Vector3(1.0, 3.0, 2.0), 0.0);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glEnable(GL_BLEND);

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

		rec1.Draw(program);
		rec1.Update(elapsed);
		rec2.Draw(program);
		rec2.Update(elapsed);
		rec3.Draw(program);
		rec3.Update(elapsed);

		std::vector<std::pair<float, float>> e1Points;
		std::vector<std::pair<float, float>> e2Points;
		std::vector<std::pair<float, float>> e3Points;

		//Rectangle 1

		world(rec1, -0.5, 0.5, e1Points);
		world(rec1, -0.5, -0.5, e1Points);
		world(rec1, 0.5, -0.5, e1Points);
		world(rec1, 0.5, 0.5, e1Points);

		//Rectangle 2

		world(rec2, -0.5, 0.5, e2Points);
		world(rec2, -0.5, -0.5, e2Points);
		world(rec2, 0.5, -0.5, e2Points);
		world(rec2, 0.5, 0.5, e2Points);

		//Rectangle 3

		world(rec3, -0.5, 0.5, e3Points);
		world(rec3, -0.5, -0.5, e3Points);
		world(rec3, 0.5, -0.5, e3Points);
		world(rec3, 0.5, 0.5, e3Points);


		std::pair<float, float> penetration;
		bool collided = CheckSATCollision(e1Points, e2Points, penetration);
		if (collided){
			rec1.position.x += (penetration.first * 0.5f);
			rec1.position.y += (penetration.second * 0.5f);

			rec2.position.x -= (penetration.first * 0.5f);
			rec2.position.y -= (penetration.second * 0.5f);

			rec1.velocity.x *= -1;
			rec1.velocity.y *= -1;

			rec2.velocity.x *= -1;
			rec2.velocity.y *= -1;
		}
		collided = CheckSATCollision(e1Points, e3Points, penetration);
		if (collided) {
			rec1.position.x += (penetration.first * 0.5f);
			rec1.position.y += (penetration.second * 0.5f);

			rec3.position.x -= (penetration.first * 0.5f);
			rec3.position.y -= (penetration.second * 0.5f);

			rec1.velocity.x *= -1;
			rec1.velocity.y *= -1;

			rec3.velocity.x *= -1;
			rec3.velocity.y *= -1;
		}
		collided = CheckSATCollision(e2Points, e3Points, penetration);
		if (collided) {
			rec2.position.x += (penetration.first * 0.5f);
			rec2.position.y += (penetration.second * 0.5f);

			rec3.position.x -= (penetration.first * 0.5f);
			rec3.position.y -= (penetration.second * 0.5f);

			rec3.velocity.x *= -1;
			rec3.velocity.y *= -1;

			rec2.velocity.x *= -1;
			rec2.velocity.y *= -1;
		}




		SDL_GL_SwapWindow(displayWindow);
	}

	glDisable(GL_BLEND);

	SDL_Quit();
	return 0;
}

