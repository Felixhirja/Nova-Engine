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
    
    // SIMD-friendly alignment hint
    static constexpr size_t PreferredAlignment = alignof(T) >= 16 ? alignof(T) : 16;
    
    // Check if component is suitable for SIMD operations
    static constexpr bool IsSIMDFriendly = std::is_trivially_copyable_v<T> && 
                                           (sizeof(T) == 4 || sizeof(T) == 8 || 
                                            sizeof(T) == 16 || sizeof(T) == 32);
};

template<typename T>
inline constexpr bool IsTriviallyRelocatable =
    ComponentTraits<T>::CopyPolicy == ComponentCopyPolicy::Trivial;

template<typename T>
inline constexpr bool IsSIMDFriendly = ComponentTraits<T>::IsSIMDFriendly;

} // namespace ecs

