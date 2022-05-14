Legion+
=======

Asset extraction tool for Apex Legends and Titanfall 2

Originally created by DTZxPorter in 2019.

---
## Table of Contents
- [Legion+](#legion)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
  - [Usage](#usage)
    - [Command Line Options](#command-line-options)
      - [Modes](#modes)
      - [Export Load Flags](#export-load-flags)
      - [Export Format Flags](#export-format-flags)
      - [Other Flags](#other-flags)
  - [Support](#support)
  - [Known Issues](#known-issues)

## Installation
To install Legion+, you may choose to either download the latest pre-compiled release from [the releases page](https://github.com/r-ex/LegionPlus/releases/latest), or compile the source code for yourself using the provided Visual Studio Solution file (.sln)

Compilation is currently only supported on Windows due to some platform-specific libraries that are required

## Usage

### Command Line Options

#### Modes
```
--export <path to rpak>
Exports the specified rpak according to your saved configuration, unless load flags are provided

--exportaudio <path to general.mbnk>
Exports the Audio files

--list <path to rpak>
Produces a list of all exportable assets within the specified rpak
```

#### Export Load Flags
When any load flag is used, your saved configuration is ignored and only the specified flags are used
When multiple load flags are used together, all specified types will be loaded

```
--loadmodels
--loadanimations
--loadimages
--loadmaterials
--loaduiimages
--loaddatatables
--loadshadersets
```

#### Export Format Flags

```
Models: --mdlfmt <semodel, obj/wavefront, xnalara_binary, smd/source, xmodel, maya/ma, fbx, cast, rmdl>
Animations: --animfmt <seanim, cast, ranim>
Images: --imgfmt <dds, png, tiff>
Text: --textfmt <csv, txt>
Shadersets: --nmlrecalc <none, directx/dx, opengl/ogl>
Audio Language: --audiolanguage <english, french, german, spanish, italian, japanese, polish, russian, mandarin, korean>
```
`Example: LegionPlus.exe --export <path to rpak> --loadmodels --loadanimations --mdlfmt obj --animfmt seanim --imgfmt png`

#### Other Flags
```
--overwrite - Enables file overwriting for replacing existing versions of exported assets
--nologfile - Disables log files being created
--prioritylvl - Sets Priority Level by using: <realtime, high, above_normal, normal, below_normal, idle>
--fullpath - Enables full path naming for the list export
--audiolanguagefolder - Enables Audio Language Folder
--usetxtrguids - Enables the renaming of Guid names for Textures (e.g. adding _albedoTexture, etc.)
```

## Support
If you encounter any issues or errors during your usage of Legion+, please let us know by opening a new Issue and providing as much detail as possible.

We also have a [discord server](https://discord.gg/ADek6fxVGe) where you will be able to directly ask for support and receive updates about the project

## Known Issues

Full TODO/task list is available [here.](https://github.com/r-ex/LegionPlus/projects/1)
