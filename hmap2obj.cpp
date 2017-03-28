/**
 * @file hmap2obj.cpp
 * @brief Convert heightmap/displacement file to Wavefront's OBJ.
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
class hmap2obj
{
public:
    /// 32-bit unsigned 2d vector
    typedef std::array<std::uint32_t, 2> uvec2;
    /// Single float based 3d vector
    typedef std::array<float, 3> vec3;

    /// Describes the application parameters
    struct param_type
    {
        std::string hmap;   ///< Input *.* heightmap binary file to read from
        std::string obj;    ///< Output *.obj file to write to
        uvec2 hmap_size;    ///< How big is the integer grid of the input heighmap file
        vec3 obj_blo;       ///< The lowest corner of the obj bounding box
        vec3 obj_bhi;       ///< The highest corner of the obj bounding box
    };

    // 
    static param_type parse_cli (std::vector<std::string> const& args);

    //
    static std::string validate_params (param_type const& params);

    /// Just inits the app parameters.
    hmap2obj (param_type const& p) : params (p) {};
    /// Empty dtor
    ~ hmap2obj () {};

    //
    void read_hmap ();

    /// Peek how small is the minimum elevation in the read heightmap
    auto hmap_min () const {
        return vmin;
    }
    /// Peek how big is the maximum elevation in the read heightmap
    auto hmap_max () const {
        return vmax;
    }

    //
    void make_xyz ();

    //
    void dump_obj ();

private:
    param_type params;      ///< The input to the app
    std::vector<vec3> xyz;  ///< The point cloud data coming from the obj file
    std::vector<vec3::value_type> grid; ///< The imported height values in XY order
    uvec2::value_type vmin, vmax;       ///< The min/max elevation data of the imported heightmap
};

//--------------------------------------------------------------------------------------------------

/** 
 * Create application parameters out of the C++ main() arguments
 *
 * The arguments are expected to be in any order, but certain priority. 
 *
 * @param args as reported by main() (e.g. the first one is the exe name path)
 */

