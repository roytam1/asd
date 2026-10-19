#include "asd.h"




#define window_size 	16
//more than minhit to be encoded = 3 at least
#define minhit 	2
#define buffer_size	4096

#define hash_size       4096

static unsigned char window[window_size+minhit+256];
static unsigned char buffer[buffer_size];
int left_in_window;
static unsigned int wpos,bpos;

int match_length,match_position;
int i,j,k,l,m,c;
//FILE *infile,*outfile;

static unsigned char asd_extra;

//I'll do this static since it only uses small memmories with a 4k buffer..
struct
{
  unsigned int next;
  unsigned int prev;
} hash_item[buffer_size];


//gives values from 0-4095
#define HASH(char1,char2,char3) (((char1 ^(char2<<3)) ^ char3) & 0xFFF)

//w_total to make code smaller ;)
#define w_total  (window_size+minhit+asd_extra)

//sort above hashes here (points to first hash_item)
unsigned int hash_table[hash_size];

#define EMPTY    0xFFFF

//hash_item[x].prev  >4095 then a hash_tree object (5000 to be safe ;) )
#define HASH_PREV_OFFSET    5000

//how deep in the hash chain we should go
unsigned int HASH_DEEP;



FILE *t1,*t2;



unsigned long count;


//functions
void hashinit(void);
void findmatch_raw(void);
void findmatch_hash(void);
void add_to_buffer(void);
void encode(unsigned int,unsigned char, FILE*,FILE*);






void hashinit(void)
{
  for (i=0; i<buffer_size; i++)
    {
      buffer[i]='0';
      hash_item[i].next=EMPTY;
      hash_item[i].prev=EMPTY;
    }
  for (i=0; i<hash_size; i++)
    hash_table[i]=EMPTY;
}


void findmatch_raw(void)
{

  int n_hash;
  unsigned char check;

  //slow way..  
  for (i=0; i<buffer_size; i++)
    {
      if ((window[wpos])==(buffer[(bpos+i) % (buffer_size)]))
	{
	  j=0;check=1;
	  while (check==1)
	    {
	      j++;
	      k=(wpos+j) % (w_total);
	      l=(bpos+i+j) % (buffer_size);
	      if (window[k]!=buffer[l]) check=0;
	      if (j>=(w_total))check=0;
	      //to make it compatible with old asd 0.1
	      if ( (i+j) > 4095) check=0;
	    }
	  if (j>match_length)
	    {
	      match_length=j;
	      match_position=4095-i;
	    }
	}
    }
}


void findmatch_hash(void)
{
  unsigned int n_hash,h_deep_count;
  int temp_i;
  
  //hash way  
  h_deep_count=0;

  n_hash=hash_table[HASH(window[((wpos) % (w_total))],window[((wpos+1) %(w_total))],window[((wpos+2) % (w_total))])];
  
  if (n_hash==EMPTY)  return;
  
  if (n_hash!=EMPTY) 
    {
      while (n_hash!=EMPTY)
	{
	  h_deep_count++;
	  temp_i=0;
	  while(window[( (wpos+temp_i) % (w_total) )] == buffer[( (buffer_size+n_hash-2+temp_i) % (buffer_size) )])
	    {
	      temp_i++;
	      if (temp_i>=(w_total)) break;
	      //to make it compatible with old asd 0.1
	      if (( (unsigned int) temp_i-1)>=(buffer_size+bpos - (n_hash-1)) % (buffer_size)) break;
	    }
	  if (temp_i>match_length)
	    {
	      match_length=temp_i;
	      match_position=(buffer_size+bpos - (n_hash-1)) % (buffer_size);
	      
	      //if max match hit found return
	      if (match_length>=w_total) return;
	    }
	  //next
	  n_hash=hash_item[n_hash].next;
	  if (h_deep_count==HASH_DEEP) break;
	}
    }
}




void add_to_buffer(void)
{
  int hash_value;
 
  //add char in window to buffer (wpos has last value)
  buffer[bpos]=window[((wpos) % (w_total))];
    
  //first we give it a hash value
  hash_value=HASH(buffer[(buffer_size+bpos-2) % (buffer_size)],buffer[(buffer_size+bpos-1) % (buffer_size)],buffer[(bpos) % (buffer_size)]);
  
  //first remove old hash_item[bpos]
  if ((hash_item[bpos].prev>=HASH_PREV_OFFSET) &&  (hash_item[bpos].prev!=EMPTY))
    hash_table[(hash_item[bpos].prev-HASH_PREV_OFFSET)]=EMPTY;
  else
    {
      if (hash_item[bpos].prev!=EMPTY)
	hash_item[(hash_item[bpos].prev)].next=EMPTY;
    }
  
  //now insert hash in table
  if (hash_table[hash_value]!=EMPTY)
    {
      hash_item[(hash_table[hash_value])].prev=bpos;
      hash_item[bpos].next=hash_table[hash_value];
    }
  else
    {  
      hash_item[bpos].next=EMPTY;
    }
  
  hash_table[hash_value]=bpos;
  hash_item[bpos].prev=HASH_PREV_OFFSET+hash_value;
  
  //and last increase buffer value
  bpos++;bpos%=buffer_size;
}




