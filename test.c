/* gmtime example */
#include "asd.h"
#include <time.h>
#include <stdio.h>
/*
 struct stat {
                  dev_t         st_dev; 
                  ino_t         st_ino; 
                  mode_t        st_mode;  
                  nlink_t       st_nlink;   
                  uid_t         st_uid;   
                  gid_t         st_gid;    
                  dev_t         st_rdev;    
                  off_t         st_size;  
                  blksize_t     st_blksize;  
                  blkcnt_t      st_blocks;  
                  time_t        st_atime;   
                  time_t        st_mtime;  
                  time_t        st_ctime;    
              };
*/
int main ()
{

  struct stat statts;
  time_t result;
  const char *filen="fil\0";
  struct tm * timeinfo;
  char * weekday[] = { "Sunday", "Monday",
                       "Tuesday", "Wednesday",
                       "Thursday", "Friday", "Saturday"};


  int a,b,c;

  
  stat(filen,&statts);


  result=statts.st_mtime;

  /*
struct ftime {
  unsigned ft_tsec:5;	 0-29, double to get real seconds 
  unsigned ft_min:6;	 0-59 
  unsigned ft_hour:5;	 0-23 
  unsigned ft_day:5;	 1-31 
  unsigned ft_month:4;	 1-12 
  unsigned ft_year:7;	 since 1980 
}

int    tm_sec   Seconds [0,60]. 
int    tm_min   Minutes [0,59]. 
int    tm_hour  Hour [0,23]. 
int    tm_mday  Day of month [1,31]. 
int    tm_mon   Month of year [0,11]. 
int    tm_year  Years since 1900. 
int    tm_wday  Day of week [0,6] (Sunday =0). 
int    tm_yday  Day of year [0,365]. 
int    tm_isdst Daylight Savings flag. 



  */

  printf("st_mtime %ld \n",statts.st_mtime);
  printf("st_size %ld \n",statts.st_size);
  

  timeinfo = localtime ( &statts.st_mtime );

  printf("sec %ld  \n", timeinfo->tm_sec );
  printf("min %ld  \n", timeinfo->tm_min );
  printf("hour %ld  \n", timeinfo->tm_hour );
  printf("day %ld  \n", timeinfo->tm_wday );
  printf("month %ld  \n", timeinfo->tm_mon +1);
  printf("year %ld  \n", timeinfo->tm_year -80 );
 


  a=5;
  printf("%ld\n",a <3);





 printf ("That day is a %s.\n", weekday[timeinfo->tm_wday]);
  

    return 0;
}
