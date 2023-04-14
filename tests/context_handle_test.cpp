#include <infiniband/verbs.h>

#include "adverbs.h"
#include "gtest/gtest.h"

TEST(context_handle, attributes) {
  adverbs::scoped_device_list device_list;

  for (auto& dev : device_list) {
    adverbs::context_handle orig(dev);
    adverbs::context_handle handle = orig;

    auto attr = handle.query_device_attr();
    EXPECT_GT(attr.max_mr_size, 0);

    auto ports = handle.query_ports();

    auto ib_ports = handle.query_ports([](const auto& port) {
      return port.link_layer == IBV_LINK_LAYER_INFINIBAND;
    });
  }
}
