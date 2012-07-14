#include "project.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(Project, NewProjectIsInitalized) {
    Project p;
    EXPECT_EQ(nullptr,p.callGraph);
    ASSERT_TRUE(p.pProcList.empty());
    ASSERT_TRUE(p.binary_path().empty());
    ASSERT_TRUE(p.project_name().empty());
    ASSERT_TRUE(p.symtab.empty());
}

TEST(Project, CreatedProjectHasValidNames) {
    Project p;
    std::vector<std::string> strs     = {"./Project1.EXE","/home/Project2.EXE","/home/Pro\\ ject3"};
    std::vector<std::string> expected = {"Project1","Project2","Pro\\ ject3"};
    for(size_t i=0; i<strs.size(); i++)
    {
        p.create(strs[i]);
        EXPECT_EQ(nullptr,p.callGraph);
        ASSERT_TRUE(p.pProcList.empty());
        EXPECT_EQ(expected[i],p.project_name());
        EXPECT_EQ(strs[i],p.binary_path());
        ASSERT_TRUE(p.symtab.empty());
    }
}
