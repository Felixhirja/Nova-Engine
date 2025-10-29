#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace ecs {

enum class ComponentCopyPolicy {
    Trivial,
    CopyConstructor,
    Custom
};

template<typename T>
struct ComponentTraits {
    static constexpr ComponentCopyPolicy CopyPolicy =
        std::is_trivially_copyable_v<T> ? ComponentCopyPolicy::Trivial : ComponentCopyPolicy::CopyConstructor;

    static void CopyRange(T* dst, const T* src, size_t count) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (count > 0) {
                std::memcpy(dst, src, sizeof(T) * count);
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                dst[i] = src[i];
            }
        }
    }
};

template<typename T>
inline constexpr bool IsTriviallyRelocatable =
    ComponentTraits<T>::CopyPolicy == ComponentCopyPolicy::Trivial;

} // namespace ecs

