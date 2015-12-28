return {
------------------------------------------------------------------
-- Basic functions
------------------------------------------------------------------
    {name = "collectgarbage",
     module = "_G",
     section = "basic",
     params = "",
     description = [[
<p>
Perform a full garbage collection cycle.
]],
    },
------------------------------------------------------------------
    {name = "require",
     module = "_G",
     section = "basic",
     params = "module",
     description = [[
<p>
Loads a module. <code>module</code> should be the
name of a lua file in the same directory as the file
calling <code>require</code>, without the <code>.lua</code>
suffix. If the module has not been loaded before, then
the module is run, its return value cached, and returned
by <code>require</code>. Otherwise the previously cached
value is returned by <code>require</code>.
]],
    },
------------------------------------------------------------------
-- Vectors and Matrices
------------------------------------------------------------------
    {name = "vec2",
     module = "_G",
     section = "vecmat",
     params = "&middot;&middot;&middot;",
     description = [[
<p>
Constructs a 2 dimensional vector.
See <a href="#vecmat-intro">Vectors and Matrices</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "vec3",
     module = "_G",
     section = "vecmat",
     params = "&middot;&middot;&middot;",
     description = [[
<p>
Constructs a 3 dimensional vector.
See <a href="#vecmat-intro">Vectors and Matrices</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "vec4",
     module = "_G",
     section = "vecmat",
     params = "&middot;&middot;&middot;",
     description = [[
<p>
Constructs a 4 dimensional vector.
See <a href="#vecmat-intro">Vectors and Matrices</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "mat2",
     module = "_G",
     section = "vecmat",
     params = "&middot;&middot;&middot;",
     description = [[
<p>
Constructs a 2x2 matrix.
See <a href="#vecmat-intro">Vectors and Matrices</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "mat3",
     module = "_G",
     section = "vecmat",
     params = "&middot;&middot;&middot;",
     description = [[
<p>
Constructs a 3x3 matrix.
See <a href="#vecmat-intro">Vectors and Matrices</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "mat4",
     module = "_G",
     section = "vecmat",
     params = "&middot;&middot;&middot;",
     description = [[
<p>
Constructs a 4x4 matrix.
See <a href="#vecmat-intro">Vectors and Matrices</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "dot",
     module = "math",
     section = "vecmat",
     params = "vector1, vector2",
     description = [[
<p>
Returns the dot product of two vectors. The
vectors must have the same dimensions.
]],
    },
------------------------------------------------------------------
    {name = "cross",
     module = "math",
     section = "vecmat",
     params = "vector1, vector2",
     description = [[
<p>
Returns the cross product of two 3 dimensional vectors.
]],
    },
------------------------------------------------------------------
    {name = "normalize",
     module = "math",
     section = "vecmat",
     params = "vector",
     description = [[
<p>
Returns the normalized version of a vector (i.e.
the vector that points in the same direction, but whose
length is 1).
If the given vector has zero length, then a vector
of the same dimensions is returned whose first
component is 1 and remaining components are 0.
]],
    },
------------------------------------------------------------------
    {name = "reflect",
     module = "math",
     section = "vecmat",
     params = "I, N",
     description = [[
<p>
For the incident vector <code>I</code> and surface orientation <code>N</code>, 
returns the reflection direction: <code>result = I - 2.0 * dot(N, I) * N</code>.
See the <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/reflect.xml">GLSL manual page</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "refract",
     module = "math",
     section = "vecmat",
     params = "I, N, eta",
     description = [[
<p>
For the incident vector <code>I</code> and surface normal <code>N</code>, 
and the ratio of indices of refraction <code>eta</code>, 
return the refraction vector.
See the <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/refract.xml">GLSL manual page</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "faceforward",
     module = "math",
     section = "vecmat",
     params = "N, I, Nref",
     description = [[
<p>
If <code>dot(Nref, I) < 0.0</code>, returns <code>N</code>, otherwise, returns <code>-N</code>.
See the <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/faceforward.xml">GLSL manual page</a>
for more details.
]],
    },
------------------------------------------------------------------
    {name = "length",
     module = "math",
     section = "vecmat",
     params = "vector",
     description = [[
<p>
Returns the length of a vector.
]],
    },
------------------------------------------------------------------
    {name = "distance",
     module = "math",
     section = "vecmat",
     params = "vector1, vector2",
     description = [[
<p>
Returns the distance between two vectors.
]],
    },
------------------------------------------------------------------
    {name = "inverse",
     module = "math",
     section = "vecmat",
     params = "matrix",
     description = [[
<p>
Returns the inverse of a matrix.
]],
    },
------------------------------------------------------------------
    {name = "lookat",
     module = "math",
     section = "vecmat",
     params = "eye, center, up",
     description = [[
<p>
Creates a 4x4 &quot;look at&quot; view matrix.

<p>
<code>eye</code> is the position of the camera (a 3 dimensional vector).
<code>center</code> is a point at which the camera should be looking (another 3 dimensional vector).
<code>up</code> is the &quot;up&quot; direction of the camera (a normalized 3 dimensional vector).
This is typically <code>vec3(0, 1, 0)</code>.
]],
    },
------------------------------------------------------------------
    {name = "perspective",
     module = "math",
     section = "vecmat",
     params = "fovy, aspect, near, far",
     description = [[
<p>
Creates a 4x4 matrix for a symetric perspective-view frustum.

<code>fovy</code> is the field of view in the y plain, in radians.
<code>aspect</code> is typically the window width divided by its height.
<code>near</code> and <code>far</code> are the distances of the near and
far clipping plains from the camera.
]],
    },
------------------------------------------------------------------
    {name = "euleryxz3",
     module = "math",
     section = "vecmat",
     params = "vector",
     description = [[
<p>
Creates a 3D 3x3 rotation matrix from euler angles
<code>vector.y</code> (pitch), <code>vector.x</code> (yaw) and <code>vector.z</code> (roll),
applied in that order.
All angles are in radians.
]],
    },
------------------------------------------------------------------
    {name = "euleryxz4",
     module = "math",
     section = "vecmat",
     params = "vector",
     description = [[
<p>
Creates a 3D 4x4 homogeneous rotation matrix from euler angles
<code>vector.y</code> (pitch), <code>vector.x</code> (yaw) and <code>vector.z</code> (roll),
applied in that order.
All angles are in radians.
]],
    },
------------------------------------------------------------------
-- Graphics
------------------------------------------------------------------
    {name = "draw_elements",
     module = "amulet",
     section = "graphics",
     params = "indices [, primitive]",
     description = [[
<p>
Returns a node that renders the given primitives by looking up
each vertex in the currently bound arrays using the given indices.

<p>
<code>indices</code> should be a view of type <code>uint_elem</code>
or <code>ushort_elem</code> with a stride of 4 or 2 respectively.
<code>primitive</code> should be one of the strings
<code>"triangles"</code> (the default),
<code>"triangle_strip"</code>,
<code>"lines"</code>,
<code>"line_strip"</code> or
<code>"points"</code>.
]],
    },
------------------------------------------------------------------
    {name = "draw_arrays",
     module = "amulet",
     section = "graphics",
     params = "[primitive]",
     description = [[
<p>
Returns a node that renders the given primitives using the
currently bound arrays.

<p>
<code>primitive</code> can be one of the strings
<code>"triangles"</code> (the default),
<code>"triangle_strip"</code>,
<code>"lines"</code>,
<code>"line_strip"</code> or
<code>"points"</code>.
]],
    },
}
