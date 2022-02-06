# LegionPlus

Apex Legends/Titanfall 2 asset extraction tool

---

## Command Line Usage:

### Modes:
```
--export <path to rpak>
Exports the specified rpak according to your saved configuration, unless load flags are provided

--list   <path to rpak>
Produces a list of all exportable assets within the specified rpak
```

### Export Load Flags:
When any load flag is used, your saved configuration is ignored and only the specified flags are used
When multiple load flags are used together, all specified types will be loaded

```
--loadmodels
--loadanimations
--loadimages
--loadmaterials
--loaduiimages
--loaddatatables
```

### Other Flags:

```
--overwrite - Enables file overwriting for replacing existing versions of exported assets
```


copyright 2021 DTZxPorter.