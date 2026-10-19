// -------------------------------------------------------------
//  ASD - Archiever version 0.2.0 (c) 1996-1998 Tobias Svensson
//
//  sources released under GPL2
// -------------------------------------------------------------

#include "asd.h"

/* for sfx files */
#ifdef __WINDOWS__
#include "sfx_windows.h"
#endif
#ifdef __UNIX__
#include "sfx_linux.h"
#endif
#ifdef __DOS__
#include "sfx_dos.h"
#endif

#define version  "0.2.0"


/*
header:            bytes:
ASD01(asc26)        (6)
-
files in archive    (2-3) 
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
BITFILE *bit_infile, *bit_outfile;
unsigned long buffer_size=4096;
unsigned int minhit = 3;


static char *buffer;
static unsigned int bpos;

/* for crc32_value */
unsigned long crc32_val;

/* extra asd (default 15) (BUT only used in old asd 0.1.x) */
unsigned int asd_extra;

long num_files=0;
char *all_files="*.*";

/* each filesize counter (resetted when new file appear in solid archive) */
unsigned long filesize_count;

/* 1= true 0=false to test archive */
char testa,t_testa;
char always;
char subdir;
/* to disable attributes */
char n_attrib;
int xpos,ypos;
char noperm;

/* to count errors during for example decompress and test */
unsigned int errors;
unsigned int fa_type;
unsigned int c_hash_deep;
char asd_version[4] = "0.0\0";

/* below will be default values for this */
unsigned long add_buffer_bits=20;
unsigned long add_window_bits=10;

unsigned int rescale_factor=18;
unsigned int rehuff_factor=4;

char is_exe=0;
char need_asd_ext=0;

int wildcmp(char *mwild, char *mstring) 
{
  char *cp, *mp;
  int i,j;
  char *wild;
  char *string;

  string = (char*) malloc (sizeof(char)*strlen(mstring)+4);
  wild   = (char*) malloc (sizeof(char)*strlen(mwild)+1);
  string[0]='\0';
  wild[0]='\0';
  memcpy(wild,mwild,strlen(mwild));
  memcpy(string,mstring,strlen(mstring));
  
  for (i=0; (unsigned)i<strlen(mwild); i++)
    {
      wild[i]=toupper(mwild[i]);
    }
  for (i=0; (unsigned)i<strlen(mstring); i++)
    {
      string[i]=toupper(mstring[i]);
    }
  wild[strlen(mwild)]='\0';
  string[strlen(mstring)]='\0';

  j=0;
  for (i=0; (unsigned)i<strlen(string); i++)
    {
      if (string[i]=='.') j=1;
    }
  if (j==0)
    {
      strcat(string,".\0");
    }

  while ((*string) && (*wild != '*')) {
    if ((*wild != *string) && (*wild != '?')) 
      {
	free_mem(string);
	free_mem(wild);
	return 0;
      }
    wild++;
    string++;
  }
  
  while (*string) 
    {
      if (*wild == '*') 
	{
	  if (!*++wild) 
	    {
	      free_mem(string);
	      free_mem(wild);
	      return 1;
	    }
	  
	  mp = wild;
	  cp = string+1;
	} 
      else if ((*wild == *string) || (*wild == '?')) 
	{
	  wild++;
	  string++;
	} 
      else 
	{
	  wild = mp;
	  string = cp++;
	}
    }
  
  while (*wild == '*') 
    {
      wild++;
    }
  free_mem(string);
  free_mem(wild);
  return !*wild;
}














