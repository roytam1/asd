//#include <time.h>
#include "asd.h"


#ifdef __MAIN__
#include <time.h>
#endif

unsigned int e_minhit;
unsigned int window_size;
unsigned long hash_size;
unsigned long bmask;
unsigned long e_buffer_size;
unsigned char *e_buffer;
unsigned int mask1,mask2;
long left_in_window;
unsigned long e_bpos;
unsigned long match_length;
unsigned long match_position;
unsigned long i,j,k,l,c;

struct my_hash_item
{
  unsigned long next;
  unsigned long prev;
};
struct my_hash_item *hash_item;

/* for code table */
unsigned long cw_bits,cp_bits;
unsigned long cw_value,cp_value;
unsigned long cw_code,cp_code;

unsigned long w_total;
unsigned long *hash_table;
unsigned long EMPTY;
unsigned long HASH_PREV_OFFSET;

unsigned long HASH_DEEP;



/* adaptive huffman variables */
int E_LITERAL_NUM_CODES;
unsigned long *e_literal_count;
int *e_literal_high_tree;
int *e_literal_low_tree;
short *e_literal_counted_tree;
int *e_literal_from_tree;
int e_literal_huff_counter=0;
unsigned long e_literal_scale_counter=0;
int e_literal_rehuff;
unsigned int e_literal_rescale;
int E_OFFSET_NUM_CODES;
unsigned long *e_offset_count;
int *e_offset_high_tree;
int *e_offset_low_tree;
short *e_offset_counted_tree;
int *e_offset_from_tree;
int e_offset_huff_counter=0;
unsigned long e_offset_scale_counter=0;
int e_offset_rehuff;
unsigned int e_offset_rescale;






/* functions */
void hashinit(void);
void findmatch_hash(int);
void add_to_buffer(void);
void encode(unsigned long, unsigned int, unsigned int, unsigned int,unsigned int, unsigned int, unsigned int, FILE*, BITFILE*);
void calc_pos(unsigned long);
void calc_win(unsigned long);
unsigned long HASH(unsigned char,unsigned char,unsigned char);
int e_literal_get_lowerst(void);
int e_literal_huff_code(BITFILE*,int);
void e_literal_build_tree(void);
void e_literal_rescale_counts(void);
int e_offset_get_lowerst(void);
int e_offset_huff_code(BITFILE*,int);
void e_offset_build_tree(void);
void e_offset_rescale_counts(void);



unsigned long HASH(unsigned char char1,unsigned char char2,unsigned char char3)
{
  return (((char1 ^ (char2<<mask1)) ^ (char3<<mask2)) & (e_buffer_size-1));
}



void e_literal_rescale_counts()
{
  int i;
  for (i=0; i<E_LITERAL_NUM_CODES; i++)
    {
      e_literal_count[i]=e_literal_count[i]/2;
      if (e_literal_count[i]==0)
	{
	  e_literal_count[i]=1;
	}
    }
}

void e_offset_rescale_counts()
{
  int i;
  for (i=0; i<E_OFFSET_NUM_CODES; i++)
    {
      e_offset_count[i]=e_offset_count[i]/2;
      if (e_offset_count[i]==0)
	{
	  e_offset_count[i]=1;
	}
    }
}


int e_literal_get_lowerst()
{
  int i,lowerst;
  unsigned long lowerst_val=0xFFFFFFFF;
  for (i=0; i<(E_LITERAL_NUM_CODES*2); i++)
    {
      if ((e_literal_counted_tree[i]==0) &&  (e_literal_count[i]!=0) && (e_literal_count[i]<lowerst_val))
	{
	  lowerst_val=e_literal_count[i];
	  lowerst=i;
	}
    }
  return(lowerst);
}

int e_offset_get_lowerst()
{
  int i,lowerst;
  unsigned long lowerst_val=0xFFFFFFFF;
  for (i=0; i<(E_OFFSET_NUM_CODES*2); i++)
    {
      if ((e_offset_counted_tree[i]==0) &&  (e_offset_count[i]!=0) && (e_offset_count[i]<lowerst_val))
	{
	  lowerst_val=e_offset_count[i];
	  lowerst=i;
	}
    }
  return(lowerst);
}

