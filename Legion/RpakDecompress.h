#pragma once
#include <cstdint>
#include <emmintrin.h>

// stub

struct RpakDecompressState
{
	uint64_t DecompressedSize;
	uint64_t DecompressedBuffer;
	uint64_t DecompressOffset;
};

//void RpakDecompressInit(int64_t param_buf, uint8_t* file_buf, uint64_t file_size, uint64_t offset_no_header, uint64_t header_size);
//void RpakDecompress(int64_t* params, uint64_t file_size, uint64_t buffer_size);

void RpakDecompressDynamicTrack(uint32_t frame, uint8_t* data, float idk1, float* idk2, float* idk3);
void RpakDecompressConvertRotation(const __m128i* eulerResult, float* result);

void RpakUnswizzleBlock(uint32_t x, uint32_t y, uint32_t a3, uint32_t a4, uint32_t x1, uint32_t y2);