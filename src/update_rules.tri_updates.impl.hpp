#ifndef __UPDATE_RULES_TRI_UPDATES_IMPL_HPP__
#define __UPDATE_RULES_TRI_UPDATES_IMPL_HPP__

#include <src/config.hpp>

#include <algorithm>
#if PRINT_UPDATES
#    include <cstdio>
#endif
#include <cmath>

#include "common.hpp"
#include "common.defs.hpp"
#include "common.macros.hpp"
#include "hybrid.hpp"
#include "update_rules.utils.hpp"

#define l__(x) std::sqrt((dp_dot_dp*(x) + 2*dp_dot_p0)*(x) + p0_dot_p0)
#define check__(x) std::fabs(alpha*l__(x) - dp_dot_p0 - dp_dot_dp*(x))

#define char_sqrt__(c) _sqrt_table[static_cast<int>(c)]
#define s__(x) (s + (1 - x)*s0 + x*s1)/2

#define F0_line__(i) (u##i + h*(s + s##i)*char_sqrt__(p##i##_dot_p##i)/2)

template <char p0, char p1>
update_info<1>
update_rules::mp0_tri_updates::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  ffvec<p0>, ffvec<p1>, double tol) const
{
  using std::min;

  (void) tol;

  static constexpr double _sqrt_table[4] = {
    0.0,
    1.0,
    1.4142135623730951,
    1.7320508075688772
  };

  constexpr char p0_dot_p0 = dot(p0, p0);
  constexpr char p0_dot_p1 = dot(p0, p1);
  constexpr char p1_dot_p1 = dot(p1, p1);
  constexpr char dp_dot_p0 = p0_dot_p1 - p0_dot_p0;
  constexpr char dp_dot_p0_sq = dp_dot_p0*dp_dot_p0;
  constexpr char dp_dot_dp = p1_dot_p1 - 2*p0_dot_p1 + p0_dot_p0;

  double const du = u1 - u0;
  double const alpha = -du/(s__(0.5)*h), alpha_sq = alpha*alpha;
  double const tmp = alpha_sq - dp_dot_dp;
  double const a = dp_dot_dp*tmp;
  double const b = dp_dot_p0*tmp;
  double const c = alpha_sq*p0_dot_p0 - dp_dot_p0_sq;
  double const disc = b*b - a*c;

  update_info<1> update;
  if (disc < 0 || a == 0) {
    double const tmp0 = F0_line__(0), tmp1 = F0_line__(1);
    if (tmp0 < tmp1) {
      update.value = tmp0;
      update.lambda[0] = 0;
    } else {
      update.value = tmp1;
      update.lambda[0] = 1;
    }
  } else {
    double const lhs = -b/a, rhs = sqrt(disc)/a;
    double const lam1 = lhs - rhs, lam2 = lhs + rhs;
    update.lambda[0] = check__(lam1) < check__(lam2) ? lam1 : lam2;
    update.value = update.lambda[0] < 0 || 1 < update.lambda[0] ?
      min(F0_line__(0), F0_line__(1)) :
      u0 + update.lambda[0]*du + h*s__(update.lambda[0])*l__(update.lambda[0]);
  }

#if PRINT_UPDATES
  printf("tri<%d, %d>::update_impl(u0 = %g, u1 = %g, "
         "s = %g, s0 = %g, s1 = %g, h = %g) -> %g\n",
         p0, p1, u0, u1, s, s0, s1, h, update.value);
#endif

  assert(update.value >= std::max(u0, u1));

  return update;
}

#undef F0_line__

