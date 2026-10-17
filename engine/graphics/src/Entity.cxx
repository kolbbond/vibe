// store individual entity
// obj, coordinates, velocity, acceleration

// include header
#include <cassert>

#include "Entity.hxx"
#include "vibe/common/Extra.hxx"
#include "vibe/common/error.hxx"

namespace vibe {

// Local helper to convert Vector3 to armadillo column vector
inline arma::Col<fltp> vec3ToArma(const Vector3& v) {
	return arma::Col<fltp>{static_cast<fltp>(v.x()), static_cast<fltp>(v.y()), static_cast<fltp>(v.z())};
}

// constructor
Entity::Entity() {
	// initialize
	_Rn.set_size(3);
	_Vn.set_size(3);
	_An.set_size(3);
}

// destructor
Entity::~Entity() {}

// factory
ShEntityPr Entity::create() {
	return std::make_shared<Entity>();
}

// kinematics equations
// update velocities with accelerations
void Entity::update_kinematics(fltp dt) {
	// @hey, we need to include time

	// update velocity
	_Vn = _Vn + _An * dt;

	// update position
	_Rn = _Rn + _Vn * dt;
}

void Entity::translate(arma::Col<fltp> dRn) {
	assert(dRn.n_elem == 3);
	_Rn = _Rn + dRn;
}

void Entity::translate(Vector3 dRn) {
	// wrap
	translate(vec3ToArma(dRn));
}

void Entity::add_acceleration(arma::Col<fltp> dAn) {
	// input check
	assert(dAn.n_elem == 3);
	_An = _An + dAn;
}

void Entity::add_acceleration(Vector3 dAn) {
	// wrap
	add_acceleration(vec3ToArma(dAn));
}

void Entity::add_rotation(arma::Col<fltp> dPn) {
	// input check
	assert(dPn.n_elem == 3);
	assert(!dPn.is_empty());

    // rotate orientation vectors
    _Ln = Extra::rotate(_Ln,dPn);
    _Nn = Extra::rotate(_Nn,dPn);
    _Tn = Extra::rotate(_Tn,dPn);

    // add to rotation
    _Pn = _Pn + dPn;
}
void Entity::add_rotation(Vector3 dPn) {
	// wrap
	add_rotation(vec3ToArma(dPn));
}


bool Entity::is_moveable() {

	return _flag_moveable;
}

void Entity::set_type(EntityEnum ent) {
	_type = ent;
}

void Entity::set_moveable(bool moveable) {
	_flag_moveable = moveable;
}

void Entity::set_position(arma::Col<fltp> Rn) {
	// input check?
	assert(Rn.n_rows == 3);
	_Rn = Rn;
}
void Entity::set_velocity(arma::Col<fltp> Vn) {
	// input check?
	assert(Vn.n_rows == 3);
	_Vn = Vn;
}
void Entity::set_acceleration(arma::Col<fltp> An) {
	// input check?
	assert(An.n_rows == 3);
	_An = An;
}


// setters

// getters

// static
} // namespace vibe
