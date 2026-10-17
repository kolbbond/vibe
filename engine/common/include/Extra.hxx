
// Extra

#pragma once

#include <memory>

#include <armadillo>
#include "typedefs.hxx"

namespace vibe {

typedef std::shared_ptr<class Extra> ShExtraPr;

class Extra {

protected:
	// properties
public:
	// methods
	// constructor
	Extra();

	// destructor
	~Extra();

	// factory
	static ShExtraPr create();

	// setters

	// getters

	// static
	static fltp vecnorm(arma::Col<fltp> De);
	static arma::Row<fltp> vecnorm(arma::Mat<fltp> De);
	static arma::Col<fltp> rotate(arma::Col<fltp> De, arma::Col<fltp> Te);
	static arma::Mat<fltp> rotation_matrix(arma::Col<fltp> Te);

	static void panic(const char* format, ...);
	static int my_vasprintf(char** buffer, const char* format, va_list args);
	static std::string format(const char* fmt, ...);
};

} // namespace vibe
