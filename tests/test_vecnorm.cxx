#include <cassert>

#include "vibe/common/Extra.hxx"

int main(){

    arma::Col<fltp> De{1,2,3};

    fltp L = vibe::Extra::vecnorm(De);

    assert(L = sqrt(De(0)*De(0)+De(1)*De(1)+De(2)*De(2)));


}