hmap2obj::param_type hmap2obj::parse_cli (std::vector<std::string> const& args)
{
    using namespace std;

    param_type p;
    p.hmap_size.fill (0);
    p.obj_blo.fill (numeric_limits<float>::quiet_NaN ());
    p.obj_bhi.fill (numeric_limits<float>::quiet_NaN ());

    for (auto& arg: args) 
    {
        if (p.hmap.empty ()) 
        {
            p.hmap = arg;
            continue;
        }

        if (p.obj.empty ()) 
        {
            p.obj = arg;
            continue;
        }

        try
        {
            bool succ = false;
            int n = stoi (arg, nullptr, 0);
            if (n > 0) 
                for (auto& d: p.hmap_size) if (d <= 0) 
                {
                    d = static_cast<unsigned> (n);
                    succ = true;
                    break;
                }
            if (succ) continue;
        }
        catch (exception&)
        {}

        try
        {
            bool succ = false;
            float x = stof (arg);

            for (size_t i = 0, n = p.obj_blo.size (); i < n; ++i)
                if (isnan (p.obj_blo[i])) 
                {
                    p.obj_blo[i] = x;
                    succ = true;
                    break;
                }

            if (succ) continue;

            for (size_t i = 0, n = p.obj_bhi.size (); i < n; ++i)
                if (isnan (p.obj_bhi[i])) 
                {
                    p.obj_bhi[i] = x;
                    break;
                }
        }
        catch (exception&)
        {}
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

std::string hmap2obj::validate_params (param_type const& p)
{
    using namespace std;

    if ([&p] () -> bool {
            ifstream f;
            f.open (p.hmap);
            return !f.is_open ();
        } ())
        return "An input heightmap file was not opened!";

    if ([&p] () -> bool {
            ofstream f;
            f.open (p.obj, ios_base::app);
            return !f.is_open ();
        } ())
        return "An output Wavefront *.obj file was not opened!";

    for (auto n: p.hmap_size)
        if (n <= 0)
            return "The heightmap size parameter is invalid!";

    for (auto n: p.obj_blo)
        if (!isnormal (n) && n != 0)
            return "The obj lowest corner parameter is invalid!";

    for (auto n: p.obj_bhi)
        if (!isnormal (n) && n != 0)
            return "The obj highest corner parameter is invalid!";

    for (size_t i = 0, n = p.obj_blo.size (); i < n; ++i) 
    {
        if (p.obj_blo[i] >= p.obj_bhi[i])
            return "Obj lowest corner value is greater!";
    }

    return "";
}

//--------------------------------------------------------------------------------------------------

/**
 * Parse and extract up the elevation data from the heightmap file.
 */

void hmap2obj::read_hmap ()
{
    using namespace std;

    ifstream hmap (params.hmap, ios_base::binary);

    grid.clear ();
    grid.resize (params.hmap_size[0] * params.hmap_size[1], 0);

    vmin = numeric_limits<decltype(vmin)>::max ();
    vmax = numeric_limits<decltype(vmax)>::min ();

    uint16_t p;
    for (size_t i = 0, n = grid.size (); 
            i < n && hmap.read (reinterpret_cast<char*> (&p), sizeof p); ++i, p = 0)
    {
        grid[i] = p;
        vmin = min<decltype(vmin)> (vmin, p);        
        vmax = max<decltype(vmin)> (vmax, p);        
    };
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert the elevation data to XYZ point cloud.
 *
 * For each point we first obtain its percentage location and then remap it to the obj dimension.
 */

void hmap2obj::make_xyz ()
{
    using namespace std;

    xyz.clear ();
    xyz.resize (grid.size (), { 0.f, 0.f, 0.f });

    for (size_t i = 0, n = grid.size (); i < n; ++i)
    {
        vec3 pt;
        pt[0] = (i % params.hmap_size[0]) / (params.hmap_size[0] - 1.f);
        pt[2] = (i / params.hmap_size[1]) / (params.hmap_size[1] - 1.f);
        pt[1] = (grid[i] - vmin) / (vmax - vmin);

        for (size_t j = params.obj_blo.size (); j--; )
            pt[j] = params.obj_blo[j] + pt[j] * (params.obj_bhi[j] - params.obj_blo[j]);

        xyz[i] = pt;
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * Dump the XYZ cloud onto a Wavefront file object.
 */

void hmap2obj::dump_obj ()
{
    using namespace std;

    ofstream file (params.obj);

    for (auto v: xyz)
    {
       file << "v " << v[0] << ' ' << v[1] << ' ' << v[2] << '\n'; 
    }

    for (size_t i = 1, n = xyz.size () - params.hmap_size[0]; i <= n; ++i)
    {
        if (i % params.hmap_size[0])
        {
            auto v1 = i;
            auto v2 = i + 1;
            auto v3 = i + params.hmap_size[0];
            file << "f " << v1 << ' ' << v2 << ' ' << v3 << '\n';

            v1 = v2;
            v2 = v3;
            v3 = v2 + 1;
            file << "f " << v1 << ' ' << v2 << ' ' << v3 << '\n';
        }
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
        "hmap2obj - A binary heightmap convertor to Wavefront *.obj file\n"
        "\n"
        "hmap2obj HMAP OBJ SIZE_X SIZE_Y OBJ_LOW_CORNER OBJ_HIGH_CORNER\n"
        "HMAP       - is the output binary heightmap file\n"
        "OBJ        - is the input obj file\n"
        "SIZE_XY    - the two integer dimensions of the heightmap which to put into the obj\n"
        "OBJ_CORNER - the low/high 3d floating corners of the obj to hold the heightmap\n"
        "\n"
        "Example:\n"
        "hmap2obj terrain.r16 terrain.obj 4096 4096 -0.5 0 -0.5 0.5 0.1 0.5\n"
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

        auto p = hmap2obj::parse_cli (args);
        auto err = hmap2obj::validate_params (p);
        if (err.size ())
        {
            cerr << err << endl;
            return 1;
        }

        hmap2obj tool (move (p));

        // Parse heightmap
        cout << "Read heightmap file..." << endl;
        tool.read_hmap ();
        cout << "Min height: " << tool.hmap_min () << '\n'
             << "Max height: " << tool.hmap_max () << '\n';

        // Create XYZ cloud
        cout << "Create point cloud..." << endl;
        tool.make_xyz ();

        // Dump obj
        cout << "Dump object file..." << endl;
        tool.dump_obj ();

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

