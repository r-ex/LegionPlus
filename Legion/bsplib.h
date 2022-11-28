#pragma once

#include "StringBase.h"
#include "ListBase.h"
#include "DictionaryBase.h"
#include "MemoryStream.h"
#include "FileStream.h"
#include "BinaryReader.h"

#include "RpakAssets.h"
#include "ApexAsset.h"

#include "Model.h"
#include "Exporter.h"
#include "RpakLib.h"
#include "Path.h"


enum class ApexRBspLumps
{
    TEXTURES = 0x2,
    VERTICES = 0x3,
    MODELS = 0xE,
    SURFACE_NAMES = 0xF,
    NORMALS = 0x1E,
    GAME_LUMPS = 0x23,
    VERTEX_UNLIT = 0x47,
    VERTEX_LIT_FLAT = 0x48,
    VERTEX_LIT_BUMP = 0x49,
    VERTEX_UNLIT_TS = 0x4A,
    FACES = 0x4F,
    MESHES = 0x50,
    MATERIALS = 0x52
};

enum class TF2RBspLumps
{
    TEXTURES = 0x2,
    VERTICES = 0x3,
    MODELS = 0xE,
    NORMALS = 0x1E,
    GAME_LUMPS = 0x23,
    SURFACE_NAMES = 0x2B,
    VERTEX_UNLIT = 0x47,
    VERTEX_LIT_FLAT = 0x48,
    VERTEX_LIT_BUMP = 0x49,
    VERTEX_UNLIT_TS = 0x4A,
    FACES = 0x4F,
    MESHES = 0x50,
    MATERIALS = 0x52
};

#define R2_BSP_VERSION 0x25

