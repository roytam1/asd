// -------------------------------------------------------------
//  ASD - Archiever version 0.1.5 (c) 1996-1997 Tobias Svensson
//
//  sources released under GPL2
// -------------------------------------------------------------

#include "asd.h"

//for sfx files
#ifdef __WINDOWS__
#include "sfx_windows.h"
#endif
#ifdef __UNIX__
#include "sfx_linux.h"
#endif
#ifdef __DOS__
#include "sfx_dos.h"
#endif

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
int list(char*);
int find(int,char *[]);
int add(char*,int,char *[]);
int sfx(char*);


//don't forget to free mem too later..
int read_f_archive(void)
{
  int i,j,c,d;
  
  //num_files
  if ((num_files=_getc(infile))==EOF) exit(-1);

  num_files+=_getc(infile)*0x100;
  
  //no files in the archive ?
  if (num_files<1)
    {
      tc(RED);printf("Empty or corrupt archive !\n");tc(NORMAL);
      exit(EXIT_FAILURE);
    }
  
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

  //open infile (check_file() opens the file and also tries name.asd
  //if the file itself does not exist, it exits if that fails too)
  
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
      //no more files in the archive
      if (f_num==-1) break;
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

		  //no more files in the archive
		  if (f_num==-1) break;

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

	      //no more files in the archive
	      if (f_num==-1) break;

	      if (testa!=1) putc(c,outfile);
	      crc32_val=crc32(c,crc32_val);
	      buffer[bpos++]=c;bpos = bpos % buffer_size;
	      filesize_count++;
	    }
	  //no more files in the archive
	  if (f_num==-1) break;
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












int list(char *filename)
{
  unsigned char c;
  unsigned int i,j,k,x,y,num_files;
  unsigned long size,total_size,ttime,ddate;
  unsigned long crc;
  int c1,c2;

  char att[10]="- - - -  \0";
  total_size=0;

  //check so it is a ASD archive
  check_file(filename);

  tc(YELLOW);
  printf("- ");tc(NORMAL);printf("Listing archive: ");tc(GREEN);printf("[");tc(NORMAL);
  printf("%s",filename);
  tc(GREEN);printf("]\r\n\r\n");tc(NORMAL);
  
  //test this so it works ok.. (should return correct number of files..)
  num_files=_getc(infile); num_files+=_getc(infile)*0x100;

  printf("Filename            filesize     Crc32     Date       Time       Attribute\n");
  
  tc(GREEN);printf("\r---------------------------------------------------------------------------\n");tc(NORMAL);
  for (i=0; i<num_files; i++)
    {
      printf("\r");
      x=_getc(infile);
      for (j=0; j<x; j++)
	printf("%c",_getc(infile));
      if (j>16) printf("\n                ");
      for (k=j; k<16; k++) printf(" ");
      size=_getc(infile); size+=_getc(infile)*0x100;
      size+=_getc(infile)*0x10000; size+=_getc(infile)*0x1000000;
      total_size=total_size+size;
      printf("  %*ld ",11,size);
      crc=_getc(infile);         crc+=_getc(infile)*0x100;
      crc+=_getc(infile)*0x10000; crc+=_getc(infile)*0x1000000;
      printf("  %08lX ",crc);
      ttime=_getc(infile); ttime+=_getc(infile)*0x100;
      ddate=_getc(infile); ddate+=_getc(infile)*0x100;
      printf("  %ld-%02d-%02d %02d:%02d:%02d",((ddate & 0xfe00) >>9)+1980,
	     (ddate & 0x1e0) >>5,(ddate & 0x1f),(ttime & 0xf800) >>11,
	     (ttime & 0x7e0) >>5,(ttime & 0x1f) << 1);
      //note: the chars must be read one by one, the order of the
      //evaluation of the operands is not the same in every compiler
      c1=_getc(infile);
      c2=_getc(infile);
      y=c1+c2*256;

#ifndef __UNIX__
      if (y & FA_ARCH)   att[0]='N'; else att[0]='-';  
      if (y & FA_RDONLY) att[2]='R'; else att[2]='-';  
      if (y & FA_HIDDEN) att[4]='H'; else att[4]='-';  
      if (y & FA_SYSTEM) att[6]='S'; else att[6]='-';  
      printf("%*s",10,att);
#endif

#ifdef __UNIX__
      att[0]='-';
      if (y & S_IRUSR) att[1]='r'; else att[1]='-';  
      if (y & S_IWUSR) att[2]='w'; else att[2]='-';  
      if (y & S_IXUSR) att[3]='x'; else att[3]='-';  

      if (y & S_IRGRP) att[4]='r'; else att[4]='-';  
      if (y & S_IWGRP) att[5]='w'; else att[5]='-';  
      if (y & S_IXGRP) att[6]='x'; else att[6]='-';  

      if (y & S_IROTH) att[7]='r'; else att[7]='-';  
      if (y & S_IWOTH) att[8]='w'; else att[8]='-';  
      if (y & S_IXOTH) att[9]='x'; else att[9]='-';  
      printf("%*s",12,att);
#endif

      printf("\r\n");
    }
  tc(GREEN);
  printf("---------------------------------------------------------------------------\r\n");
  tc(YELLOW);
  printf("- ");tc(NORMAL);
  if (total_size>0)
    printf("Total file(s) in archive:%ld, Space saved %ld%\n",num_files,100-((filelength(fileno(infile))*100)/total_size) );
  else
    printf("Total file(s) in archive:%ld\n",num_files);
  tc(NORMAL);
  fclose(infile);
  return(0);
}














