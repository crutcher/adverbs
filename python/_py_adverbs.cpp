#include <infiniband/verbs.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "adverbs.h"

#ifndef IBV_PORT_LINK_SPEED_NDR_SUP
#define IBV_PORT_LINK_SPEED_NDR_SUP ((enum ibv_port_cap_flags2)(1 << 10))
#endif

namespace nb = nanobind;
using namespace nb::literals;

// This is a poorman's substitute for std::format, which is a C++20 feature.
// nanobind currently forces C++17; so we don't have access to it here.
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

  return buf.get();

#pragma GCC diagnostic pop
}

// This class is a proxy for the ibv_device struct.
//
// By using the proxy struct, we can avoid holding a pointer to the
// ibv_device struct, which is not meant to be held past the lifetime of
// the ibv_get_device_list call.
//
// We can open a context on the device using the kernel index, which is
// guaranteed to be stable across ibv_get_device_list calls; and we can
// search for the same device in the list, and open a context on it.
struct IBDeviceProxy {
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

  [[nodiscard]]
  static std::vector<std::string> py_dir() {
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

  const int kernel_index;
  const __be64 guid;
  const ibv_node_type node_type;
  const std::string name;
  const std::string dev_name;
  const std::string dev_path;
  const std::string ibdev_path;

  explicit IBDeviceProxy(const struct ibv_device* dev)
      : kernel_index(ibv_get_device_index(const_cast<ibv_device*>(dev))),
        guid(ibv_get_device_guid(const_cast<ibv_device*>(dev))),
        node_type(dev->node_type),
        name(dev->name),
        dev_name(dev->dev_name),
        dev_path(dev->dev_path),
        ibdev_path(dev->ibdev_path) {}

  [[nodiscard]]
  std::string to_string() const {
    return "<IBDeviceProxy kernel_index=" + std::to_string(kernel_index) +
           " guid=" + std::to_string(guid) + " node_type=\"" +
           std::string(ibv_node_type_str(node_type)) + "\" name=\"" + name +
           "\" dev_name=\"" + dev_name + "\" dev_path=\"" + dev_path +
           "\" ibdev_path=\"" + ibdev_path + "\">";
  }

  friend std::ostream& operator<<(
      std::ostream& os,
      const IBDeviceProxy& device) {
    return os << device.to_string();
  }

  [[nodiscard]]
  adverbs::context_handle open() const {
    adverbs::scoped_device_list device_list;
    auto* _dev = device_list.lookup_by_kernel_index(kernel_index);
    if (_dev == nullptr) {
      throw std::runtime_error(
          format("IBDeviceProxy with kernel index %d not found", kernel_index));
    }
    return adverbs::context_handle(_dev);
  }
};

std::vector<IBDeviceProxy> list_devices() {
  adverbs::scoped_device_list device_list;
  std::vector<IBDeviceProxy> devices;
  devices.reserve(device_list.size());
  for (auto dev : device_list) {
    devices.emplace_back(dev);
  }
  return devices;
}

static constexpr const char* __doc__ = R"""(
    This module provides access to the Infiniband verbs API through the adverbs library.
)""";

NB_MODULE(_py_adverbs, m) {
  m.doc() = __doc__;

  nb::enum_<ibv_gid_type>(m, "IBV_GID_TYPE")
      .value("IBV_GID_TYPE_IB", IBV_GID_TYPE_IB)
      .value("IBV_GID_TYPE_ROCE_V1", IBV_GID_TYPE_ROCE_V1)
      .value("IBV_GID_TYPE_ROCE_V2", IBV_GID_TYPE_ROCE_V2)
      .export_values();

  nb::enum_<ibv_node_type>(m, "IBV_NODE_TYPE")
      .value("IBV_NODE_UNKNOWN", IBV_NODE_UNKNOWN)
      .value("IBV_NODE_CA", IBV_NODE_CA)
      .value("IBV_NODE_SWITCH", IBV_NODE_SWITCH)
      .value("IBV_NODE_ROUTER", IBV_NODE_ROUTER)
      .value("IBV_NODE_NRIC", IBV_NODE_RNIC)
      .value("IBV_NODE_USNIC", IBV_NODE_USNIC)
      .value("IBV_NODE_USNIC_UDP", IBV_NODE_USNIC_UDP)
      .export_values();

  nb::enum_<ibv_transport_type>(m, "IBV_TRANSPORT_TYPE")
      .value("IBV_TRANSPORT_UNKNOWN", IBV_TRANSPORT_UNKNOWN)
      .value("IBV_TRANSPORT_IB", IBV_TRANSPORT_IB)
      .value("IBV_TRANSPORT_IWARP", IBV_TRANSPORT_IWARP)
      .value("IBV_TRANSPORT_USNIC", IBV_TRANSPORT_USNIC)
      .value("IBV_TRANSPORT_USNIC_UDP", IBV_TRANSPORT_USNIC_UDP)
      .export_values();

  nb::enum_<ibv_device_cap_flags>(m, "IBV_DEVICE_CAP_FLAGS")
      .value("IBV_DEVICE_RESIZE_MAX_WR", IBV_DEVICE_RESIZE_MAX_WR)
      .value("IBV_DEVICE_BAD_PKEY_CNTR", IBV_DEVICE_BAD_PKEY_CNTR)
      .value("IBV_DEVICE_BAD_QKEY_CNTR", IBV_DEVICE_BAD_QKEY_CNTR)
      .value("IBV_DEVICE_RAW_MULTI", IBV_DEVICE_RAW_MULTI)
      .value("IBV_DEVICE_AUTO_PATH_MIG", IBV_DEVICE_AUTO_PATH_MIG)
      .value("IBV_DEVICE_CHANGE_PHY_PORT", IBV_DEVICE_CHANGE_PHY_PORT)
      .value("IBV_DEVICE_UD_AV_PORT_ENFORCE", IBV_DEVICE_UD_AV_PORT_ENFORCE)
      .value("IBV_DEVICE_CURR_QP_STATE_MOD", IBV_DEVICE_CURR_QP_STATE_MOD)
      .value("IBV_DEVICE_SHUTDOWN_PORT", IBV_DEVICE_SHUTDOWN_PORT)
      .value("IBV_DEVICE_INIT_TYPE", IBV_DEVICE_INIT_TYPE)
      .value("IBV_DEVICE_PORT_ACTIVE_EVENT", IBV_DEVICE_PORT_ACTIVE_EVENT)
      .value("IBV_DEVICE_SYS_IMAGE_GUID", IBV_DEVICE_SYS_IMAGE_GUID)
      .value("IBV_DEVICE_RC_RNR_NAK_GEN", IBV_DEVICE_RC_RNR_NAK_GEN)
      .value("IBV_DEVICE_SRQ_RESIZE", IBV_DEVICE_SRQ_RESIZE)
      .value("IBV_DEVICE_N_NOTIFY_CQ", IBV_DEVICE_N_NOTIFY_CQ)
      .value("IBV_DEVICE_MEM_WINDOW", IBV_DEVICE_MEM_WINDOW)
      .value("IBV_DEVICE_UD_IP_CSUM", IBV_DEVICE_UD_IP_CSUM)
      .value("IBV_DEVICE_XRC", IBV_DEVICE_XRC)
      .value("IBV_DEVICE_MEM_MGT_EXTENSIONS", IBV_DEVICE_MEM_MGT_EXTENSIONS)
      .value("IBV_DEVICE_MEM_WINDOW_TYPE_2A", IBV_DEVICE_MEM_WINDOW_TYPE_2A)
      .value("IBV_DEVICE_MEM_WINDOW_TYPE_2B", IBV_DEVICE_MEM_WINDOW_TYPE_2B)
      .value("IBV_DEVICE_RC_IP_CSUM", IBV_DEVICE_RC_IP_CSUM)
      .value("IBV_DEVICE_RAW_IP_CSUM", IBV_DEVICE_RAW_IP_CSUM)
      .value(
          "IBV_DEVICE_MANAGED_FLOW_STEERING",
          IBV_DEVICE_MANAGED_FLOW_STEERING)
      .export_values();

  nb::enum_<ibv_fork_status>(m, "IBV_FORK_STATUS")
      .value("IBV_FORK_DISABLED", IBV_FORK_DISABLED)
      .value("IBV_FORK_ENABLED", IBV_FORK_ENABLED)
      .value("IBV_FORK_UNNEEDED", IBV_FORK_UNNEEDED)
      .export_values();

  nb::enum_<ibv_atomic_cap>(m, "IBV_ATOMIC_CAP")
      .value("IBV_ATOMIC_NONE", IBV_ATOMIC_NONE)
      .value("IBV_ATOMIC_HCA", IBV_ATOMIC_HCA)
      .value("IBV_ATOMIC_GLOB", IBV_ATOMIC_GLOB)
      .export_values();

  nb::enum_<ibv_port_state>(m, "IBV_PORT_STATE")
      .value("IBV_PORT_NDP", IBV_PORT_NOP)
      .value("IBV_PORT_DOWN", IBV_PORT_DOWN)
      .value("IBV_PORT_INIT", IBV_PORT_INIT)
      .value("IBV_PORT_ARMED", IBV_PORT_ARMED)
      .value("IBV_PORT_ACTIVE", IBV_PORT_ACTIVE)
      .value("IBV_PORT_ACTIVE_DEFER", IBV_PORT_ACTIVE_DEFER)
      .export_values();

  nb::enum_<ibv_port_cap_flags>(m, "IBV_PORT_CAP_FLAGS")
      .value("IBV_PORT_SM", IBV_PORT_SM)
      .value("IBV_PORT_NOTICE_SP", IBV_PORT_NOTICE_SUP)
      .value("IBV_PORT_TRAP_SUP", IBV_PORT_TRAP_SUP)
      .value("IBV_PORT_OPT_IPD_SUP", IBV_PORT_OPT_IPD_SUP)
      .value("IBV_PORT_AUTO_MIGR_SUP", IBV_PORT_AUTO_MIGR_SUP)
      .value("IBV_PORT_SL_MAP_SUP", IBV_PORT_SL_MAP_SUP)
      .value("IBV_PORT_MKEY_NVRAM", IBV_PORT_MKEY_NVRAM)
      .value("IBV_PORT_PKEY_NVRAM", IBV_PORT_PKEY_NVRAM)
      .value("IBV_PORT_LED_INFO_SUP", IBV_PORT_LED_INFO_SUP)
      .value("IBV_PORT_SYS_IMAGE_GUID_SUP", IBV_PORT_SYS_IMAGE_GUID_SUP)
      .value(
          "IBV_PORT_PKEY_SW_EXT_PORT_TRAP_SUP",
          IBV_PORT_PKEY_SW_EXT_PORT_TRAP_SUP)
      .value("IBV_PORT_EXTENDED_SPEEDS_SUP", IBV_PORT_EXTENDED_SPEEDS_SUP)
      .value("IBV_PORT_CM_SUP", IBV_PORT_CM_SUP)
      .value("IBV_PORT_SNMP_TUNNEL_SUP", IBV_PORT_SNMP_TUNNEL_SUP)
      .value("IBV_PORT_REINIT_SUP", IBV_PORT_REINIT_SUP)
      .value("IBV_PORT_DEVICE_MGMT_SUP", IBV_PORT_DEVICE_MGMT_SUP)
      .value("IBV_PORT_VENDOR_CLASS_SUP", IBV_PORT_VENDOR_CLASS_SUP)
      .value("IBV_PORT_DR_NOTICE_SUP", IBV_PORT_DR_NOTICE_SUP)
      .value("IBV_PORT_CAP_MASK_NOTICE_SUP", IBV_PORT_CAP_MASK_NOTICE_SUP)
      .value("IBV_PORT_BOOT_MGMT_SUP", IBV_PORT_BOOT_MGMT_SUP)
      .value("IBV_PORT_LINK_LATENCY_SUP", IBV_PORT_LINK_LATENCY_SUP)
      .value("IBV_PORT_CLIENT_REG_SUP", IBV_PORT_CLIENT_REG_SUP)
      .value("IBV_PORT_IP_BASED_GIDS", IBV_PORT_IP_BASED_GIDS)
      .export_values();

  nb::enum_<ibv_port_cap_flags2>(m, "IBV_PORT_CAP_FLAGS2")
      .value("IBV_PORT_SET_NODE_DESC_SUP", IBV_PORT_SET_NODE_DESC_SUP)
      .value("IBV_PORT_INFO_EXT_SUP", IBV_PORT_INFO_EXT_SUP)
      .value("IBV_PORT_VIRT_SUP", IBV_PORT_VIRT_SUP)
      .value(
          "IBV_PORT_SWITCH_PORT_STATE_TABLE_SUP",
          IBV_PORT_SWITCH_PORT_STATE_TABLE_SUP)
      .value("IBV_PORT_LINK_WIDTH_2X_SUP", IBV_PORT_LINK_WIDTH_2X_SUP)
      .value("IBV_PORT_LINK_SPEED_HDR_SUP", IBV_PORT_LINK_SPEED_HDR_SUP)
      .value("IBV_PORT_LINK_SPEED_NDR_SUP", IBV_PORT_LINK_SPEED_NDR_SUP)
      .export_values();

  nb::enum_<ibv_mtu>(m, "IBV_MTU")
      .value("256", IBV_MTU_256)
      .value("512", IBV_MTU_512)
      .value("1024", IBV_MTU_1024)
      .value("2048", IBV_MTU_2048)
      .value("4096", IBV_MTU_4096)
      .export_values();

  m.def("list_devices", &list_devices, "List all Infiniband devices.");

  nb::class_<IBDeviceProxy>(m, "IBDevice")
      .def_ro_static("__doc__", &IBDeviceProxy::__doc__)
      .def(
          "__dir__",
          [](const IBDeviceProxy& dev) -> std::vector<std::string> {
            // .def() doesn't support static methods on objects;
            // and we don't want to define __dir__ on the class using
            // .def_static(); so we have this little hack.
            return IBDeviceProxy::py_dir();
          })
      .def_ro("kernel_index", &IBDeviceProxy::kernel_index)
      .def_ro("guid", &IBDeviceProxy::guid)
      .def_ro("node_type", &IBDeviceProxy::node_type)
      .def_ro("name", &IBDeviceProxy::name)
      .def_ro("dev_name", &IBDeviceProxy::dev_name)
      .def_ro("dev_path", &IBDeviceProxy::dev_path)
      .def_ro("ibdev_path", &IBDeviceProxy::ibdev_path)
      .def("__repr__", &IBDeviceProxy::to_string)
      .def("__str__", &IBDeviceProxy::to_string)
      .def("open", &IBDeviceProxy::open);

  nb::class_<adverbs::context_handle>(m, "IBContext")
      .def(
          "attr",
          &adverbs::context_handle::query_device_attr,
          "Query the device attributes.")
      .def(
          "ports",
          [](const adverbs::context_handle& ctx) { return ctx.query_ports(); },
          "List the device ports.");

  nb::class_<ibv_device_attr>(m, "IBDeviceAttr")
      .def(
          "__dir__",
          [](const ibv_device_attr& attr) {
            return std::vector<std::string>{
                "max_mr_size",
                "max_qp",
                "max_qp_wr",
                "max_sge",
                "max_sge_rd",
                "max_cq",
                "max_cqe",
                "max_mr",
                "max_pd",
                "max_qp_rd_atom",
                "max_ee_rd_atom",
                "max_res_rd_atom",
                "max_qp_init_rd_atom",
                "max_ee_init_rd_atom",
                "atomic_cap",
                "max_ee",
                "max_rdd",
                "max_mw",
                "max_raw_ipv6_qp",
                "max_raw_ethy_qp",
                "max_mcast_grp",
                "max_mcast_qp_attach",
                "max_total_mcast_qp_attach",
                "max_ah",
                "max_fmr",
                "max_map_per_fmr",
                "max_srq",
                "max_srq_wr",
                "max_srq_sge",
                "max_pkeys",
                "local_ca_ack_delay",
                "phys_port_cnt",
                "fw_ver",
                "node_guid",
                "sys_image_guid",
                "page_size_cap",
                "vendor_id",
                "vendor_part_id",
            };
          })
      .def_ro("fw_ver", &ibv_device_attr::fw_ver)
      .def_ro("node_guid", &ibv_device_attr::node_guid)
      .def_ro("sys_image_guid", &ibv_device_attr::sys_image_guid)
      .def_ro("max_mr_size", &ibv_device_attr::max_mr_size)
      .def_ro("page_size_cap", &ibv_device_attr::page_size_cap)
      .def_ro("vendor_id", &ibv_device_attr::vendor_id)
      .def_ro("vendor_part_id", &ibv_device_attr::vendor_part_id)
      .def_ro("hw_ver", &ibv_device_attr::hw_ver)
      .def_ro("max_qp", &ibv_device_attr::max_qp)
      .def_ro("max_qp_wr", &ibv_device_attr::max_qp_wr)
      .def_ro("device_cap_flags", &ibv_device_attr::device_cap_flags)
      .def_ro("max_sge", &ibv_device_attr::max_sge)
      .def_ro("max_sge_rd", &ibv_device_attr::max_sge_rd)
      .def_ro("max_cq", &ibv_device_attr::max_cq)
      .def_ro("max_cqe", &ibv_device_attr::max_cqe)
      .def_ro("max_mr", &ibv_device_attr::max_mr)
      .def_ro("max_pd", &ibv_device_attr::max_pd)
      .def_ro("max_qp_rd_atom", &ibv_device_attr::max_qp_rd_atom)
      .def_ro("max_ee_rd_atom", &ibv_device_attr::max_ee_rd_atom)
      .def_ro("max_res_rd_atom", &ibv_device_attr::max_res_rd_atom)
      .def_ro("max_qp_init_rd_atom", &ibv_device_attr::max_qp_init_rd_atom)
      .def_ro("max_ee_init_rd_atom", &ibv_device_attr::max_ee_init_rd_atom)
      .def_ro("atomic_cap", &ibv_device_attr::atomic_cap)
      .def_ro("max_ee", &ibv_device_attr::max_ee)
      .def_ro("max_rdd", &ibv_device_attr::max_rdd)
      .def_ro("max_mw", &ibv_device_attr::max_mw)
      .def_ro("max_raw_ipv6_qp", &ibv_device_attr::max_raw_ipv6_qp)
      .def_ro("max_raw_ethy_qp", &ibv_device_attr::max_raw_ethy_qp)
      .def_ro("max_mcast_grp", &ibv_device_attr::max_mcast_grp)
      .def_ro("max_mcast_qp_attach", &ibv_device_attr::max_mcast_qp_attach)
      .def_ro(
          "max_total_mcast_qp_attach",
          &ibv_device_attr::max_total_mcast_qp_attach)
      .def_ro("max_ah", &ibv_device_attr::max_ah)
      .def_ro("max_fmr", &ibv_device_attr::max_fmr)
      .def_ro("max_map_per_fmr", &ibv_device_attr::max_map_per_fmr)
      .def_ro("max_srq", &ibv_device_attr::max_srq)
      .def_ro("max_srq_wr", &ibv_device_attr::max_srq_wr)
      .def_ro("max_srq_sge", &ibv_device_attr::max_srq_sge)
      .def_ro("max_pkeys", &ibv_device_attr::max_pkeys)
      .def_ro("local_ca_ack_delay", &ibv_device_attr::local_ca_ack_delay)
      .def_ro("phys_port_cnt", &ibv_device_attr::phys_port_cnt);

  nb::class_<ibv_port_attr>(m, "IbPortAttr")
      .def(
          "__dir__",
          [](const ibv_port_attr& attr) {
            return std::vector<std::string>{
                "state",          "max_mtu",
                "active_mtu",     "gid_tbl_len",
                "port_cap_flags", "max_msg_sz",
                "bad_pkey_cntr",  "qkey_viol_cntr",
                "pkey_tbl_len",   "lid",
                "sm_lid",         "lmc",
                "max_vl_num",     "sm_sl",
                "subnet_timeout", "init_type_reply",
                "active_width",   "active_speed",
                "phys_state",     "link_layer",
            };
          })
      .def_ro("state", &ibv_port_attr::state)
      .def_ro("max_mtu", &ibv_port_attr::max_mtu)
      .def_ro("active_mtu", &ibv_port_attr::active_mtu)
      .def_ro("gid_tbl_len", &ibv_port_attr::gid_tbl_len)
      .def_ro("port_cap_flags", &ibv_port_attr::port_cap_flags)
      .def_ro("max_msg_sz", &ibv_port_attr::max_msg_sz)
      .def_ro("bad_pkey_cntr", &ibv_port_attr::bad_pkey_cntr)
      .def_ro("qkey_viol_cntr", &ibv_port_attr::qkey_viol_cntr)
      .def_ro("pkey_tbl_len", &ibv_port_attr::pkey_tbl_len)
      .def_ro("lid", &ibv_port_attr::lid)
      .def_ro("sm_lid", &ibv_port_attr::sm_lid)
      .def_ro("lmc", &ibv_port_attr::lmc)
      .def_ro("max_vl_num", &ibv_port_attr::max_vl_num)
      .def_ro("sm_sl", &ibv_port_attr::sm_sl)
      .def_ro("subnet_timeout", &ibv_port_attr::subnet_timeout)
      .def_ro("init_type_reply", &ibv_port_attr::init_type_reply)
      .def_ro("active_width", &ibv_port_attr::active_width)
      .def_ro("active_speed", &ibv_port_attr::active_speed)
      .def_ro("phys_state", &ibv_port_attr::phys_state)
      .def_ro("link_layer", &ibv_port_attr::link_layer)
      .def(
          "expand_flags",
          [
          ](const ibv_port_attr& attr) -> std::vector<enum ibv_port_cap_flags> {
            std::vector<enum ibv_port_cap_flags> flagset;
            std::vector<int> fs = {
                IBV_PORT_SM,
                IBV_PORT_NOTICE_SUP,
                IBV_PORT_TRAP_SUP,
                IBV_PORT_OPT_IPD_SUP,
                IBV_PORT_AUTO_MIGR_SUP,
                IBV_PORT_SL_MAP_SUP,
                IBV_PORT_MKEY_NVRAM,
                IBV_PORT_PKEY_NVRAM,
                IBV_PORT_LED_INFO_SUP,
                IBV_PORT_SYS_IMAGE_GUID_SUP,
                IBV_PORT_PKEY_SW_EXT_PORT_TRAP_SUP,
                IBV_PORT_EXTENDED_SPEEDS_SUP,
                IBV_PORT_CM_SUP,
                IBV_PORT_SNMP_TUNNEL_SUP,
                IBV_PORT_REINIT_SUP,
                IBV_PORT_DEVICE_MGMT_SUP,
                IBV_PORT_VENDOR_CLASS_SUP,
                IBV_PORT_DR_NOTICE_SUP,
                IBV_PORT_CAP_MASK_NOTICE_SUP,
                IBV_PORT_BOOT_MGMT_SUP,
                IBV_PORT_LINK_LATENCY_SUP,
                IBV_PORT_CLIENT_REG_SUP,
                IBV_PORT_IP_BASED_GIDS,
            };
            for (int f : fs) {
              if (attr.port_cap_flags & f) {
                flagset.push_back((enum ibv_port_cap_flags)f);
              }
            }
            return flagset;
          })
      .def(
          "expand_flags2",
          [](const ibv_port_attr& attr)
              -> std::vector<enum ibv_port_cap_flags2> {
            std::vector<enum ibv_port_cap_flags2> flagset;
            std::vector<int> fs = {
                IBV_PORT_SET_NODE_DESC_SUP,
                IBV_PORT_INFO_EXT_SUP,
                IBV_PORT_VIRT_SUP,
                IBV_PORT_SWITCH_PORT_STATE_TABLE_SUP,
                IBV_PORT_LINK_WIDTH_2X_SUP,
                IBV_PORT_LINK_SPEED_HDR_SUP,
                IBV_PORT_LINK_SPEED_NDR_SUP,
            };
            for (int f : fs) {
              if (attr.port_cap_flags2 & f) {
                flagset.push_back((enum ibv_port_cap_flags2)f);
              }
            }
            return flagset;
          });
}
