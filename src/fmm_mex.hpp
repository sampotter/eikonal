#ifndef __FMM_MEX_HPP__
#define __FMM_MEX_HPP__

#include <cstddef>

#include "typedefs.hpp"

enum class marcher_type {
  basic,
  olim8_mp0,
  olim8_mp1,
  olim8_rhr
};

extern "C"
void fmm_mex(double * out, bool * in, size_t M, size_t N, double h,
			 double * pvalues, marcher_type type);

#endif // __FMM_MEX_HPP__

// Local Variables:
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
