#pragma once
#include <cstdint>

struct mstudioseqdesc_t
{
	int baseptr;
	int szlabelindex;
	int szactivitynameindex;
	int flags;

	int activity;
	int actweight;

	int numevents;
	int eventindex;

	float bbmin[3];
	float bbmax[3];

	int numblends;
	int animindexindex;

	// there is more but we don't need to use it here. see respawn-mdl/rmdl/rmdl_structs.bt at mstudioseqdesc_t_v54
};

struct mstudioanimdescv54_t
{
	int baseptr;
	int sznameindex;

	float fps;

	int flags;
	int numframes;

	int nummovements;
	int movementindex;

	int compressedikerrorindex;
	int animindex;

	int Flags2;
	int UnknownTableOffset;


	int sectionindex; // was chunk offset
	int FrameSplitCount;
	int FrameMedianCount;
	uint64_t Padding;
	uint64_t SomeDataOffset;
};

struct mstudioanimdescv53_t
{
	uint32_t Zero;
	uint32_t NameOffset;

	float Framerate;

	uint32_t Flags;
	uint32_t FrameCount;

	uint32_t Zero1;
	uint32_t Zero2;

	uint32_t UnknownOffset;
	uint32_t FirstChunkOffset;

	uint32_t UnknownCount2;
	uint32_t UnknownOffset2;

	uint32_t Zero3;
	uint32_t Zero4;

	uint32_t OffsetToChunkOffsetsTable;
	uint32_t FrameSplitCount;

	uint8_t UnknownZero[0x20];
};

#define STUDIO_LOOPING	0x0001		// ending frame should be the same as the starting frame
#define STUDIO_SNAP		0x0002		// do not interpolate between previous animation and this one
#define STUDIO_DELTA	0x0004		// this sequence "adds" to the base sequences, not slerp blends
#define STUDIO_AUTOPLAY	0x0008		// temporary flag that forces the sequence to always play
#define STUDIO_POST		0x0010		// 
#define STUDIO_ALLZEROS	0x0020		// this animation/sequence has no real animation data
#define STUDIO_FRAMEANIM 0x0040		// animation is encoded as by frame x bone instead of RLE bone x frame
#define STUDIO_CYCLEPOSE 0x0080		// cycle index is taken from a pose parameter index
#define STUDIO_REALTIME	0x0100		// cycle index is taken from a real-time clock, not the animations cycle index
#define STUDIO_LOCAL	0x0200		// sequence has a local context sequence
#define STUDIO_HIDDEN	0x0400		// don't show in default selection views
#define STUDIO_OVERRIDE	0x0800		// a forward declared sequence (empty)
#define STUDIO_ACTIVITY	0x1000		// Has been updated at runtime to activity index
#define STUDIO_EVENT	0x2000		// Has been updated at runtime to event index on server
#define STUDIO_WORLD	0x4000		// sequence blends in worldspace
#define STUDIO_NOFORCELOOP 0x8000	// do not force the animation loop
#define STUDIO_EVENT_CLIENT 0x10000	// Has been updated at runtime to event index on client

struct RAnimBoneFlag
{
	uint16_t Size : 12;
	uint16_t bAdditiveCustom : 1;
	uint16_t bDynamicScale : 1;			// If zero, one per data set
	uint16_t bDynamicRotation : 1;		// If zero, one per data set
	uint16_t bDynamicTranslation : 1;	// If zero, one per data set
};

struct RAnimTitanfallBoneFlag
{
	uint8_t Unused : 1;
	uint8_t bStaticTranslation : 1;		// If zero, one per data set
	uint8_t bStaticRotation : 1;		// If zero, one per data set
	uint8_t bStaticScale : 1;			// If zero, one per data set
	uint8_t Unused2 : 1;
	uint8_t Unused3 : 1;
	uint8_t Unused4 : 1;
};

struct RAnimBoneHeader
{
	float TranslationScale;

	uint8_t BoneIndex;
	RAnimTitanfallBoneFlag BoneFlags;
	uint8_t Flags2;
	uint8_t Flags3;

	union
	{
		struct
		{
			uint16_t OffsetX;
			uint16_t OffsetY;
			uint16_t OffsetZ;
			uint16_t OffsetL;
		};

		uint64_t PackedRotation;
	} RotationInfo;

	uint16_t TranslationX;
	uint16_t TranslationY;
	uint16_t TranslationZ;

	uint16_t ScaleX;
	uint16_t ScaleY;
	uint16_t ScaleZ;

	uint32_t DataSize;
};