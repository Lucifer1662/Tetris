#define GLEW_STATIC
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <glm\glm.hpp>
#include <vector>
#include <GLHelpers/Program.h>
#include <GLHelpers/Buffer.h>
#include <string>
#include <time.h>
using std::string;

typedef glm::tvec2<int, glm::precision::mediump> ivec2;
using glm::vec2;
const int gridWidth = 10;
const int gridHeight = 20;

const int numOfBockTypes=7;

const float downSpeed = 0.1f;
const float horizontalSpeed = 0.1f;

const float speeds[3] = {horizontalSpeed, horizontalSpeed, downSpeed};



const int blockSpeed = 30;
const ivec2 gridSize = { gridWidth, gridHeight };
const int blockSize = 30;
ivec2 screenSize;

float blockFallSpeed = 0.3f;




struct Block {
	static unsigned int blockProgram;
	static Buffer squareBuffer;
	static int offsetLocation;
	static int colourLocation;
	ivec2 pos;
	glm::vec3 colour;
	bool isDrawable = false;

	Block(ivec2 pos, glm::vec3 colour, bool isDrawable = false) : pos(pos), colour(colour), isDrawable(isDrawable) {}

	static void Init() {
		squareBuffer.CreateBuffer();
		{
			float x = 1 / (float)gridSize.x;
			float y = 1 / (float)gridSize.y;
			float verts[12] = { -x,-y, -x,y, x,y,
				x,y, x,-y, -x,-y };
			squareBuffer.SetData(verts, sizeof(verts));
		}

		string vert = R"V0G0N(
		#version 430
		uniform vec2 offset;
		layout(location = 0) in vec2 pos;
		out vec2 uv;
		void main() {
			gl_Position = vec4(pos+offset, 0, 1);
			uv = vec2(0.5, 0.5);
		}
	)V0G0N";

		string frag = R"V0G0N(
		#version 430
		uniform vec3 colour;
		in vec3 pos;
		void main() {
			gl_FragColor = vec4(colour,1);
		}
	)V0G0N";


		blockProgram = CreateProgram(vert, frag);
		glUseProgram(blockProgram);
		offsetLocation = glGetUniformLocation(blockProgram, "offset");
	 	colourLocation = glGetUniformLocation(blockProgram, "colour");
		
		
	}

	void Render() {
		if (isDrawable) {
			glUniform3fv(colourLocation, 1, &colour[0]);

			//need to convert to screen space
			glm::vec2 p = pos;
			p.x = (p.x / (float)gridSize.x + 1 / (gridSize.x / 0.5f)) * 2 - 1;
			p.y = (p.y / (float)gridSize.y + 1 / (gridSize.y / 0.5f)) * 2 - 1;
			glUniform2fv(offsetLocation, 1, &p[0]);
			squareBuffer.Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}
};

unsigned int Block::blockProgram = 0;
Buffer Block::squareBuffer = Buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, false);
int Block::offsetLocation;
int Block::colourLocation;



int at(ivec2 pos) {
	return pos.x * gridHeight + pos.y;
}

bool isBlockHere(const std::vector<Block> blocks, ivec2 pos) {
	for (auto b = blocks.begin(); b != blocks.end(); b++) {
		if (b->pos == pos)
			return true;
	}
	return false;
}

struct FallingPiece {
	static const std::vector<std::vector<ivec2>> pieces[];
	int rotation;
	ivec2 pos;

	std::vector<std::vector<Block>> blocks;
	FallingPiece(int type, glm::vec3 colour): rotation(0) {
		pos = ivec2(gridWidth / 2, gridHeight);
		blocks.resize(pieces[type].size());
		for (size_t i = 0; i < pieces[type].size(); i++)
		{
			blocks[i].reserve(pieces[type][i].size());
			for (size_t j = 0; j < pieces[type][i].size(); j++)
			{
				blocks[i].emplace_back(pieces[type][i][j], colour, true);
			}
		}
	}
	void Move(ivec2 direction, const std::vector<Block>& constBlocks) {
		if (CanMoveThisWay(direction, constBlocks))
			//move all the blocks
			pos += direction;
		//for (auto b = blocks[rotation].begin(); b != blocks[rotation].end(); b++) {
		//	b->pos += direction;
		//}
	}

	void Render() {
		for (auto b = blocks[rotation].begin(); b != blocks[rotation].end(); b++) {
			b->pos += pos;
			b->Render();
			b->pos -= pos;
		}
	}

	bool ConflictingBlocks(ivec2 direction, const std::vector<Block>& blocks, const std::vector<Block>& constBlocks) {

		ivec2 p;
		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			p = b->pos + direction + pos;
			if (p.x < 0 || p.x >= gridWidth || isBlockHere(constBlocks, p) || p.y == -1)
				return true;
		}
		return false;
	}

	bool CanMoveThisWay(ivec2 direction, const std::vector<Block>& constBlocks) {
		return !ConflictingBlocks(direction, blocks[rotation], constBlocks);
	}

	void Rotate(const std::vector<Block> constBlocks) {
		int rot = rotation + 1;
		if (rot >= blocks.size())
			rot = 0;
		if (!ConflictingBlocks({ 0,0 }, blocks[rot], constBlocks))
			rotation = rot;

	}

	bool hasLost() {
		for (auto b = blocks[rotation].begin(); b < blocks[rotation].end(); b++)
		{
			if (b->pos.y + pos.y >= gridHeight)
				return true;
		}
		return false;
	}
	
	const std::vector<Block> GetBlocks() {
		std::vector<Block> b = blocks[rotation];
		for (size_t i = 0; i < b.size(); i++)
		{
			b[i].pos += pos;
		}
		return b;
	}


};

