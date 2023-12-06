#include <gtest/gtest.h>
#include <igdemo/logic/locomotion.h>
#include <igecs/world_view.h>

TEST(EnttUsage, IgecsCreateAndDestroyEntity) {
  entt::registry r;
  auto wv = igecs::WorldView::Thin(&r);

  auto e = wv.create();
  EXPECT_TRUE(wv.valid(e));
  wv.destroy(e);
  EXPECT_FALSE(wv.valid(e));
}

TEST(EnttUsage, IgecsIteratorHitsCreatedEntities) {
  entt::registry r;

  entt::entity e1, e2;

  {
    auto wv = igecs::WorldView::Thin(&r);
    e1 = wv.create();
    e2 = wv.create();

    wv.attach<igdemo::PositionComponent>(e1,
                                         igdemo::PositionComponent{{1.f, 1.f}});
    wv.attach<igdemo::PositionComponent>(e2,
                                         igdemo::PositionComponent{{2.f, 2.f}});

    wv.attach<igdemo::OrientationComponent>(e1,
                                            igdemo::OrientationComponent{10.f});
    wv.attach<igdemo::OrientationComponent>(e2,
                                            igdemo::OrientationComponent{20.f});
  }

  {
    bool has_e1 = false, has_e2 = false;
    auto wv = igecs::WorldView::Thin(&r);

    auto view =
        wv.view<igdemo::PositionComponent, igdemo::OrientationComponent>();

    for (auto [e, p, o] : view.each()) {
      if (e == e1) {
        has_e1 = true;
        EXPECT_EQ(p.map_position.x, 1.f);
        EXPECT_EQ(p.map_position.y, 1.f);
        EXPECT_EQ(o.radAngle, 10.f);
      } else if (e == e2) {
        has_e2 = true;
        EXPECT_EQ(p.map_position.x, 2.f);
        EXPECT_EQ(p.map_position.y, 2.f);
        EXPECT_EQ(o.radAngle, 20.f);
      } else {
        FAIL("Unexpected entity found!");
      }
    }

    EXPECT_TRUE(has_e1);
    EXPECT_TRUE(has_e2);
  }
}

TEST(EnttUsage, IgecsIteratorDoesNotHitDestroyedEntities) {
  entt::registry r;

  entt::entity e1, e2;

  {
    auto wv = igecs::WorldView::Thin(&r);
    e1 = wv.create();
    e2 = wv.create();

    wv.attach<igdemo::PositionComponent>(e1,
                                         igdemo::PositionComponent{{1.f, 1.f}});
    wv.attach<igdemo::PositionComponent>(e2,
                                         igdemo::PositionComponent{{2.f, 2.f}});

    wv.attach<igdemo::OrientationComponent>(e1,
                                            igdemo::OrientationComponent{10.f});
    wv.attach<igdemo::OrientationComponent>(e2,
                                            igdemo::OrientationComponent{20.f});

    wv.destroy(e1);
  }

  {
    bool has_e2 = false;
    auto wv = igecs::WorldView::Thin(&r);

    auto view =
        wv.view<igdemo::PositionComponent, igdemo::OrientationComponent>();

    for (auto [e, p, o] : view.each()) {
      if (e == e1) {
        FAIL("Should not hit e1!");
      } else if (e == e2) {
        has_e2 = true;
        EXPECT_EQ(p.map_position.x, 2.f);
        EXPECT_EQ(p.map_position.y, 2.f);
        EXPECT_EQ(o.radAngle, 20.f);
      } else {
        FAIL("Unexpected entity found!");
      }
    }

    EXPECT_TRUE(has_e2);
  }
}
