For 200D.101 using [stm32doom](https://github.com/floppes/stm32doom)

To compile:

```
cd platform/200D.101
make clean && make -j32
```
With WIFI updater([WIP](https://github.com/coon42/drysh_tools)):
```
make clean && make CFLAG_USER='-DSSID=\"turtius\" -DPASS=\"turtius@123\" -DIP=\"192.168.100.13\"' -j32
```
Attributions:

-[kitor](https://github.com/kitor) & names_are_hard: display code

-[coon](https://github.com/coon42): wifi updater


Magic Lantern
=============

Magic Lantern (ML) is a software enhancement that offers increased
functionality to the excellent Canon DSLR cameras.
  
It's an open framework, licensed under GPL, for developing extensions to the
official firmware.

Magic Lantern is not a *hack*, or a modified firmware, **it is an
independent program that runs alongside Canon's own software**. 
Each time you start your camera, Magic Lantern is loaded from your memory
card. Our only modification was to enable the ability to run software
from the memory card.

ML is being developed by photo and video enthusiasts, adding
functionality such as: HDR images and video, timelapse, motion
detection, focus assist tools, manual audio controls much more.

For more details on Magic Lantern please see [http://www.magiclantern.fm/](http://www.magiclantern.fm/)
