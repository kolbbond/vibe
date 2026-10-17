#pragma once

#include <vector>
#include <armadillo>

#include "Magnum/Magnum.h"
#include "Magnum/GL/Mesh.h"
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Math.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Matrix3.h>



// Terminal color definitions
// color definitions
#define KNRM "\x1B[0m"
#define KBLD "\033[1m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

typedef double fltp;

//using namespace Magnum;
// mesh types
// simplest triangle vertex
struct TriangleVertex {
	Magnum::Vector3 position;
	Magnum::Color3 color;
};

// stores quad
// position (x,y,z), color
struct QuadVertex3D {
	Magnum::Vector3 position;
	Magnum::Color3 color;
};

// (x,y)
struct QuadVertex2D {
	Magnum::Vector2 position;
	Magnum::Color3 color;
};

// and element index
struct ElementVertex {
	QuadVertex3D quad_vertex;
	int element_index;
};

struct AreaMesh {
	std::vector<QuadVertex2D> point_vertices;
	std::vector<QuadVertex2D> line_vertices;
	std::vector<QuadVertex2D> quad_vertices;
	std::vector<int> quad_indices;
	std::vector<int> line_indices;
	Magnum::GL::Mesh point_mesh;
	Magnum::GL::Mesh line_mesh;
	Magnum::GL::Mesh quad_mesh;
};

struct FrameMesh {
	// frame is a path essentially
	// we should add drawing the orientation vectors
	std::vector<QuadVertex3D> point_vertices;
	std::vector<QuadVertex3D> line_vertices;
	std::vector<QuadVertex3D> direction_vertices;
	std::vector<QuadVertex3D> normal_vertices;
	std::vector<QuadVertex3D> transverse_vertices;
	std::vector<int> line_indices;
	std::vector<int> direction_indices;
	std::vector<int> normal_indices;
	std::vector<int> transverse_indices;
	Magnum::GL::Mesh point_mesh;
	Magnum::GL::Mesh line_mesh;
	Magnum::GL::Mesh direction_mesh;
	Magnum::GL::Mesh normal_mesh;
	Magnum::GL::Mesh transverse_mesh;
};

struct HexMesh {
	std::vector<QuadVertex3D> point_vertices;
	std::vector<QuadVertex3D> line_vertices;
	std::vector<QuadVertex3D> direction_vertices;
	std::vector<QuadVertex3D> normal_vertices;
	std::vector<QuadVertex3D> transverse_vertices;
	std::vector<QuadVertex3D> hex_vertices;
	std::vector<int> line_indices;
	std::vector<int> direction_indices;
	std::vector<int> normal_indices;
	std::vector<int> transverse_indices;
	std::vector<int> hex_indices;
	Magnum::GL::Mesh point_mesh;
	Magnum::GL::Mesh line_mesh;
	Magnum::GL::Mesh direction_mesh;
	Magnum::GL::Mesh normal_mesh;
	Magnum::GL::Mesh transverse_mesh;
	Magnum::GL::Mesh hex_mesh;
};

