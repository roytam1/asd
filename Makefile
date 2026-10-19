#################################################################
#
#    ASD Archiver Makefile for Borland C++ 5.0
#
#    For Microsoft Visual C++ 6.0 use build.bat instead
#
#################################################################

all:    asd.exe unasd.exe

asd.exe:    asd.obj encode.obj crc.obj util.obj bitfile.obj decode.obj
    bcc32 -M -easd asd.obj encode.obj crc.obj util.obj bitfile.obj decode.obj

unasd.exe:    unasd.obj crc.obj util.obj bitfile.obj decode.obj
    bcc32 -M -eunasd unasd.obj crc.obj util.obj bitfile.obj decode.obj

    del unasd.obj asd.obj crc.obj encode.obj util.obj bitfile.obj decode.obj
    del unasd.tds asd.tds
    del asd.map unasd.map

asd.obj:    asd.c asd.h

unasd.obj:    unasd.c asd.h

encode.obj:    encode.c asd.h

crc.obj:    crc.c asd.h

util.obj:    util.c asd.h

bitfile.obj:    bitfile.c asd.h

decode.obj:    decode.c asd.h

.c.obj:
    bcc32 -D__WINDOWS__ -c -N -k $<

clean:
    del asd.exe unasd.exe

# end of makefile
