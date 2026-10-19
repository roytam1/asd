#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *infile,*outfile;



int main()
{
  long f_size;
  int c,linebreak;
  char s[34];
  linebreak=0;

  infile=fopen("sfx.dat","rb");
  outfile=fopen("sfx.h","wb");
  
  f_size=filelength(fileno(infile));
  itoa(f_size,s,10);
  
  fputs("#define sfx_data_size ",outfile);
  fputs(s,outfile);
  fputs("\n\n",outfile);


  fputs("static unsigned char sfx_data[]= {\n",outfile);
  


  while(1)
    if ((c=getc(infile))!=EOF)
      {	
	itoa(c,s,10);
	//printf("%s",s);
	
	//not 1:st time
	if (linebreak!=0) fputs(",",outfile);
	
	if (linebreak==20)
	  {
	    linebreak=1;
	    fputs("\n",outfile);
	  }
	itoa(c,s,10);

	linebreak++;
	fputs(s,outfile);
	
	
      }
    else
      {
	fputs("};\n",outfile);
	fclose(infile);fclose(outfile);
	exit(-1);
      }
  
}
