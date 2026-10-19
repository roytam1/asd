#define __COLOR__

//#define __WINDOWS__      
//or
//#define __DOS__
//or
//#define __UNIX__


//include files
#include <stdio.h>
#include <stdlib.h>
#include "bitfile.h"

//for stat in get_filesize and stat
#include <sys/stat.h>



#define free_mem(a) { if (a) free(a); }



#ifdef __UNIX__
#include <time.h>
#include <fnmatch.h>
#include <dirent.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>



struct ftime {
  unsigned ft_tsec:5;	/* 0-29, double to get real seconds */
  unsigned ft_min:6;	/* 0-59 */
  unsigned ft_hour:5;	/* 0-23 */
  unsigned ft_day:5;	/* 1-31 */
  unsigned ft_month:4;	/* 1-12 */
  unsigned ft_year:7;	/* since 1980 */
};


struct ffblk {
  char lfn_magic[6];	    /* LFN: the magic "LFN32" signature */
  short lfn_handle;	    /* LFN: the handle used by findfirst/findnext */
  unsigned short lfn_ctime; /* LFN: file creation time */
  unsigned short lfn_cdate; /* LFN: file creation date */
  unsigned short lfn_atime; /* LFN: file last access time (usually 0) */
  unsigned short lfn_adate; /* LFN: file last access date */
  char ff_reserved[5];      /* used to hold the state of the search */
  unsigned char ff_attrib;  /* actual attributes of the file found */
  unsigned short ff_ftime;  /* hours:5, minutes:6, (seconds/2):5 */
  unsigned short ff_fdate;  /* (year-1980):7, month:4, day:5 */
  unsigned long ff_fsize;   /* size of file */
  char ff_name[260];        /* name of file as ASCIIZ string */
};

//#include <dirent.h>
#define FA_RDONLY 1
#define FA_HIDDEN 2
#define FA_SYSTEM 4
#define FA_LABEL  8
#define FA_DIREC  16
#define FA_ARCH   32

#endif


#define CMP_ZERO      0
#define CMP_FASTEST   1
#define CMP_FAST      8
#define CMP_NORMAL    48
#define CMP_GOOD      512
#define CMP_MAX       4095
#define CMP_ULTRA     65535




#ifdef __WINDOWS__
 #include <time.h>
 #include <windows.h>
 #include <dir.h>
 #include <io.h>
#endif

#ifdef __DOS__
 #include <time.h>
 #include <dos.h>
 #include <dir.h>
 #include <io.h>
 #include <conio.h>

// #define EXIT_SUCCESS 0
// #define EXIT_FAILURE -1
 #define _dos_setfileattr(a,b) _chmod(a,1,b)
#define _gotoxy(a,b) gotoxy(a,b)
#define _tc(a) textcolor(a)
#define _getch() getch()
#endif



#ifdef __WINDOWS__
int _gotoxy(int x, int y);
int _wherey(void);
int _wherex(void);
int tc(int _attr);
#endif




//util.c
int file_exist(char*);
#ifndef __DOS__
int _getch(void);
#endif


//asd.c
int read_f_archive(void);
int check_file(char *filename);
long next_file(long n_file);
int extract(char *filename);
int file_exist(char *file);
int list(char *filename);
int find(int num_args,char *args[]);
int add(char *filename, int argc, char *argv[]);
int sfx(char *filename);
void help();

//decode.c

int literal_get_lowerst(void);
void literal_build_tree(void);
void literal_rescale_counts(void);
int literal_get_literal(BITFILE *);
int offset_get_lowerst(void);
void offset_build_tree(void);
void offset_rescale_counts(void);
int offset_get_offset(BITFILE*);


//crc.c
unsigned long crc32(unsigned int ch, unsigned long crc32val);
unsigned long crc32_c(const unsigned char *s, unsigned int len);

//encode.c
//int b_len(int i, int j);
//void InsertNode(int r);
//void DeleteNode(int p);
//void encode(unsigned int,unsigned char, FILE*,BITFILE*);
void encode(unsigned long, unsigned int, unsigned int, unsigned int,unsigned int, unsigned int, unsigned int, FILE*, BITFILE*);

//util.c
#ifndef __DOS__
int _gotoxy(int, int);
int _wherey(void);
#endif
int tc(int);
int file_exist(char*);
long get_filesize(char*);
int _getc(FILE*);
#ifndef __DOS__
int _dos_setfileattr(char *, int);
#endif
int create_dir(char*);




//color number (from conio.h)
#define YELLOW  14
#define RED     4
#define NORMAL  7
#define GREEN   2
#define CYAN    3
#define WHITE   15




//variables to be used in more than one places..

extern FILE *infile, *outfile;
extern BITFILE *bit_infile, *bit_outfile;

//for crc32_value
extern unsigned long crc32_val;

//extra asd default 15
//extern int asd_extra;
extern long num_files;

//each filesize counter (resetted when new file appear in solid archive)
extern unsigned long filesize_count;

//1= true 0=false

//to test archive
extern char testa,t_testa;
//always
extern char always;
//subdir
extern char subdir;

extern int error;

extern unsigned int buffer_bits,window_bits;
extern unsigned long buffer_size;
extern int offset_huff_rescale_factor;
extern int offset_huff_rehuff_factor;
extern int literal_huff_rescale_factor;
extern int literal_huff_rehuff_factor;

//for code table
extern unsigned long ct_bits;
extern unsigned long ct_value;
extern unsigned long ct_code;

extern char *buffer;
extern unsigned int bpos;

/* huffman coding part */
extern int LITERAL_NUM_CODES;
extern unsigned long *literal_count;
extern int *literal_high_tree,*literal_low_tree;
extern int *literal_from_tree;
extern short *literal_counted_tree;
extern unsigned int literal_scale_counter;
extern unsigned int literal_huff_counter;
extern unsigned int literal_rehuff;
extern unsigned int literal_rescale;

extern int OFFSET_NUM_CODES;
extern unsigned long *offset_count;
extern int *offset_high_tree,*offset_low_tree;
extern int *offset_from_tree;
extern short *offset_counted_tree;
extern unsigned int offset_scale_counter;
extern unsigned int offset_huff_counter;
extern unsigned int offset_rehuff;
extern unsigned int offset_rescale;



