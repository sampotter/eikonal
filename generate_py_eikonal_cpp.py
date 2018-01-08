#!/usr/bin/env python

import argparse

from string import Template

marchers = {
    'basic_marcher': 'BasicMarcher',
    'olim4_mp0': 'Olim4Mid0',
    'olim4_rhr': 'Olim4Rect',
    'olim8_mp0': 'Olim8Mid0',
    'olim8_mp1': 'Olim8Mid1',
    'olim8_rhr': 'Olim8Rect'
}

# TODO: we should probably make the format that's computed here
# something that's computed and returned by the marcher on the C++
# side. This would make things a little simpler and cleaner here.
marcher_template = Template('''
  py::class_<${cpp_class_name}>(m, "${py_class_name}", py::buffer_protocol())
    .def_buffer([] (${cpp_class_name} & m_) -> py::buffer_info {
        auto const format =
          py::format_descriptor<${cpp_class_name}::float_type>::format();
        return {
          m_.get_node_pointer(),
          sizeof(${cpp_class_name}::float_type),
          format,
          ${cpp_class_name}::ndims,
          {m_.get_height(), m_.get_width()},
          {
            sizeof(${cpp_class_name}::node_type)*m_.get_width(), 
            sizeof(${cpp_class_name}::node_type)
          }
        };
      })
    .def(
      py::init<int, int, double, speed_function, double, double>(),
      "height"_a,
      "width"_a,
      "h"_a = 1.0,
      "s"_a = py::cpp_function(static_cast<speed_func>(default_speed_func)),
      "x0"_a = 0.0,
      "y0"_a = 0.0)
    .def("run", &${cpp_class_name}::run)
    .def(
      "addBoundaryNode",
      py::overload_cast<int, int, double>(
        &${cpp_class_name}::add_boundary_node),
      "i"_a,
      "j"_a,
      "value"_a = 0.0)
    .def("getValue", &${cpp_class_name}::get_value, "i"_a, "j"_a);
''')

def get_3d_marcher_map(compile_all_3d_marchers=False):
    d = dict()
    if compile_all_3d_marchers:
        for i in range(2**8):
            groups = tuple((i & (1 << j)) >> j for j in range(8))
            for mode in ['rhr', 'mp0', 'mp1']:
                fmt = ((mode,) + groups)
                k = 'olim3d_%s<groups_t<%d, %d, %d, %d, %d, %d, %d, %d>>'
                v = '%s_%d%d%d%d%d%d%d%d'
                d[k % fmt] = v % fmt
    else:
        d['basic_marcher_3d'] = 'BasicMarcher3D'
        d['olim6_mp0'] = 'Olim6Mid0'
        d['olim6_mp1'] = 'Olim6Mid1'
        d['olim6_rhr'] = 'Olim6Rect'
        d['olim18_mp0'] = 'Olim18Mid0'
        d['olim18_mp1'] = 'Olim18Mid1'
        d['olim18_rhr'] = 'Olim18Rect'
        d['olim26_mp0'] = 'Olim26Mid0'
        d['olim26_mp1'] = 'Olim26Mid1'
        d['olim26_rhr'] = 'Olim26Rect'
    return d

# TODO: see comment above for `marcher_template' variable.
marcher3d_template = Template('''
  py::class_<${cpp_class_name}>(m, "${py_class_name}", py::buffer_protocol())
    .def_buffer([] (${cpp_class_name} & m_) -> py::buffer_info {
        auto const format =
          py::format_descriptor<${cpp_class_name}::float_type>::format();
        return {
          m_.get_node_pointer(),
          sizeof(${cpp_class_name}::float_type),
          format,
          ${cpp_class_name}::ndims,
          {m_.get_height(), m_.get_width(), m_.get_depth()},
          {
            sizeof(${cpp_class_name}::node_type)*m_.get_width(),
            sizeof(${cpp_class_name}::node_type),
            sizeof(${cpp_class_name}::node_type)*m_.get_width()*m_.get_height(),
          }
        };
      })
    .def(
      py::init<int, int, int, double, speed_function_3d, double, double, double>(),
      "height"_a,
      "width"_a,
      "depth"_a,
      "h"_a = 1.0,
      "s"_a = py::cpp_function(static_cast<speed_func_3d>(default_speed_func)),
      "x0"_a = 0.0,
      "y0"_a = 0.0,
      "z0"_a = 0.0)
    .def("run", &${cpp_class_name}::run)
    .def(
      "addBoundaryNode",
      py::overload_cast<int, int, int, double>(
        &${cpp_class_name}::add_boundary_node),
      "i"_a,
      "j"_a,
      "k"_a,
      "value"_a = 0.0)
    .def("getValue", &${cpp_class_name}::get_value, "i"_a, "j"_a, "k"_a);
''')

def build_src_txt():
    src_txt = '''
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include "fmm.h"

#include "basic_marcher.hpp"
#include "basic_marcher_3d.hpp"
#include "olim.hpp"
#include "olim3d.hpp"

namespace py = pybind11;
using namespace py::literals;

using speed_function = std::function<double(double, double)>;
using speed_function_3d = std::function<double(double, double, double)>;

PYBIND11_MODULE(eikonal, m) {
  m.doc() = "Testing testing";
'''
    for k in sorted(marchers.keys()):
        v = marchers[k]
        src_txt += marcher_template.substitute(
            cpp_class_name=k, py_class_name=v)
    for k in sorted(marchers3d.keys()):
        v = marchers3d[k]
        src_txt += marcher3d_template.substitute(
            cpp_class_name=k, py_class_name=v)
    src_txt += '''}
'''
    return src_txt

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--all', action='store_true')
    args = parser.parse_args()

    marchers3d = get_3d_marcher_map(args.all)

    src_txt = build_src_txt()

    print(src_txt.strip())
