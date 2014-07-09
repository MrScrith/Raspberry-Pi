General comments:
Compiling this code out-of-the-box will result in a 320x240 interpretation of the framebuffer contents.
To achieve a 640x480 resolution, uncomment the loadFrameBuffer_640x480 function call, comment the loadFrameBuffer_320x240 function call, and re-compile. The raspberry pi framebuffer settings in /boot/config.txt must be configured for consistency with the specified resolution.

Some comments on licensing:
The mzt280 source is based on the work over at https://github.com/yaolet/mztx06a

While no formal license is specified, the author does extend the invitation to 
"Feel free to spit, twit, modify or legendise [the source]"

The bcm2835 source is covered under the GPLv2 COPYING license included in the MZT280-PI-EXT directory.

Any bits which are uniquely attributable to this project page may be utilized in the same spirit as the yaolet project. 
Feel free to mix, remix, modify, etc.