void e_literal_build_tree()
{
  int i,j;
  int counts=E_LITERAL_NUM_CODES;
  unsigned long k;
  int root_node = E_LITERAL_NUM_CODES;

  for (i=0; i<(E_LITERAL_NUM_CODES*2); i++)
    {
      e_literal_counted_tree[i]=0;
    }
  for (i=E_LITERAL_NUM_CODES; i<(E_LITERAL_NUM_CODES*2); i++)
    {
      e_literal_count[i]=0;
    }
  while (counts>1)
    {
      i=e_literal_get_lowerst();
      k=e_literal_count[i];
      e_literal_counted_tree[i]=1;
      j=e_literal_get_lowerst();
      e_literal_count[root_node]=e_literal_count[j]+k;
      e_literal_counted_tree[j]=1;
      e_literal_from_tree[i]=root_node;
      e_literal_from_tree[j]=root_node;
      e_literal_low_tree[root_node-E_LITERAL_NUM_CODES]=i;
      e_literal_high_tree[root_node-E_LITERAL_NUM_CODES]=j;
      root_node++;
      counts--;
    }  
}

void e_offset_build_tree()
{
  int i,j;
  int counts=E_OFFSET_NUM_CODES;
  unsigned long k;
  int root_node = E_OFFSET_NUM_CODES;

  for (i=0; i<(E_OFFSET_NUM_CODES*2); i++)
    {
      e_offset_counted_tree[i]=0;
    }
  for (i=E_OFFSET_NUM_CODES; i<(E_OFFSET_NUM_CODES*2); i++)
    {
      e_offset_count[i]=0;
    }
  while (counts>1)
    {
      i=e_offset_get_lowerst();
      k=e_offset_count[i];
      e_offset_counted_tree[i]=1;
      j=e_offset_get_lowerst();
      e_offset_count[root_node]=e_offset_count[j]+k;
      e_offset_counted_tree[j]=1;
      e_offset_from_tree[i]=root_node;
      e_offset_from_tree[j]=root_node;
      e_offset_low_tree[root_node-E_OFFSET_NUM_CODES]=i;
      e_offset_high_tree[root_node-E_OFFSET_NUM_CODES]=j;
      root_node++;
      counts--;
    }  
}

int e_literal_huff_code(BITFILE *outfile,int val)
{
  int i,j,k;
  unsigned long sum=0;
  j=val;
  k=0;
  while (  (i=e_literal_from_tree[j])>=E_LITERAL_NUM_CODES)
    {
      if (e_literal_high_tree[i-E_LITERAL_NUM_CODES]==j) 
	{
	  sum+=(1 << k);
	}
      j=i;
      k++;
    }
  OutputBits(outfile,sum,k);
  return(sum);
}

int e_offset_huff_code(BITFILE *outfile,int val)
{
  int i,j,k;
  unsigned long sum=0;
  j=val;
  k=0;
  while (  (i=e_offset_from_tree[j])>=E_OFFSET_NUM_CODES)
    {
      if (e_offset_high_tree[i-E_OFFSET_NUM_CODES]==j) 
	{
	  sum+=(1 << k);
	}
      j=i;
      k++;
    }
  OutputBits(outfile,sum,k);
  return(sum);
}




void calc_win(unsigned long offset)
{
  unsigned long cw_i,cw_j;

  if (offset<8)
    {
      cw_bits=0;
      cw_value=offset;
      cw_code=offset+256;
    }
  else
    {
      cw_i=offset;
      cw_j=-2;
      while (cw_i>1)
	{
	  cw_i=cw_i >> 1;
	  cw_j++;
	}
     cw_bits=cw_j;
     cw_value=offset & ((1<<cw_j)-1);
     cw_code=(cw_j*4)+((offset>>cw_j)&3)+4+256;
    }

}


void calc_pos(unsigned long offset)
{
  unsigned long orig_offset=offset;
  
  if ((offset>=0) && (offset<=3))
    {
      cp_bits=2;
      cp_value=offset;
      cp_code=offset;
    }
  else if ((offset>=4) && (offset<=e_buffer_size))
    {
      cp_bits=1;
      while (offset>1)
	{
	  cp_code=offset;
	  offset=offset >> 1;
	  cp_bits++;
	}
      cp_bits-=2;
      cp_code-=2;
      cp_code=(cp_bits*2)+2+cp_code;

      cp_value=((1<<(cp_code/2)) + ((cp_code%2)*(1<<((cp_code-2)/2))));
      cp_value=orig_offset-cp_value;
    }
}


void hashinit(void)
{
  for (i=0; i<e_buffer_size; i++)
    {
      e_buffer[i]='0';
      hash_item[i].next=EMPTY;
      hash_item[i].prev=EMPTY;
    }
  for (i=0; i<hash_size; i++)
    hash_table[i]=EMPTY;
}



