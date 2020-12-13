
sbtest.exe : sbtest.obj sbinfo.obj dsp.obj wave.obj dmabuf.obj
	wlink system dos &
		  option map &
		  name sbtest &
		  file sbtest.obj,sbinfo.obj,dsp.obj,wave.obj,dmabuf.obj

.c.obj:
	wcc /mm /2 /s /wx $*.c