template <int dim>
update_info<1>
update_rules::mp0_tri_updates::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  double const * p0, double const * p1, double const * p_fac, double s_fac,
  double tol) const
{
  double sh = ((s + (s0 + s1)/2)/2)*h, shfac = s_fac*h;
  double lfac0 = dist2<dim>(p0, p_fac);
  double lfac1 = dist2<dim>(p1, p_fac);
  double T0 = shfac*lfac0, T1 = shfac*lfac1;
  double tau0 = u0 - T0, tau1 = u1 - T1, dtau = tau1 - tau0;

  double dp[dim];
  sub<dim>(p1, p0, dp);

  double nu[dim], nufac[dim];

  auto const grad = [&] (double lam) {
    axpy<dim>(lam, dp, p0, nu);
    sub<dim>(nu, p_fac, nufac);
    scal_inplace<dim>(norm2<dim>(nu), nu);
    scal_inplace<dim>(norm2<dim>(nufac), nufac);
    return dtau + shfac*dot<3>(dp, nufac) + sh*dot<3>(dp, nu);
  };

  double arglam;
  hybrid_status status;
  std::tie(arglam, status) = hybrid(grad, 0., 1., tol);

  update_info<1> update;
  if (status == hybrid_status::DEGENERATE) {
    double F0 = tau0 + T0 + sh*norm2<dim>(p0);
    double F1 = tau1 + T1 + sh*norm2<dim>(p1);
    update.lambda[0] = F0 < F1 ? 0 : 1;
    update.value = std::min(F0, F1);
  }
  else {
    update.lambda[0] = arglam;
    axpy<dim>(arglam, dp, p0, nu);
    sub<dim>(nu, p_fac, nufac);
    update.value = tau0 + dtau*arglam + shfac*norm2<dim>(nufac) +
      sh*norm2<dim>(nu);
  }

  assert(update.value >= std::max(u0, u1));

  return update;
}

template <int d>
update_info<1>
update_rules::mp0_tri_updates::tri(
  double const * p0, double const * p1, double u0, double u1,
  double s, double s0, double s1, double h, double tol) const
{
  (void) tol;

  using std::min;

  double const dp[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
  double const dp_dot_dp = dp[0]*dp[0] + dp[1]*dp[1] + dp[2]*dp[2];
  double const dp_dot_p0 = dp[0]*p0[0] + dp[1]*p0[1] + dp[2]*p0[2];
  double const dp_dot_p0_sq = dp_dot_p0*dp_dot_p0;
  double const p0_dot_p0 = p0[0]*p0[0] + p0[1]*p0[1] + p0[2]*p0[2];

  double const du = u1 - u0;
  double const stheta = (s + (s0 + s1)/2)/2;
  double const alpha = -du/(stheta*h), alpha_sq = alpha*alpha;
  double const tmp = alpha_sq - dp_dot_dp;
  double const a = dp_dot_dp*tmp;
  double const b = dp_dot_p0*tmp;
  double const c = alpha_sq*p0_dot_p0 - dp_dot_p0_sq;
  double const disc = b*b - a*c;

  double const F0 = u0 + h*(s + s0)*norm2<d>(p0)/2;
  double const F1 = u1 + h*(s + s1)*norm2<d>(p1)/2;

  update_info<1> update;
  if (disc < 0 || a == 0) {
    if (F0 < F1) {
      update.value = F0;
      update.lambda[0] = 0;
    } else {
      update.value = F1;
      update.lambda[0] = 1;
    }
  } else {
    double const lhs = -b/a, rhs = sqrt(disc)/a;
    double const lam1 = lhs - rhs, lam2 = lhs + rhs;
    update.lambda[0] = check__(lam1) < check__(lam2) ? lam1 : lam2;
    update.value = update.lambda[0] < 0 || 1 < update.lambda[0] ?
      min(F0, F1) :
      u0 + update.lambda[0]*du + h*s__(update.lambda[0])*l__(update.lambda[0]);
  }

#if PRINT_UPDATES
  printf("tri<%d>::update_impl(u0 = %g, u1 = %g, "
         "s = %g, s0 = %g, s1 = %g, h = %g) -> %g\n",
         d, u0, u1, s, s0, s1, h, update.value);
#endif

  assert(update.value >= std::max(u0, u1));

  return update;
}

#define F0_line__(i) (u##i + sh*char_sqrt__(p##i##_dot_p##i))

template <char p0, char p1>
update_info<1>
update_rules::rhr_tri_updates::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  ffvec<p0>, ffvec<p1>, double tol) const
{
  using std::min;

  (void) s0;
  (void) s1;
  (void) tol;

  static constexpr double _sqrt_table[4] = {
    0.0,
    1.0,
    1.4142135623730951,
    1.7320508075688772
  };

  constexpr char p0_dot_p0 = dot(p0, p0);
  constexpr char p0_dot_p1 = dot(p0, p1);
  constexpr char p1_dot_p1 = dot(p1, p1);
  constexpr char dp_dot_p0 = p0_dot_p1 - p0_dot_p0;
  constexpr char dp_dot_p0_sq = dp_dot_p0*dp_dot_p0;
  constexpr char dp_dot_dp = p1_dot_p1 - 2*p0_dot_p1 + p0_dot_p0;

  double const du = u1 - u0;
  double const sh = s*h;
  double const alpha = -du/sh, alpha_sq = alpha*alpha;
  double const tmp = alpha_sq - dp_dot_dp;
  double const a = dp_dot_dp*tmp;
  double const b = dp_dot_p0*tmp;
  double const c = alpha_sq*p0_dot_p0 - dp_dot_p0_sq;
  double const disc = b*b - a*c;

  update_info<1> update;
  if (disc < 0 || a == 0) {
    double const tmp0 = F0_line__(0), tmp1 = F0_line__(1);
    if (tmp0 < tmp1) {
      update.value = tmp0;
      update.lambda[0] = 0;
    } else {
      update.value = tmp1;
      update.lambda[0] = 1;
    }
  } else {
    double const lhs = -b/a, rhs = sqrt(disc)/a;
    double const lam1 = lhs - rhs, lam2 = lhs + rhs;
    update.lambda[0] = check__(lam1) < check__(lam2) ? lam1 : lam2;
    update.value = update.lambda[0] < 0 || 1 < update.lambda[0] ?
      min(F0_line__(0), F0_line__(1)) :
      u0 + update.lambda[0]*du + sh*l__(update.lambda[0]);
  }
#if PRINT_UPDATES
  printf("tri<%d, %d>::update_impl(u0 = %g, u1 = %g, "
         "s = %g, h = %g) -> %g\n", p0, p1, u0, u1, s, h, update.value);
#endif

  assert(update.value >= std::max(u0, u1));

  return update;
}

