#include <unison/session/session_config.hpp>

#include <catch2/catch_test_macros.hpp>

#include <boost/pfr.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{

unison::session::SessionConfig sampleConfig()
{
    unison::session::SessionConfig config;
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

unison::session::SessionConfig withFieldChanged(const unison::session::SessionConfig& config, std::size_t field)
{
    unison::session::SessionConfig changed = config;

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

TEST_CASE("configs that agree on everything hash alike")
{
    const unison::session::SessionConfig one = sampleConfig();
    const unison::session::SessionConfig other = sampleConfig();

    REQUIRE(unison::session::hashOf(one) == unison::session::hashOf(other));
}

TEST_CASE("a change to any field of a config changes its hash")
{
    const unison::session::SessionConfig config = sampleConfig();
    const std::uint64_t hash = unison::session::hashOf(config);

    for (std::size_t field = 0; field < boost::pfr::tuple_size_v<unison::session::SessionConfig>; ++field)
    {
        CAPTURE(field);

        REQUIRE(unison::session::hashOf(withFieldChanged(config, field)) != hash);
    }
}
