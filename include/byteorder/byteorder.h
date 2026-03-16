#pragma once

#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <array>
#include <utility>
#include <version>

// Platform-specific SIMD intrinsics and CPU feature detection
// are provided by simd_feature_check (see cmake/Dependencies.cmake).
#include <simd/feature_check.hpp>

#if defined(_MSC_VER)
#  define ENDIAN_ALWAYS_INLINE  __forceinline
#  define ENDIAN_LIKELY(x)      (x)
#  define ENDIAN_UNLIKELY(x)    (x)
#  define ENDIAN_RESTRICT       __restrict
#  define ENDIAN_ALIGNED(x)     __declspec(align(x))
#  define ENDIAN_VECTORCALL     __vectorcall
#  define ENDIAN_EXPORT         __declspec(dllexport)
#  define ENDIAN_IMPORT         __declspec(dllimport)
#  define ENDIAN_NOINLINE       __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#  define ENDIAN_ALWAYS_INLINE  inline __attribute__((always_inline))
#  define ENDIAN_LIKELY(x)      __builtin_expect(!!(x), 1)
#  define ENDIAN_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#  define ENDIAN_RESTRICT       __restrict__
#  define ENDIAN_ALIGNED(x)     __attribute__((aligned(x)))
#  define ENDIAN_VECTORCALL
#  define ENDIAN_EXPORT         __attribute__((visibility("default")))
#  define ENDIAN_IMPORT
#  define ENDIAN_NOINLINE       __attribute__((noinline))
#else
#  define ENDIAN_ALWAYS_INLINE  inline
#  define ENDIAN_LIKELY(x)      (x)
#  define ENDIAN_UNLIKELY(x)    (x)
#  define ENDIAN_RESTRICT
#  define ENDIAN_ALIGNED(x)
#  define ENDIAN_VECTORCALL
#  define ENDIAN_EXPORT
#  define ENDIAN_IMPORT
#  define ENDIAN_NOINLINE
#endif

#if defined(ENDIAN_BUILD_SHARED)
#  define ENDIAN_API ENDIAN_EXPORT
#elif defined(ENDIAN_USE_SHARED)
#  define ENDIAN_API ENDIAN_IMPORT
#else
#  define ENDIAN_API
#endif

#if defined(__cpp_lib_hardware_interference_size) && (__cpp_lib_hardware_interference_size != 201703L)
inline constexpr std::size_t hardware_destructive_interference_size = std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace endian
{

struct version
{
    static constexpr uint32_t       major = 1;
    static constexpr uint32_t       minor = 0;
    static constexpr uint32_t       patch = 0;
    static constexpr std::string_view str = "1.0.0";
};

class endian_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class alignment_error : public endian_error
{
public:
    explicit alignment_error(const char* msg) : endian_error(msg) {}
};

class invalid_byte_order_error : public endian_error
{
public:
    explicit invalid_byte_order_error(const char* msg) : endian_error(msg) {}
};

namespace detail
{


template <typename T>
concept ByteSwappable =
    std::is_trivially_copyable_v<T> &&
    ((sizeof(T) == 1) || (sizeof(T) == 2) || (sizeof(T) == 4) || (sizeof(T) == 8) || (sizeof(T) == 16)) &&
    !std::is_same_v<T, bool> &&
    !std::is_same_v<T, char>;

template <typename T>
concept IntegralByteSwappable =
    ByteSwappable<T> && std::is_integral_v<T>;

template <typename T>
concept FloatingPointByteSwappable =
    ByteSwappable<T> && std::is_floating_point_v<T>;

template <typename T>
struct simd_traits
{
    static constexpr size_t vector_size = 0;
    static constexpr size_t alignment   = alignof(T);
};

// uint16_t: 16 x u16 in 256-bit (AVX2)
template <>
struct simd_traits<uint16_t>
{
    static constexpr size_t vector_size = 16;
    static constexpr size_t alignment   = 32;
};

// uint32_t / uint64_t: sizes differ between AVX-512 and AVX2
#if defined(__AVX512F__)
template <>
struct simd_traits<uint32_t>
{
    static constexpr size_t vector_size = 16;  // 16 x u32 in 512-bit
    static constexpr size_t alignment   = 64;
};

template <>
struct simd_traits<uint64_t>
{
    static constexpr size_t vector_size = 8;   // 8 x u64 in 512-bit
    static constexpr size_t alignment   = 64;
};
#else
template <>
struct simd_traits<uint32_t>
{
    static constexpr size_t vector_size = 8;   // 8 x u32 in 256-bit (AVX2)
    static constexpr size_t alignment   = 32;
};

template <>
struct simd_traits<uint64_t>
{
    static constexpr size_t vector_size = 4;   // 4 x u64 in 256-bit (AVX2)
    static constexpr size_t alignment   = 32;
};
#endif

template <IntegralByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE consteval T byte_swap_consteval(T v) noexcept
{
    if constexpr (sizeof(T) == 1)
    {
        return v;
    }
    else if constexpr (sizeof(T) == 2)
    {
        return static_cast<T>(
            (static_cast<std::make_unsigned_t<T>>(v) & 0xFF00u) >> 8 |
            (static_cast<std::make_unsigned_t<T>>(v) & 0x00FFu) << 8);
    }
    else if constexpr (sizeof(T) == 4)
    {
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(
            (U(v) & 0xFF000000u) >> 24 |
            (U(v) & 0x00FF0000u) >>  8 |
            (U(v) & 0x0000FF00u) <<  8 |
            (U(v) & 0x000000FFu) << 24);
    }
    else if constexpr (sizeof(T) == 8)
    {
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(
            (U(v) & 0xFF00000000000000ull) >> 56 |
            (U(v) & 0x00FF000000000000ull) >> 40 |
            (U(v) & 0x0000FF0000000000ull) >> 24 |
            (U(v) & 0x000000FF00000000ull) >>  8 |
            (U(v) & 0x00000000FF000000ull) <<  8 |
            (U(v) & 0x0000000000FF0000ull) << 24 |
            (U(v) & 0x000000000000FF00ull) << 40 |
            (U(v) & 0x00000000000000FFull) << 56);
    }
    else
    {
        return v;
    }
}

template <IntegralByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T byte_swap(T v) noexcept
{
    if constexpr (sizeof(T) == 1)
    {
        return v;
    }
    else if constexpr (sizeof(T) == 2)
    {
#if defined(_MSC_VER)
        if (!std::is_constant_evaluated())
            return static_cast<T>(_byteswap_ushort(static_cast<uint16_t>(v)));
#endif
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(
            (U(v) & 0xFF00u) >> 8 |
            (U(v) & 0x00FFu) << 8);
    }
    else if constexpr (sizeof(T) == 4)
    {
#if defined(_MSC_VER)
        if (!std::is_constant_evaluated())
            return static_cast<T>(_byteswap_ulong(static_cast<uint32_t>(v)));
#elif defined(__GNUC__) || defined(__clang__)
        if (!std::is_constant_evaluated())
            return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(v)));
#endif
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(
            (U(v) & 0xFF000000u) >> 24 |
            (U(v) & 0x00FF0000u) >>  8 |
            (U(v) & 0x0000FF00u) <<  8 |
            (U(v) & 0x000000FFu) << 24);
    }
    else if constexpr (sizeof(T) == 8)
    {
#if defined(_MSC_VER)
        if (!std::is_constant_evaluated())
            return static_cast<T>(_byteswap_uint64(static_cast<uint64_t>(v)));
#elif defined(__GNUC__) || defined(__clang__)
        if (!std::is_constant_evaluated())
            return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(v)));