int find_files(char * dir,char *type, int subdir)
{
  FILE *test_out_tempfile;
  struct ffblk ffblk;
  struct ffblk tmp_ffblk;
  int done;
  char *temp;
  char *temp1;

  temp = (char*) malloc (sizeof(char)*258);
  temp[0]='\0';
  if (strcmp(dir,""))
    {
      strcat(temp,dir);
      strcat(temp,"\\");
    }
  strcat(temp,"*.*\0");

  done = findfirst(temp,&ffblk,fa_type);
  free_mem(temp);

  while (!done)
    {
      if ( strcmp(ffblk.ff_name,".") && strcmp(ffblk.ff_name,"..") )
	{
	  temp1 = (char*) malloc (sizeof(char)*258);
	  temp = (char*) malloc (sizeof(char)*258);
	  temp1[0]='\0';
	  temp[0]='\0';

	  if (ffblk.ff_attrib & FA_DIREC)
	    {
	      if (subdir)
		{
		  if (strcmp(dir,""))
		    {
		      strcat(temp1,dir);
		      strcat(temp1,"\\");
		    }
		  strcat(temp1,ffblk.ff_name);
		  find_files(temp1,type,subdir);
		}
	    }	    
	  else
	    {
	      if (strcmp(dir,""))
		{
		  strcat(temp1,dir);
		  strcat(temp1,"\\");
		  strcat(temp,temp1);
		  strcat(temp,"*.*\0");
		}
	      else
		{
		  strcat(temp,"*.*\0");
		}
	      
	      strcat(temp1,ffblk.ff_name);
	      findfirst(temp,&tmp_ffblk,fa_type);
  
	      if (wildcmp(type,ffblk.ff_name)==1)
	      	{
		  if (temp1)
		    /* readable for me ? */
		    if ((test_out_tempfile=fopen(temp1,"r"))==NULL) 
		      {
			printf("Permission denied when trying to read [%s]\n",temp1);
		      }
		    else
		      {
			fclose(test_out_tempfile);
			f_archive = (file_struct*) realloc (f_archive,sizeof(file_struct)*(num_files+1));
			
			//f_archive[num_files].filename_length=(unsigned int) malloc (sizeof(unsigned int));
			f_archive[num_files].filename_length=strlen(temp1);
			f_archive[num_files].filename = (char*) malloc (sizeof(char)*(strlen(temp1)+1));
			strcpy(f_archive[num_files].filename,temp1);
			//f_archive[num_files].filesize= (long) malloc (sizeof(long));
			f_archive[num_files].filesize=ffblk.ff_fsize;
			/* note you need to set CRC value during compress.. */
			//f_archive[num_files].filetime=(long) malloc (sizeof(long));
			f_archive[num_files].filetime=ffblk.ff_ftime+ffblk.ff_fdate*65536;
			//f_archive[num_files].fileattribute=(int) malloc (sizeof(int));
			f_archive[num_files].fileattribute=ffblk.ff_attrib;
			if (n_attrib==1)
			  f_archive[num_files].fileattribute=0;
			num_files++;
		      }
		}
	    }
	  free_mem(temp1);
	  free_mem(temp);
	}
      done = findnext(&ffblk);
    }
  return 0;
}


















