#ifndef __MARCHER_IMPL_HPP_HPP__
#define __MARCHER_IMPL_HPP_HPP__

#include <cassert>
#include <cmath>
#if PRINT_UPDATES
#    include <cstdio>
#endif // PRINT_UPDATES

#include "common.macros.hpp"
#include "offsets.hpp"

#define __di(k) di<2>[k]
#define __dj(k) dj<2>[k]

static inline size_t get_initial_heap_size(int width, int height) {
  return static_cast<size_t>(std::max(8.0, std::log(width*height)));
}

template <class base, class node>
marcher<base, node>::marcher(int height, int width, double h,
                             no_speed_func_t const &):
  abstract_marcher {get_initial_heap_size(width, height)},
  _nodes {new node[width*height]},
  _s_cache {new double[width*height]},
  _h {h},
  _height {height},
  _width {width}
{
  init();
}

template <class base, class node>
marcher<base, node>::marcher(int height, int width, double h,
                             double const * s_cache):
  abstract_marcher {get_initial_heap_size(width, height)},
  _nodes {new node[width*height]},
  _s_cache {new double[width*height]},
  _h {h},
  _height {height},
  _width {width}
{
  memcpy((void *) _s_cache, (void *) s_cache, sizeof(double)*height*width);
  init();
}

template <class base, class node>
marcher<base, node>::marcher(int height, int width, double h,
                       std::function<double(double, double)> s,
                       double x0, double y0):
  abstract_marcher {get_initial_heap_size(width, height)},
  _nodes {new node[width*height]},
  _s_cache {new double[width*height]},
  _h {h},
  _height {height},
  _width {width}
{
  double * ptr = const_cast<double *>(_s_cache);
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      ptr[i*width + j] = s(h*j - x0, h*i - y0);
    }
  }

  init();
}

template <class base, class node>
marcher<base, node>::~marcher()
{
  delete[] _nodes;
  delete[] _s_cache;
}

/**
 * TODO: this function is a little broken right now. If we remove the
 * "is_far" assertion and allow the user to add boundary nodes
 * wherever, then it's possible to invalidate the heap property in the
 * underlying min-heap. One way to get this to happen is to add a
 * boundary node and then add another boundary node which is adjacent
 * to the node just added. This should be fixed, since there is a
 * correct way to enable this behavior.
 */
template <class base, class node>
void marcher<base, node>::add_boundary_node(int i, int j, double value) {
#if PRINT_UPDATES
  printf("add_boundary_node(i = %d, j = %d, value = %g)\n", i, j, value);
#endif // PRINT_UPDATES
  assert(in_bounds(i, j));
  assert(operator()(i, j).is_far());
  visit_neighbors(&(operator()(i, j) = {i, j, value}));
}

template <class base, class node>
void marcher<base, node>::add_boundary_node(double x, double y, double value) {
  auto const dist = [x, y] (int i, int j) -> double {
    return std::sqrt((i - y)*(i - y) + (j - x)*(j - x));
  };

  int i0 = floor(y), i1 = ceil(y);
  int j0 = floor(x), j1 = ceil(x);

  node nodes[4] = {
    {i0, j0, value + dist(i0, j0)},
    {i0, j1, value + dist(i0, j1)},
    {i1, j0, value + dist(i1, j0)},
    {i1, j1, value + dist(i1, j1)}
  };

  add_boundary_nodes(nodes, 4);
}

template <class base, class node>
void marcher<base, node>::add_boundary_nodes(node const * nodes, int num) {
#if PRINT_UPDATES
  printf("add_boundary_nodes(nodes = %p, num = %d)\n", nodes, num);
#endif // PRINT_UPDATES

  node const * n;
  int i, j;

  /**
   * First, add the sequence of nodes to the grid.
   */
  for (int k = 0; k < num; ++k) {
    n = &nodes[k];
    i = n->get_i();
    j = n->get_j();
    assert(in_bounds(i, j));
    assert(operator()(i, j).is_far());
    operator()(i, j) = {i, j, n->get_value()};
  }

  /**
   * Next, with the nodes added, update their neighbors---this is done
   * in order to avoid breaking the heap property of the underlying
   * min-heap. Batch adding of boundary nodes may also be more
   * efficient for a large number of boundary nodes? (a guess)
   */
  for (int k = 0; k < num; ++k) {
    n = &nodes[k];
    i = n->get_i();
    j = n->get_j();
#if PRINT_UPDATES
    printf("add_boundary_node(i = %d, j = %d, value = %g)\n",
           i, j, n->get_value());
#endif // PRINT_UPDATES
    visit_neighbors(&operator()(i, j));
  }
}

template <class base, class node>
double marcher<base, node>::get_value(int i, int j) const {
  assert(in_bounds(i, j));
  return operator()(i, j).get_value();
}

template <class base, class node>
node & marcher<base, node>::operator()(int i, int j) {
  assert(in_bounds(i, j));
  return _nodes[_width*i + j];
}

template <class base, class node>
node const & marcher<base, node>::operator()(int i, int j) const {
  assert(in_bounds(i, j));
  return _nodes[_width*i + j];
}