int find(int num_args,char *args[])
{
  struct ffblk ffblk;
  int c,i,j,done;
  char *read_buff[1024];
  unsigned int fa_type;
  unsigned long ttime,ddate;
  struct ftime ft;
  FILE *test_out_tempfile;

#ifdef __UNIX__
  DIR *dirp;
  struct dirent *dp;
  struct stat statts;
  int fnresult;

  struct tm *tminfo;
#endif
  

  num_files=0;
  j=0;
  if (num_args==0) 
    {
      j=1;num_args=1;
    }

  fa_type=FA_RDONLY+FA_HIDDEN+FA_SYSTEM+FA_ARCH;   //and if dir too add FA_DIREC
  //allocating memmory
  f_archive = (file_struct*) malloc ((sizeof(file_struct)));
  
#ifndef __UNIX__
  for (i=0; i<num_args; i++)
    {
      //16 instead of 0 dirname included 
      if (j==1)
	done = findfirst("*.*",&ffblk,fa_type);
      else
	done = findfirst(args[i],&ffblk,fa_type);
      
      while (!done)
	{
	  if (ffblk.ff_name)
	    //readable for me ?
	    if ((test_out_tempfile=fopen(ffblk.ff_name,"r"))==NULL) 
	      {
		printf("Permission denied when trying to read [%s]\n",ffblk.ff_name);
	      }
	    else
	      {
		fclose(test_out_tempfile);
	        
		//realloc
		f_archive = (file_struct*) realloc (f_archive,sizeof(file_struct)*(num_files+1));
		
		f_archive[num_files].filename_length=(unsigned int) strlen(ffblk.ff_name);
		f_archive[num_files].filename = (char*) malloc (sizeof(char)*(strlen(ffblk.ff_name)+1));//null too
		strcpy(f_archive[num_files].filename,ffblk.ff_name);
		f_archive[num_files].filesize=ffblk.ff_fsize;
		//note you need to set CRC value during compress..
		f_archive[num_files].filetime=ffblk.ff_ftime+ffblk.ff_fdate*65536;
		f_archive[num_files].fileattribute=ffblk.ff_attrib;
		if (n_attrib==1)
		  f_archive[num_files].fileattribute=0;
	       num_files++;
	      }
	  done = findnext(&ffblk);
	}
    }
#endif
  


#ifdef __UNIX__

  //need to add default +.+ too
  for (i=0; i<num_args; i++)
    {
      dirp=opendir(".");
      while (dp = readdir(dirp))
	{
	  if (j==1)
	    fnresult=fnmatch("*",dp->d_name,9);
	  else
	    fnresult=fnmatch((const char*) args[i], dp->d_name,0);
	  
	  if (strlen(dp->d_name)==1)
	    if ( (dp->d_name[0]=='.')) fnresult=-1;
	  
	  if (strlen(dp->d_name)==2)
	    if ( (dp->d_name[0]=='.') && (dp->d_name[1]='.')) fnresult=-1;

	  if (fnresult==0)
	    {
	      if (stat(dp->d_name,&statts)==-1) fnresult=-1;
	      
	      //a directory ?
	      if (S_ISDIR(statts.st_mode)) fnresult=-1;
	      
	      //readable for me ?
	      if ((test_out_tempfile=fopen(dp->d_name,"r"))==NULL) 
		{
		  printf("Permission denied when trying to read [%s]\n",dp->d_name);
		  fnresult=-1;
		}
	      else
		fclose(test_out_tempfile);
	    }
	  
	  //not ..  and not .
	  if (fnresult==0)
	    {
	      //add here..
	      f_archive = (file_struct*) realloc (f_archive,sizeof(file_struct)*(num_files+1));
	      f_archive[num_files].filename_length=(unsigned int) malloc (sizeof(unsigned int));
	      f_archive[num_files].filename_length=strlen(dp->d_name);
	      f_archive[num_files].filename = (char*) malloc (sizeof(char)*(strlen(dp->d_name)+1));//null too
	      strcpy(f_archive[num_files].filename,dp->d_name);
	      f_archive[num_files].filesize = (long) malloc (sizeof(long));
	      f_archive[num_files].filesize = statts.st_size;

	      tminfo=localtime(&statts.st_mtime);
	      f_archive[num_files].filetime = (long) malloc (sizeof(long));
	      f_archive[num_files].filetime =(tminfo->tm_sec) /2;
	      f_archive[num_files].filetime+=((tminfo->tm_min)    *  32);
	      f_archive[num_files].filetime+=((tminfo->tm_hour)   *  2048);
	      f_archive[num_files].filetime+=((tminfo->tm_mday)   *  64*1024);
	      f_archive[num_files].filetime+=((tminfo->tm_mon+1)  *  2048*1024);
	      f_archive[num_files].filetime+=((tminfo->tm_year-80)*  32*1024*1024);

	      f_archive[num_files].fileattribute = (int) malloc (sizeof(int));
	      f_archive[num_files].fileattribute = statts.st_mode;



	      if (n_attrib==1)
		f_archive[num_files].fileattribute=0;


	      num_files++;
	    }
	}
      closedir(dirp);
    }

#endif

  //no files to compress..
  if (num_files==0)
    {
      free(f_archive);
      return(-11);
    }
  
  return 0;
}
























