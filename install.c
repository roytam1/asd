// ------------------------------------------------------------------
//  ASD - Archiever SFX version 0.2.0  (c) 1996-1998 Tobias Svensson
//
//  sources released under GPL2
// ------------------------------------------------------------------

#include "asd.h"

//windows
#define sfx_data_size 36864

//unix
//#define sfx_data_size 11861

#define version  "0.2.0"


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
BITFILE *bit_infile;

unsigned long buffer_size=4096;
int minhit=3;

//might be  16+minhit+256 extra =max win_size
static char *buffer;//[buffer_size+minhit+15+256];
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

char asd_version[4] = "0.2\0";



int read_f_archive(void);
int check_file(char*);
long next_file(long);
//int extract(char*);
int file_exist(char*);




//don't forget to free mem too later..








int read_f_archive(void)
{
  int i,j,c;

 
  if ((unsigned)(num_files =  InputBits(bit_infile,8))==B_EOF) exit(-1);
  if ((unsigned)(num_files += InputBits(bit_infile,8)*0x100)==B_EOF) exit(-1);
  if (asd_version[2]=='2')
    {
      if ((unsigned)(num_files += InputBits(bit_infile,8)*0x10000)==B_EOF) exit(-1);     
    }
  
  // printf("\nnum_files=%d\n\n",num_files);


  //allocating memmory
  f_archive = (file_struct*) malloc ((sizeof(file_struct))* (num_files));

  for (i=0; i<num_files; i++)
    {
      c = InputBits(bit_infile,8);
      /* allocating memmory for name +1 for null too */
      f_archive[i].filename = (char*) malloc (sizeof(char)*(c+1));

      for (j=0; j<c; j++)
	{
	  f_archive[i].filename[j] = InputBits(bit_infile,8);
#ifdef __UNIX__
	  if (f_archive[i].filename[j]=='\\')
	    {
	      f_archive[i].filename[j]='/';
	    }
#endif
	}
      /* add important null char */
      f_archive[i].filename[j]='\0';

      f_archive[i].filesize=InputBits(bit_infile,8);
      f_archive[i].filesize+=InputBits(bit_infile,8)*0x100;
      f_archive[i].filesize+=InputBits(bit_infile,8)*0x10000;
      f_archive[i].filesize+=InputBits(bit_infile,8)*0x1000000;

      f_archive[i].filecrc=InputBits(bit_infile,8);
      f_archive[i].filecrc+=InputBits(bit_infile,8)*0x100;
      f_archive[i].filecrc+=InputBits(bit_infile,8)*0x10000;
      f_archive[i].filecrc+=InputBits(bit_infile,8)*0x1000000;;

      f_archive[i].filetime=InputBits(bit_infile,8);
      f_archive[i].filetime+=InputBits(bit_infile,8)*0x100;
      f_archive[i].filetime+=InputBits(bit_infile,8)*0x10000;
      f_archive[i].filetime+=InputBits(bit_infile,8)*0x1000000;

      f_archive[i].fileattribute=InputBits(bit_infile,8);
      f_archive[i].fileattribute+=InputBits(bit_infile,8)*0x100;

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
  temp_filename = (char*) malloc ((sizeof(char)*(strlen(filename)+10)) );
  temp_filename[0]='\0';
  strcpy(temp_filename,filename);

  /* room for the name + ".asd" + null (the name of the caller must not be changed) */
  asd_filename = (char*) malloc ((sizeof(char)*(strlen(filename)+6)) );
  strcpy(asd_filename,filename);
  
  if ((infile = fopen(filename,"rb"))==NULL) 
    {
      result=1;
      strcat(asd_filename,".asd");
      if ((infile = fopen(asd_filename,"rb"))==NULL) 
	{
 	  tc(RED);printf("File [%s] or [%s.asd] does not exist !\n",temp_filename,temp_filename);tc(NORMAL);
 	  free_mem(asd_filename);
 	  free_mem(temp_filename);
 	  exit(EXIT_FAILURE);
	}
    }
  free_mem(asd_filename);

  /* note: the chars must be read one by one, the order of the
   * evaluation of the operands is not the same in every compiler */
  c1=_getc(infile);
  c2=_getc(infile);
  c3=_getc(infile);

  if ( (c1!='A') | (c2!='S') | (c3!='D')  )
    {
      /* if file is exe file */
      if  (filelength(fileno(infile)) > sfx_data_size)
	{
	  fseek(infile,sfx_data_size,SEEK_SET);
	  c1=_getc(infile);
	  c2=_getc(infile);
	  c3=_getc(infile);
	  if ( (c1=='A') && (c2=='S') && (c3=='D')  )
	    {
	      asd_version[0] = _getc(infile);
	      asd_version[2] = _getc(infile);
	      c1=_getc(infile);
	      if ( (asd_version[0] != '0') | ((asd_version[2] != '1') & (asd_version[2] != '2')) | (c1!=26)  )
		{
		  tc(RED);printf("[%s] is NOT an ASD 0.1.x archive, please download latest ASD archiver and try again !\n",filename);tc(NORMAL);
		  fclose(infile);
		  free_mem(temp_filename);
		  exit(EXIT_FAILURE);
		}
	      free_mem(temp_filename);
	      return(result);
	    }
	}
      tc(RED);printf("[%s] is NOT an ASD archive file !\n",filename);tc(NORMAL);
      fclose(infile);
      free_mem(temp_filename);
      exit(EXIT_FAILURE);
    }

  asd_version[0] = _getc(infile);
  asd_version[2] = _getc(infile);
  c1=_getc(infile);
  if ( (asd_version[0] != '0') | ((asd_version[2] != '1') & (asd_version[2] != '2') ) | (c1!=26)  )
    {
      tc(RED);printf("[%s] is NOT an ASD 0.1.x or 0.2.x archive, please download latest ASD archiver and try again !\n ",filename);tc(NORMAL);
      fclose(infile);
      free_mem(temp_filename);
      exit(EXIT_FAILURE);
    }
  free_mem(temp_filename);
  return(result);
}






























long next_file(long n_file)
{
  int ch;  
  struct ftime ft;

  /* check crc32 only if n_files >0 (before any n_file =-1) */
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
  
  /* set filedate */
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

  /* not first time (no file then).. */
  if (n_file!=-1 && testa!=1)
    fclose(outfile);
  
  /* set file attribute (need to be done after the file is closed) */
  if ((testa!=1) && (n_file!=-1) && (n_attrib==0) && (f_archive[(n_file)].fileattribute!=0)) _dos_setfileattr(f_archive[(n_file)].filename,(f_archive[(n_file)].fileattribute));

  /* no more files in archive */
  if ((num_files-1)==n_file) return(-1);

  /* time to create the next file to be unarchived check for file exist to be added.. */
  printf("%-19s",f_archive[(n_file+1)].filename);

  if (strlen(f_archive[(n_file+1)].filename)>18) printf("%-20s","\n");

  if ((testa!=1)  || (t_testa==1))
    {
      /* if you entered N previously */
      testa=0;
      /* TODO check and warn if it exist before.. */
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
	      /* t_testa means (ask if test next file) */
	      t_testa=1;
	      testa=1;
	    }
	}
      
      /* set to normal as default */
      if (testa!=1) _dos_setfileattr(f_archive[(n_file+1)].filename,32);
      
      /* open next file */
      if (testa!=1) 
	{
	  /* autocreate dir here */
	  create_dir(f_archive[(n_file+1)].filename);

	  if ((outfile=fopen(f_archive[(n_file+1)].filename,"wb"))==NULL)
	    {
	      t_testa=1;
	      testa=1;
	      noperm=1;
	    }
	}
      
    }
  crc32_val=0xffffffff;

  filesize_count=0;
  return(n_file+1);
}












