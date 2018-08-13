#define GLEW_STATIC
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <glm\glm.hpp>
#include <vector>
#include <GLHelpers/Program.h>
#include <GLHelpers/Buffer.h>
#include <string>
#include <array>
#include <algorithm>
#include <chrono> 

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
const float blockFallSpeed = 0.3f;



float randf() {
	return rand() / (float)INT16_MAX;
}

struct Block {
	static unsigned int blockProgram;
	static Buffer squareBuffer;
	static int offsetLocation;
	static int colourLocation;
	static glm::vec3 colours[UINT8_MAX];

	unsigned char colourId;
	Block(unsigned char colour = 0): colourId(colour) {}

	static void Init() {
		squareBuffer.CreateBuffer();
		{
			float x = 1 / (float)gridSize.x;
			float y = 1 / (float)gridSize.y;
			float verts[12] = { -x,-y, -x,y, x,y,
				x,y, x,-y, -x,-y };
			squareBuffer.SetData(verts, sizeof(verts));
		}

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

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

		colours[0] = { 0,0,0 };
		for (size_t i = 1; i < sizeof(colours)/sizeof(glm::vec3); i++)
			colours[i] = { randf(), randf(), randf() };
	}

	void Render(glm::vec2 pos) {
		Block::Render(pos, colourId);
	}

	static void Render(glm::vec2 pos, unsigned char colour) {
		if (colour > 0) {
			//need to convert to screen space
			pos.x = (pos.x / (float)gridSize.x + 1 / (gridSize.x / 0.5f)) * 2 - 1;
			pos.y = (pos.y / (float)gridSize.y + 1 / (gridSize.y / 0.5f)) * 2 - 1;
			glUniform2fv(offsetLocation, 1, &pos[0]);
			glUniform3fv(colourLocation, 1, &colours[colour][0]);
			squareBuffer.Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	bool isReal() const { return colourId != 0; }
};

unsigned int Block::blockProgram = 0;
Buffer Block::squareBuffer = Buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, false);
int Block::offsetLocation;
int Block::colourLocation;
glm::vec3 Block::colours[UINT8_MAX];

struct Grid {
	typedef std::vector<Block> Row;
	std::array<Row, gridHeight> rows;
	Grid(){
		for (size_t i = 0; i < rows.size(); i++)
			rows[i] = std::vector<Block>(gridWidth);
	}
	bool isWithinGrid(const ivec2& pos) const {
		return pos.x >= 0 && pos.y >= 0 && pos.x < gridWidth && pos.y < gridHeight;
	}

	bool isBlockHere(ivec2 pos) const {
		if (!isWithinGrid(pos))
			return true;
		return rows[pos.y][pos.x].isReal();
	}
	void Add(ivec2 p, unsigned char colour) {
		if(isWithinGrid(p))
			rows[p.y][p.x].colourId = colour;
	}
	void Render() {
		for (size_t y = 0; y < rows.size(); y++)
			for (size_t x = 0; x < rows[y].size(); x++)
				rows[y][x].Render(glm::vec2(x, y));
	}
	void removeSwap(std::vector<Block>& blocks, int i) {
		blocks[i] = blocks.back();
		blocks.pop_back();
	}
	bool isFull(const Row& row) {
		return std::all_of(row.begin(), row.end(), [](const Block& block) {return block.isReal(); });
	}
	void ClearRow(Row& row) {
		memset(row.data(), 0, row.size() * sizeof(Row::value_type));
	}
	void DoRemoval() {
		for (size_t i = 0; i < rows.size(); i++)
		{
			if (isFull(rows[i])) {
				std::copy(rows.begin() + i+1, rows.end(), rows.begin()+i);
				ClearRow(rows.back());
				i--;
			}
		}
	}


};




struct FallingPiece {
	typedef std::vector<ivec2> Piece;
	typedef std::vector<Piece> RotationPiece;
	static const RotationPiece pieces[];

