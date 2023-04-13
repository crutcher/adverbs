#ifndef ADVERBS_LIBRARY_H
#define ADVERBS_LIBRARY_H

#include <infiniband/verbs.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace adverbs {

/**
 * RAII wrapper for ibv_get_device_list and ibv_free_device_list
 *
 * Example usage:
 *
 *     scoped_device_list device_list;
 *     for (const auto& dev : device_list) {
 *         std::cout << dev->name << std::endl;
 *     }
 *
 *     for (auto it = device_list.begin(); it != device_list.end(); ++it) {
 *         std::cout << (*it)->name << std::endl;
 *     }
 *
 *     for (int i = 0; i < device_list.size(); ++i) {
 *         std::cout << device_list[i].name << std::endl;
 *     }
 */
class scoped_device_list {
 public:
  typedef struct ibv_device *const *const_iterator;

  /**
   * Construct a scoped_device_list.
   * Calls ibv_get_device_list to populate the list of devices.
   */
  scoped_device_list() { _device_list = ibv_get_device_list(&num_devices); }

  /**
   * Destruct a scoped_device_list.
   * Calls ibv_free_device_list to free the list of devices.
   */
  ~scoped_device_list() { ibv_free_device_list(_device_list); }

  /**
   * Get a reference to the device at index i.
   *
   * @param i The index of the device to return.
   * @return A reference to the device at index i.
   */
  const struct ibv_device &operator[](size_t i) const {
    return *(_device_list[i]);
  }

  /**
   * Find an ibv_device by name.
   * Returns nullptr if no device with the given name is found.
   *
   * Example:
   *
   *     scoped_device_list device_list;
   *     auto dev = device_list.lookup_by_name("mlx5_0");
   *     if (dev) {
   *         std::cout << dev->name << std::endl;
   *     }
   *
   * @param name The name of the device to find.
   * @return A pointer to the device with the given name, or nullptr if no such
   * device exists.
   */
  const struct ibv_device *const lookup_by_name(const char *name) const {
    for (int i = 0; i < num_devices; ++i) {
      if (strncmp(_device_list[i]->name, name, IBV_SYSFS_NAME_MAX) == 0) {
        return _device_list[i];
      }
    }
    return nullptr;
  }

  /**
   * Lookup an ibv_device by name.
   * Returns nullptr if no device with the given name is found.
   *
   * Example:
   *
   *     scoped_device_list device_list;
   *     auto dev = device_list.lookup_by_name("mlx5_0");
   *     if (dev) {
   *         std::cout << dev->name << std::endl;
   *     }
   *
   * @param name The name of the device to find.
   * @return A pointer to the device with the given name, or nullptr if no such
   * device exists.
   */
  const struct ibv_device *const lookup_by_name(const std::string &name) const {
    return lookup_by_name(name.c_str());
  }

  /**
   * Find an ibv_device by kernel device index.
   * Returns nullptr if no device with the given index is found.
   *
   * Only available on kernels with support for IB device query
   * over netlink interface.
   *
   * @param kernel_index The kernel device index of the device to find.
   * @return A pointer to the device with the given name, or nullptr if no such
   * device exists.
   */
  const struct ibv_device *const lookup_by_kernel_index(
      int kernel_index) const {
    for (auto dev : *this) {
      if (ibv_get_device_index(dev) == kernel_index) return dev;
    }

    return nullptr;
  }

  struct ibv_device *const *get() { return _device_list; }

  /**
   * Get the number of devices in the list.
   *
   * @return The number of devices in the list.
   */
  int size() { return num_devices; }

  const_iterator begin() const { return _device_list; }
  const_iterator end() const { return _device_list + num_devices; }

 private:
  struct ibv_device **_device_list;
  int num_devices;
};

/**
 * RAII wrapper for ibv_open_device and ibv_close_device
 *
 * Example usage:
 *
 *     scoped_device_list device_list;
 *     device_handle dev(device_list[0]);
 *     auto attr = dev.get_device_attr();
 *     std::cout << attr.max_qp << std::endl;
 */
class device_handle {
 public:
  device_handle(const struct ibv_device *device)
      : _context(
            ibv_open_device(const_cast<struct ibv_device *>(device)),
            ibv_close_device) {}

  struct ibv_context *get() { return _context.get(); }

  /**
   * Query the device attributes.
   * Calls ibv_query_device.
   *
   * @return struct ibv_device_attr containing the device attributes.
   */
  struct ibv_device_attr query_device_attr() const {
    struct ibv_device_attr attr;
    if (ibv_query_device(_context.get(), &attr)) {
      throw std::runtime_error("ibv_query_device failed");
    }
    return attr;
  }

  /**
   * Query the port attributes.
   * Calls ibv_query_port for each port.
   *
   * @return a vector of struct ibv_port_attr containing the port attributes.
   */
  std::vector<struct ibv_port_attr> query_ports() const {
    struct ibv_device_attr attr = query_device_attr();
    std::vector<struct ibv_port_attr> port_attr(attr.phys_port_cnt);
    for (int i = 0; i < attr.phys_port_cnt; ++i) {
      if (ibv_query_port(_context.get(), i + 1, &port_attr[i])) {
        throw std::runtime_error("ibv_query_port failed");
      }
    }
    return port_attr;
  }

  /**
   * Query the port attributes.
   * Calls ibv_query_port for each port.
   *
   * @param filter A function that returns true if the port should be included
   * in the result.
   * @return a vector of struct ibv_port_attr containing the port attributes.
   */
  std::vector<struct ibv_port_attr> query_ports(
      std::function<bool(const struct ibv_port_attr &)> filter) const {
    auto ports = query_ports();
    ports.erase(
        std::remove_if(
            ports.begin(),
            ports.end(),
            [&filter](const auto &port) { return !filter(port); }),
        ports.end());
    return ports;
  }

 private:
  std::shared_ptr<struct ibv_context> _context;
};

}  // namespace adverbs

#endif  // ADVERBS_LIBRARY_H
