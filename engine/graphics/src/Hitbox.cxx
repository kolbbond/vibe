// store hitbox stuff
// Hitbox
// ###### -


// include header
#include <cassert>

// @hey any reason to make these smg includes?
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include "Magnum/Shaders/Shaders.h"
#include "Magnum/Shaders/VertexColorGL.h"
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include "Magnum/Primitives/Icosphere.h"
#include <Magnum/Trade/MeshData.h>
#include "Magnum/MeshTools/Copy.h"

// for buffer setting with vectors
#include <Corrade/Corrade.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include "vibe/common/Extra.hxx"

#include "Hitbox.hxx"

namespace vibe {

// constructor
Hitbox::Hitbox(fltp w, fltp h, fltp l) {

	// create vertices
	// around 0

	// create hitboxes (unit cube
	arma::Mat<fltp> Rn1(3, 8);
	Rn1.row(0) = arma::Row<fltp>{-w / 2, w / 2, w / 2, -w / 2, -w / 2, w / 2, w / 2, -w / 2}; // x
	Rn1.row(1) = arma::Row<fltp>{-h / 2, -h / 2, h / 2, h / 2, -h / 2, -h / 2, h / 2, h/2}; // y
	Rn1.row(2) = arma::Row<fltp>{-l / 2, -l / 2, -l / 2, -l / 2, l / 2, l / 2, l / 2, l/2}; // z

	// set vertices and generate mesh
	set_vertices(Rn1);
	calc_bounds();
	create_mesh();
}

Hitbox::Hitbox() {}

// destructor
Hitbox::~Hitbox() {}

// factory
ShHitboxPr Hitbox::create() {
	return std::make_shared<Hitbox>();
}
// overload for hitbox with
// w,h,l
ShHitboxPr Hitbox::create(fltp w, fltp h, fltp l) {

	return std::make_shared<Hitbox>(w, h, l);
}


// calc bounds of hitbox i.e. min/max x,y,z
void Hitbox::calc_bounds() {

	printf("calc bounds\n");

	// allocate
	_bounds.resize(3, 2);

	// walk over dimensions
	for(int i = 0; i < 3; i++) {
		_bounds(i, 0) = _vertices.row(i).min();
		_bounds(i, 1) = _vertices.row(i).max();
	}

	// @hey, add element indexing
	_Rn = arma::Mat<fltp>(3, 8);
	_Rn.col(0) = arma::Col<fltp>{_bounds(0, 0), _bounds(1, 0), _bounds(2, 0)};
	_Rn.col(1) = arma::Col<fltp>{_bounds(0, 1), _bounds(1, 0), _bounds(2, 0)};
	_Rn.col(2) = arma::Col<fltp>{_bounds(0, 1), _bounds(1, 1), _bounds(2, 0)};
	_Rn.col(3) = arma::Col<fltp>{_bounds(0, 0), _bounds(1, 1), _bounds(2, 0)};
	_Rn.col(4) = arma::Col<fltp>{_bounds(0, 0), _bounds(1, 0), _bounds(2, 1)};
	_Rn.col(5) = arma::Col<fltp>{_bounds(0, 1), _bounds(1, 0), _bounds(2, 1)};
	_Rn.col(6) = arma::Col<fltp>{_bounds(0, 1), _bounds(1, 1), _bounds(2, 1)};
	_Rn.col(7) = arma::Col<fltp>{_bounds(0, 0), _bounds(1, 1), _bounds(2, 1)};

	// find max distance for first cut sphere radius
	_radius = (arma::max(_bounds.col(1) - _bounds.col(0))) / 2;

	// find middle point of box
	assert(_Rn.n_rows == 3 && _Rn.n_cols == 8);
	_Rmid = arma::Col<fltp>(3);
	_Rmid(0) = arma::sum(_Rn.row(0)) / 8;
	_Rmid(1) = arma::sum(_Rn.row(1)) / 8;
	_Rmid(2) = arma::sum(_Rn.row(2)) / 8;

	// output
	printf("Min/max x: (%0.2f,%0.2f)\n", _bounds(0, 0), _bounds(0, 1));
	printf("Min/max y: (%0.2f,%0.2f)\n", _bounds(1, 0), _bounds(1, 1));
	printf("Min/max z: (%0.2f,%0.2f)\n", _bounds(2, 0), _bounds(2, 1));
}

// create hitbox mesh
// by hand who cares
void Hitbox::create_mesh() {
	// assume the original hitbox is in the x,y plane

	// allocate our triangle vertex
	// hitbox is 8 points
	//      7  6
	//  / / 4  5
	// /  / / /
	// 3  2
	// 0  1
	using namespace Math::Literals;
	printf("create mesh\n");

	// check bounds are not empty
	assert(!_bounds.is_empty());

	// bounds is 3x2 (x,y,z) (min/max)
	//Color3 colorw = Color3{0xffffff_rgbf};
	Color3 colorw = _color;
	_hex_vertices.push_back({Vector3{_bounds(0, 0), _bounds(1, 0), _bounds(2, 0)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 1), _bounds(1, 0), _bounds(2, 0)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 1), _bounds(1, 1), _bounds(2, 0)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 0), _bounds(1, 1), _bounds(2, 0)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 0), _bounds(1, 0), _bounds(2, 1)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 1), _bounds(1, 0), _bounds(2, 1)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 1), _bounds(1, 1), _bounds(2, 1)}, colorw});
	_hex_vertices.push_back({Vector3{_bounds(0, 0), _bounds(1, 1), _bounds(2, 1)}, colorw});

	// copy to line vertices
	_line_vertices = _hex_vertices;

	// index the hex vertices
	_hex_indices = {0, 1, 3, 3, 1, 2, 1, 5, 2, 2, 5, 6, 3, 2, 7, 7, 2, 6, 4, 0, 7, 7, 0, 3, 5, 4, 2, 2, 4, 0};
	_line_indices = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 3, 7, 2, 6, 1, 5, 0, 4};

	// create mesh buffer
	_buffer_hex.setData(_hex_vertices);
	_buffer_wireframe.setData(_line_vertices);

	// create mesh
	_mesh_hex = Magnum::GL::Mesh(Magnum::GL::MeshPrimitive::Triangles);
	_mesh_wireframe = Magnum::GL::Mesh(Magnum::GL::MeshPrimitive::Lines);

	// set quad mesh
	_mesh_hex.setCount(_hex_indices.size())
		.addVertexBuffer(
			GL::Buffer{_hex_vertices}, 0, Shaders::VertexColorGL3D::Position{}, Shaders::VertexColorGL3D::Color3{})
		.setIndexBuffer(GL::Buffer{_hex_indices}, 0, GL::MeshIndexType::UnsignedInt);

	// set wireframe mesh separate
	_mesh_wireframe.setCount(_line_indices.size())
		.addVertexBuffer(
			GL::Buffer{_line_vertices}, 0, Shaders::VertexColorGL3D::Position{}, Shaders::VertexColorGL3D::Color3{})
		.setIndexBuffer(GL::Buffer{_line_indices}, 0, GL::MeshIndexType::UnsignedInt);

	// sphere mesh
	Trade::MeshData sphereMesh = MeshTools::copy(Primitives::icosphereSolid(1));
	Trade::MeshData sphereWireframeMesh = MeshTools::copy(Primitives::icosphereWireframe());

	// scale the meshdata by the calculate radius
	Matrix4 mat_scale = Matrix4::scaling(Vector3{static_cast<float>(_radius), static_cast<float>(_radius), static_cast<float>(_radius)});
	MeshTools::transform3DInPlace(sphereMesh, mat_scale);
	MeshTools::transform3DInPlace(sphereWireframeMesh, mat_scale);

	// compile in GL meshes
	_mesh_sphere = MeshTools::compile(sphereMesh);
	_mesh_sphere_wireframe = MeshTools::compile(sphereWireframeMesh);
}


