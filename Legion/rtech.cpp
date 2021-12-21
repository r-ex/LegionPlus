#include "pch.h"
#include <cmath>
#include "rtech.h"
#include "basetypes.h"

/******************************************************************************
-------------------------------------------------------------------------------
File   : rtech.cpp
Date   : 18:07:2021
Author : Kawe Mazidjatari
Purpose: Implements the 'rtech_game' core utilities
-------------------------------------------------------------------------------
History:
- 18:07:2021 | 13:02 : Created by Kawe Mazidjatari
- 10:09:2021 | 18:22 : Implement 'StringToGuid' method
- 12:11:2021 | 14:41 : Add decompression method to ConCommand callback

******************************************************************************/

//-----------------------------------------------------------------------------
// Purpose: calculate 'decompressed' size and commit parameters
//-----------------------------------------------------------------------------
uint64_t __fastcall RTech::DecompressPakfileInit(rpak_decomp_state* state, uint8_t* file_buffer, int64_t file_size, int64_t off_no_header, int64_t header_size)
{
	__int64 input_byte_pos_init; // r9
	unsigned __int64 byte_init; // r11
	int decompressed_size_bits; // ecx
	__int64 byte_1_low; // rdi
	unsigned __int64 input_byte_pos_1; // r10
	unsigned int bit_pos_final; // ebp
	unsigned __int64 byte_1; // rdi
	unsigned int brih_bits; // er11
	unsigned __int64 inv_mask_in; // r8
	unsigned __int64 byte_final_full; // rbx
	unsigned __int64 bit_pos_final_1; // rax
	int byte_bit_offset_final; // ebp
	unsigned __int64 input_byte_pos_final; // r10
	unsigned __int64 byte_final; // rbx
	unsigned int brih_bytes; // er11
	__int64 byte_tmp; // rdx
	__int64 stream_len_needed; // r14
	__int64 result; // rax
	unsigned __int64 inv_mask_out; // r8
	__int64 qw70; // rcx
	__int64 stream_compressed_size_new; // rdx

	const uintptr_t mask = UINT64_MAX;
	const auto file_buf = uintptr_t(file_buffer);

	state->input_buf = file_buf;
	state->out = 0i64;
	state->out_mask = 0i64;
	state->dword44 = 0;                           // y r u gay
	state->file_len_total = file_size + off_no_header;
	state->mask = mask;
	input_byte_pos_init = off_no_header + header_size + 8;
	byte_init = *(uint64_t*)((mask & (off_no_header + header_size)) + file_buf);
	state->decompressed_position = header_size;
	decompressed_size_bits = byte_init & 0x3F;
	byte_init >>= 6;
	state->input_byte_pos = input_byte_pos_init;
	state->decompressed_size = byte_init & ((1i64 << decompressed_size_bits) - 1) | (1i64 << decompressed_size_bits);
	byte_1_low = *(uint64_t*)((mask & input_byte_pos_init) + file_buf) << (64
		- ((unsigned __int8)decompressed_size_bits
			+ 6));
	input_byte_pos_1 = input_byte_pos_init + ((unsigned __int64)(unsigned int)(decompressed_size_bits + 6) >> 3);
	state->input_byte_pos = input_byte_pos_1;
	bit_pos_final = ((decompressed_size_bits + 6) & 7) + 13;
	byte_1 = (0xFFFFFFFFFFFFFFFFui64 >> ((decompressed_size_bits + 6) & 7)) & ((byte_init >> decompressed_size_bits) | byte_1_low);
	brih_bits = (((uint8_t)byte_1 - 1) & 0x3F) + 1;
	inv_mask_in = 0xFFFFFFFFFFFFFFFFui64 >> (64 - (unsigned __int8)brih_bits);
	state->inv_mask_in = inv_mask_in;
	state->inv_mask_out = 0xFFFFFFFFFFFFFFFFui64 >> (63 - (((byte_1 >> 6) - 1) & 0x3F));
	byte_final_full = (byte_1 >> 13) | (*(uint64_t*)((mask & input_byte_pos_1) + file_buf) << (64
		- (unsigned __int8)bit_pos_final));
	bit_pos_final_1 = bit_pos_final;
	byte_bit_offset_final = bit_pos_final & 7;
	input_byte_pos_final = (bit_pos_final_1 >> 3) + input_byte_pos_1;
	byte_final = (0xFFFFFFFFFFFFFFFFui64 >> byte_bit_offset_final) & byte_final_full;
	state->input_byte_pos = input_byte_pos_final;
	if (inv_mask_in == -1i64)
	{
		state->header_skip_bytes_bs = 0;
		stream_len_needed = file_size;
	}
	else
	{
		brih_bytes = brih_bits >> 3;
		state->header_skip_bytes_bs = brih_bytes + 1;
		byte_tmp = *(uint64_t*)((mask & input_byte_pos_final) + file_buf);
		state->input_byte_pos = input_byte_pos_final + brih_bytes + 1;
		stream_len_needed = byte_tmp & ((1i64 << (8 * ((unsigned __int8)brih_bytes + 1))) - 1);
	}
	result = state->decompressed_size;
	inv_mask_out = state->inv_mask_out;
	qw70 = off_no_header + state->inv_mask_in - 6i64;
	state->len_needed = stream_len_needed + off_no_header;
	state->qword70 = qw70;
	state->byte = byte_final;
	state->byte_bit_offset = byte_bit_offset_final;
	state->dword6C = 0;
	state->stream_compressed_size = stream_len_needed + off_no_header;
	state->stream_decompressed_size = result;
	if (result - 1 > inv_mask_out)
	{
		stream_compressed_size_new = stream_len_needed + off_no_header - state->header_skip_bytes_bs;
		state->stream_decompressed_size = inv_mask_out + 1;
		state->stream_compressed_size = stream_compressed_size_new;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: decompress input pakfile
//-----------------------------------------------------------------------------
uint8_t __fastcall RTech::DecompressPakFile(int64_t* param_buffer, uint64_t data_size, uint64_t buffer_size)
{
	char   result; // al
	int64_t    v5; // r15
	int64_t    v6; // r11
	uint32_t   v7; // ebp
	uint64_t   v8; // rsi
	uint64_t   v9; // rdi
	uint64_t  v10; // r12
	int64_t   v11; // r13
	uint32_t  v12; // ecx
	uint64_t  v13; // rsi
	uint64_t    i; // rax
	uint64_t  v15; // r8
	int64_t   v16; // r9
	int       v17; // ecx
	uint64_t  v18; // rax
	uint64_t  v19; // rsi
	int64_t   v20; // r14
	int       v21; // ecx
	uint64_t  v22; // r11
	int       v23; // edx
	int64_t   v24; // rax
	int       v25; // er8
	uint32_t  v26; // er13
	int64_t   v27; // r10
	int64_t   v28; // rax
	uint64_t* v29; // r10
	uint64_t  v30; // r9
	uint64_t  v31; // r10
	uint64_t  v32; // r8
	uint64_t  v33; // rax
	uint64_t  v34; // rax
	uint64_t  v35; // rax
	uint64_t  v36; // rcx
	uint64_t  v37; // rdx
	uint64_t  v38; // r14
	uint64_t  v39; // r11
	char      v40; // cl
	uint64_t  v41; // rsi
	uint64_t  v42; // rcx
	uint64_t  v43; // r8
	int       v44; // er11
	uint8_t   v45; // r9
	uint64_t  v46; // rcx
	uint64_t  v47; // rcx
	uint64_t  v48; // r9
	uint64_t    l; // r8
	uint32_t  v50; // er9
	uint64_t  v51; // r8
	uint64_t  v52; // rdx
	uint64_t    k; // r8
	char*     v54; // r10
	uint64_t  v55; // rdx
	uint32_t  v56; // er14
	uint64_t* v57; // rdx
	uint64_t* v58; // r8
	char      v59; // al
	uint64_t  v60; // rsi
	uint64_t  v61; // rax
	uint64_t  v62; // r9
	int       v63; // er10
	uint8_t   v64; // cl
	uint64_t  v65; // rax
	uint32_t  v66; // er14
	uint32_t    j; // ecx
	uint64_t  v68; // rax
	uint64_t  v69; // rcx
	uint64_t  v70; // [rsp+0h] [rbp-58h]
	int       v71; // [rsp+60h] [rbp+8h]
	uint64_t  v74; // [rsp+78h] [rbp+20h]
	
	if (data_size < param_buffer[11])
	{
		return 0;
	}
	v5 = param_buffer[10];
	if (buffer_size < param_buffer[7] + (v5 & (uint64_t)~param_buffer[7]) + 1 && buffer_size < param_buffer[5])
	{
		return 0;
	}
	v6 = param_buffer[1];
	v7 = *((uint32_t*)param_buffer + 26);
	v8 = param_buffer[12];
	v9 = param_buffer[9];
	v10 = param_buffer[14];
	v11 = *param_buffer;
	if (param_buffer[15] < v10)
	{
		v10 = param_buffer[15];
	}
	v12 = *((uint32_t*)param_buffer + 27);
	v74 = v11;
	v70 = v6;
	v71 = v12;
	if (!v7)
	{
		goto LABEL_11;
	}
	v13 = (*(uint64_t*)((v9 & param_buffer[2]) + v11) << (64 - (uint8_t)v7)) | v8;
	for (i = v7; ; i = v7)
	{
		v7 &= 7u;
		v9 += i >> 3;
		v12 = v71;
		v8 = (0xFFFFFFFFFFFFFFFFui64 >> v7) & v13;
	LABEL_11:
		v15 = (uint64_t)v12 << 8;
		v16 = v12;
		v17 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + (uint8_t)v8 + v15 + 512);
		v18 = (uint8_t)v8 + v15;
		v7 += v17;
		v19 = v8 >> v17;
		v20 = (uint32_t)*((char*)&LUT_Pakfile_Decompress_0 + v18);
		if (*((char*)&LUT_Pakfile_Decompress_0 + v18) < 0)
		{
			v56 = -(int)v20;
			v57 = (uint64_t*)(v11 + (v9 & param_buffer[2]));
			v71 = 1;
			v58 = (uint64_t*)(v6 + (v5 & param_buffer[3]));
			if (v56 == *((uint8_t*)&LUT_Pakfile_Decompress_0 + v16 + 1248))
			{
				if ((~v9 & param_buffer[6]) < 0xF || (param_buffer[7] & (uint64_t)~v5) < 0xF || (uint64_t)(param_buffer[5] - v5) < 0x10)
				{
					v56 = 1;
				}
				v59 = v19;
				v60 = v19 >> 3;
				v61 = v59 & 7;
				v62 = v60;
				if (v61)
				{
					v63 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + v61 + 1232);
					v64 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + v61 + 1240);
				}
				else
				{
					v62 = v60 >> 4;
					v65 = v60 & 0xF;
					v7 += 4;
					v63 = *((uint32_t*)&LUT_Pakfile_Decompress_0 + v65 + 288);
					v64 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + v65 + 1216);
				}
				v7 += v64 + 3;
				v19 = v62 >> v64;
				v66 = v63 + (v62 & ((1 << v64) - 1)) + v56;
				for (j = v66 >> 3; j; --j)
				{
					v68 = *v57++;
					*v58++ = v68;
				}
				if ((v66 & 4) != 0)
				{
					*(uint32_t*)v58 = *(uint32_t*)v57;
					v58 = (uint64_t*)((char*)v58 + 4);
					v57 = (uint64_t*)((char*)v57 + 4);
				}
				if ((v66 & 2) != 0)
				{
					*(uint16_t*)v58 = *(uint16_t*)v57;
					v58 = (uint64_t*)((char*)v58 + 2);
					v57 = (uint64_t*)((char*)v57 + 2);
				}
				if ((v66 & 1) != 0)
				{
					*(uint8_t*)v58 = *(uint8_t*)v57;
				}
				v9 += v66;
				v5 += v66;
			}
			else
			{
				*v58 = *v57;
				v58[1] = v57[1];
				v9 += v56;
				v5 += v56;
			}
		}
		else
		{
			v21 = v19 & 0xF;
			v71 = 0;
			v22 = ((uint64_t)(uint32_t)v19 >> (((uint32_t)(v21 - 31) >> 3) & 6)) & 0x3F;
			v23 = 1 << (v21 + ((v19 >> 4) & ((24 * (((uint32_t)(v21 - 31) >> 3) & 2)) >> 4)));
			v7 += (((uint32_t)(v21 - 31) >> 3) & 6) + *((uint8_t*)&LUT_Pakfile_Decompress_0 + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((uint32_t)(v21 - 31) >> 3) & 2)) >> 4));
			v24 = param_buffer[3];
			v25 = 16 * (v23 + ((v23 - 1) & (v19 >> ((((uint32_t)(v21 - 31) >> 3) & 6) + *((uint8_t*)&LUT_Pakfile_Decompress_0 + v22 + 1088)))));
			v19 >>= (((uint32_t)(v21 - 31) >> 3) & 6) + *((uint8_t*)&LUT_Pakfile_Decompress_0 + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((uint32_t)(v21 - 31) >> 3) & 2)) >> 4));
			v26 = v25 + *((uint8_t*)&LUT_Pakfile_Decompress_0 + v22 + 1024) - 16;
			v27 = v24 & (v5 - v26);
			v28 = v70 + (v5 & v24);
			v29 = (uint64_t*)(v70 + v27);
			if ((uint32_t)v20 == 17)
			{
				v40 = v19;
				v41 = v19 >> 3;
				v42 = v40 & 7;
				v43 = v41;
				if (v42)
				{
					v44 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + v42 + 1232);
					v45 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + v42 + 1240);
				}
				else
				{
					v7 += 4;
					v46 = v41 & 0xF;
					v43 = v41 >> 4;
					v44 = *((uint32_t*)&LUT_Pakfile_Decompress_0 + v46 + 288);
					v45 = *((uint8_t*)&LUT_Pakfile_Decompress_0 + v46 + 1216);
					if (v74 && v7 + v45 >= 0x3D)
					{
						v47 = v9++ & param_buffer[2];
						v43 |= (uint64_t)*(uint8_t*)(v47 + v74) << (61 - (uint8_t)v7);
						v7 -= 8;
					}
				}
				v7 += v45 + 3;
				v19 = v43 >> v45;
				v48 = ((uint32_t)v43 & ((1 << v45) - 1)) + v44 + 17;
				v5 += v48;
				if (v26 < 8)
				{
					v50 = v48 - 13;
					v5 -= 13i64;
					if (v26 == 1)
					{
						v51 = *(uint8_t*)v29;
						v52 = 0i64;
						for (k = 0x101010101010101i64 * v51; (uint32_t)v52 < v50; v52 = (uint32_t)(v52 + 8))
						{
							*(uint64_t*)(v52 + v28) = k;
						}
					}
					else
					{
						if (v50)
						{
							v54 = (char*)v29 - v28;
							v55 = v50;
							do
							{
								*(uint8_t*)v28 = v54[v28];
								++v28;
								--v55;
							} while (v55);
						}
					}
				}
				else
				{
					for (l = 0i64; (uint32_t)l < (uint32_t)v48; l = (uint32_t)(l + 8))
					{
						*(uint64_t*)(l + v28) = *(uint64_t*)((char*)v29 + l);
					}
				}
			}
			else
			{
				v5 += v20;
				*(uint64_t*)v28 = *v29;
				*(uint64_t*)(v28 + 8) = v29[1];
			}
			v11 = v74;
		}
		if (v9 >= v10)
		{
			break;
		}
	LABEL_29:
		v6 = v70;
		v13 = (*(uint64_t*)((v9 & param_buffer[2]) + v11) << (64 - (uint8_t)v7)) | v19;
	}
	if (v5 != param_buffer[16])
	{
		goto LABEL_25;
	}
	v30 = param_buffer[5];
	if (v5 == v30)
	{
		result = 1;
		goto LABEL_69;
	}
	v31 = param_buffer[6];
	v32 = *((uint32_t*)param_buffer + 16);
	v33 = v31 & -(int64_t)v9;
	v19 >>= 1;
	++v7;
	if (v32 > v33)
	{
		v9 += v33;
		v34 = param_buffer[14];
		if (v9 > v34)
		{
			param_buffer[14] = v31 + v34 + 1;
		}
	}
	v35 = v9 & param_buffer[2];
	v9 += v32;
	v36 = v5 + param_buffer[7] + 1;
	v37 = *(uint32_t*)(v35 + v11) & ((1i64 << (8 * (uint8_t)v32)) - 1);
	v38 = v37 + param_buffer[11];
	v39 = v37 + param_buffer[15];
	param_buffer[11] = v38;
	param_buffer[15] = v39;
	if (v36 >= v30)
	{
		v36 = v30;
		param_buffer[15] = v32 + v39;
	}
	param_buffer[16] = v36;
	if (data_size >= v38 && buffer_size >= v36)
	{
	LABEL_25:
		v10 = param_buffer[14];
		if (v9 >= v10)
		{
			v9 = ~param_buffer[6] & (v9 + 7);
			v10 += param_buffer[6] + 1;
			param_buffer[14] = v10;
		}
		if (param_buffer[15] < v10)
		{
			v10 = param_buffer[15];
		}
		goto LABEL_29;
	}
	v69 = param_buffer[14];
	if (v9 >= v69)
	{
		v9 = ~v31 & (v9 + 7);
		param_buffer[14] = v69 + v31 + 1;
	}
	*((uint32_t*)param_buffer + 27) = v71;
	result = 0;
	param_buffer[12] = v19;
	*((uint32_t*)param_buffer + 26) = v7;
