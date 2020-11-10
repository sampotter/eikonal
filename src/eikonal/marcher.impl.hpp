#pragma once

#include <assert.h>
#include <math.h>

#include "../common.hpp"
#include "../range.hpp"

#include "offsets.hpp"

// TODO: really need an external memory constructor that will let us
// use external memory somewhere for _s and _U so we don't have
// to double up on these...

template <class derived, int n, int num_nb, ordering ord>
eikonal::marcher<derived, n, num_nb, ord>::marcher(
  ivec dims, double h, no_slow_t const &):
  base_marcher<marcher<derived, n, num_nb, ord>, n> {dims},
  _s(this->_size),
  _h {h}
{
  // TODO: this may be unnecessary
  detail::fill_boundary<double, n, ord>(dims, &_s[0], inf<double>);

  /**
   * Precompute arrays of offsets to child nodes.
   */
  for (int i = 0; i < num_nb; ++i) {
    ivec offset = detail::get_offset<n>(i);
    for (int j = 0; j < num_nb; ++j) {
      ivec offset_ = offset + detail::get_offset<n>(j);
      _child_offset[i][j] = offset_.normi() > 1 ?
        -1 :
        get_linear_offset<n>(offset_);
    }
  }
}

template <class derived, int n, int num_nb, ordering ord>
eikonal::marcher<derived, n, num_nb, ord>::marcher(ivec dims, double h):
  marcher {dims, h, no_slow_t {}}
{
  for (int i = 0; i < this->_size; ++i) {
    _s[i] = 1;
  }
}

template <class derived, int n, int num_nb, ordering ord>
eikonal::marcher<derived, n, num_nb, ord>::marcher(
  ivec dims, double h, double const * s):
  marcher {dims, h, no_slow_t {}}
{
  // TODO: this isn't the most efficient way to do this. It would be
  // better to iterate over pointers to the heads of each maximum
  // contiguous block of memory and call memcpy for each one. This is
  // more complicated but would reduce the amount of index math
  // substantially.
  for (auto inds: range<n, ord> {dims}) {
    auto lin = this->to_linear_index(inds + ivec::one());
    _s[lin] = s[::to_linear_index<ord>(inds, dims)];
  }
}

template <class derived, int n, int num_nb, ordering ord>
void
eikonal::marcher<derived, n, num_nb, ord>::add_src(fvec coords, double s, double U)
{
  double h = get_h();
  fvec inds = coords/h;
  assert(this->in_bounds(inds));
  inds += fvec::one();

  // TODO: this isn't as general as it could be. We also want to
  // handle cases where i or j are grid-aligned, in which case we need
  // to process 6 nodes. If i and j are both grid aligned, then we
  // should just call the integer add_src.
  int corners[n][2];
  for (int i = 0; i < n; ++i) {
    corners[i][0] = (int) floor(inds[i]);
    corners[i][1] = (int) floor(inds[i]) + 1;
  }

  for (auto inds_: range<n, ord>::cube(2)) {
    ivec inds__;
    for (int i = 0; i < n; ++i) {
      inds__[i] = corners[i][inds_[i]];
    }

    fvec p = inds - fvec {inds__};

    int lin = this->to_linear_index(inds__);
    this->_U[lin] = static_cast<derived *>(this)->line(p.norm2(), U, _s[lin], s, h);
    this->_state[lin] = state::trial;
    this->_heap.insert(lin);
  }
}

template <class derived, int n, int num_nb, ordering ord>
void
eikonal::marcher<derived, n, num_nb, ord>::factor(int * inds, fac_src const * src)
{
  factor(ivec {inds}, src);
}

template <class derived, int n, int num_nb, ordering ord>
void
eikonal::marcher<derived, n, num_nb, ord>::factor(ivec inds, fac_src const * src)
{
#if OLIM_DEBUG && !RELWITHDEBINFO
  assert(this->in_bounds(inds));
#endif
  inds += ivec::one();
  _fac_srcs[this->to_linear_index(inds)] = src;
}

