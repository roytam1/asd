#include "asd.h"


#ifdef __UNIX__

struct stat sta;

#endif




int _gotoxy(int, int);
int _wherey(void);
int tc(int);
int file_exist(char*);
long get_filesize(char*);
int _getc(FILE*);



//might be good to add better errormessage here..
int _getc(FILE *stream)
{
  int cd;
  if ( (cd=getc(stream)) == EOF) return(EOF);

  return(cd);
}



long get_filesize(char *FileName)
{
   
     
    struct stat file;
     
     if(!stat(FileName,&file))
     {
         return file.st_size;
     }

   
     return 0;
}



int file_exist(char *file)
{
#ifndef __UNIX__
  FILE *f;
  
  if (f=fopen(file,"r")) 
    {
      fclose(f);return(1);
    }
  else
    return(0);

#endif

#ifdef __UNIX__
  int res;
  struct stat statts;

  res=stat(file,&statts);
  if (res==0) 
    return(1);
  else
    return(0);

#endif
}

#ifdef __UNIX__

int tc(int c)
{
  int cc;
  if (c==7)
    {
      printf("\033[0m");
      return(0);
    }
  switch (c)
    {
    case 0: cc = 30; break;
    case 1: cc = 34; break;
    case 2: cc = 32; break;
    case 3: cc = 36; break;
    case 4: cc = 31; break;
    case 5: cc = 35; break;
    case 6: cc = 33; break;
    case 7: cc = 37; break;
    case 8:  cc = 30; break;
    case 9:  cc = 34; break;
    case 10: cc = 32; break;
    case 11: cc = 36; break;
    case 12: cc = 31; break;
    case 13: cc = 35; break;
    case 14: cc = 33; break;
    case 15: cc = 37; break;
    }
  if (c<7)
    printf("\033[%dm",cc);
  else
    printf("\033[1;%dm",cc);
  
  return(0);
}

int _gotoxy(int x,int y)
{
  //since we only use y=wherey we can do this 
  fflush(stdout);
  printf("\033[%dD",200);
  if (x>0) printf("\033[%dC",x);
  return(0);
}

int _wherey(void)
{
  return(0);
}

//work
int _getch(void)
{
  int kbdinput;
  struct termios t_orig, t_new;
  
  // We need to change terminal settings so getchar() does't
  // require a CR at the end. Also disable local echo.
  tcgetattr(0, &t_orig);
  t_new = t_orig;
  t_new.c_lflag &= ~ICANON; t_new.c_lflag &= ~ECHO;
  t_new.c_lflag &= ~ISIG;   t_new.c_cc[VMIN] = 1;
  t_new.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &t_new);
  
  // Get actual input
  kbdinput = getchar();
  
  // Reset terminal settings.
  tcsetattr(0, TCSANOW, &t_orig);

  return (kbdinput);  
}

//work
int filelength(int t)
{
  fstat(t,&sta);
  return(sta.st_size);
}


//just ignore this
//you could use stat and get status..
//or use

int _dos_setfileattr(char *fil, int attr)
{
  chmod(fil,attr);
}


int findnext(int a)
{
  return(0);
}

int findfirst()
{
  return(0);
}


#endif







#ifdef __WINDOWS__

//not defined in all win32 header versions
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

//_getch() is provided by the msvc crt (conio.h) so dont redefine it here


//length of the file behind a file descriptor
int asd_filelength(int fd)
{
  struct _stat st;

  if (_fstat(fd,&st)!=0) return(0);
  return((int)st.st_size);
}


//set the dos file attributes of a file
//(the dos attribute bits are the same as the windows ones)
int _dos_setfileattr(const char *filename,int attr)
{
  DWORD cur,newattr;

  cur=GetFileAttributes(filename);
  if (cur==INVALID_FILE_ATTRIBUTES) return(0);

  newattr=cur & FILE_ATTRIBUTE_DIRECTORY;

  if (attr & FA_RDONLY) newattr|=FILE_ATTRIBUTE_READONLY;
  if (attr & FA_HIDDEN) newattr|=FILE_ATTRIBUTE_HIDDEN;
  if (attr & FA_SYSTEM) newattr|=FILE_ATTRIBUTE_SYSTEM;
  if (attr & FA_ARCH)   newattr|=FILE_ATTRIBUTE_ARCHIVE;

  if (SetFileAttributes(filename,newattr)) return(0);
  return(-1);
}