#endif
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(
            (U(v) & 0xFF00000000000000ull) >> 56 |
            (U(v) & 0x00FF000000000000ull) >> 40 |
            (U(v) & 0x0000FF0000000000ull) >> 24 |
            (U(v) & 0x000000FF00000000ull) >>  8 |
            (U(v) & 0x00000000FF000000ull) <<  8 |
            (U(v) & 0x0000000000FF0000ull) << 24 |
            (U(v) & 0x000000000000FF00ull) << 40 |
            (U(v) & 0x00000000000000FFull) << 56);
    }
    else
    {
        return v;
    }
}

template <FloatingPointByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T byte_swap(T v) noexcept
{
    if constexpr (sizeof(T) == 4)
    {
        uint32_t bits;
        std::memcpy(&bits, &v, sizeof(float));
        bits = byte_swap(bits);
        T result;
        std::memcpy(&result, &bits, sizeof(float));
        return result;
    }
    else if constexpr (sizeof(T) == 8)
    {
        uint64_t bits;
        std::memcpy(&bits, &v, sizeof(double));
        bits = byte_swap(bits);
        T result;
        std::memcpy(&result, &bits, sizeof(double));
        return result;
    }
    else
    {
        return v;
    }
}

template <IntegralByteSwappable T>
ENDIAN_ALWAYS_INLINE void byte_swap_scalar_fast(const T* ENDIAN_RESTRICT src,
                                                 T* ENDIAN_RESTRICT dst,
                                                 size_t count) noexcept
{
#if defined(__AVX512F__)
    if (!std::is_constant_evaluated() && simd::detail::CPUInfo::has_avx512f() && sizeof(T) == 4)
    {
        const __m512i shuf = _mm512_set_epi8(
            28, 29, 30, 31, 24, 25, 26, 27, 20, 21, 22, 23, 16, 17, 18, 19,
            12, 13, 14, 15,  8,  9, 10, 11,  4,  5,  6,  7,  0,  1,  2,  3,
            60, 61, 62, 63, 56, 57, 58, 59, 52, 53, 54, 55, 48, 49, 50, 51,
            44, 45, 46, 47, 40, 41, 42, 43, 36, 37, 38, 39, 32, 33, 34, 35);
        
        size_t i = 0;
        for (; i + 15 < count; i += 16)
        {
            __m512i v = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(src + i));
            v = _mm512_shuffle_epi8(v, shuf);
            _mm512_storeu_si512(reinterpret_cast<__m512i*>(dst + i), v);
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

#if defined(__x86_64__) || defined(_M_X64)
    if (!std::is_constant_evaluated() && simd::detail::CPUInfo::has_avx2() && sizeof(T) == 4)
    {
        const __m256i shuf = _mm256_set_epi8(
            28, 29, 30, 31, 24, 25, 26, 27,
            20, 21, 22, 23, 16, 17, 18, 19,
            12, 13, 14, 15,  8,  9, 10, 11,
             4,  5,  6,  7,  0,  1,  2,  3);
        
        size_t i = 0;
        for (; i + 7 < count; i += 8)
        {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i),
                                _mm256_shuffle_epi8(v, shuf));
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    if (!std::is_constant_evaluated() && sizeof(T) == 4)
    {
        size_t i = 0;
        for (; i + 3 < count; i += 4)
        {
            uint32x4_t data = vld1q_u32(reinterpret_cast<const uint32_t*>(src + i));
            vst1q_u32(reinterpret_cast<uint32_t*>(dst + i),
                      vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(data))));
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

    for (size_t i = 0; i < count; ++i)
        dst[i] = byte_swap(src[i]);
}

template <IntegralByteSwappable T>
ENDIAN_ALWAYS_INLINE void byte_swap_scalar_fast_16(const T* ENDIAN_RESTRICT src,
                                                    T* ENDIAN_RESTRICT dst,
                                                    size_t count) noexcept
{
#if defined(__x86_64__) || defined(_M_X64)
    if (!std::is_constant_evaluated() && simd::detail::CPUInfo::has_avx2())
    {
        size_t i = 0;
        for (; i + 15 < count; i += 16)
        {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
            v = _mm256_shufflehi_epi16(v, _MM_SHUFFLE(2, 3, 0, 1));
            v = _mm256_shufflelo_epi16(v, _MM_SHUFFLE(2, 3, 0, 1));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), v);
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    if (!std::is_constant_evaluated())
    {
        size_t i = 0;
        for (; i + 7 < count; i += 8)
        {
            uint16x8_t data = vld1q_u16(reinterpret_cast<const uint16_t*>(src + i));
            vst1q_u16(reinterpret_cast<uint16_t*>(dst + i),
                      vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(data))));
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

    for (size_t i = 0; i < count; ++i)
        dst[i] = byte_swap(src[i]);
}

template <IntegralByteSwappable T>
ENDIAN_ALWAYS_INLINE void byte_swap_scalar_fast_64(const T* ENDIAN_RESTRICT src,
                                                    T* ENDIAN_RESTRICT dst,
                                                    size_t count) noexcept
{
#if defined(__AVX512F__)
    if (!std::is_constant_evaluated() && simd::detail::CPUInfo::has_avx512f())
    {
        const __m512i shuf = _mm512_set_epi8(
            56, 57, 58, 59, 60, 61, 62, 63,
            48, 49, 50, 51, 52, 53, 54, 55,
            40, 41, 42, 43, 44, 45, 46, 47,
            32, 33, 34, 35, 36, 37, 38, 39,
            24, 25, 26, 27, 28, 29, 30, 31,
            16, 17, 18, 19, 20, 21, 22, 23,
             8,  9, 10, 11, 12, 13, 14, 15,
             0,  1,  2,  3,  4,  5,  6,  7);
        
        size_t i = 0;
        for (; i + 7 < count; i += 8)
        {
            __m512i v = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(src + i));
            v = _mm512_shuffle_epi8(v, shuf);
            _mm512_storeu_si512(reinterpret_cast<__m512i*>(dst + i), v);
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

#if defined(__x86_64__) || defined(_M_X64)
    if (!std::is_constant_evaluated() && simd::detail::CPUInfo::has_avx2())
    {
        const __m256i shuf = _mm256_set_epi8(
            24, 25, 26, 27, 28, 29, 30, 31,
            16, 17, 18, 19, 20, 21, 22, 23,
             8,  9, 10, 11, 12, 13, 14, 15,
             0,  1,  2,  3,  4,  5,  6,  7);
        
        size_t i = 0;
        for (; i + 3 < count; i += 4)
        {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
            v = _mm256_shuffle_epi8(v, shuf);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), v);
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    if (!std::is_constant_evaluated())
    {
        size_t i = 0;
        for (; i + 1 < count; i += 2)
        {
            uint64x2_t data = vld1q_u64(reinterpret_cast<const uint64_t*>(src + i));
            vst1q_u64(reinterpret_cast<uint64_t*>(dst + i),
                      vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(data))));
        }
        for (; i < count; ++i)
            dst[i] = byte_swap(src[i]);
        return;
    }
#endif

    for (size_t i = 0; i < count; ++i)
        dst[i] = byte_swap(src[i]);
}

