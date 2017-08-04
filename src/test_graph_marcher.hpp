#ifndef __TEST_GRAPH_MARCHER_HPP__
#define __TEST_GRAPH_MARCHER_HPP__

#include "graph_marcher.hpp"
#include "graph_node.hpp"
#include "speed_funcs.hpp"
#include "typedefs.h"

struct test_graph_marcher: public graph_marcher<graph_node> {
  test_graph_marcher(int height, int width, double h = 1,
                     speed_func S = default_speed_func, double x0 = 0.0,
                     double y0 = 0.0);
  void add_boundary_node(int i, int j, double value = 0);
  double get_value(int i, int j) const;
protected:
  virtual double S(graph_node * node) const override final;
  virtual void get_valid_neighbors(abstract_node * node, abstract_node ** nb)
    override final;
  virtual void update_impl(graph_node * node, double & T) override final;

  double get_h() const { return _h; }

private:
  speed_func _S;
  int _height, _width;
  double _h;
};

#endif // __TEST_GRAPH_MARCHER_HPP__

// Local Variables:
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