#undef char_sqrt__
#undef s__

template <int dim>
update_info<1>
update_rules::rhr_tri_updates::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  double const * p0, double const * p1, double const * p_fac, double s_fac,
  double tol) const
{
  (void) s0;
  (void) s1;

  double sh = s*h, shfac = s_fac*h;
  double lfac0 = dist2<dim>(p0, p_fac);
  double lfac1 = dist2<dim>(p1, p_fac);
  double T0 = shfac*lfac0, T1 = shfac*lfac1;
  double tau0 = u0 - T0, tau1 = u1 - T1, dtau = tau1 - tau0;

  double dp[dim];
  sub<dim>(p1, p0, dp);

  double nu[dim], nufac[dim];

  auto const grad = [&] (double lam) {
    axpy<dim>(lam, dp, p0, nu);
    sub<dim>(nu, p_fac, nufac);
    scal_inplace<dim>(norm2<dim>(nu), nu);
    scal_inplace<dim>(norm2<dim>(nufac), nufac);
    return dtau + shfac*dot<3>(dp, nufac) + sh*dot<3>(dp, nu);
  };

  double arglam;
  hybrid_status status;
  std::tie(arglam, status) = hybrid(grad, 0., 1., tol);

  update_info<1> update;
  if (status == hybrid_status::DEGENERATE) {
    double F0 = tau0 + T0 + sh*norm2<dim>(p0);
    double F1 = tau1 + T1 + sh*norm2<dim>(p1);
    update.lambda[0] = F0 < F1 ? 0 : 1;
    update.value = std::min(F0, F1);
  }
  else {
    update.lambda[0] = arglam;
    axpy<dim>(arglam, dp, p0, nu);
    sub<dim>(nu, p_fac, nufac);
    update.value = tau0 + dtau*arglam + shfac*norm2<dim>(nufac) +
      sh*norm2<dim>(nu);
  }

  assert(update.value >= std::max(u0, u1));

  return update;
}