template <class derived, int n, int num_nb, ordering ord>
double
eikonal::marcher<derived, n, num_nb, ord>::get_s(ivec inds) const
{
#if OLIM_DEBUG && !RELWITHDEBINFO
  assert(in_bounds(inds));
#endif
  return _s[to_linear_index(inds + ivec::one())];
}

template <int n, int num_nb>
int get_parent(int i);

template <>
int get_parent<2, 4>(int i) {
  static constexpr int lut[4] = {2, 3, 0, 1};
  return lut[i];
}

template <>
int get_parent<2, 8>(int i) {
  static constexpr int lut[8] = {2, 3, 0, 1, 6, 7, 4, 5};
  return lut[i];
}

template <>
int get_parent<3, 6>(int i) {
  static constexpr int lut[6] = {3, 4, 5, 0, 1, 2};
  return lut[i];
}

template <>
int get_parent<3, 18>(int i) {
  static constexpr int lut[18] = {
    3, 4, 5, 0, 1, 2,
    16, 17, 14, 15, 12, 13, 10, 11,  8,  9,  6,  7
  };
  return lut[i];
}

template <>
int get_parent<3, 26>(int i) {
  static constexpr int lut[26] = {
    3, 4, 5, 0, 1, 2,
    16, 17, 14, 15, 12, 13, 10, 11,  8,  9,  6,  7,
    24, 25, 22, 23, 20, 21, 18, 19
  };
  return lut[i];
}

template <class derived, int n, int num_nb, ordering ord>
void
eikonal::marcher<derived, n, num_nb, ord>::visit_neighbors(int lin_center)
{
  int valid_nb[detail::max_num_nb(n)];
  int child_nb[num_nb];

  // TODO: another optimization idea:
  // - we gather stuff from "general memory" (_state[lin], _U[lin}, etc.)
  // - it might make sense to fetch all of stuff here, and then use SIMD
  //   instructions to do comparisons
  // - many of the things we do with this stuff can be done fully
  //   in parallel

  // Find the valid neighbors in the "full" neighborhood of n
  // (i.e. the unit max norm ball).
  for (int i = 0; i < detail::max_num_nb(n); ++i) {
    int lin = lin_center + this->_linear_offset[i];
    valid_nb[i] = this->_state[lin] == state::valid ? lin : -1;
  }

  // This function fills the `child_nb' array with the indices of the
  // neighboring nodes of the child node that will be updated. Since
  // we're solving the eikonal equation, we can get away with only
  // checking for nodes which are at most a distance of 1 in the max
  // norm from the newly valid node.
  auto const set_child_nb = [&] (int parent, int const * offsets) {
    for (int i = 0; i < num_nb; ++i) {
      child_nb[i] = offsets[i] == -1 ? -1 : valid_nb[offsets[i]];
    }
    child_nb[parent] = lin_center;
  };

  // Update the node at (i, j). Before calling, `nb' needs to be
  // filled appropriately. Upon updating, this sets the value of n and
  // adjusts its position in the heap.
  auto const update = [&] (int lin_hat, int parent, state state_) {
#if OLIM_DEBUG && !RELWITHDEBINFO
    assert(state_ == state::trial);
#endif
    auto U = inf<double>;
    static_cast<derived *>(this)->update_impl(lin_hat, child_nb, parent, U);
    if (U < this->_U[lin_hat]) {
#if OLIM_DEBUG && !RELWITHDEBINFO
      assert(U >= 0);
#endif
      this->_U[lin_hat] = U;
      if (state_ == state::trial) {
        this->_heap.update(lin_hat);
      }
    }
  };

  // This is the main update loop. Each neighbor of n which isn't
  // `valid' is now `trial' or `boundary'. We ignore `boundary' nodes;
  // for each neighboring `trial` node, use `set_child_nb' to grab its
  // valid neighbors and use `update' to actually update the node's
  // value and update its position in the heap.
  for (int i = 0; i < num_nb; ++i) {
    if (valid_nb[i] == -1) {
      int lin = lin_center + this->_linear_offset[i];
      auto const state_ = this->_state[lin];
      if (state_ == state::trial) {
        int parent = get_parent<n, num_nb>(i);
        set_child_nb(parent, _child_offset[i]);
        update(lin, parent, state_);
      }
    }
  }
}