template <IntegralByteSwappable T>
ENDIAN_ALWAYS_INLINE void batch_byte_swap_aligned(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count) noexcept
{
    if (ENDIAN_UNLIKELY(count == 0))
        return;

    if constexpr (sizeof(T) == 2)
        byte_swap_scalar_fast_16(src, dst, count);
    else if constexpr (sizeof(T) == 4)
        byte_swap_scalar_fast(src, dst, count);
    else if constexpr (sizeof(T) == 8)
        byte_swap_scalar_fast_64(src, dst, count);
    else
        for (size_t i = 0; i < count; ++i)
            dst[i] = byte_swap(src[i]);
}

template <IntegralByteSwappable T>
ENDIAN_ALWAYS_INLINE void batch_byte_swap_unaligned(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count) noexcept
{
    batch_byte_swap_aligned(src, dst, count);
}

template <IntegralByteSwappable T>
void batch_byte_swap(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count,
    bool check_alignment = true)
{
    if (ENDIAN_UNLIKELY(count == 0))
        return;

    if (check_alignment)
    {
        if (reinterpret_cast<std::uintptr_t>(src) % alignof(T) != 0 ||
            reinterpret_cast<std::uintptr_t>(dst) % alignof(T) != 0)
        {
            throw alignment_error("Source or destination pointer is not properly aligned");
        }
        batch_byte_swap_aligned(src, dst, count);
    }
    else
    {
        batch_byte_swap_unaligned(src, dst, count);
    }
}

template <IntegralByteSwappable T>
ENDIAN_ALWAYS_INLINE void batch_byte_swap_inplace(
    T* ENDIAN_RESTRICT data,
    size_t count) noexcept
{
    if (ENDIAN_UNLIKELY(count == 0))
        return;

    if constexpr (sizeof(T) == 1)
    {
        return;
    }
    else
    {
        for (size_t i = 0; i < count; ++i)
            data[i] = byte_swap(data[i]);
    }
}

