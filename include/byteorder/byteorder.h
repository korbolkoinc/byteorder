#pragma once

#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#if defined(_MSC_VER)
#  include <intrin.h>      // __cpuid, _byteswap_ushort/ulong/uint64
#endif
#if defined(__x86_64__) || defined(_M_X64)
#  include <immintrin.h>   // AVX2 (_mm256_*)
#  if defined(__GNUC__) || defined(__clang__)
#    include <cpuid.h>     // __get_cpuid
#  endif
#elif defined(__ARM_NEON)
#  include <arm_neon.h>
#endif

// ============================================================================
// Compiler / Platform Portability Macros
// ============================================================================

#if defined(_MSC_VER)
#  define ENDIAN_ALWAYS_INLINE  __forceinline
#  define ENDIAN_LIKELY(x)      (x)
#  define ENDIAN_UNLIKELY(x)    (x)
#  define ENDIAN_RESTRICT       __restrict
#  define ENDIAN_ALIGNED(x)     __declspec(align(x))
#  define ENDIAN_VECTORCALL     __vectorcall
#  define ENDIAN_EXPORT         __declspec(dllexport)
#  define ENDIAN_IMPORT         __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
#  define ENDIAN_ALWAYS_INLINE  inline __attribute__((always_inline))
#  define ENDIAN_LIKELY(x)      __builtin_expect(!!(x), 1)
#  define ENDIAN_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#  define ENDIAN_RESTRICT       __restrict__
#  define ENDIAN_ALIGNED(x)     __attribute__((aligned(x)))
#  define ENDIAN_VECTORCALL
#  define ENDIAN_EXPORT         __attribute__((visibility("default")))
#  define ENDIAN_IMPORT
#else
#  define ENDIAN_ALWAYS_INLINE  inline
#  define ENDIAN_LIKELY(x)      (x)
#  define ENDIAN_UNLIKELY(x)    (x)
#  define ENDIAN_RESTRICT
#  define ENDIAN_ALIGNED(x)
#  define ENDIAN_VECTORCALL
#  define ENDIAN_EXPORT
#  define ENDIAN_IMPORT
#endif

#if defined(ENDIAN_BUILD_SHARED)
#  define ENDIAN_API ENDIAN_EXPORT
#elif defined(ENDIAN_USE_SHARED)
#  define ENDIAN_API ENDIAN_IMPORT
#else
#  define ENDIAN_API
#endif

namespace endian
{

// ============================================================================
// Version Information
// ============================================================================

struct version
{
    static constexpr uint32_t       major = 1;
    static constexpr uint32_t       minor = 0;
    static constexpr uint32_t       patch = 0;
    static constexpr std::string_view str = "1.0.0";
};

// ============================================================================
// Exception Types
// ============================================================================

class endian_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class alignment_error : public endian_error
{
public:
    explicit alignment_error(const char* msg) : endian_error(msg) {}
};

// ============================================================================
// Implementation Details (not for direct use)
// ============================================================================

namespace detail
{

// ----------------------------------------------------------------------------
// CPU Feature Detection
// ----------------------------------------------------------------------------

struct cpu_features
{
    [[nodiscard]] static bool has_avx2() noexcept
    {
#if defined(__x86_64__) || defined(_M_X64)
#  if defined(_MSC_VER)
        int cpu_info[4];
        __cpuid(cpu_info, 7);
        return (cpu_info[1] & (1 << 5)) != 0;
#  elif defined(__GNUC__) || defined(__clang__)
        unsigned int eax, ebx, ecx, edx;
        __get_cpuid(7, &eax, &ebx, &ecx, &edx);
        return (ebx & (1 << 5)) != 0;
#  else
        return false;
#  endif
#else
        return false;
#endif
    }

