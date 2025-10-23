#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <typeindex>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace ecs {
namespace debug {

inline std::string GetReadableTypeName(const std::type_index& typeIndex) {
#ifdef __GNUG__
    int status = 0;
    std::unique_ptr<char, void (*)(void*)> demangled(
        abi::__cxa_demangle(typeIndex.name(), nullptr, nullptr, &status),
        std::free);
    if (status == 0 && demangled) {
        return demangled.get();
    }
#endif
    return typeIndex.name();
}

} // namespace debug
} // namespace ecs

