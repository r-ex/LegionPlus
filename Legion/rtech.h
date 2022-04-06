#pragma once
#include <cstdint>
#include "intrin.h"

#define PAK_HEADER_SIZE   0x80
#define PAK_PARAM_SIZE    0xB0
#define DCMP_BUF_SIZE 0x400000

struct __declspec(align(8)) rpak_decomp_state
{
  uint64_t input_buf;
  uint64_t out;
  uint64_t mask;
  uint64_t out_mask;
  uint64_t file_len_total;
  uint64_t decompressed_size;
  uint64_t inv_mask_in;
  uint64_t inv_mask_out;
  unsigned int header_skip_bytes_bs;
  uint32_t dword44;
  uint64_t input_byte_pos;
  uint64_t decompressed_position;
  uint64_t len_needed;
  uint64_t byte;
  uint32_t byte_bit_offset;
  uint32_t dword6C;
  uint64_t qword70;
  uint64_t stream_compressed_size;
  uint64_t stream_decompressed_size;
};
static_assert(sizeof(rpak_decomp_state) == 0x88);

namespace
{
	/*unk_141313180*/
	// LUT_Pakfile_Decompress_0 redacted now, split LUT into multiple parts.
	std::array<int8_t, 512> LUT_0
	{
		4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 11, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 247, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 243, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 14, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 9, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 241, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 13, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 247, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 242, 4, 254, 252, 8, 4, 239, 17, 249, 4, 253, 252, 7, 4, 5, 255, 244, 4, 254, 252, 16, 4, 239, 17, 246, 4, 253, 252, 251, 4, 6, 255, 15, 4, 254, 252, 8, 4, 239, 17, 248, 4, 253, 252, 12, 4, 5, 255, 10, 4, 254, 252, 16, 4, 239, 17, 245, 4, 253, 252, 250, 4, 6, 255, 240, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 12, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 9, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 14, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 11, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 10, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 16, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 12, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 9, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 15, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 17, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 13, 4, 5, 4, 6, 4, 5, 4, 7, 4, 5, 4, 6, 4, 5, 4, 10, 4, 5, 4, 6, 4, 5, 4, 8, 4, 5, 4, 6, 4, 5, 4, 255
	};
	std::array<uint8_t, 512> LUT_200
	{
		2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 7, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 7, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 6, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 2, 4, 3, 5, 2, 4, 4, 6, 2, 4, 3, 6, 2, 5, 4, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 8
	};
	std::array<uint8_t, 0x40> LUT_400
	{
		0, 8, 0, 4, 0, 8, 0, 6, 0, 8, 0, 1, 0, 8, 0, 11, 0, 8, 0, 12, 0, 8, 0, 9, 0, 8, 0, 3, 0, 8, 0, 14, 0, 8, 0, 4, 0, 8, 0, 7, 0, 8, 0, 2, 0, 8, 0, 13, 0, 8, 0, 12, 0, 8, 0, 10, 0, 8, 0, 5, 0, 8, 0, 15
	};
	std::array<uint8_t, 0x40> LUT_440
	{
		1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 5, 1, 2, 1, 6, 1, 2, 1, 6, 1, 2, 1, 6
	};
	std::array<uint32_t, 16> LUT_480
	{
		74, 106, 138, 170, 202, 234, 266, 298, 330, 362, 394, 426, 938, 1450, 9642, 140714
	};
	std::array<uint8_t, 16> LUT_4C0
	{
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 9, 9, 13, 17, 21
	};
	std::array<uint8_t, 8> LUT_4D0
	{
		0, 0, 2, 4, 6, 8, 10, 42
	};
	std::array<uint8_t, 8> LUT_4D8
	{
		0, 1, 1, 1, 1, 1, 5, 5
	};
	std::array<uint8_t, 32> LUT_4E0
	{
		17, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	/*off_5384508116*/
	std::array<std::uint8_t, 4> LUT_Dynamic_Track_0
	{
		0, 2, 4, 0
	};

	/*dword_140F13EE0*/
	std::array<float, 4> LUT_Dynamic_Track_1
	{
		0.0, 3.0, 15.0, 0.0
	};

	/*byte_1410B0CE0*/
	std::array<std::uint8_t, 16> LUT_Snowflake_0
	{
		0, 3, 7, 15, 35, 63, 82, 71, 66, 69, 0, 0, 0, 0, 0, 0
	};

	std::array<std::uint64_t, 4> LUT_Snowflake_1
	{
		4611756117654110208,
		4611756116592754688,
		4611686559597395968,
		53199311768322048,
	};
};

class RTech
{
public:
	uint64_t __fastcall DecompressPakfileInit(rpak_decomp_state* state, uint8_t* file_buffer, int64_t file_size, int64_t off_no_header, int64_t header_size);
	uint8_t __fastcall DecompressPakFile(rpak_decomp_state* state, uint64_t file_size, uint64_t buffer_size);
	int64_t DecompressSnowflakeInit(int64_t param_buf, int64_t data_buf, uint64_t data_size);
	bool DecompressSnowflake(int64_t param_buffer, uint64_t data_size, uint64_t buffer_size);
	float* __fastcall DecompressDynamicTrack(int frame_count, uint8_t* in_translation_buffer, float translation_scale, float* out_translation_buffer, float* time_scale/*'time_scale' might nit be correct*/);
	void __fastcall DecompressConvertRotation(const __m128i* rotation_buffer, float* result_buffer);
	uint64_t __fastcall StringToGuid(const char* asset_name);
	float __fastcall FrameToEulerTranslation(uint8_t* translation_buffer, int frame_count, float translation_scale);
	void UnswizzleBlock(uint32_t x, uint32_t y, uint32_t a3, uint32_t a4, uint32_t& x1, uint32_t& y2);

	// UNK functions
	__int64 sub_7FF7FC23C680(__int64 a1, unsigned __int64* a2, unsigned __int8 a3, unsigned int a4, void* a5);
	int64_t sub_7FF7FC23B960(int64_t param_buffer, uint32_t unk1);
	int64_t sub_7FF7FC23C880(int64_t param_buffer, uint8_t a2, int64_t a3);
	__int64 sub_7FF7FC23CD20(unsigned __int8* param_buffer, unsigned int a2);
};

///////////////////////////////////////////////////////////////////////////////
extern RTech* g_pRtech;
