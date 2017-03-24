# obj2hmap

Very simple convertor of Wavefront's obj files to displacement map (projected on one of the main XYZ planes)

This tool grew up out of my personal frustration with Blender, myself unable to import an obj file
and make depth texture, render and whatever else. Too much hassle. So I have spent several hours to
mash up (and straighten a bit later) this tool. It is really one file-show.

As always the best documentation is the code itself. And this code is small - <1k lines of code with
the huge Doxygen comments around.

Processing 1GiB obj file for 4k by 4k vertives, consumes like 20-30 seconds. Most of which are to
read the file itself.

## Requirements

Any C++11/14 conformant compiler. Didn't tried it, but I think even GCC 4.9 will suffice. During my
development I used GCC 6.2. No other external dependencies.

```
c++ obj2hmap.cpp -o obj2hmap -O2
```

Is enough. You can skip even the optimization level `-O2` and the named executable `-o obj2hmap`.

I have not tested with the Microsoft's compilers, but it should be easy to build with them too.

## Usage

```
obj2hmap <OBJ> <HMAP> <SIZE_X> <SIZE_Y> <SIZE_Z> <AXIS> [HMAP_TYPE]
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
* HMAP TYPE one of `u8`, `u16`, `u32`, `f32`, `tu8`, `tu16`, `tu32` or `tf32`.
  Is the type of values to dump into the heightmap file. The `u` prefix means unsigned, the `f`
  means floating point value, the number is the bit size and the `t` prefix means to output in text
  format and not binary values.

## Example

```
obj2hmap terrain.obj terrain.r16 4097 0xFFFF 4097 y u16
```

This will convert my ZBrush exported tool, into terrain of 4k size. It is 4097 and not 4096 because
of the Z considering the point to be in the middle of the pixel, so you have 1 more to "close-up"
the range.

Then as in my case, the Y axis shows the actual elevation of the terrain, I have put `0xFFFF` or the
max 16 bit unsigned value - this will force the tool to scale or stretch my elevation into the full
range of 16 bit.

The `y` part basically says where is the elevation data in the OBJ file.

The last `u16` is optional and this is the default assumption. It will dump binary terrain of 16 bit
values - quite common nowdays. Later it can be converted to image by some app like Gimp/Krita or fed
up to some virtual engine.

## License

The software code is distributed under [GPL 3](https://www.gnu.org/licenses/gpl-3.0)

