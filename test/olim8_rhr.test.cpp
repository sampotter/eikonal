#include "olim8.hpp"
#include "olim.test.common.hpp"

int main() {
  using olim = olim8_rhr;

  trivial_case_works<olim>();
  adjacent_update_works<olim>();
  error_is_monotonic<olim>();
}
