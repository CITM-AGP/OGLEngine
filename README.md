# OGLEngine

by Aitor Simona & Daniel Lorenzo

## New Techniques:

### Bloom

![BloomON](docs/BloomON.PNG)
![BloomOFF](docs/BloomOFF.PNG)

Through the debug window, you can configure the blur kernel radius, the bloom brightest threshold and
the blur LODs intensities. Image below.

![BloomOptions](docs/BloomOptions.PNG)

Uses BLOOM / BLOOM_BLUR and BLOOM_BRIGHTEST shaders

### Relief mapping

## Work Distribution:

Aitor:

	- deferred renderer
	- shaders 
	- entities / lights
	- framebuffer (shaders relationship & texture fill)
	- bloom

Dani:

	- framebuffer (API)
	- WASD and orbit camera controls
	- editor support (imgui)
	- entity realtime movement
	- relief mapping
	
