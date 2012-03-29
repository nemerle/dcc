#include "project.h"
#include "loader.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(Loader, NewProjectIsInitalized) {
    Project p;
    EXPECT_EQ(nullptr,p.callGraph);
    ASSERT_TRUE(p.pProcList.empty());
    ASSERT_TRUE(p.binary_path().empty());
    ASSERT_TRUE(p.project_name().empty());
    ASSERT_TRUE(p.symtab.empty());
}

