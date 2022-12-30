#pragma once
#include <cstdint>
typedef unsigned short uint16;

// sequence and autolayer flags
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
#define STUDIO_WORLD_AND_RELATIVE 0x20000 // do worldspace blend, then do normal blend on top
#define STUDIO_ROOTXFORM 0x40000	// sequence wants to derive a root re-xform from a given bone

// autolayer flags
//							0x0001
//							0x0002
//							0x0004
//							0x0008
#define STUDIO_AL_POST		0x0010	// 
//							0x0020
#define STUDIO_AL_SPLINE	0x0040	// convert layer ramp in/out curve is a spline instead of linear
#define STUDIO_AL_XFADE		0x0080	// pre-bias the ramp curve to compense for a non-1 weight, assuming a second layer is also going to accumulate
//							0x0100
#define STUDIO_AL_NOBLEND	0x0200	// animation always blends at 1.0 (ignores weight)
//							0x0400
//							0x0800
#define STUDIO_AL_LOCAL		0x1000	// layer is a local context sequence
#define STUDIO_AL_UNK_52	0x2000
#define STUDIO_AL_POSE		0x4000	// layer blends using a pose parameter instead of parent cycle
#define STUDIO_AL_UNK_53	0x8000  // added in v53 (probably)

enum eventtype : uint8_t
{
	NEW_EVENT_STYLE = (1 << 10),
};

struct mstudioactivitymodifierv53_t
{
	int sznameindex;
	bool negate; // 0 or 1 observed.
};

struct mstudioactivitymodifierv54_t_v16
{
	short sznameindex;
	bool negate; // 0 or 1 observed.

	inline mstudioactivitymodifierv53_t Downgrade() 
	{ 
		return mstudioactivitymodifierv53_t{ (int)this->sznameindex , negate };
	};
};

struct mstudioposeparamdescv54_t
{
	int sznameindex;

	int flags; // ????
	float start; // starting value
	float end; // ending value
	float loop;	// looping range, 0 for no looping, 360 for rotations, etc.
};

struct mstudioposeparamdescv54_t_v16
{
	uint16_t sznameindex;

	short flags; // ????
	float start; // starting value
	float end; // ending value
	float loop;	// looping range, 0 for no looping, 360 for rotations, etc.

	inline mstudioposeparamdescv54_t Downgrade()
	{
		mstudioposeparamdescv54_t out{};
		
		out.sznameindex = this->sznameindex;
		out.flags = this->flags;
		out.start = this->start;
		out.end = this->end;
		out.loop = this->loop;

		return out;
	}
};

struct mstudioautolayerv54_t
{
	// this needs to have a guid descriptor in rpak
	int64_t guidSequence; // hashed aseq guid asset

	short iSequence; // only used within an rmdl I would imagine
	short iPose;

	int flags;
	float start;	// beginning of influence
	float peak;	// start of full influence
	float tail;	// end of full influence
	float end;	// end of all influence
};

struct mstudioeventv54_t
{
	float cycle;
	int	event;
	int type; // this will be 0 if old style I'd imagine
	char options[256];
	int szeventindex;
};

// rseq v10
struct mstudioeventv54_t_v122
{
	float cycle;
	int	event;
	eventtype type; // this will be 0 if old style I'd imagine

	int unk;

	char options[256]; // this is the only difference compared to normal v54
	int szeventindex;
};

struct mstudioeventv54_t_v16
{
	float cycle;
	int	event;
	int type; // this will be 0 if old style I'd imagine
	int unk;
	short szoptionsindex;
	short szeventindex;
};

// fake type
struct mstudioeventv54
{
	float cycle;
	int	event;
	int type;
	string szoptions;
	string szevent;

	inline void Init(int version, IO::BinaryReader& Reader)
	{
		uint64_t oldpos = Reader.GetBaseStream()->GetPosition();

		mstudioeventv54_t event = Reader.Read<mstudioeventv54_t>();;
		this->cycle = event.cycle;
		this->event = event.event;
		this->type = event.type;

		Reader.GetBaseStream()->SetPosition(oldpos);

		InitOptions(version, Reader);
		InitEvent(version, Reader);
		return;
	}
	
private:
	inline void InitOptions(int version, IO::BinaryReader& Reader)
	{
		uint64_t oldpos = Reader.GetBaseStream()->GetPosition();
		switch (version)
		{
		case 7:
		{
			this->szoptions = string(Reader.Read<mstudioeventv54_t>().options);
			break;
		}

		case 10:
		{
			this->szoptions = string(Reader.Read<mstudioeventv54_t_v122>().options);
			break;
		}
		case 11:
		{
			mstudioeventv54_t_v16 event = Reader.Read<mstudioeventv54_t_v16>();
			Reader.GetBaseStream()->SetPosition(oldpos + event.szoptionsindex);
			this->szoptions = Reader.ReadCString();
			break;
		}
		default:
			break;
		}

		Reader.GetBaseStream()->SetPosition(oldpos);
		return;
	}

