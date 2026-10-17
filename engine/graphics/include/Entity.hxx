// Entity.hh

#pragma once

#include <memory>

#include "Magnum/Math/Vector3.h"
#include "vibe/common/typedefs.hxx"
#include "typedefs.hxx"

namespace vibe {

typedef std::shared_ptr<class Entity> ShEntityPr;
using namespace Magnum;

class Entity {


protected:
	// properties

public:
	// position, velocity, acceleration
	arma::Mat<fltp> _Rn;
	arma::Mat<fltp> _Vn;
	arma::Mat<fltp> _An;

	// direction
	arma::Col<fltp> _Dn;

	// orientations (how to init these with an object file)?
	arma::Col<fltp> _Ln = arma::Col<fltp>{0, 0, 1};
	arma::Col<fltp> _Nn = arma::Col<fltp>{0, 1, 0};
	arma::Col<fltp> _Tn = arma::Col<fltp>{-1, 0, 0};

	// rotation
	arma::Col<fltp> _Pn = arma::Col<fltp>{0, 0, 0};

	// flags
	bool _flag_moveable = true;

	EntityEnum _type;


public:
	// methods
	// constructor
	Entity();

	// destructor
	~Entity();

	// factory
	static ShEntityPr create();

	void update_kinematics(fltp dt);

	// change
	void add_acceleration(arma::Col<fltp> dAn);
	void add_acceleration(Vector3 dAn);

	void add_rotation(arma::Col<fltp> dAn);
	void add_rotation(Vector3 dAn);

	void translate(arma::Col<fltp> dRn);
	void translate(Vector3 dRn);

	// setters
	void set_position(arma::Col<fltp> Rn);
	void set_velocity(arma::Col<fltp> Vn);
	void set_acceleration(arma::Col<fltp> An);

	void set_type(EntityEnum ent);
	void set_moveable(bool moveable);

	// getters
	bool is_moveable();
	arma::Mat<fltp> get_position();
	arma::Mat<fltp> get_velocity();
	arma::Mat<fltp> get_acceleration();

	// static
};

} // namespace vibe
