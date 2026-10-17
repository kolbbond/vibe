// mesher header
// Mesher

#pragma once

#include <memory>
#include <cstdio>


#include "Magnum/Shaders/VertexColorGL.h"
#include "Magnum/Math/Matrix4.h"

namespace vibe {

typedef std::shared_ptr<class Mesher> ShMesherPr;

class Mesher {

protected:
	// properties
public:

    // shader
	Magnum::Shaders::VertexColorGL3D _shader;

    // matrices
	Magnum::Matrix4 _transformation;
	Magnum::Matrix4 _projection;
	Magnum::Matrix4 _translation;

public:
	// methods
	// constructor
	Mesher();

	// destructor
	~Mesher();

	// factory
	static ShMesherPr create();

	// setters

	// getters

	// static
};

} // namespace vibe
