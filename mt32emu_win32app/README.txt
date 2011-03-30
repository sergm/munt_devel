MT32Emu WinMM Application
*************************

This win32 application is intended for use with some sort of a MIDI thru driver. It opens MIDI in port 0 and plays MIDI messages via the mt32emu engine. As opposed to DirectMusic driver it features:

* use of various sample rates (including MT-32 native 32 kHz) that makes the emu sounds much better

* no problem usage within x64 environment

* installing/rebooting do not required, simpler in control, usage and debugging

* possibility to make wave recordings with exact MIDI message timing (It is observed that DirectMusic driver of v. 0.1.3 sometimes introduces minor time shifts in MIDI message playback and I haven’t found any inconsistencies in the sources)

* a good example of using mt32synth API, much clearer then in the driver

* DirectMusic is deprecated and Microsoft discourages everyone in future developments. For now, there are difficulties in compiling the driver already.

--
As an example of a MIDI thru, it is proposed to use MIDI Yoke Junction, from http://www.midiox.com/, currently Version 1.75, 09-23-07.