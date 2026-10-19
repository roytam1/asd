// ------------------------------------------------------------------
//  ASD - Archiever SFX version 0.1.5  (c) 1996-1997 Tobias Svensson
//
//  sources released under GPL2
// ------------------------------------------------------------------

#include "asd.h"

//windows
//#define sfx_data_size 34816

//unix
#define sfx_data_size 11861

#define version  "0.1.5"


/*
header:            bytes:
ASD01(asc26)        (6)
-
files in archive    (2)
-
-loop for (files in archive)
length of filename  (1)
filename            (0-255)
filesize            (4)
filcrc32            (4)
filedate / time     (4)
fileattributes      (2)
-
asd extra           (1)
the file            (x---)
*/


//variables in this file specific


//file struct
typedef struct t_file
{
  unsigned int filename_length;
  char *filename;
  unsigned long filesize;
  unsigned long filecrc;
  unsigned long filetime;
  unsigned int fileattribute;
} file_struct;

file_struct *f_archive;

FILE *infile, *outfile;

#define buffer_size 4096
#define minhit 3

//might be  16+minhit+256 extra =max win_size
static unsigned char buffer[buffer_size+minhit+15+256];
static unsigned int bpos;

//for crc32_value
unsigned long crc32_val;

//extra asd default 15
unsigned int asd_extra;
long num_files;

char *all_files="*.*";

//each filesize counter (resetted when new file appear in solid archive)
unsigned long filesize_count;

//1= true 0=false
//to test archive
char testa,t_testa;
//always
char always;
//subdir
char subdir;
//to disable attributes
char n_attrib;
//for wherey
int xpos,ypos;

//permission
char noperm;

//to count errors during for example decompress and test
unsigned int errors;

unsigned int c_hash_deep;


int read_f_archive(void);
int check_file(char*);
int next_file(long);
int extract(char*);
int file_exist(char*);




//don't forget to free mem too later..
int read_f_archive(void)
{
  int i,j,c,d;
  
  //num_files
  if ((num_files=_getc(infile))==EOF) exit(-1);

  num_files+=_getc(infile)*0x100;
  
  //allocating memmory
  f_archive = (file_struct*) malloc ((sizeof(file_struct))* (num_files));

  for (i=0; i<num_files; i++)
    {
      c=_getc(infile);

      //allocating memmory for name +1 for null too
      f_archive[i].filename = (char*) malloc (sizeof(char)*(c+1));

      for (j=0; j<c; j++)
	{
	  f_archive[i].filename[j]=_getc(infile);
	}
      //add important null char
      f_archive[i].filename[j]='\0';
      
      f_archive[i].filesize=_getc(infile);
      f_archive[i].filesize+=_getc(infile)*0x100;
      f_archive[i].filesize+=_getc(infile)*0x10000;
      f_archive[i].filesize+=_getc(infile)*0x1000000;

      f_archive[i].filecrc=_getc(infile);
      f_archive[i].filecrc+=_getc(infile)*0x100;
      f_archive[i].filecrc+=_getc(infile)*0x10000;
      f_archive[i].filecrc+=_getc(infile)*0x1000000;;

      f_archive[i].filetime=_getc(infile);
      f_archive[i].filetime+=_getc(infile)*0x100;
      f_archive[i].filetime+=_getc(infile)*0x10000;
      f_archive[i].filetime+=_getc(infile)*0x1000000;

      f_archive[i].fileattribute=_getc(infile);
      f_archive[i].fileattribute+=_getc(infile)*0x100;
      if (n_attrib==1)
	f_archive[i].fileattribute=0;

    }
  return(num_files);
}




