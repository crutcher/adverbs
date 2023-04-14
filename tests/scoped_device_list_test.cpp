#include <infiniband/verbs.h>

#include "adverbs.h"
#include "gtest/gtest.h"

TEST(scoped_device_list, iterators) {
  int ref_num_devices;
  struct ibv_device** ref_device_list = ibv_get_device_list(&ref_num_devices);

  auto ref_device_list_deleter = std::shared_ptr<struct ibv_device*[]>(
      ref_device_list,
      ibv_free_device_list);

  adverbs::scoped_device_list device_list;

  for (int i = 0; i < device_list.size(); ++i) {
    EXPECT_EQ(device_list[i].node_type, ref_device_list[i]->node_type);
  }

  int i = 0;
  for (auto it = device_list.begin(); it != device_list.end(); ++it) {
    EXPECT_EQ((*it)->node_type, ref_device_list[i]->node_type);
    ++i;
  }

  i = 0;
  for (auto dev : device_list) {
    EXPECT_EQ(dev->node_type, ref_device_list[i]->node_type);
    ++i;
  }
}

TEST(scoped_device_list, lookup_by_name) {
  adverbs::scoped_device_list orig;
  adverbs::scoped_device_list device_list = orig;

  EXPECT_EQ(nullptr, device_list.lookup_by_name("nonexistent"));

  for (auto& dev : device_list) {
    EXPECT_EQ(dev, device_list.lookup_by_name(dev->name));
    EXPECT_EQ(dev, device_list.lookup_by_name(std::string(dev->name)));
  }

  for (auto& dev : device_list) {
    adverbs::context_handle device_handle(dev);
  }
}

TEST(scoped_device_list, lookup_by_kernel_index) {
  adverbs::scoped_device_list device_list;

  EXPECT_EQ(nullptr, device_list.lookup_by_kernel_index(13370));

  for (auto& dev : device_list) {
    EXPECT_EQ(
        dev,
        device_list.lookup_by_kernel_index(ibv_get_device_index(dev)));
  }
}
