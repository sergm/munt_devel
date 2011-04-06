MT32Emu WinMM Application
-------------------------

This win32 application is intended for use with some sort of a MIDI thru driver. It opens MIDI in port 0 and plays MIDI messages via the mt32emu engine. As opposed to DirectMusic driver it features:

* use of various sample rates (including MT-32 native 32 kHz) that makes the emu sounds much better

* no problem usage within x64 environment

* installing/rebooting do not required, simpler in control, usage and debugging

* possibility to make wave recordings with exact MIDI message timing (It is observed that DirectMusic driver of v. 0.1.3 sometimes introduces minor time shifts in MIDI message playback and I haven’t found any inconsistencies in the sources)

* a good example of using mt32synth API, much clearer then in the driver

* DirectMusic is deprecated and Microsoft discourages everyone in future developments. For now, there are difficulties in compiling the driver already.

--
As an example of a MIDI thru, it is proposed to use MIDI Yoke Junction, from http://www.midiox.com/, currently Version 1.75, 09-23-07.

              
License
-------

Copyright (C) 2003, 2004, 2005, 2011 Dean Beeler, Jerome Fisher
Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


Trademark disclaimer
--------------------

Roland is a trademark of Roland Corp. All other brand and product names are
trademarks or registered trademarks of their respective holder. Use of
trademarks is for informational purposes only and does not imply endorsement by
or affiliation with the holder.
