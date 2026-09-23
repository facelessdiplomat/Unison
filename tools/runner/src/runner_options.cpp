#include <unison/runner/runner_options.hpp>

#include <unison/net/protocol.hpp>

#include <cxxopts.hpp>

#include <string_view>

namespace unison::runner
{

namespace
{

constexpr float kWholeLossPercent = 100.0F;

cxxopts::Options describedOptions()
{
    cxxopts::Options options{"unison_runner",
                             "Plays a match between clients and a relay over a simulated network and checks that every "
                             "client verifies the same frames."};

    cxxopts::OptionAdder add = options.add_options();

    add("players", "clients in the match", cxxopts::value<std::uint32_t>()->default_value("2"));
    add("frames", "frames every client verifies", cxxopts::value<std::uint32_t>()->default_value("600"));
    add("seed", "seed of the network and the scripted inputs", cxxopts::value<std::uint64_t>()->default_value("1"));
    add("latency", "one-way delay in milliseconds", cxxopts::value<std::uint32_t>()->default_value("0"));
    add("jitter", "how far the delay strays either way, in ms", cxxopts::value<std::uint32_t>()->default_value("0"));
    add("loss", "share of unreliable messages lost, in per cent", cxxopts::value<float>()->default_value("0"));
    add("tick-rate", "ticks per second", cxxopts::value<std::uint16_t>()->default_value("60"));
    add("checksum-interval", "verified frames between checksums", cxxopts::value<std::uint32_t>()->default_value("1"));
    add("record", "file the run is recorded into", cxxopts::value<std::string>()->default_value(""));
    add("help", "lists these options");

    return options;
}

tl::expected<RunnerOptions, Error> refused(std::string_view why)
{
    return tl::unexpected{Error{ErrorCode::InvalidOption, why}};
}

}

tl::expected<RunnerOptions, Error> parseRunnerOptions(std::span<const char* const> arguments)
{
    cxxopts::Options options = describedOptions();
    const cxxopts::ParseResult parsed = options.parse(static_cast<int>(arguments.size()), arguments.data());

    RunnerOptions read;
    read.players = parsed["players"].as<std::uint32_t>();
    read.frames = parsed["frames"].as<std::uint32_t>();
    read.seed = parsed["seed"].as<std::uint64_t>();
    read.latencyMilliseconds = parsed["latency"].as<std::uint32_t>();
    read.jitterMilliseconds = parsed["jitter"].as<std::uint32_t>();
    const float lossPercent = parsed["loss"].as<float>();
    read.tickRate = parsed["tick-rate"].as<std::uint16_t>();
    read.checksumInterval = parsed["checksum-interval"].as<std::uint32_t>();
    read.recordPath = parsed["record"].as<std::string>();
    read.isHelpAsked = parsed.count("help") > 0;

    if (read.players == 0 || read.players > net::kMaxSlots)
    {
        return refused("--players takes one player at least and eight at most");
    }

    if (read.frames == 0)
    {
        return refused("--frames takes one frame at least");
    }

    if (!(lossPercent >= 0.0F && lossPercent <= kWholeLossPercent))
    {
        return refused("--loss takes a share from 0 to 100 per cent");
    }

    read.lossRate = lossPercent / kWholeLossPercent;

    if (read.tickRate == 0)
    {
        return refused("--tick-rate takes one tick a second at least");
    }

    if (read.checksumInterval == 0)
    {
        return refused("--checksum-interval takes one frame at least");
    }

    return read;
}

std::string runnerHelp()
{
    return describedOptions().help();
}

}