    [[nodiscard]] static bool has_neon() noexcept
    {
#if defined(__ARM_NEON)
        return true;
#else
        return false;
#endif
    }
};

// ----------------------------------------------------------------------------
// ByteSwappable Concept
// ----------------------------------------------------------------------------

template <typename T>
concept ByteSwappable =
    std::is_trivially_copyable_v<T> &&
    ((sizeof(T) == 1) || (sizeof(T) == 2) || (sizeof(T) == 4) || (sizeof(T) == 8)) &&
    !std::is_same_v<T, bool> &&
    !std::is_same_v<T, char>;

// ----------------------------------------------------------------------------
// byte_swap — scalar byte-reversal with hardware intrinsic acceleration
// ----------------------------------------------------------------------------

template <ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T byte_swap(T v) noexcept
{
    if constexpr (sizeof(T) == 1)
    {
        return v;
    }

    // Runtime SIMD paths — skipped entirely during constant evaluation
#if defined(__x86_64__) || defined(_M_X64)
    if (!std::is_constant_evaluated() && cpu_features::has_avx2())
    {
        if constexpr (sizeof(T) == 2)
            return static_cast<T>(_byteswap_ushort(static_cast<uint16_t>(v)));
        if constexpr (sizeof(T) == 4)
            return static_cast<T>(_byteswap_ulong(static_cast<uint32_t>(v)));
        if constexpr (sizeof(T) == 8)
            return static_cast<T>(_byteswap_uint64(static_cast<uint64_t>(v)));
    }
#elif defined(__ARM_NEON)
    if (!std::is_constant_evaluated() && cpu_features::has_neon())
    {
        if constexpr (sizeof(T) == 2)
        {
            uint16x4_t data = vdup_n_u16(static_cast<uint16_t>(v));
            data = vreinterpret_u16_u8(vrev16_u8(vreinterpret_u8_u16(data)));
            return static_cast<T>(vget_lane_u16(data, 0));
        }
        if constexpr (sizeof(T) == 4)
        {
            uint32x2_t data = vdup_n_u32(static_cast<uint32_t>(v));
            data = vreinterpret_u32_u8(vrev32_u8(vreinterpret_u8_u32(data)));
            return static_cast<T>(vget_lane_u32(data, 0));
        }
        if constexpr (sizeof(T) == 8)
        {
            uint64x1_t data = vdup_n_u64(static_cast<uint64_t>(v));
            data = vreinterpret_u64_u8(vrev64_u8(vreinterpret_u8_u64(data)));
            return static_cast<T>(vget_lane_u64(data, 0));
        }
    }
#endif

    // Portable fallback — also used at compile time
    if constexpr (sizeof(T) == 2)
    {
        return static_cast<T>(((v & 0xFF00u) >> 8) | ((v & 0x00FFu) << 8));
    }
    if constexpr (sizeof(T) == 4)
    {
        return static_cast<T>(
            ((v & 0xFF000000u) >> 24) |
            ((v & 0x00FF0000u) >>  8) |
            ((v & 0x0000FF00u) <<  8) |
            ((v & 0x000000FFu) << 24));
    }
    if constexpr (sizeof(T) == 8)
    {
        return static_cast<T>(
            ((v & 0xFF00000000000000ull) >> 56) |
            ((v & 0x00FF000000000000ull) >> 40) |
            ((v & 0x0000FF0000000000ull) >> 24) |
            ((v & 0x000000FF00000000ull) >>  8) |
            ((v & 0x00000000FF000000ull) <<  8) |
            ((v & 0x0000000000FF0000ull) << 24) |
            ((v & 0x000000000000FF00ull) << 40) |
            ((v & 0x00000000000000FFull) << 56));
    }
}

// ----------------------------------------------------------------------------
// batch_byte_swap — SIMD-accelerated bulk reversal
// ----------------------------------------------------------------------------

template <ByteSwappable T>
ENDIAN_ALWAYS_INLINE void batch_byte_swap(
    const T* ENDIAN_RESTRICT src,
    T*       ENDIAN_RESTRICT dst,
    size_t count,
    bool check_alignment = true)
{
    if (check_alignment)
    {
        if (reinterpret_cast<std::uintptr_t>(src) % alignof(T) != 0 ||
            reinterpret_cast<std::uintptr_t>(dst) % alignof(T) != 0)
        {
            throw alignment_error("Source or destination pointer is not properly aligned");
        }
    }

#if defined(__x86_64__) || defined(_M_X64)
    if (cpu_features::has_avx2())
    {
        if constexpr (sizeof(T) == 2)
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
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i),
                                    _mm256_shuffle_epi8(v, shuf));
            }
            for (; i < count; ++i)
                dst[i] = byte_swap(src[i]);
            return;
        }
        if constexpr (sizeof(T) == 8)
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
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i),
                                    _mm256_shuffle_epi8(v, shuf));
            }
            for (; i < count; ++i)
                dst[i] = byte_swap(src[i]);
            return;
        }
    }