const std::vector<std::vector<ivec2>> FallingPiece::pieces[numOfBockTypes]{
{ 
	{ { 0,0 },{-1,0},{1,0},{0,1} },
	{ { 0,0 },{0,-1},{1,0},{0,1} },
	{ { 0,0 },{-1,0},{1,0},{0,-1} },
	{ { 0,0 },{-1,0},{0,1},{0,-1} }

},{

	{ { 0,1 },{0,0},{0,-1},{0,-2} },
	{ { -1,0 },{0,0},{1,0},{2,0} },

},{

	{ { 0,0 },{0,1},{1,1},{-1,0} },
	{ { 0,0 },{0,1},{-1,1},{-1,2} },

},{

	{ { 0,0 },{1,0},{0,1},{-1,1} },
	{ { 0,0 },{0,1},{1,1},{1,2} },
},{

	{ { 0,0 },{1,0},{0,1},{1,1} },
},{

	{ { 0,0 },{-1,0},{-1,1},{1,0} },
	{ { 0,0 },{0,1},{1,1},{0,-1} },
	{ { 0,0 },{-1,0},{1,0},{1,-1} },
	{ { 0,0 },{0,1},{0,-1},{-1,-1} },
},{

	{ { 0,0 },{-1,0},{1,1},{1,0} },
	{ { 0,0 },{0,1},{1,-1},{0,-1} },
	{ { 0,0 },{-1,0},{1,0},{-1,-1} },
	{ { 0,0 },{0,1},{0,-1},{-1,1} },
}


};


void removeSwap(std::vector<Block>& blocks, int i) {
	blocks[i] = blocks.back();
	blocks.pop_back();
}

void DoRemoval(std::vector<Block>& blocks) {
	int count[gridHeight];
	memset(count, 0, sizeof(count));
	for (size_t i = 0; i < blocks.size(); i++)
	{
		count[blocks[i].pos.y] ++;
	}

	int removals[gridHeight];
	memset(removals, 0, sizeof(removals));
	int lowestLevel = gridHeight+1;
	for (size_t i = 0; i < blocks.size(); i++)
	{
		int lvl = blocks[i].pos.y;
		if (count[lvl] == gridWidth) {
			if (lowestLevel < lvl)
				lowestLevel = lvl;
			removeSwap(blocks, i--);

			removals[lvl] = 1;
		}
	}

	for (size_t i = 1; i < gridHeight; i++)
	{
		removals[i] += removals[i-1];
	}

	for (size_t i = 0; i < blocks.size(); i++)
	{
		blocks[i].pos.y -= removals[blocks[i].pos.y];
	}
}

float randf() {
	return rand() / (float)INT16_MAX;
}

#include <Windows.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
//int main(int argc, char** argv) {
	glfwInit();
	screenSize = { blockSize * gridSize.x, blockSize * gridSize.y };
	glfwWindowHint(GLFW_RESIZABLE, false);
	auto window = glfwCreateWindow(screenSize.x, screenSize.y, "Tetris", NULL, NULL);
	glfwMakeContextCurrent(window);

	glewInit();
	glClearColor(0.5f, 0.5f, 0.5f, 1);
	
	glfwSetWindowPos(window, 0, 40);

	Block::Init();


	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	srand(clock());
	while (true)
	{

		std::vector<Block> blocks;

		FallingPiece piece = FallingPiece(rand() % numOfBockTypes, { randf(),randf(),randf() });


		float timeSinceLastMovedDown = 0;
		float timeSinceLastPressed[4];
		bool released[4]{ 1,1,1,1 };
		clock_t start;
		float delta = 0;

		while (true) {
			start = clock();
			glfwPollEvents();
			glfwSwapBuffers(window);
			glClear(GL_COLOR_BUFFER_BIT);


			if (timeSinceLastMovedDown > blockFallSpeed) {
				if (!piece.CanMoveThisWay({ 0,-1 }, blocks)) {
					if (piece.hasLost())
						break;
					std::vector<Block> b = piece.GetBlocks();
					blocks.insert(blocks.end(), b.begin(), b.end());
					piece = FallingPiece(rand() % numOfBockTypes, { randf(),randf(),randf() });
					DoRemoval(blocks);
					
				}

				timeSinceLastMovedDown = 0;
				piece.Move({ 0,-1 }, blocks);
			}

			static ivec2 dir[3] = { {1,0},{-1,0},{0,-1} };

			for (size_t i = 0; i < 3; i++)
			{
				if (glfwGetKey(window, GLFW_KEY_RIGHT+i) == GLFW_PRESS) {
					if (released[i] == true)
						piece.Move(dir[i], blocks);
					released[i] = false;
					timeSinceLastPressed[i] += delta;
				}
				if (glfwGetKey(window, GLFW_KEY_RIGHT+i) == GLFW_RELEASE) {
					released[i] = true;
					timeSinceLastPressed[i] = 0;
				}
				if (!released[i]) {
					if (timeSinceLastPressed[i] > speeds[i]) {
						timeSinceLastPressed[i] = 0;
						piece.Move(dir[i], blocks);
					}
				}
			}
		

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				if (released[3] == true)
					piece.Rotate(blocks);
				released[3] = false;
			}

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE) {
				released[3] = true;
			}
			
			if (glfwWindowShouldClose(window)) {
				return 0;
			}
	

			//Render blocks

			for (auto b = blocks.begin(); b != blocks.end(); b++)
				b->Render();

			piece.Render();

			delta = (clock() - start) / (float)CLOCKS_PER_SEC;
			timeSinceLastMovedDown += delta;
			
		}

	}


	return 0;
}