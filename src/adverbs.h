#ifndef ADVERBS_LIBRARY_H
#define ADVERBS_LIBRARY_H

#include <infiniband/verbs.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace adverbs {

// RAII wrapper for ibv_get_device_list and ibv_free_device_list
// Usage:
//     scoped_device_list device_list;
//     for (const auto& dev : device_list) {
//         std::cout << dev->name << std::endl;
//     }
//
//     for (auto it = device_list.begin(); it != device_list.end(); ++it) {
//         std::cout << (*it)->name << std::endl;
//     }
//
//     for (int i = 0; i < device_list.size(); ++i) {
//         std::cout << device_list[i].name << std::endl;
//     }
class scoped_device_list {
 public:
  typedef struct ibv_device *const *const_iterator;

  scoped_device_list() { _device_list = ibv_get_device_list(&num_devices); }

  ~scoped_device_list() { ibv_free_device_list(_device_list); }

  const struct ibv_device &operator[](size_t i) const {
    return *(_device_list[i]);
  }

  struct ibv_device *const *get() { return _device_list; }

  int size() { return num_devices; }

  const_iterator begin() const { return _device_list; }
  const_iterator end() const { return _device_list + num_devices; }

 private:
  //  int num;
  //  struct ibv_device** devices = ibv_get_device_list(&num);
  //  for (int i = 0; i < num; ++i) {
  //    if (strncmp(devices[i]->name, ibDevName, IBV_SYSFS_NAME_MAX) == 0) {
  //      _ctx->ctx = ibv_open_device(devices[i]);
  //      break;
  //    }

  struct ibv_device **_device_list;
  int num_devices;
};

}  // namespace adverbs

#endif  // ADVERBS_LIBRARY_H
