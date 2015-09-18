.. _quick-start:

Quick start guide
=================

This section will get you up and running with Amulet.
You'll learn how to create a window, draw and animate
simple 2D graphics, play audio and respond to use input.

Install Amulet
--------------

First `download <http://xxx/download.html>`__ and run the Amulet installer
(or if you're using Linux extract it to a directory and add that directory
to your PATH).

Amulet is a command line tool and doesn't come with an IDE or
editor.

Create a folder for your new project anywhere you like. In that
folder create a text file called ``main.lua`` containing the following:

..  code-block:: lua

    print("Hello!")

Now open a terminal (or "command prompt" on Windows) and navigate
to the new project folder. Then type ``amulet main.lua``:

..  code-block:: sh

    $ amulet main.lua
    Hello!

If you see the text ``Hello!`` Amulet is installed and working.

Creating a window and rendering some text
-----------------------------------------

Instead of printing "Hello!" to the terminal, let's instead display
it in a new window. 
Type the following into main.lua:

..  code-block:: lua
    :linenos:

    local win = am.window{title = "Hi", width = 400, height = 300}
    local camera = am.camera2d(400, 300, vec2(0, 0))
    local text = am.text("Hello!")
    camera:append(text)
    win.scene = camera

Let's go through this script line by line.

On line 1 we create a window by calling the :func:`am.window` function.
The returned window object is stored in a local variable called
``win``.

On line 2 we create a 2D camera *scene node*. In Amulet you don't draw
directly to a window. Instead you create a *scene* and attach that
to the window. Amulet then renders the scene to the window each frame.
A scene is composed of scene nodes. The camera node we create on 
line 2 is going to be the topmost node of the scene, also know as
the root node.
It sets up a viewing area into which other nodes will be drawn.
The first argument (``400``) is with width of the viewing area and
the second argument (``300``) is its height. The third argument
is the position of the center of the viewing area.
``vec2(0, 0)`` sets the center of the viewing to the point
where the x and y coordinates are both zero.

On line 3 we create a scene node that renders some text.
By default text is centered around the origin (i.e. the point where x
and y are zero).

Line 4 adds the new text node as a *child node* of the camera
node. This has the effect of drawing the text in the viewing
area set up by the camera node.

Finally on line 5 we set the camera node as the window's scene.
This tells the window to render this scene each frame.

Now run the script as before:

..  code-block:: sh

    $ amulet main.lua

This time a small window should appear containing the text "Hello!".
It should look something like this:

..  figure:: screenshots/hello1.png
    :alt: 

Notice that the script doesn't immediately terminate as it did
before. This is because Amulet will continue to run the
script as long as there is an open window.

Close the window by clicking its close button and the
script will end.

Let's try changing the camera position. Change line 2 to:

..  code-block:: lua

    local camera = am.camera2d(400, 300, vec2(100, 100))

Now when we run the script we get the following:

..  figure:: screenshots/hello2.png
    :alt: 

The camera is now centered at the point (100, 100).
This is 100 above and to the right of the text node,
which is at position (0, 0), so the text node appears
to the left and below the center of the camera.

..  note::

    By convention the y coordinate increases in the upward direction
    in Amulet.

Drawing shapes
--------------



Making things move
------------------

Responding to key presses
-------------------------

Playing sounds
--------------