template <class base, class node>
bool marcher<base, node>::in_bounds(int i, int j) const {
  return (unsigned) i < (unsigned) _height && (unsigned) j < (unsigned) _width;
}

template <class base, class node>
double marcher<base, node>::get_speed(int i, int j) const {
  assert(in_bounds(i, j));
  return _s_cache[_width*i + j];
}

template <class base, class node>
bool marcher<base, node>::is_valid(int i, int j) const {
  return in_bounds(i, j) && operator()(i, j).is_valid();
}

template <class base, class node>
void marcher<base, node>::init() {
  /**
   * Set the indices associated with each node in the grid.
   */
  for (int i = 0; i < _height; ++i) {
    for (int j = 0; j < _width; ++j) {
      operator()(i, j).set_i(i);
      operator()(i, j).set_j(j);
    }
  }
}

template <class base, class node>
void marcher<base, node>::visit_neighbors_impl(abstract_node * n) {
  int i = static_cast<node *>(n)->get_i();
  int j = static_cast<node *>(n)->get_j();

#if PRINT_UPDATES
  printf("marcher::visit_neighbors_impl(i = %d, j = %d)\n",
         i, j);
#endif

  // Create an array to keep a pointer to all neighboring nodes that
  // are valid.
  abstract_node * nb[8], * child_nb[8];
  memset(nb, 0x0, 8*sizeof(abstract_node *));

  // Traverse each neighboring node that's in bounds (on the grid):
  // stage the neighbor if its state is `far', or add it to `nb' if
  // it's valid. Note that after this step, all neighboring nodes will
  // either be trial or far, so `nb' does double duty as a registry of
  // node states (if nb[k] == nullptr, then the node has state
  // `trial'; otherwise, `valid').
  int a, b;
  for (int k = 0; k < 8; ++k) {
    a = i + __di(k), b = j + __dj(k);
    if (!in_bounds(a, b)) {
      continue;
    }
    if (k < base::nneib && operator()(a, b).is_far()) {
      operator()(a, b).set_trial();
      insert_into_heap(&operator()(a, b));
    } else if (operator()(a, b).is_valid()) {
      nb[k] = &this->operator()(a, b);
    }
  }

  // TODO: this is a temporary hack. Our goal is to fix update_impl so
  // that it doesn't need to operate on a full neighborhood (an array
  // like `nb' above). However, as a temporary means of getting part
  // of our changes working, we'll just fill out a "fake" array
  // here. That is, even though we will only need to incorporate the
  // neighbors in one quadrant, if we pass this array down to
  // update_impl, we won't need to modify its operation.
  auto const set_child_nb = [&] (int k) {
    memset(child_nb, 0x0, 8*sizeof(abstract_node *));
    switch (k) {
    case 0:
      child_nb[1] = nb[4];
      child_nb[2] = &this->operator()(i, j);
      child_nb[3] = nb[7];
      break;
    case 1:
      break;
      child_nb[0] = nb[4];
      child_nb[2] = nb[5];
      child_nb[3] = &this->operator()(i, j);
    case 2:
      child_nb[0] = &this->operator()(i, j);
      child_nb[1] = nb[5];
      child_nb[3] = nb[6];
      break;
    case 3:
      child_nb[0] = nb[7];
      child_nb[1] = &this->operator()(i, j);
      child_nb[2] = nb[6];
      break;
    case 4:
      child_nb[2] = nb[1];
      child_nb[3] = nb[0];
      break;
    case 5:
      child_nb[0] = nb[1];
      child_nb[3] = nb[2];
      break;
    case 6:
      child_nb[0] = nb[3];
      child_nb[1] = nb[2];
      break;
    case 7:
      child_nb[1] = nb[0];
      child_nb[2] = nb[3];
      break;
    }
    if (base::nneib >= 8) {
      switch (k) {
      case 0:
        child_nb[5] = nb[1];
        child_nb[6] = nb[3];
        break;
      case 1:
        child_nb[6] = nb[2];
        child_nb[7] = nb[0];
        break;
      case 2:
        child_nb[4] = nb[1];
        child_nb[7] = nb[3];
        break;
      case 3:
        child_nb[4] = nb[0];
        child_nb[5] = nb[2];
        break;
      case 4:
        child_nb[6] = &this->operator()(i, j);
        break;
      case 5:
        child_nb[7] = &this->operator()(i, j);
        break;
      case 6:
        child_nb[4] = &this->operator()(i, j);
        break;
      case 7:
        child_nb[5] = &this->operator()(i, j);
        break;
      }
    }
  };

  auto const update = [&] (int i, int j) {
    double T = INF(double);
    update_impl(i, j, child_nb, T);
    auto n = &operator()(i, j);
    if (T < n->get_value()) {
      n->set_value(T);
      adjust_heap_entry(n);
    }
  };

  for (int k = 0; k < base::nneib; ++k) {
    if (!nb[k]) {
      a = i + __di(k), b = j + __dj(k);
      if (!in_bounds(a, b)) {
        continue;
      }
      set_child_nb(k);
      update(a, b);
    }
  }
}

#undef __di
#undef __dj

#endif // __MARCHER_IMPL_HPP_HPP__