enum apexlumps_t
{
    LUMP_ENTITIES = 0x0000, // 0
    LUMP_PLANES = 0x0001, // 1
    LUMP_TEXDATA = 0x0002, // 2
    LUMP_VERTEXES = 0x0003, // 3
    LUMP_LIGHTPROBE_PARENT_INFOS = 0x0004,
    LUMP_SHADOW_ENVIRONMENTS = 0x0005,
    LUMP_UNUSED_6 = 0x0006,
    LUMP_UNUSED_7 = 0x0007,
    LUMP_UNUSED_8 = 0x0008,
    LUMP_UNUSED_9 = 0x0009,
    LUMP_UNUSED_10 = 0x000A,
    LUMP_UNUSED_11 = 0x000B,
    LUMP_UNUSED_12 = 0x000C,
    LUMP_UNUSED_13 = 0x000D,
    LUMP_MODELS = 0x000E,
    LUMP_TEXDATA_STRING_DATA = 0x000F, // LUMP_TEXDATA_STRING_DATA
    LUMP_CONTENTS_MASKS = 0x0010,
    LUMP_SURFACE_PROPERTIES = 0x0011,
    LUMP_BVH_NODES = 0x0012,
    LUMP_BVH_LEAF_DATA = 0x0013,
    LUMP_PACKED_VERTICES = 0x0014,
    LUMP_UNUSED_21 = 0x0015,
    LUMP_UNUSED_22 = 0x0016,
    LUMP_UNUSED_23 = 0x0017,
    LUMP_ENTITYPARTITIONS = 0x0018, // 24
    LUMP_UNUSED_25 = 0x0019,
    LUMP_UNUSED_26 = 0x001A,
    LUMP_UNUSED_27 = 0x001B,
    LUMP_UNUSED_28 = 0x001C,
    LUMP_UNUSED_29 = 0x001D,
    LUMP_VERTNORMALS = 0x001e, // 30
    LUMP_UNUSED_31 = 0x001F,
    LUMP_UNUSED_32 = 0x0020,
    LUMP_UNUSED_33 = 0x0021,
    LUMP_UNUSED_34 = 0x0022,
    LUMP_GAME_LUMP = 0x0023,
    LUMP_UNUSED_36 = 0x0024,
    LUMP_UNKNOWN_37 = 0x0025,  // connected to VIS lumps
    LUMP_UNKNOWN_38 = 0x0026,  // connected to CSM lumps
    LUMP_UNKNOWN_39 = 0x0027,  // connected to VIS lumps
    LUMP_PAKFILE = 0x0028,  // zip file, contains cubemaps
    LUMP_UNUSED_41 = 0x0029,
    LUMP_CUBEMAPS = 0x002a, // 42
    LUMP_UNKNOWN_43 = 0x002B,
    LUMP_UNUSED_44 = 0x002C,
    LUMP_UNUSED_45 = 0x002D,
    LUMP_UNUSED_46 = 0x002E,
    LUMP_UNUSED_47 = 0x002F,
    LUMP_UNUSED_48 = 0x0030,
    LUMP_UNUSED_49 = 0x0031,
    LUMP_UNUSED_50 = 0x0032,
    LUMP_UNUSED_51 = 0x0033,
    LUMP_UNUSED_52 = 0x0034,
    LUMP_UNUSED_53 = 0x0035,
    LUMP_WORLD_LIGHTS = 0x0036,
    LUMP_WORLD_LIGHT_PARENT_INFOS = 0x0037,
    LUMP_UNUSED_56 = 0x0038,
    LUMP_UNUSED_57 = 0x0039,
    LUMP_UNUSED_58 = 0x003A,
    LUMP_UNUSED_59 = 0x003B,
    LUMP_UNUSED_60 = 0x003C,
    LUMP_UNUSED_61 = 0x003D,
    LUMP_UNUSED_62 = 0x003E,
    LUMP_UNUSED_63 = 0x003F,
    LUMP_UNUSED_64 = 0x0040,
    LUMP_UNUSED_65 = 0x0041,
    LUMP_UNUSED_66 = 0x0042,
    LUMP_UNUSED_67 = 0x0043,
    LUMP_UNUSED_68 = 0x0044,
    LUMP_UNUSED_69 = 0x0045,
    LUMP_UNUSED_70 = 0x0046,
    LUMP_VERTS_UNLIT = 0x0047, // 71
    LUMP_VERTS_LIT_FLAT = 0x0048, // 72
    LUMP_VERTS_LIT_BUMP = 0x0049, // 73
    LUMP_VERTS_UNLIT_TS = 0x004a, // 74
    LUMP_VERTS_BLINN_PHONG = 0x004b, // 75
    LUMP_VERTS_RESERVED_5 = 0x004c, // 76
    LUMP_VERTS_RESERVED_6 = 0x004d, // 77
    LUMP_VERTS_RESERVED_7 = 0x004e, // 78
    LUMP_MESH_INDICES = 0x004f, // 79
    LUMP_MESHES = 0x0050, // 80
    LUMP_MESH_BOUNDS = 0x0051, // 81
    LUMP_MATERIAL_SORT = 0x0052, // 82
    LUMP_LIGHTMAP_HEADERS = 0x0053, // 83
    LUMP_UNUSED_84 = 0x0054,
    LUMP_TWEAK_LIGHTS = 0x0055,
    LUMP_UNUSED_86 = 0x0056,
    LUMP_UNUSED_87 = 0x0057,
    LUMP_UNUSED_88 = 0x0058,
    LUMP_UNUSED_89 = 0x0059,
    LUMP_UNUSED_90 = 0x005A,
    LUMP_UNUSED_91 = 0x005B,
    LUMP_UNUSED_92 = 0x005C,
    LUMP_UNUSED_93 = 0x005D,
    LUMP_UNUSED_94 = 0x005E,
    LUMP_UNUSED_95 = 0x005F,
    LUMP_UNUSED_96 = 0x0060,
    LUMP_UNKNOWN_97 = 0x0061,
    LUMP_LIGHTMAP_DATA_SKY = 0x0062, // 98
    LUMP_CSM_AABB_NODES = 0x0063, // 99
    LUMP_CSM_OBJ_REFS = 0x0064, // 100
    LUMP_LIGHTPROBES = 0x0065, // 101
    LUMP_STATIC_PROP_LIGHTPROBE_INDEX = 0x0066, // 102
    LUMP_LIGHTPROBETREE = 0x0067, // 103
    LUMP_LIGHTPROBEREFS = 0x0068, // 104
    LUMP_LIGHTMAP_DATA_REAL_TIME_LIGHTS = 0x0069, // 105
    LUMP_CELL_BSP_NODES = 0x006a, // 106
    LUMP_CELLS = 0x006b, // 107
    LUMP_PORTALS = 0x006c, // 108
    LUMP_PORTAL_VERTS = 0x006d, // 109
    LUMP_PORTAL_EDGES = 0x006e, // 110
    LUMP_PORTAL_VERT_EDGES = 0x006f, // 111
    LUMP_PORTAL_VERT_REFS = 0x0070, // 112
    LUMP_PORTAL_EDGE_REFS = 0x0071, // 113
    LUMP_PORTAL_EDGE_ISECT_EDGE = 0x0072, // 114
    LUMP_PORTAL_EDGE_ISECT_AT_VERT = 0x0073, // 115
    LUMP_PORTAL_EDGE_ISECT_HEADER = 0x0074, // 116
    LUMP_OCCLUSIONMESH_VERTS = 0x0075, // 117
    LUMP_OCCLUSIONMESH_INDICES = 0x0076, // 118
    LUMP_CELL_AABB_NODES = 0x0077, // 119
    LUMP_OBJ_REFS = 0x0078, // 120
    LUMP_OBJ_REF_BOUNDS = 0x0079, // 121
    LUMP_LIGHTMAP_DATA_RTL_PAGE = 0x007A,
    LUMP_LEVEL_INFO = 0x007B,
    LUMP_SHADOW_MESH_OPAQUE_VERTS = 0x007C,
    LUMP_SHADOW_MESH_ALPHA_VERTS = 0x007D,
    LUMP_SHADOW_MESH_INDICES = 0x007E,
    LUMP_SHADOW_MESH_MESHES = 0x007F,
};