int read_f_archive(void)
{
  int i,j,c;
  
  if ((unsigned)(num_files =  InputBits(bit_infile,8))==B_EOF) exit(-1);
  if ((unsigned)(num_files += InputBits(bit_infile,8)*0x100)==B_EOF) exit(-1);
  if (asd_version[2]=='2')
    {
      if ((unsigned)(num_files += InputBits(bit_infile,8)*0x10000)==B_EOF) exit(-1);     
    }
  

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
  result=0;
  temp_filename = (char*) malloc ((sizeof(char)*(strlen(filename)+10)) );

  temp_filename[0]='\0';
  strcpy(temp_filename,filename);
  if ((infile = fopen(filename,"rb"))==NULL) 
    {
      result=1;
      filename = (char*) realloc(filename,( strlen(filename)+6) * sizeof(char));
      strcat(filename,".asd\0");
      if ((infile = fopen(filename,"rb"))==NULL) 
	{
	  tc(RED);printf("File [%s] or [%s.asd] does not exist !\n",temp_filename,temp_filename);tc(NORMAL);
	  fclose(infile);
	  free_mem(temp_filename);
	  exit(EXIT_FAILURE);
	}
      need_asd_ext=1;
    }
  if ( (_getc(infile)!='A') | (_getc(infile)!='S') | (_getc(infile)!='D')  )
    {
      /* if file is exe file */
      if  (filelength(fileno(infile)) > sfx_data_size)
	{
	  fseek(infile,sfx_data_size,SEEK_SET);
	  if ( (_getc(infile)=='A') && (_getc(infile)=='S') && (_getc(infile)=='D')  )
	    {
	      asd_version[0] = _getc(infile);
	      asd_version[2] = _getc(infile);
	      if ( (asd_version[0] != '0') | ((asd_version[2] != '1') & (asd_version[2] != '2')) | (_getc(infile)!=26)  )
		{
		  tc(RED);printf("[%s] is NOT an ASD 0.1.x archive, please download latest ASD archiver and try again !\n",filename);tc(NORMAL);
		  fclose(infile);
		  free_mem(temp_filename);
		  exit(EXIT_FAILURE);
		}
	      is_exe=1;
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
  if ( (asd_version[0] != '0') | ((asd_version[2] != '1') & (asd_version[2] != '2') ) | (_getc(infile)!=26)  )
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

  if (need_asd_ext==1)
    {
      filename = (char*) realloc(filename,(strlen(filename)+6) * sizeof(char));
      strcat(filename,".asd\0");
    }


  if ((bit_infile = OpenInputBitFile(filename)) == NULL)
    {
      tc(RED);
      printf("\nError: could not open file to extract\n");
      tc(NORMAL);
      exit(-1);
    }
  if (is_exe==1)
    {
      FseekInputBitFile(bit_infile,sfx_data_size);
    }
  
  /* ASD0X(asc26) */
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);
  InputBits(bit_infile,8);

  tc(YELLOW);printf("þ ");tc(NORMAL);
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
		  if (hit==(15+minhit)) hit=hit+asd_extra;
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
	  literal_high_tree[l1]=NULL;
	  literal_low_tree[l1]=NULL;
	  /* x2 */
	  literal_count[l1]=NULL;
	  literal_from_tree[l1]=NULL;
	  literal_counted_tree[l1]=NULL;
	  literal_count[l1*2]=NULL;
	  literal_from_tree[l1*2]=NULL;
	  literal_counted_tree[l1*2]=NULL;
	}
      for (l1=0; l1<((unsigned)OFFSET_NUM_CODES); l1++)
	{
	  offset_high_tree[l1]=NULL;
	  offset_low_tree[l1]=NULL;
	  /* x2 */
	  offset_count[l1]=NULL;
	  offset_from_tree[l1]=NULL;
	  offset_counted_tree[l1]=NULL;
	  offset_count[l1*2]=NULL;
	  offset_from_tree[l1*2]=NULL;
	  offset_counted_tree[l1*2]=NULL;
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
      tc(YELLOW);printf("þ ");tc(NORMAL);printf("All files OK !\n");tc(NORMAL);
    }
  else
    {
      tc(RED);printf("%ld Errors found !\n",errors);tc(NORMAL);
    }

  CloseInputBitFile(bit_infile);
  return(0);
}












int list(char *filename)
{
  unsigned int i,j,k,x,y,num_files;
  unsigned long size,total_size,ttime,ddate;
  unsigned long crc;
#ifdef __UNIX__
  int slashfix;
#endif

  char att[10]="- - - -  \0";
  total_size=0;

  /* check so it is a ASD archive */
  check_file(filename);

  tc(YELLOW);
  printf("þ ");tc(NORMAL);printf("Listing archive: ");tc(GREEN);printf("[");tc(NORMAL);
  printf("%s",filename);
  tc(GREEN);printf("]\r\n\r\n");tc(NORMAL);
  
  /* test this so it works ok.. (should return correct number of files..) */
  num_files=_getc(infile); 
  num_files+=_getc(infile)*0x100;
  if (asd_version[2]=='2')
    {
      num_files+=_getc(infile)*0x10000;
    }
  

  printf("Filename            filesize     Crc32     Date       Time       Attribute\n");
  
  tc(GREEN);printf("\r---------------------------------------------------------------------------\n");tc(NORMAL);
  for (i=0; i<num_files; i++)
    {
      printf("\r");
      x=_getc(infile);
      for (j=0; j<x; j++)
	{
#ifdef __UNIX__
	  slashfix=_getc(infile);
	  if (slashfix=='\\')
	    {
	      printf("/");
	    }
	  else
	    {
	      printf("%c",slashfix);
	    }
#endif
#ifndef __UNIX__
	  printf("%c",_getc(infile));
#endif
	}
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
      y=_getc(infile)+_getc(infile)*256;

#ifndef __UNIX__
      if (y & FA_ARCH)   att[0]='N'; else att[0]='-';  
      if (y & FA_RDONLY) att[2]='R'; else att[2]='-';  
      if (y & FA_HIDDEN) att[4]='H'; else att[4]='-';  
      if (y & FA_SYSTEM) att[6]='S'; else att[6]='-';  
      att[7]='\0';
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
      att[10]='\0';
      printf("%*s",12,att);
#endif

      printf("\r\n");
    }
  tc(GREEN);
  printf("---------------------------------------------------------------------------\r\n");
  tc(YELLOW);
  printf("þ ");tc(NORMAL);printf("Total file(s) in archive:%ld, Space saved %ld%\n",num_files,100-((filelength(fileno(infile))*100)/total_size) );tc(NORMAL);
  fclose(infile);
  return(0);
}
























int find(int num_args,char *args[])
{
  struct ffblk ffblk;
  int c,i,j,k,l,done;
  char *read_buff[1024];
  unsigned long ttime,ddate;
  char t_dir[512];
  char t_file[512];


#ifdef __UNIX__
  DIR *dirp;
  struct dirent *dp;
  struct stat statts;
  int fnresult;
  FILE *test_out_tempfile;

  struct tm *tminfo;
#endif
  



  t_dir[0]='\0';
  t_file[0]='\0';


  num_files=0;
  j=0;
  if (num_args==0) 
    {
      j=1;num_args=1;
    }

  /* allocating memmory */
  f_archive = (file_struct*) malloc ((sizeof(file_struct)));

#ifndef __UNIX__  
  for (i=0; i<num_args; i++)
    {
      /* 16 instead of 0 dirname included */
      if (j==1)
	{
	  find_files("","*.*",subdir);
	}
      else
	{
	  l=0;
	  for (k=0; (unsigned)k<strlen(args[i]); k++)
	  {
	    if (args[i][k]=='\\')
	      {
		l=k;
	      }
	  }

	  if (l!=0)
	    {
	      strncpy (t_dir,&args[i][0],l);
	      t_dir[l]='\0';
	      strncpy (t_file,&args[i][l+1],strlen(args[i])-l);
	      t_file[strlen(args[i])-l-1]='\0';
	      /* printf("start find files with %s   %s\n\n\n\n",t_dir,t_file); */
	      find_files(t_dir,t_file,subdir);
	    }
	  else
	    {
	      find_files("",args[i],subdir);
	    }
	}
    }
#endif



#ifdef __UNIX__
  /* need to add default +.+ too */
  if (subdir==0)
    {
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
		  
		  /* a directory ? */
		  if (S_ISDIR(statts.st_mode)) fnresult=-1;
		  
		  /* readable for me ? */
		  if ((test_out_tempfile=fopen(dp->d_name,"r"))==NULL) 
		    {
		      printf("Permission denied when trying to read [%s]\n",dp->d_name);
		      fnresult=-1;
		    }
		  else
		    fclose(test_out_tempfile);
		}
	      
	      /* not ..  and not . */
	      if (fnresult==0)
		{
		  /* add here.. */
		  f_archive = (file_struct*) realloc (f_archive,sizeof(file_struct)*(num_files+1));
		  f_archive[num_files].filename_length=(unsigned int) malloc (sizeof(unsigned int));
		  f_archive[num_files].filename_length=strlen(dp->d_name);
		  f_archive[num_files].filename = (char*) malloc (sizeof(char)*(strlen(dp->d_name)+1)); /* NULL too */
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
    }
  else
    {
      printf("subdir not implemented yet..\n\n\n");
    }

#endif
   
  /* no files to compress.. */
  if (num_files==0)
    {
      free_mem(f_archive);
      return(-11);
    }
  
  return 0;
}


















int add(char *filenam, int argc, char *argv[])
{
  int c,i,j,k,l;
  int ch;
  long lj;
  FILE *out_tempfile;
  FILE *in_tempfile;

  /* this 3 lines and filenam in function is only for stupid gcc ;( */
  /* and free filename at the end here too.. */

  char* filename;
  filename = (char*) malloc((strlen(filenam)+8)*sizeof(char));
  filename[0]='\0';
  strcpy(filename,filenam);

  /* first make sure that extension of the file is .asd */
  i=(strlen(filename))-1;

  if (i<4) {
    filename = (char*) realloc(filename,(i+6) * sizeof(char));
    strcat(filename,".asd");
  }

  i=strlen(filename)-1;
  if ( !((filename[i-3]=='.') &&  (toupper(filename[i-2])=='A') &&  (toupper(filename[i-1])=='S') &&  (toupper(filename[i])=='D')))  {
    filename=(char *) realloc(filename,(i+6)* sizeof(char));
    strcat(filename,".asd");    
  }
  
  tc(YELLOW);printf("þ ");tc(NORMAL);printf("Creating archive ");
  tc(GREEN);printf("[");tc(NORMAL);printf("%s",filename);tc(GREEN);
  printf("]\n");tc(NORMAL);



  filename[strlen(filename)]='\0';

  if ( (file_exist(filename))==1 && always==0)
    {
      tc(RED);printf("File [%s] already exist. Owerwrite ? <Y/N>",filename);tc(NORMAL);
      do
	ch=_getch();
      while((ch!='A') && (ch!='a') && (ch!='Y') && (ch!='y') && (ch!='N') && (ch!='n'));
      if (toupper(ch)=='N')
	{
	  /* free up things.. */
	  free_mem(filename);
	  printf("\n");
	  exit(-1);
	}

      remove(filename);
      _gotoxy(0,_wherey());printf("                                                                              ");_gotoxy(0,_wherey());
    }
 
  tc(YELLOW);printf("þ ");tc(NORMAL);printf("Collecting files");tc(YELLOW);
  printf("....\n");tc(NORMAL);

   if (argc==0)
    {
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
  


  /* if you want to add soft it do it here.. (will normally gain compression ratio a bit..) */

 
  out_tempfile=tmpfile();

  /* copy all to one temp file and add crc value */
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

      crc32_val=0xffffffff;
      while ( (c=_getc(in_tempfile)) !=EOF  )
	{
	  fputc(c,out_tempfile);  
	  /* OutputBits(out_tempfile,c,8); */
	  crc32_val=crc32(c,crc32_val);
	}


      //f_archive[i].filecrc=(long) malloc (sizeof(long));
      f_archive[i].filecrc=crc32_val^0xffffffff;
       
      fclose(in_tempfile);
    }

  /* writing file (should be checked before so this should work) */
  if ((bit_outfile=OpenOutputBitFile(filename))==NULL)
    {
      tc(RED);
      printf("\nError: could not Write file (permission?) !\n");
      tc(NORMAL);
      exit(-1);
    }

  OutputBits(bit_outfile,'A',8);
  OutputBits(bit_outfile,'S',8);
  OutputBits(bit_outfile,'D',8);
  OutputBits(bit_outfile,'0',8);
  OutputBits(bit_outfile,'2',8);
  OutputBits(bit_outfile, 0x1A,8);
  OutputBits(bit_outfile,(num_files & 0x0000FF),8 );
  OutputBits(bit_outfile,(num_files & 0x00FF00)>>8,8 );
  OutputBits(bit_outfile,(num_files & 0xFF0000)>>16,8 );

  for (i=0; i<num_files; i++)
    {
      /* here we add a check if we have a c: or x: we remove this */
      if (f_archive[i].filename[1]==':')
	{
	  OutputBits(bit_outfile,   (f_archive[i].filename_length-2)    ,8);
	  /* name */
	  for (j=2; j<(int)(f_archive[i].filename_length); j++)
	    {
	      OutputBits(bit_outfile,f_archive[i].filename[j] ,8);
	    }
	}
      else
	{
	  OutputBits(bit_outfile,f_archive[i].filename_length,8);
	  /* name */
	  for (j=0; j<(int)f_archive[i].filename_length; j++)
	    {
	      OutputBits(bit_outfile,f_archive[i].filename[j],8);
	    }
	}
      
      lj=f_archive[i].filesize;
      OutputBits(bit_outfile, (lj & 0x000000FF),8);
      OutputBits(bit_outfile,((lj & 0x0000FF00) >>8  ),8);
      OutputBits(bit_outfile,((lj & 0x00FF0000) >>16  ),8);
      OutputBits(bit_outfile,((lj & 0xFF000000) >>24  ),8);

      lj=f_archive[i].filecrc;
      OutputBits(bit_outfile, (lj & 0x000000FF),8);
      OutputBits(bit_outfile,((lj & 0x0000FF00) >>8  ),8);
      OutputBits(bit_outfile,((lj & 0x00FF0000) >>16  ),8);
      OutputBits(bit_outfile,((lj & 0xFF000000) >>24  ),8);

      lj=f_archive[i].filetime;
      OutputBits(bit_outfile, (lj & 0x000000FF),8);
      OutputBits(bit_outfile,((lj & 0x0000FF00) >>8  ),8);
      OutputBits(bit_outfile,((lj & 0x00FF0000) >>16  ),8);
      OutputBits(bit_outfile,((lj & 0xFF000000) >>24  ),8);

      lj=f_archive[i].fileattribute;
      if (n_attrib==1) lj=0;
      
      OutputBits(bit_outfile, (lj & 0x00FF),8);
      OutputBits(bit_outfile,((lj & 0xFF00) >>8  ),8);
    }

  /* free mem */
  for (i=0; i<num_files; i++)
    free_mem(f_archive[i].filename);
  free_mem(f_archive);

  tc(YELLOW);printf("þ ");tc(NORMAL);
  printf("Total files to add: %ld\r\n",num_files);

  tc(YELLOW);printf("þ ");tc(NORMAL);

  if (c_hash_deep==CMP_ZERO)
    printf("Encoding level: Zero");
  else if (c_hash_deep==CMP_FASTEST)
    printf("Encoding level: Fastest");
  else if (c_hash_deep==CMP_FAST)
    printf("Encoding level: Fast");
  else if (c_hash_deep==CMP_NORMAL)
    printf("Encoding level: Normal");
  else if (c_hash_deep==CMP_GOOD)
    printf("Encoding level: Good");
  else if (c_hash_deep==CMP_MAX)
    printf("Encoding level: Maximum");
  else if (c_hash_deep==CMP_ULTRA)
    printf("Encoding level: Ultra Max");

  /* time to encode ! */
  fseek(out_tempfile,0,SEEK_SET);
  encode(c_hash_deep,add_window_bits,add_buffer_bits,rescale_factor,rehuff_factor,rescale_factor,rehuff_factor,out_tempfile,bit_outfile);
  CloseOutputBitFile(bit_outfile);
  fclose(out_tempfile);
  free_mem(filename);
  return(0);  
}










int sfx(char *fame)
{
  int ch;
  unsigned long s_size;
  char *fname;

  fname = (char*) malloc(strlen(fame)*sizeof(char)+5);
  fname[0]='\0';
  strcpy(fname,fame);


  /* first check so filename.asd exists */
  check_file(fname);
  fseek(infile,0,SEEK_SET);
  ch=strlen(fname);


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

  /* then check if filename.exe exists */
  if ( (file_exist(fname))==1 && always==0)
    {
      tc(RED);printf("File [%s] already exist. Owerwrite ? <Y/N>",fname);tc(NORMAL);
      do
	ch=_getch();
      while((ch!='A') && (ch!='a') && (ch!='Y') && (ch!='y') && (ch!='N') && (ch!='n'));
      if (toupper(ch)=='N')
	{
	  /* free up things.. */
	  free_mem(fname);
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
  

     tc(YELLOW);printf("þ ");tc(NORMAL);printf("Converting to SFX file....");
     tc(GREEN);printf("[");tc(NORMAL);
     printf(fname);
     tc(GREEN);printf("]\r\n");tc(NORMAL);
  
     /* copy sfx header.. */
     for (s_size=0; s_size<sfx_data_size; s_size++)
       {
	 fputc(sfx_data[s_size],outfile);
       }
     
     /* ok then copy file */
     while ( (ch=_getc(infile))!=EOF)
       putc(ch,outfile);
     
     fclose(outfile);
#ifdef __UNIX__
     chmod(fname,777);
#endif
     tc(YELLOW);printf("þ ");tc(NORMAL);printf("Done !\r\n");tc(NORMAL);
     free_mem(fname);
     return(0);
}





















int main(int argc, char *argv[])
{
  int switches;
  int m;

  /* below need to be initialised as 0 */
  noperm=0;
  always=0;
  testa=0;
  t_testa=0;
  subdir=0;
  n_attrib=0;
  errors=0;
  num_files=0;
  switches=0;
  c_hash_deep=CMP_NORMAL; /* normal for asd 2.0 */
  fa_type=FA_RDONLY+FA_HIDDEN+FA_SYSTEM+FA_ARCH+FA_DIREC;

  /* asd_extra default 20 (only used in old 0.1.x) */
  asd_extra=20;
  
  printf("\r\n");
  tc(NORMAL);
  printf("ASD - archiver %s, Copyright (c) 1996-1998 Tobias Svensson  12 Nov 1998\r\n",version);
  printf("Freeware archiver from Sweden,    Type \"ASD -?\" for more help \n\r\n");

  /* display help */
  if ((argc==1) || (toupper(*argv[1])=='H') || ((argv[1][0])=='-') && (argv[1][1]=='?') )
    {
      tc(CYAN);
      printf("\rUsage:    ASD <option> [<switch(es)>] <arc name> <files.(wildcards allowed)>\r\n");
      printf("\rExamples: ASD a -r archive *.exe, ASD x -y arch.asd, ASD l arch.asd\r\n\n");
      tc(YELLOW);
      printf("\r<Options:>\r\n");
      tc(NORMAL);
      printf("\r  a  = add files to archive           l  = list files in archive\r\n");
      printf("\r  x  = extract files from archive     t  = test files in archive\r\n");
      printf("\r  s  = convert archive to SFX         h  = Help with some examples\r\n");
      tc(YELLOW);
      printf("\r\n");
      printf("\r<Switches:>\r\n");
      tc(NORMAL);
      printf("\r  -y        =  Assume YES on all queries     \r\n");
     printf("\r  -a        =  disable attributes\r\n");
      printf("\r  -r        =  include subdirectories\r\n");
      printf("\r  -m[1..5]  =  Set compression level (1=Fastest.. 3=Normal.. 5=Maximum)\r\n");
      printf("\r  -md[a..h] =  Set dictionary size in kb (64,128,256,512,1024,2048,4096,8192)\r\n");
      printf("\r               Note: asd requires (dictionary size * 12) free memmory)\r\n");

      if ((argc>1) && ((toupper(argv[1][0])=='H') || ((argv[1][0])=='-') && (argv[1][1]=='?')))
	{
	  printf("\r\n");
	  tc(YELLOW);
	  printf("\r<Advanced Switches:>\r\n");
	  tc(NORMAL);
	  printf("\r  -rs[0..9] =  Set Huffman Rescale factor (0=Rescale Often)\r\n");
	  printf("\r  -rh[0..9] =  Set Huffman rebuild factor (0=Rebuild often)\r\n");
   


	  printf("\r\n                    ---Press any key to continue---\r\n");always=_getch();
	  tc(GREEN);printf("\r\n                    ---------- ASD Help ----------\r\n");tc(NORMAL);
	  printf("ASD archiver is a FREEWARE archiver from Sweden by Tobias Svensson\r\n");
	  printf("This program Uses a variable 64k-8192k buffer and a window of 1k\n");
	  printf("You can change the buffer size with the \"-mda\" to \"-mdh\" switch, the last one\r\n");
	  printf("should normally result in best compression. (Default is the -mde option)\r\n");
	  printf("BUT NOTE that the -mdg and -mdh options will require much memory and might not\r\n");
	  printf("work on some old computers with low ram.\r\n");
	  printf("\r\n");
	  printf("The Hash table deep can be changed by switch -m1 to -m5\r\n");
	  printf("The maximum -m5 improves compression the problem is just that the\r\n");
	  printf("speed gets slower if you do this.\r\n");
	  printf("(Actually there is a -m6 option too (ULTRA TIGHT), but will run very slow) \r\n");
	  printf("\r\n");
	  printf("You can also change the Rescale and Rehuff factor for the huffman data, but\r\n");
	  printf("this is only for very advanced users. Since the files differ you will probably\r\n");
	  printf("need to test all combinations of -rs[0..9] combined with -rh[0..9] to get the\r\n");
	  printf("ultimate smallerst asd file.\r\n");
	  printf("Normally high rescae value and low rehuff value will get best result\r\n");
	  printf("\r\n");
	  printf("For normal users the \"-m5\" and the \"-mdg\" or \"-mdh\" is the one to use\r\n");
	  printf("for maximum compression, but note that thoose will require much free memmory\r\n");
	  printf("which is about (buffer_size * 12)\r\n");
	  printf("\r\n                    ---Press any key to continue---\r\n");always=_getch();

	  printf("\r\n");
	  printf("                     -- News in ASD Archiver %s --\r\n",version);
	  printf("\r\n");
	  printf("þ Totally rewritten compression ratio that is MUCH better.\r\n");
	  printf("þ Added ability to include subdirectories.\r\n");
	  printf("þ And a lot of other switches added.\r\n");
	  printf("þ Released with full source codes.\r\n");
	  printf("\r\n");
	  printf("This version should still be able to extract old 0.1.x archives\r\n");
	  printf("But the best thing to do is to rearchive them with this version\r\n");
	  printf("(will result in much better compression)\r\n");
	  printf("\r\n");
	  printf("I have tested ASD archiver against famous current archiver like:LHA ZIP RAR ARJ\r\n");
	  printf("JAR UC2 SQZ, and ASD archived beated them all in compression in almost all\r\n");
	  printf("cases, in some one really much beaten\r\n");
	  printf("To get a more detailed list of this you can check different Compression tests\r\n");
	  printf("on internet to get better info.\r\n"); 
	  printf("\r\n");
	  printf("To download the latest version of ASD archiver just let yourself out\n");
	  printf("on the Internet world and navigate your navigator to:\n");
	  printf("http://hem1.passagen.se/svto\n");
	  printf("Probably also available at:\r\n");
	  printf("ftp://ftp.elf.stuba.sk/pub/pc/pack\r\n");

	  printf("\r\n                    ---Press any key to continue---\r\n");always=_getch();
	  printf("\r\n                  Small Command Guide part 1/2\r\n");
	  printf("\n");
	  printf("Below follow some examples how to use ASD Archiver\r\n");
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
	  printf("                    ---Press any key to continue---\r\n");always=_getch();
	  printf("\r\n                   Small Command Guide part 2/2\n");
	  printf("\n");
	  printf("If you want compress better compression ratio you should\r\n");
	  printf("add the extra parameter -m5 and a -mdh option\r\n");
	  printf("see below example that will create archive ar.asd:\r\n");
	  printf("ASD a -mdh -h5 ar\n");
	  printf("(the -mdh option will require a lot of ram so leave this option if you have a \r\ncomputer with low ram or try -mde or -mdg instead)\r\n");
	  printf("\r\n");
	  printf("The option -y will assume YES on all questions like Overwrite and so on\n");
	  printf("Use that option with care\n");
	  printf("Example:\n");
	  printf("ASD x -y ar.asd\r\n");
	  printf("\r\n");
	  printf("Hope you will enjoy this archive //the arthor Tobias Svensson\n");
	  printf("\r\n");
	  printf("If you want no attributes during compress or uncompress just \r\n");
	  printf("add the switch -a and no attributes will be saved (if compress)\r\n");
	  printf("or changed on decompressed files (if uncompress)\r\n");
	  printf("\r\n");
	  printf("Do you have improvments or comments? ,send an email to tobiassvensson@home.se\r\n\n\n");
	  exit(EXIT_SUCCESS);
	}
      else
	exit(EXIT_SUCCESS);
    }

  /* count switches */
  for (m=1; m<argc; m++)
    {
      if (toupper(argv[m][0]=='-'))
	{
	  switches++;
	  if (toupper(argv[m][1])=='Y') always=1;
	  if (toupper(argv[m][1])=='R') subdir=1;
	  /* compress level (hash deep) */
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='0'))
	    {
	      rehuff_factor=50;
	      rescale_factor=50;
	      c_hash_deep=CMP_ZERO;
	    }
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='1'))
	    {
	      c_hash_deep=CMP_FASTEST;
	      rehuff_factor=16;
	    }
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='2')) 
	    {
	      c_hash_deep=CMP_FAST;
	      rehuff_factor=8;
	    }
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='3')) c_hash_deep=CMP_NORMAL;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='4')) 
	    {
	      c_hash_deep=CMP_GOOD;
	      rehuff_factor=2;
	    }
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='5'))
	    {
	      c_hash_deep=CMP_MAX;
	      rehuff_factor=1;
	    }
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='6')) 
	    {
	      c_hash_deep=CMP_ULTRA;
	      rehuff_factor=1;
	    }
	  
	  /* buffer size */
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='A')) add_buffer_bits=16;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='B')) add_buffer_bits=17;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='C')) add_buffer_bits=18;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='D')) add_buffer_bits=19;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='E')) add_buffer_bits=20;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='F')) add_buffer_bits=21;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='G')) add_buffer_bits=22;
	  if ((toupper(argv[m][1])=='M') && (toupper(argv[m][2])=='D') && (toupper(argv[m][3])=='H')) add_buffer_bits=23;

	  /* rescale factor */
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='0')) rescale_factor=1;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='1')) rescale_factor=2;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='2')) rescale_factor=4;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='3')) rescale_factor=6;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='4')) rescale_factor=9;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='5')) rescale_factor=12;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='6')) rescale_factor=17;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='7')) rescale_factor=21;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='8')) rescale_factor=29;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='S') && (toupper(argv[m][3])=='9')) rescale_factor=40;

	  /* rehuff factor */
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='0')) rehuff_factor=1;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='1')) rehuff_factor=2;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='2')) rehuff_factor=4;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='3')) rehuff_factor=6;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='4')) rehuff_factor=9;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='5')) rehuff_factor=12;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='6')) rehuff_factor=17;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='7')) rehuff_factor=21;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='8')) rehuff_factor=29;
	  if ((toupper(argv[m][1])=='R') && (toupper(argv[m][2])=='H') && (toupper(argv[m][3])=='9')) rehuff_factor=40;
	  if (toupper(argv[m][1])=='A') n_attrib=1;
	}
    }
  
  /* extract archive */
  if ((toupper(argv[1][0])=='X') && (argc>2))
    {
      testa=0;
      extract(argv[(2+switches)]);
      exit(EXIT_SUCCESS);
    }
  
  /* test archive */
  if ((toupper(argv[1][0])=='T') && (argc>2))
    {
      testa=1;
      extract(argv[(2+switches)]);
      exit(EXIT_SUCCESS);
    }
  
  /* list archive */
  if ((toupper(argv[1][0])=='L') && (argc>2))
    {
      list(argv[2]);
      exit(EXIT_SUCCESS);
    }
  
  /* create archive */
  if ((toupper(argv[1][0])=='A') && (argc >2))
    {
      add(argv[(2+switches)],(argc-switches-3),&argv[(2+switches+1)]);
      exit(EXIT_SUCCESS);
    }
  
  /* create SFX archive */
  if ((toupper(argv[1][0])=='S') && (argc >2))
    {
      sfx(argv[(2+switches)]);
      exit(EXIT_SUCCESS);
    }

  return(EXIT_SUCCESS);
}
