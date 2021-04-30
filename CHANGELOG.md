##### 2.1.0:
    Set MT mode: MT_NICE_FILTER.
    Added support for 8..16-bit clips.
    Added support for other chroma subsampling than 4:2:0.
    If the dimensions of the image aren't divisible by blockx and blocky, the right and/or bottom edges will be copied instead not processed.
    Added Linux building option.

##### 2.0 2013/11/09 - Developer: dubhater 
    This version now works by processing blocks rather than whole frames. Static rainbows can now be processed even if there's lots of motion in other parts of the image.
    The "scenelumathresh" parameter was renamed to "luma_thresh" and its default value is now 10.0
    The "conservativemask" parameter was renamed to "conservative_mask".
    
##### v1.1 2007/03/04 - Developer: Myrsoik 
    Corrected several bad assumptions about pitch and no longer requests frames out of range which makes it compatible with avisynth 2.5.7.
    Uses the whole image and not just the upper half to determine if there is too much motion, this reduces artifacts greatly in certain clips but the scenelumathresh argument now has to be twice as big as in previous versions for the same effect.
    Processes the image borders properly instead of just ignoring them.
    Can look two frames in either direction instead of just one frame in both. 
    This makes it possible to remove rainbows in frames right before and after scenechanges.