#pragma pack(push, 1)
struct lump_t
{
    int fileofs;
    int filelen;
    int version;
    int uncompLen;
};

struct BSPHeader_t
{
    int ident;
    short version;
    short bExternal; // yea this would be better as a bool and char padding but idc so cry about it
    int mapRevision;
    int lastLump;

    lump_t lumps[128];
};

// gamelump
struct dgamelumpheader_t
{
    int lumpCount;
};

struct dgamelump_t
{
    int id;
    unsigned short flags;
    unsigned short version;
    int fileofs;
    int filelen;
};

namespace apexlegends
{
    struct dmodel_t
    {
        Vector3 mins;
        Vector3 maxs;
        int firstMesh;
        int meshCount;
        int unk[8];
    };

    struct dtexdata_t
    {
        int nameStringTableID;
        int width;
        int height;
        int flags;
    };

    struct dvertLitBump
    {
        int posIdx;
        int nmlIdx;
        Vector2 tex; // texture uvs
        char color[4]; // i think this is correct
        Vector2 lmap; // lightmap uvs?
        char unk[4];
    };

    struct dvertUnlitTS
    {
        int posIdx;
        int nmlIdx;
        Math::Vector2 tex;
        int unk[2];
    };
}

// most of these will be tf1 as well, but tf1 isnt supported (yet:TM:)
namespace titanfall2
{
    struct dmodel_t
    {
        Vector3 mins;
        Vector3 maxs;
        int firstMesh;
        int meshCount;
    };

    struct dtexdata_t
    {
        Vector3 reflectivity;
        int nameStringTableID;
        int width;
        int height;
        int view_width;
        int view_height;
        int flags;
    };

    struct dvertLitBump
    {
        int posIdx;
        int nmlIdx;
        Math::Vector2 tex; // texture uvs
        Math::Vector2 UVs2;
        Math::Vector2 UVs3;
        int unk[3];
    };

    struct dvertUnlitTS
    {
        int posIdx;
        int nmlIdx;
        Math::Vector2 tex;
        int unk[3];
    };
}

struct dmesh_t
{
    int firstIdx;
    short triCount;
    short firstVtxRel;
    short lastVtxOfs;
    char vtxType;
    char unused_0;
    char lightStyles[4];
    short luxelOrg[2];
    char luxelOfsMax[2];
    short mtlSortIdx;
    int flags;
};

struct dmaterialsort_t
{
    short texdata;
    short lmapIdx;
    short cubemapIdx;
    short lastVtxOfs;
    int firstVertex;
};