void findmatch_hash(int extra)
{
  unsigned long n_hash,h_deep_count;
  unsigned long temp_i;
  unsigned long trew;
  unsigned long zpos;
  unsigned long mp;

  zpos=(e_bpos+extra) & bmask;

  h_deep_count=0;
  trew=HASH(e_buffer[((zpos+1) & (bmask))],
	    e_buffer[((zpos+2) & (bmask))],
	    e_buffer[((zpos+3) & (bmask))]);

  n_hash=hash_table[trew];
  
  if (n_hash==EMPTY)  return;
  
  if (n_hash!=EMPTY) 
    {
      while (n_hash!=EMPTY)
	{
	  h_deep_count++;
	  temp_i=0;
	  
	  mp=(e_buffer_size+zpos - (n_hash)) % (e_buffer_size);
	  
	  while(e_buffer[( (zpos+1+temp_i) & (bmask) )] == e_buffer[( (n_hash+temp_i+e_buffer_size-2) & (bmask) )])
	    {
	      if ((mp+temp_i)>=(e_buffer_size-left_in_window)) break;
	      if (temp_i>=(w_total)) break;	      
     	      temp_i++;
	      //if ((e_buffer_size+temp_i+zpos - (n_hash)) % (e_buffer_size)>= (e_buffer_size-left_in_window)) break;
	    }
	  
	  if (temp_i>match_length)
	    {
	      match_length=temp_i;
	      match_position=mp;//(e_buffer_size+zpos - (n_hash)) % (e_buffer_size);
	      if (match_length>=w_total) return;
	    }
	  n_hash=hash_item[n_hash].next;
	  if (h_deep_count==HASH_DEEP) break;
	}
    }
}


void add_to_buffer(void)
{
  unsigned long hash_value;

  e_bpos=(e_bpos+1) & bmask;
  
  hash_value=HASH(e_buffer[(e_buffer_size+e_bpos-2) & (bmask)],e_buffer[(e_buffer_size+e_bpos-1) & (bmask)],e_buffer[(e_bpos+e_buffer_size) & (bmask)]);

  /* first remove old hash_item[e_bpos] */
  if ((hash_item[e_bpos].prev>=HASH_PREV_OFFSET) &&  (hash_item[e_bpos].prev!=EMPTY))
    {
      hash_table[(hash_item[e_bpos].prev-HASH_PREV_OFFSET)]=EMPTY;
    }
  else
    {
      if (hash_item[e_bpos].prev!=EMPTY)
	hash_item[(hash_item[e_bpos].prev)].next=EMPTY;
    }
  /* now insert hash in table */
  if (hash_table[hash_value]!=EMPTY)
    {
      hash_item[(hash_table[hash_value])].prev=e_bpos;
      hash_item[e_bpos].next=hash_table[hash_value];
    }
  else
    {  
      hash_item[e_bpos].next=EMPTY;
    }
  
  hash_table[hash_value]=e_bpos;
  hash_item[e_bpos].prev=HASH_PREV_OFFSET+hash_value;
}














