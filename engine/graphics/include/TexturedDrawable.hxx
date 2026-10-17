
// TexturedDrawable

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

typedef std::shared_ptr<class TexturedDrawable> ShTexturedDrawablePr;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;

class TexturedDrawable: public SceneGraph::Drawable3D {
public:
	explicit TexturedDrawable(Object3D& object,
		Shaders::PhongGL& shader,
		GL::Mesh& mesh,
		GL::Texture2D& texture,
		SceneGraph::DrawableGroup3D& group)
		: SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _texture(texture) {}

private:
	void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

	Shaders::PhongGL& _shader;
	GL::Mesh& _mesh;
	GL::Texture2D& _texture;
};

} // namespace vibe
