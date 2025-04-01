// server/test/performance_tests.cpp
#include <gtest/gtest.h>
#include "DatabaseManager.h"
#include <chrono>
#include <cstdio>

class PerformanceTest : public ::testing::Test {
protected:
    std::string perfDB = "perf_test.db";
    void SetUp() override {
        std::remove(perfDB.c_str());
    }
    void TearDown() override {
        std::remove(perfDB.c_str());
    }
};

TEST_F(PerformanceTest, BulkMessageInsert) {
    DatabaseManager dbManager(perfDB);
    ASSERT_TRUE(dbManager.initDB());

    const int numMessages = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numMessages; i++) {
        bool result = dbManager.storeMessage("performance", "tester", "Performance test message", "2025-03-31T17:00:00Z");
        ASSERT_TRUE(result);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Time to insert " << numMessages << " messages: " << diff.count() << " seconds" << std::endl;

    double avgTime = diff.count() / numMessages;
    EXPECT_LT(avgTime, 0.005);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
