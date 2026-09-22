#pragma once

namespace unison::sim
{

/// The events systems raise while one tick runs. A frame owns one, so events live and die with the
/// tick they belong to and never leak into the next.
class EventBuffer
{
};

}