// TODO: even though this is a general template, this is actually a
// specialization for d = 3! this needs to be fixed at some point
template <int d>
update_info<1>
update_rules::rhr_tri_updates::tri(
  double const * p0, double const * p1, double u0, double u1,
  double s, double s0, double s1, double h, double tol) const
{
  (void) tol;

  using std::min;

  double const dp[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
  double const dp_dot_dp = dp[0]*dp[0] + dp[1]*dp[1] + dp[2]*dp[2];
  double const dp_dot_p0 = dp[0]*p0[0] + dp[1]*p0[1] + dp[2]*p0[2];
  double const dp_dot_p0_sq = dp_dot_p0*dp_dot_p0;
  double const p0_dot_p0 = p0[0]*p0[0] + p0[1]*p0[1] + p0[2]*p0[2];

  double const du = u1 - u0;
  double const sh = s*h;
  double const alpha = -du/sh, alpha_sq = alpha*alpha;
  double const tmp = alpha_sq - dp_dot_dp;
  double const a = dp_dot_dp*tmp;
  double const b = dp_dot_p0*tmp;
  double const c = alpha_sq*p0_dot_p0 - dp_dot_p0_sq;
  double const disc = b*b - a*c;

  double const F0 = u0 + h*(s + s0)*norm2<d>(p0)/2;
  double const F1 = u1 + h*(s + s1)*norm2<d>(p1)/2;

  update_info<1> update;
  if (disc < 0 || a == 0) {
    if (F0 < F1) {
      update.value = F0;
      update.lambda[0] = 0;
    } else {
      update.value = F1;
      update.lambda[0] = 1;
    }
  } else {
    double const lhs = -b/a, rhs = sqrt(disc)/a;
    double const lam1 = lhs - rhs, lam2 = lhs + rhs;
    update.lambda[0] = check__(lam1) < check__(lam2) ? lam1 : lam2;
    update.value = update.lambda[0] < 0 || 1 < update.lambda[0] ?
      min(F0, F1) :
      u0 + update.lambda[0]*du + sh*l__(update.lambda[0]);
  }

#if PRINT_UPDATES
  printf("tri<%d>::update_impl(u0 = %g, u1 = %g, s = %g, h = %g) -> %g\n",
         d, u0, u1, s, h, update.value);
#endif

  assert(update.value >= std::max(u0, u1));

  return update;
}

#undef check__
#undef l__

#define u__(x) ((1 - (x))*u0 + (x)*u1)
#define q__(x) ((dp_dot_dp*x + 2*dp_dot_p0)*(x) + p0_dot_p0)
#define l__(x) std::sqrt(q__(x))
#define s__(x) ((s + (1 - (x))*s0 + (x)*s1)/2)
#define F1__(x) (u__(x) + h*s__(x)*l__(x))
#define dF1__(x) (du + h*(ds*q__(x)/2 + s__(x)*dp_dot_plam)/l__(x))
#define d2F1__(x) h*(s__(x)*dp_dot_plam*dp_dot_plam/q__(x) + \
                     ds*dp_dot_plam/2 + 2*s__(x)*dp_dot_dp)/l__(x)

/**
 * F1 specialization
 */
