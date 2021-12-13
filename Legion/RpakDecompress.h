#pragma once
#include <cstdint>
#include <emmintrin.h>

struct RpakDecompressState
{
	uint64_t DecompressedSize;
	uint64_t DecompressedBuffer;
	uint64_t DecompressOffset;
};

// stub
void RpakUnswizzleBlock(uint32_t x, uint32_t y, uint32_t a3, uint32_t a4, uint32_t x1, uint32_t y2);