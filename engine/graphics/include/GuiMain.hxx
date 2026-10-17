// header guimain

// GuiMain

#pragma once

#include <memory>

#include "Magnum/SceneGraph/Scene.h"
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

#include "smg/GuiBase.hh"

#include "vibe/common/Log.hxx"
#include "vibe/audio/NoiseMaker.hxx" // @hey: remove dependency on audio in graphics library

#include "Mesher.hxx"
#include "ColoredDrawable.hxx"
#include "TexturedDrawable.hxx"
#include "ObjData.hxx"

namespace vibe {

using namespace Magnum;
using namespace Math::Literals;

class GuiMain {

protected:
	// properties

public:
    // log 
    ShLogPr _lg = NullLog::create();

	// render object
	Shaders::PhongGL _coloredShader;
	Shaders::PhongGL _texturedShader{
		Shaders::PhongGL::Configuration{}.setFlags(Shaders::PhongGL::Flag::DiffuseTexture)};
	Containers::Array<Containers::Optional<GL::Texture2D>> _textures;

	Shaders::PhongGL _shader;

	// scene
	Scene3D _scene;
	Object3D _manipulator, _cameraObject;
	Vector3 _previousPosition;

	// mesher object
	SceneGraph::Camera3D* _camera;

	// storage for all objects
	SceneGraph::DrawableGroup3D _drawables;

	ShMesherPr _mesher;

    // @hey: remove audio objects
    daw::ShNoiseMakerPr _nm;

	// matrices @hey, move to mesher
	Magnum::Matrix4 _transformation;
	Magnum::Matrix4 _projection;
	Magnum::Matrix4 _translation;
	Magnum::Color3 _color;

	// my objects
	std::vector<ShObjDataPr> _obj_list;
	int _num_objects = 0;

	// @hey put this in mesher
	Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
	std::vector<GL::Mesh> _meshes2;
	Vector3 positionOnSphere(const Vector2i& position) const;

    // times
    fltp _tt = 0;
    fltp _last_tt = 0;
    fltp _dtt = 0;

public:
	// methods
	// constructor
	GuiMain();
	//GuiMain(smg::Args arguments);

	// destructor
	~GuiMain();

	// factory
	static ShGuiMainPr create();

	// drawings
	void show_objects();

	void set_object(ShObjDataPr obj);
	void add_object(ShObjDataPr obj);

	void load_objects();

	// setters
	void set_noisemaker(daw::ShNoiseMakerPr nm);

	// getters


	// static

    // probably move this to smg @hey
	static void print_matrix(Magnum::Matrix4 matrix);
};


} // namespace vibe
