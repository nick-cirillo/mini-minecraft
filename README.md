# mini-minecraft

## Demo Video

[![Demo Video](https://img.youtube.com/vi/zn0icurnbRY/0.jpg)](https://www.youtube.com/watch?v=zn0icurnbRY)
> A quick demo of my Mini Minecraft project, which I developed with Viraj Doshi and Steven Chang. We tour you through some caves systems and biomes, show you the day/night cycle and directional lighting, and display the post-processing effects from going underwater or into lava. 

> The choppy framerate is because we recorded on remote PCs with latency!

## Mini Minecraft Team
I worked on this project with [Viraj Doshi](https://github.com/virajdoshi02) and [Steven Chang](https://github.com/zminsc).

## Features

### [Nick Cirillo](https://github.com/nick-cirillo)

#### Efficient Terrain Rendering and Chunking

I used a single interleaved VBO to contain the vertex, normal, and color data. I created a structure to contain relative block faces to avoid creating repeated code in the VBO setup function, and I used a map for the colors to make accessing them easier without a big if-else statement. I also implemented logic to load new chunks, and additionally, the m_setupChunks map to keep track of which chunks were setup and did not need to have their VBO created again, thus making rendering many times more efficient. I switched the rendering of the chunks to the progLambert shader, which ensured the model matrix was applied properly. I also used the attribute OpenGL hooks to separate the positions, normals, and colors, which then led me to create an index buffer that only references the positional information.

I also implemented a bounds-check in the rendering call so that the engine only draws the 9 "terrain generation zones" that surround the player.

#### Texturing

I added the ability to use UV coordinates in the VBOs for each chunk, thus mapping a texture from a texture map to each visible block face. I created a helper file for this purpose, which contains a nested mapping that allowed me to access block textures without a large branching if/else statement. I also modified the fragment shader to use these UV coordinates to render the texture with lambertian shading. I included an invariant of each UV component of the VBO such that the w-coordinate of each UV vec4 represented the alpha of that block (whether it is transparent, 1, or not, 0).

I also implemented texture animation for the water and lava, which makes use of a uniform variable for time passed to the fragment shader. The texture "slides" along a repeated pattern in the texture file, and loops smoothly by modding the time value with the number of pixels in those textures. Which blocks should animated is specified in a set in the chunkhelpers file. I also modified the texture file to include several transparent spaces so that I only render the top of the water.

I also split the VBOs for opaque and transparent/translucent blocks. I created new variables and functions to handle the new transparency VBO and pass the data to OpenGL. I also introduced additional loops to the terrain draw function so that all transparent blocks in all chunks are rendered after all opaque blocks across all chunks.


#### Day/Night Cycle

I created a day/night cycle in which the sun rises and sets. Clouds and a hazy sky are created with worley noise, and then different color palettes (day, sunset, dusk, evening) are blended in the fragment shader, meaning that the sky colors change throughout the cycle - bright and cloudy during the day, vivid during sunset, and dark blue at night.

The sun moves throughout the sky. It is not an entity, rather, it is a direction specified in the fragment shader, and the sun itself is created by finding the angle between the fragment direction and the direction of the sun. If the angle is small, we set the color to yellow, if it is slightly larger, we blend yellow into the sky to create a sun halo effect. If it is larger than a certain amount, we just set to the blended sky color. 

The sky colors change by checking the height of the sun and comparing it to several manually-set boundaries representing different times of day using a smoothstep function, then blending different proportions of the color palettes into the sky using the output of those smoothstep functions.

Finally, I created directional lighting for the world, meaning the angle of the sun influences how the blocks are being lit. The sun direction is substituted in for the light vector in the lambert shader, which creates an effect where the position of the sun illuminates the blocks.

### [Viraj Doshi](https://github.com/virajdoshi02)

#### Procedural terrain 

I used perlin noise for the hills, and worley noise for the mountains. I made the biome noise function (smoothstep with perlin noise). I used a method biomeBlock with flow control to determine which block is placed (dependent on y and type of biome), and another worley noise function to determine where to place dirt inside the mountains. 

I also had to create a new white snow block for snow. I had some difficulty in the flow control with my maxY and the stone and water levels, where I was placing stone where there was supposed to be water but those were resolved quite quickly with multiple tests in the if statement.

#### Cave systems and Post-processing pipeline<br>
I had to create the cave systems and post processing pipeline for minecraft. The former was difficult, because I am on mac so everything runs very slowly. I went to the fishbowl to mess with 3D perlin noise until I found a suitable method. First, I calculated an offset using fbm, then I used 3D perlin noise to create caves everywhere it was negative. I altered the frequency value until I got a lot of caves, because I wanted the underground region to be expansive. I also had a calculation to make sure that water "fell down" into the caves (i.e. I filled the air under water with more water). 

The post processing pipeline was by far the worst part of this.  Most difficulties stemmed from a lack of understanding of the pipeline, but I think I get it better now. I also had to alter a lot of existing classes to make it work, but I finished in the end. I created 2 post process shaders that only rendered the blue/red channels for water/lava to make it look tinted. I decided to change shaders depending on whether or not the player camera was about to submerge into water and lava in the collision logic.

#### Cheese and Spaghetti caves, water and lava post process shades, and more biomes

I worked on a few things. Firstly, I used 2 separate 3D perlin noise functions, one with a large frequency for cheese, and a smaller frequency for spaghetti in order to make cheese and spaghetti caves. This had the effect of making numerous thin strings of caves, with some intermediate large cheese holes. 

Then, I edited the lava and water shades to make newer post process effects that make it look like the water is shining in the player's eyes, and the lava effect is the same thing with all the white lines turned black to look like burns. 

Lastly, I created a new b2 variable in the terrain class, and used bilinear interpolation to interpolate between 4 biomes, as I added a desert biome where there is no water, and an ice biome where all the water is ice. All 4 biome have different terrain shapes on top, but the same cave systems below.


### [Steven Chang](https://github.com/zminsc)

#### Game Engine Tick Function and Player Physics

I had to add two new boolean fields to the struct InputBundle: qPressed and ePressed, to move the player up and down while in flight mode. I added boolean fields flightMode, flightModeModified, and jumping to the Player class. In particular, flightModeModified was added for smoother transitions between flightMode and non-flightMode, in the event the user held down the F button instead of clicking it. 

I also implemented the gridMarch method for collision raycasting, which was by far the most difficult part. To add and remove blocks, I simply made use of the Terrain class' getBlockAt() and setBlockAt() functions in addition to raycasting.

**Note:** Compatibility issues between Mac and Windows prevented the look implementation from working smoothly.