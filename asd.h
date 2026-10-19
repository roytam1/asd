#define __COLOR__

//select the target platform, windows is the default
//#define __DOS__
//or
//#define __UNIX__
#if !defined(__DOS__) && !defined(__UNIX__)
#define __WINDOWS__
#endif


//include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


//for stat in get_filesize and stat
#include <sys/stat.h>


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


#ifdef __WINDOWS__
 #include <windows.h>
 #include <io.h>
 #include <conio.h>
 #include <time.h>
 #include <sys/utime.h>


//MSVC has no dir.h and no struct ffblk, so here is a
//Borland compatible one (implemented in util.c)
struct ffblk {
  long handle;             //search handle (MSVC internal)
  unsigned char ff_attrib;  //file attributes
  unsigned short ff_ftime;  //dos packed time
  unsigned short ff_fdate;  //dos packed date
  unsigned long ff_fsize;   //file size
  char ff_name[260];        //name of file as ASCIIZ string
};


//dos packed file time (the same layout as the borland one)
struct ftime {
  unsigned ft_tsec:5;	/* 0-29, double to get real seconds */
  unsigned ft_min:6;	/* 0-59 */
  unsigned ft_hour:5;	/* 0-23 */
  unsigned ft_day:5;	/* 1-31 */
  unsigned ft_month:4;	/* 1-12 */
  unsigned ft_year:7;	/* since 1980 */
};


//dos file attributes (same values as the windows ones)
#define FA_RDONLY 0x01
#define FA_HIDDEN 0x02
#define FA_SYSTEM 0x04
#define FA_LABEL  0x08
#define FA_DIREC  0x10
#define FA_ARCH   0x20

#endif

#ifdef __DOS__
 #include <dos.h>
 #include <dir.h>
 #include <io.h>
 #define EXIT_SUCCESS 0
 #define EXIT_FAILURE -1
 #define _dos_setfileattr(a,b) _chmod(a,1,b)
#endif



#ifdef __WINDOWS__
int _gotoxy(int x, int y);
int _wherey(void);
int _wherex(void);
int tc(int _attr);
int _getc(FILE *stream);

//util.c (msvc compatibility layer)
//the crt filelength() only works on files opened for writing,
//so use our own one (util.c) instead
int asd_filelength(int fd);
#define filelength(fd) asd_filelength(fd)
int findfirst(const char *path, struct ffblk *ffblk, int attrib);
int findnext(struct ffblk *ffblk);
int _dos_setfileattr(const char *filename, int attr);
int setftime(int fd, struct ftime *ft);
#endif




//util.c
int file_exist(char*);


//asd.c
int read_f_archive(void);
int check_file(char *filename);
int next_file(long n_file);
int extract(char *filename);
int file_exist(char *file);
int list(char *filename);
int find(int num_args,char *args[]);
int add(char *filename, int argc, char *argv[]);
int sfx(char *filename);
void help();


//crc.c
unsigned long crc32(unsigned int ch, unsigned long crc32val);
unsigned long crc32_c(const unsigned char *s, unsigned int len);

//encode.c
int b_len(int i, int j);
void InsertNode(int r);
void DeleteNode(int p);
int Encode(char *in_file, char *out_file, int asd_val);
void encode(unsigned int hash_level_deep, unsigned char asd_extra_value, FILE *infile, FILE *outfile);




//color number (from conio.h)
#define YELLOW  14
#define RED     4
#define NORMAL  7
#define GREEN   2
#define CYAN    3
#define WHITE   15




//variables to be used in more than one places..

extern FILE *infile, *outfile;

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




