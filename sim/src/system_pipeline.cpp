#include <unison/sim/system_pipeline.hpp>

#include <unison/core/contract.hpp>

namespace unison::sim
{

void SystemPipeline::add(ISystem& system)
{
    UNISON_VERIFY(!systems.isFull());

    systems.pushBack(&system);
}

void SystemPipeline::update(Frame& frame, const FrameInputs& inputs) const
{
    for (ISystem* system : systems)
    {
        system->update(frame, inputs);
    }
}

std::size_t SystemPipeline::size() const
{
    return systems.size();
}

const ISystem& SystemPipeline::at(std::size_t index) const
{
    UNISON_VERIFY(index < systems.size());

    return *systems[index];
}

}