#elif defined(__ARM_NEON)
    if (cpu_features::has_neon())
    {
        if constexpr (sizeof(T) == 2)
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
        if constexpr (sizeof(T) == 4)
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
        if constexpr (sizeof(T) == 8)
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
    }
#endif

    // Portable fallback
    for (size_t i = 0; i < count; ++i)
        dst[i] = byte_swap(src[i]);
}

// ----------------------------------------------------------------------------
// cache_aligned_storage — per-value storage padded to a cache line
// ----------------------------------------------------------------------------

template <typename T, size_t CacheLineSize = 64>
struct ENDIAN_ALIGNED(CacheLineSize) cache_aligned_storage
{
    static_assert(sizeof(T) <= CacheLineSize,
                  "T must not exceed CacheLineSize bytes");

    T          value;
    std::byte  padding[CacheLineSize - (sizeof(T) % CacheLineSize)];

    static constexpr size_t alignment = CacheLineSize;

    constexpr cache_aligned_storage() noexcept : value{}, padding{} {}
    constexpr explicit cache_aligned_storage(T v) noexcept : value(v), padding{} {}
};

} // namespace detail

// ============================================================================
// basic_endian — primary value-wrapper type
// ============================================================================

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

    // -------------------------------------------------------------------------
    // Read accessors
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Write accessors
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Batch conversion (contiguous ranges)
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Memory-mapped I/O (atomic read/write with configurable memory order)
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Zero-copy buffer view
    // -------------------------------------------------------------------------

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

        [[nodiscard]] constexpr std::span<std::byte>       raw_data()       noexcept { return data_; }
        [[nodiscard]] constexpr std::span<const std::byte> raw_data() const noexcept { return data_; }
    };

    // -------------------------------------------------------------------------
    // Network byte-order helpers (big-endian ≡ network order per RFC 1700)
    // -------------------------------------------------------------------------

    struct network_order
    {
        static constexpr std::endian byte_order = std::endian::big;

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE T host_to_network(T v) noexcept
        {
            if constexpr (std::endian::native == std::endian::little)
                return detail::byte_swap(v);
            else
                return v;
        }

        [[nodiscard]] static ENDIAN_ALWAYS_INLINE T network_to_host(T v) noexcept
        {
            return host_to_network(v); // symmetric
        }
    };

    // -------------------------------------------------------------------------
    // Stream serialization (always writes big-endian / network order)
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Optional performance counters (compile with -DENDIAN_ENABLE_METRICS)
    // -------------------------------------------------------------------------

#ifdef ENDIAN_ENABLE_METRICS
    struct performance_metrics
    {
        std::atomic<uint64_t> conversions{0};
        std::atomic<uint64_t> batch_operations{0};
        std::atomic<uint64_t> mmio_operations{0};

        void reset() noexcept
        {
            conversions      = 0;
            batch_operations = 0;
            mmio_operations  = 0;
        }
    };

    [[nodiscard]] static performance_metrics& metrics() noexcept
    {
        static performance_metrics instance;
        return instance;
    }
#endif

    // -------------------------------------------------------------------------
    // Operators
    // -------------------------------------------------------------------------

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

// ============================================================================
// Convenience Type Aliases
// ============================================================================

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

// ============================================================================
// Free-function Helpers
// ============================================================================

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
to_native(const basic_endian<T>& v) noexcept { return v.native(); }

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
to_little(const basic_endian<T>& v) noexcept { return v.little(); }

template <detail::ByteSwappable T>
[[nodiscard]] ENDIAN_ALWAYS_INLINE constexpr T
to_big(const basic_endian<T>& v) noexcept { return v.big(); }

// ============================================================================
// User-defined Literals
// ============================================================================

namespace literals
{

// Usage:  auto v = 0xDEADBEEF_le;   auto w = 0xDEADBEEF_be;
// Note:   combine with static_cast<> or U suffix, NOT with 'ull' suffix
[[nodiscard]] constexpr auto operator""_le(unsigned long long value) noexcept
{
    return basic_endian<uint64_t>(static_cast<uint64_t>(value));
}

[[nodiscard]] constexpr auto operator""_be(unsigned long long value) noexcept
{
    return basic_endian<uint64_t>(static_cast<uint64_t>(value));
}

} // namespace literals

} // namespace endian