template <char p0, char p1>
update_info<1>
update_rules::mp1_tri_updates::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  ffvec<p0>, ffvec<p1>, double tol) const
{
  constexpr char p0_dot_p0 = dot(p0, p0);
  constexpr char p0_dot_p1 = dot(p0, p1);
  constexpr char p1_dot_p1 = dot(p1, p1);
  constexpr char dp_dot_p0 = p0_dot_p1 - p0_dot_p0;
  constexpr char dp_dot_dp = p1_dot_p1 - 2*p0_dot_p1 + p0_dot_p0;

  constexpr double c1 = 1e-4;
  double const ds = s1 - s0, du = u1 - u0;

  update_info<1> update;

  // Check gradients at endpoints to see if we can skip this update
  double dp_dot_plam = dp_dot_p0;
  if (dF1__(0) > 0) {
    update.value = F1__(0);
    update.lambda[0] = 0;
    return update;
  }
  dp_dot_plam += dp_dot_dp;
  if (dF1__(1) < 0) {
    update.value = F1__(1);
    update.lambda[0] = 1;
    return update;
  }

  // Try to minimize F1 by starting from the mp0 minimizer using
  // Newton's method. This may fail if the function isn't
  // well-behaved. To try to determine when this happens, keep track
  // of the iteration count, and use a modified Newton's method if we
  // exceed a limit of 10 iterations.
  mp0_tri_updates mp0;
  update = mp0.tri(u0, u1, s, s0, s1, h, ffvec<p0> {}, ffvec<p1> {}, tol);
  double lam = update.lambda[0], g, iter = 0;
  do {
    dp_dot_plam = dp_dot_p0 + lam*dp_dot_dp;
    g = -dF1__(lam)/d2F1__(lam);
    lam = std::max(0., std::min(1., lam + g));
  } while (iter++ < 10 && fabs(g) > tol);

  if (iter == 10) {
    bool conv;
    double lam[2], F1[2], dF1, d2F1, g, alpha;
    lam[0] = update.lambda[0];
    F1[0] = F1__(lam[0]);
    do {
      alpha = 1;
      dp_dot_plam = dp_dot_p0 + lam[0]*dp_dot_dp;
      dF1 = dF1__(lam[0]);
      d2F1 = d2F1__(lam[0]);
      g = -dF1/d2F1;

      double lam_ = lam[0] + alpha*g;
      double u_ = u__(lam_);
      double s_ = s__(lam_);
      double l_ = l__(lam_);
      double tmp = u_ + h*s_*l_;
      while (tmp > F1[0] + c1*alpha*dF1*g) {
        alpha *= 0.9;
        lam_ = lam[0] + alpha*g;
        u_ = u__(lam_);
        s_ = s__(lam_);
        l_ = l__(lam_);
        tmp = u_ + h*s_*l_;
      }
      lam[1] = std::max(0., std::min(1., lam[0] + alpha*g));
      F1[1] = F1__(lam[1]);
      conv = fabs(lam[1] - lam[0]) <= tol || fabs(F1[1] - F1[0]) <= tol;
      lam[0] = lam[1];
      F1[0] = F1[1];
    } while (!conv);
    update.value = F1[1];
    update.lambda[0] = lam[1];
  } else {
    update.value = F1__(lam);
    update.lambda[0] = lam;
  }

#if PRINT_UPDATES
  printf("tri<%d, %d>::update_impl(u0 = %g, u1 = %g, "
         "s = %g, s0 = %g, s1 = %g, h = %g) -> %g\n",
         p0, p1, u0, u1, s, s0, s1, h, update.value);
#endif

  assert(update.value >= std::max(u0, u1));

  return update;
}

template <int dim>
update_info<1>
update_rules::mp1_tri_updates::tri(
  double u0, double u1, double s, double s0, double s1, double h,
  double const * p0, double const * p1, double const * p_fac, double s_fac,
  double tol) const
{
  double shfac = s_fac*h;
  double ds = s1 - s0;
  double T0 = shfac*dist2<dim>(p0, p_fac);
  double T1 = shfac*dist2<dim>(p1, p_fac);
  double tau0 = u0 - T0, tau1 = u1 - T1, dtau = tau1 - tau0;

  double dp[dim], nu[dim], nufac[dim];
  sub<dim>(p1, p0, dp);

  double s_lam, l_lam;

  auto const grad = [&] (double lam) {
    axpy<dim>(lam, dp, p0, nu);
    sub<dim>(nu, p_fac, nufac);
    l_lam = norm2<dim>(nu);
    scal_inplace<dim>(l_lam, nu);
    scal_inplace<dim>(norm2<dim>(nufac), nufac);
    s_lam = (s + s0 + ds*lam)/2;
    return dtau + shfac*dot<3>(dp, nufac) +
      h*(s_lam*dot<3>(dp, nu) + l_lam*ds/2);
  };

  double arglam;
  hybrid_status status;
  std::tie(arglam, status) = hybrid(grad, 0., 1., tol);

  update_info<1> update;
  if (status == hybrid_status::DEGENERATE) {
    double F0 = tau0 + T0 + (s + s0)*h*norm2<dim>(p0)/2;
    double F1 = tau1 + T1 + (s + s1)*h*norm2<dim>(p1)/2;
    update.lambda[0] = F0 < F1 ? 0 : 1;
    update.value = std::min(F0, F1);
  }
  else {
    update.lambda[0] = arglam;
    axpy<dim>(arglam, dp, p0, nu);
    sub<dim>(nu, p_fac, nufac);
    s_lam = (s + s0 + ds*arglam)/2;
    update.value = tau0 + dtau*arglam + shfac*norm2<dim>(nufac) +
      s_lam*h*norm2<dim>(nu);
  }

  assert(update.value >= std::max(u0, u1));

  return update;
}

