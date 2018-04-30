# Draw My Aurora

![](http://p80d95pxq.bkt.gdipper.com/auroras1.png)

This project is meant to enable users to draw the path of aurora, and view the created aurora as if standing on any specified point on the earth (currently excluding the exact north pole point and the Southern Hemisphere). The image shown above is a sample output. The program is designed to have very few dependencies and very small executable.

The user interfaces and the way to control them is presented in the next section, then come dependencies and related papers.

##User Interfaces

![](http://p80d95pxq.bkt.gdipper.com/auroras2.png)

Run the program, and you'll see a 3D earth model, with some curves and rectangles on the top. Those curves are Catmull-Rom spline curves, and they will be used as the path of aurora. Later you'll see how to change the shape of them by moving control points (i.e. those rectangles on curves).

Three buttons are shown on the bottom. "EDITING" is initally off. At this state, you are free to drag the earth to rotate it. You may also scroll the mouse to zoom in/out. "DAYLIGHT" is initally off as well. If you click on it, the earth will be in the day light, as shown in the image below. This can be useful if you feel hard to find your home on the earth at night.

![](http://p80d95pxq.bkt.gdipper.com/auroras3.png)

After "EDITING" is pressed, the earth will be locked, and does not respond to your dragging. Now you'll see "PATHx" buttons on the top, which are used to choose which path to edit. You may press on the left key of mouse to drag a control point (small rectangle) around. You may also press the right key of mouse; if you are right clicking on a control point, that point will be deleted; if you are right clicking on nowhere, a new control point will be inserted there. Just try it out.

![](http://p80d95pxq.bkt.gdipper.com/auroras4.png)

The last step is to click the "AURORA" button on the bottom. You will be "placed" on the place that is currently in the center of the frame (eg: if the U.S. is right facing you, you will be put there), looking north, and view the aurora you just "created":

![](http://p80d95pxq.bkt.gdipper.com/auroras5.png)

The aurora shown above imitates the photo I have taken in Findland:

![](http://p80d95pxq.bkt.gdipper.com/auroraIMGP2446.JPG)

![](http://p80d95pxq.bkt.gdipper.com/auroras6.png)

![](http://p80d95pxq.bkt.gdipper.com/auroras7.png)

The aurora shown above imitates another photo I took:

![](http://p80d95pxq.bkt.gdipper.com/auroraIMGP2464.JPG)

##Acknowledgement

The program is dependent on:

- [OpenGL](https://www.khronos.org/opengl/), of course
- [GLAD](https://github.com/Dav1dde/glad), to load OpenGL function pointers
- [GLM](https://glm.g-truc.net/), to handle all linear algebra operations
- [GLFW](http://www.glfw.org), to manage the window
- [stb](https://github.com/nothings/stb), to load images
- [FreeType](https://www.freetype.org), to load fonts

That's all. 

The method to render aurora is mostly inspired by Dr. Orion Sky Lawlor and Dr. Jon Genetti's paper [*Interactive Volume Rendering Aurora on the GPU*](https://www.cs.uaf.edu/~olawlor/papers/2010/aurora/lawlor_aurora_2010.pdf). The shader code *aurora.vs* and the code for computing the atmosphere thickness (in *airtrans.cpp*) are directly modifies from Dr. Orion Sky Lawlor's code (which can be found [here](https://www.cs.uaf.edu/~olawlor/papers/index.html)).

The code for generating the distance field is modified from Richard Mitton's [implementation](http://www.codersnotes.com/notes/signed-distance-fields/). I have tried to accelerate it and achieved a 87% sppedup. In my another [repo](https://github.com/lun0522/8ssedt), you can see how I achieved it step by step.