#include <y/flow.h>
#include <gtest/gtest.h>

TEST(Tests, BasicObserve)
{
    y::Observable variable(1024);
    ASSERT_EQ(int(variable), 1024);

    int replica = 0;
    auto reactive = y::reactive([&replica](int value) { replica = value; }, variable).build();
    ASSERT_EQ(replica, 0);

    variable = 1;
    ASSERT_EQ(replica, 1);
}

TEST(Tests, MultiObserve)
{
    y::Observable variable1(1024);
    y::Observable<float> variable2(1.5);
    ASSERT_EQ(int(variable1), 1024);
    ASSERT_EQ(float(variable2), 1.5);

    int replica1 = 0;
    float replica2 = 0.0;
    auto reactive = y::reactive(
            [&replica1, &replica2](int value1, float value2) {
                replica1 = value1;
                replica2 = value2;
            },
            variable1, variable2).build();
    ASSERT_EQ(replica1, 0);
    ASSERT_EQ(replica2, 0.0);

    y::Data::flush();
    ASSERT_EQ(replica1, 1024);
    ASSERT_EQ(replica2, 1.5);

    variable1 = 1;
    ASSERT_EQ(replica1, 1);
    ASSERT_EQ(replica2, 1.5);

    variable2 = 2.0;
    ASSERT_EQ(replica1, 1);
    ASSERT_EQ(replica2, 2.0);
}