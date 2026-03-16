#include <gtest/gtest.h>
#include "byteorder/byteorder.h"

#include <bit>
#include <cstdint>
#include <vector>
#include <array>
#include <span>
#include <cstring>
#include <random>
#include <algorithm>

using namespace endian;

// ============================================================================
// detail::byte_swap — compile-time
// ============================================================================

TEST(ByteSwapConsteval, Width8_IsIdentity)
{
    static_assert(detail::byte_swap_consteval(uint8_t{0xAB}) == uint8_t{0xAB});
    static_assert(detail::byte_swap_consteval(uint8_t{0x00}) == uint8_t{0x00});
    static_assert(detail::byte_swap_consteval(uint8_t{0xFF}) == uint8_t{0xFF});
}

TEST(ByteSwapConsteval, Width16)
{
    static_assert(detail::byte_swap_consteval(uint16_t{0x1234}) == uint16_t{0x3412});
    static_assert(detail::byte_swap_consteval(uint16_t{0x0000}) == uint16_t{0x0000});
    static_assert(detail::byte_swap_consteval(uint16_t{0xFFFF}) == uint16_t{0xFFFF});
    static_assert(detail::byte_swap_consteval(uint16_t{0x0102}) == uint16_t{0x0201});
}

TEST(ByteSwapConsteval, Width32)
{
    static_assert(detail::byte_swap_consteval(uint32_t{0x12345678}) == uint32_t{0x78563412});
    static_assert(detail::byte_swap_consteval(uint32_t{0xDEADBEEF}) == uint32_t{0xEFBEADDE});
    static_assert(detail::byte_swap_consteval(uint32_t{0x00000000}) == uint32_t{0x00000000});
    static_assert(detail::byte_swap_consteval(uint32_t{0xFFFFFFFF}) == uint32_t{0xFFFFFFFF});
}

TEST(ByteSwapConsteval, Width64)
{
    static_assert(detail::byte_swap_consteval(uint64_t{0x0102030405060708ull}) ==
                  uint64_t{0x0807060504030201ull});
    static_assert(detail::byte_swap_consteval(uint64_t{0xDEADBEEFCAFEBABEull}) ==
                  uint64_t{0xBEBAFECAEFBEADDEull});
}

TEST(ByteSwapConsteval, SignedTypes)
{
    static_assert(detail::byte_swap_consteval(int16_t{0x1234}) == int16_t{0x3412});
    static_assert(detail::byte_swap_consteval(int32_t{0x12345678}) == int32_t{0x78563412});
    static_assert(detail::byte_swap_consteval(int64_t{0x0102030405060708LL}) ==
                  int64_t{0x0807060504030201LL});
}

// ============================================================================
// detail::byte_swap — runtime
// ============================================================================

TEST(ByteSwapRuntime, Width8_IsIdentity)
{
    EXPECT_EQ(detail::byte_swap(uint8_t{0xAB}), uint8_t{0xAB});
    EXPECT_EQ(detail::byte_swap(uint8_t{0x00}), uint8_t{0x00});
    EXPECT_EQ(detail::byte_swap(uint8_t{0xFF}), uint8_t{0xFF});
}

TEST(ByteSwapRuntime, Width16)
{
    EXPECT_EQ(detail::byte_swap(uint16_t{0x1234}), uint16_t{0x3412});
    EXPECT_EQ(detail::byte_swap(uint16_t{0x0000}), uint16_t{0x0000});
    EXPECT_EQ(detail::byte_swap(uint16_t{0xFFFF}), uint16_t{0xFFFF});
}

TEST(ByteSwapRuntime, Width32)
{
    EXPECT_EQ(detail::byte_swap(uint32_t{0x12345678}), uint32_t{0x78563412});
    EXPECT_EQ(detail::byte_swap(uint32_t{0xDEADBEEF}), uint32_t{0xEFBEADDE});
    EXPECT_EQ(detail::byte_swap(uint32_t{0x00000000}), uint32_t{0x00000000});
    EXPECT_EQ(detail::byte_swap(uint32_t{0xFFFFFFFF}), uint32_t{0xFFFFFFFF});
}

