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
    - [Controls](#controls)
  - [Support](#support)
  - [Known Issues](#known-issues)

## Installation
To install Legion+, you may choose to either download the latest pre-compiled release from [the releases page](https://github.com/r-ex/LegionPlus/releases/latest), or compile the source code for yourself using the provided Visual Studio Solution file (.sln)

Compilation is currently only supported on Windows due to some platform-specific libraries that are required

## Usage

### Command Line Options

#### Modes
```
--export <path to .rpak or .mbnk>
Exports the specified rpak or Audio file according to your saved configuration, unless load flags are provided

--list <path to .rpak or .mbnk>
Produces a list of all exportable assets within the specified .rpak or .mbnk file
```

#### Export Load Flags
When any load flag is used, your saved configuration is ignored and only the specified flags are used
When multiple load flags are used together, all specified types will be loaded

```
--loadmodels
--loadanimations
--loadanimationseqs
--loadimages
--loadmaterials
--loaduiimages
--loaddatatables
--loadshadersets
--loadsettingssets
--loadrsons
```

#### Export Format Flags

```
Models: --mdlfmt <semodel, obj/wavefront, xnalara_ascii, xnalara_binary, smd/source, xmodel, maya/ma, fbx, cast, rmdl>
Animations: --animfmt <seanim, cast, ranim>
Images: --imgfmt <dds, png, tiff, tga>
Text: --textfmt <csv, txt>
Shadersets: --nmlrecalc <none, directx/dx, opengl/ogl>
Audio Language: --audiolanguage <english, french, german, spanish, italian, japanese, polish, russian, mandarin, korean>
MatCPU: --matcpu <none, struct, cpu>
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
--skinexport - Enables exporting of all skins for available models
```
---
### Controls
Asset List
```
Keyboard
P - open preview window for currently selected asset
E - extract currently selected assets

Mouse
Right Click - copies the names of the currently selected assets to clipboard
```

Preview
```
Alt + Left Click - move mouse to pivot camera around target
Alt + Right Click - move mouse to zoom in and out of the target
Alt + Middle Click - move mouse to pan camera
```
---

## Support
If you encounter any issues or errors during your usage of Legion+, please let us know by opening a new Issue and providing as much detail as possible.

We also have a [discord server](https://discord.gg/ADek6fxVGe) where you will be able to directly ask for support and receive updates about the project

## Known Issues

Full TODO/task list is available [here.](https://github.com/r-ex/LegionPlus/projects/1)
