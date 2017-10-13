#ifndef __OLIM8LUT_IMPL_HPP__
#define __OLIM8LUT_IMPL_HPP__

#include <algorithm>

#include <src/config.hpp>

template <class update_rules>
void olim8<update_rules>::update_impl(int i, int j, double & T) {
  using std::min;

  abstract_node * nb[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  abstract_node * x0 = 0x0;
  abstract_node * x1 = 0x0;
  get_valid_neighbors(i, j, nb);
  double s = speed(i, j), s0, s1, u0, u1, h = get_h();

  // implementing constrained algorithm for now
  for (int k = 0, l = 1, m = 2; k < 8; k += 2, l = k + 1, m = (l + 1) % 8) {
    switch (!!nb[k] + 2*!!nb[l] + 2*!!nb[m]) {
    case 2:
      LINE2(k);
      break;
    case 4:
      TRI2(l, m);
      break;
    case 3:
    case 6:
      LINE1(k);
      break;
    case 5: 
    case 7:
      TRI2(k, l);
      break;
    default:
      break;
    }
  }
}

#endif // __OLIM8LUT_IMPL_HPP__

// Local Variables:
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