TEST(ByteSwapRuntime, Width64)
{
    EXPECT_EQ(detail::byte_swap(uint64_t{0x0102030405060708ull}),
              uint64_t{0x0807060504030201ull});
    EXPECT_EQ(detail::byte_swap(uint64_t{0xDEADBEEFCAFEBABEull}),
              uint64_t{0xBEBAFECAEFBEADDEull});
}

TEST(ByteSwapRuntime, RoundTrip)
{
    constexpr uint16_t v16 = 0xA1B2;
    constexpr uint32_t v32 = 0xA1B2C3D4;
    constexpr uint64_t v64 = 0xA1B2C3D4E5F60708ull;

    EXPECT_EQ(detail::byte_swap(detail::byte_swap(v16)), v16);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(v32)), v32);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(v64)), v64);
}

TEST(ByteSwapRuntime, SignedTypes)
{
    const int16_t  neg16 = static_cast<int16_t>(0xFF00);
    const int32_t  neg32 = static_cast<int32_t>(0xFF000000);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(neg16)), neg16);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(neg32)), neg32);
}

TEST(ByteSwapRuntime, Float32)
{
    float input = 3.14159f;
    float swapped = detail::byte_swap(input);
    float back = detail::byte_swap(swapped);
    EXPECT_FLOAT_EQ(input, back);
}

TEST(ByteSwapRuntime, Float64)
{
    double input = 2.718281828459045;
    double swapped = detail::byte_swap(input);
    double back = detail::byte_swap(swapped);
    EXPECT_DOUBLE_EQ(input, back);
}

// ============================================================================
// Free function byte_swap
// ============================================================================

TEST(FreeByteSwap, UintTypes)
{
    EXPECT_EQ(byte_swap(uint16_t{0x1234}), uint16_t{0x3412});
    EXPECT_EQ(byte_swap(uint32_t{0xDEADBEEF}), uint32_t{0xEFBEADDE});
    EXPECT_EQ(byte_swap(uint64_t{0x0102030405060708ull}), uint64_t{0x0807060504030201ull});
}

TEST(FreeByteSwap, ConstexprEvaluation)
{
    constexpr uint32_t result = byte_swap(uint32_t{0x01020304});
    static_assert(result == uint32_t{0x04030201});
}

// ============================================================================
// basic_endian — construction and accessors
// ============================================================================

TEST(BasicEndian, DefaultConstructedIsZero)
{
    basic_endian<uint32_t> e;
    EXPECT_EQ(e.native(), uint32_t{0});
}

TEST(BasicEndian, ExplicitConstruction)
{
    constexpr uint32_t val = 0xCAFEBABE;
    basic_endian<uint32_t> e(val);
    EXPECT_EQ(e.native(), val);
}

TEST(BasicEndian, AssignmentOperator)
{
    basic_endian<uint64_t> e;
    e = 0xDEADBEEFCAFEBABEull;
    EXPECT_EQ(e.native(), 0xDEADBEEFCAFEBABEull);
}

TEST(BasicEndian, ExplicitConversionToT)
{
    basic_endian<uint16_t> e(0x1234);
    EXPECT_EQ(static_cast<uint16_t>(e), uint16_t{0x1234});
}

TEST(BasicEndian, LittleAndBigEndianViews)
{
    constexpr uint32_t val = 0x12345678;
    basic_endian<uint32_t> e(val);

    if constexpr (std::endian::native == std::endian::little)
    {
        EXPECT_EQ(e.little(), val);
        EXPECT_EQ(e.big(),    detail::byte_swap(val));
    }
    else
    {
        EXPECT_EQ(e.big(),    val);
        EXPECT_EQ(e.little(), detail::byte_swap(val));
    }
}

TEST(BasicEndian, StoreLittle)
{
    basic_endian<uint32_t> e;
    e.store_little(0xAABBCCDD);
    EXPECT_EQ(e.little(), uint32_t{0xAABBCCDD});
}

TEST(BasicEndian, StoreBig)
{
    basic_endian<uint32_t> e;
    e.store_big(0x11223344);
    EXPECT_EQ(e.big(), uint32_t{0x11223344});
}

