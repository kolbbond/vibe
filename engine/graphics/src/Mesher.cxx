//

// include header
#include "Mesher.hxx"

namespace vibe {

using namespace Magnum::Math::Literals;
using namespace Magnum;

// constructor
Mesher::Mesher() {

	// initialize shader?
	printf("mesh constructor\n");
	_transformation = Magnum::Matrix4::rotationX(-80.0_degf) * Magnum::Matrix4::rotationY(0.0_degf);
	_projection =
		Magnum::Matrix4::perspectiveProjection(35.0_degf, Magnum::Vector2{{1920, 1080}}.aspectRatio(), 0.01f, 500.0f) *
		Magnum::Matrix4::translation(Magnum::Vector3::zAxis(-2.0f));

    // additional shift 
    float dz = -50;
	_projection = _projection * Magnum::Matrix4::translation(Vector3{0, 0, dz});
}

// destructor
Mesher::~Mesher() {}

// factory
ShMesherPr Mesher::create() {
	return std::make_shared<Mesher>();
}

/*
void Mesher::create_axes_mesh() {

	std::vector<TriangleVertex> line_vertices;
	using namespace Magnum;

	// (x,y,z) axes like in matlab
	// @hey, add the cute little arrows at the end
	float dl = 0.1;
	line_vertices.push_back({Vector3{0, 0, 0}, Color3{0xff0000_rgbf}});
	line_vertices.push_back({Vector3{dl, 0, 0}, Color3{0xff0000_rgbf}});
	line_vertices.push_back({Vector3{0, 0, 0}, Color3{0x00ff00_rgbf}});
	line_vertices.push_back({Vector3{0, dl, 0}, Color3{0x00ff00_rgbf}});
	line_vertices.push_back({Vector3{0, 0, 0}, Color3{0x0000ff_rgbf}});
	line_vertices.push_back({Vector3{0, 0, dl}, Color3{0x0000ff_rgbf}});

	// index
	std::vector<int> line_indices = {0, 1, 2, 3, 4, 5};

	Magnum::GL::Buffer line_buffer;
	line_buffer.setData(line_vertices);

	// create mesh
	_axes_mesh = Magnum::GL::Mesh(Magnum::GL::MeshPrimitive::Lines);


	// set wireframe mesh separate
	_axes_mesh.setCount(line_indices.size())
		.addVertexBuffer(
			GL::Buffer{line_vertices}, 0, Shaders::VertexColorGL3D::Position{}, Shaders::VertexColorGL3D::Color3{})
		.setIndexBuffer(GL::Buffer{line_indices}, 0, GL::MeshIndexType::UnsignedInt);
}
*/

// setters

// getters

// static
} // namespace vibe
