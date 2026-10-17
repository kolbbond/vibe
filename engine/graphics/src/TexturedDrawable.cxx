
// TexturedDrawable
// ###### -

// include header
#include "TexturedDrawable.hxx"

namespace vibe { 

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
	_shader.setLightPositions({{camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}})
		.setTransformationMatrix(transformationMatrix)
		.setNormalMatrix(transformationMatrix.normalMatrix())
		.setProjectionMatrix(camera.projectionMatrix())
		.bindDiffuseTexture(_texture)
		.draw(_mesh);
}

// setters

// getters

// static
} // namespace vibe
