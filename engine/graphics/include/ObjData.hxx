// header for objdata
// ObjData

#pragma once

#include <memory>
#include "boost/filesystem.hpp"

#include <armadillo>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/MeshTools/Compile.h>

#include "ColoredDrawable.hxx"
#include "TexturedDrawable.hxx"
#include "Hitbox.hxx"
#include "typedefs.hxx"
#include "Entity.hxx"
#include "MeshSwapAnimator.hxx"

namespace vibe {

using namespace Magnum;

typedef std::shared_ptr<class ObjData> ShObjDataPr;

typedef std::shared_ptr<class GuiMain> ShGuiMainPr;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct TriangleVertex4 {
	Magnum::Vector3 position;
	Magnum::Color4 color;
};

class ObjData: public Entity {

protected:
	// properties
	// storage for all objects
	SceneGraph::DrawableGroup3D _drawables;
	Containers::Array<Containers::Optional<GL::Texture2D>> _textures;

	// mesh
	//Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
public:
	std::vector<GL::Mesh> _meshes;
	GL::Mesh _mesh;
	GL::Mesh _mesh_orientations;
	GL::Mesh _mesh2;
	GL::Mesh _hitbox_mesh;
	GL::Mesh _wireframe_mesh;

	// position
	// moved to Entity

	Magnum::Matrix4 _translation;
	Magnum::Matrix4 _rotation;

	// hitbox
	Containers::Array<Vector3> _vertices;
	Containers::Array<Color4> _colors;
	Containers::Array<UnsignedInt> _indices;

	ShHitboxPr _hitbox;

	// mesh swap animator
	ShMeshSwapAnimatorPr _animator;

	// diffuse color
	Magnum::Color3 _color;

	// type of obj

	// flags
	bool _flag_show = true;

public:
	// methods
	// constructor
	ObjData();

	// destructor
	~ObjData();

	// factory
	static ShObjDataPr create();


	// load object file
	void load(std::string fpath);
	void load(boost::filesystem::path fpath);

	void calc_hitbox();
	void calc_wireframe();

	void create_orientations();

	// setters
	void set_color(Magnum::Color3 color);
	void set_hitbox(ShHitboxPr hitbox);

	// getters
	Magnum::Color3 get_color();
	Magnum::Matrix4 get_translation_matrix();
	Magnum::Matrix4 get_rotation_matrix();
	GL::Mesh& get_hitbox_mesh();
	GL::Mesh& get_wireframe_mesh();
	GL::Mesh& getActiveMesh();

	// static
};

} // namespace vibe