	inline void InitEvent(int version, IO::BinaryReader& Reader)
	{
		uint64_t oldpos = Reader.GetBaseStream()->GetPosition();
		int index = 0;
		switch (version)
		{
		case 7:
			index = Reader.Read<mstudioeventv54_t>().szeventindex;
			break;
		case 10:
			index = Reader.Read<mstudioeventv54_t_v122>().szeventindex;
			break;
		case 11:
			index = Reader.Read<mstudioeventv54_t_v16>().szeventindex;
			break;
		default:
			break;
		}

		if (index)
		{
			Reader.GetBaseStream()->SetPosition(oldpos + index);
			this->szevent = Reader.ReadCString();
		}

		Reader.GetBaseStream()->SetPosition(oldpos);

		return;
	}
};

struct mstudioseqdesc_t_v16
{
	short szlabelindex;

	short szactivitynameindex;

	int flags; // looping/non-looping flags

	short activity; // initialized at loadtime to game DLL values
	short actweight;

	short numevents;
	short eventindex;

	Vector3 bbmin; // per sequence bounding box
	Vector3 bbmax;

	short numblends;

	// Index into array of shorts which is groupsize[0] x groupsize[1] in length
	short animindexindex;

	//short movementindex; // [blend] float array for blended movement
	short paramindex[2]; // X, Y, Z, XR, YR, ZR
	float paramstart[2]; // local (0..1) starting value
	float paramend[2]; // local (0..1) ending value
	//short paramparent;

	float fadeintime; // ideal cross fate in time (0.2 default)
	float fadeouttime; // ideal cross fade out time (0.2 default)

	byte fill[4];

	// stuff is different after this
	/*
	short localentrynode; // transition node at entry
	short localexitnode; // transition node at exit
	short nodeflags; // transition rules

	float entryphase; // used to match entry gait
	float exitphase; // used to match exit gait

	float lastframe; // frame that should generation EndOfSequence

	short nextseq; // auto advancing sequences
	short pose; // index of delta animation between end and nextseq*/

	short numikrules;

	short numautolayers;
	uint16 autolayerindex;

	uint16 weightlistindex;

	byte groupsize[2];

	uint16 posekeyindex;

	short numiklocks;
	short iklockindex;

	// Key values
	uint16 keyvalueindex;
	short keyvaluesize;

	//short cycleposeindex; // index of pose parameter to use as cycle index

	short activitymodifierindex;
	short numactivitymodifiers;

	int unk;
	int unk1;

	uint16 unkindex;
	short unkcount;
};


struct mstudioseqdesc_t
{
	int baseptr;

	int	szlabelindex;

	int szactivitynameindex;

	int flags; // looping/non-looping flags

	int activity; // initialized at loadtime to game DLL values
	int actweight;

	int numevents;
	int eventindex;

	Vector3 bbmin; // per sequence bounding box
	Vector3 bbmax;

	int numblends;

	// Index into array of shorts which is groupsize[0] x groupsize[1] in length
	int animindexindex;

	int movementindex; // [blend] float array for blended movement
	int groupsize[2];
	int paramindex[2]; // X, Y, Z, XR, YR, ZR
	Vector2 paramstart; // local (0..1) starting value
	Vector2 paramend; // local (0..1) ending value
	int paramparent;

	float fadeintime; // ideal cross fate in time (0.2 default)
	float fadeouttime; // ideal cross fade out time (0.2 default)

	int localentrynode; // transition node at entry
	int localexitnode; // transition node at exit
	int nodeflags; // transition rules

	float entryphase; // used to match entry gait
	float exitphase; // used to match exit gait

	float lastframe; // frame that should generation EndOfSequence

	int nextseq; // auto advancing sequences
	int pose; // index of delta animation between end and nextseq

	int numikrules;

	int numautolayers;
	int autolayerindex;

	int weightlistindex;

	int posekeyindex;

	int numiklocks;
	int iklockindex;

	// Key values
	int keyvalueindex;
	int keyvaluesize;

	int cycleposeindex; // index of pose parameter to use as cycle index

	int activitymodifierindex;
	int numactivitymodifiers;

	int unk;
	int unk1;

	int unkindex;

	int unk2;