int add(char *filenam, int argc, char *argv[])
{
  int c,i,j,k,l;
  int ch;
  long lj;
  FILE *out_tempfile,*in_tempfile;

  //this 3 lines and filenam in function is only for stupid gcc ;(
  //and free filename at the end here too..
  char* filename;
  filename = (char*) malloc((strlen(filenam)+6)*sizeof(char));
  strcpy(filename,filenam);


  //first make sure that extension of the file is .asd
  i=(strlen(filename))-1;

  if ((i<4) || !((filename[i-3]=='.') &&  (toupper(filename[i-2])=='A') &&  (toupper(filename[i-1])=='S') &&  (toupper(filename[i])=='D')))
    {
      strcat(filename,".asd");
    }
  
  tc(YELLOW);printf("- ");tc(NORMAL);printf("Creating archive ");
  tc(GREEN);printf("[");tc(NORMAL);printf("%s",filename);tc(GREEN);
  printf("]\n");tc(NORMAL);


  if ( (file_exist(filename))==1 && always==0)
    {
      tc(RED);printf("File [%s] already exist. Owerwrite ? <Y/N>",filename);tc(NORMAL);
      do
	ch=_getch();
      while((ch!='A') && (ch!='a') && (ch!='Y') && (ch!='y') && (ch!='N') && (ch!='n'));
      if (toupper(ch)=='N')
	{
	  //free up things..
	  free(filename);
	  printf("\n");
	  exit(-1);
	}

      remove(filename);
      _gotoxy(0,_wherey());printf("                                                                              ");_gotoxy(0,_wherey());
    }
 
  tc(YELLOW);printf("- ");tc(NORMAL);printf("Collecting files");tc(YELLOW);
  printf("....\n");tc(NORMAL);

   if (argc==0)
    {
      // i=find(0,"*.*");
      i=find(0,&all_files);
    }
  else
    i=find(argc,&argv[0]);


  if (i<0)
    {
      tc(RED);
      if (i==-11)
	printf("\r\nError: No Files found \r\n");
      else
	printf("\r\nError: creating archive failed..\r\n");tc(NORMAL);
      exit(-1);
    }



  //if you want to add soft it do it here.. (will normally gain compression ratio a bit..)

 
  //tempfile
  out_tempfile=tmpfile();

  //copy all to one temp file and add crc value
  for (i=0; i<num_files; i++)
    {

      printf("%s",f_archive[i].filename);
      if (f_archive[i].filename_length>40) printf("\n");
      _gotoxy(42,_wherey());
      tc(GREEN);printf(" [");tc(NORMAL);printf("%d",i+1);
      tc(GREEN);printf(" / ");tc(NORMAL);printf("%d",num_files);
      tc(GREEN);printf("]\n");tc(NORMAL);
      if ((in_tempfile=fopen(f_archive[i].filename,"rb"))==NULL)
	{
	  tc(RED);
	  printf("\nError: could not open tempfile to add.\n");
	  tc(NORMAL);
	  exit(-1);
	}



      //check crc here too
      crc32_val=0xffffffff;
      while ( (c=_getc(in_tempfile)) !=EOF  )
	{
	  fputc(c,out_tempfile);  
	  crc32_val=crc32(c,crc32_val);
	}
      f_archive[i].filecrc=crc32_val^0xffffffff;
       
      fclose(in_tempfile);
    }



  //writing file (should be checked before so this should work)
  if ((outfile=fopen(filename,"wb"))==NULL)
    {
      tc(RED);
      printf("\nError: could not Write file (permission?) !\n");
      tc(NORMAL);
      exit(-1);
    }





  fputc('A',outfile);fputc('S',outfile);fputc('D',outfile);fputc('0',outfile);fputc('1',outfile);fputc(0x1A,outfile);

  fputc( (num_files & 0x00FF),outfile);
  fputc( (num_files >> 8),outfile);
  //  find(argc,&argv[1]);
  for (i=0; i<num_files; i++)
    {
      //name len
      fputc(f_archive[i].filename_length,outfile);
      //name
      for (j=0; j<f_archive[i].filename_length; j++)
	fputc(f_archive[i].filename[j],outfile);
      //filesize
      lj=f_archive[i].filesize;
      fputc( (lj & 0x000000FF),outfile);
      fputc(((lj & 0x0000FF00) >>8  ),outfile);
      fputc(((lj & 0x00FF0000) >>16  ),outfile);
      fputc(((lj & 0xFF000000) >>24  ),outfile);

       //filecrc
      lj=f_archive[i].filecrc;
      fputc( (lj & 0x000000FF),outfile);
      fputc(((lj & 0x0000FF00) >>8  ),outfile);
      fputc(((lj & 0x00FF0000) >>16  ),outfile);
      fputc(((lj & 0xFF000000) >>24  ),outfile);


      //filedate
      lj=f_archive[i].filetime;
      fputc( (lj & 0x000000FF),outfile);
      fputc(((lj & 0x0000FF00) >>8  ),outfile);
      fputc(((lj & 0x00FF0000) >>16  ),outfile);
      fputc(((lj & 0xFF000000) >>24  ),outfile);

      //fileattr
      lj=f_archive[i].fileattribute;
      if (n_attrib==1) lj=0;
      fputc((lj  & 0x00FF),outfile);
      fputc(((lj & 0xFF00) >>8  ),outfile);
    }

  //free mem
  for (i=0; i<num_files; i++)
    free(f_archive[i].filename);
  free(f_archive);

  tc(YELLOW);printf("- ");tc(NORMAL);
  printf("Total files to add: %ld\r\n",num_files);

  tc(YELLOW);printf("- ");tc(NORMAL);
  if (c_hash_deep<400 && c_hash_deep>=100)
    printf("Encoding level: Normal");
  if (c_hash_deep>=400)
    printf("Encoding level: Maximum");
  if (c_hash_deep<100)
    printf("Encoding level: Fast");

  //time to encode !
  fseek(out_tempfile,0,SEEK_SET);
  
  //encode
  encode(c_hash_deep,asd_extra,out_tempfile,outfile);
  
  fclose(outfile);
  fclose(out_tempfile);
  free(filename);
  return(0);  
}










