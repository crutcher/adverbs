#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "adverbs.h"


NB_MODULE(adverbs_pyext, m) {
   m.doc() = "Python bindings for adverbs";

   m.def("device_names", []() {
        adverbs::scoped_device_list device_list;
        std::vector<std::string> names;
        for (auto& dev : device_list) {
             names.push_back(dev->name);
        }
        return names;
   });
}
