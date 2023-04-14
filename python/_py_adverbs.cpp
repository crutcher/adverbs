#include <infiniband/verbs.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "adverbs.h"

namespace nb = nanobind;
using namespace nb::literals;

// This is a poorman's substitute for std::format, which is a C++20 feature.
template <typename... Args>
std::string format(const std::string& format, Args... args) {
// Shutup format warning.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

  // Dry-run to the get the buffer size:
  // Extra space for '\0'
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }

  // allocate buffer
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);

  // actually format
  std::snprintf(buf.get(), size, format.c_str(), args...);

  // Bulid the return string.
  // We don't want the '\0' inside
  return std::string(buf.get(), buf.get() + size - 1);

#pragma GCC diagnostic pop
}

struct DeviceProxy {
  static constexpr const char* __doc__ = R"""(
    A device represents a single Infiniband device.

    Attributes:
      kernel_index: The kernel index of the device.
      guid: The GUID of the device.
      node_type: The node type of the device.
      name: The name of the device.
      dev_name: The device name of the device.
      dev_path: The device path of the device.
      ibdev_path: The ibdev path of the device.
  )""";

  explicit DeviceProxy(const struct ibv_device* dev)
      : kernel_index(ibv_get_device_index(const_cast<ibv_device*>(dev))),
        guid(ibv_get_device_guid(const_cast<ibv_device*>(dev))),
        node_type(dev->node_type),
        name(dev->name),
        dev_name(dev->dev_name),
        dev_path(dev->dev_path),
        ibdev_path(dev->ibdev_path) {}

  [[nodiscard]]
  std::string to_string() const {
    return "<DeviceProxy kernel_index=" + std::to_string(kernel_index) +
           " guid=" + std::to_string(guid) + " node_type=\"" +
           std::string(ibv_node_type_str(node_type)) + "\" name=\"" + name +
           "\" dev_name=\"" + dev_name + "\" dev_path=\"" + dev_path +
           "\" ibdev_path=\"" + ibdev_path + "\">";
  }

  friend std::ostream& operator<<(std::ostream& os, const DeviceProxy& device) {
    return os << device.to_string();
  }

  [[nodiscard]]
  std::vector<std::string> py_dir() const {
    // sorted by name
    return {
        "dev_name",
        "dev_path",
        "guid",
        "ibdev_path",
        "kernel_index",
        "name",
        "node_type",
    };
  }

  [[nodiscard]]
  adverbs::context_handle open() const {
    adverbs::scoped_device_list device_list;
    auto* _dev = device_list.lookup_by_kernel_index(kernel_index);
    if (_dev == nullptr) {
      throw std::runtime_error(
          format("DeviceProxy with kernel index %d not found", kernel_index));
    }
    return adverbs::context_handle(_dev);
  }

  const int kernel_index;
  const __be64 guid;

  const ibv_node_type node_type;
  const std::string name;
  const std::string dev_name;
  const std::string dev_path;
  const std::string ibdev_path;
};

std::vector<DeviceProxy> list_devices() {
  adverbs::scoped_device_list device_list;
  std::vector<DeviceProxy> devices;
  devices.reserve(device_list.size());
  for (auto dev : device_list) {
    devices.emplace_back(dev);
  }
  return devices;
}

NB_MODULE(_py_adverbs, m) {
  m.doc() = "Python bindings for adverbs";

  m.def("list_devices", &list_devices, "List all Infiniband devices.");

  nb::enum_<ibv_node_type>(m, "NodeType")
      .value("Unknown", IBV_NODE_UNKNOWN)
      .value("Ca", IBV_NODE_CA)
      .value("Switch", IBV_NODE_SWITCH)
      .value("Router", IBV_NODE_ROUTER)
      .value("Rdma", IBV_NODE_RNIC)
      .value("Usnic", IBV_NODE_USNIC)
      .value("Usnic_UDP", IBV_NODE_USNIC_UDP)
      .export_values();

  nb::class_<DeviceProxy>(m, "Device")
      .def_ro_static("__doc__", &DeviceProxy::__doc__)
      .def_ro("kernel_index", &DeviceProxy::kernel_index)
      .def_ro("guid", &DeviceProxy::guid)
      .def_ro("node_type", &DeviceProxy::node_type)
      .def_ro("name", &DeviceProxy::name)
      .def_ro("dev_name", &DeviceProxy::dev_name)
      .def_ro("dev_path", &DeviceProxy::dev_path)
      .def_ro("ibdev_path", &DeviceProxy::ibdev_path)
      .def("__dir__", &DeviceProxy::py_dir)
      .def("__repr__", &DeviceProxy::to_string)
      .def("__str__", &DeviceProxy::to_string)
      .def("open", &DeviceProxy::open);

  nb::class_<adverbs::context_handle>(m, "Context");
}