int extract(char *filename)
{
  unsigned int c,d,i,j,k,l,hit,pos;
  unsigned long mb_val;
  long f_num=-1; /* first -1 to be 0 at first file */
  unsigned long l1;

  filesize_count=0;
  bpos=0;

  check_file(filename);
  fclose(infile);
  
  //  check_file(filename); 
  //fclose(infile);

  if ((bit_infile = OpenInputBitFile(filename)) == NULL)
    {
      tc(RED);
      printf("\nError: could not open file to extract\n");
      tc(NORMAL);
      exit(-1);
    }
  //fseek !!

  FseekInputBitFile(bit_infile,sfx_data_size);

  /* ASD0X(asc26) */
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);

  tc(YELLOW);printf("- ");tc(NORMAL);
  if (testa==1)
    printf("\rTesting archive: ");
  else
    printf("Extracting archive: ");
  tc(GREEN);printf("[");tc(NORMAL);printf("%s",filename);tc(GREEN);printf("]\r\n");tc(NORMAL);     

  read_f_archive();


  f_num=next_file(f_num);
  
  /* can also be done with memset(buffer,' ',buffer_size); */

  /* This is for OLD 0.1 files */
  if ((asd_version[0]=='0') && (asd_version[2]=='1'))
    {
      /* buffer size always 4096 on 0.1.x */
      buffer_size=4096;
      
      /* malloc mem */
      buffer = (char*) malloc (sizeof(char) * (buffer_size+256+3+15) );
      asd_extra = InputBits(bit_infile,8);
     
      for (i=0; i<buffer_size; i++)
	{
	  buffer[i]='0';
	}
  
      while (1)
	{
	  if ((d = InputBits(bit_infile,8)) == B_EOF) break;
	  for (k=128; k>=1; k-=(k/2))
	    {
	      if (d>=k)
		{
		  d-=k;
		  /* code */
		  if ((i = InputBits(bit_infile,8)) == B_EOF) break;
		  if ((j = InputBits(bit_infile,8)) == B_EOF) break;
		  hit=(i>>4)+minhit;
		  if (hit==(15+(unsigned)minhit)) hit=hit+asd_extra;
		  pos=((i & 0x0f)*256) + j;
		  for (l=0; l<hit;  l++)
		    {
		      /* next file ? */
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
		  /* no code */
		  if ((c = InputBits(bit_infile,8)) == B_EOF) break;
		  
		  /* next file ? */
		  while (f_archive[f_num].filesize==filesize_count)
		    if ((f_num=next_file(f_num))==-1)
		      break;
		  
		  if (testa!=1) putc(c,outfile);
		  crc32_val=crc32(c,crc32_val);
		  buffer[bpos++]=c;bpos = bpos % buffer_size;
		  filesize_count++;
		}
	      /* break when 1 */
	      if (k==1) break;
	    }
	}
      
      /* until no more files in archive */
      if (f_num!=-1)
	while((f_num=next_file(f_num))!=-1) { }
      
      
      /* Freeing memmory.. */
      free_mem(f_archive);
      free_mem(buffer);
    }


  /* And now if we have a 0.2 archive */
  if (asd_version[0]=='0' && asd_version[2]=='2')
    {
      /* for 0.2 is minhit 2 not 3 ! */
      minhit=2;
      /* read header.. */
      window_bits = InputBits(bit_infile,4);
      buffer_bits = InputBits(bit_infile,5);
      offset_huff_rescale_factor = InputBits(bit_infile,7);
      offset_huff_rehuff_factor = InputBits(bit_infile,7);
      literal_huff_rescale_factor = InputBits(bit_infile,7);
      literal_huff_rehuff_factor = InputBits(bit_infile,7);
      
      buffer_size = (1 << buffer_bits);

      /* malloc time */
      buffer = (char*) malloc (sizeof(char)*(buffer_size+minhit+1024));
      LITERAL_NUM_CODES = (256+((window_bits-1)*4));
      literal_count = (unsigned long*) malloc (sizeof(unsigned long) * (LITERAL_NUM_CODES * 2));
      literal_high_tree = (int*) malloc (sizeof(int)*(LITERAL_NUM_CODES));
      literal_low_tree = (int*) malloc (sizeof(int)*(LITERAL_NUM_CODES));
      literal_from_tree = (int*) malloc (sizeof(int)*(LITERAL_NUM_CODES * 2));
      literal_counted_tree = (short*) malloc (sizeof(short)*(LITERAL_NUM_CODES * 2));
      OFFSET_NUM_CODES = ((buffer_bits*2));
      offset_count = (unsigned long*) malloc (sizeof(unsigned long) * (OFFSET_NUM_CODES * 2));
      offset_high_tree = (int*) malloc (sizeof(int)*(OFFSET_NUM_CODES));
      offset_low_tree = (int*) malloc (sizeof(int)*(OFFSET_NUM_CODES));
      offset_from_tree = (int*) malloc (sizeof(int)*(OFFSET_NUM_CODES * 2));
      offset_counted_tree = (short*) malloc (sizeof(short)*(OFFSET_NUM_CODES * 2));
      
      literal_rehuff = (LITERAL_NUM_CODES * literal_huff_rehuff_factor);
      literal_rescale = (LITERAL_NUM_CODES * literal_huff_rescale_factor);
      offset_rehuff = (OFFSET_NUM_CODES * offset_huff_rehuff_factor);
      offset_rescale = (OFFSET_NUM_CODES * offset_huff_rescale_factor);
        /* set data so no errors will appear in the future.. */
      for (l1=0; l1<(unsigned)(LITERAL_NUM_CODES); l1++)
	{
	  literal_high_tree[l1]=0;
	  literal_low_tree[l1]=0;
	  /* x2 */
	  literal_count[l1]=0;
	  literal_from_tree[l1]=0;
	  literal_counted_tree[l1]=0;
	  literal_count[l1*2]=0;
	  literal_from_tree[l1*2]=0;
	  literal_counted_tree[l1*2]=0;
	}
      for (l1=0; l1<((unsigned)OFFSET_NUM_CODES); l1++)
	{
	  offset_high_tree[l1]=0;
	  offset_low_tree[l1]=0;
	  /* x2 */
	  offset_count[l1]=0;
	  offset_from_tree[l1]=0;
	  offset_counted_tree[l1]=0;
	  offset_count[l1*2]=0;
	  offset_from_tree[l1*2]=0;
	  offset_counted_tree[l1*2]=0;
	}

      /* init huffman */
      for (i = 0; i < (unsigned) LITERAL_NUM_CODES; i++)
	{
	  literal_count[i]=1;
	}

      literal_build_tree();

      for (i = 0; i < (unsigned) OFFSET_NUM_CODES; i++)
	{
	  offset_count[i]=1;
	}

      offset_build_tree();

      for (i = 0; i < (buffer_size+minhit+1024); i++)
	{
	  buffer[i] = '0';
	}


      while (1)
	{
	  if ( (d = literal_get_literal(bit_infile)) == B_EOF) break;
	  if (d>255)
	    {	
	      /* code */
	      if (d>263)
		{
		  d=d-264;
		  if ((i=InputBits(bit_infile,((d>>2)+1) ))==B_EOF) break;
		  i=i + ( 1 << ( ( (d>>2)+1 ) +2) ) + ( (d&3) << ((d>>2)+1) ); 
		}
	      else
		{
		  i=d-=256;
		}
	      if ( (ct_code=offset_get_offset(bit_infile))==B_EOF) break;
	      
	      if (ct_code<4)
		{
		  if ((pos=InputBits(bit_infile, 2 ))==B_EOF) break;
		}
	      else
		{
		  if ((mb_val=InputBits(bit_infile, ((ct_code-2)/2) ))==B_EOF) break;
		  pos=(1<<(ct_code/2)) + ((ct_code%2)*(1<<((ct_code-2)/2) )) +mb_val;
		}
	      hit=i+minhit+1;
	      for (l=0; l<hit;  l++)
		{
		  /* next file ? */
		  while (f_archive[f_num].filesize==filesize_count)
		    if ((f_num=next_file(f_num))==-1)
		      break;

		  c=buffer[(bpos+buffer_size-pos-3) % buffer_size];
		  if (testa!=1) putc(c,outfile);
		  crc32_val=crc32(c,crc32_val);
		  buffer[bpos++]=c;bpos = bpos % buffer_size;
		  filesize_count++;
		}
	    }
	  else
	    {
	      /* no code */
	      while (f_archive[f_num].filesize==filesize_count)
		if ((f_num=next_file(f_num))==-1)
		  break;

	      if (testa!=1) putc(d,outfile);
	      crc32_val=crc32(d,crc32_val);
	      buffer[bpos++]=d;bpos = bpos % buffer_size;
	      filesize_count++;
	    }
	  /* until no more files in archive */
#if 0
	  if (f_num!=-1)
	    while((f_num=next_file(f_num))!=-1) { }
	  
	  
	  /* Freeing memmory.. */
	  free_mem(f_archive);
#endif
	}
      if (f_num!=-1)
	while((f_num=next_file(f_num))!=-1) { }

      free_mem(f_archive);
      free_mem(buffer);
      free_mem(literal_count);
      free_mem(literal_high_tree);
      free_mem(literal_low_tree);
      free_mem(literal_from_tree);
      free_mem(literal_counted_tree);
      free_mem(offset_count);
      free_mem(offset_high_tree);
      free_mem(offset_low_tree);
      free_mem(offset_from_tree);
      free_mem(offset_counted_tree);
    }

  



  if (errors==0)
    {
      tc(YELLOW);printf("- ");tc(NORMAL);printf("All files OK !\n");tc(NORMAL);
    }
  else
    {
      tc(RED);printf("%ld Errors found !\n",errors);tc(NORMAL);
    }

  CloseInputBitFile(bit_infile);
  return(0);
}


































