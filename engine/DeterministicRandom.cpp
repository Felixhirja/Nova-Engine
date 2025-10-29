#include "DeterministicRandom.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
uint64_t HashSeed(uint64_t base, uint64_t offset, const std::string& label) {
    std::hash<std::string> hasher;
    uint64_t value = base ^ (offset + 0x9E3779B97F4A7C15ull + (base << 6) + (base >> 2));
    value ^= hasher(label) + 0x9E3779B97F4A7C15ull + (value << 6) + (value >> 2);
    return value;
}
}

DeterministicRandom::DeterministicRandom() {
    SetGlobalSeed(0u);
}

void DeterministicRandom::SetGlobalSeed(uint64_t seed) {
    globalSeed_ = seed;
    contextStack_.clear();
    Context global;
    global.name = "global";
    global.seed = seed;
    global.draws = 0u;
    global.engine.seed(seed);
    contextStack_.push_back(global);
}

uint64_t DeterministicRandom::NextUInt64() {
    Context& ctx = ActiveContext();
    uint64_t value = ctx.engine();
    ++ctx.draws;
    return value;
}

double DeterministicRandom::NextDouble() {
    return static_cast<double>(NextUInt64()) / static_cast<double>(std::numeric_limits<uint64_t>::max());
}

int DeterministicRandom::NextInt(int minInclusive, int maxInclusive) {
    if (minInclusive > maxInclusive) {
        std::swap(minInclusive, maxInclusive);
    }
    std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return static_cast<int>(dist(ActiveContext().engine));
}

void DeterministicRandom::PushContext(const std::string& name, uint64_t seedOffset) {
    const Context& parent = ActiveContext();
    Context child;
    child.name = name;
    child.seed = HashSeed(parent.seed, seedOffset, name);
    child.draws = 0u;
    child.engine.seed(child.seed);
    contextStack_.push_back(child);
}

void DeterministicRandom::PopContext() {
    if (contextStack_.size() <= 1) {
        throw std::runtime_error("DeterministicRandom: cannot pop root context");
    }
    contextStack_.pop_back();
}

std::string DeterministicRandom::CurrentContext() const {
    return ActiveContext().name;
}

DeterministicRandom::StreamState DeterministicRandom::GetState() const {
    const Context& ctx = ActiveContext();
    return StreamState{ctx.seed, ctx.draws};
}

void DeterministicRandom::RestoreState(const StreamState& state) {
    Context& ctx = ActiveContext();
    ctx.seed = state.seed;
    ctx.draws = 0u;
    ctx.engine.seed(state.seed);
    for (uint64_t i = 0; i < state.draws; ++i) {
        (void)ctx.engine();
        ++ctx.draws;
    }
}

void DeterministicRandom::RegisterNamedStream(const std::string& name, uint64_t seed) {
    Context ctx;
    ctx.name = name;
    ctx.seed = seed;
    ctx.draws = 0u;
    ctx.engine.seed(seed);
    namedStreams_[name] = ctx;
}

DeterministicRandom::StreamState DeterministicRandom::GetNamedStreamState(const std::string& name) const {
    auto it = namedStreams_.find(name);
    if (it == namedStreams_.end()) {
        return StreamState{};
    }
    return StreamState{it->second.seed, it->second.draws};
}

void DeterministicRandom::RestoreNamedStream(const std::string& name, const StreamState& state) {
    auto it = namedStreams_.find(name);
    if (it == namedStreams_.end()) {
        RegisterNamedStream(name, state.seed);
        it = namedStreams_.find(name);
    }
    it->second.seed = state.seed;
    it->second.draws = 0u;
    it->second.engine.seed(state.seed);
    for (uint64_t i = 0; i < state.draws; ++i) {
        (void)it->second.engine();
        ++it->second.draws;
    }
}

DeterministicRandom::Context& DeterministicRandom::ActiveContext() {
    if (contextStack_.empty()) {
        SetGlobalSeed(globalSeed_);
    }
    return contextStack_.back();
}

const DeterministicRandom::Context& DeterministicRandom::ActiveContext() const {
    if (contextStack_.empty()) {
        throw std::runtime_error("DeterministicRandom: active context requested before initialization");
    }
    return contextStack_.back();
}

DeterministicRandomScope::DeterministicRandomScope(DeterministicRandom& random, const std::string& name, uint64_t seedOffset)
    : random_(random) {
    random_.PushContext(name, seedOffset);
}

DeterministicRandomScope::~DeterministicRandomScope() {
    random_.PopContext();
}
