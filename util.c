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
int _getch(void);
int getch(void);
int create_dir(char*);

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
  
  if ((f=fopen(file,"r"))!=0) 
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

int _getch(void)
{
  return(getch());
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


int create_dir(char *mdir)
{
  int i,j;
  int subs=0;
  char t_dir[300];

  t_dir[0]='\0';
  
  
  for (i=0; (unsigned)i<strlen(mdir); i++)
    {
#ifdef __UNIX__
      if (mdir[i]=='/')
#endif
#ifndef __UNIX__
      if (mdir[i]=='\\')
#endif
	{
	  subs++;
	}

    }
  
  //normal file
  if (subs==0) 
    {
      return 0;
    }
  
  
  i=0;
  j=0;
  while (1)
    {
#ifdef __UNIX__
      if (mdir[i]=='/')
#endif
#ifndef __UNIX__
      if (mdir[i]=='\\')
#endif
	{
	  j++;
	  t_dir[i]='\0';

#ifdef __UNIX__
	  mkdir(t_dir,0xFFFF);
#endif
#ifndef __UNIX__
	  mkdir(t_dir);
#endif
	  if (j==subs) 
	    {
	      break;
	    }

#ifdef __UNIX__
	  t_dir[i]='/';
#endif
#ifndef __UNIX__
	  t_dir[i]='\\';
#endif
	}
      else
	{
	  t_dir[i]=mdir[i];
	}
      i++;
    }
  return 1;
}



