#include <unison/net/session_config.hpp>

#include <catch2/catch_test_macros.hpp>

#include <boost/pfr.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{

unison::net::SessionConfig sampleConfig()
{
    unison::net::SessionConfig config;
    config.tickRate = 60;
    config.slotCount = 4;
    config.inputSize = 8;
    config.maxPrediction = 10;
    config.seed = 20260923;
    config.assetHash = 0x1234'5678'9abc'def0U;
    config.pipelineHash = 0x0fed'cba9'8765'4321U;
    config.buildId = 42;

    return config;
}

unison::net::SessionConfig withFieldChanged(const unison::net::SessionConfig& config, std::size_t field)
{
    unison::net::SessionConfig changed = config;

    boost::pfr::for_each_field(changed,
                               [field](auto& value, std::size_t index)
                               {
                                   if (index == field)
                                   {
                                       value = static_cast<std::remove_reference_t<decltype(value)>>(value ^ 1U);
                                   }
                               });

    return changed;
}

}

TEST_CASE("a client runs up to twenty frames ahead of what it has verified unless the config says otherwise")
{
    REQUIRE(unison::net::SessionConfig{}.maxPrediction == 20U);
}

TEST_CASE("configs that agree on everything hash alike")
{
    const unison::net::SessionConfig one = sampleConfig();
    const unison::net::SessionConfig other = sampleConfig();

    REQUIRE(unison::net::hashOf(one) == unison::net::hashOf(other));
}

TEST_CASE("a change to any field of a config changes its hash")
{
    const unison::net::SessionConfig config = sampleConfig();
    const std::uint64_t hash = unison::net::hashOf(config);

    for (std::size_t field = 0; field < boost::pfr::tuple_size_v<unison::net::SessionConfig>; ++field)
    {
        CAPTURE(field);

        REQUIRE(unison::net::hashOf(withFieldChanged(config, field)) != hash);
    }
}
