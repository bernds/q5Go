/* $Header: /cvsroot/qgo/qgo/src/wavplay.c,v 1.8 2004/10/30 23:35:00 yfh2 Exp $
 * Warren W. Gay VE3WWG		Sun Feb 16 20:14:02 1997
 *
 * RECORD/PLAY MODULE FOR WAVPLAY :
 *
 * 	X LessTif WAV Play :
 *
 * 	Copyright (C) 1997  Warren W. Gay VE3WWG
 *
 * This  program is free software; you can redistribute it and/or modify it
 * under the  terms  of  the GNU General Public License as published by the
 * Free Software Foundation version 2 of the License.
 *
 * This  program  is  distributed  in  the hope that it will be useful, but
 * WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details (see enclosed file COPYING).
 *
 * You  should have received a copy of the GNU General Public License along
 * with this  program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Send correspondance to:
 *
 * 	Warren W. Gay VE3WWG
 *
 * Email:
 *	ve3wwg@yahoo.com
 *	wgay@mackenziefinancial.com
 *
 *
 * Revision 1.6  2004/05/26 17:44:05  yfh2
 * 0.2.1 b1 preparing BSD compatibiility
 *
 * Revision 1.4  2003/04/14 07:41:49  frosla
 * 0.0.16b8: skipped some widget; cosmetics
 *
 * Revision 1.2  1999/12/04 00:01:20  wwg
 * Implement wavplay-1.4 release changes
 *
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.3  1997/04/19 01:24:22  wwg
 * 1.0pl2 : Removed the extraneous ' after Hz in the wav info
 * display. Also removed some extraneous ';' from the same
 * series of prints.
 *
 * Revision 1.2  1997/04/17 23:42:02  wwg
 * Added #include <errno.h> for 1.0pl2 fix.
 *
 * Revision 1.1  1997/04/14 00:18:31  wwg
 * Initial revision
 *
 */

#ifdef __linux__

static const char rcsid[] = "@(#)recplay.c $Revision: 1.8 $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/soundcard.h>
#include "wavplay.h"
/*/#include "server.h"*/


static ErrFunc v_erf;				/* Error function for reporting */

/*
 * Play a series of WAV files:
 *
 */
 
void play(const char *Pathname) {


  static pthread_t play_tid;
  /*/fprintf(stdout,"thread opened\n");*/

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED))
     err("pthread_attr_setdetachstate() failed!");

 
  if (pthread_create (&play_tid, &attr, (int *)wavplay, Pathname))
			err("pthread_create() failed!");

}
  
int wavplay(char *argv,ErrFunc erf) {

  WavPlayOpts wavopts;
	WAVFILE *wfile;				/* Opened wav file */
	DSPFILE *dfile = NULL;			/* Opened /dev/dsp device */
	const char *Pathname;			/* Pathname of the open WAV file */
	int e;					/* Saved error code */

  memset(&wavopts,0,sizeof wavopts);	/* Zero this structure */
	wavopts.IPCKey = 0;		/* Default IPC Key for lock */
/*/	wavopts.Mode = getOprMode(argv[0],&cmd_name,&clntIPC);*/
	wavopts.Channels.optChar = 0;
	wavopts.ipc = -1;			/* Semaphore ipc ID */
	wavopts.DataBits.optValue = 16;		/* Default to 16 bits */
	wavopts.Channels.optValue = Stereo;
	wavopts.SamplingRate.optValue = 8000;

  
	if ( erf != NULL )			/* If called from external module.. */
		v_erf = erf;			/* ..set error reporting function */


  Pathname = argv;
    /*
    fprintf(stdout,"Playing WAV file %s \n",Pathname);
    fprintf(stdout, "AUDIODEV : %s : \n",AUDIODEV);
    fprintf(stdout, "WAVPLAYPATH : %s : \n",WAVPLAYPATH);
    fprintf(stdout, "WAVPLAYPATH : % : \n", AUDIOLCK);
     */

		/*
		 * Open the wav file for read, unless its stdin:
		 */
		if ( (wfile = WavOpenForRead(Pathname,v_erf)) == NULL )
			goto errxit;


		/*
		 * Merge in command line option overrides:
		 */
 
		WavReadOverrides(wfile,&wavopts);



  		if ( !wavopts.bInfoMode ) {
			/*
			 * If not -i mode, play the file:
			 */
			if ( (dfile = OpenDSP(wfile,O_WRONLY,v_erf)) == NULL )
				goto errxit;

       
			if ( PlayDSP(dfile,wfile,NULL,v_erf) )
				goto errxit;

			if ( CloseDSP(dfile,v_erf) ) {	/* Close /dev/dsp */
				dfile = NULL;		/* Mark it as closed */
				goto errxit;
			}
     
		}

		dfile = NULL;				/* Mark it as closed */
		if ( WavClose(wfile,v_erf) )		/* Close the wav file */
			wfile = NULL;			/* Mark the file as closed */
		wfile = NULL;				/* Mark the file as closed */



	return 0;

	/*
	 * Error exit:
	 */
errxit:	e = errno;					/* Save errno value */

  fprintf(stdout, "error %s : \n",strerror (errno));
	if ( wfile != NULL )
		WavClose(wfile,NULL);			/* Don't report errors here */
	if ( dfile != NULL )
		CloseDSP(dfile,NULL);			/* Don't report errors here */
	errno = e;					/* Restore error code */
	return -1;
}


#endif