int sfx(char *fame)
{
  int ch;
  unsigned long s_size;
  char *fname;

  fname = (char*) malloc((strlen(fame)+6)*sizeof(char));
  strcpy(fname,fame);


  //first check so filename.asd exists
  check_file(fname);
  ch=strlen(fname);

  if ((infile=fopen(fname,"rb"))==NULL)
    {
      tc(RED);
      printf("\nError: could not open infile (permission denied) !\n");
      tc(NORMAL);
      exit(-1);
    }





  if ((ch<5) || toupper(fname[ch-1 ])!='D' || toupper(fname[ch-2])!='S' ||    toupper(fname[ch-3])!='A' ||   fname[ch-4]!='.' )
#ifdef __UNIX__
  strcat(fname,".bin");
  else
    {
      fname[ch-1]='n';
      fname[ch-2]='i';
      fname[ch-3]='b';
      fname[ch-4]='.';
    }
#endif

#ifndef __UNIX__
  strcat(fname,".exe");
  else
    {
      fname[ch-1]='e';
      fname[ch-2]='x';
      fname[ch-3]='e';
      fname[ch-4]='.';
    }
#endif

  //then check if filename.exe exists
  if ( (file_exist(fname))==1 && always==0)
    {
      tc(RED);printf("File [%s] already exist. Owerwrite ? <Y/N>",fname);tc(NORMAL);
      do
	ch=_getch();
      while((ch!='A') && (ch!='a') && (ch!='Y') && (ch!='y') && (ch!='N') && (ch!='n'));
      if (toupper(ch)=='N')
	{
	  //free up things..
	  free(fname);
	  printf("\n");
	  exit(-1);
	}

      _gotoxy(0,_wherey());printf("                                                                              ");_gotoxy(0,_wherey());
    }
  
 

  if ((outfile=fopen(fname,"wb"))==NULL)
    {
      tc(RED);
      printf("\nError: could not write outfile (permission denied) !\n");
      tc(NORMAL);
      exit(-1);
    }
  

     tc(YELLOW);printf("- ");tc(NORMAL);printf("Converting to SFX file....");
     tc(GREEN);printf("[");tc(NORMAL);
     printf("%s",fname);
     tc(GREEN);printf("]\r\n");tc(NORMAL);
  
     //copy sfx header..
     for (s_size=0; s_size<sfx_data_size; s_size++)
       {
	 fputc(sfx_data[s_size],outfile);
       }
     
     //ok then copy file
     while ( (ch=_getc(infile))!=EOF)
       putc(ch,outfile);
     
     fclose(outfile);
#ifdef __UNIX__
     chmod(fname,777);
#endif
     tc(YELLOW);printf("- ");tc(NORMAL);printf("Done !\r\n");tc(NORMAL);
     free(fname);
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
  n_attrib=0;
  errors=0;
  num_files=0;
  switches=0;
  //default hash_deep
  c_hash_deep=300;

  //asd_extra default 20
  asd_extra=20;
  
  printf("\r\n");
  tc(NORMAL);
  printf("ASD - archiver version %s, Copyright (c) 1996-1997 Tobias Svensson\r\n",version);
  printf("Freeware archiver from Sweden,  *** with Long filename supported ***\n\r\n");

  //display help
  if ((argc==1) || (toupper(*argv[1])=='H') || ((argv[1][0])=='-') && (argv[1][1]=='?') )
    {
      tc(CYAN);
      printf("\rUsage:    ASD <option> [<switch(es)>] <arc name> <files.(wildcards allowed)>\r\n");
      printf("\rExamples: ASD a -m archive *.exe, ASD x -y arch.asd, ASD l arch.asd\r\n\n");
      tc(YELLOW);
      printf("\r<Options:>\r\n");
      tc(NORMAL);
      printf("\r  a  = add files to archive           l  = list files in archive\r\n");
      printf("\r  x  = extract files from archive     t  = test files in archive\r\n");
      printf("\r  s  = convert archive to SFX         h  = Help with some examples\r\n");
      tc(YELLOW);
      printf("\r<Switches:>\r\n");
      tc(NORMAL);
      printf("\r  -y = Assume YES on all queries      -f = Set fast compression mode\r\n");
      printf("\r  -m = Set maximum compression mode   -a = disable attributes\r\n");
      
      if ((argc>1) && ((toupper(argv[1][0])=='H') || ((argv[1][0])=='-') && (argv[1][1]=='?')))
	{
	  printf("\r\n                    ---Press any key to continue---\r\n");always=_getch();
	  tc(GREEN);printf("\r\n                    ---------- ASD Help ----------\r\n");tc(NORMAL);
	  printf("ASD archiver is a FREEWARE archiver from Sweden made by Tobias Svensson\r\n");
	  printf("With support for Windows long filenames\r\n");
	  printf("\r\n");
	  printf("This program Uses a 4k buffer and a window of 35 chars\n");
	  printf("The Hash table uses a normal maximum deep of 256 but can be changed\r\n");
	  printf("to the maximum 4096 to improve compression, the problem is just that the\r\n");
	  printf("speed gets slower if you do this.\r\n");
	  printf("(to use maximum compression enter -m as a switch.)\r\n");
	  printf("The old option to change window max has been removed in this version since\r\n");
	  printf("it did not do much to the compression ratio.\r\n");
	  printf("\n");
	  printf("New in 0.1.5\r\n");
	  printf("- Rewritten into C code.\r\n");
	  printf("- Uses Hash table to speed up compression. (about 20 times faster)\r\n");
	  printf("- Also fixed some bugs from 0.1.4\r\n");
	  printf("- And dome other things too, see ASD.DOC for more info.\r\n");
	  printf("- Released with full source codes.\r\n");
	  printf("\r\n");
	  printf("To download the latest version of ASD archiver just let yourself out\n");
	  printf("on the Internet world and navigate your navigator to:\n");
	  printf("http://hem1.passagen.se/svto\n");
	  printf("Probably also available at: ftp://ftp.elf.stuba.sk/pub/pc/pack\r\n");
	  printf("\r\n                    ---Press any key to continue---");always=_getch();
	  printf("\r\n                  Small Command Guide part 1/2\r\n");
	  printf("\n");
	  printf("Below follow some examples how to use ASR Archiver\r\n");
	  printf("To add the file [allp.txt] and [poll.dat] to an archive [ar.asd] enter:\r\n");
	  printf("ASD a ar allp.txt poll.dat\r\n");
	  printf("\r\n");
	  printf("To view that archive just enter:\r\n");
	  printf("ASD l ar\r\n");
	  printf("\r\n");
	  printf("To convert that archive to an SELFEXTRACTING file (SFX) just enter:\r\n");
	  printf("ASD s ar\n");
#ifdef __UNIX__
	  printf("and a new SFX file named ar.bin will be created !\n");
	  printf("\n");
	  printf("To extract the file ar.asd just enter:\n");
	  printf("ASD x ar\n");
	  printf("\n");
	  printf("To test above archive (check crc) just enter:\r\n");
	  printf("ASD t ar\r\n");
	  printf("\r\n");
	  printf("you also can test/list/extract SFX files ! ,example:\n");
	  printf("ASD t ar.bin\n");
	  printf("ASD l ar.bin\n");
	  printf("ASD x ar.bin\n");
#endif
#ifndef __UNIX__
	  printf("and a new SFX file named ar.exe will be created !\n");
	  printf("\n");
	  printf("To extract the file ar.asd just enter:\n");
	  printf("ASD x ar\n");
	  printf("\n");
	  printf("To test above archive (check crc) just enter:\r\n");
	  printf("ASD t ar\r\n");
	  printf("\r\n");
	  printf("you also can test/list/extract SFX files ! ,example:\n");
	  printf("ASD t ar.exe\n");
	  printf("ASD l ar.exe\n");
	  printf("ASD x ar.exe\n");
#endif


	  printf("\n");
	  printf("                    ---Press any key to continue---");always=_getch();
	  printf("\r\n                   Small Command Guide part 2/2\n");
	  printf("\n");
	  printf("If you want compress better compression ratio you should\r\n");
	  printf("add the extra parameter -m\r\n");
	  printf("see below example that will create archive ar.asd:\r\n");
	  printf("ASD a -m ar\n");
	  printf("\n");
	  printf("The option -y will assume YES on all questions like Overwrite and so on\n");
	  printf("Use that option with care\n");
	  printf("Example:\n");
	  printf("ASD x ar.asd\r\n");
	  printf("\r\n");
	  printf("Hope you will enjoy this archive //the arthor Tobias Svensson\n");
	  printf("\r\n");
	  printf("If you want no attributes during compress or uncompress just \r\n");
	  printf("add the switch -a and no attributes will be saved (if compress)\r\n");
	  printf("or changed on decompressed files (if uncompress)\r\n");
	  printf("\r\n\n");
	  printf("Do you have improvments or comments? ,send an email to tobiassvensson@home.se\n\n\r\n\n");
	  exit(EXIT_SUCCESS);
	}
      else
	exit(EXIT_SUCCESS);
    }

  //count switches
  for (m=1; m<argc; m++)
    {
      if ((argv[m][0]=='-') && (argv[m][1]!='\0'))
	{
	  switches++;
	  if (toupper(argv[m][1])=='Y') always=1;
	  if (toupper(argv[m][1])=='R') subdir=1;
	  if (toupper(argv[m][1])=='M') c_hash_deep=4095;
	  if (toupper(argv[m][1])=='F') c_hash_deep=20;
	  if (toupper(argv[m][1])=='A') n_attrib=1;
	}
    }
  
  //extract archive
  if ((toupper(argv[1][0])=='X') && (argc>2))
    {
      testa=0;
      extract(argv[(2+switches)]);
      exit(EXIT_SUCCESS);
    }
  
  //test archive
  if ((toupper(argv[1][0])=='T') && (argc>2))
    {
      testa=1;
      extract(argv[(2+switches)]);
      exit(EXIT_SUCCESS);
    }
  
  //list archive
  if ((toupper(argv[1][0])=='L') && (argc>2))
    {
      list(argv[2]);
      exit(EXIT_SUCCESS);
    }
  
  //create archive
  if ((toupper(argv[1][0])=='A') && (argc >2))
    {
      add(argv[(2+switches)],(argc-switches-3),&argv[(2+switches+1)]);
      exit(EXIT_SUCCESS);
    }
  
  
  //create SFX archive
  if ((toupper(argv[1][0])=='S') && (argc >2))
    {
      sfx(argv[(2+switches)]);
      exit(EXIT_SUCCESS);
    }

  return(EXIT_SUCCESS);
}