LABEL_69:
	param_buffer[10] = v5;
	param_buffer[9] = v9;
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: calculate 'decompressed' size and commit parameters
//-----------------------------------------------------------------------------
int64_t RTech::DecompressSnowflakeInit(int64_t param_buffer, int64_t data_buffer, uint64_t buffer_size)
{
	return 0; // return gaming for now

	// dmp location: 49 8B 0F 48 89 9E ? ? ? ? 
	// retail location: sig to containing function: (+0x11) E8 ? ? ? ? 4C 8B 53 38 44 8D 47 10

	auto sub_140A44040 = [](int64_t param_buffer, uint32_t unk1) -> int64_t
	{
		// dmp location: sig: 8B 49 2C 
		// retail location: direct reference: [actual address in first opcode] E8 ? ? ? ? 8D 4B 09 
		uint32_t v4; // ecx
		int64_t v5; // r8
		char v6; // bl
		int64_t v7; // rbp
		uint32_t v8; // er11
		int64_t v9; // rsi
		int64_t v10; // rdi
		uint64_t v11; // rdx
		uint32_t v12; // er8
		int v13; // er8
		int64_t result; // rax
		uint32_t v15; // edx

		v4 = *(uint32_t*)(param_buffer + 44);
		if (v4 >= unk1)
		{
			v15 = *(uint32_t*)(param_buffer + 40);
			*(uint32_t*)(param_buffer + 44) = v4 - unk1;
			*(uint32_t*)(param_buffer + 40) = v15 >> unk1;
			result = v15 & ((1 << unk1) - 1);
		}
		else
		{
			v5 = *(uint64_t*)(param_buffer + 149184);
			v6 = v4 + 32;
			v7 = *(uint64_t*)(param_buffer + 149168);
			v8 = v4 + 32 - unk1;
			v9 = v5 + 4;
			v10 = *(uint64_t*)(param_buffer + 149192);
			v11 = *(uint32_t*)(param_buffer + 40) | ((uint64_t)*(uint32_t*)((v5 & v10) + v7) << v4);
			*(uint64_t*)(param_buffer + 149184) = v5 + 4;
			*(uint32_t*)(param_buffer + 44) = v4 + 32;
			if (v4 + 32 >= unk1)
			{
				v13 = v11 >> unk1;
			}
			else
			{
				v12 = *(uint32_t*)((v9 & v10) + v7);
				*(uint64_t*)(param_buffer + 149184) = v9 + 4;
				v11 |= (uint64_t)v12 << v6;
				v13 = v12 >> (unk1 - v6);
				v8 += 32;
			}
			*(uint32_t*)(param_buffer + 44) = v8;
			result = v11 & ((1i64 << unk1) - 1);
			*(uint32_t*)(param_buffer + 40) = v13;
		}

		return result;
	};

	auto sub_140A43530 = [](int64_t param_buffer, uint8_t a2, int64_t a3) -> int64_t
	{
		// dmp location: direct reference: [actual address in first opcode] E8 ? ? ? ? 44 8D 04 33
		// retail location: direct reference: [actual address in first opcode] E8 ? ? ? ? 8D 0C 33  

		int64_t v4; // rax
		int64_t v5; // rdx
		uint16_t* v6; // rcx
		uint32_t* v7; // rax
		int64_t v8; // rax
		int64_t v9; // rcx
		int64_t v10; // rcx
		int64_t v11; // rax
		int64_t result; // rax

		v4 = a2;
		v5 = 4i64;
		v6 = (uint16_t*)(param_buffer + 216);
		*(uint8_t*)(param_buffer + 1108) = LUT_Snowflake_0[v4];
		*(uint16_t*)(param_buffer + 1088) = 2048;
		v7 = (uint32_t*)v6 + 65;
		*(uint32_t*)param_buffer = 126353408;
		*(uint32_t*)(param_buffer + 4) = 378998543;
		*(uint32_t*)(param_buffer + 8) = 631643678;
		*(uint32_t*)(param_buffer + 12) = 884288813;
		*(uint32_t*)(param_buffer + 16) = 1136933948;
		*(uint32_t*)(param_buffer + 20) = 1389579083;
		*(uint32_t*)(param_buffer + 24) = 1642224218;
		*(uint32_t*)(param_buffer + 28) = 1894869353;
		*(uint32_t*)(param_buffer + 32) = -2147452808;
		*(uint32_t*)(param_buffer + 36) = 119275520;
		*(uint32_t*)(param_buffer + 40) = 357895737;
		*(uint32_t*)(param_buffer + 44) = 596515954;
		*(uint32_t*)(param_buffer + 48) = 835136171;
		*(uint32_t*)(param_buffer + 52) = 1073756388;
		*(uint32_t*)(param_buffer + 56) = 0x10000000;
		*(uint32_t*)(param_buffer + 60) = 805314560;
		*(uint32_t*)(param_buffer + 64) = 1342193664;
		*(uint32_t*)(param_buffer + 68) = 1879072768;
		*(uint16_t*)(param_buffer + 72) = 0x8000;
		*(uint32_t*)(param_buffer + 74) = 126353408;
		*(uint32_t*)(param_buffer + 78) = 378998543;
		*(uint32_t*)(param_buffer + 82) = 631643678;
		*(uint32_t*)(param_buffer + 86) = 884288813;
		*(uint32_t*)(param_buffer + 90) = 1136933948;
		*(uint32_t*)(param_buffer + 94) = 1389579083;
		*(uint32_t*)(param_buffer + 98) = 1642224218;
		*(uint32_t*)(param_buffer + 102) = 1894869353;
		*(uint32_t*)(param_buffer + 106) = -2147452808;
		*(uint32_t*)(param_buffer + 110) = 63176704;
		*(uint32_t*)(param_buffer + 114) = 189466504;
		*(uint32_t*)(param_buffer + 118) = 315821839;
		*(uint32_t*)(param_buffer + 122) = 442111639;
		*(uint32_t*)(param_buffer + 126) = 568466974;
		*(uint32_t*)(param_buffer + 130) = 694756774;
		*(uint32_t*)(param_buffer + 134) = 821112109;
		*(uint32_t*)(param_buffer + 138) = 947401909;
		*(uint32_t*)(param_buffer + 142) = 1073757244;
		*(uint32_t*)(param_buffer + 146) = 126353408;
		*(uint32_t*)(param_buffer + 150) = 378998543;
		*(uint32_t*)(param_buffer + 154) = 631643678;
		*(uint32_t*)(param_buffer + 158) = 884288813;
		*(uint32_t*)(param_buffer + 162) = 1136933948;
		*(uint32_t*)(param_buffer + 166) = 1389579083;
		*(uint32_t*)(param_buffer + 170) = 1642224218;
		*(uint32_t*)(param_buffer + 174) = 1894869353;
		*(uint32_t*)(param_buffer + 178) = -2147452808;
		*(uint32_t*)(param_buffer + 182) = 0x4000000;
		*(uint32_t*)(param_buffer + 186) = 201328640;
		*(uint32_t*)(param_buffer + 190) = 335548416;
		*(uint32_t*)(param_buffer + 194) = 469768192;
		*(uint32_t*)(param_buffer + 198) = 603987968;
		*(uint32_t*)(param_buffer + 202) = 738207744;
		*(uint32_t*)(param_buffer + 206) = 872427520;
		*(uint32_t*)(param_buffer + 210) = 1006647296;
		*(uint16_t*)(param_buffer + 214) = 0x4000;
		do
		{
			*(v7 - 32) = 67109376;
			*v6 = 0;
			v6 += 109;
			*(v7 - 31) = 134219264;
			*(v7 - 30) = 201329152;
			*(v7 - 29) = 268439040;
			*(v7 - 28) = 0x10000000;
			*(v7 - 27) = 805314560;
			*(v7 - 26) = 0x4000;
			*(v7 - 25) = 536875008;
			*(v7 - 24) = 1073754112;
			*(v7 - 23) = 0x10000000;
			*(v7 - 22) = 805314560;
			*(v7 - 21) = 0x4000;
			*(v7 - 20) = 536875008;
			*(v7 - 19) = 1073754112;
			*(v7 - 18) = 0x10000000;
			*(v7 - 17) = 805314560;
			*(v7 - 16) = 0x4000;
			*(v7 - 15) = 536875008;
			*(v7 - 14) = 1073754112;
			*(v7 - 13) = 1610633216;
			*(v7 - 12) = -2147454976;
			*(v7 - 11) = 0x10000000;
			*(v7 - 10) = 805314560;
			*(v7 - 9) = 1342193664;
			*(v7 - 8) = 1879072768;
			*((uint16_t*)v7 - 14) = 0x8000;
			*(v7 - 2) = 76677120;
			*(v7 - 1) = 230099237;
			*v7 = 383455817;
			v7[1] = 536877934;
			v7[2] = 76677120;
			v7[3] = 230099237;
			v7[4] = 383455817;
			v7[5] = 536877934;
			v7[6] = 76677120;
			v7[7] = 230099237;
			v7[8] = 383455817;
			v7[9] = 536877934;
			v7[10] = 76677120;
			v7[11] = 230099237;
			v7[12] = 383455817;
			v7[13] = 536877934;
			v7[14] = 76677120;
			v7[15] = 230099237;
			v7[16] = 383455817;
			v7[17] = 536877934;
			v7[18] = 76677120;
			v7[19] = 230099237;
			v7[20] = 383455817;
			v7[21] = 536877934;
			*(uint32_t*)((char*)v7 - 26) = 0x10000000;
			*(uint32_t*)((char*)v7 - 22) = 805314560;
			*(uint32_t*)((char*)v7 - 18) = 1342193664;
			*(uint32_t*)((char*)v7 - 14) = 1879072768;
			*((uint16_t*)v7 - 5) = 0x8000;
			v7 = (uint32_t*)((char*)v7 + 218);
			--v5;
		} while (v5);

		if (a3)
		{
		//  todo implement this(pix);
		//	sub_7FF7FC23BA70(param_buffer, a3);
		}
		else
		{
			v8 = param_buffer + 1116;
			v9 = 256i64;
			do
			{
				*(uint32_t*)(v8 - 4) = 0x8000000;
				*(uint16_t*)(v8 + 28) = 0x8000;
				*(uint32_t*)v8 = 402657280;
				*(uint32_t*)(v8 + 4) = 671096832;
				*(uint32_t*)(v8 + 8) = 939536384;
				*(uint32_t*)(v8 + 12) = 1207975936;
				*(uint32_t*)(v8 + 16) = 1476415488;
				*(uint32_t*)(v8 + 20) = 1744855040;
				*(uint32_t*)(v8 + 24) = 2013294592;
				v8 += 34i64;
				--v9;
			} while (v9);
			v10 = 4096i64;
			v11 = param_buffer + 9820;
			do
			{
				*(uint32_t*)(v11 - 4) = 0x8000000;
				*(uint16_t*)(v11 + 28) = 0x8000;
				*(uint32_t*)v11 = 402657280;
				*(uint32_t*)(v11 + 4) = 671096832;
				*(uint32_t*)(v11 + 8) = 939536384;
				*(uint32_t*)(v11 + 12) = 1207975936;
				*(uint32_t*)(v11 + 16) = 1476415488;
				*(uint32_t*)(v11 + 20) = 1744855040;
				*(uint32_t*)(v11 + 24) = 2013294592;
				v11 += 34i64;
				--v10;
			} while (v10);
		}
		*(uint32_t*)(param_buffer + 1092) = 96;
		*(uint16_t*)(param_buffer + 1090) = 1024;
		result = 0i64;
		*(uint32_t*)(param_buffer + 1096) = 128;
		*(uint32_t*)(param_buffer + 1100) = 80;
		*(uint32_t*)(param_buffer + 1104) = 112;

		return result;
	};

	uint32_t v2; // eax
	int v3; // edi
	uint64_t v4; // rax
	int64_t v6; // rcx
	int v7; // ebx
	char v8; // al
	uint8_t v9; // r14
	uint32_t v10; // ecx
	int64_t result; // rax
	int v12; // eax
	uint32_t v13; // esi
	int64_t v14; // rdi
	int v15; // ebx
	int64_t v16; // rcx
	int v17; // er8
	char v18[256]; // [rsp+30h] [rbp-128h] BYREF

	*(uint64_t*)(param_buffer + 0x246B0) = data_buffer;
	*(uint64_t*)(param_buffer + 0x246B8) = buffer_size;
	*(uint64_t*)(param_buffer + 0x246C8) = -1;
	*(uint64_t*)(param_buffer + 0x40) = 0;

	// Cut alot of stuff from retail here, actual legion didn't use em either.

	v2 = sub_140A44040(param_buffer, 6);
	v3 = -1;
	v4 = (sub_140A44040(param_buffer, v2) | (1i64 << v2)) - 1;
	*(uint64_t*)(param_buffer + 149144) = v4;
	if (_BitScanReverse64((unsigned long*)&v6, (v4 >> 6) + 100 + v4)) // Had to cast v6 to unsigned long here, it might cause issues.
		v3 = v6;
	*(uint64_t*)(param_buffer + 149160) = sub_140A44040(param_buffer, (unsigned int)(v3 + 1));
	v7 = sub_140A44040(param_buffer, 4i64);
	v8 = sub_140A44040(param_buffer, 4i64);
	*(uint32_t*)(param_buffer + 149136) = 1i64 << ((unsigned __int8)v7 + 9);
	*(uint32_t*)(param_buffer + 149140) = 1i64 << (v8 + 9);
	*(uint32_t*)(param_buffer + 149132) = v7 + 8;
	v9 = sub_140A44040(param_buffer, 2i64);
	v10 = sub_140A44040(param_buffer, 3i64) | 8;
	if (v10 <= 8)
		return sub_140A43530(param_buffer + 48, v9, 0i64);

	// continue this yeet.
}