	int rotation;
	ivec2 pos;
	unsigned char colourId;
	const RotationPiece* piece;

	FallingPiece(int type, unsigned char colour = rand()% (UINT8_MAX-1) + 1)
		: colourId(colour), rotation(0), piece(&pieces[type]) {
		pos = ivec2(gridWidth / 2, gridHeight);
	}

	void Move(ivec2 direction, const Grid& grid) {
		if (CanMoveThisWay(direction, grid))
			//move all the blocks
			pos += direction;
	}

	void Render() {
		for (auto blockPos = CurrentPiece().begin(); blockPos != CurrentPiece().end(); blockPos++) {
					Block::Render(*blockPos + pos, colourId);
		}
	}

	bool ConflictingBlocks(ivec2 direction, const Grid& grid) {
		ivec2 p;
		for (auto positions = CurrentPiece().begin();
			positions != CurrentPiece().end(); positions++) {
			p = *positions + direction + pos;
			if (p.x < 0 || p.x >= gridWidth || grid.isBlockHere(p) && p.y<gridHeight || p.y == -1)
				return true;
		}
		return false;
	}

	bool CanMoveThisWay(ivec2 direction, const Grid& grid) {
		return !ConflictingBlocks(direction, grid);
	}

	void Rotate(const Grid& grid) {
		int rot = rotation;
		rotation++;
		if (rotation >= piece->size())
			rotation = 0;
		if (ConflictingBlocks({ 0,0 }, grid))
			rotation = rot;
	}

	bool hasLoss() {
		for (auto b = CurrentPiece().begin(); b < CurrentPiece().end(); b++)
		{
			if (b->y + pos.y >= gridHeight)
				return true;
		}
		return false;
	}

	const Piece& CurrentPiece() {
		return (*piece)[rotation];
	}

	void AddToGrid(Grid& grid) {
		for (auto p = CurrentPiece().begin(); p != CurrentPiece().end(); p++)
			grid.Add(pos + *p, colourId);
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



#include <Windows.h>
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
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

	srand(clock());

	while (true){

		Grid grid = Grid();

		FallingPiece piece = FallingPiece(rand() % numOfBockTypes);

		float timeSinceLastMovedDown = 0;
		float timeSinceLastPressed[4];
		bool released[4]{ 1,1,1,1 };
		auto start = std::chrono::high_resolution_clock::now();
		float delta = 0;
		while (true) {
			start = std::chrono::high_resolution_clock::now();
			glfwWaitEventsTimeout(blockFallSpeed/30.0f);


			if (blockFallSpeed < timeSinceLastMovedDown) {
				if (!piece.CanMoveThisWay({ 0,-1 }, grid)) {
					if (piece.hasLoss())
						break;
					piece.AddToGrid(grid);
					piece = FallingPiece(rand() % numOfBockTypes);
					grid.DoRemoval();
				}

				timeSinceLastMovedDown = 0;
				piece.Move({ 0,-1 }, grid);
			}
			static ivec2 dir[3] = { {1,0},{-1,0},{0,-1} };

			for (size_t i = 0; i < 3; i++)
			{
				if (glfwGetKey(window, GLFW_KEY_RIGHT+i) == GLFW_PRESS) {
					if (released[i] == true) {
						piece.Move(dir[i], grid); 
					}
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
						piece.Move(dir[i], grid);
					}
				}
			}


			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				if (released[3] == true)
					piece.Rotate(grid); 
				released[3] = false;
			}

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE)
				released[3] = true;
			
			if (glfwWindowShouldClose(window))		
				return 0;
	
			//Render blocks
			grid.Render();
			piece.Render();
			glfwSwapBuffers(window);
			glClear(GL_COLOR_BUFFER_BIT);
			
			delta = ((std::chrono::duration<double>)(std::chrono::high_resolution_clock::now() - start)).count();
			timeSinceLastMovedDown += delta;
		}
	}
	return 0;
}