/**
 * @file obj2hmap.cpp
 * @brief Convert Wavefront's OBJ to heightmap/displacement file.
 * @internal
 *
 * Copyright(c) 2017 by ryobg@users.noreply.github.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @endinternal
 *
 * @details
 * Add detailed description of file
 */


#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <limits>
#include <array>
#include <algorithm>
#include <numeric>
#include <exception>
#include <utility>

/**
 * Application of converting Wavefront's OBJ file into binary 2d heightfield file.
 *
 * This class can parse command line arguments, validate them, and report a help message if needed.
 */
class obj2hmap
{
public:
    /// 32-bit integer based 3d vector
    typedef std::array<std::uint32_t, 3> uvec3;
    /// Single float based 3d vector
    typedef std::array<float, 3> vec3;
    /// Another special - boolean 3d vector
    typedef std::array<bool, 3> bvec3;

    /// Describes the application parameters
    struct param_type
    {
        std::string obj;    ///< Input *.obj file to read from
        std::string hmap;   ///< Output *.* binary file to write to
        uvec3 hmap_size;    ///< Says how big is the integer grid for the heightmap
        bvec3 height_coord; ///< Toggles which one of the 3 coords is the displacement axis
        enum file_type      ///< Talks about what kind of heightmap values we should output
        {
            u8, u16, u32, f32,      ///< Binary 8/16/32 unsigned and 32 bit float
            tu8, tu16, tu32, tf32   ///< Same, but in text variant
        } 
        ftype;              ///< The selected heightmap file
    };

    // 
    static param_type parse_cli (std::vector<std::string> const& args);

    //
    static std::string validate_params (param_type const& params);

    /// Just inits the app parameters.
    obj2hmap (param_type const& p) : params (p) {};
    /// Empty dtor
    ~ obj2hmap () {};

    //
    void read_obj ();

    /// Peek at the read up point cloud data
    std::vector<vec3> const& obj_vertices () const {
        return xyz;
    }
    /// Report the axis aligned bounding box of the point cloud data
    std::pair<vec3, vec3> obj_aabb () const {
        return std::make_pair (blo, bhi);
    }

    //
    void make_grid ();

    //
    void dump_heightmap ();

private:
    param_type params;      ///< The input to the app
    vec3 blo;               ///< Lowest corner of the obj bounding box
    vec3 bhi;               ///< Highest corner of the obj bounding box
    std::vector<vec3> xyz;  ///< The point cloud data coming from the obj file
    std::vector<vec3::value_type> grid; ///< The integer XY grid of height values

    /// Detects which is height/displacement axis
    std::size_t find_disp_axis () const 
    { 
        return std::find (params.height_coord.cbegin (), params.height_coord.cend (), true)
            - params.height_coord.cbegin ();
    }
};

//--------------------------------------------------------------------------------------------------

/** 
 * Create application parameters out of the C++ main() arguments
 *
 * The arguments are expected to be (in any order): 
 * * obj and then heightmap file 
 * * heightmap dimensions in hex/dec X Y Z format.
 * * One of X Y or Z which shows the actual height of the displacement (e.g. height of terrain)
 * * Optionally, one of the obj2hmap#param_type#file_type members in text format
 * * --help or so - this makes this func to throw with the message
 *
 * @param args as reported by main() (e.g. the first one is the exe name path)
 */

obj2hmap::param_type obj2hmap::parse_cli (std::vector<std::string> const& args)
{
    using namespace std;

    param_type p;
    p.hmap_size.fill (0);
    p.height_coord.fill (false);

    for (auto& arg: args) 
    {
        if (arg == "x" || arg == "X")
        {
            p.height_coord[0] = true;
            continue;
        }
        if (arg == "y" || arg == "Y")
        {
            p.height_coord[1] = true;
            continue;
        }
        if (arg == "z" || arg == "Z")
        {
            p.height_coord[2] = true;
            continue;
        }

        if (arg == "u8") {
            p.ftype = param_type::u8;
            continue;
        }
        if (arg == "u16") {
            p.ftype = param_type::u16;
            continue;
        }
        if (arg == "u32") {
            p.ftype = param_type::u32;
            continue;
        }
        if (arg == "f32") {
            p.ftype = param_type::f32;
            continue;
        }
        if (arg == "tu8") {
            p.ftype = param_type::tu8;
            continue;
        }
        if (arg == "tu16") {
            p.ftype = param_type::tu16;
            continue;
        }
        if (arg == "tu32") {
            p.ftype = param_type::tu32;
            continue;
        }
        if (arg == "tf32") {
            p.ftype = param_type::tf32;
            continue;
        }

        try
        {
            int n = stoi (arg, nullptr, 0);
            if (n > 0) 
                for (auto& d: p.hmap_size) if (!d) 
                {
                    d = static_cast<unsigned> (n);
                    break;
                }
            continue;
        }
        catch (exception&)
        {}

        if (p.obj.empty ()) 
        {
            p.obj = arg;
            continue;
        }

        if (p.hmap.empty ()) 
        {
            p.hmap = arg;
            continue;
        }
    }

    return p;
}

