# obj2hmap

Very simple convertor of Wavefront's obj files to displacement map (projected on one of the main 
XYZ planes).

This tool grew up out of my personal frustration with Blender, myself unable to import an obj file
and make depth texture, render and whatever else. Too much hassle. So, I have spent few days to
mash up (and straighten a bit later) this tool. It is really one file-show.

As always the best documentation is the code itself. And this code is small - <1k lines of code with
the huge Doxygen comments around.

Processing 1GiB obj file for 4k by 4k vertices, consumes like 20-30 seconds. Most of which are to
read the file itself.

## hmap2obj

There is one more tool created: hmap2obj

Its purpose is basically to convert a simple binary heightmap file of 16 bit unsigned values into an
Wavefront obj file. This way, the cycle is closed. Again this tool was born just to escape using 
GUI tools and clicking here and there, loosing time.

## Requirements

Any C++11/14 conformant compiler. Didn't tried it, but I think even GCC 4.9 will suffice. During my
development I used GCC 6.2. No other external dependencies.

```
c++ obj2hmap.cpp -o obj2hmap -O2
```

Is enough. You can skip even the optimization level `-O2` and the named executable `-o obj2hmap`.

I have tested with Visual Studio Community 2015 and succeeded to build:

```
cl /EHsc obj2hmap.cpp
```

The `/EHsc` apparently turns on some C++ exception mechanism in Visual C++ compiler.

## Usage obj2hmap

```
obj2hmap <OBJ> <HMAP> <SIZE_X> <SIZE_Y> <SIZE_Z> <AXIS> [OBJ_HEIGHT] [HMAP_TYPE]
```

* OBJ 
  Is the Wavefront's formatted file. The tools reads from it and caches the vertices. Can be any
  file system path.
* HMAP 
  Is the destination heightmap/displacement map file. We dump there the chosen plane coordinates.
  Can be any file system path.
* SIZE XYZ 
  Are the heightmap's dimensions into which we will squash the OBJ vertices. The numbers should be
  anything larger than 0 and less than the 32-bit range. You can put and oct/dec/hex formatted ones.
* AXIS one of `x`, `y` or `z` 
  That's the OBJ vertices coordinate which will hold the height values for our final map. Or you can
  say elevation data.
* OBJ HEIGHT as two floating point values. 
  By default, the min/max height obj values are are mapped to SIZE parameter. So if you have a
  terrain of 0.0 to 0.5, 0.5 will equal the SIZE param (e.g. 0xFFFF). If you specify explicitly a
  bigger range it will be used instead. For example, instead of 0.0 to 0.5 you can specify -1 to +1.
  This will map -1 as 0 and +1 as 0xFFFF. This option is of particular interest when your object
  file in fact is piece of another, bigger heightmap. In this case, whatever changes you do in the
  OBJ file, you can merge back the heightmaps - otherwise you will get pieces of very contrast 
  patches. Note, that if one of the OBJ min or max height values is bigger than the corresponding
  OBJ HEIGHT param - it will be used instead.
* HMAP TYPE one of `u8`, `u16`, `u32`, `f32`, `tu8`, `tu16`, `tu32` or `tf32`.
  Is the type of values to dump into the heightmap file. The `u` prefix means unsigned, the `f`
  means floating point value, the number is the bit size and the `t` prefix means to output in text
  format and not binary values.

## Example obj2hmap

```
obj2hmap terrain.obj terrain.r16 4097 0xFFFF 4097 y u16 0.0 0.02
```

This will convert my ZBrush exported tool, into terrain of 4k size. It is 4097 and not 4096 just to
demonstrate the not power of 2 ability :)

Then as in my case, the Y axis shows the actual elevation of the terrain, I have put `0xFFFF` or the
max 16 bit unsigned value - this will force the tool to scale or stretch my elevation into the full
range of 16 bit.

The `y` part basically says where is the elevation data in the OBJ file.

The `u16` is optional and this is the default assumption. It will dump binary terrain of 16 bit
values - quite common nowdays. Later it can be converted to image by some app like Gimp/Krita or fed
up to some virtual engine.

Then the last two parameters `0.0` and `0.02` were specified so that this terrain does not occupy
the full 16-bit range, but only a part of it. The actual occupation zone depends on the OBJ height
values in the file. If in OBJ there are values between 0.01 and 0.02, then this means that the end
binary 16-bit heightmap values will be from 50% to 100% of 0xFFFF (50% because 0.01 is just half
of 0.02).

## Usage hmap2obj

```
hmap2obj <HMAP> <OBJ> <SIZE_X> <SIZE_Y> <OBJ_LOW_XYZ> <OBJ_HIGH_XYZ> [--absolute]
```

* HMAP 
  Is binary raw heightmap values. Such file can be obtains from different terrain or even raster
  editor tools. This file will be read from.
* OBJ 
  Is the destination Wavefront's object file. It will contain vertices and faces indices. Vertices
  are of fixed width, double floating point data as maximum precision is searched for.
* SIZE XY 
  Are two unsigned integer values, saying how big is the heightmap data. Usually the product of
  these two should give the HMAP byte size. Anything bigger than that is not read.
* OBJ LOW HIGH XYZ 
  Are two 3d floating point coordinates. They specify the dimension into which to put heightmap. It
  is normal in most software X and Z to be -0.5 and +0.5. The Y direction is considerd the up or the
  elevation axis. Note that if you put too high value here, the vectorized terrain could look very
  distorted - if too low value, too flat. The proper way to calc this parameter is to relate the
  highest and lowest points to the size of the terrain.
* absolute
  Is an optional boolean switch. By default, the min/max height values found in HMAP are used to
  stretch map it to the OBJ LOW/HIGH Y range. So if in the heightmap 1000 is the lowest and 20000 is
  the highest value, you will have a range of 19000 to into the OBJ LOW/HIGH Y range. Assuming OBJ
  LOW Y to be 0 and OBJ HIGH Y to be 1, you will have 1/19000 ratio - 1000 will be 0.0 in the OBJ
  file and 20000 will be 1.0 in the OBJ file. Now, if you use this switch, the heightmap values will
  be stretched over the full 16-bit range instead. In the given example, you will have 1/65535 ratio
  and 1000 will be ~0.15259 and 20000 will be ~0.305180 instead. This option is of interest when the
  heightmap is actually a piece of bigger map. Then as the parent map is in the full 16-bit range
  each of its tiles should be kept relative.

## Example hmap2obj

```
hmap2obj.exe terrain.r16 terrain.obj 4096 4096 -0.5 0 -0.5 0.5 0.02 0.5 --absolute
```

Will vectorize a 4k terrain into somewhat casual for rendering software (Blender, ZBrus) obj range
of -0.5 to +0.5. Note that the displacement axis was kept between 0 and 0.02, this makes the terrain
to look proportional. The absolute switch was given as this terrain is cut of bigger one, so later
the obj can be transferred back to heightmap and stitched together.

Note that the 0.0 and 0.02 values should be the same as in the obj2hmap tool. Together with the 
`--absolute` switch we can transfer chunks of big maps back and forth without loose of data.

## License

The software code is distributed under [GPL 3](https://www.gnu.org/licenses/gpl-3.0)