template <typename T, size_t CacheLineSize = 64>
struct ENDIAN_ALIGNED(CacheLineSize) cache_aligned_storage
{
    static_assert(sizeof(T) <= CacheLineSize,
                  "T must not exceed CacheLineSize bytes");

    T          value;
    std::byte  padding[CacheLineSize - sizeof(T)];

    static constexpr size_t alignment = CacheLineSize;

    constexpr cache_aligned_storage() noexcept : value{}, padding{} {}
    constexpr explicit cache_aligned_storage(T v) noexcept : value(v), padding{} {}
};

template <typename T>
struct ENDIAN_ALIGNED(hardware_destructive_interference_size) cache_line_padded
{
    T value;

    constexpr cache_line_padded() noexcept : value{} {}
    constexpr explicit cache_line_padded(T v) noexcept : value(v) {}

    T* operator&() noexcept { return &value; }
    const T* operator&() const noexcept { return &value; }
    T& operator*() noexcept { return value; }
    const T& operator*() const noexcept { return value; }
};

template <typename T>
class atomic_padded
{
    alignas(hardware_destructive_interference_size) std::atomic<T> value_{};

public:
    atomic_padded() = default;
    explicit atomic_padded(T v) noexcept : value_(v) {}

    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept
    {
        return value_.load(order);
    }

    void store(T v, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        value_.store(v, order);
    }

    T exchange(T v, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return value_.exchange(v, order);
    }

    bool compare_exchange_weak(T& expected, T desired,
                               std::memory_order success = std::memory_order_seq_cst,
                               std::memory_order failure = std::memory_order_seq_cst) noexcept
    {
        return value_.compare_exchange_weak(expected, desired, success, failure);
    }

    bool compare_exchange_strong(T& expected, T desired,
                                 std::memory_order success = std::memory_order_seq_cst,
                                 std::memory_order failure = std::memory_order_seq_cst) noexcept
    {
        return value_.compare_exchange_strong(expected, desired, success, failure);
    }
};

template <typename T>
constexpr bool is_power_of_two(T n) noexcept
{
    return n > 0 && (n & (n - 1)) == 0;
}

template <typename T>
[[nodiscard]] constexpr T align_up(T ptr, size_t alignment) noexcept
{
    static_assert(std::is_pointer_v<T> || std::is_integral_v<T>);
    if constexpr (std::is_pointer_v<T>)
    {
        using U = std::uintptr_t;
        return reinterpret_cast<T>((reinterpret_cast<U>(ptr) + alignment - 1) & ~(alignment - 1));
    }
    else
    {
        return (ptr + alignment - 1) & ~(alignment - 1);
    }
}

template <typename T>
[[nodiscard]] constexpr bool is_aligned(T ptr, size_t alignment) noexcept
{
    if constexpr (std::is_pointer_v<T>)
    {
        return (reinterpret_cast<std::uintptr_t>(ptr) & (alignment - 1)) == 0;
    }
    else
    {
        return (ptr & (alignment - 1)) == 0;
    }
}

ENDIAN_ALWAYS_INLINE void prefetch_read(const void* addr) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 0, 3);
#elif defined(_MSC_VER)
    _mm_prefetch(const_cast<char*>(static_cast<const char*>(addr)), _MM_HINT_T0);
#else
    (void)addr;
#endif
}

ENDIAN_ALWAYS_INLINE void prefetch_write(void* addr) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 1, 3);
#elif defined(_MSC_VER)
    _mm_prefetch(static_cast<char*>(addr), _MM_HINT_T0);
#else
    (void)addr;
#endif
}

template <IntegralByteSwappable T>
void batch_byte_swap_prefetch(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count,
    size_t prefetch_distance = 8) noexcept
{
    if (ENDIAN_UNLIKELY(count == 0))
        return;

    const size_t prefetch_limit = (count > prefetch_distance) ? (count - prefetch_distance) : 0;

    for (size_t i = 0; i < prefetch_limit; ++i)
    {
        prefetch_read(src + i + prefetch_distance);
        prefetch_write(dst + i + prefetch_distance);
        dst[i] = byte_swap(src[i]);
    }

    for (size_t i = prefetch_limit; i < count; ++i)
        dst[i] = byte_swap(src[i]);
}

template <IntegralByteSwappable T>
void batch_byte_swap_nt(
    T* ENDIAN_RESTRICT dst,
    const T* ENDIAN_RESTRICT src,
    size_t count) noexcept
{
    if (ENDIAN_UNLIKELY(count == 0))
        return;

#if defined(__x86_64__) || defined(_M_X64)
    if (!std::is_constant_evaluated() && simd::detail::CPUInfo::has_avx2())
    {
        if constexpr (sizeof(T) == 4)
        {
            const __m256i shuf = _mm256_set_epi8(
                28, 29, 30, 31, 24, 25, 26, 27,
                20, 21, 22, 23, 16, 17, 18, 19,
                12, 13, 14, 15,  8,  9, 10, 11,
                 4,  5,  6,  7,  0,  1,  2,  3);

            size_t i = 0;
            for (; i + 7 < count; i += 8)
            {
                __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
                v = _mm256_shuffle_epi8(v, shuf);
                _mm256_stream_si256(reinterpret_cast<__m256i*>(dst + i), v);
            }
            for (; i < count; ++i)
                dst[i] = byte_swap(src[i]);
            return;
        }
    }
#endif

    batch_byte_swap(src, dst, count, false);
}

} // namespace detail

