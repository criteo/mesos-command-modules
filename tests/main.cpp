#include <gtest/gtest.h>

std::string g_resourcesPath;

int main(int argc, char* argv[])
{
  g_resourcesPath = (argc > 1 ? argv[1] : "./tests/scripts/");
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
