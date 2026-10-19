#include "asd.h"

/* to be placed in main file */
//#define minhit      2

unsigned int buffer_bits,window_bits;
//unsigned long buffer_size;
int offset_huff_rescale_factor;
int offset_huff_rehuff_factor;
int literal_huff_rescale_factor;
int literal_huff_rehuff_factor;
unsigned long ct_code;
char *buffer;
unsigned int bpos;

int LITERAL_NUM_CODES;
unsigned long *literal_count;
int *literal_high_tree,*literal_low_tree;
int *literal_from_tree;
short *literal_counted_tree;
unsigned int literal_scale_counter=0;
unsigned int literal_huff_counter=0;
unsigned int literal_rehuff;
unsigned int literal_rescale;
int OFFSET_NUM_CODES;
unsigned long *offset_count;
int *offset_high_tree,*offset_low_tree;
int *offset_from_tree;
short *offset_counted_tree;
unsigned int offset_scale_counter=0;
unsigned int offset_huff_counter=0;
unsigned int offset_rehuff;
unsigned int offset_rescale;

/*          ...     */




int literal_get_lowerst(void);
void literal_build_tree(void);
void literal_rescale_counts(void);
int literal_get_literal(BITFILE *);
int offset_get_lowerst(void);
void offset_build_tree(void);
void offset_rescale_counts(void);
int offset_get_offset(BITFILE*);
int literal_get_literal(BITFILE *);
int offset_get_offset(BITFILE *);


void literal_rescale_counts()
{
  int i;
  for (i=0; i<LITERAL_NUM_CODES; i++)
    {
      literal_count[i]=literal_count[i]/2;
      if (literal_count[i]==0)
	{
	  literal_count[i]=1;
	}
    }
}

void offset_rescale_counts()
{
  int i;
  for (i=0; i<OFFSET_NUM_CODES; i++)
    {
      offset_count[i]=offset_count[i]/2;
      if (offset_count[i]==0)
	{
	  offset_count[i]=1;
	}
    }
}

int literal_get_lowerst()
{
  int i,lowerst;
  unsigned long lowerst_val=0xFFFFFFFF;
  for (i=0; i<(LITERAL_NUM_CODES*2); i++)
    {
      if (literal_counted_tree[i]==0)
	if ((literal_count[i]!=0) && (literal_count[i]<lowerst_val))
	  {
	    lowerst_val=literal_count[i];
	    lowerst=i;
	  }
    }
  return(lowerst);
}

int offset_get_lowerst()
{
  int i,lowerst;
  unsigned long lowerst_val=0xFFFFFFFF;
  for (i=0; i<(OFFSET_NUM_CODES*2); i++)
    {
      if (offset_counted_tree[i]==0)
	if ((offset_count[i]!=0) && (offset_count[i]<lowerst_val))
	  {
	    lowerst_val=offset_count[i];
	    lowerst=i;
	  }
    }
  return(lowerst);
}

void literal_build_tree()
{
  int i,j;
  unsigned long k;
  int counts=LITERAL_NUM_CODES;
  int root_node=counts;

  for (i=0; i<(LITERAL_NUM_CODES*2); i++)
    {
      literal_counted_tree[i]=0;
    }

  for (i=LITERAL_NUM_CODES; i<(LITERAL_NUM_CODES*2); i++)
    {
      literal_count[i]=0;
    }

  while (counts>1)
    {
      i=literal_get_lowerst();
      k=literal_count[i];
      literal_counted_tree[i]=1;
      j=literal_get_lowerst();
      literal_count[root_node]=literal_count[j]+k;
      literal_counted_tree[j]=1;
      literal_from_tree[i]=root_node;
      literal_from_tree[j]=root_node;
      literal_low_tree[root_node-LITERAL_NUM_CODES]=i;
      literal_high_tree[root_node-LITERAL_NUM_CODES]=j;
      root_node++;
      counts--;
    }  
}

void offset_build_tree()
{
  int i,j;
  unsigned long k;
  int counts=OFFSET_NUM_CODES;
  int root_node=counts;

  for (i=0; i<(OFFSET_NUM_CODES*2); i++)
    {
      offset_counted_tree[i]=0;
    }
  for (i=OFFSET_NUM_CODES; i<(OFFSET_NUM_CODES*2); i++)
    {
      offset_count[i]=0;
    }
  while (counts>1)
    {
      i=offset_get_lowerst();
      k=offset_count[i];
      offset_counted_tree[i]=1;
      j=offset_get_lowerst();
      offset_count[root_node]=offset_count[j]+k;
      offset_counted_tree[j]=1;
      offset_from_tree[i]=root_node;
      offset_from_tree[j]=root_node;
      offset_low_tree[root_node-OFFSET_NUM_CODES]=i;
      offset_high_tree[root_node-OFFSET_NUM_CODES]=j;
      root_node++;
      counts--;
    }  
}

int literal_get_literal(BITFILE *infile)
{
  int c;
  int current_node=(LITERAL_NUM_CODES*2)-2;

  do
    {
      c=InputBit(infile);
      if (c==B_EOF)
	{
	  return(B_EOF);
	}
      if (c==1) 
	{
	  current_node=literal_high_tree[current_node-LITERAL_NUM_CODES];
	}
      else
	{
	  current_node=literal_low_tree[current_node-LITERAL_NUM_CODES];
	}
      if (current_node<LITERAL_NUM_CODES)
	{
	  literal_count[current_node]++;
	  literal_huff_counter++;
	  literal_scale_counter++;
	  if (literal_huff_counter>literal_rehuff)
	    {
	      literal_build_tree();
	      literal_huff_counter=0;
	    }
	  if (literal_scale_counter>literal_rescale)
	    {
	      literal_scale_counter=0;
	      literal_rescale_counts();
	    }
	  return(current_node);
	}
    }
  while (c!=B_EOF);
  return (B_EOF);
}

int offset_get_offset(BITFILE *infile)
{
  int c;
  int current_node=(OFFSET_NUM_CODES*2)-2;

  do
    {
      c=InputBit(infile);
      
      if (c==B_EOF)
	{
	  return(B_EOF);
	}
      if (c==1) 
	{
	  current_node=offset_high_tree[current_node-OFFSET_NUM_CODES];
	}
      else
	{
	  current_node=offset_low_tree[current_node-OFFSET_NUM_CODES];
	}
      if (current_node<OFFSET_NUM_CODES)
	{
	  offset_count[current_node]++;
	  offset_huff_counter++;
	  offset_scale_counter++;
	  if (offset_huff_counter>offset_rehuff)
	    {
	      offset_build_tree();
	      offset_huff_counter=0;
	    }
	  if (offset_scale_counter>offset_rescale)
	    {
	      offset_scale_counter=0;
	      offset_rescale_counts();
	    }
	  return(current_node);
	}
    }
  while (c!=B_EOF);
  return (B_EOF);
}