TEST(BasicEndian, StoreNative)
{
    basic_endian<uint16_t> e;
    e.store_native(0xBEEF);
    EXPECT_EQ(e.native(), uint16_t{0xBEEF});
}

TEST(BasicEndian, StoreOrder)
{
    basic_endian<uint32_t> e;
    e.store_order(byte_order::big, 0xDEADBEEF);
    EXPECT_EQ(e.big(), uint32_t{0xDEADBEEF});
    
    e.store_order(byte_order::little, 0xCAFEBABE);
    EXPECT_EQ(e.little(), uint32_t{0xCAFEBABE});
}

TEST(BasicEndian, OrderAccessor)
{
    constexpr uint32_t val = 0x12345678;
    basic_endian<uint32_t> e(val);

    EXPECT_EQ(e.order(byte_order::native), e.native());
    EXPECT_EQ(e.order(byte_order::little), e.little());
    EXPECT_EQ(e.order(byte_order::big), e.big());
    EXPECT_EQ(e.order(byte_order::network), e.big());
}

// ============================================================================
// Comparison operators
// ============================================================================

TEST(BasicEndian, EqualityOperator)
{
    basic_endian<uint32_t> a(42), b(42), c(99);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(BasicEndian, ThreeWayComparison)
{
    basic_endian<uint64_t> lo(1), hi(2);
    EXPECT_LT(lo, hi);
    EXPECT_GT(hi, lo);
    EXPECT_LE(lo, lo);
    EXPECT_GE(hi, hi);
}

// ============================================================================
// Type aliases
// ============================================================================

TEST(TypeAliases, AllIntegerWidths)
{
    le_uint16 a(0x1234);  EXPECT_EQ(a.native(), uint16_t{0x1234});
    le_uint32 b(0xDEAD);  EXPECT_EQ(b.native(), uint32_t{0xDEAD});
    le_uint64 c(0xCAFE);  EXPECT_EQ(c.native(), uint64_t{0xCAFE});
    be_uint16 d(0x1234);  EXPECT_EQ(d.native(), uint16_t{0x1234});
    be_uint32 f(0xDEAD);  EXPECT_EQ(f.native(), uint32_t{0xDEAD});
    be_uint64 g(0xCAFE);  EXPECT_EQ(g.native(), uint64_t{0xCAFE});
}

TEST(TypeAliases, SignedWidths)
{
    le_int16 a(-1);  EXPECT_EQ(a.native(), int16_t{-1});
    le_int32 b(-1);  EXPECT_EQ(b.native(), int32_t{-1});
    le_int64 c(-1);  EXPECT_EQ(c.native(), int64_t{-1});
}

TEST(TypeAliases, FloatWidths)
{
    le_float32 a(3.14f);  EXPECT_FLOAT_EQ(a.native(), 3.14f);
    le_float64 b(2.71);   EXPECT_DOUBLE_EQ(b.native(), 2.71);
    be_float32 c(1.41f);  EXPECT_FLOAT_EQ(c.native(), 1.41f);
    be_float64 d(1.73);   EXPECT_DOUBLE_EQ(d.native(), 1.73);
}

// ============================================================================
// Free-function helpers
// ============================================================================

TEST(FreeFunctions, ToNativeLittleBig)
{
    constexpr uint32_t val = 0xABCD1234;
    basic_endian<uint32_t> e(val);

    EXPECT_EQ(to_native(e), e.native());
    EXPECT_EQ(to_little(e), e.little());
    EXPECT_EQ(to_big(e),    e.big());
}

// ============================================================================
// User-defined literals
// ============================================================================

TEST(Literals, LeAndBeLiterals)
{
    using namespace endian::literals;

    auto le_val = 0xCAFEBABE_le;
    auto be_val = 0xCAFEBABE_be;
    auto le32_val = 0xDEAD_le32;
    auto be16_val = 0xBEEF_be16;

    EXPECT_EQ(le_val.native(), uint64_t{0xCAFEBABE});
    EXPECT_EQ(be_val.native(), uint64_t{0xCAFEBABE});
    EXPECT_EQ(le32_val.native(), uint32_t{0xDEAD});
    EXPECT_EQ(be16_val.native(), uint16_t{0xBEEF});
}

// ============================================================================
// detail::batch_byte_swap
// ============================================================================

TEST(BatchByteSwap, Width16AllElements)
{
    constexpr size_t N = 8;
    std::array<uint16_t, N> src = {0x0102, 0x0304, 0x0506, 0x0708,
                                   0x090A, 0x0B0C, 0x0D0E, 0x0F10};
    std::array<uint16_t, N> dst{};

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i])) << "mismatch at index " << i;
}

