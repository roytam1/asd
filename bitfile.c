#include "asd.h"

void FseekInputBitFile(BITFILE *bitfile, long position)
{
  fseek( bitfile->file,position,SEEK_SET);
  bitfile->rack = 0;
  bitfile->mask = 0x80;
}

void FseekOututBitFile(BITFILE *bitfile, long position)
{
  fseek( bitfile->file,position,SEEK_SET);
  bitfile->rack = 0;
  bitfile->mask = 0x80;
}

unsigned long GetInputBitFilePos( BITFILE *bitfile)
{
  fflush(bitfile->file);
  return (filelength(fileno(bitfile->file)));
}

unsigned long GetOutputBitFilePos( BITFILE *bitfile)
{
  fflush(bitfile->file);
  return (filelength(fileno(bitfile->file)));
}

BITFILE *OpenOutputBitFile(char *name)
{
  BITFILE *bitfile;

  bitfile = (BITFILE *) calloc( 1, sizeof( BITFILE ) );
  if ( bitfile == NULL )
    {
      return( bitfile );
    }
  bitfile->file = fopen( name, "wb" );
  bitfile->rack = 0;
  bitfile->mask = 0x80;
  return( bitfile );
}

BITFILE *OpenInputBitFile(char *name)
{
  BITFILE *bitfile;
  
  bitfile = (BITFILE *) calloc( 1, sizeof( BITFILE ) );
  if ( bitfile == NULL )
    {
      return( bitfile );
    }
  bitfile->file = fopen( name, "rb" );
  bitfile->rack = 0;
  bitfile->mask = 0x80;
  return( bitfile );
}

void CloseOutputBitFile(BITFILE *bitfile)
{
  if ( bitfile->mask != 0x80 )
    {
      if ( putc( bitfile->rack, bitfile->file ) != bitfile->rack )
	{
	  printf("ASD Fatal error closebitfile !\n");
      }
    }
  fclose( bitfile->file );
  free_mem( (char *) bitfile );
}

void CloseInputBitFile(BITFILE *bitfile)
{
  fclose( bitfile->file );
  free_mem( (char *) bitfile );
}

void OutputBit(BITFILE *bitfile,int bit)
{
  if ( bit )
    {
      bitfile->rack |= bitfile->mask;
    }
  bitfile->mask >>= 1;
  if ( bitfile->mask == 0 ) 
    {
      if ( putc( bitfile->rack, bitfile->file ) != bitfile->rack )
	{
	  printf("ASD Fatal error in outputbit!\n");
	}
      bitfile->rack = 0;
      bitfile->mask = 0x80;
    }
}

void OutputBits(BITFILE *bitfile,unsigned long code,int count)
{
  unsigned long mask;
  mask = 1L << ( count - 1 );
  while ( mask != 0) 
    {
      if ( mask & code )
	{
	  bitfile->rack |= bitfile->mask;
	}
      bitfile->mask >>= 1;
      if ( bitfile->mask == 0 ) 
	{
	  if ( putc( bitfile->rack, bitfile->file ) != bitfile->rack )
	    {
	      printf("fatal error outputbit!\n");
	    }
	  bitfile->rack = 0;
	  bitfile->mask = 0x80;
        }
      mask >>= 1;
    }
}

int InputBit(BITFILE *bitfile)
{
  int value;
  
  if ( bitfile->mask == 0x80 ) 
    {
      bitfile->rack = getc( bitfile->file );
      if ( bitfile->rack == EOF )
	{
	  return(B_EOF);
	}
    }
  value = bitfile->rack & bitfile->mask;
  bitfile->mask >>= 1;
  if ( bitfile->mask == 0 )
    {
      bitfile->mask = 0x80;
    }
  return( value ? 1 : 0 );
}

unsigned long InputBits(BITFILE *bitfile,int bit_count)
{
  unsigned long mask;
  unsigned long return_value;
  
  mask = 1L << ( bit_count - 1 );
  return_value = 0;
  while ( mask != 0) 
    {
      if ( bitfile->mask == 0x80 ) 
	{
	  bitfile->rack = getc( bitfile->file );
	  if ( bitfile->rack == EOF )
	    {
	      return(B_EOF);
	    }
	}
      if ( bitfile->rack & bitfile->mask )
	{
	  return_value |= mask;
	}
      mask >>= 1;
      bitfile->mask >>= 1;
      if ( bitfile->mask == 0 )
	{
	  bitfile->mask = 0x80;
	}
    }
  return( return_value );
}

void FilePrintBinary(FILE *file,unsigned int code,int bits)
{
  unsigned int mask;
    
  mask = 1 << ( bits - 1 );
  while ( mask != 0 ) 
    {
      if ( code & mask )
	{
	  fputc( '1', file );
	}
      else
	{
	  fputc( '0', file );
	}
      mask >>= 1;
    }
}
