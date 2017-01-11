wavfix v0.1
===

Wavfix is able to repair broken wav files while preserving metadata.

Wavfix is built upon real case scenarios where files are corrupted during recording. It's capacity to preserve metadata (all non-audio chunks) tend to keep software compatibility and workflow integrity, making wavfix usefull in professional and broadcast environment.

Wavfix __will never overwrite your original files__, but instead will create new ones if needed. So you're safe using it.

## Features
Wavfix currently support the following :

* All standard __RIFF / WAVE__ PCM files including all possible chunks.
* RIFF header size recovery.
* &lt;data&gt; chunk size recovery.
* &lt;data&gt; chunk reconstruction if missing or corrupted.
* &lt;fmt &gt; chunk reconstruction if missing or corrupted.


## Options

```
wavfix, version 0.1 by Adrien Gesta-Fline <github: agfline/wavfix>

wavfix can repair damaged wave files keeping all meta-chunks  intact,  thus
preserving metadata and proprietary stuff,  meaning software compatibility.
As a last resort, remember you still can use Audacity  'import > raw data'.

usage : wavfix <options> broken_file.wav

    audio options :
	-f, --frequency   <n>    set frequency to n Hz (44100, 48000, etc.)
	-b, --bit-depth   <n>    set bit-depth to n bits (16, 24, 32, etc.)
	-c, --channels    <n>    set channels number to  n  channels (1 for
                             mono, 2 for stereo, etc.)

	-F                       force using previous values instead of the
	                         ones contained in <bext> chunk if present.

    output options:
	-s, --suffix             repaired file names will  be  the same  as
                             the broken ones plus this suffix.  Default
                             is '_REPAIRED'.

    misc options :
	-N, --no-repair          use this option to only investigate files.
	-h, --help               show this help
```

## Examples

Simple file analysis, no repair :

```
$ wavfix -N file.wav
```
Analyse all files and repair if needed :

```
$ wavfix *.wav
```
If &lt;fmt &gt; chunk is missing and wavfix cannot detect audio parameters :

```
$ wavfix -f 48000 -b 16 -c 1 corrupted_file.wav
```

### Output Examples
Example from a malfunctioning __Sound Devices 552__ mixer :
```
$ wavfix ./SD552.wav

> Processing './SD552.wav' 
| Current file structure :
| ======================
|     <bext> chunk [offset: 0000000012; size: 0000000858 + 4 + 4;] 
|     <iXML> chunk [offset: 0000000878; size: 0000005226 + 4 + 4;] 
|     [NULL] chunk [offset: 0000006112; size: 0006696988 + 0 + 0;] 
|
| [w] missing <data> chunk. recovering.. 
|  |  trying to locate the biggest unknown bytes block..
|  |  got 6696988 bytes begining at offset 6112.
|  |  assume these are audio data. Rebuilding <data>..
|  |  done.
| [w] missing <fmt > chunk. recovering.. 
|  |  found <bext> chunk with coding_history data.
|  |  using it to retrieve audio parameters. Rebuilding <fmt >..
|  |  done.
|
| Recovered file structure :
| ========================
|     <bext> chunk [offset: 0000000012; size: 0000000858 + 4 + 4;] 
|     <iXML> chunk [offset: 0000000878; size: 0000005226 + 4 + 4;] 
|     <fmt > chunk [offset: 0000006112; size: 0000000016 + 4 + 4;] 
|     <data> chunk [offset: 0000006136; size: 0006696948 + 4 + 4;]
|
| [i] saving repaired file to './SD552_REPAIRED.wav'
| File successfully recovered.

```

Example from a crashed __parecord__ recording :
```
$ wavfix ./PARECORD.wav 


> Processing './PARECORD.wav' 
| [w] wrong RIFF size: 0000000008 B + 8 [file size: 0003140248 B;]
| Current file structure :
| ======================
|     <fmt > chunk [offset: 0000000012; size: 0000000016 + 4 + 4;] 
|     <data> chunk [offset: 0000000036; size: 0000000000 + 4 + 4;] 
|     [NULL] chunk [offset: 0000000044; size: 0003140204 + 0 + 0;] 
|
| [i] checking <data> chunk.. 
|  |  a block of 3140204 unknown bytes comes after <data> chunk.
|  |  assume those are audio data. Merging them with <data> chunk..
|  |  done.
|
| Recovered file structure :
| ========================
|     <fmt > chunk [offset: 0000000012; size: 0000000016 + 4 + 4;] 
|     <data> chunk [offset: 0000000036; size: 0003140204 + 4 + 4;]
|
| [i] saving repaired file to './PARECORD_REPAIRED.wav'
| File successfully recovered.
```

## Install

```
$ make
```

## How to contribute

Feel free to report any bug, any recovery failure (or success !) so i can adapt the code to some eventual special cases.

## License
Copyright © 2016, 2017 Adrien Gesta-Fline<br />
Wavfix is released under the __GNU AGPLv3__ : http://www.gnu.org/licenses/gpl-3.0.txt
