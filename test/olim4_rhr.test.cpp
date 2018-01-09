#include "olim.test.common.hpp"

#include <basic_marcher.hpp>
#include <olim.hpp>

int main() {
  using olim = olim4_rhr;

  trivial_case_works<olim>();
  adjacent_update_works<olim>();
  for (auto & s: speed_funcs) {
    olims_agree<olim, basic_marcher>(s);
  }
  result_is_symmetric<olim>((speed_func) default_speed_func);
  result_is_symmetric<olim>((speed_func) s1);
  correct_corners_in_limit<olim>(101, 1.86e-2);
}
