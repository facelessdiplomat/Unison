#include <unison/runner/client_outcome.hpp>

#include <algorithm>

namespace unison::runner
{

session::RollbackStats rollbacksOfAll(std::span<const ClientOutcome> clients)
{
    session::RollbackStats all;

    for (const ClientOutcome& client : clients)
    {
        all.rollbacks += client.rollbacks.rollbacks;
        all.deepestRollback = std::max(all.deepestRollback, client.rollbacks.deepestRollback);
        all.resimulatedFrames += client.rollbacks.resimulatedFrames;
        all.stalledTicks += client.rollbacks.stalledTicks;
        all.framesPlayed += client.rollbacks.framesPlayed;
    }

    return all;
}

}
