/*
 * Copyright (C) 2023  Renan S. Silva, aka h3nnn4n
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "scene.h"

vec3 camera_pos         = {0.0, 0.0, 0.0};
vec3 camera_orientation = {0.0, 270.0, 45};

vec4  positions[n_spheres];
float radius[n_spheres];
vec4  albedo[n_spheres];
float roughness[n_spheres];
int   material_type[n_spheres];

void init_scene() {
    // clang-format off
    object_t data[] = {
      //  position            ,   radius , albedo            , roughness , type
      { { 2.0 ,  0.0 , -10.0} ,   1.0    , {2.0 , 3.0 , 4.0 } , 0.0       , LIGHT      } ,
      { { 0.0 , -1.0 , -10.0} ,   1.0    , {1.0 , 0.2 , 0.3 } , 0.0       , DIFFUSE    } ,
      { { 4.0 , -1.0 ,  -9.0} ,   1.0    , {0.3 , 0.9 , 0.1 } , 0.0       , DIFFUSE    } ,
      { {-4.0 , -1.0 , -10.0} ,   1.0    , {0.0 , 0.2 , 0.9 } , 0.0       , DIFFUSE    } ,
      { { 0.0 ,  1.0 , -10.0} ,   1.0    , {0.7 , 0.6 , 0.2 } , 0.1       , METAL      } ,
      { { 4.0 ,  1.0 , -10.0} ,   1.0    , {0.3 , 0.8 , 0.2 } , 0.3       , METAL      } ,
      { {-4.0 ,  1.0 , -10.0} ,   1.0    , {0.3 , 0.1 , 0.8 } , 0.5       , METAL      } ,
      { {-2.0 ,  0.0 , -15.0} ,   3.0    , {0.7 , 0.6 , 0.3 } , 0.25      , METAL      } ,
      { {-2.0 ,  0.0 , -10.0} ,   1.0    , {1.0 , 1.0 , 1.0 } , 0.0       , DIELECTRIC } ,
      { { 0.5 ,  0.0 ,  -7.0} ,   1.0    , {1.0 , 1.0 , 1.0 } , 0.0       , DIELECTRIC } ,
    };
    // clang-format on

    for (int i = 0; i < n_spheres; i++) {
        for (int j = 0; j < 3; j++) {
            positions[i][j] = data[i].position[j];
            albedo[i][j]    = data[i].albedo[j];
        }

        positions[i][3] = 1.0;
        albedo[i][3]    = 1.0;

        radius[i]        = data[i].radius;
        roughness[i]     = data[i].roughness;
        material_type[i] = data[i].material_type;
    }
}

// clang-format off
// clang-format on