//set the time of an open file (borland setftime())
int setftime(int fd,struct ftime *ft)
{
  struct tm t;
  struct _utimbuf ut;
  time_t tt;

  memset(&t,0,sizeof(t));
  t.tm_sec   = ft->ft_tsec*2;
  t.tm_min   = ft->ft_min;
  t.tm_hour  = ft->ft_hour;
  t.tm_mday  = ft->ft_day;
  t.tm_mon   = ft->ft_month-1;
  t.tm_year  = ft->ft_year+80;   //years since 1900
  t.tm_isdst = -1;

  tt=mktime(&t);
  if (tt==(time_t)-1) return(-1);

  ut.actime=tt;
  ut.modtime=tt;
  return(_futime(fd,&ut));
}


//fill in a borland style ffblk from a msvc _finddata_t
static void ffblk_from_finddata(struct ffblk *ffblk,struct _finddata_t *fd)
{
  time_t t;
  struct tm *lt;

  ffblk->ff_attrib=(unsigned char)(fd->attrib & 0xff);
  ffblk->ff_fsize=(unsigned long)fd->size;
  strncpy(ffblk->ff_name,fd->name,sizeof(ffblk->ff_name)-1);
  ffblk->ff_name[sizeof(ffblk->ff_name)-1]='\0';

  //convert to the dos packed time/date used by the archive format
  t=fd->time_write;
  lt=localtime(&t);
  if (lt==NULL) return;

  ffblk->ff_ftime=(unsigned short)((lt->tm_sec/2) | (lt->tm_min<<5) | (lt->tm_hour<<11));
  ffblk->ff_fdate=(unsigned short)(lt->tm_mday | ((lt->tm_mon+1)<<5) | ((lt->tm_year+1900-1980)<<9));
}


//find the first file matching the pattern (borland compatible)
int findfirst(const char *path,struct ffblk *ffblk,int attrib)
{
  struct _finddata_t fd;
  long handle;

  //the attribute mask is not used, all normal files are returned

  handle=_findfirst(path,&fd);
  if (handle==-1) return(-1);

  //skip directories (the dos version did that with the attribute mask)
  while (fd.attrib & _A_SUBDIR)
    {
      if (_findnext(handle,&fd)==-1)
        {
          _findclose(handle);
          return(-1);
        }
    }

  ffblk->handle=handle;
  ffblk_from_finddata(ffblk,&fd);
  return(0);
}


//find the next file matching the pattern (borland compatible)
int findnext(struct ffblk *ffblk)
{
  struct _finddata_t fd;

  if (_findnext(ffblk->handle,&fd)==-1)
    {
      _findclose(ffblk->handle);
      ffblk->handle=-1;
      return(-1);
    }

  while (fd.attrib & _A_SUBDIR)
    {
      if (_findnext(ffblk->handle,&fd)==-1)
        {
          _findclose(ffblk->handle);
          ffblk->handle=-1;
          return(-1);
        }
    }

  ffblk_from_finddata(ffblk,&fd);
  return(0);
}


int _gotoxy(int x, int y) {
 COORD c;
 c.X = x;
 c.Y = y;
 SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE), c);
 return(0);
}

int _wherey(void) {
 CONSOLE_SCREEN_BUFFER_INFO info;
 GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
 return info.dwCursorPosition.Y;
}

/*
int _wherex(void) {
 CONSOLE_SCREEN_BUFFER_INFO info;
 GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
 return info.dwCursorPosition.X;}
*/

int tc(int _attr) {
#ifdef __COLOR__
 SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), _attr);
#endif
 return(0);
}
#endif
#ifdef __DOS__
 #ifdef __COLOR__
  #define tc(a) textcolor(a)
  #define printf cprintf
 #endif 
// #define _wherex() wherex()
 #define _wherey() wherey()
 #define _gotoxy(a,b) gotoxy(a,b)
#endif
