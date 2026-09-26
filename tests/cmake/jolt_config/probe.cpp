#include <unison_jolt_config.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>

int main()
{
    return JPH::VerifyJoltVersionID() ? 0 : 1;
}