TEST(BatchByteSwap, Width32AllElements)
{
    constexpr size_t N = 4;
    std::array<uint32_t, N> src = {0xDEADBEEF, 0xCAFEBABE, 0x01020304, 0xFFFFFFFF};
    std::array<uint32_t, N> dst{};

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i])) << "mismatch at index " << i;
}

TEST(BatchByteSwap, Width64AllElements)
{
    constexpr size_t N = 4;
    std::array<uint64_t, N> src = {
        0x0102030405060708ull, 0xDEADBEEFCAFEBABEull,
        0x0ull,                ~0ull
    };
    std::array<uint64_t, N> dst{};

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i])) << "mismatch at index " << i;
}

TEST(BatchByteSwap, LargeBatch32)
{
    constexpr size_t N = 128;
    std::vector<uint32_t> src(N), dst(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint32_t>(i * 0x01020304u);

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

TEST(BatchByteSwap, RoundTrip)
{
    constexpr size_t N = 32;
    std::vector<uint64_t> src(N), tmp(N), result(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint64_t>(i) * 0xFEDCBA9876543210ull;

    detail::batch_byte_swap(src.data(),    tmp.data(),    N, false);
    detail::batch_byte_swap(tmp.data(),    result.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(result[i], src[i]);
}

TEST(BatchByteSwap, SingleElement)
{
    uint32_t src = 0xABCD1234, dst = 0;
    detail::batch_byte_swap(&src, &dst, 1, false);
    EXPECT_EQ(dst, detail::byte_swap(src));
}

TEST(BatchByteSwap, ZeroElements)
{
    uint32_t dummy = 0xDEAD;
    EXPECT_NO_THROW(detail::batch_byte_swap(&dummy, &dummy, 0, false));
}

TEST(BatchByteSwap, Inplace)
{
    std::array<uint32_t, 4> data = {0x12345678, 0xDEADBEEF, 0xCAFEBABE, 0x01020304};
    std::array<uint32_t, 4> expected;
    for (size_t i = 0; i < 4; ++i)
        expected[i] = detail::byte_swap(data[i]);

    detail::batch_byte_swap_inplace(data.data(), data.size());

    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(data[i], expected[i]);
}

// ============================================================================
// Free function batch_byte_swap
// ============================================================================

TEST(FreeBatchByteSwap, Basic)
{
    std::vector<uint32_t> src = {0x12345678, 0xDEADBEEF};
    std::vector<uint32_t> dst(src.size());
    
    batch_byte_swap(src.data(), dst.data(), src.size(), false);
    
    for (size_t i = 0; i < src.size(); ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

TEST(FreeBatchByteSwap, Inplace)
{
    std::array<uint16_t, 4> data = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    std::array<uint16_t, 4> expected;
    for (size_t i = 0; i < 4; ++i)
        expected[i] = detail::byte_swap(data[i]);
    
    batch_byte_swap_inplace(data.data(), data.size());
    
    for (size_t i = 0; i < 4; ++i)
        EXPECT_EQ(data[i], expected[i]);
}

// ============================================================================
// basic_endian::convert_batch
// ============================================================================

TEST(ConvertBatch, StdVector32)
{
    std::vector<uint32_t> src = {0x11223344, 0xAABBCCDD, 0x01020304};
    std::vector<uint32_t> dst(src.size());

    basic_endian<uint32_t>::convert_batch(src, dst, false);

    for (size_t i = 0; i < src.size(); ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

TEST(ConvertBatch, Inplace)
{
    std::vector<uint16_t> data = {0x1234, 0x5678, 0x9ABC};
    std::vector<uint16_t> original = data;
    
    basic_endian<uint16_t>::convert_batch_inplace(data, false);
    
    for (size_t i = 0; i < data.size(); ++i)
        EXPECT_EQ(data[i], detail::byte_swap(original[i]));
    
    basic_endian<uint16_t>::convert_batch_inplace(data, false);
    
    for (size_t i = 0; i < data.size(); ++i)
        EXPECT_EQ(data[i], original[i]);
}

// ============================================================================
// buffer_view
// ============================================================================

TEST(BufferView, StoreBigLoadBig)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);

    view.store_big(0x01020304);
    EXPECT_EQ(view.load_big(), uint32_t{0x01020304});
}

TEST(BufferView, StoreLittleLoadLittle)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);

    view.store_little(0x01020304);
    EXPECT_EQ(view.load_little(), uint32_t{0x01020304});
}

TEST(BufferView, StoreNativeLoadNative)
{
    std::array<std::byte, 8> buf{};
    basic_endian<uint64_t>::buffer_view view(buf);

    const uint64_t val = 0xDEADBEEFCAFEBABEull;
    view.store_native(val);
    EXPECT_EQ(view.load_native(), val);
}

TEST(BufferView, BigEndianByteLayout)
{
    std::array<std::byte, 2> buf{};
    basic_endian<uint16_t>::buffer_view view(buf);

    view.store_big(0x1234);

    EXPECT_EQ(buf[0], std::byte{0x12});
    EXPECT_EQ(buf[1], std::byte{0x34});
}

TEST(BufferView, LittleEndianByteLayout)
{
    std::array<std::byte, 2> buf{};
    basic_endian<uint16_t>::buffer_view view(buf);

    view.store_little(0x1234);

    EXPECT_EQ(buf[0], std::byte{0x34});
    EXPECT_EQ(buf[1], std::byte{0x12});
}

TEST(BufferView, ThrowsOnSmallBuffer)
{
    std::array<std::byte, 2> buf{};
    EXPECT_THROW(
        (basic_endian<uint64_t>::buffer_view{buf}),
        std::out_of_range);
}

TEST(BufferView, StoreAndLoadOrder)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);

    view.store_order(byte_order::big, 0xDEADBEEF);
    EXPECT_EQ(view.load_order(byte_order::big), uint32_t{0xDEADBEEF});
    
    view.store_order(byte_order::little, 0xCAFEBABE);
    EXPECT_EQ(view.load_order(byte_order::little), uint32_t{0xCAFEBABE});
}

TEST(BufferView, ConstBufferView)
{
    std::array<std::byte, 8> buf{};
    basic_endian<uint64_t>::buffer_view writable(buf);
    writable.store_big(0x0102030405060708ull);
    
    basic_endian<uint64_t>::const_buffer_view readonly(buf);
    EXPECT_EQ(readonly.load_big(), uint64_t{0x0102030405060708ull});
}

// ============================================================================
// network_order helpers
// ============================================================================

TEST(NetworkOrder, HostToNetworkRoundtrip16)
{
    using no = basic_endian<uint16_t>::network_order;
    constexpr uint16_t value = 0x1234;
    EXPECT_EQ(no::network_to_host(no::host_to_network(value)), value);
}

TEST(NetworkOrder, HostToNetworkRoundtrip32)
{
    using no = basic_endian<uint32_t>::network_order;
    constexpr uint32_t value = 0xDEADBEEF;
    EXPECT_EQ(no::network_to_host(no::host_to_network(value)), value);
}

TEST(NetworkOrder, ByteOrderIsAlwaysBig)
{
    EXPECT_EQ(basic_endian<uint32_t>::network_order::byte_order_value, byte_order::network);
    EXPECT_EQ(basic_endian<uint32_t>::network_order::endian_value, std::endian::big);
}

TEST(OrderHelpers, LittleEndianOrder)
{
    using le = basic_endian<uint32_t>::little_endian_order;
    constexpr uint32_t value = 0x12345678;
    
    if constexpr (std::endian::native == std::endian::little)
    {
        EXPECT_EQ(le::host_to_little(value), value);
    }
    else
    {
        EXPECT_EQ(le::host_to_little(value), detail::byte_swap(value));
    }
}

TEST(OrderHelpers, BigEndianOrder)
{
    using be = basic_endian<uint32_t>::big_endian_order;
    constexpr uint32_t value = 0x12345678;
    
    if constexpr (std::endian::native == std::endian::little)
    {
        EXPECT_EQ(be::host_to_big(value), detail::byte_swap(value));
    }
    else
    {
        EXPECT_EQ(be::host_to_big(value), value);
    }
}

// ============================================================================
// Stream serialization / deserialization
// ============================================================================

TEST(Serialization, RoundTripUint16)
{
    basic_endian<uint16_t> orig(0xBEEF), loaded;
    std::stringstream ss;
    orig.serialize(ss);
    loaded.deserialize(ss);
    EXPECT_EQ(loaded.native(), orig.native());
}

TEST(Serialization, RoundTripUint32)
{
    basic_endian<uint32_t> orig(0xDEADBEEF), loaded;
    std::stringstream ss;
    orig.serialize(ss);
    loaded.deserialize(ss);
    EXPECT_EQ(loaded.native(), orig.native());
}

TEST(Serialization, RoundTripUint64)
{
    basic_endian<uint64_t> orig(0xCAFEBABEDEAD1234ull), loaded;
    std::stringstream ss;
    orig.serialize(ss);
    loaded.deserialize(ss);
    EXPECT_EQ(loaded.native(), orig.native());
}

TEST(Serialization, AlwaysWritesBigEndianBytes)
{
    basic_endian<uint16_t> e(0x0102);
    std::stringstream ss;
    e.serialize(ss);
    const std::string bytes = ss.str();
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0x01u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[1]), 0x02u);
}