template <int d>
update_info<1>
update_rules::mp1_tri_updates::tri(
  double const * p0, double const * p1, double u0, double u1,
  double s, double s0, double s1, double h, double tol) const
{
  double const dp[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};

  double const dp_dot_dp = dp[0]*dp[0] + dp[1]*dp[1] + dp[2]*dp[2];
  double const dp_dot_p0 = dp[0]*p0[0] + dp[1]*p0[1] + dp[2]*p0[2];
  double const p0_dot_p0 = p0[0]*p0[0] + p0[1]*p0[1] + p0[2]*p0[2];

  constexpr double c1 = 1e-4;

  double const ds = s1 - s0, du = u1 - u0;

  update_info<1> update;

  // Check gradients at endpoints to see if we can skip this update
  double dp_dot_plam = dp_dot_p0;
  if (dF1__(0) > 0) {
    update.value = F1__(0);
    update.lambda[0] = 0;
    return update;
  }
  dp_dot_plam += dp_dot_dp;
  if (dF1__(1) < 0) {
    update.value = F1__(1);
    update.lambda[0] = 1;
    return update;
  }

  // Try to minimize F1 by starting from the mp0 minimizer using
  // Newton's method. This may fail if the function isn't
  // well-behaved. To try to determine when this happens, keep track
  // of the iteration count, and use a modified Newton's method if we
  // exceed a limit of 10 iterations.
  mp0_tri_updates mp0;
  update = mp0.tri<d>(p0, p1, u0, u1, s, s0, s1, h, tol);
  double lam = update.lambda[0], g, iter = 0;
  do {
    dp_dot_plam = dp_dot_p0 + lam*dp_dot_dp;
    g = -dF1__(lam)/d2F1__(lam);
    lam = std::max(0., std::min(1., lam + g));
  } while (iter++ < 10 && fabs(g) > tol);

  if (iter == 10) {
    bool conv;
    double lam[2], F1[2], dF1, d2F1, g, alpha;
    lam[0] = update.lambda[0];
    F1[0] = F1__(lam[0]);
    do {
      alpha = 1;
      dp_dot_plam = dp_dot_p0 + lam[0]*dp_dot_dp;
      dF1 = dF1__(lam[0]);
      d2F1 = d2F1__(lam[0]);
      g = -dF1/d2F1;

      double lam_ = lam[0] + alpha*g;
      double u_ = u__(lam_);
      double s_ = s__(lam_);
      double l_ = l__(lam_);
      double tmp = u_ + h*s_*l_;
      while (tmp > F1[0] + c1*alpha*dF1*g) {
        alpha *= 0.9;
        lam_ = lam[0] + alpha*g;
        u_ = u__(lam_);
        s_ = s__(lam_);
        l_ = l__(lam_);
        tmp = u_ + h*s_*l_;
      }
      lam[1] = std::max(0., std::min(1., lam[0] + alpha*g));
      F1[1] = F1__(lam[1]);
      conv = fabs(lam[1] - lam[0]) <= tol || fabs(F1[1] - F1[0]) <= tol;
      lam[0] = lam[1];
      F1[0] = F1[1];
    } while (!conv);
    update.value = F1[1];
    update.lambda[0] = lam[1];
  } else {
    update.value = F1__(lam);
    update.lambda[0] = lam;
  }

#if PRINT_UPDATES
  printf("tri<%d, %d>::update_impl(u0 = %g, u1 = %g, "
         "s = %g, s0 = %g, s1 = %g, h = %g) -> %g\n",
         p0, p1, u0, u1, s, s0, s1, h, update.value);
#endif

  assert(update.value >= std::max(u0, u1));

  return update;
}

#undef u__
#undef q__
#undef l__
#undef theta__
#undef s__
#undef F1__
#undef dF1__
#undef d2F1__

#endif // __UPDATE_RULES_TRI_UPDATES_IMPL_HPP__
