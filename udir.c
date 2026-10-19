#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#include <stdio.h>
#include <string.h>

int compar(const FTSENT **f1, const FTSENT **f2)
{
return strcmp( (*f1)->fts_name, (*f2)->fts_name);
}

int is_dot_txt(FTSENT *f)
{
return f->fts_info == FTS_F
&& f->fts_namelen > 4
&& strcmp(f->fts_name + f->fts_namelen - 4, ".txt") == 0;
}

int main(void)
{
FTS *fts;
FTSENT *ftsent;
char * const path[] = { ".", 0};

fts = fts_open(path, FTS_PHYSICAL, compar);

while ( (ftsent = fts_read(fts)) != NULL) {
if ( is_dot_txt(ftsent) ) {
printf("%s\n", ftsent->fts_path);
}
}
fts_close(fts);

return 0;
}
