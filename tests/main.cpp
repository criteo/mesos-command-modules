#include <gtest/gtest.h>

std::string g_resourcesPath = "./tests/scripts/";

int main(int argc, char* argv[]) {
  if (const char* resources_path = std::getenv("TEST_RESOURCES_PATH"))
    g_resourcesPath = resources_path;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