struct dvertUnlit
{
    int posIdx;
    int nmlIdx;
    Math::Vector2 tex;
    char color[4];
};

struct dvertLitFlat
{
    int posIdx;
    int nmlIdx;
    Math::Vector2 tex;
    char color[4];
};

struct StaticPropLump_t
{
    Math::Vector3 origin;
    Math::Vector3 angles;
    float scale;
    short nameIdx;

    char unk[0x22];
};
#pragma pack(pop)

#define MESH_VERTEX_LIT_FLAT 0x000
#define MESH_VERTEX_LIT_BUMP 0x200
#define MESH_VERTEX_UNLIT    0x400
#define MESH_VERTEX_UNLIT_TS 0x600

// void ExportApexBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, std::unique_ptr<IO::FileStream>& Stream, BSPHeader_t& Header, const string& Asset, const string& Path);
typedef void(*exportFunc_t)(const std::unique_ptr<RpakLib>& rpakFS, std::unique_ptr<IO::FileStream>& stream, BSPHeader_t& header, const string& bspPath, const string& exportPath);

struct game_t
{
    const char* name;
    int minVersion;
    int maxVersion;
    int ident;

    exportFunc_t exportFunc;
};
inline std::vector<game_t> s_BSPGameFormats;
inline std::unique_ptr<Assets::Exporters::Exporter> s_BSPModelExporter;

void ExportBsp(const std::unique_ptr<RpakLib>& RpakFileSystem, const string& Asset, const string& Path);

// Initializes a model exporter
void InitializeBSPModelExporter(ModelExportFormat_t Format = ModelExportFormat_t::SEModel);

// Exports an on-disk bsp asset, returning list of prop model names
List<string> ExportPropContainer(std::unique_ptr<IO::MemoryStream>& Stream, const string& Name, const string& Path);

template<typename T>
static void ReadExternalLumpFile(const string& BasePath, uint32_t Lump, uint32_t DataSize, List<T>& Data)
{
    string Path = string::Format("%s.bsp.%04x.bsp_lump", BasePath.ToCString(), Lump);

    if (IO::File::Exists(Path))
        IO::File::OpenRead(Path)->Read((uint8_t*)&Data[0], 0, DataSize);
}

static void ReadExternalLumpArrayFile(const string& BasePath, uint32_t Lump, uint32_t DataSize, uint8_t* Data)
{
    string Path = string::Format("%s.bsp.%04x.bsp_lump", BasePath.ToCString(), Lump);

    if (IO::File::Exists(Path))
        IO::File::OpenRead(Path)->Read(Data, 0, DataSize);
    else
    {
        string ex = "Couldn't find file " + IO::Path::GetFileName(Path) + "\nMake sure you have exported all .bsp_lump files for this map";
        throw std::exception(ex.ToCString());
    }

}

#define ADD_BSP_FORMAT(x, y) auto __bsp_format_##x = s_BSPGameFormats.emplace_back(x)
#define XREGISTER_BSP(x, y) ADD_BSP_FORMAT(x,y)
#define REGISTER_BSP(x) XREGISTER_BSP(x, __COUNTER__)

#define XREADLUMP(X, D) \
			if (helper.header.bExternal) { \
				ReadExternalLumpFile(BasePath, D, helper.header.lumps[D].filelen, X); \
			} else { \
				Stream->get()->SetPosition(helper.header.lumps[D].fileofs); \
				Stream->get()->Read((uint8_t*)&X[0], 0, helper.header.lumps[D].filelen); \
			}

//auto modelsLumpData = List<dmodel_t>(header.lumps[LUMP_MODELS].filelen / sizeof(dmodel_t), true);
//READ_LUMP(modelsLumpData, LUMP_MODELS);

struct LumpLoadHelper_t
{
    BSPHeader_t& header;
    std::unique_ptr<IO::FileStream>* Stream;
    string Path;
};

template<typename T>
List<T> ReadLump(LumpLoadHelper_t& helper, int lump)
{
    List<T> list = List<T>(helper.header.lumps[lump].filelen / sizeof(T), true);


    string BasePath = helper.Path;
    std::unique_ptr<IO::FileStream>* Stream = helper.Stream;

    XREADLUMP(list, lump);

    return list;
}
