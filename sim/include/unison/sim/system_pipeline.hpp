#pragma once

#include <unison/core/fixed_vector.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>

#include <cstddef>
#include <string_view>

namespace unison::sim
{

/// One rule of the game, applied to a whole frame once per tick. A system keeps no state of its own:
/// everything it reads and writes lives in the frame, so a rollback takes its work back with it.
class ISystem
{
public:
    ISystem() = default;
    virtual ~ISystem() = default;

    ISystem(const ISystem&) = delete;
    ISystem& operator=(const ISystem&) = delete;
    ISystem(ISystem&&) = delete;
    ISystem& operator=(ISystem&&) = delete;

    virtual void update(Frame& frame, const FrameInputs& inputs) = 0;

    /// Names the system for the pipeline hash two clients compare before they agree to play.
    [[nodiscard]] virtual std::string_view name() const = 0;
};

/// The systems of a game in the order they run, which is the order a game states once and never
/// varies: both clients must apply the same rules in the same order or they part ways at once. The
/// pipeline does not own its systems; a game holds them for as long as the session runs.
class SystemPipeline
{
public:
    static constexpr std::size_t kMaxSystems = 64;

    void add(ISystem& system);

    void update(Frame& frame, const FrameInputs& inputs) const;

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] const ISystem& at(std::size_t index) const;

private:
    unison::FixedVector<ISystem*, kMaxSystems> systems{};
};

}