int check_file(char *filename)
{
  int result;
  char *temp_filename;
  char *asd_filename;
  int c1,c2,c3;

  result=0;

  //room for the name + ".asd" + null
  asd_filename=(char*) malloc (sizeof(char)*(strlen(filename)+6));
  strcpy(asd_filename,filename);

  //room for the name + null
  temp_filename = (char*) malloc ((sizeof(char)*(strlen(filename)+1)) );
  strcpy(temp_filename,filename);

  if ((infile = fopen(filename,"rb"))==NULL) 
    {
      result=1;
      strcat(asd_filename,".asd");
      if ((infile = fopen(asd_filename,"rb"))==NULL) 
	{
 	  tc(RED);printf("File [%s] or [%s.asd] does not exist !\n",temp_filename,temp_filename);tc(NORMAL);
 	  free(asd_filename);
 	  free(temp_filename);
 	  exit(EXIT_FAILURE);
	}
    }
  free(asd_filename);

  //note: the chars must be read one by one, the order of the
  //evaluation of the operands is not the same in every compiler
  c1=_getc(infile);
  c2=_getc(infile);
  c3=_getc(infile);

  if ( (c1!='A') | (c2!='S') | (c3!='D')  )
    {
      //if file is exe file
      if  (filelength(fileno(infile)) > sfx_data_size)
	{
	  fseek(infile,sfx_data_size,SEEK_SET);
	  c1=_getc(infile);
	  c2=_getc(infile);
	  c3=_getc(infile);
	  if ( (c1=='A') && (c2=='S') && (c3=='D')  )
	    {
	      c1=_getc(infile);
	      c2=_getc(infile);
	      c3=_getc(infile);
	      if ( (c1!='0') | (c2!='1') | (c3!=26)  )
		{
		  tc(RED);printf("NOT an ASD 0.1.x archive, please download latest ASD archiver and try again !\n");tc(NORMAL);
		  fclose(infile);
		  free(temp_filename);
		  exit(EXIT_FAILURE);
		}
	      free(temp_filename);
	      return(result);
	    }
	  
	}
      tc(RED);printf("NOT an ASD archive file !\n");tc(NORMAL);
      fclose(infile);
      free(temp_filename);
      exit(EXIT_FAILURE);
    }
  
  c1=_getc(infile);
  c2=_getc(infile);
  c3=_getc(infile);
  if ( (c1!='0') | (c2!='1') | (c3!=26)  )
    {
      tc(RED);printf("NOT an ASD 0.1.x archive, please download latest ASD archiver and try again !\n ");tc(NORMAL);
      fclose(infile);
      free(temp_filename);
      exit(EXIT_FAILURE);
    }
  free(temp_filename);
  return(result);
}




int next_file(long n_file)
{
  int ch;  
  int ddate,ttime,date,time;
  struct ftime ft;


  //check crc32 only if n_files >0 (before any n_file =-1)
  if ((n_file>-1) && (noperm==0))
    if (f_archive[(n_file)].filecrc==(crc32_val^0xffffffff))
      {
	_gotoxy(20,_wherey());
	tc(GREEN);printf("OK\n");tc(NORMAL);
      }
    else
      {
	_gotoxy(20,_wherey());
	tc(RED);printf("CRC ERROR !\n");tc(NORMAL);
	errors++;
      }
  
  if (noperm==1)
    {
      _gotoxy(20,_wherey());
      tc(RED);printf("No permission to write file! \n");
      tc(NORMAL);
      errors++;
    }


  noperm=0;

  
  //set filedate
  ft.ft_tsec=(f_archive[(n_file)].filetime & 0x0000001F);
  ft.ft_min=(f_archive[(n_file)].filetime & 0x000007E0) >> 5;
  ft.ft_hour=(f_archive[(n_file)].filetime & 0x0000F800) >> 11;
  ft.ft_day=(f_archive[(n_file)].filetime & 0x001F0000) >> 16;
  ft.ft_month=(f_archive[(n_file)].filetime & 0x01E00000) >> 21;
  ft.ft_year=(f_archive[(n_file)].filetime & 0xFE000000) >> 25;

#ifdef __UNIX__
  if ((testa!=1) && (n_file!=-1)) utimes(f_archive[(n_file)].filename,&ft);
#endif
#ifndef __UNIX__
  if ((testa!=1) && (n_file!=-1)) setftime(fileno(outfile),&ft);
#endif

  //not first time (no file then)..
  if (n_file!=-1 && testa!=1)
    fclose(outfile);
  

  //set file attribute (need to be done after the file is closed)
  if ((testa!=1) && (n_file!=-1) && (n_attrib==0) && (f_archive[(n_file)].fileattribute!=0)) _dos_setfileattr(f_archive[(n_file)].filename,(f_archive[(n_file)].fileattribute));

  //no more files in archive
  if ((num_files-1)==n_file) return(-1);

  //time to create the next file to be unarchived
  //check for file exist to be added..
  printf("%-19s",f_archive[(n_file+1)].filename);

  //printf("%-19s",f_archive[(n_file+1)].filename);
  if (strlen(f_archive[(n_file+1)].filename)>18) printf("%-20s","\n");

  if ((testa!=1)  || (t_testa==1))
    {
      //if you entered N previously
      testa=0;
      //TODO check and warn if it exist before..
      if ((file_exist(f_archive[(n_file+1)].filename)) && always==0)
	{
	  _gotoxy(20,_wherey());
	  tc(RED);printf("Already exist !   Overwrite <Yes/No/Always yes>");tc(NORMAL);
	  do 
	    ch=_getch();
	  while((ch!='A') && (ch!='a') && (ch!='Y') && (ch!='y') && (ch!='N') && (ch!='n'));


      	  _gotoxy(20,_wherey());printf("                                                ");
  
	  if ((ch=='A') || (ch=='a')) always=1;
	  if ((ch=='N') || (ch=='n')) 
	    {
	      //t_testa means (ask if test next file)
	      t_testa=1;
	      testa=1;
	    }
	}
      
      //set to normal as default
      if (testa!=1) _dos_setfileattr(f_archive[(n_file+1)].filename,32);
      
      //Here we should have the autocreate dir

      //open next file
      if (testa!=1) 
	{
	  //autocreate dir here

	  //then 
	  if ((outfile=fopen(f_archive[(n_file+1)].filename,"wb"))==NULL)
	    {
	      t_testa=1;
	      testa=1;
	      noperm=1;
	      tc(RED);
	      //	      printf("Error: Can not write this file, no permission !");
	      tc(NORMAL);
	    }
	}
      
    }
  //resett crc for new file
  crc32_val=0xffffffff;

  filesize_count=0;
  return(n_file+1);
}




