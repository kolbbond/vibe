// static utility functions
// Extra
// ###### -

// include header
#include <cassert>

#include "Extra.hxx"
#include "error.hxx"
#include "Magnum/Math/Vector3.h"

namespace vibe {

// constructor
Extra::Extra() {}

// destructor
Extra::~Extra() {}

// factory
ShExtraPr Extra::create() {
	return std::make_shared<Extra>();
}

// setters

// getters

// static

// @hey overload for single input
fltp Extra::vecnorm(arma::Col<fltp> De) {
	arma::Row<fltp> L = Extra::vecnorm(arma::Mat<fltp>{De});

	// return single output
	return L(0);
}
// vec norm for 3xN vectors
arma::Row<fltp> Extra::vecnorm(arma::Mat<fltp> De) {
	// check input size
	assert(De.n_rows == 3);

	// allocate output
	arma::Row<fltp> L(De.n_cols);

	// walk over each column
	for(arma::uword i = 0; i < De.n_cols; i++) {
		L(i) = sqrt(De(0, i) * De(0, i) + De(1, i) * De(1, i) + De(2, i) * De(2, i));
	}

	// return
	return L;
}

arma::Col<fltp> Extra::rotate(arma::Col<fltp> De, arma::Col<fltp> Te) {
	// input check
	assert(De.n_elem == 3 && Te.n_elem == 3);

	// rotation matrix
    arma::Col<fltp> Do;
	Do = (Extra::rotation_matrix(Te) * De);
    assert(Do.n_elem==3);

    // return
    return Do;
}

arma::Mat<fltp> Extra::rotation_matrix(arma::Col<fltp> Te) {
	assert(Te.n_elem == 3);

	// allocate
	arma::Mat<fltp> Rotx(3, 3);
	arma::Mat<fltp> Roty(3, 3);
	arma::Mat<fltp> Rotz(3, 3);
	arma::Mat<fltp> Rot(3, 3);

	// check each component
	if(Te(0) != 0) { //x
		Rotx.row(0) = arma::Row<fltp>{1, 0, 0};
		Rotx.row(1) = arma::Row<fltp>{0, std::cos(Te(0)), -std::sin(Te(0))};
		Rotx.row(2) = arma::Row<fltp>{0, std::sin(Te(0)), std::cos(Te(0))};
	}
	if(Te(1) != 0) { //y
		Roty.row(0) = arma::Row<fltp>{std::cos(Te(1)), 0, std::sin(Te(1))};
		Roty.row(1) = arma::Row<fltp>{0, 1, 0};
		Roty.row(2) = arma::Row<fltp>{-std::sin(Te(1)), 0, std::cos(Te(1))};
	}
	if(Te(2) != 0) { //z
		Rotz.row(0) = arma::Row<fltp>{std::cos(Te(2)), 0, std::sin(Te(2))};
		Rotz.row(1) = arma::Row<fltp>{std::sin(Te(2)), std::cos(Te(2)), 0};
		Rotz.row(2) = arma::Row<fltp>{0, 0, 1};
	}

	// return matrix multiplication
	Rot =  Rotz * Roty * Rotx;
    assert(Rot.n_rows==3&&Rot.n_cols==3);

    // return
    return Rot;
}

void Extra::panic(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    abort();
}
int Extra::my_vasprintf(char** buffer, const char* format, va_list args) {
#ifdef _WIN32
	int size = _vscprintf(format, args);
	if(size == -1) {
		return -1; // An error occurred
	}

	// Allocate the buffer
	*buffer = (char*)malloc(size + 1);
	if(*buffer == NULL) {
		return -1; // Allocation failed
	}

	// Write the formatted string to the buffer
	int result = vsprintf_s(*buffer, size + 1, format, args);
	if(result == -1) {
		free(*buffer);
		*buffer = NULL;
	}
	return result;
#endif
}

std::string Extra::format(const char* fmt, ...) {
	char* result = 0;
	va_list ap;
	va_start(ap, fmt);

// platform specific
#ifdef _WIN32
	if(my_vasprintf(&result, fmt, ap) == -1) throw std::bad_alloc();
#else
	if(vasprintf(&result, fmt, ap) == -1) throw std::bad_alloc();
#endif
	va_end(ap);
	std::string str_result(result);
	free(result);
	return str_result;
}


} // namespace vibe