// setters
// set vertices into hitbox
// manually copy as armadillo matrix
void Hitbox::set_vertices(Containers::Array<Vector3>& vertices) {
	// allocate
	_vertices.resize(3, vertices.size());

	// manually copy
	for(size_t i = 0; i < vertices.size(); i++) {
		fltp x = vertices[i].x();
		fltp y = vertices[i].y();
		fltp z = vertices[i].z();

		_vertices.col(i) = arma::Col<fltp>{x, y, z};
	}
}

// overlap for set with armadillo
void Hitbox::set_vertices(arma::Mat<fltp> vertices) {

	// check input
	assert(vertices.n_rows == 3 && vertices.n_cols == 8);

	// allocate
	_vertices = arma::Mat<fltp>(3, vertices.n_cols);

	_vertices = vertices;
}

void Hitbox::set_color(Color3 color) {
	_color = color;
}

arma::Mat<fltp> Hitbox::get_coordinates() {
	return _Rn;
}

fltp Hitbox::get_radius() {

	return _radius;
}

arma::Mat<float> Hitbox::get_bounds() {
	// return hitbox min/max
	return _bounds;
}

GL::Mesh& Hitbox::get_mesh() {
	return _mesh_wireframe;
}

arma::Col<fltp> Hitbox::get_middle_point() {
	return _Rmid;
}

// getters

// static
// calculate if two hitboxes overlap?
// naive implementation (better method)? @hey
bool Hitbox::overlap(ShHitboxPr& box1, ShHitboxPr& box2) {
	// check boxes?

	assert(box1 != NULL && box2 != NULL);

	// check if box1 points inside bounds of box2
	arma::Mat<fltp> Rn1 = box1->get_coordinates();
	arma::Mat<float> bnds = box2->get_bounds();
	printf("difference between points\n");

	// first check sphere hitboxes (hitspheres)
	// this check is faster and we can remove many checks if this fails
	printf("difference between points\n");
	arma::Col<fltp> Rmid1 = box1->get_middle_point();
	arma::Col<fltp> Rmid2 = box2->get_middle_point();
	fltp radius1 = box1->get_radius();
	fltp radius2 = box2->get_radius();

	// difference between middle points
	printf("difference between points\n");
	arma::Col<fltp> dR = Rmid2 - Rmid1;

	printf("find dist\n");
	fltp dist = Extra::vecnorm(dR);
	printf("find dist\n");

	// there is overlap is the distance between middle points is
	// Rmid1 --R1 ---> <--R2---- Rmid2
	if(dist < (radius2 + radius1)) return true;

	// we early exit but if that check fails we need a more
	// fine resolution check

	/*
	// if any point inside
	//  there is overlap
	bool is_inside = inside(box1, box2);
    */

	// return success
	return false;
}


// calculate if hitbox inside another ...
// we will use this for the boundaries
// if box1 is inside box2
bool Hitbox::inside(ShHitboxPr& box1, ShHitboxPr& box2) {
	// check if box1 points inside bounds of box2
	arma::Col<fltp> Rn1 = box1->get_coordinates();
	arma::Mat<float> bnds = box2->get_bounds();

	// check each dimension
	arma::Col<arma::uword> fidx = arma::find(Rn1.row(0) > bnds(0, 0) && Rn1.row(0) < bnds(0, 1) &&
		Rn1.row(1) > bnds(1, 0) && Rn1.row(1) < bnds(1, 1) && Rn1.row(2) > bnds(2, 0) && Rn1.row(2) < bnds(2, 1));

	// if no point found
	if(!fidx.is_empty())
		return true;
	else
		return false;
}

} // namespace vibe