enum class byte_order : uint8_t
{
    little,
    big,
    native = std::endian::native == std::endian::little ? little : big,
    network = big
};

template <typename T>
struct is_endian_compatible : std::bool_constant<detail::ByteSwappable<T>> {};

template <typename T>
inline constexpr bool is_endian_compatible_v = is_endian_compatible<T>::value;

template <detail::ByteSwappable T, size_t Alignment = 64>
class ENDIAN_API basic_endian
{
    using storage_type = detail::cache_aligned_storage<T, Alignment>;
    storage_type storage_;

public:
    using value_type = T;
    static constexpr size_t size      = sizeof(T);
    static constexpr size_t alignment = Alignment;

    constexpr basic_endian() noexcept = default;
    constexpr explicit basic_endian(T v) noexcept : storage_(v) {}

    [[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T native() const noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            return storage_.value;
        else
            return detail::byte_swap(storage_.value);
    }

    [[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T little() const noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            return storage_.value;
        else
            return detail::byte_swap(storage_.value);
    }

    [[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T big() const noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            return detail::byte_swap(storage_.value);
        else
            return storage_.value;
    }

    [[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T order(byte_order o) const noexcept
    {
        if (o == byte_order::big || o == byte_order::network)
            return big();
        if (o == byte_order::little)
            return little();
        return native();
    }

    ENDIAN_ALWAYS_INLINE constexpr void store_native(T v) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            storage_.value = v;
        else
            storage_.value = detail::byte_swap(v);
    }

    ENDIAN_ALWAYS_INLINE constexpr void store_little(T v) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            storage_.value = v;
        else
            storage_.value = detail::byte_swap(v);
    }

    ENDIAN_ALWAYS_INLINE constexpr void store_big(T v) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            storage_.value = detail::byte_swap(v);
        else
            storage_.value = v;
    }

    ENDIAN_ALWAYS_INLINE constexpr void store_order(byte_order o, T v) noexcept
    {
        if (o == byte_order::big || o == byte_order::network)
            store_big(v);
        else if (o == byte_order::little)
            store_little(v);
        else
            store_native(v);
    }

    template <typename Container>
        requires std::ranges::contiguous_range<Container> &&
                 std::same_as<std::ranges::range_value_t<Container>, T>
    static ENDIAN_ALWAYS_INLINE void convert_batch(
        const Container& src,
        Container&       dst,
        bool check_alignment = true)
    {
        detail::batch_byte_swap(
            std::ranges::data(src), std::ranges::data(dst),
            std::ranges::size(src), check_alignment);
    }

    template <typename Container>
        requires std::ranges::contiguous_range<Container> &&
                 std::same_as<std::ranges::range_value_t<Container>, T>
    static ENDIAN_ALWAYS_INLINE void convert_batch_inplace(
        Container& data,
        bool check_alignment = true)
    {
        if (check_alignment)
        {
            if (reinterpret_cast<std::uintptr_t>(std::ranges::data(data)) % alignof(T) != 0)
                throw alignment_error("Container data is not properly aligned");
        }
        detail::batch_byte_swap_inplace(std::ranges::data(data), std::ranges::size(data));
    }

    template <typename Ptr>
        requires std::is_pointer_v<Ptr> &&
                 std::same_as<std::remove_cvref_t<std::remove_pointer_t<Ptr>>, T>
    static ENDIAN_ALWAYS_INLINE void write_mmio(
        Ptr dest, T value,
        std::memory_order order = std::memory_order_release) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            std::atomic_ref<T>(*dest).store(detail::byte_swap(value), order);
        else
            std::atomic_ref<T>(*dest).store(value, order);
    }

    template <typename Ptr>
        requires std::is_pointer_v<Ptr> &&
                 std::same_as<std::remove_cvref_t<std::remove_pointer_t<Ptr>>, T>
    [[nodiscard]] static ENDIAN_ALWAYS_INLINE T read_mmio(
        Ptr src,
        std::memory_order order = std::memory_order_acquire) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
            return detail::byte_swap(std::atomic_ref<T>(*src).load(order));
        else
            return std::atomic_ref<T>(*src).load(order);
    }

    class buffer_view
    {
        std::span<std::byte> data_;

    public:
        constexpr explicit buffer_view(std::span<std::byte> data)
            : data_(data)
        {
            if (ENDIAN_UNLIKELY(data.size() < sizeof(T)))
                throw std::out_of_range("Buffer size is too small for type T");
        }

        template <size_t N>
        constexpr explicit buffer_view(std::byte (&arr)[N])
            : data_(arr)
        {
            static_assert(N >= sizeof(T), "Array size must be at least sizeof(T)");
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_native() const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            return result;
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_little() const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            if constexpr (std::endian::native != std::endian::little)
                result = detail::byte_swap(result);
            return result;
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_big() const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            if constexpr (std::endian::native == std::endian::little)
                result = detail::byte_swap(result);
            return result;
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_order(byte_order o) const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            if (o == byte_order::little)
            {
                if constexpr (std::endian::native != std::endian::little)
                    result = detail::byte_swap(result);
            }
            else if (o == byte_order::big || o == byte_order::network)
            {
                if constexpr (std::endian::native == std::endian::little)
                    result = detail::byte_swap(result);
            }
            // native: data is already in native byte order, no swap needed
            return result;
        }

        ENDIAN_ALWAYS_INLINE void store_native(T value) noexcept
        {
            std::memcpy(data_.data(), &value, sizeof(T));
        }