//-----------------------------------------------------------------------------
// Purpose: decompress input streamed data
//-----------------------------------------------------------------------------
void RTech::DecompressSnowflake(int64_t param_buffer, uint64_t data_size, uint64_t buffer_size)
{
	/*TODO 'UIIA'*/
}

//-----------------------------------------------------------------------------
// Purpose: decompress input animation data
//-----------------------------------------------------------------------------
float* __fastcall RTech::DecompressDynamicTrack(int frame_count, uint8_t* in_translation_buffer, float translation_scale, float* out_translation_buffer, float* time_scale)
{
	int           v5; // er10
	int64_t       v6; // rcx
	float*        v7; // rdi
	float         v8; // xmm15_4
	uint8_t*      v9; // rbx
	int          v10; // eax
	int          v11; // er11
	int          v12; // er8
	int          v13; // er8
	int64_t      v14; // xmm12
	int64_t      v15; // xmm14
	int64_t      v16; // r9
	unsigned int v17; // edx
	unsigned int v18; // er8
	float        v19; // xmm1_4
	float        v20; // xmm11_4
	float        v21; // xmm13_4
	unsigned int v22; // er11
	float        v23; // xmm0_4
	float        v24; // xmm15_4
	int64_t      v25; // rax
	int64_t      v26; // xmm9
	float        v27; // xmm10_4
	int64_t      v28; // xmm1
	__m128i      v29; // xmm7
	int64_t      v30; // xmm5
	int64_t      v31; // xmm2
	float        v32; // xmm8_4
	int          v33; // ecx
	float        v34; // xmm6_4
	int64_t      v35; // rax
	int64_t      v36; // rax
	int          v37; // er9
	float        v38; // xmm1_4
	float*    result; // rax
	int16_t      v40; // dx
	float        v41; // xmm0_4
	float        v42; // [rsp+F0h] [rbp+8h]
	float        v43; // [rsp+F8h] [rbp+10h]
	float        v44; // [rsp+100h] [rbp+18h]
	int      v73[16];

	v73[0] = 0x2100F01;
	v73[1] = 0xF020807;
	v73[2] = 0xF0300;
	v73[3] = 0x5000F04;
	v73[4] = 0xF06000F;
	v73[5] = 0xF0700;
	v73[6] = 0x3020F02;
	v73[7] = 0xF04020F;
	v73[8] = 0x20F0502;
	v73[9] = 0x7020F06;
	v73[10] = 0xF02020F;
	v73[11] = 0x40F0304;
	v73[12] = 0x5040F04;
	v73[13] = 0xF06040F;
	v73[14] = 0x40F0704;

	v44 = translation_scale;
	v5 = frame_count;
	LODWORD(v6) = in_translation_buffer[1];
	v7 = out_translation_buffer;
	v8 = translation_scale;
	v9 = in_translation_buffer;
	if (v5 >= (int)v6)
	{
		LOBYTE(v10) = v6;
		do
		{
			v5 -= (uint8_t)v6;
			v6 = *((uint8_t*)v73 + 3 * *v9) + ((uint64_t)(*((uint8_t*)v73 + 3 * *v9 + 1) + (uint8_t)v10 * (unsigned int)*((uint8_t*)v73 + 3 * *v9 + 2)) >> 4);
			v10 = v9[2 * v6 + 1];
			v9 += 2 * v6;
			LOBYTE(v6) = v10;
		} while (v5 >= v10);
	}
	v11 = v9[1] - 1;
	if (v5 >= v11)
	{
		*out_translation_buffer = FrameToEulerTranslation(v9, v5, translation_scale);
		v41 = FrameToEulerTranslation(v9, 0, translation_scale);
		result = time_scale;
		*time_scale = v41;
	}
	else
	{
		v12 = *v9;
		if (*v9)
		{
			if (v12 == 1)
			{
				v40 = *((uint16_t*)v9 + 1);
				*time_scale = (float)(*((int16_t*)v9 + 1) + (char)v9[v5 + 4]) * translation_scale;
				if (v5 > 0)
				{
					v40 += (char)v9[v5 + 3];
				}
				result = (float*)(unsigned int)v40;
				*out_translation_buffer = (float)v40 * translation_scale;
			}
			else
			{
				v13 = v12 - 2;
				v14 = 0x3F800000u;
				v15 = 0x3F800000u;
				v16 = (unsigned int)(v13 / 6);
				v17 = 1;
				v18 = v13 % 6;
				v19 = 1.0 / (float)v11;
				v43 = 1.0 / (float)v11;
				v20 = (float)*((int16_t*)v9 + 1);
				v21 = (float)*((int16_t*)v9 + 1);
				if (v18 >= 1)
				{
					if (v18 >= 4)
					{
						v22 = 3;
						v23 = (float)(v5 + 1) * v19;
						v24 = (float)v5 * v19;
						v42 = (float)(v5 + 1) * v19;
						do
						{
							*(float*)&v15 = *(float*)&v15 * v23;
							v25 = v17;
							v17 += 4;
							*(float*)&v14 = *(float*)&v14 * v24;
							v26 = v15;
							v27 = *(float*)&v15;
							*(float*)&v26 = *(float*)&v15 * v23;
							v28 = v14;
							*(float*)&v28 = *(float*)&v14 * v24;
							v29 = _mm_cvtsi32_si128(*(int16_t*)&v9[2 * v22]);
							v30 = v26;
							*(float*)&v30 = (float)(*(float*)&v15 * v23) * v23;
							v31 = v28;
							v32 = (float)*(int16_t*)&v9[2 * v25 + 2];
							v33 = *(int16_t*)&v9[2 * v22 + 2];
							LODWORD(v25) = *(int16_t*)&v9[2 * v22 + 4];
							v22 += 4;
							*(float*)&v31 = (float)(*(float*)&v14 * v24) * v24;
							v15 = v30;
							*(float*)&v15 = *(float*)&v30 * v23;
							v34 = *(float*)&v14 * v32;
							v14 = v31;
							*(float*)&v14 = *(float*)&v31 * v24;
							v29.m128i_i32[0] = _mm_extract_ps(_mm_cvtepi32_ps(v29), 0);
							v21 = (float)((float)((float)((float)(v27 * v32) + v21) + (float)(*(float*)v29.m128i_i8 * *(float*)&v26))
								+ (float)((float)v33 * *(float*)&v30))
								+ (float)((float)(int)v25 * (float)(*(float*)&v30 * v23));
							v20 = (float)((float)((float)(v34 + v20) + (float)(*(float*)v29.m128i_i8 * *(float*)&v28))
								+ (float)((float)v33 * *(float*)&v31))
								+ (float)((float)(int)v25 * (float)(*(float*)&v31 * v24));
							v23 = v42;
						} while (v17 <= v18 - 3);
						v8 = v44;
						v19 = v43;
					}
					for (; v17 <= v18; v20 = v20 + (float)((float)*(int16_t*)&v9[2 * v35 + 2] * *(float*)&v14))
					{
						v35 = v17++;
						*(float*)&v14 = *(float*)&v14 * (float)((float)v5 * v19);
						*(float*)&v15 = *(float*)&v15 * (float)((float)(v5 + 1) * v19);
						v21 = v21 + (float)((float)*(int16_t*)&v9[2 * v35 + 2] * *(float*)&v15);
					}
				}
				if ((uint32_t)v16)
				{
					v36 = (unsigned int)v16;
					v37 = LUT_Dynamic_Track_0[v16];
					v38 = LUT_Dynamic_Track_1[v36];
					v20 = (float)(2
						* (uint16_t)(((1 << v37) - 1) & (*(uint16_t*)&v9[2 * (v18 + (int64_t)(v5 * v37 / 16)) + 4] >> (v5 * v37 % -16))))
						+ (float)(v20 - v38);
					v21 = (float)(2 * (uint16_t)(((1 << v37) - 1) & (*(uint16_t*)&v9[2 * (v18 + (int64_t)((v37 + v5 * v37) / 16))
							+ 4] >> ((v37 + v5 * v37) % -16)))) + (float)(v21 - v38);
				}
				result = time_scale;
				*v7 = v20 * v8;
				*time_scale = v21 * v8;
			}
		}
		else
		{
			*out_translation_buffer = (float)*(int16_t*)&v9[2 * v5 + 2] * translation_scale;
			result = time_scale;
			*time_scale = (float)*(int16_t*)&v9[2 * v5 + 4] * translation_scale;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: convert bone rotations from input
//-----------------------------------------------------------------------------
void __fastcall RTech::DecompressConvertRotation(const __m128i* in_rotation_buffer, float* out_rotation_buffer)
{
	__m128  v2; // xmm4
	__m128i v3; // xmm9
	__m128i v4; // xmm1
	__m128i v5; // xmm8
	__m128i v6; // xmm5
	__m128i v7; // xmm9
	__m128  v8; // xmm2
	__m128  v9; // xmm8
	__m128 v10; // xmm4
	__m128 v11; // xmm2
	__m128 v12; // xmm3
	__m128 v13; // xmm1
	__m128 v14; // xmm7
	__m128 v15; // xmm6

	v2 = _mm_mul_ps(_mm_and_ps(_mm_castsi128_ps(_mm_set_epi32(0x0, 0xffffffff, 0xffffffff, 0xffffffff)), _mm_castsi128_ps(_mm_lddqu_si128(in_rotation_buffer))), _mm_castsi128_ps(_mm_set_epi32(0x3F000000, 0x3F000000, 0x3F000000, 0x3F000000)));
	__m128i m1 = _mm_set_epi32(0x1, 0x1, 0x1, 0x1);
	v3 = _mm_load_si128(&m1);
	v4 = _mm_cvtps_epi32(_mm_mul_ps((__m128)_mm_castsi128_ps(_mm_set_epi32(0x3F22F983, 0x3F22F983, 0x3F22F983, 0x3F22F983)), v2));
	v5 = v3;
	__m128i m2 = _mm_set_epi32(0x2, 0x2, 0x2, 0x2);
	__m128i m3 = _mm_set_epi32(0x2, 0x2, 0x2, 0x2);
	v6 = _mm_and_si128(_mm_load_si128(&m2), v4);
	v7 = _mm_and_si128(_mm_add_epi32(v3, v4), m3);
	v8 = _mm_cvtepi32_ps(v4);
	v9 = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(v5, v4), _mm_setzero_si128()));
	v10 = _mm_sub_ps(
		_mm_sub_ps(
			_mm_sub_ps(v2, _mm_mul_ps(_mm_castsi128_ps(_mm_set_epi32(0x3FC91000, 0x3FC91000, 0x3FC91000, 0x3FC91000)), v8)),
			_mm_mul_ps(_mm_castsi128_ps(_mm_set_epi32(0xB6957000, 0xB6957000, 0xB6957000, 0xB6957000)), v8)),
		_mm_mul_ps(_mm_castsi128_ps(_mm_set_epi32(0xB06F4B9E, 0xB06F4B9E, 0xB06F4B9E, 0xB06F4B9E)), v8));
	v11 = _mm_mul_ps(v10, v10);
	v12 = _mm_add_ps(
		_mm_mul_ps(
			_mm_add_ps(
				_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_castsi128_ps(_mm_set_epi32(0xB94D6102, 0xB94D6102, 0xB94D6102, 0xB94D6102)), v11), _mm_castsi128_ps(_mm_set_epi32(0x3C0885D2, 0x3C0885D2, 0x3C0885D2, 0x3C0885D2))), v11),
				_mm_castsi128_ps(_mm_set_epi32(0xBE2AAAA8, 0xBE2AAAA8, 0xBE2AAAA8, 0xBE2AAAA8))),
			_mm_mul_ps(v11, v10)),
		v10);
	v13 = _mm_add_ps(
		_mm_mul_ps(
			_mm_add_ps(
				_mm_mul_ps(
					_mm_add_ps(
						_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_castsi128_ps(_mm_set_epi32(0x37CF14C2, 0x37CF14C2, 0x37CF14C2, 0x37CF14C2)), v11), _mm_castsi128_ps(_mm_set_epi32(0xBAB60B22, 0xBAB60B22, 0xBAB60B22, 0xBAB60B22))), v11),
						_mm_castsi128_ps(_mm_set_epi32(0x3D2AAAA9, 0x3D2AAAA9, 0x3D2AAAA9, 0x3D2AAAA9))),
					v11),
				_mm_castsi128_ps(_mm_set_epi32(0xBF000000, 0xBF000000, 0xBF000000, 0xBF000000))),
			v11),
		_mm_castsi128_ps(_mm_set_epi32(0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000)));
	v14 = _mm_xor_ps(_mm_or_ps(_mm_andnot_ps(v9, v13), _mm_and_ps(v12, v9)), _mm_castsi128_ps(_mm_slli_epi32(v6, 0x1Eu)));
	v15 = _mm_xor_ps(_mm_or_ps(_mm_andnot_ps(v9, v12), _mm_and_ps(v13, v9)), _mm_castsi128_ps(_mm_slli_epi32(v7, 0x1Eu)));
	v6.m128i_i32[0] = _mm_extract_ps(_mm_shuffle_ps(v15, v15, 85), 0);
	v10.m128_i32[0] = _mm_extract_ps(_mm_shuffle_ps(v14, v14, 85), 0);
	v11.m128_f32[0] = v15.m128_f32[0] * v10.m128_f32[0];
	v12.m128_f32[0] = *(float*)v6.m128i_i8 * v14.m128_f32[0];
	*(float*)v6.m128i_i8 = *(float*)v6.m128i_i8 * v15.m128_f32[0];
	v10.m128_f32[0] = v10.m128_f32[0] * v14.m128_f32[0];
	v14.m128_i32[0] = _mm_extract_ps(_mm_shuffle_ps(v14, v14, 170), 0);
	v15.m128_i32[0] = _mm_extract_ps(_mm_shuffle_ps(v15, v15, 170), 0);
	*out_rotation_buffer = (float)(v15.m128_f32[0] * v12.m128_f32[0]) - (float)(v14.m128_f32[0] * v11.m128_f32[0]);
	out_rotation_buffer[1] = (float)(v15.m128_f32[0] * v11.m128_f32[0]) + (float)(v14.m128_f32[0] * v12.m128_f32[0]);
	out_rotation_buffer[3] = (float)(v15.m128_f32[0] * *(float*)v6.m128i_i8) + (float)(v14.m128_f32[0] * v10.m128_f32[0]);
	out_rotation_buffer[2] = (float)(v14.m128_f32[0] * *(float*)v6.m128i_i8) - (float)(v15.m128_f32[0] * v10.m128_f32[0]);
}

