#pragma once

namespace unison::sim
{

/// Keeps Jolt's process-wide registration alive: the default allocator, the factory and the built-in
/// types are installed when the first scope opens and removed after the last one closes. Jolt keeps
/// this state in globals of its own and offers nothing to inject, so the open scopes are counted;
/// they are opened and closed on the one thread the simulation runs on.
class JoltRuntime
{
public:
    JoltRuntime();
    ~JoltRuntime();

    JoltRuntime(const JoltRuntime&) = delete;
    JoltRuntime& operator=(const JoltRuntime&) = delete;
    JoltRuntime(JoltRuntime&&) = delete;
    JoltRuntime& operator=(JoltRuntime&&) = delete;
};

}