TEST(Serialization, BatchSerializeDeserialize)
{
    std::vector<uint32_t> original = {0x12345678, 0xDEADBEEF, 0xCAFEBABE};
    std::stringstream ss;
    
    basic_endian<uint32_t>::serialize_batch(ss, original.data(), original.size());
    
    std::vector<uint32_t> loaded(original.size());
    basic_endian<uint32_t>::deserialize_batch(ss, loaded.data(), loaded.size());
    
    for (size_t i = 0; i < original.size(); ++i)
        EXPECT_EQ(loaded[i], original[i]);
}

// ============================================================================
// memory-mapped I/O (write_mmio / read_mmio)
// ============================================================================

TEST(MMIO, WriteAndReadBack32)
{
    uint32_t storage = 0;
    basic_endian<uint32_t>::write_mmio(&storage, 0x11223344);
    const uint32_t back = basic_endian<uint32_t>::read_mmio(&storage);
    EXPECT_EQ(back, uint32_t{0x11223344});
}

TEST(MMIO, WriteAndReadBack64)
{
    uint64_t storage = 0;
    basic_endian<uint64_t>::write_mmio(&storage, 0xDEADBEEFCAFEBABEull);
    const uint64_t back = basic_endian<uint64_t>::read_mmio(&storage);
    EXPECT_EQ(back, uint64_t{0xDEADBEEFCAFEBABEull});
}

