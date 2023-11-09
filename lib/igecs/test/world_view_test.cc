#define IG_ENABLE_ECS_VALIDATION
#define IG_ECS_TEST_VALIDATIONS

#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>
#include <igecs/world_view.h>

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

TEST(IgECS_WorldView, UnrestrictedWorldViewAllowsAccesses) {
  entt::registry registry;
  auto& ctx = registry.ctx();

  ctx.emplace<FooT>(1);
  ctx.emplace<BarT>(2, 3);

  auto decl = WorldView::Decl::Thin();
  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.ctx<FooT>().a, 1);
  EXPECT_EQ(world_view.ctx<BarT>().b, 3);

  world_view.mut_ctx<BarT>().b = 5;
  EXPECT_EQ(world_view.ctx<BarT>().b, 5);

  entt::entity e = registry.create();
  world_view.attach<FooT>(e, 5);
  world_view.attach<BarT>(e, 10, 15);

  EXPECT_EQ(world_view.read<FooT>(e).a, 5);
  EXPECT_EQ(world_view.read<BarT>(e).a, 10);
  EXPECT_EQ(world_view.read<BarT>(e).b, 15);

  world_view.write<FooT>(e).a = 222;
  EXPECT_EQ(world_view.read<FooT>(e).a, 222);

  world_view.enqueue_event(FooT{5});
  world_view.enqueue_event(FooT{10});
  auto foo_evts = world_view.consume_events<FooT>();
  EXPECT_EQ(foo_evts.size(), 2);
  EXPECT_EQ(foo_evts[0].a, 5);
  EXPECT_EQ(foo_evts[1].a, 10);
}

TEST(IgECS_WorldView, CtxReadWriteSucceeds) {
  entt::registry registry;
  auto& ctx = registry.ctx();

  ctx.emplace<FooT>(1);

  WorldView::Decl decl;
  decl.ctx_writes<FooT>();

  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.ctx<FooT>().a, 1);

  world_view.mut_ctx<FooT>().a = 2;
  EXPECT_EQ(world_view.ctx<FooT>().a, 2);
}

TEST(IgECS_WorldView, ComponentReadWriteSucceeds) {
  entt::registry registry;

  entt::entity e = registry.create();
  registry.emplace<FooT>(e, 1);

  WorldView::Decl decl;
  decl.writes<FooT>();

  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.read<FooT>(e).a, 1);
  world_view.write<FooT>(e).a = 2;
  EXPECT_EQ(world_view.read<FooT>(e).a, 2);
}

TEST(IgECS_WorldView, IterateSucceedsAndTouchesEverything) {
  entt::registry registry;

  entt::entity e1 = registry.create();
  entt::entity e2 = registry.create();

  registry.emplace<FooT>(e1, 1);
  registry.emplace<FooT>(e2, 2);
  registry.emplace<BarT>(e1, 10, 20);
  registry.emplace<BarT>(e2, 30, 40);

  WorldView::Decl decl;
  decl.writes<FooT>().reads<BarT>();

  auto wv = decl.create(&registry);

  auto view = wv.view<FooT, const BarT>();
  bool has_1 = false, has_2 = false;
  int ct = 0;
  for (auto [e, f, b] : view.each()) {
    ct++;

    if (f.a == 1 && b.a == 10 && b.b == 20) {
      has_1 = true;
      continue;
    }

    if (f.a == 2 && b.a == 30 && b.b == 40) {
      has_2 = true;
      continue;
    }
  }

  EXPECT_EQ(ct, 2);
  EXPECT_TRUE(has_1);
  EXPECT_TRUE(has_2);
}

TEST(IgECS_WorldView, MergesInDecl) {
  {
    WorldView::Decl d;
    d.reads<FooT>();
    WorldView::Decl d2;
    d2.reads<BarT>();
    d.merge_in_decl(d2);
    EXPECT_TRUE(d.can_read<BarT>());
  }

  {
    WorldView::Decl d;
    d.writes<FooT>();
    WorldView::Decl d2;
    d2.writes<BarT>();
    d.merge_in_decl(d2);
    EXPECT_TRUE(d.can_write<BarT>());
  }

  {
    WorldView::Decl d;
    d.ctx_reads<FooT>();
    WorldView::Decl d2;
    d2.ctx_reads<BarT>();
    d.merge_in_decl(d2);
    EXPECT_TRUE(d.can_ctx_read<BarT>());
  }

  {
    WorldView::Decl d;
    d.ctx_writes<FooT>();
    WorldView::Decl d2;
    d2.ctx_writes<BarT>();
    d.merge_in_decl(d2);
    EXPECT_TRUE(d.can_ctx_write<BarT>());
  }

  {
    WorldView::Decl d, d2;
    d.evt_writes<FooT>();
    WorldView::Decl d3;
    d.evt_consumes<BarT>();
    d.merge_in_decl(d2).merge_in_decl(d3);
    EXPECT_TRUE(d.can_evt_write<FooT>());
    EXPECT_TRUE(d.can_evt_consume<BarT>());
    EXPECT_FALSE(d.can_evt_write<BarT>());
    EXPECT_FALSE(d.can_evt_consume<FooT>());
  }
}

