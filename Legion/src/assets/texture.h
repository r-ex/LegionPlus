#pragma once

// texture type according to shader resource binding slots
// textures are not necessarily always in these slots, but the vast majority will be
enum eTextureType
{
	ALBEDO = 0,
	NORMAL = 1,
	GLOSS = 2,
	SPECULAR = 3,
	EMISSIVE = 4,
	AO = 5,
	CAVITY = 6,
	OPACITY_MULTIPLY = 7,
	DETAIL = 8,
	DETAIL_NORMAL = 9,
	EMISSIVE_MULTIPLY = 10,
	UV_DISTORTION = 11,
	UV_DISTORTION2 = 12,
	SCATTER_THICKNESS = 13,
	TRANSMITTANCE_TINT = 14,
	ANISO_SPEC_DIR = 15,
	LAYER_BLEND = 16,

	ALBEDO2 = 22,
	NORMAL2 = 23,
	GLOSS2 = 24,
	SPECULAR2 = 25,
	EMISSIVE2 = 26,
	EMISSIVE_MULTIPLY2 = 27,
	AO2 = 28,
	CAVITY2 = 29,
	OPACITY_MULTIPLY2 = 30,
	DETAIL2 = 31,
	DETAIL_NORMAL2 = 32,
	SCATTER_THICKNESS2 = 33,
	TRANSMITTANCE_TINT2 = 34,

	// these seem to mainly be used in particle shaders
	// these are not in the right positions for their shader resources
	// however they would overlap with ALBEDO and NORMAL, which isn't helpful when comparing types
	REFRACT,
	COLOR_RAMP,

};
