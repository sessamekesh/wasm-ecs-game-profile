#include <gtest/gtest.h>
#include <igecs/ctti_type_id.h>

using namespace igecs;

namespace {
struct FooT {
  int a;
};
struct BarT {
  int a;
  int b;
};
}  // namespace

TEST(IgECS_CttiTypeId, DiscriminatesTypes) {
  auto foo_t = CttiTypeId::of<FooT>();
  auto bar_t = CttiTypeId::of<BarT>();

  auto foo_2 = CttiTypeId::of<FooT>();
  auto bar_2 = CttiTypeId::of<BarT>();

  EXPECT_NE(foo_t, bar_t);
  EXPECT_EQ(foo_t, foo_2);
  EXPECT_EQ(bar_t, bar_2);
}

TEST(IgECS_CttiTypeId, OutputsTypeNames) {
  EXPECT_TRUE(CttiTypeId::name<FooT>().find_last_of("FooT") > 0);
  EXPECT_STREQ(CttiTypeId::name<CttiTypeId>().c_str(), "igecs::CttiTypeId");
}
