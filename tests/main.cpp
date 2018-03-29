#include <gtest/gtest.h>

std::string resources_path;

int main(int argc, char* argv[])
{
  resources_path = (argc > 1 ? argv[1] : "./tests/scripts/");
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
