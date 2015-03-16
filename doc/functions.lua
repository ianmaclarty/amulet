return {
------------------------------------------------------------------
    {name = "collectgarbage",
     module = "_G",
     section = "basic",
     signature = "collectgarbage()",
     description = [[
<p>
Perform a full garbage collection cycle.
]],
    },
------------------------------------------------------------------
    {name = "require",
     module = "_G",
     section = "basic",
     signature = "require(module)",
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
    {name = "draw_elements",
     module = "amulet",
     section = "graphics",
     signature = "amulet.draw_elements(indices [, primitive])",
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
     signature = "amulet.draw_elements([primitive])",
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
