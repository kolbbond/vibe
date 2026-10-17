//
// ColoredDrawable

#pragma once

#include <memory>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Math/Color.h>

namespace vibe {

using namespace Magnum;

typedef std::shared_ptr<class GuiMain> ShGuiMainPr;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

typedef std::shared_ptr<class ColoredDrawable> ShColoredDrawablePr;

class ColoredDrawable: public SceneGraph::Drawable3D {
public:
	explicit ColoredDrawable(Object3D& object,
		Shaders::PhongGL& shader,
		GL::Mesh& mesh,
		const Color4& color,
		SceneGraph::DrawableGroup3D& group)
		: SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _color{color} {}

private:
	void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

	Shaders::PhongGL& _shader;
	GL::Mesh& _mesh;
	Color4 _color;
};


} // namespace vibe