//--------------------------------------------------------------------------------------------------

/** 
 * Validation of the paramaters (the object does not assumes such).
 *
 * @param p to check
 * @return an human-readable error message if any issue was found
 */

std::string obj2hmap::validate_params (param_type const& p)
{
    if ([&p] () -> bool {
            std::ifstream f;
            f.open (p.obj);
            return !f.is_open ();
        } ())
        return "An input Wavefront *.obj file was not opened!";

    if ([&p] () -> bool {
            std::ofstream f;
            f.open (p.hmap, std::ios_base::app);
            return !f.is_open ();
        } ())
        return "An output heightmap file was not opened!";

    for (auto n: p.hmap_size)
        if (n < 1)
            return "The heightmap size parameter is invalid!";

    if (1 != std::accumulate (p.height_coord.cbegin (), p.height_coord.cend (), 
                0, [] (int r, bool x) { return r += int (x); }))
            return "The heightmap displacement axis parameter is invalid!";

    return "";
}

//--------------------------------------------------------------------------------------------------

/**
 * Parse and extract up the *.obj file vertices.
 *
 * A terrain mesh of 8k can reach up like 1GiB of file size. So this can take few minutes, depending
 * on the system.
 *
 * This function should be safe to be called multiple times, though it does not make sense for the
 * current application. Note that used RAM can increase a lot - a 8k by 8k map is like 768MiB.
 *
 * After the call to this function, the @ref xyz and @ref blo / @ref bhi members will have actual
 * values.
 */

void obj2hmap::read_obj ()
{
    using namespace std;

    ifstream obj (params.obj);

    blo.fill (numeric_limits<float>::max ());
    bhi.fill (numeric_limits<float>::min ());

    xyz.clear ();
    xyz.reserve (   /// Good guess is that the requested hmap is 1:1 with the supplied OBJ vertices
            accumulate (params.hmap_size.cbegin (), params.hmap_size.cend (), 
            1u, multiplies<unsigned> ()));

    // As getline, but w/o the memory store - useless optimization.
    auto skipline = [] (ifstream& is) { 
        for (char c; is.get (c) && c != '\n'; ); 
    };

    for (; obj; skipline (obj))
    {
        char t[2] = { 0, 0 };
        obj.read (t, 2);
        if (t[0] != 'v' || t[1] != ' ') // Wavefront's vertex type text line
            continue;

        vec3 v;
        for (size_t i = 0; i < v.size (); ++i)
        {
            obj >> v[i];
            blo[i] = min (blo[i], v[i]); 
            bhi[i] = max (bhi[i], v[i]);
        }

        xyz.push_back (v);
    }

    xyz.shrink_to_fit ();
}

//--------------------------------------------------------------------------------------------------

/**
 * Fit the point cloud into integer grid (i.e. plane or heightmap)
 *
 * It is expected that the point cloud is already created with #read_obj(). The non-height
 * dimensions are fit into integer grid by rounding. The height dimension is just carried over.
 *
 * At the end of this state we will have the #grid object populated in 2d.
 */

