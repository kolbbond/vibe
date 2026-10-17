// object to load .obj files

// ObjData
// ###### -

// include header
#include "ObjData.hxx"
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Trade/MeshData.h>

// for buffer setting with vectors
#include <Corrade/Corrade.h>
#include <Corrade/Containers/ArrayViewStl.h>
#include "Magnum/Primitives/Icosphere.h"

#include "vibe/common/error.hxx"

namespace vibe {

// Helper to convert armadillo column vector to Magnum Vector3
inline Magnum::Vector3 armaToVector3(const arma::Col<fltp>& v) {
    return Magnum::Vector3{static_cast<float>(v(0)), static_cast<float>(v(1)), static_cast<float>(v(2))};
}


using namespace Magnum;
using namespace Magnum::Math::Literals;

// constructor
ObjData::ObjData() {
	// initialize
	_Rn = arma::Col<fltp>(3, arma::fill::zeros);

	// create empty mesh
	_mesh = MeshTools::compile(Primitives::icosphereSolid(1));

    // other meshes
    create_orientations();
}


// destructor
ObjData::~ObjData() {}

// factory
ShObjDataPr ObjData::create() {
	return std::make_shared<ObjData>();
}

void ObjData::load(boost::filesystem::path fpath) {

	// log
	printf("loading file: %s\n", fpath.string().c_str());

	/* Load a scene importer plugin */
	printf("scene importer\n");
	PluginManager::Manager<Trade::AbstractImporter> manager;
	Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnySceneImporter");

	// import our file
	printf("file import\n");
	if(!importer || !importer->openFile(fpath.string().c_str())) std::exit(1);


	// get vertex data from loaded mesh
	// if we don't get it here I think it's gone forever!
	Containers::ArrayView<const char> vertex_data;
	GL::Buffer vertex_buffer;

	/* Load all meshes. Meshes that fail to load will be NullOpt. Generate
       normals if not present. */

	for(UnsignedInt i = 0; i != importer->meshCount(); ++i) {
		Containers::Optional<Trade::MeshData> meshData;
		if(!(meshData = importer->mesh(i))) {
			Warning{} << "Cannot load mesh" << i << importer->meshName(i);
			continue;
		}

		// get vertex data somehow
		vertex_data = meshData->vertexData();
		vertex_buffer.setData(meshData->vertexData());
		_vertices = meshData->positions3DAsArray();

		//_colors = meshData->colorsAsArray();
		printf("test\n");

		// get indices too
		_indices = meshData->indicesAsArray();

		std::cout << "vertex data size: " << vertex_data.size() << std::endl;
		std::cout << "vertex buffer size: " << vertex_buffer.size() << std::endl;
		std::cout << "vertices  size: " << _vertices.size() << std::endl;
		std::cout << "indices  size: " << _indices.size() << std::endl;

		// create new with our vertex colors
		std::vector<TriangleVertex4> verts4;
		std::vector<TriangleVertex> verts3;
		for(size_t i = 0; i < _vertices.size(); i++) {
			//verts4.push_back(TriangleVertex4{_vertices[i], _colors[i]});
			//Color4 c4 = Color4{color.
			verts4.push_back(TriangleVertex4{_vertices[i], _color});
			verts3.push_back(TriangleVertex{_vertices[i], _color});
		}


		// generate mesh
		MeshTools::CompileFlags flags;
		//_meshes.push_back(MeshTools::compile(*meshData, flags));
		_mesh = (MeshTools::compile(*meshData, flags));

		// allow meshtools OR hand compile
		//_mesh = MeshTools::compile(*meshData, flags);
		/*
		_mesh = Magnum::GL::Mesh(Magnum::GL::MeshPrimitive::Triangles);
		_mesh.setCount(verts4.size());
		_mesh.addVertexBuffer(GL::Buffer{verts4}, 0, Shaders::PhongGL::Position{}, Shaders::PhongGL::Color4{});
		_mesh.setIndexBuffer(GL::Buffer{_indices}, 0, GL::MeshIndexType::UnsignedInt);
        */

		_mesh2 = Magnum::GL::Mesh(Magnum::GL::MeshPrimitive::Triangles);
		_mesh2.setCount(verts3.size());
		_mesh2.addVertexBuffer(GL::Buffer{verts3}, 0, Shaders::PhongGL::Position{}, Shaders::PhongGL::Color3{});
		_mesh2.setIndexBuffer(GL::Buffer{verts3}, 0, GL::MeshIndexType::UnsignedInt);
	}

	// check size?
	assert((int)vertex_data.size() == vertex_buffer.size());
	//assert(vertex_data.size() == vertices.size());

	// walk over vertex data
	/* debug
	std::cout << vertex_buffer.data() << std::endl;
	for(size_t i = 0; i < _vertices.size(); i++) {
		//printf("Rn(%lu)(x,y,z): (%0.2f,%0.2f,%0.2f)\n", i, _vertices[i].x(), _vertices[i].y(), _vertices[i].z());
	}
    */

	// create hitbox
	ShHitboxPr hitbox = Hitbox::create();
	hitbox->set_vertices(_vertices);
	hitbox->calc_bounds();
	hitbox->create_mesh();

	set_hitbox(hitbox);

	// calc wireframe
	calc_wireframe();
}
void ObjData::create_orientations() {
	// input check
	assert(!_Rn.is_empty());
	assert(!_Ln.is_empty());

	// orientation mesh
	std::vector<TriangleVertex4> vtx_axes;
	vtx_axes.push_back(TriangleVertex4{armaToVector3(_Rn), Color4{1.0, 0.0, 0.0, 1.0}});
	vtx_axes.push_back(TriangleVertex4{armaToVector3(_Rn + _Ln*10), Color4{1.0, 0.0, 0.0, 1.0}});
	vtx_axes.push_back(TriangleVertex4{armaToVector3(_Rn), Color4{0.0, 1.0, 0.0, 1.0}});
	vtx_axes.push_back(TriangleVertex4{armaToVector3(_Rn + _Tn*10), Color4{0.0, 1.0, 0.0, 1.0}});
	vtx_axes.push_back(TriangleVertex4{armaToVector3(_Rn), Color4{0.0, 0.0, 1.0, 1.0}});
	vtx_axes.push_back(TriangleVertex4{armaToVector3(_Rn + _Nn*10), Color4{0.0, 0.0, 1.0, 1.0}});

	_mesh_orientations = Magnum::GL::Mesh(Magnum::GL::MeshPrimitive::Lines);
	_mesh_orientations.setCount(vtx_axes.size());
	_mesh_orientations.addVertexBuffer(
		GL::Buffer{vtx_axes}, 0, Shaders::PhongGL::Position{}, Shaders::PhongGL::Color4{});
	_mesh_orientations.setIndexBuffer(GL::Buffer{vtx_axes}, 0, GL::MeshIndexType::UnsignedInt);
}

// calculate hitbox
void ObjData::calc_hitbox() {

	// get mesh vertices

	// walk over vertices and get min/max x,y,z
}

// calculate the wireframe of this obj
// from the vertices and indices
void ObjData::calc_wireframe() {
	// check vertices and indices

	// since each triangle is 0,1,2
	// we create the lines 0,1,1,2,2,3,3,0
}


Magnum::Matrix4 ObjData::get_translation_matrix() {
	// calculate translation matrix from coordinates

	// convert arma to Vector3
	Vector3d Rn = Vector3d{_Rn(0), _Rn(1), _Rn(2)};
	Vector3 Rnf = Vector3(Rn);

	_translation = Magnum::Matrix4::translation(Rnf);

	return _translation;
}

Magnum::Matrix4 ObjData::get_rotation_matrix() {
	_rotation = Matrix4::rotationZ(Math::Rad(static_cast<float>(_Pn(2))));
	_rotation = _rotation * Matrix4::rotationY(Math::Rad(static_cast<float>(_Pn(1))));
	_rotation = _rotation * Matrix4::rotationX(Math::Rad(static_cast<float>(_Pn(0))));

	// return it
	return _rotation;
}


// setters
void ObjData::set_color(Magnum::Color3 color) {
	_color = color;
}
void ObjData::set_hitbox(ShHitboxPr hitbox) {
	_hitbox = hitbox;
}

// getters

GL::Mesh& ObjData::get_hitbox_mesh() {
	// return mesh from hitbox
	return _hitbox->get_mesh();
}
GL::Mesh& ObjData::get_wireframe_mesh() {
	return _wireframe_mesh;
}

Magnum::Color3 ObjData::get_color() {
	return _color;
}

GL::Mesh& ObjData::getActiveMesh() {
	if (_animator) {
		GL::Mesh* m = _animator->currentMesh();
		if (m) return *m;
	}
	return _mesh;
}

// static
} // namespace vibe