//-----------------------------------------------------------------------------
// Purpose: calculate 'GUID' from input data
//-----------------------------------------------------------------------------
uint64_t __fastcall RTech::StringToGuid(const char* asset_name)
{
	uint32_t* v1; // r8
	uint64_t         v2; // r10
	int              v3; // er11
	uint32_t         v4; // er9
	uint32_t          i; // edx
	uint64_t         v6; // rcx
	int              v7; // er9
	int              v8; // edx
	int              v9; // eax
	uint32_t        v10; // er8
	int             v12; // ecx
	uint32_t* a1 = (uint32_t*)asset_name;

	v1 = a1;
	v2 = 0i64;
	v3 = 0;
	v4 = (*a1 - 45 * ((~(*a1 ^ 0x5C5C5C5Cu) >> 7) & (((*a1 ^ 0x5C5C5C5Cu) - 0x1010101) >> 7) & 0x1010101)) & 0xDFDFDFDF;
	for (i = ~*a1 & (*a1 - 0x1010101) & 0x80808080; !i; i = v8 & 0x80808080)
	{
		v6 = v4;
		v7 = v1[1];
		++v1;
		v3 += 4;
		v2 = ((((uint64_t)(0xFB8C4D96501i64 * v6) >> 24) + 0x633D5F1 * v2) >> 61) ^ (((uint64_t)(0xFB8C4D96501i64 * v6) >> 24)
			+ 0x633D5F1 * v2);
		v8 = ~v7 & (v7 - 0x1010101);
		v4 = (v7 - 45 * ((~(v7 ^ 0x5C5C5C5Cu) >> 7) & (((v7 ^ 0x5C5C5C5Cu) - 0x1010101) >> 7) & 0x1010101)) & 0xDFDFDFDF;
	}
	v9 = -1;
	v10 = (i & -(signed)i) - 1;
	if (_BitScanReverse((unsigned long*)&v12, v10))
	{
		v9 = v12;
	}
	return 0x633D5F1 * v2 + ((0xFB8C4D96501i64 * (uint64_t)(v4 & v10)) >> 24) - 0xAE502812AA7333i64 * (uint32_t)(v3 + v9 / 8);
}