	inline mstudioseqdesc_t_v16 Upgrade()
	{
		mstudioseqdesc_t_v16 Out{};

		Out.szlabelindex = this->szlabelindex;
		Out.szactivitynameindex = this->szactivitynameindex;
		Out.flags = this->flags;
		Out.activity = this->activity;
		Out.actweight = this->actweight;
		Out.numevents = this->numevents;
		Out.eventindex = this->eventindex;
		Out.bbmin = this->bbmin;
		Out.bbmax = this->bbmax;
		Out.numblends = this->numblends;
		Out.animindexindex = this->animindexindex;
		Out.groupsize[0] = this->groupsize[0];
		Out.groupsize[1] = this->groupsize[1];
		Out.paramindex[0] = this->paramindex[0];
		Out.paramindex[1] = this->paramindex[1];
		Out.paramstart[0] = this->paramstart[0];
		Out.paramstart[1] = this->paramstart[1];
		Out.paramend[0] = this->paramend[0];
		Out.paramend[1] = this->paramend[1];
		Out.fadeintime = this->fadeintime;
		Out.fadeouttime = this->fadeouttime;
		Out.numikrules = this->numikrules;
		Out.numautolayers = this->numautolayers;
		Out.autolayerindex = this->autolayerindex;
		Out.weightlistindex = this->weightlistindex;
		Out.posekeyindex = this->posekeyindex;
		Out.numiklocks = this->numiklocks;
		Out.iklockindex = this->iklockindex;
		Out.keyvalueindex = this->keyvalueindex;
		Out.keyvaluesize = this->keyvaluesize;
		Out.activitymodifierindex = this->activitymodifierindex;
		Out.numactivitymodifiers = this->numactivitymodifiers;
		Out.unk = this->unk;
		Out.unk1 = this->unk1;
		Out.unkindex = this->unkindex;

		return Out;
	}
};

struct mstudioanimdescv54_t_v16
{
	float fps; // frames per second	
	int flags; // looping/non-looping flags

	short numframes;

	// piecewise movement
	//short nummovements;
	//short movementindex;

	short unk_v16;

	uint16 sznameindex;

	uint16 compressedikerrorindex;

	int animindex; // non-zero when anim data isn't in sections

	short numikrules;
	uint16 ikruleindex; // non-zero when IK data is stored in the mdl

	uint16 unk1[5];

	uint16 sectionindex;

	uint16 unk2; // what, obviously section related as it's wedged between sectionindex and sectiom frames

	uint16 sectionframes; // number of frames used in each fast lookup section, zero if not used
};

// rseq v7.1
struct mstudioanimdescv54_t_v121
{
	int baseptr;

	int sznameindex;

	float fps; // frames per second
	int flags; // looping/non-looping flags

	int numframes;

	// piecewise movement
	int nummovements;
	int movementindex;

	int compressedikerrorindex;
	int animindex; // non-zero when anim data isn't in sections

	int numikrules;
	int ikruleindex; // non-zero when IK data is stored in the mdl

	int sectionindex;

	int unk; // what, obviously section related as it's wedged between sectionindex and sectiom frames

	int sectionframes; // number of frames used in each fast lookup section, zero if not used

	int unk1[4];

	//int unk1; // Padding
	//int unk2; // Padding
	//int unk3; // SomeDataOffset
	//int unk4; // SomeDataOffset

	// it seems like there's another int here but I'm unsure

	inline mstudioanimdescv54_t_v16 Upgrade()
	{
		mstudioanimdescv54_t_v16 out{};
		out.fps = this->fps;
		out.flags = this->flags;
		out.numframes = this->numframes;
		out.sznameindex = this->sznameindex;
		out.compressedikerrorindex = this->compressedikerrorindex;
		out.animindex = this->animindex;
		out.numikrules = this->numikrules;
		out.ikruleindex = this->ikruleindex;
		out.sectionindex = this->sectionindex;
		out.sectionframes = this->sectionframes;
		return out;
	}
};

// this struct is the wrong size
struct mstudioanimdescv54_t
{
	int baseptr;

	int sznameindex;

	float fps; // frames per second
	int flags; // looping/non-looping flags

	int numframes;

	// piecewise movement
	int nummovements;
	int movementindex;

	int compressedikerrorindex;
	int animindex; // non-zero when anim data isn't in sections

	int numikrules;
	int ikruleindex; // non-zero when IK data is stored in the mdl

	int sectionindex;
	int sectionframes; // number of frames used in each fast lookup section, zero if not used
	int mediancount;
	uint64_t padding;
	uint64_t somedataoffset;

	inline mstudioanimdescv54_t_v16 Upgrade()
	{
		mstudioanimdescv54_t_v16 out{};
		out.fps = this->fps;
		out.flags = this->flags;
		out.numframes = this->numframes;
		out.sznameindex = this->sznameindex;
		out.compressedikerrorindex = this->compressedikerrorindex;
		out.animindex = this->animindex;
		out.numikrules = this->numikrules;
		out.ikruleindex = this->ikruleindex;
		out.sectionindex = this->sectionindex;
		out.sectionframes = this->sectionframes;
		return out;
	}
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

struct mstudioanimsectionsv53_t
{
	int animindex;
};

struct mstudioanimsectionsv54_t_v121
{
	int animindex;
	int isExternal; // 0 or 1, if 1 section is not in rseq (I think)
};


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