void encode(unsigned long hash_level_deep,
	    unsigned int e_window_bits,
	    unsigned int e_buffer_bits,
	    unsigned int e_offset_huff_rescale_factor,
	    unsigned int e_offset_huff_rehuff_factor,
	    unsigned int e_literal_huff_rescale_factor,
	    unsigned int e_literal_huff_rehuff_factor,
	    FILE *infile, 
	    BITFILE *outfile)
{
  long l1,l2;
  double d1,d2,d3;
  unsigned int extra;
  int cc;
  unsigned long t_len;
  unsigned long e_fstatus=0;
  unsigned long e_count=0;
  unsigned long e_fsize;
 
  if (hash_level_deep<1) hash_level_deep=1;

  mask1=(e_buffer_bits/2)-4;
  mask2=(e_buffer_bits-8);

  e_minhit = 2;

  e_literal_huff_counter=0;
  e_literal_scale_counter=0;
  e_offset_huff_counter=0;
  e_offset_scale_counter=0;

  window_size =(1 << e_window_bits);
  e_buffer_size = (1 << e_buffer_bits);

  E_LITERAL_NUM_CODES = (256+((e_window_bits-1)*4));
  E_OFFSET_NUM_CODES = ((e_buffer_bits*2));
  hash_size = e_buffer_size;
  bmask = e_buffer_size-1;
  w_total = window_size+e_minhit;
  EMPTY = e_buffer_size+1;
  HASH_PREV_OFFSET = hash_size;

  e_literal_rehuff =(E_LITERAL_NUM_CODES * e_literal_huff_rehuff_factor);
  e_literal_rescale= (E_LITERAL_NUM_CODES * e_literal_huff_rescale_factor);
  e_offset_rehuff= (E_OFFSET_NUM_CODES * e_offset_huff_rehuff_factor);
  e_offset_rescale= (E_OFFSET_NUM_CODES * e_offset_huff_rescale_factor);

  e_buffer = (unsigned char*) malloc (sizeof(unsigned char)*(e_buffer_size+256+window_size));
  hash_table = (unsigned long*) malloc (sizeof(unsigned long)*(e_buffer_size+1));
  hash_item = (struct my_hash_item*) malloc (sizeof(struct my_hash_item)*(e_buffer_size+1));
  e_literal_count = (unsigned long*) malloc (sizeof(unsigned long)*E_LITERAL_NUM_CODES*2);
  e_offset_count = (unsigned long*) malloc (sizeof(unsigned long)*E_OFFSET_NUM_CODES*2);
  e_literal_high_tree = (int*) malloc (sizeof(int)*E_LITERAL_NUM_CODES);
  e_literal_low_tree = (int*) malloc (sizeof(int)*E_LITERAL_NUM_CODES);
  e_offset_high_tree = (int*) malloc (sizeof(int)*E_OFFSET_NUM_CODES);
  e_offset_low_tree = (int*) malloc (sizeof(int)*E_OFFSET_NUM_CODES);
  e_offset_counted_tree = (short*) malloc (sizeof(short)*E_OFFSET_NUM_CODES*2);
  e_offset_from_tree = (int*) malloc (sizeof(int)*E_OFFSET_NUM_CODES*2);
  e_literal_counted_tree = (short*) malloc (sizeof(short)*E_LITERAL_NUM_CODES*2);
  e_literal_from_tree = (int*) malloc (sizeof(int)*E_LITERAL_NUM_CODES*2);
  
  e_fsize = e_fsize=filelength(fileno(infile));


  /* init ahuff */
  for (l1=0; l1<E_LITERAL_NUM_CODES; l1++)
    {
      e_literal_count[l1]=1;
      e_literal_high_tree[l1]=0;
      e_literal_low_tree[l1]=0;
    }
  for (l1=0; l1<E_OFFSET_NUM_CODES; l1++)
    {
      e_offset_count[l1]=1;
      e_offset_high_tree[l1]=0;
      e_offset_low_tree[l1]=0;
    }
  for (l1 = 0; l1 < (E_OFFSET_NUM_CODES*2); l1++)
    {
      e_offset_from_tree[l1] = 0;
    }
  for (l1 = 0; l1<(E_LITERAL_NUM_CODES*2); l1++)
    {
      e_literal_from_tree[l1]=0;
    }

  printf("\n100%% left to compress. ");


  /* write header.. */
  OutputBits(outfile,e_window_bits,4);
  OutputBits(outfile,e_buffer_bits,5);
  OutputBits(outfile,e_offset_huff_rescale_factor,7);
  OutputBits(outfile,e_offset_huff_rehuff_factor,7);
  OutputBits(outfile,e_literal_huff_rescale_factor,7);
  OutputBits(outfile,e_literal_huff_rehuff_factor,7);

  
  /*   root_node=NUM_CODES;   */

   e_literal_build_tree();
   e_offset_build_tree();


  HASH_DEEP=hash_level_deep;

  hashinit();

  e_bpos=0;
  left_in_window=0;

  /* read chars to window */
  for (i=0; ((i<(w_total)) && ((cc=getc(infile))!=EOF));i++)
    {
      e_buffer[(++left_in_window) % (e_buffer_size)]=cc;
    }

  while (1)
    {
      match_length=0;
      match_position=0;
      
      findmatch_hash(1);
      t_len=match_length;
      
      match_length=0;
      match_position=0;
      findmatch_hash(0);
      if (t_len>match_length)
	{
	  match_length=0;
	  match_position=0;
	}



      /* we can only handle up to minhit+window_size */
      if (match_length>(e_minhit+window_size)) 
	match_length=e_minhit+window_size;

      /* l is now used when writing the code */
      l=match_length;
      if (l==w_total)   l=window_size+e_minhit;

      calc_pos(match_position);

      extra=0;

      if (cp_bits>6)
	{
	  extra++;
	}
      if (cp_bits>(6+8))
	{
	  extra++;
	}
      if (cp_bits>(6+16))
	{
	  extra++;
	}
      if (cp_bits>(6+24))
	{
	  extra++;
	}
      if (cp_bits>(6+32))
	{
	  extra++;
	}


      if (match_length>(e_minhit+extra))
	{
	  e_count=e_count+match_length;
	  calc_win(l-e_minhit-1);
	  e_literal_huff_code(outfile,cw_code);
	  e_literal_count[cw_code]++;

	  e_literal_scale_counter++;
	  e_literal_huff_counter++;
	  if (e_literal_huff_counter>e_literal_rehuff)
	    {
	      e_literal_huff_counter=0;
	      e_literal_build_tree();
	    }
	  if (e_literal_scale_counter>e_literal_rescale)
	    {
	      e_literal_scale_counter=0;
	      e_literal_rescale_counts();
	    }

	  if (cw_bits>0)
	    {
	      OutputBits(outfile,cw_value,cw_bits); 
	    }

	  e_offset_huff_code(outfile, cp_code);
	  e_offset_count[cp_code]++;

	  e_offset_scale_counter++;
	  e_offset_huff_counter++;
	  if (e_offset_huff_counter>e_offset_rehuff)
	    {
	      e_offset_huff_counter=0;
	      e_offset_build_tree();
	    }
	  if (e_offset_scale_counter>e_offset_rescale)
	    {
	      e_offset_scale_counter=0;
	      e_offset_rescale_counts();
	    }
	  OutputBits(outfile, cp_value, cp_bits);
	  for (i=0;i<match_length;i++)
	    {
	      add_to_buffer();
	      if ((cc=getc(infile))!=EOF)
		{
		  e_buffer[ (e_bpos+left_in_window) & (bmask)  ]=cc;
		}
	      else
		{
		  left_in_window--;
		}
	      
	      e_fstatus++;
	      if (e_fstatus>(20020))
		{
		  e_fstatus=0;
		  _gotoxy(0,_wherey());
		  if (e_fsize>100)
		    printf("%ld %% left to compress.                      ",(100-(e_count/(e_fsize/100))) );
		  else
		    printf("100 %% left to compress.                      ");
		}
	      if (left_in_window<1) break;
	    }
	}

      else
	{  
	  e_count++;
	  e_literal_huff_code(outfile, e_buffer[ (e_bpos+1) & (bmask)]);
	  e_literal_count[(e_buffer[ (e_bpos+1) & (bmask)])]++;
	  e_literal_huff_counter++;
	  e_literal_scale_counter++;
	  if (e_literal_huff_counter>e_literal_rehuff)
	    {
	      e_literal_huff_counter=0;
	      e_literal_build_tree();
	    }
	  if (e_literal_scale_counter>e_literal_rescale)
	    {
	      e_literal_scale_counter=0;
	      e_literal_rescale_counts();
	    }
	  add_to_buffer();
	  if ((cc=getc(infile))!=EOF)
	    { 
	      e_buffer[(e_bpos+left_in_window) & (bmask)]=cc;
	    }
	  else
	    {
	      left_in_window--;
	    }
	}
       if (left_in_window<1) break;
  }

  free_mem(e_buffer);
  free_mem(hash_table);
  free_mem(hash_item);
  free_mem(e_literal_count);
  free_mem(e_offset_count);
  free_mem(e_literal_low_tree);
  free_mem(e_literal_high_tree);
  free_mem(e_offset_low_tree);
  free_mem(e_offset_high_tree);
  free_mem(e_offset_from_tree);
  free_mem(e_literal_from_tree);
  free_mem(e_literal_counted_tree);
  free_mem(e_offset_counted_tree);
      
  _gotoxy(0,_wherey());
  fflush(infile);
  //fflush((FILE*)outfile);

  tc(YELLOW);printf("- ");tc(NORMAL);
  printf("infile:%ld     outfile:%ld    ",filelength(fileno(infile)),GetOutputBitFilePos(outfile)+1);

  d1=(filelength(fileno(infile)));

  /* if zero we just add 1 to solve null pointer later.. */
  if (d1<1) 
    {
      d1++;
    }
  d2=GetOutputBitFilePos(outfile)+1;
  d3=((d1-d2)/d1)*100;

  printf(" => Saved %.2f%%      ",d3);
  printf("\r\n");
}




#ifdef __MAIN__
int main(int argc, char* argv[])
{  
  FILE *infile;
  BITFILE *outfile;
  clock_t cstart, cfinish;
  double  duration;

  
  if (argc==3)
    {
      infile=fopen(argv[1],"rb");
      outfile=OpenOutputBitFile(argv[2]);
      cstart = clock();
      encode(4096,9,22,12,12,12,12,infile, outfile);
      CloseOutputBitFile(outfile);
      fclose(infile);
      cfinish = clock();
      duration = (double)(cfinish - cstart) / CLOCKS_PER_SEC;
      printf("Encode time: %2.2f seconds\n", duration );
    }
  else
    printf("encode util encode <infile> <outfile>\n");

  return(0);
}
#endif