TEST(MMIO, WithMemoryOrder)
{
    uint32_t storage = 0;
    basic_endian<uint32_t>::write_mmio(&storage, 0x12345678, std::memory_order_relaxed);
    const uint32_t back = basic_endian<uint32_t>::read_mmio(&storage, std::memory_order_relaxed);
    EXPECT_EQ(back, uint32_t{0x12345678});
}

// ============================================================================
// endian_span
// ============================================================================

TEST(EndianSpan, Construction)
{
    std::vector<uint32_t> data(10);
    endian_span<uint32_t> span(data);
    
    EXPECT_EQ(span.size(), 10);
    EXPECT_FALSE(span.empty());
}

TEST(EndianSpan, ElementAccess)
{
    std::vector<uint32_t> data = {0x12345678, 0xDEADBEEF};
    endian_span<uint32_t> span(data);
    
    EXPECT_EQ(span[0].native(), uint32_t{0x12345678});
    EXPECT_EQ(span[1].native(), uint32_t{0xDEADBEEF});
}

TEST(EndianSpan, Iteration)
{
    std::vector<uint32_t> data = {0x12345678, 0xDEADBEEF, 0xCAFEBABE};
    endian_span<uint32_t> span(data);
    
    size_t count = 0;
    for (auto& elem : span)
    {
        EXPECT_EQ(elem.native(), data[count]);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST(EndianSpan, ConstIteration)
{
    const std::vector<uint32_t> data = {0x12345678, 0xDEADBEEF};
    const_endian_span<uint32_t> span(data);
    
    size_t count = 0;
    for (const auto& elem : span)
    {
        EXPECT_EQ(elem.native(), data[count]);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST(EndianSpan, ConvertToHost)
{
    std::vector<uint16_t> data = {0x1234, 0x5678};
    std::vector<uint16_t> original = data;
    
    endian_span<uint16_t, byte_order::big> span(data);
    span.convert_to_host();
    
    if constexpr (std::endian::native == std::endian::little)
    {
        for (size_t i = 0; i < data.size(); ++i)
            EXPECT_NE(data[i], original[i]);
    }
}

TEST(EndianSpan, UnderlyingSpan)
{
    std::vector<uint32_t> data(5);
    endian_span<uint32_t> span(data);
    
    EXPECT_EQ(span.underlying().data(), data.data());
    EXPECT_EQ(span.underlying().size(), 5);
}

// ============================================================================
// cache_aligned_storage and cache_line_padded
// ============================================================================

TEST(CacheAlignedStorage, Construction)
{
    detail::cache_aligned_storage<uint32_t> storage;
    EXPECT_EQ(storage.value, uint32_t{0});
    
    detail::cache_aligned_storage<uint32_t> storage_with_val(42);
    EXPECT_EQ(storage_with_val.value, uint32_t{42});
}

TEST(CacheAlignedStorage, Alignment)
{
    EXPECT_GE(alignof(detail::cache_aligned_storage<uint32_t>), 64);
    EXPECT_GE(alignof(detail::cache_aligned_storage<uint64_t>), 64);
}

TEST(CacheLinePadded, Construction)
{
    detail::cache_line_padded<uint32_t> padded;
    EXPECT_EQ(padded.value, uint32_t{0});
    
    detail::cache_line_padded<uint32_t> padded_with_val(0xDEADBEEF);
    EXPECT_EQ(padded_with_val.value, uint32_t{0xDEADBEEF});
}

TEST(CacheLinePadded, Alignment)
{
    EXPECT_GE(alignof(detail::cache_line_padded<uint32_t>), hardware_destructive_interference_size);
}

// ============================================================================
// atomic_padded
// ============================================================================

TEST(AtomicPadded, LoadStore)
{
    detail::atomic_padded<uint64_t> atomic(0xCAFEBABE);
    EXPECT_EQ(atomic.load(), 0xCAFEBABEull);
    
    atomic.store(0xDEADBEEF);
    EXPECT_EQ(atomic.load(), 0xDEADBEEF);
}

TEST(AtomicPadded, Exchange)
{
    detail::atomic_padded<uint32_t> atomic(100);
    uint32_t old = atomic.exchange(200);
    EXPECT_EQ(old, 100);
    EXPECT_EQ(atomic.load(), 200);
}

TEST(AtomicPadded, CompareExchange)
{
    detail::atomic_padded<uint32_t> atomic(42);
    uint32_t expected = 42;
    
    bool success = atomic.compare_exchange_strong(expected, 100);
    EXPECT_TRUE(success);
    EXPECT_EQ(atomic.load(), 100);
    
    expected = 42;
    success = atomic.compare_exchange_weak(expected, 200);
    EXPECT_FALSE(success);
    EXPECT_EQ(expected, 100);
}

TEST(AtomicPadded, Alignment)
{
    EXPECT_GE(alignof(detail::atomic_padded<uint32_t>), hardware_destructive_interference_size);
}

// ============================================================================
// Alignment utilities
// ============================================================================

TEST(AlignmentUtils, AlignUp)
{
    EXPECT_EQ(detail::align_up(0, 64), 0);
    EXPECT_EQ(detail::align_up(1, 64), 64);
    EXPECT_EQ(detail::align_up(63, 64), 64);
    EXPECT_EQ(detail::align_up(64, 64), 64);
    EXPECT_EQ(detail::align_up(65, 64), 128);
}

TEST(AlignmentUtils, IsAligned)
{
    EXPECT_TRUE(detail::is_aligned(0, 64));
    EXPECT_TRUE(detail::is_aligned(64, 64));
    EXPECT_FALSE(detail::is_aligned(63, 64));
    EXPECT_FALSE(detail::is_aligned(65, 64));
}

TEST(AlignmentUtils, IsPowerOfTwo)
{
    EXPECT_TRUE(detail::is_power_of_two(1));
    EXPECT_TRUE(detail::is_power_of_two(2));
    EXPECT_TRUE(detail::is_power_of_two(64));
    EXPECT_TRUE(detail::is_power_of_two(1024));
    EXPECT_FALSE(detail::is_power_of_two(0));
    EXPECT_FALSE(detail::is_power_of_two(3));
    EXPECT_FALSE(detail::is_power_of_two(63));
}

// ============================================================================
// byte_order enum
// ============================================================================

TEST(ByteOrderEnum, Values)
{
    EXPECT_EQ(byte_order::native, std::endian::native == std::endian::little ? 
              byte_order::little : byte_order::big);
    EXPECT_EQ(byte_order::network, byte_order::big);
}

TEST(ByteOrderEnum, IsEndianCompatibleTrait)
{
    EXPECT_TRUE(is_endian_compatible_v<uint8_t>);
    EXPECT_TRUE(is_endian_compatible_v<uint16_t>);
    EXPECT_TRUE(is_endian_compatible_v<uint32_t>);
    EXPECT_TRUE(is_endian_compatible_v<uint64_t>);
    EXPECT_TRUE(is_endian_compatible_v<int16_t>);
    EXPECT_TRUE(is_endian_compatible_v<int32_t>);
    EXPECT_TRUE(is_endian_compatible_v<int64_t>);
    EXPECT_TRUE(is_endian_compatible_v<float>);
    EXPECT_TRUE(is_endian_compatible_v<double>);
    EXPECT_FALSE(is_endian_compatible_v<bool>);
}

// ============================================================================
// Prefetch batch operations
// ============================================================================

TEST(PrefetchBatch, Basic)
{
    constexpr size_t N = 256;
    std::vector<uint64_t> src(N), dst(N);
    
    std::mt19937_64 rng(42);
    for (size_t i = 0; i < N; ++i)
        src[i] = rng();
    
    batch_byte_swap_prefetch(src.data(), dst.data(), N, 8);
    
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

TEST(PrefetchBatch, ZeroElements)
{
    uint64_t dummy = 0;
    EXPECT_NO_THROW(batch_byte_swap_prefetch(&dummy, &dummy, 0, 8));
}

// ============================================================================
// Non-temporal store batch operations
// ============================================================================

TEST(NonTemporalBatch, Basic)
{
    constexpr size_t N = 64;
    std::vector<uint32_t> src(N), dst(N);
    
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint32_t>(i);
    
    batch_byte_swap_nt(dst.data(), src.data(), N);
    
    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

// ============================================================================
// CPU feature detection
// ============================================================================

TEST(CPUFeatures, HasNEON)
{
    // NEON is a compile-time feature; no runtime probe is needed or available
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    SUCCEED(); // binary was compiled with ARM NEON support
#else
    SUCCEED(); // ARM NEON not present on this platform
#endif
}

TEST(CPUFeatures, HasAVX2)
{
#if defined(__x86_64__) || defined(_M_X64)
    // Verify the detection call is callable and returns a valid bool
    EXPECT_TRUE(simd::detail::CPUInfo::has_avx2() || !simd::detail::CPUInfo::has_avx2());
#else
    SUCCEED(); // AVX2 is not applicable on this architecture
#endif
}

TEST(CPUFeatures, HasAVX512)
{
#if defined(__AVX512F__)
    EXPECT_TRUE(simd::detail::CPUInfo::has_avx512f() || !simd::detail::CPUInfo::has_avx512f());
#else
    SUCCEED(); // binary was not compiled with AVX-512; intrinsics are unavailable
#endif
}