void obj2hmap::make_grid ()
{
    using namespace std;

    constexpr vec3::value_type default_value = 0.f;

    grid.clear ();
    grid.resize ([] (uvec3 const& sz, bvec3 const& ax) {
            size_t acc = 1;
            for (size_t n = sz.size (), i = 0; i < n; ++i) 
                if (!ax[i]) acc *= sz[i];
            return acc;
    } (params.hmap_size, params.height_coord), default_value);
 
    vec3 gridsz;
    for (size_t n = gridsz.size (), i = 0; i < n; ++i) 
    {
        gridsz[i]  = (params.hmap_size[i] - 1) / (bhi[i] - blo[i]);
        gridsz[i] *= !params.height_coord[i];
    }

    size_t haxis = find_disp_axis ();

    for (size_t end = xyz.size (), beg = 0; beg < end; ++beg)
    {
        size_t ndx = 0, ndxmul = 1;
        for (size_t n = gridsz.size (), i = 0; i < n; ++i)
        {
            auto p = trunc ((xyz[beg][i] - blo[i]) * gridsz[i]);
            ndx += static_cast<size_t> (p) * ndxmul;
            ndxmul = ndxmul * !params.height_coord[i] * params.hmap_size[i]
                   + ndxmul *  params.height_coord[i];
        }
        grid.at (ndx) = xyz[beg][haxis];
    }
}

//--------------------------------------------------------------------------------------------------

/// Utility to allow basic_ostream to write unformatted objects (apart of char_type pointers).
template<class CV, class V, class S>
static inline void bstream_write (S& os, V val)
{
    CV v (static_cast<CV> (val));
    os.write (reinterpret_cast<typename S::char_type*> (&v), sizeof v);
}

/**
 * Dump the grid plane onto a binary file of proper format.
 *
 * At this point of time, the #grid should be already available and using the other parameters we
 * can write a file. Size of each file unit (8 bit, 16 bit or 32 bit) is decided by looking at the 
 * size of the height axis.
 */

void obj2hmap::dump_heightmap ()
{
    using namespace std;

    ofstream file (params.hmap, ios_base::binary);

    size_t haxis = find_disp_axis ();
    auto height = params.hmap_size.at (haxis) / (bhi[haxis] - blo[haxis]);

    for (auto h: grid)
    {
        auto val = (h - blo[haxis]) * height;

        switch (params.ftype) {
        case param_type::u8  : bstream_write<uint8_t > (file, val); break;
        default              :
        case param_type::u16 : bstream_write<uint16_t> (file, val); break;
        case param_type::u32 : bstream_write<uint32_t> (file, val); break;
        case param_type::f32 : bstream_write<float   > (file, val); break;
        case param_type::tu8 : file << static_cast<uint8_t > (val); break;
        case param_type::tu16: file << static_cast<uint16_t> (val); break;
        case param_type::tu32: file << static_cast<uint32_t> (val); break;
        case param_type::tf32: file << static_cast<float   > (val); break;
        };
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * The venerable C++ main() function.
 */

int main (int argc, const char* argv[])
{
    using namespace std;

    const char* info = 
        "obj2hmap - An Wavefront *.obj file convertor to binary heightmap file\n"
        "\n"
        "obj2hmap OBJ HMAP x|y|z SIZE_X SIZE_Y SIZE_Z\n"
        "OBJ        - is the input obj file\n"
        "HMAP       - is the output binary heightmap file\n"
        "x y z      - one of the axes showing the displacement value of the heightmap\n"
        "SIZE_XYZ   - The three integer dimensions of the heightmap into which to put the obj\n"
        "\n"
        "Example:\n"
        "obj2hmap terrain.obj terrain.r16 y 4097 0xFFFF 4097\n"
        ;
    try
    {
        // CLI

        vector<string> args (argv + 1, argv + argc);
        for (auto& str: args)
            if (str == "--help")
            {
                cout << info << endl;
                return 0;
            }

        auto p = obj2hmap::parse_cli (args);
        auto err = obj2hmap::validate_params (p);
        if (err.size ())
        {
            cerr << err << endl;
            return 1;
        }

        obj2hmap tool (move (p));

        // Parse *.obj

        cout << "Read obj file..." << endl;
        tool.read_obj ();
        cout << "Parsed vertices: " << tool.obj_vertices ().size () << '\n'
             << "Bounding box   :";
        auto aabb = tool.obj_aabb ();
        for (auto i: aabb.first) cout << ' ' << i;
        cout << ';';
        for (auto i: aabb.second) cout << ' ' << i;
        cout << endl;

        // Create integer grid
        cout << "Fit into grid..." << endl;
        tool.make_grid ();

        // Dump data
        cout << "Dump heights..." << endl;
        tool.dump_heightmap ();

        cout << "Done." << endl;
    }
    catch (exception& ex)
    {
        cerr << ex.what () << endl;
        return 1;
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------

