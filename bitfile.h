#ifndef _BITFILE_H
#define _BITFILE_H

#define B_EOF 0xFFFFFFFF

typedef struct bitfile {
    FILE *file;
    unsigned char mask;
    int rack;
} BITFILE;

unsigned long GetInputBitFilePos( BITFILE *bitfile);
unsigned long GetOutputBitFilePos( BITFILE *bitfile);
void          FseekInputBitFile(BITFILE *bitfile,long position);
void          FseekOutPutBitFile(BITFILE *bitfile,long position);
BITFILE       *OpenInputBitFile( char *name );
BITFILE       *OpenOutputBitFile( char *name );
void          OutputBit( BITFILE *bitfile, int bit );
void          OutputBits( BITFILE *bitfile, unsigned long code, int count );
int           InputBit( BITFILE *bitfile );
unsigned long InputBits( BITFILE *bitfile, int bit_count );
void          CloseInputBitFile( BITFILE *bitfile );
void          CloseOutputBitFile( BITFILE *bitfile );
void          FilePrintBinary( FILE *file, unsigned int code, int bits );

#endif
