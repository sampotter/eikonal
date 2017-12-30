#ifndef __UPDATE_RULES_TRI_UPDATES_IMPL_HPP__
#define __UPDATE_RULES_TRI_UPDATES_IMPL_HPP__

#include <algorithm>

#include "common.hpp"
#include "common.defs.hpp"
#include "common.macros.hpp"
#include "olim_util.hpp"

template <class speed_est, char degree>
template <char p0, char p1>
double
update_rules::tri_updates<speed_est, degree>::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  ffvec<p0>, ffvec<p1>) const
{
  return tri_impl(
    u0, u1, s, s0, s1, h,
    ffvec<p0> {}, ffvec<p1> {},
    std::integral_constant<char, degree> {});
}

/**
 * F0 specialization
 */
template <class speed_est, char degree>
template <char p0, char p1>
double
update_rules::tri_updates<speed_est, degree>::tri_impl(
  double u0, double u1, double s, double s0, double s1, double h,
  ffvec<p0>, ffvec<p1>, std::integral_constant<char, 0>) const
{
  constexpr char p0_dot_p0 = dot(p0, p0);
  constexpr char p0_dot_p1 = dot(p0, p1);
  constexpr char p1_dot_p1 = dot(p1, p1);
  constexpr char dp_dot_p0 = p0_dot_p1 - p0_dot_p0;
  constexpr char dp_dot_dp = p1_dot_p1 - 2*p0_dot_p1 + p0_dot_p0;

  double const du = u1 - u0;
  double const sh = this->s_hat(s, s0, s1)*h;
  double const alpha = -du/sh, alpha_sq = alpha*alpha;
  double const tmp = alpha_sq - dp_dot_dp;
  double const a = dp_dot_dp*tmp;
  double const b = dp_dot_p0*tmp;
  double const c = alpha_sq*p0_dot_p0 - dp_dot_p0*dp_dot_p0;
  double const disc = b*b - a*c;

  if (disc < 0) {
    return std::min(u0 + sh*sqrt(p0_dot_p0), u1 + sh*sqrt(p1_dot_p1));
  } else {
    double const lhs = -b/a;
    double const rhs = sqrt(disc)/a;
    double const lam1 = lhs - rhs, lam2 = lhs + rhs;
    double const l1 = sqrt((dp_dot_dp*lam1 + 2*dp_dot_p0)*lam1 + p0_dot_p0);
    double const l2 = sqrt((dp_dot_dp*lam2 + 2*dp_dot_p0)*lam2 + p0_dot_p0);
    double const check1 = alpha*l1 - dp_dot_p0 - lam1*dp_dot_dp;
    double const check2 = alpha*l2 - dp_dot_p0 - lam2*dp_dot_dp;
    double const lam = check1 < check2 ? lam1 : lam2;
    if (lam < 0 || 1 < lam) {
      return std::min(u0 + sh*sqrt(p0_dot_p0), u1 + sh*sqrt(p1_dot_p1));
    } else {
      return (1 - lam)*u0 + lam*u1 +
        sh*sqrt(dp_dot_dp*lam*lam + 2*dp_dot_p0*lam + p0_dot_p0);
    }
  }
}

#define u__(x) ((1 - (x))*u0 + (x)*u1)
#define s__(x) this->s_hat(s, s0, s1, (x))
#define q__(x) ((dp_dot_dp*x + 2*dp_dot_p0)*(x) + p0_dot_p0)
#define l__(x) std::sqrt(q__(x))
#define F1__(x) (u__(x) + h*s__(x)*l__(x))
#define dF1__(x) (du + h*(ds*this->theta()*q__(x) + s__(x)*dp_dot_plam)/l__(x))
#define d2F1__(x) h*(s__(x)*dp_dot_plam*dp_dot_plam/q__(x) + \
                     ds*this->theta()*dp_dot_plam + 2*s__(x)*dp_dot_dp)/l__(x)

template <class speed_est, char degree>
template <char p0, char p1>
double
update_rules::tri_updates<speed_est, degree>::tri_impl(
  double u0, double u1, double s, double s0, double s1, double h,
  ffvec<p0>, ffvec<p1>, std::integral_constant<char, 1>) const
{
  constexpr double tol = 1e-13;

  constexpr char p0_dot_p0 = dot(p0, p0);
  constexpr char p0_dot_p1 = dot(p0, p1);
  constexpr char p1_dot_p1 = dot(p1, p1);
  constexpr char dp_dot_p0 = p0_dot_p1 - p0_dot_p0;
  constexpr char dp_dot_dp = p1_dot_p1 - 2*p0_dot_p1 + p0_dot_p0;

  double const ds = s1 - s0, du = u1 - u0;

  bool conv;
  double lam[2], F1[2];
  double g, dp_dot_plam;
  lam[0] = 0.5;
  F1[0] = F1__(lam[0]);
  do {
    dp_dot_plam = dp_dot_p0 + lam[0]*dp_dot_dp;
    g = -dF1__(lam[0])/d2F1__(lam[0]);
    lam[1] = std::max(0., std::min(1., lam[0] - g));
    F1[1] = F1__(lam[1]);
    conv = fabs(lam[1] - lam[0]) <= tol || fabs(F1[1] - F1[0]) <= tol;
    lam[0] = lam[1];
    F1[0] = F1[1];
  } while (!conv);
  return F1[1];
}

#undef u__
#undef s__
#undef q__
#undef l__
#undef F1__
#undef dF1__
#undef d2F1__

#endif // __UPDATE_RULES_TRI_UPDATES_IMPL_HPP__
