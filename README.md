Avisynth FPS Downconversion Filter
==================================

This filter reduces the framerate of a video by 1/2, by blending odd and even
frames together. However, it does this in a smart way such that in case of
duplicate frames, it will do the smart thing to remove unnecessary blurring in
the output video.

Source: https://github.com/arkeet/fpsdown

Binary download: http://dl.dropbox.com/u/33903796/avs/fpsdown.zip

Demonstration: http://www.youtube.com/watch?v=XULTJAWCDWk


Interface
---------

    FpsDown(clip clip [, bool maxnorm, int threshold, bool debug,
                         bool adjust3xslowdown, float gamma])

- clip: This is the clip to reduce the framerate of. Note that only clips in
  RGB32 format are accepted.

- maxnorm: Selects which norm to use when determining how close two consecutive
  frames have to be to be considered duplicates. If false, uses an L^1 norm
  (sum over all pixels of absolute values of differences in each channel). If
  true, uses a L^infinity (max) norm (the largest absolute value of differences
  over all pixels). Default value is true.

- threshold: Two consecutive frames are considered duplicates if the norm (as
  determined by the maxnorm argument) is smaller than this value. Default value
  is 8 if maxnorm = true, and 2000000 if maxnorm = false.

- debug: If true, this will show a small square in the bottom left which is
  normally white, but black when duplicate frames which should not be blended
  are detected.

- adjust3xslowdown: If true, this will blend frames differently in situations
  with 3 consecutive duplicate frames, in an attempt to make the result
  smoother and less jittery. Default value is false.

- gamma: Set the gamma value for gamma correction when blending frames
  together. Default value is 2.2.


Notes
-----

To load the plugin, simply place fpsdown.dll in the Avisynth plugins directory.
Then, to do the framerate conversion, for default settings it suffices to just
have this line:

    FpsDown()

It is good to use the debug option when testing, however. If two frames which
are supposed to be duplicates are not actually detected as such (so that the
debug square remains white), try increasing the threshold.

Important note! Because I'm lazy, this function only accepts clips in RGB32
format. If necessary, use the ConvertToRGB32 function first.
