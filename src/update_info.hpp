#pragma once

#include "vec.hpp"

template <int d>
struct update_info;

template <>
struct update_info<1>
{
  double value {inf<double>};
  vec<double, 1> lambda = {0.5};
  double tol {1e1*eps<double>};
  inline bool inbounds() const {
    return 0 <= lambda[0] && lambda[0] <= 1;
  }
  inline bool finite_lambda() const {
    return isfinite(lambda[0]);
  }
  inline bool in_interior() const {
    return eps<double> <= lambda[0] && lambda[0] <= 1 - eps<double>;
  }
};

template <>
struct update_info<2>
{
  double value {inf<double>};
  vec2<double> lambda = {1./3, 1./3};
  double tol {1e1*eps<double>};
  inline bool inbounds() const {
    return 0 <= lambda[0] && 0 <= lambda[1] && lambda[0] + lambda[1] <= 1;
  }
  inline bool finite_lambda() const {
    return isfinite(lambda[0]) && isfinite(lambda[1]);
  }
  inline bool in_interior() const {
    return eps<double> <= lambda[0] && eps<double> <= lambda[1] &&
      eps<double> <= 1 - lambda[0] - lambda[1];
  }
};