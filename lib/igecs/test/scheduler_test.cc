#define IG_ENABLE_ECS_VALIDATION
#define IG_ECS_TEST_VALIDATIONS

#include <gtest/gtest.h>
#include <igecs/scheduler.h>

namespace igecs {

namespace {
struct FooT {
  int a;
};
struct BarT {
  int b;
};

WorldView::Decl read_foo_decl() {
  WorldView::Decl d;
  d.reads<FooT>();
  return d;
}

WorldView::Decl write_foo_decl() {
  WorldView::Decl d;
  d.writes<FooT>();
  return d;
}

WorldView::Decl read_bar_decl() {
  WorldView::Decl d;
  d.reads<BarT>();
  return d;
}

WorldView::Decl write_bar_decl() {
  WorldView::Decl d;
  d.writes<BarT>();
  return d;
}

}  // namespace

TEST(IgECS_Scheduler, TrivialExecutes) {
  Scheduler::Builder sb;

  bool n1_ran = false;

  Scheduler::Node n =
      sb.add_node().with_decl(write_foo_decl()).build([&n1_ran](WorldView* wv) {
        EXPECT_TRUE(wv->can_write<FooT>());
        EXPECT_TRUE(wv->can_read<FooT>());
        EXPECT_FALSE(wv->can_read<BarT>());
        EXPECT_FALSE(wv->can_write<BarT>());
        EXPECT_FALSE(wv->can_ctx_read<FooT>());
        EXPECT_FALSE(wv->can_ctx_write<FooT>());

        n1_ran = true;

        return igasync::Promise<void>::Immediate();
      });

  sb.max_spin_time(std::chrono::seconds(2));
  Scheduler scheduler = sb.build();

  entt::registry w;

  scheduler.execute(nullptr, &w);

  EXPECT_TRUE(n1_ran);
}

TEST(IgECS_Scheduler, ExecutesWithDependencies) {
  Scheduler::Builder sb;

  bool n1_ran = false, n2_ran = false;

  Scheduler::Node n1 =
      sb.add_node().with_decl(write_foo_decl()).build([&n1_ran](WorldView* wv) {
        EXPECT_TRUE(wv->can_write<FooT>());
        n1_ran = true;

        auto view = wv->view<FooT>();
        int runs = 0;
        for (auto [e, foo] : view.each()) {
          foo.a = 100;
          runs++;
        }
        EXPECT_EQ(runs, 2);

        return igasync::Promise<void>::Immediate();
      });

  Scheduler::Node n2 = sb.add_node()
                           .with_decl(read_foo_decl())
                           .depends_on(n1)
                           .build([&n2_ran, &n1_ran](WorldView* wv) {
                             EXPECT_FALSE(wv->can_write<FooT>());
                             EXPECT_TRUE(wv->can_read<FooT>());
                             EXPECT_TRUE(n1_ran);

                             auto view = wv->view<const FooT>();
                             int runs = 0;
                             for (auto [e, foo] : view.each()) {
                               runs++;
                               EXPECT_EQ(foo.a, 100);
                             }
                             EXPECT_EQ(runs, 2);

                             n2_ran = true;
                             return igasync::Promise<void>::Immediate();
                           });

  Scheduler scheduler = sb.build();

  entt::registry world;
  auto e1 = world.create();
  auto e2 = world.create();

  world.emplace<FooT>(e1, 0);
  world.emplace<FooT>(e2, 0);

  scheduler.execute(nullptr, &world);

  EXPECT_TRUE(n1_ran);
  EXPECT_TRUE(n2_ran);
}

TEST(IgECS_Scheduler, SuccessfullyBuildsWithCorrectDepChaining) {
  Scheduler::Builder sb;

  auto write_node = sb.add_node().with_decl(write_foo_decl()).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });
  auto read_node_1 =
      sb.add_node()
          .with_decl(read_foo_decl())
          .with_decl(write_bar_decl())
          .depends_on(write_node)
          .build([](auto*) { return igasync::Promise<void>::Immediate(); });
  auto read_node_2 =
      sb.add_node()
          .with_decl(read_foo_decl())
          .with_decl(read_bar_decl())
          .depends_on(read_node_1)
          .build([](auto*) { return igasync::Promise<void>::Immediate(); });

  auto scheduler = sb.build();
  entt::registry r;
  scheduler.execute(nullptr, &r);
}

TEST(IgECS_Scheduler, SuccessfullyBuildsWithGoodEventOrdering) {
  Scheduler::Builder sb;

  struct EvtType {
    int payload;
  };
  WorldView::Decl write_decl = WorldView::Decl().evt_writes<EvtType>();
  WorldView::Decl consume_decl = WorldView::Decl().evt_consumes<EvtType>();

  auto producer_node_a = sb.add_node().with_decl(write_decl).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });
  auto producer_node_b = sb.add_node().with_decl(write_decl).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });

  auto consumer_node =
      sb.add_node()
          .with_decl(consume_decl)
          .depends_on(producer_node_a)
          .depends_on(producer_node_b)
          .build([](auto*) { return igasync::Promise<void>::Immediate(); });

  auto scheduler = sb.build();
  entt::registry r;
  scheduler.execute(nullptr, &r);
}

#ifndef NDEBUG
TEST(IgECS_SchedulerDeathTest, FailsToBuildWithUnclearDepOrdering) {
  Scheduler::Builder sb;

  auto n1 = sb.add_node().with_decl(write_foo_decl()).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });
  auto n2 = sb.add_node().with_decl(read_foo_decl()).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });

  EXPECT_DEATH({ auto scheduler = sb.build(); },
               "\\[IgECS::Scheduler\\] Strict dependency not found");
}

TEST(IgECS_SchedulerDeathTest, CannotBuildNodeTwice) {
  // This is an API mitigation against cycles - being able to build a node twice
  //  would result in the possibility of introducing cycles by being unable to
  //  distinguish between nodes, which would be a problem.
  Scheduler::Builder sb;

  auto nb = sb.add_node();

  auto n = nb.build([](auto*) { return igasync::Promise<void>::Immediate(); });

  EXPECT_DEATH(
      {
        auto n2 =
            nb.build([](auto*) { return igasync::Promise<void>::Immediate(); });
      },
      "\\[IgECS::Scheduler\\] Node is already built");
}

TEST(IgECS_SchedulerDeathTest, FailsWithUnclearEventOrdering) {
  Scheduler::Builder sb;

  struct EvtType {
    int payload;
  };
  WorldView::Decl write_decl = WorldView::Decl().evt_writes<EvtType>();
  WorldView::Decl consume_decl = WorldView::Decl().evt_consumes<EvtType>();

  auto producer_node_a = sb.add_node().with_decl(write_decl).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });
  auto producer_node_b = sb.add_node().with_decl(write_decl).build([](auto*) {
    return igasync::Promise<void>::Immediate();
  });

  auto consumer_node =
      sb.add_node()
          .with_decl(consume_decl)
          .depends_on(producer_node_a)
          // Leave commented out - this is the missing piece
          // .depends_on(producer_node_b)
          .build([](auto*) { return igasync::Promise<void>::Immediate(); });

  EXPECT_DEATH({ auto scheduler = sb.build(); },
               "\\[IgECS::Scheduler\\] Strict dependency not found");
}
#endif

}  // namespace igecs
