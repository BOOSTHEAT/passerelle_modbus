#include "test_ServerSingleClient.h"
#include "test_ServerMultipleClients.h"
#include "test_RequestProxy.h"

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
