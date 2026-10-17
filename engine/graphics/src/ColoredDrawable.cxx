
// ColoredDrawable
// ###### -

// include header
#include "ColoredDrawable.hxx"

namespace vibe { 

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
	_shader.setDiffuseColor(_color)
		.setLightPositions({{camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}})
		.setTransformationMatrix(transformationMatrix)
		.setNormalMatrix(transformationMatrix.normalMatrix())
		.setProjectionMatrix(camera.projectionMatrix())
		.draw(_mesh);
}

// setters

// getters

// static
} // namespace vibe
