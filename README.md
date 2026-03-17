# Second Reality for IRIX

Second Reality is DOS demo created by Future Crew.
The demo won first place at the Assembly 1993 demoparty.


This repo is based on a fork of:
https://github.com/XorJS/SecondRealityPlusPlus
..which is a version in C++ for linux of:
https://github.com/ConspiracyHu/SecondRealityW32
..which is a W32 port of the original DOS demo:
https://github.com/mtuomi/SecondReality


Original code by:
    - Sami "PSI" Tammilehto
    - Mika "Trug" Tuomi
    - Arto "Wildfire" Vuori
    - st3play v1.01 by Olav "8bitbubsy" Sørensen (https://16-bits.org)


## IRIX port

Modified to run on IRIX big endian with OpenGL 1.3

Needs SDL2 with GL support (included in release bundle).
Needs graphics card with TRAM. Tested on VPro10/12. 
SSI w/o TRAM will not work.

flexion 2026


## Build on IRIX

You will need SGUG-RSE gcc toolchain, SDL2-dev headers, flx-SDL2 from flexions SGUG-RSE tdnf repo or your own SDL2 library with GL hardware rendering support.