        ENDIAN_ALWAYS_INLINE void store_little(T value) noexcept
        {
            if constexpr (std::endian::native != std::endian::little)
                value = detail::byte_swap(value);
            std::memcpy(data_.data(), &value, sizeof(T));
        }

        ENDIAN_ALWAYS_INLINE void store_big(T value) noexcept
        {
            if constexpr (std::endian::native == std::endian::little)
                value = detail::byte_swap(value);
            std::memcpy(data_.data(), &value, sizeof(T));
        }

        ENDIAN_ALWAYS_INLINE void store_order(byte_order o, T value) noexcept
        {
            if (o == byte_order::big || o == byte_order::network)
                store_big(value);
            else if (o == byte_order::little)
                store_little(value);
            else
                store_native(value);
        }

        [[nodiscard]] constexpr std::span<std::byte>       raw_data()       noexcept { return data_; }
        [[nodiscard]] constexpr std::span<const std::byte> raw_data() const noexcept { return data_; }
        [[nodiscard]] constexpr size_t                     size()     const noexcept { return data_.size(); }
    };

    class const_buffer_view
    {
        std::span<const std::byte> data_;

    public:
        constexpr explicit const_buffer_view(std::span<const std::byte> data)
            : data_(data)
        {
            if (ENDIAN_UNLIKELY(data.size() < sizeof(T)))
                throw std::out_of_range("Buffer size is too small for type T");
        }

        template <size_t N>
        constexpr explicit const_buffer_view(const std::byte (&arr)[N])
            : data_(arr)
        {
            static_assert(N >= sizeof(T), "Array size must be at least sizeof(T)");
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_native() const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            return result;
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_little() const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            if constexpr (std::endian::native != std::endian::little)
                result = detail::byte_swap(result);
            return result;
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_big() const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            if constexpr (std::endian::native == std::endian::little)
                result = detail::byte_swap(result);
            return result;
        }

        [[nodiscard]] ENDIAN_ALWAYS_INLINE T load_order(byte_order o) const noexcept
        {
            T result;
            std::memcpy(&result, data_.data(), sizeof(T));
            if (o == byte_order::little)
            {
                if constexpr (std::endian::native != std::endian::little)
                    result = detail::byte_swap(result);
            }
            else if (o == byte_order::big || o == byte_order::network)
            {
                if constexpr (std::endian::native == std::endian::little)
                    result = detail::byte_swap(result);
            }
            // native: data is already in native byte order, no swap needed
            return result;
        }

        [[nodiscard]] constexpr std::span<const std::byte> raw_data() const noexcept { return data_; }
        [[nodiscard]] constexpr size_t                     size()        const noexcept { return data_.size(); }
    };