//-----------------------------------------------------------------------------
// Purpose: calculate 'euler' translations for each frame
//-----------------------------------------------------------------------------
float __fastcall RTech::FrameToEulerTranslation(uint8_t* translation_buffer, int frame_count, float translation_scale)
{
	int           v3; // ebx
	float         v4; // xmm4_4
	int64_t       v5; // rdi
	uint8_t* v6; // r11
	float         v7; // xmm1_4
	int           v8; // ebx
	int64_t       v9; // r10
	unsigned int v10; // edx
	unsigned int v11; // ebx
	float        v12; // xmm5_4
	float        v13; // xmm2_4
	unsigned int v14; // er8
	float        v15; // xmm3_4
	int64_t      v16; // rax
	float        v17; // xmm1_4
	float        v18; // xmm0_4
	float        v19; // xmm1_4
	float        v20; // xmm2_4
	float        v21; // xmm0_4
	int          v22; // ecx
	float        v23; // xmm0_4
	float        v24; // xmm1_4
	float        v25; // xmm2_4
	float        v26; // xmm0_4
	int64_t      v27; // rax
	float     result; // xmm0_4
	int16_t      v29; // dx

	v3 = *translation_buffer;
	v4 = translation_scale;
	v5 = frame_count;
	v6 = translation_buffer;
	if (!*translation_buffer)
	{
		return (float)*(int16_t*)&translation_buffer[2 * frame_count + 2] * translation_scale;
	}
	if (v3 == 1)
	{
		v29 = *((uint16_t*)translation_buffer + 1);
		if ((int)v5 > 0)
		{
			v29 += (char)translation_buffer[v5 + 3];
		}
		result = (float)v29 * translation_scale;
	}
	else
	{
		v7 = 1.0;
		v8 = v3 - 2;
		v9 = (unsigned int)(v8 / 6);
		v10 = 1;
		v11 = v8 % 6;
		v12 = 1.0 / (float)(translation_buffer[1] - 1);
		v13 = (float)*((int16_t*)translation_buffer + 1);
		if (v11 >= 1)
		{
			if (v11 >= 4)
			{
				v14 = 3;
				v15 = (float)(int)v5 * v12;
				do
				{
					v16 = v10;
					v17 = v7 * v15;
					v10 += 4;
					v18 = (float)*(int16_t*)&v6[2 * v16 + 2] * v17;
					v19 = v17 * v15;
					v20 = v13 + v18;
					v21 = (float)*(int16_t*)&v6[2 * v14];
					v22 = *(int16_t*)&v6[2 * v14 + 2];
					LODWORD(v16) = *(int16_t*)&v6[2 * v14 + 4];
					v14 += 4;
					v23 = v21 * v19;
					v24 = v19 * v15;
					v25 = v20 + v23;
					v26 = (float)v22 * v24;
					v7 = v24 * v15;
					v13 = (float)(v25 + v26) + (float)((float)(int)v16 * v7);
				} while (v10 <= v11 - 3);
			}
			for (; v10 <= v11; v13 = v13 + (float)((float)*(int16_t*)&v6[2 * v27 + 2] * v7))
			{
				v27 = v10++;
				v7 = v7 * (float)((float)(int)v5 * v12);
			}
		}
		if ((uint32_t)v9)
		{
			v13 = (float)(2 * (uint16_t)(((1 << (uint8_t)LUT_Dynamic_Track_0[v9]) - 1)
				& (*(uint16_t*)&v6[2 * (v11 + (int64_t)((int)v5 * LUT_Dynamic_Track_0[v9] / 16)) + 4] >> ((int)v5 * LUT_Dynamic_Track_0[v9] % -16))))
				+ (float)(v13 - LUT_Dynamic_Track_1[v9]);
		}
		result = v13 * v4;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: unswizzle 'UIIA' datablock
//-----------------------------------------------------------------------------
void RTech::UnswizzleBlock(uint32_t x, uint32_t y, uint32_t a3, uint32_t a4, uint32_t x1, uint32_t y2)
{
	/*TODO 'UIIA'*/
};
///////////////////////////////////////////////////////////////////////////////
RTech* g_pRtech;
