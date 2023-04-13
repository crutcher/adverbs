#include "gtest/gtest.h"
#include "adverbs.h"


TEST(BasicTestSuite, HellaBasic){
    EXPECT_EQ(adverbs::basic(3), 6);
}