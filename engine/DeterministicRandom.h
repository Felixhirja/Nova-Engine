#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

class DeterministicRandom {
public:
    struct StreamState {
        uint64_t seed = 0u;
        uint64_t draws = 0u;
    };

    DeterministicRandom();

    void SetGlobalSeed(uint64_t seed);
    uint64_t GetGlobalSeed() const { return globalSeed_; }

    uint64_t NextUInt64();
    double NextDouble();
    int NextInt(int minInclusive, int maxInclusive);

    void PushContext(const std::string& name, uint64_t seedOffset = 0u);
    void PopContext();
    std::string CurrentContext() const;

    StreamState GetState() const;
    void RestoreState(const StreamState& state);

    void RegisterNamedStream(const std::string& name, uint64_t seed);
    StreamState GetNamedStreamState(const std::string& name) const;
    void RestoreNamedStream(const std::string& name, const StreamState& state);

private:
    struct Context {
        std::string name;
        uint64_t seed = 0u;
        uint64_t draws = 0u;
        std::mt19937_64 engine;
    };

    Context& ActiveContext();
    const Context& ActiveContext() const;

    uint64_t globalSeed_ = 0u;
    std::vector<Context> contextStack_;
    std::unordered_map<std::string, Context> namedStreams_;
};

class DeterministicRandomScope {
public:
    DeterministicRandomScope(DeterministicRandom& random, const std::string& name, uint64_t seedOffset = 0u);
    ~DeterministicRandomScope();

private:
    DeterministicRandom& random_;
};