int extract(char *filename)
{
  unsigned int c,d,i,j,k,l,hit,pos,tmp;
  //-1 first..to be 0 then..
  long f_num=-1; //first -1 to be 0 at first file

  filesize_count=0;
  bpos=0;

  //open infile (add errorchecking here later..)
  if ((infile=fopen(filename,"rb"))==NULL) 
    {
      tc(RED);
      printf("ERROR: could not open infile (permission denied) \n");
      tc(NORMAL);
    }


  //check files...  
  check_file(filename); 
  
  tc(YELLOW);printf("- ");tc(NORMAL);
  if (testa==1)
    printf("\rTesting archive: ");
  else
    printf("Extracting archive: ");
  tc(GREEN);printf("[");tc(NORMAL);printf("%s",filename);tc(GREEN);printf("]\r\n");tc(NORMAL);     

  //get files...  
  read_f_archive();

  f_num=next_file(f_num);

  //get asd extra
  asd_extra=_getc(infile);

  //can also be done with memset(buffer,' ',buffer_size);

  for (i=0; i<buffer_size; i++)
    buffer[i]='0';
  
  while (1)
    {
      if ((d=_getc(infile)) == EOF) break;
      for (k=128; k>=1; k-=(k/2))
	{
	  if (d>=k)
	    {
	      d-=k;
	      //code
	      if ((i=_getc(infile)) == EOF) break;
	      if ((j=_getc(infile)) == EOF) break;
	      hit=(i>>4)+minhit;
	      if (hit==(15+minhit)) hit=hit+asd_extra;
	      pos=((i & 0x0f)*256) + j;
	      for (l=0; l<hit;  l++)
		{
		  //next file ?
		  while (f_archive[f_num].filesize==filesize_count)
		    if ((f_num=next_file(f_num))==-1)
		      break;

		  c=buffer[(bpos+buffer_size-pos-1) % buffer_size];
		  if (testa!=1) putc(c,outfile);
		  crc32_val=crc32(c,crc32_val);
		  buffer[bpos++]=c;bpos = bpos % buffer_size;
		  filesize_count++;
		}
	    }
	  else
	    {
	      //no code
	      if ((c=_getc(infile)) == EOF) break;
	      
    	      //next file ?
	      while (f_archive[f_num].filesize==filesize_count)
		if ((f_num=next_file(f_num))==-1)
		  break;

	      if (testa!=1) putc(c,outfile);
	      crc32_val=crc32(c,crc32_val);
	      buffer[bpos++]=c;bpos = bpos % buffer_size;
	      filesize_count++;
	    }
	  //break when 1
	  if (k==1) break;
	}
    }

  //until no more files in archive
  if (f_num!=-1)
    while((f_num=next_file(f_num))!=-1) { }

  //Freeing memmory..
  free(f_archive);

  if (errors==0)
    {
      tc(YELLOW);printf("- ");tc(NORMAL);printf("All files OK !\n");tc(NORMAL);
    }
  else
    {
      tc(RED);printf("%ld Errors found !\n",errors);tc(NORMAL);
    }

  //close infile
  fclose(infile);
  return(0);
}











int main(int argc, char *argv[])
{
  int switches;
  int m;






  //below need to be initialised as 0
  noperm=0;
  always=0;
  testa=0;
  t_testa=0;
  subdir=0;
  errors=0;
  num_files=0;
  switches=0;
  //default hash_deep
  //asd_extra default 20
  asd_extra=20;
  
  printf("\r\n");
  tc(NORMAL);
  printf("ASD - archiver SFX version %s SFX, Copyright (c) 1996-1997 Tobias Svensson\r\n",version);
  printf("Freeware archiver from Sweden,  *** with Long filename supported ***\n\r");

  printf("\r\nPress almost any key to start extraction, or ESC to quit ! \r\n");
  testa=0;
  testa=_getch();
  if (testa==27) exit(-1);

  
  //extract archive
  extract(argv[(0)]);
  
  return(EXIT_SUCCESS);
}
