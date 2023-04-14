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
 *     adverbs::scoped_device_list device_list;
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
  scoped_device_list()
      : _device_list(ibv_get_device_list(&_size), ibv_free_device_list) {}

  /**
   * Get a reference to the device at index i.
   *
   * @param i The index of the device to return.
   * @return A reference to the device at index i.
   */
  const struct ibv_device &operator[](size_t i) const {
    return *(_device_list.get()[i]);
  }

  /**
   * Find an ibv_device by name.
   * Returns nullptr if no device with the given name is found.
   *
   * Example:
   *
   *     adverbs::scoped_device_list device_list;
   *     auto dev = device_list.lookup_by_name("mlx5_0");
   *     if (dev) {
   *         std::cout << dev->name << std::endl;
   *     }
   *
   * @param name The name of the device to find.
   * @return A pointer to the device with the given name, or nullptr if no such
   * device exists.
   */
  [[nodiscard]]
  const struct ibv_device *lookup_by_name(const char *name) const {
    return lookup_by_predicate([name](const struct ibv_device *dev) {
      return strncmp(dev->name, name, IBV_SYSFS_NAME_MAX) == 0;
    });
  }

  /**
   * Lookup an ibv_device by name.
   * Returns nullptr if no device with the given name is found.
   *
   * Example:
   *
   *     adverbs::scoped_device_list device_list;
   *     auto dev = device_list.lookup_by_name("mlx5_0");
   *     if (dev) {
   *         std::cout << dev->name << std::endl;
   *     }
   *
   * @param name The name of the device to find.
   * @return A pointer to the device with the given name, or nullptr if no such
   * device exists.
   */
  [[nodiscard]]
  const struct ibv_device *lookup_by_name(const std::string &name) const {
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
  [[nodiscard]]
  const struct ibv_device *lookup_by_kernel_index(int kernel_index) const {
    return lookup_by_predicate([kernel_index](const struct ibv_device *dev) {
      return ibv_get_device_index(const_cast<struct ibv_device *>(dev)) ==
             kernel_index;
    });
  }

  /**
   * Find an ibv_device by GUID.
   * Returns nullptr if no device with the given GUID is found.
   *
   * @param guid the Globally Unique Identifier of the device to find.
   * @return A pointer to the device with the given GUID, or nullptr if no such
   * device exists.
   */
  [[nodiscard]]
  const struct ibv_device *lookup_by_guid(uint64_t guid) const {
    return lookup_by_predicate([guid](const struct ibv_device *dev) {
      return ibv_get_device_guid(const_cast<struct ibv_device *>(dev)) == guid;
    });
  }

  /**
   * Find an ibv_device by predicate.
   * Returns nullptr if no device matching the given predicate is found.
   *
   * @param predicate A function that returns true if the device matches the
   * predicate.
   * @return a pointer to the device that matches the predicate, or nullptr if
   * no such device exists.
   */
  [[nodiscard]]
  const struct ibv_device *lookup_by_predicate(
      const std::function<bool(const struct ibv_device *)> &predicate) const {
    for (const auto dev : *this) {
      if (predicate(dev)) return dev;
    }
    return nullptr;
  }

  /**
   * Get a pointer to the underlying ibv_device array.
   *
   * @return A pointer to the underlying ibv_device array.
   */
  [[nodiscard]]
  struct ibv_device *const *get() const {
    return _device_list.get();
  }

  /**
   * Get the number of devices in the list.
   *
   * @return The number of devices in the list.
   */
  [[nodiscard]]
  size_t size() const {
    return (size_t)_size;
  }

  [[nodiscard]]
  const_iterator begin() const {
    return _device_list.get();
  }

  [[nodiscard]]
  const_iterator end() const {
    return _device_list.get() + _size;
  }

 private:
  int _size = 0;
  std::shared_ptr<struct ibv_device *> _device_list;
};

/**
 * RAII wrapper for ibv_open_device and ibv_close_device
 *
 * Example usage:
 *
 *     adverbs::scoped_device_list device_list;
 *     adverbs::context_handle dev(device_list[0]);
 *     auto attr = dev.get_device_attr();
 *     std::cout << attr.max_qp << std::endl;
 */
class context_handle {
 public:
  explicit context_handle(const struct ibv_device *device)
      : _context(
            ibv_open_device(const_cast<struct ibv_device *>(device)),
            ibv_close_device) {}

  struct ibv_context *get() { return _context.get(); }

  /**
   * Query the device attributes.
   * Calls ibv_query_device.
   *
   * @return struct ibv_device_attr containing the device attributes.
   * @throws std::runtime_error if ibv_query_device fails.
   */
  [[nodiscard]]
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
   * @throws std::runtime_error if ibv_query_port fails.
   */
  [[nodiscard]]
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
   * @throws std::runtime_error if ibv_query_port fails.
   */
  [[nodiscard]]
  std::vector<struct ibv_port_attr> query_ports(
      const std::function<bool(const struct ibv_port_attr &)> &filter) const {
    auto ports = query_ports();
    ports.erase(
        std::remove_if(ports.begin(), ports.end(), filter),
        ports.end());
    return ports;
  }

 private:
  std::shared_ptr<struct ibv_context> _context;
};

}  // namespace adverbs

#endif  // ADVERBS_LIBRARY_H
