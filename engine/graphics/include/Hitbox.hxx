// header for hitbox.cpp
// Hitbox

#pragma once

#include <memory>

#include <armadillo>

#include <Corrade/Containers/Array.h>

#include "Magnum/Math/Vector3.h"
#include "Magnum/GL/GL.h"
#include "Magnum/GL/Mesh.h"
#include "Magnum/Math/Color.h"
#include <Magnum/GL/DefaultFramebuffer.h>

#include "smg/typedefs.hh"

#include "typedefs.hxx"

namespace vibe {

using namespace Magnum;
using namespace Magnum::Math::Literals;


typedef std::shared_ptr<class Hitbox> ShHitboxPr;

class Hitbox {

	// properties
protected:
public:
	// hitbox nodes
	arma::Mat<float> _bounds; // 2D as 2x3 matrix min/max x,y,z
	arma::Mat<fltp> _Rn; // box nodes
    arma::Col<fltp> _Rmid; // middle (avg) node

	// should store the direction of the hitbox
	arma::Col<fltp> _De;

	// and orientation...
	arma::Col<fltp> _Le, _Ne, _Te;

    // dimensions
    fltp _radius;

	// store hitbox mesh too?
	// @hey, unneccessary but who cares
	arma::Mat<fltp> _vertices;

	// hex
	std::vector<TriangleVertex> _hex_vertices;
	std::vector<int> _hex_indices;

	// wireframe
	std::vector<TriangleVertex> _line_vertices;
	std::vector<int> _line_indices;

    // color
	Color3 _color = Color3{0xffffff_rgbf};

public:
	// mesh data
	Magnum::GL::Buffer _buffer_hex;
	Magnum::GL::Mesh _mesh_hex;
	Magnum::GL::Buffer _buffer_wireframe;
	Magnum::GL::Mesh _mesh_wireframe;
	Magnum::GL::Buffer _buffer;
	Magnum::GL::Mesh _mesh;

    // spherical hitbox
    Magnum::GL::Mesh _mesh_sphere;
    Magnum::GL::Mesh _mesh_sphere_wireframe;

public:
	// methods
	// constructor
	Hitbox(fltp w, fltp h, fltp l);
	Hitbox();

	// destructor
	~Hitbox();

	// factory
	static ShHitboxPr create(fltp w, fltp h, fltp l);
	static ShHitboxPr create();

	void init();

	void calc_bounds();
	void create_mesh();

	// setters
	void set_vertices(Containers::Array<Vector3>& vertices);
	void set_vertices(arma::Mat<fltp> vertices);
    void set_color(Color3 color);

	// getters
	arma::Mat<fltp> get_coordinates();
	arma::Col<fltp> get_middle_point();
	arma::Mat<float> get_bounds();
    fltp get_radius();
	Magnum::GL::Mesh& get_mesh();

	// static
	static bool overlap(ShHitboxPr& box1, ShHitboxPr& box2);
	static bool inside(ShHitboxPr& box1, ShHitboxPr& box2);
};

} // namespace vibe
