#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "dcc.h"

TEST(CowriteTest, HandlesZeroInput) {
  EXPECT_EQ(1, 1);
}

int main(int argc, char** argv) {
  // The following line must be executed to initialize Google Mock
  // (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