    struct network_order
    {
        static constexpr byte_order byte_order_value = byte_order::network;
        static constexpr std::endian endian_value    = std::endian::big;

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T host_to_network(T v) noexcept
        {
            if constexpr (std::endian::native == std::endian::little)
                return detail::byte_swap(v);
            else
                return v;
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T network_to_host(T v) noexcept
        {
            return host_to_network(v);
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T convert(T v) noexcept
        {
            return host_to_network(v);
        }
    };

    struct little_endian_order
    {
        static constexpr byte_order byte_order_value = byte_order::little;
        static constexpr std::endian endian_value    = std::endian::little;

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T host_to_little(T v) noexcept
        {
            if constexpr (std::endian::native == std::endian::big)
                return detail::byte_swap(v);
            else
                return v;
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T little_to_host(T v) noexcept
        {
            return host_to_little(v);
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T convert(T v) noexcept
        {
            return host_to_little(v);
        }
    };

    struct big_endian_order
    {
        static constexpr byte_order byte_order_value = byte_order::big;
        static constexpr std::endian endian_value    = std::endian::big;

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T host_to_big(T v) noexcept
        {
            if constexpr (std::endian::native == std::endian::little)
                return detail::byte_swap(v);
            else
                return v;
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T big_to_host(T v) noexcept
        {
            return host_to_big(v);
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE constexpr T convert(T v) noexcept
        {
            return host_to_big(v);
        }
    };

    template <typename Stream>
    void serialize(Stream& stream) const
    {
        T value = storage_.value;
        if constexpr (std::endian::native == std::endian::little)
            value = detail::byte_swap(value);
        stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template <typename Stream>
    void deserialize(Stream& stream)
    {
        T value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(T));
        if constexpr (std::endian::native == std::endian::little)
            storage_.value = detail::byte_swap(value);
        else
            storage_.value = value;
    }

    template <typename Stream>
    static void serialize_batch(Stream& stream, const T* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            T value = data[i];
            if constexpr (std::endian::native == std::endian::little)
                value = detail::byte_swap(value);
            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }
    }

    template <typename Stream>
    static void deserialize_batch(Stream& stream, T* data, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            T value;
            stream.read(reinterpret_cast<char*>(&value), sizeof(T));
            if constexpr (std::endian::native == std::endian::little)
                data[i] = detail::byte_swap(value);
            else
                data[i] = value;
        }
    }

#ifdef ENDIAN_ENABLE_METRICS
    struct performance_metrics
    {
        std::atomic<uint64_t> conversions{0};
        std::atomic<uint64_t> batch_operations{0};
        std::atomic<uint64_t> mmio_operations{0};
        std::atomic<uint64_t> bytes_processed{0};

        void reset() noexcept
        {
            conversions      = 0;
            batch_operations = 0;
            mmio_operations  = 0;
            bytes_processed  = 0;
        }

        [[nodiscard]] double conversions_per_second() const noexcept
        {
            return static_cast<double>(conversions.load(std::memory_order_relaxed));
        }

        [[nodiscard]] double throughput_mbps() const noexcept
        {
            return static_cast<double>(bytes_processed.load(std::memory_order_relaxed)) / (1024.0 * 1024.0);
        }
    };

    [[nodiscard]] static performance_metrics& metrics() noexcept
    {
        static performance_metrics instance;
        return instance;
    }
#endif

    constexpr explicit operator T() const noexcept { return native(); }

    constexpr basic_endian& operator=(T value) noexcept
    {
        store_native(value);
        return *this;
    }

    friend constexpr bool operator==(const basic_endian& lhs,
                                     const basic_endian& rhs) noexcept
    {
        return lhs.native() == rhs.native();
    }

    friend constexpr auto operator<=>(const basic_endian& lhs,
                                      const basic_endian& rhs) noexcept
    {
        return lhs.native() <=> rhs.native();
    }
};

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
byte_swap(T v) noexcept
{
    return detail::byte_swap(v);
}

template <detail::IntegralByteSwappable T>
[[nodiscard]] consteval T
byte_swap_consteval(T v) noexcept
{
    return detail::byte_swap_consteval(v);
}

template <detail::ByteSwappable T>
void batch_byte_swap(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count,
    bool check_alignment = true)
{
    detail::batch_byte_swap(src, dst, count, check_alignment);
}

template <detail::IntegralByteSwappable T>
void batch_byte_swap_inplace(
    T* ENDIAN_RESTRICT data,
    size_t count) noexcept
{
    detail::batch_byte_swap_inplace(data, count);
}

template <detail::IntegralByteSwappable T>
void batch_byte_swap_prefetch(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count,
    size_t prefetch_distance = 8) noexcept
{
    detail::batch_byte_swap_prefetch(src, dst, count, prefetch_distance);
}

template <detail::IntegralByteSwappable T>
void batch_byte_swap_nt(
    T* ENDIAN_RESTRICT dst,
    const T* ENDIAN_RESTRICT src,
    size_t count) noexcept
{
    detail::batch_byte_swap_nt(dst, src, count);
}

using le_int16  = basic_endian<int16_t>;
using le_int32  = basic_endian<int32_t>;
using le_int64  = basic_endian<int64_t>;
using le_uint16 = basic_endian<uint16_t>;
using le_uint32 = basic_endian<uint32_t>;
using le_uint64 = basic_endian<uint64_t>;

using be_int16  = basic_endian<int16_t>;
using be_int32  = basic_endian<int32_t>;
using be_int64  = basic_endian<int64_t>;
using be_uint16 = basic_endian<uint16_t>;
using be_uint32 = basic_endian<uint32_t>;
using be_uint64 = basic_endian<uint64_t>;

using le_float32 = basic_endian<float>;
using le_float64 = basic_endian<double>;

using be_float32 = basic_endian<float>;
using be_float64 = basic_endian<double>;

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
to_native(const basic_endian<T>& v) noexcept { return v.native(); }

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
to_little(const basic_endian<T>& v) noexcept { return v.little(); }

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
to_big(const basic_endian<T>& v) noexcept { return v.big(); }

namespace literals
{

[[nodiscard]] constexpr auto operator""_le(unsigned long long value) noexcept
{
    return basic_endian<uint64_t>(value);
}

[[nodiscard]] constexpr auto operator""_be(unsigned long long value) noexcept
{
    return basic_endian<uint64_t>(value);
}

[[nodiscard]] constexpr auto operator""_le32(unsigned long long value) noexcept
{
    return basic_endian<uint32_t>(static_cast<uint32_t>(value));
}

[[nodiscard]] constexpr auto operator""_be32(unsigned long long value) noexcept
{
    return basic_endian<uint32_t>(static_cast<uint32_t>(value));
}

[[nodiscard]] constexpr auto operator""_le16(unsigned long long value) noexcept
{
    return basic_endian<uint16_t>(static_cast<uint16_t>(value));
}

[[nodiscard]] constexpr auto operator""_be16(unsigned long long value) noexcept
{
    return basic_endian<uint16_t>(static_cast<uint16_t>(value));
}

} // namespace literals

namespace detail
{

template <typename T>
class endian_iterator_proxy
{
    T* ptr_;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = basic_endian<std::remove_const_t<T>>;
    using difference_type   = std::ptrdiff_t;
    using pointer           = std::conditional_t<std::is_const_v<T>, const value_type*, value_type*>;
    using reference         = std::conditional_t<std::is_const_v<T>, const value_type&, value_type&>;

    explicit endian_iterator_proxy(T* ptr) noexcept : ptr_(ptr) {}

    reference operator*() const noexcept
    {
        return *reinterpret_cast<pointer>(ptr_);
    }

    pointer operator->() const noexcept
    {
        return reinterpret_cast<pointer>(ptr_);
    }

    endian_iterator_proxy& operator++() noexcept
    {
        ++ptr_;
        return *this;
    }

    endian_iterator_proxy operator++(int) noexcept
    {
        endian_iterator_proxy tmp = *this;
        ++ptr_;
        return tmp;
    }

    endian_iterator_proxy& operator--() noexcept
    {
        --ptr_;
        return *this;
    }

    endian_iterator_proxy operator--(int) noexcept
    {
        endian_iterator_proxy tmp = *this;
        --ptr_;
        return tmp;
    }

    endian_iterator_proxy& operator+=(difference_type n) noexcept
    {
        ptr_ += n;
        return *this;
    }

    endian_iterator_proxy& operator-=(difference_type n) noexcept
    {
        ptr_ -= n;
        return *this;
    }

    friend endian_iterator_proxy operator+(endian_iterator_proxy it, difference_type n) noexcept
    {
        return endian_iterator_proxy(it.ptr_ + n);
    }

    friend endian_iterator_proxy operator+(difference_type n, endian_iterator_proxy it) noexcept
    {
        return endian_iterator_proxy(it.ptr_ + n);
    }

    friend endian_iterator_proxy operator-(endian_iterator_proxy it, difference_type n) noexcept
    {
        return endian_iterator_proxy(it.ptr_ - n);
    }

    friend difference_type operator-(const endian_iterator_proxy& lhs,
                                     const endian_iterator_proxy& rhs) noexcept
    {
        return lhs.ptr_ - rhs.ptr_;
    }

    friend bool operator==(const endian_iterator_proxy& lhs,
                           const endian_iterator_proxy& rhs) noexcept
    {
        return lhs.ptr_ == rhs.ptr_;
    }

    friend bool operator<(const endian_iterator_proxy& lhs,
                          const endian_iterator_proxy& rhs) noexcept
    {
        return lhs.ptr_ < rhs.ptr_;
    }

    T& base() noexcept { return *ptr_; }
    const T& base() const noexcept { return *ptr_; }
};

} // namespace detail

template <detail::IntegralByteSwappable T, byte_order Order = byte_order::native>
class endian_span
{
    std::span<T> span_;

public:
    using value_type      = basic_endian<T>;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using iterator        = detail::endian_iterator_proxy<T>;
    using const_iterator  = detail::endian_iterator_proxy<const T>;

    constexpr endian_span() noexcept = default;

    constexpr endian_span(T* ptr, size_type count) noexcept
        : span_(ptr, count) {}

    constexpr endian_span(T* first, T* last) noexcept
        : span_(first, last) {}

    template <size_t N>
    constexpr endian_span(T (&arr)[N]) noexcept
        : span_(arr) {}

    template <typename Range>
        requires std::ranges::contiguous_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, T>
    constexpr explicit endian_span(Range&& r) noexcept
        : span_(std::ranges::data(r), std::ranges::size(r)) {}

    [[nodiscard]] constexpr iterator begin() noexcept
    {
        return iterator(span_.data());
    }

    [[nodiscard]] constexpr iterator end() noexcept
    {
        return iterator(span_.data() + span_.size());
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return const_iterator(span_.data());
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return const_iterator(span_.data() + span_.size());
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return const_iterator(span_.data());
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return const_iterator(span_.data() + span_.size());
    }

    [[nodiscard]] constexpr reference operator[](size_type idx) noexcept
    {
        return *reinterpret_cast<value_type*>(span_.data() + idx);
    }

    [[nodiscard]] constexpr const_reference operator[](size_type idx) const noexcept
    {
        return *reinterpret_cast<const value_type*>(span_.data() + idx);
    }

    [[nodiscard]] constexpr size_type size() const noexcept { return span_.size(); }
    [[nodiscard]] constexpr bool      empty() const noexcept { return span_.empty(); }

    [[nodiscard]] constexpr T*       data() noexcept { return span_.data(); }
    [[nodiscard]] constexpr const T* data() const noexcept { return span_.data(); }

    [[nodiscard]] constexpr std::span<T>       underlying() noexcept { return span_; }
    [[nodiscard]] constexpr std::span<const T> underlying() const noexcept { return span_; }

    void convert_to_host() noexcept
    {
        if constexpr (Order == byte_order::native)
            return;
        else
            detail::batch_byte_swap_inplace(span_.data(), span_.size());
    }

    void convert_to(byte_order order) noexcept
    {
        if (order == Order)
            return;
        detail::batch_byte_swap_inplace(span_.data(), span_.size());
    }
};

template <detail::IntegralByteSwappable T, byte_order Order = byte_order::native>
class const_endian_span
{
    std::span<const T> span_;

public:
    using value_type      = const basic_endian<T>;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = const basic_endian<T>&;
    using const_reference = const basic_endian<T>&;
    using iterator        = detail::endian_iterator_proxy<const T>;
    using const_iterator  = detail::endian_iterator_proxy<const T>;

    constexpr const_endian_span() noexcept = default;

    constexpr const_endian_span(const T* ptr, size_type count) noexcept
        : span_(ptr, count) {}

    constexpr const_endian_span(const T* first, const T* last) noexcept
        : span_(first, last) {}

    template <size_t N>
    constexpr const_endian_span(const T (&arr)[N]) noexcept
        : span_(arr) {}

    template <typename Range>
        requires std::ranges::contiguous_range<Range> &&
                 std::same_as<std::ranges::range_value_t<Range>, T>
    constexpr explicit const_endian_span(Range&& r) noexcept
        : span_(std::ranges::data(r), std::ranges::size(r)) {}

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return const_iterator(span_.data());
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return const_iterator(span_.data() + span_.size());
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return const_iterator(span_.data());
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return const_iterator(span_.data() + span_.size());
    }

    [[nodiscard]] constexpr const_reference operator[](size_type idx) const noexcept
    {
        return *reinterpret_cast<const value_type*>(span_.data() + idx);
    }

    [[nodiscard]] constexpr size_type size() const noexcept { return span_.size(); }
    [[nodiscard]] constexpr bool      empty() const noexcept { return span_.empty(); }

    [[nodiscard]] constexpr const T*       data() const noexcept { return span_.data(); }
    [[nodiscard]] constexpr std::span<const T> underlying() const noexcept { return span_; }
};

} // namespace endian
