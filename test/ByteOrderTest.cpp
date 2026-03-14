#include <gtest/gtest.h>
#include "byteorder/byteorder.h"

#include <bit>
#include <cstdint>

using namespace endian;

// ============================================================================
// detail::byte_swap — scalar, all integer widths
// ============================================================================

TEST(ByteSwap, Width8_IsIdentity)
{
    EXPECT_EQ(detail::byte_swap(uint8_t{0xAB}), uint8_t{0xAB});
    EXPECT_EQ(detail::byte_swap(uint8_t{0x00}), uint8_t{0x00});
    EXPECT_EQ(detail::byte_swap(uint8_t{0xFF}), uint8_t{0xFF});
}

TEST(ByteSwap, Width16)
{
    EXPECT_EQ(detail::byte_swap(uint16_t{0x1234}), uint16_t{0x3412});
    EXPECT_EQ(detail::byte_swap(uint16_t{0x0000}), uint16_t{0x0000});
    EXPECT_EQ(detail::byte_swap(uint16_t{0xFFFF}), uint16_t{0xFFFF});
    EXPECT_EQ(detail::byte_swap(uint16_t{0x0102}), uint16_t{0x0201});
}

TEST(ByteSwap, Width32)
{
    EXPECT_EQ(detail::byte_swap(uint32_t{0x12345678}), uint32_t{0x78563412});
    EXPECT_EQ(detail::byte_swap(uint32_t{0xDEADBEEF}), uint32_t{0xEFBEADDE});
    EXPECT_EQ(detail::byte_swap(uint32_t{0x00000000}), uint32_t{0x00000000});
    EXPECT_EQ(detail::byte_swap(uint32_t{0xFFFFFFFF}), uint32_t{0xFFFFFFFF});
}

TEST(ByteSwap, Width64)
{
    EXPECT_EQ(detail::byte_swap(uint64_t{0x0102030405060708ull}),
              uint64_t{0x0807060504030201ull});
    EXPECT_EQ(detail::byte_swap(uint64_t{0xDEADBEEFCAFEBABEull}),
              uint64_t{0xBEBAFECAEFBEADDEull});
    EXPECT_EQ(detail::byte_swap(uint64_t{0x0ull}), uint64_t{0x0ull});
    EXPECT_EQ(detail::byte_swap(uint64_t{~0ull}),  uint64_t{~0ull});
}

TEST(ByteSwap, RoundTrip)
{
    constexpr uint16_t v16 = 0xA1B2;
    constexpr uint32_t v32 = 0xA1B2C3D4;
    constexpr uint64_t v64 = 0xA1B2C3D4E5F60708ull;

    EXPECT_EQ(detail::byte_swap(detail::byte_swap(v16)), v16);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(v32)), v32);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(v64)), v64);
}

TEST(ByteSwap, SignedTypes)
{
    // byte_swap treats the bit-pattern — sign doesn't matter
    const int16_t  neg16 = static_cast<int16_t>(0xFF00);
    const int32_t  neg32 = static_cast<int32_t>(0xFF000000);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(neg16)), neg16);
    EXPECT_EQ(detail::byte_swap(detail::byte_swap(neg32)), neg32);
}

TEST(ByteSwap, ConstexprEvaluation)
{
    static_assert(detail::byte_swap(uint32_t{0x01020304}) == uint32_t{0x04030201});
    static_assert(detail::byte_swap(uint16_t{0xABCD})     == uint16_t{0xCDAB});
}

// ============================================================================
// basic_endian — construction and native accessor
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

// ============================================================================
// basic_endian — little() and big() accessors
// ============================================================================

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

// ============================================================================
// basic_endian — store_little / store_big
// ============================================================================

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

    auto le_val = 0xCAFEBABEull_le;
    auto be_val = 0xCAFEBABEull_be;

    EXPECT_EQ(le_val.native(), uint64_t{0xCAFEBABE});
    EXPECT_EQ(be_val.native(), uint64_t{0xCAFEBABE});
    EXPECT_EQ(le_val.little(), be_val.little());
}