int main(int argc, char *argv[])
{
  //below need to be initialised as 0
  noperm=0;
  always=0;
  testa=0;
  t_testa=0;
  subdir=0;
  errors=0;
  num_files=0;
  //default hash_deep
  //asd_extra default 20
  asd_extra=20;
  
  printf("\r\n");
  tc(NORMAL);
  printf("ASD - archiver %s SFX, Copyright (c) 1996-1998 Tobias Svensson  12 nov 1998\r\n",version);
  printf("Freeware archiver from Sweden.\n\r");
  printf("\r\n");
  tc(RED);
  printf("** This SFX file contains ASD Archiver version %s **\r\n",version);
  tc(WHITE);
  printf("- A Freeware archiver from Sweeden by Tobias Svensson.\r\n");
  printf("- Complete sourcecode available for both ASD and UNASD.\r\n");
  printf("- For the latest version of ASD (http://hem1.passagen.se/svto)\r\n");
  printf("- For more info email the arthor (tobiassvensson@home.se)\r\n");
  tc(NORMAL);
  printf("\r\nPress almost any key to start extraction, or ESC to quit ! \r\n");
  testa=0;
  testa=_getch();
  if (testa==27) exit(-1);
  
  //extract archive
  extract(argv[(0)]);
  
  return(EXIT_SUCCESS);
}