TEST(IgECS_WorldView, RecordsDebugInfoForOtherTests) {
  entt::registry world;
  auto e = world.create();

  {
    WorldView wv = WorldView::Decl::Thin().create(&world);
    EXPECT_FALSE(wv.has_read<FooT>());
    EXPECT_FALSE(wv.has_written<FooT>());
    EXPECT_FALSE(wv.has_ctx_read<FooT>());
    EXPECT_FALSE(wv.has_ctx_written<FooT>());
  }

  {
    WorldView wv = WorldView::Decl::Thin().create(&world);

    wv.attach<FooT>(e, 42);

    EXPECT_FALSE(wv.has_read<FooT>());
    EXPECT_TRUE(wv.has_written<FooT>());
    EXPECT_FALSE(wv.has_ctx_read<FooT>());
    EXPECT_FALSE(wv.has_ctx_written<FooT>());
  }

  {
    WorldView wv = WorldView::Decl::Thin().create(&world);

    wv.attach_ctx<FooT>(10);

    EXPECT_FALSE(wv.has_read<FooT>());
    EXPECT_FALSE(wv.has_written<FooT>());
    EXPECT_FALSE(wv.has_ctx_read<FooT>());
    EXPECT_TRUE(wv.has_ctx_written<FooT>());
  }

  {
    WorldView wv = WorldView::Decl::Thin().create(&world);

    EXPECT_EQ(wv.read<FooT>(e).a, 42);
    EXPECT_TRUE(wv.has_read<FooT>());
    EXPECT_FALSE(wv.has_written<FooT>());
    EXPECT_FALSE(wv.has_ctx_read<FooT>());
    EXPECT_FALSE(wv.has_ctx_written<FooT>());
  }

  {
    WorldView wv = WorldView::Decl::Thin().create(&world);

    EXPECT_EQ(wv.ctx<FooT>().a, 10);
    EXPECT_FALSE(wv.has_read<FooT>());
    EXPECT_FALSE(wv.has_written<FooT>());
    EXPECT_TRUE(wv.has_ctx_read<FooT>());
    EXPECT_FALSE(wv.has_ctx_written<FooT>());
  }
}

#ifndef NDEBUG
TEST(IgECS_WorldViewDeathTest, BadCtxReadFails) {
  entt::registry registry;
  auto& ctx = registry.ctx();

  ctx.emplace<FooT>(1);

  WorldView::Decl decl;

  auto world_view = decl.create(&registry);

  EXPECT_DEATH({ EXPECT_EQ(world_view.ctx<FooT>().a, 0); },
               "ECS validation failure: method ctx failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadCtxWriteFails) {
  entt::registry registry;
  auto& ctx = registry.ctx();

  ctx.emplace<FooT>(1);

  WorldView::Decl decl;
  decl.ctx_reads<FooT>();

  auto world_view = decl.create(&registry);

  EXPECT_EQ(world_view.ctx<FooT>().a, 1);
  EXPECT_DEATH({ EXPECT_EQ(world_view.mut_ctx<FooT>().a, 1); },
               "ECS validation failure: method mut_ctx failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadEntityReadFails) {
  entt::registry registry;

  entt::entity e = registry.create();
  registry.emplace<FooT>(e, 1);

  WorldView::Decl decl;

  auto world_view = decl.create(&registry);

  EXPECT_DEATH({ EXPECT_EQ(world_view.read<FooT>(e).a, 1); },
               "ECS validation failure: method read failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadEntityWriteFails) {
  entt::registry registry;

  entt::entity e = registry.create();
  registry.emplace<FooT>(e, 1);

  WorldView::Decl decl;
  decl.reads<FooT>();

  auto wv = decl.create(&registry);

  EXPECT_EQ(wv.read<FooT>(e).a, 1);
  EXPECT_DEATH({ EXPECT_EQ(wv.write<FooT>(e).a, 1); },
               "ECS validation failure: method write failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadViewReadFails) {
  entt::registry registry;

  WorldView::Decl decl;
  decl.writes<BarT>();
  auto wv = decl.create(&registry);

  EXPECT_DEATH({ auto view = wv.view<FooT>(); },
               "MUTABLE view_test failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadViewWriteFails) {
  entt::registry registry;

  WorldView::Decl decl;
  decl.reads<FooT>().writes<BarT>();
  auto wv = decl.create(&registry);

  // No death
  auto v1 = wv.view<BarT, const FooT>();

  // For some reason just doing wv.view<FooT, BarT>() inside EXPECT_DEATH
  //  doesn't compile? It's weird. This works though.
  auto get = [&wv]() { return wv.view<BarT, FooT>(); };
  EXPECT_DEATH({ get(); }, "IMMUTABLE view_test failed for type .*FooT");
}

TEST(IgECS_WorldViewDeathTest, BadEventQueueFails) {
  entt::registry registry;

  WorldView::Decl decl;
  decl.evt_writes<FooT>().evt_consumes<BarT>();
  auto wv = decl.create(&registry);

  // No death
  wv.enqueue_event<FooT>(FooT{1});
  EXPECT_EQ(wv.consume_events<BarT>().size(), 0);

  // Death on writing the wrong event type
  EXPECT_DEATH(
      {
        wv.enqueue_event<BarT>(BarT{1, 2});
      },
      "ECS validation failure: method enqueue_event failed for type .*BarT");

  // Death on consuming the wrong event type
  EXPECT_DEATH(
      { wv.consume_events<FooT>(); },
      "ECS validation failure: method consume_events failed for type .*FooT");
}
#endif
