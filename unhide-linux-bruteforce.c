/*
          http://sourceforge.net/projects/unhide/
*/

/*
Copyright © 2010-2024 Yago Jesus & Patrick Gouin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Needed for unistd.h to declare getpgid() and others
#define _XOPEN_SOURCE 500

// Needed for sched.h to declare sched_getaffinity()
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>
#include <sys/resource.h>
#include <errno.h>
#include <dirent.h>
#include <sched.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <ctype.h>
#include <time.h>

#include "unhide-output.h"
#include "unhide-linux.h"

/*
 *  Minimalist thread function for brute test.
 *  Set tid with the pid of the created thread. 
 */
void *functionThread (__attribute__ ((unused)) void *parametro) 
{

   tid = (pid_t) syscall (SYS_gettid);
   return(&tid) ;
};

/*
 *  Brute force the pid space via vfork and 
 *  pthread_create/pthread_join. All pid which
 *  can't be obtained are check against ps output
 */
int* allpids ;
int* allpids2 ;

void brute(void) 
{
   volatile int i = 0;
//   volatile int* allpids = NULL ;
//   volatile int* allpids2 = NULL ;
   int x;
   int y;
   int z;

   msgln(unlog, 0, "[*]Starting scanning using brute force against PIDS with fork()\n") ;

   if ( NULL == (allpids = (int *)malloc(sizeof(int) * maxpid)))
   {
      die(unlog, "Error: Cannot allocate pid arrays ! Exiting.");
   }

   if(FALSE == brutesimplecheck)   // allocate second table
   {
      if ( NULL == (allpids2 = (int *)malloc(sizeof(int) * maxpid)))
      {
         die(unlog, "Error: Cannot allocate pid arrays ! Exiting.");
      }
   }



   if(FALSE == brutesimplecheck)   // Init the two tables
   {
      // PID under 301 are reserved for kernel
      for(x=0; x < 301; x++) 
      {
         allpids[x] = 0 ;
         allpids2[x] = 0 ;
      }
   
      for(z=301; z < maxpid; z++) 
      {
         allpids[z] = z ;
         allpids2[z] = z ;
      }
   }
   else   // Init only the first table
   {
      for(x=0; x < 301; x++) 
      {
         allpids[x] = 0 ;
      }
   
      for(z=301; z < maxpid; z++) 
      {
         allpids[z] = z ;
      }
   }

   // printf("Maxpid : %06d\n", maxpid);
   for (i=301; i < maxpid; i++) 
   {
      int vpid;
      int status;

      // printf("Tested pid : %06d\r", i);
      errno= 0 ;

      if ( ( vpid =  vfork() ) == 0) 
      {
         _exit(0);
      }

      if (0 == errno) 
      {
         allpids[vpid] =  0;
         waitpid(vpid, &status, 0);
      }
   }

   if(FALSE == brutesimplecheck)   // Do the scan a second time
   {
//    printf("DOING double check ...\n") ;
      for (i=301; i < maxpid; i++) 
      {
         int vpid;
         int status;
         errno= 0 ;

         if ((vpid = vfork()) == 0) 
         {
            _exit(0);
         }

         if (0 == errno) 
         {
            allpids2[vpid] =  0;
            waitpid(vpid, &status, 0);
         }
      }
   }
   /* processes that quit at this point in time create false positives */

   for(y=0; y < maxpid; y++) 
   {
      if ((allpids[y] != 0) && ((TRUE == brutesimplecheck) || (allpids2[y] != 0))) 
      {
//       printf("Check PID : %d\n", y);
         if(!checkps(allpids[y],PS_PROC | PS_THREAD | PS_MORE) ) 
         {
            printbadpid(allpids[y]);
         }
      }
   }

   msgln(unlog, 0, "[*]Starting scanning using brute force against PIDS with pthread functions\n") ;

   if(FALSE == brutesimplecheck)   // Init the two tables
   {
      // PID under 301 are reserved for kernel
      for(x=0; x < 301; x++) 
      {
         allpids[x] = 0 ;
         allpids2[x] = 0 ;
      }
   
      for(z=301; z < maxpid; z++) 
      {
         allpids[z] = z ;
         allpids2[z] = z ;
      }
   }
   else   // Init only the first table
   {
      for(x=0; x < 301; x++) 
      {
         allpids[x] = 0 ;
      }
   
      for(z=301; z < maxpid; z++) 
      {
         allpids[z] = z ;
      }
   }



   for (i=301; i < maxpid ; i++) 
   {
      void *status;
      errno= 0 ;
      pthread_t idHilo;
      int error;

      error = pthread_create (&idHilo, NULL, functionThread, NULL);
      if (error != 0)
      {
         die(unlog, "Error: Cannot create thread ! Exiting.");
      }

      error = pthread_join(idHilo, &status);
      if (error != 0)
      {
         die(unlog, "Error : Cannot join thread ! Exiting.");
      }
      allpids[tid] =  0;

   }

   if(FALSE == brutesimplecheck)   // Do the scan a second time
   {
//    printf("DOING double check ...\n") ;
      for (i=301; i < maxpid ; i++) {
         void *status;
         errno= 0 ;
         pthread_t idHilo;
         int error;

         error = pthread_create (&idHilo, NULL, functionThread, NULL);
         if (error != 0)
         {
            die(unlog, "Error: Cannot create thread ! Exiting.");
         }

         error = pthread_join(idHilo, &status);
         if (error != 0)
         {
            die(unlog, "Error : Cannot join thread ! Exiting.");
         }
         allpids2[tid] =  0;
      }
   }

   /* processes that quit at this point in time create false positives */

   for(y=0; y < maxpid; y++) 
   {
      if ((allpids[y] != 0) && ((TRUE == brutesimplecheck) || (allpids2[y] != 0))) 
      {
         if(!checkps(allpids[y],PS_PROC | PS_THREAD | PS_MORE) ) 
         {
            printbadpid(allpids[y]);
         }
      }
   }
   
   if ( NULL != allpids)
      free((void *)allpids) ;
      
   if ( NULL != allpids2)
      free((void *)allpids2) ;
      

}
