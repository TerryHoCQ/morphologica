---
title: morph::Visual
parent: Visualization classes
grand_parent: Reference
permalink: /ref/visual/visual
layout: page
nav_order: 0
---
# morph::Visual
```c++
#include <morph/Visual.h>
```

## Overview

`morph::Visual<>` manages the graphical scene in which all your
morphologica visualizations will exist. `morph::Visual` provides a
number of 'built-in' features for your morphologica based program,
such as the ability to show a set of coordinate axes, save a PNG image
or save the scene in glTF format. `morph::Visual` supports perspective
and orthographic projections and even has an experimental cylindrical
projection mode.

You need at least one morph::Visual in your program. In general, one
morph::Visual will relate to one window. (If you have two or more
Visuals, then you will have two or more windows.)

The following code (adapted from the [graph1.cpp](https://github.com/ABRG-Models/morphologica/blob/main/examples/graph1.cpp) example) illustrates basic usage:

```c++
    // First, instantiate a 1024x768 morph::Visual object/window
    morph::Visual v(1024, 768, "Window title here");

    // You then create some kind of morph::VisualModel (GraphVisual derives from VisualModel):
    auto gv = std::make_unique<morph::GraphVisual<double>> (morph::vec<float>({0,0,0}));
    v.bindmodel (gv);                                       // bindmodel is boilerplate
    morph::vvec<double> x = {0,.1,.2,.3,.4,.5,.6,.7,.8,.9}; // x axis data
    gv->setdata (x, x.pow(3));                              // Graph y = x^3
    gv->finalize();                                         // 'completes' the VisualModel

    // By adding the VisualModel it will appear in the scene
    v.addVisualModel (gv);

    // You can now render manually with v.render() or until the user quits with this function:
    v.keepOpen();
```

![Screenshot of two computer windows each backed by a morph::Visual](https://github.com/ABRG-Models/morphologica/blob/main/docs/images/morph_two_visuals.png?raw=true)

*This is the [twowindows.cpp](https://github.com/ABRG-Models/morphologica/blob/main/examples/twowindows.cpp) example program, which displays two windows with two `morph::Visual` instances. The graph on window 2 is very similar to what the preceding code example would generate, showing a `morph::GraphVisual`. Window 1 shows another kind of `morph::VisualModel` (a rather sparse `QuiverVisual`)*

## Background: the `morph::Visual` GL version and `OWNED_MODE`

`morph::Visual` and `morph::VisualModel` conspire to hide most of the
OpenGL internals away from you, the client coder. However, there *is*
some background knowledge that you need to understand.

### OpenGL Version

When you program with OpenGL, you have to choose which of the many versions of the library you want to use. morphologica uses 'modern OpenGL' which essentially means that we draw with *GLSL shader programs*. These are C-like programs which are executed by the graphics processing unit with many parallel threads (you don't need to learn GLSL; morphologica provides [default shader programs](https://github.com/ABRG-Models/morphologica/tree/main/shaders)). Different versions of OpenGL provide different supported features in the GLSL and the C function calls that support it. 'Modern OpenGL' started with OpenGL version 3.3, but version 4.1 was chosen for morphologica's default as it is well supported across the Linux, Mac and Windows platforms.

OpenGL 4.1 was originally the *only* option, but more recently `Visual` and friends were extended to support other OpenGL versions, including OpenGL 4.1 to 4.6 (which makes it possible to use GL compute shaders) and OpenGL 3.0 ES and up, which makes it possible to run morphologica programs on the Raspberry Pi.

The desired OpenGL version is passed to `morph::Visual` as a single template argument `glver` of type `int`.

The default value for `glver` is `morph::gl::version_4_1` which requests the core version 4.1 of OpenGL. The integer values that specify each OpenGL version are defined in [morph/gl/version.h](https://github.com/ABRG-Models/morphologica/blob/main/morph/gl/version.h). Both the 'desktop' OpenGL versions (from 4.1 up to 4.6) and the 'ES' versions (3.0 ES to 3.2 ES) are supported in both core and compatibility modes.

Note that the OpenGL version integer is also used as a template parameter in the `morph::VisualModel` objects that will populate your `morph::Visual`. You should ensure that the same value for the GL version is used across all classes.

### OpenGL header inclusion

How you include OpenGL headers and link to OpenGL driver code can be complex, and can differ between Linux, Apple and Windows platforms. On Linux, morphologica will `#include` GL headers from its own code base, and on Apple, it will include them from the system. If you are integrating morphologica code into an existing program that *already* has a scheme for including OpenGL headers, then it should detect this gracefully.

Linking should be determined by the CMake system.

### `OWNED_MODE`

One more concept to introduce before getting into `morph::Visual` usage is the 'operating mode'. When Visual was first developed, it was designed to own its desktop window, which would always be provided by the [GLFW library](https://www.glfw.org/). The Visual class would manage GLFW setup and window creation/destruction. Window pointers (aliased as `morph::win_t`) were always of type `GLFWwindow`.

Later on, I wanted to add support for the Qt windowing system so that a `morph::Visual` could provide OpenGL graphics for a `QtWidget`. Qt manages OpenGL contexts and windows, so I had to create a new operating mode for `morph::Visual` in which it would use an externally managed context. To do this I defined `OWNED_MODE`. `OWNED_MODE` is enabled by `#define OWNED_MODE 1` in a relevant location (see [viswidget.h](https://github.com/ABRG-Models/morphologica/blob/main/morph/qt/viswidget.h) for Qt and [viswx.h](https://github.com/ABRG-Models/morphologica/blob/main/morph/wx/viswx.h) for wx windows).

In `OWNED_MODE`, an alternative windowing system can be used and `morph::win_t` is mapped to the appropriate window/widget type. Code that is involved in setting up the windowing system is disabled.

However, unless you are integrating morphologica into Qt or WxWidgets, you will leave `OWNED_MODE` undefined.

## Instantiating `morph::Visual`

Instantiate your `morph::Visual` with the following three-argument constructor:

```c++
morph::Visual v(1280, 1024, "Your window title goes here");
```
The first two arguments are the width and height of the window in pixels. The third argument is the text title for the window.

In addition to the usual, three-argument constructor, there's a constructor which allows you to set parameters for the coordinate arrows (length, thickness, etc). Use only if you need to adapt the coordinate arrows (see [Visual.h](https://github.com/ABRG-Models/morphologica/blob/main/morph/Visual.h#L153) for the declaration).

There *is* also a default, no-argument constructor, but you'll only need to call this if you're adapting a new `OWNED_MODE`.

## Adding labels to the window

You can add a scene-wide label to your `morph::Visual` scene. Although there are a number of overloads of `Visual::addLabel`, the best one to use is:

```c++
//! An addLabel overload that takes a morph::TextFeatures object
morph::TextGeometry addLabel (const std::string& _text,
                              const morph::vec<float, 3>& _toffset,
                              const morph::TextFeatures& tfeatures)
```

This takes the text you want to show as its first argument, a positional offset within the scene for the text and an object which specifies the font, fontsize and colour for the text.
[TextFeatures](textfeatures) is easy to use; its simplest constructor takes just a font size, leaving the rest of the text features to their defaults, so that an addLabel call might look like this:

```c++
morph::Visual v(1280, 1024, "Your window title goes here");
morph::vec<float> textpos = { 0, 0, 0 };
v.addLabel ("Test label", textpos, morph::TextFeatures(0.15f));
```

The `Visual::addLabel` function is similar to `VisualModel::addLabel`. Other ways to set the font colour and face are documented on [TextFeatures](textfeatures) and [VisualModel](visualmodel).

## Adding and removing `VisualModels`

Other than labels, the main constituents of your scenes will be `morph::VisualModel` objects. Each `VisualModel` contains a set of OpenGL vertex buffer objects that define an 'OpenGL model' comprised mostly of triangles and also of a few textures (for text). `VisualModel` is designed as a base class; you won't actually add VisualModels to the `morph::Visual`. Instead, you'll add objects of derived classes such as `morph::GraphVisual`, `morph::ScatterVisual` or `morph::GridVisual`.

`morph::Visual` takes ownership of the memory associated with each `VisualModel`. It keeps an `std::vector` of `std::unique_ptr` objects to VisualModels in the member attribute `Visual::vm`; here's an excerpt from Visual.h:

```c++
 protected:
    //! A vector of pointers to all the morph::VisualModels (HexGridVisual,
    //! ScatterVisual, etc) which are going to be rendered in the scene.
    std::vector<std::unique_ptr<morph::VisualModel<glver>>> vm;
```

When `Visual` needs to `render()`, it will iterate through this vector, calling `VisualModel::render()` for each model.

To guarantee the ownership of the model will reside in the `morph::Visual` instance, you have to 'pass in' each VisualModel. The workflow is (using `GraphVisual` as the example and assuming the `Visual` object is called `v`):

```c++
    // Create a GraphVisual. gv will be of type std::unique_ptr<morph::GraphVisual<double>>
    auto gv = std::make_unique<morph::GraphVisual<double>> (morph::vec<float>({0,0,0}));
    // Bind the new model to the Visual instance
    v.bindmodel (gv);
    // Do some GraphVisual-specific setup:
    gv->setdata (x, x.pow(3));
    // Call VisualModel::finalize(), which populates the OpenGL vertex/index buffers
    gv->finalize();

    // Now the GraphVisual has been created, add it:
    morph::GraphVisual<double>* gv_pointer = v.addVisualModel (gv);
```
Here we've created a GraphVisual, set it up and added it to the scene.

The call `v.bindmodel (gv)` is necessary to set some callbacks in the GraphVisual. Essentially it sets the Visual object `v` as the 'parent' for the `GraphVisual` and sets callbacks that give the GraphVisual access to the OpenGL shader programs. You'll write a line like this as boilerplate for every `VisualModel` that you create and add.

After any specific setup (here it was `gv->setdata (x, x.pow(3))`), you then call `VisualModel::finalize()`. This actually builds the model out of triangles and populates the OpenGL buffers. If you don't call `finalize()`, the model won't render at all.

Once it has been finalized, you call `addVisualModel()`, passing in the `GraphVisual` unique_ptr. This uses `std::move` to transfer ownership of the `unique_ptr` to the `Visual`, adding it to `Visual::vm`. `addVisualModel` returns a plain, non-owning pointer to the `VisualModel`, which you can use to interact with the model for the rest of the program (you'll need to do this if you want to update the content of the graph, grid or plot). In the example I wrote the type of `gv_pointer` explicitly, but often I code this with `auto`:

```c++
auto gv_pointer = v.addVisualModel (gv);
```

**Don't try to use `gv` after you have added it!** Your local `unique_ptr gv` *no longer owns the memory*. However, you *can* re-use `gv` if you want to, setting it with another call to `std::make_unique` for a new model.

## Setting `Visual` features

There's a selection of features that you can enable once you've instanciated your `Visual`:

### Scene translation

Once you've added several models to your scene, you may need to adjust where they appear in the window. Rather than moving each model programmatically (by altering offsets) you can simply shift the camera view point for the scene with a call to `setSceneTrans()`:

```c++
v.setSceneTrans (morph::vec<float,3>({-0.35105f, -0.352273f, -2.4f}));
```

You can find the best values for the offset by running your program, moving around with mouse commands until the position of your models is correct within the window and then press Ctrl-z and see stdout:

```bash
[seb@GPU3090 16:59:08 build]$ ./examples/graph1
This is version 3.0 of morph::Visual<glver=4.1> running on OpenGL Version 4.1.0 NVIDIA 535.171.04
Scenetrans setup code:
    v.setSceneTrans (morph::vec<float,3>({-0.35105f, -0.352273f, -2.4f}));
scene rotation is Quaternion[wxyz]=(1,0,0,0)
Writing scene trans/rotation into /tmp/Visual.json... Success.
```

The line of code you need is printed out. Convenient!

### Background colour

```c++
v.backgroundWhite(); // Sets a white background (the default)

v.backgroundBlack(); // Set a black background

// or set Visual::bgcolour directly:
v.bgcolour = std::array<float, 4>({ 1.0f, 0.0f, 1.0f, 1.0f }); // RGB triplet plus alpha. Range 0-1
```

### Lighting effects

The default shader will not apply any lighting to the scene. This means that plots that use colour to indicate values aren't distorted by lighting variance. However, sometimes it is useful to get the sense of depth that some simple diffuse lighting provides. To turn it on it's:

```c++
v.lightingEffects (true); // or false to explicitly turn off. Default arg is true.
```

You can set light_colour, intensity and the position of the diffuse light:

```c++
v.light_colour = { 1, 1, 1 };               // Element ranges 0-1
v.ambient_intensity = 0.8f;                 // Range 0-1
v.diffuse_position = { 5.0f, 5.0f, 15.0f }; // coordinates
v.diffuse_intensity = 0.4f;
```

### Perspective/Orthgraphic

### Clipping distances and field of view

### Coordinate arrows

## Working with Visuals in a loop

render(), poll(), wait(), waitevents().

## Dealing with OpenGL context

Switching between contexts.

## Saving an image to make a movie

## Under the hood

Some techy info