void encode(unsigned int hash_level_deep, unsigned char asd_extra_value, FILE *infile, FILE *outfile)
{
  unsigned char code[17]; //holds chars to be written to file
  unsigned long code_count;
  unsigned char mask;
  unsigned long fsize,fstatus;
  double d1,d2,d3;
 
  //set HASH_DEEP
  HASH_DEEP=hash_level_deep;

  //init hash_table
  hashinit();

  //filesize
  fsize=filelength(fileno(infile));

  fstatus=0;
  printf("\n100%% left to compress.  ");
  count=0;
  asd_extra=asd_extra_value;

  //save asd_extra byte
  putc(asd_extra,outfile);
 
  wpos=0;
  bpos=0;
  left_in_window=0;

  //read chars to window
  for (i=0; ((i<(w_total)) && ((c=getc(infile))!=EOF));i++)
    {
      window[(wpos+left_in_window) % (w_total)]=c;
      left_in_window++;
    }

  code[0]=0;
  code_count=1;
  mask=128;


  //loop until we break ;)
  while (1)
    {
      match_length=0;
      match_position=0;

      //findmatch_raw();
      findmatch_hash();
   


      //if (match_length>16) match_length=16;
      if ((match_length<w_total) && (match_length>(window_size+minhit)))
	match_length=17;
      if (match_length==18) match_length=17;
      //l is now used when writing the code
      l=match_length;
      if (l==w_total)   l=window_size+minhit;
      
      
   
   
      if (match_length>minhit)
	{
	  count=count+match_length;
	
	  //hit
	  code[0]=code[0]+mask;
   	  code[code_count++] = (unsigned char) (((l-minhit-1)*16)+( (match_position&0x0F00) >>8 ));
	  code[code_count++] = (unsigned char) (match_position & 0x00ff);
	  

	  for (i=0;i<match_length;i++)
	    {
	      //add the last char in window to buffer then read new char to window
	      add_to_buffer();
	      if ((c=getc(infile))!=EOF)
		{
		  window[wpos]=c;wpos++;wpos%=(w_total);
		}
	      else
		{
		  wpos++;wpos%=(w_total);left_in_window--;
		}
	    }
	}
      else
	{
	  count++;

	  //no hit
	  code[code_count++]=window[wpos];
	  
	  //add the last char in window to buffer then read new char to window
	  add_to_buffer();
	  if ((c=getc(infile))!=EOF)
	    { 
	      window[wpos]=c;wpos++;wpos%=(w_total);
	    }
	  else
	    {
	      wpos++;wpos%=(w_total);left_in_window--;
	    }
	}
      mask=mask>>1;

      if (mask==0)
	{
	  for (i = 0; i < code_count; i++)
	    putc(code[i], outfile);
	  code[0]=0; code_count = 1; mask = 128;
	}

      fstatus++;
      //print status 
      if (fstatus>(20020))
	{
	  fstatus=0;
	  _gotoxy(0,_wherey());
	  if (fsize>100)
	    printf("%ld %% left to compress.     ",(100-(count/(fsize/100))) );
	  else
	    printf("100 %% left to compress.     ");
	}
      if (left_in_window<1) break;
    }

  //send last stuff in code[]
  if (code_count > 1) 
    {
      for (i = 0; i < code_count; i++) putc(code[i], outfile);
    }
  _gotoxy(0,_wherey());
  fflush(infile);
  fflush(outfile);

  tc(YELLOW);printf("- ");tc(NORMAL);
  printf("infile:%ld     outfile:%ld    ",filelength(fileno(infile)),filelength(fileno(outfile)));

  d1=(double) (filelength(fileno(infile)));
  d2=(double) (filelength(fileno(outfile)));
  d1=d1/10;d2=d2/10;

  if ((d1/100)!=0)
    d3=(d1-d2) / (d1/100);
  else
    d3=0;

  printf(" => Saved %.2f%%      ",d3);
  printf("\r\n");
}

