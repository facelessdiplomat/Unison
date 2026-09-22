#include <catch2/catch_test_macros.hpp>

#include <Jolt/Jolt.h>

TEST_CASE("jolt is built for cross platform determinism")
{
#ifdef JPH_CROSS_PLATFORM_DETERMINISTIC
    SUCCEED();
#else
    FAIL("JPH_CROSS_PLATFORM_DETERMINISTIC is not defined");
#endif
}
