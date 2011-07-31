/* $Header: /cvsroot/qgo/qgo/src/wavfile.c,v 1.8 2005/09/08 18:46:31 yfh2 Exp $
 * Copyright: wavfile.c (c) Erik de Castro Lopo  erikd@zip.com.au
 *
 * wavfile.c - Functions for reading and writing MS-Windoze .WAV files.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2, or (at your option)
 *	any later version.
 *	
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *	
 *	This code was originally written to manipulate Windoze .WAV files
 *	under i386 Linux. Please send any bug reports or requests for 
 *	enhancements to : erikd@zip.com.au
 *	
 *
 * Revision 1.4  2004/05/26 17:44:05  yfh2
 * 0.2.1 b1 preparing BSD compatibiility
 *
 * Revision 1.3  2003/04/14 07:41:49  frosla
 * 0.0.16b8: skipped some widget; cosmetics
 *
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.2  1997/04/17 00:59:55  wwg
 * Fixed so that a fmt chunk that is larger than expected
 * is accepted, but if smaller - _then_ treat it as an
 * error.
 *
 * Revision 1.1  1997/04/14 00:14:38  wwg
 * Initial revision
 *
 *	change log:
 *	16.02.97 -	Erik de Castro Lopo		
 *	Ported from MS-Windows to Linux for Warren W. Gay's
 *	wavplay project.
 */	

#ifdef __linux__

static const char rcsid[] = "@(#)wavfile.c $Revision: 1.8 $";


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <linux/soundcard.h>




#if WORDS_BIGENDIAN
#define SwapLE16(x) ((((u_int16_t)x)<<8)|(((u_int16_t)x)>>8))
#define SwapLE32(x) ((((u_int32_t)x)<<24)|((((u_int32_t)x)<<8)&0x00FF0000) \
                     |((((u_int32_t)x)>>8)&0x0000FF00)|(((u_int32_t)x)>>24))
#else
#define SwapLE16(x) (x)
#define SwapLE32(x) (x)
#endif


#include "wavplay.h"

#define		BUFFERSIZE   		1024
#define		PCM_WAVE_FORMAT   	1

#define		TRUE			1
#define		FALSE			0

#define RECPLAY_UPDATES_PER_SEC                3


typedef  struct
{	u_int32_t  dwSize ;
	u_int16_t  wFormatTag ;
	u_int16_t  wChannels ;
	u_int32_t  dwSamplesPerSec ;
	u_int32_t  dwAvgBytesPerSec ;
	u_int16_t  wBlockAlign ;
	u_int16_t  wBitsPerSample ;
} WAVEFORMAT ;

typedef  struct
{	char    	RiffID [4] ;
	u_int32_t 	RiffSize ;
	char    	WaveID [4] ;
	char    	FmtID  [4] ;
	u_int32_t  	FmtSize ;
	u_int16_t 	wFormatTag ;
	u_int16_t  	nChannels ;
	u_int32_t	nSamplesPerSec ;
	u_int32_t	nAvgBytesPerSec ;
	u_int16_t	nBlockAlign ;
	u_int16_t	wBitsPerSample ;
	char		DataID [4] ;
	u_int32_t	nDataBytes ;
} WAVE_HEADER ;

/*=================================================================================================*/

char*  findchunk (char* s1, char* s2, size_t n) ;

/*=================================================================================================*/


static  WAVE_HEADER  waveheader =
{	{ 'R', 'I', 'F', 'F' },
		0,
	{ 'W', 'A', 'V', 'E' },
	{ 'f', 'm', 't', ' ' },
		16,								/* FmtSize*/
		PCM_WAVE_FORMAT,						/* wFormatTag*/
		0,								/* nChannels*/
		0,
		0,
		0,
		0,
	{ 'd', 'a', 't', 'a' },
		0
} ; /* waveheader*/

static ErrFunc v_erf;				/* wwg: Error reporting function */

static void _v_erf(const char *, va_list);	/* This module's error reporting function */
static char emsg[2048];



/*
 * Error reporting function for this source module:
 */
static void
err(const char *format,...) {
	va_list ap;
   fprintf(stdout, "error : %s \n",format);
	/*/if ( v_erf == NULL )*/
	/*/	return;				/* Only report error if we have function */
	va_start(ap,format);
	_v_erf(format,ap);			/* Use caller's supplied function */
	va_end(ap);
}


static void
_v_erf(const char *format,va_list ap) {
	vsprintf(emsg,format,ap);		/* Capture message into emsg[] */
}



int  WaveReadHeader  (int wavefile, int* channels, u_long* samplerate, int* samplebits, u_long* samples, u_long* datastart,ErrFunc erf)
{	static  WAVEFORMAT  waveformat ;
	static	char   buffer [ BUFFERSIZE ] ;		/* Function is not reentrant.*/
	char*   ptr ;
	u_long  databytes ;

	v_erf = erf;					/* wwg: Set error reporting function */

	if (lseek (wavefile, 0L, SEEK_SET)) {
		err("%s",sys_errlist[errno]);		/* wwg: Report error */
		return  WR_BADSEEK ;
	}

	read (wavefile, buffer, BUFFERSIZE) ;

	if (findchunk (buffer, "RIFF", BUFFERSIZE) != buffer) {
		err("Bad format: Cannot find RIFF file marker");	/* wwg: Report error */
		return  WR_BADRIFF ;
	}

	if (! findchunk (buffer, "WAVE", BUFFERSIZE)) {
		err("Bad format: Cannot find WAVE file marker");	/* wwg: report error */
		return  WR_BADWAVE ;
	}

	ptr = findchunk (buffer, "fmt ", BUFFERSIZE) ;

	if (! ptr) {
		err("Bad format: Cannot find 'fmt' file marker");	/* wwg: report error */
		return  WR_BADFORMAT ;
	}

	ptr += 4 ;	/* Move past "fmt ".*/
	memcpy (&waveformat, ptr, sizeof (WAVEFORMAT)) ;
	waveformat.dwSize = SwapLE32(waveformat.dwSize);
	waveformat.wFormatTag = SwapLE16(waveformat.wFormatTag) ;
	waveformat.wChannels = SwapLE16(waveformat.wChannels) ;
	waveformat.dwSamplesPerSec = SwapLE32(waveformat.dwSamplesPerSec) ;
	waveformat.dwAvgBytesPerSec = SwapLE32(waveformat.dwAvgBytesPerSec) ;
	waveformat.wBlockAlign = SwapLE16(waveformat.wBlockAlign) ;
	waveformat.wBitsPerSample = SwapLE16(waveformat.wBitsPerSample) ;
	
	if (waveformat.dwSize < (sizeof (WAVEFORMAT) - sizeof (u_long))) {
		err("Bad format: Bad fmt size");			/* wwg: report error */
		return  WR_BADFORMATSIZE ;
	}

	if (waveformat.wFormatTag != PCM_WAVE_FORMAT) {
		err("Only supports PCM wave format");			/* wwg: report error */
		return  WR_NOTPCMFORMAT ;
	}

	ptr = findchunk (buffer, "data", BUFFERSIZE) ;

	if (! ptr) {
		err("Bad format: unable to find 'data' file marker");	/* wwg: report error */
		return  WR_NODATACHUNK ;
	}

	ptr += 4 ;	/* Move past "data".*/
	memcpy (&databytes, ptr, sizeof (u_long)) ;

	/* Everything is now cool, so fill in output data.*/

	*channels   = waveformat.wChannels ;
	*samplerate = waveformat.dwSamplesPerSec ;
	*samplebits = waveformat.wBitsPerSample ;
	*samples    = databytes / waveformat.wBlockAlign ;
	
	*datastart  = ((u_long) (ptr + 4)) - ((u_long) (&(buffer[0]))) ;

	if (waveformat.dwSamplesPerSec != waveformat.dwAvgBytesPerSec / waveformat.wBlockAlign) {
		err("Bad file format");			/* wwg: report error */
		return  WR_BADFORMATDATA ;
	}

	if (waveformat.dwSamplesPerSec != waveformat.dwAvgBytesPerSec / waveformat.wChannels / ((waveformat.wBitsPerSample == 16) ? 2 : 1)) {
		err("Bad file format");			/* wwg: report error */
		return  WR_BADFORMATDATA ;
	}

  return  0 ;
} ; /* WaveReadHeader*/

/*===========================================================================================*/

#if 0
char*  WaveFileError (int  errno)
{	switch (errno)
	{	case	WW_BADOUTPUTFILE	: return "Bad output file.\n" ;
		case	WW_BADWRITEHEADER 	: return "Not able to write WAV header.\n" ;
		
		case	WR_BADALLOC			: return "Not able to allocate memory.\n" ;
		case	WR_BADSEEK        	: return "fseek failed.\n" ;
		case	WR_BADRIFF        	: return "Not able to find 'RIFF' file marker.\n" ;
		case	WR_BADWAVE        	: return "Not able to find 'WAVE' file marker.\n" ;
		case	WR_BADFORMAT      	: return "Not able to find 'fmt ' file marker.\n" ;
		case	WR_BADFORMATSIZE  	: return "Format size incorrect.\n" ;
		case	WR_NOTPCMFORMAT		: return "Not PCM format WAV file.\n" ;
		case	WR_NODATACHUNK		: return "Not able to find 'data' file marker.\n" ;
		case	WR_BADFORMATDATA	: return "Format data questionable.\n" ;
		default           			:  return "No error\n" ;
	} ;
	return	NULL ;	
} ; /* WaveFileError*/
#endif
/*===========================================================================================*/

char* findchunk  (char* pstart, char* fourcc, size_t n)
{	char	*pend ;
	int		k, test ;

	pend = pstart + n ;

	while (pstart < pend)
	{ 	if (*pstart == *fourcc)       /* found match for first char*/
		{	test = TRUE ;
			for (k = 1 ; fourcc [k] != 0 ; k++)
				test = (test ? ( pstart [k] == fourcc [k] ) : FALSE) ;
			if (test)
				return  pstart ;
			} ; /* if*/
		pstart ++ ;
		} ; /* while lpstart*/

	return  NULL ;
} ; /* findchuck*/

/* $Source: /cvsroot/qgo/qgo/src/wavfile.c,v $ */
/*
 * Internal routine to allocate WAVFILE structure:
 */
static WAVFILE *
wavfile_alloc(const char *Pathname) {
	WAVFILE *wfile = (WAVFILE *) malloc(sizeof (WAVFILE));

	if ( wfile == NULL ) {
		err("%s: Allocating WAVFILE structure",sys_errlist[ENOMEM]);
		return NULL;
	}

	memset(wfile,0,sizeof *wfile);

	if ( (wfile->Pathname = strdup(Pathname)) == NULL ) {
		free(wfile);
		err("%s: Allocating storage for WAVFILE.Pathname",sys_errlist[ENOMEM]);
		return NULL;
	}

	wfile->fd = -1;				/* Initialize fd as not open */
	wfile->wavinfo.Channels = Mono;
	wfile->wavinfo.DataBits = 8;

	return wfile;
}

/*
 * Internal routine to release WAVFILE structure:
 * No errors reported.
 */
static void
wavfile_free(WAVFILE *wfile) {
	if ( wfile->Pathname != NULL )
		free(wfile->Pathname);
	free(wfile);
}

/*
 * Open a WAV file for reading: returns (WAVFILE *)
 *
 * The opened file is positioned at the first byte of WAV file data, or
 * NULL is returned if the open is unsuccessful.
 */
WAVFILE *
WavOpenForRead(const char *Pathname,ErrFunc erf) {
	WAVFILE *wfile = wavfile_alloc(Pathname);
	int e;					/* Saved errno value */
	UInt32 offset;				/* File offset */
	Byte ubuf[4];				/* 4 byte buffer */
	UInt32 dbytes;				/* Data byte count */
						/* wavfile.c values : */
	int channels;				/* Channels recorded in this wav file */
	u_long samplerate;			/* Sampling rate */
	int sample_bits;			/* data bit size (8/12/16) */
	u_long samples;				/* The number of samples in this file */
	u_long datastart;			/* The offset to the wav data */

	v_erf = erf;				/* Set error reporting function */

	if ( wfile == NULL )
		return NULL;			/* Insufficient memory (class B msg) */

	/*
	 * Open the file for reading:
	 */
	if ( (wfile->fd = open(wfile->Pathname,O_RDONLY)) < 0 ) {
		err("%s:\nOpening WAV file %s",
			sys_errlist[errno],
			wfile->Pathname);
		goto errxit;
	}

	if ( lseek(wfile->fd,0L,SEEK_SET) != 0L ) {
		err("%s:\nRewinding WAV file %s",
			sys_errlist[errno],
			wfile->Pathname);
		goto errxit;		/* Wav file must be seekable device */
	}

	if ( (e = WaveReadHeader(wfile->fd,&channels,&samplerate,&sample_bits,&samples,&datastart,_v_erf)) != 0 ) {
		err("%s:\nReading WAV header from %s",
			emsg,
			wfile->Pathname);
		goto errxit;
	}

	/*
	 * Copy WAV data over to WAVFILE struct:
	 */
	if ( channels == 2 )
		wfile->wavinfo.Channels = Stereo;
	else	wfile->wavinfo.Channels = Mono;

	wfile->wavinfo.SamplingRate = (UInt32) samplerate;
	wfile->wavinfo.Samples = (UInt32) samples;
	wfile->wavinfo.DataBits = (UInt16) sample_bits;
	wfile->wavinfo.DataStart = (UInt32) datastart;
        wfile->num_samples = wfile->wavinfo.Samples;
	wfile->rw = 'R';			/* Read mode */

	offset = wfile->wavinfo.DataStart - 4;

	/*
	 * Seek to byte count and read dbytes:
	 */
	if ( lseek(wfile->fd,offset,SEEK_SET) != offset ) {
		err("%s:\nSeeking to WAV data in %s",sys_errlist[errno],wfile->Pathname);
		goto errxit;			/* Seek failure */
	}

	if ( read(wfile->fd,ubuf,4) != 4 ) {
		err("%s:\nReading dbytes from %s",sys_errlist[errno],wfile->Pathname);
		goto errxit;
	}

	/*
	 * Put little endian value into 32 bit value:
	 */
	dbytes = ubuf[3];
	dbytes = (dbytes << 8) | ubuf[2];
	dbytes = (dbytes << 8) | ubuf[1];
	dbytes = (dbytes << 8) | ubuf[0];

	wfile->wavinfo.DataBytes = dbytes;

	/*
	 * Open succeeded:
	 */
	return wfile;				/* Return open descriptor */

	/*
	 * Return error after failed open:
	 */
errxit:	e = errno;				/* Save errno */
	free(wfile->Pathname);			/* Dispose of copied pathname */
	free(wfile);				/* Dispose of WAVFILE struct */
	errno = e;				/* Restore error number */
	return NULL;				/* Return error indication */
}

/*
 * Apply command line option overrides to the interpretation of the input
 * wav file:
 *
 */
void
WavReadOverrides(WAVFILE *wfile,WavPlayOpts *wavopts) {
	UInt32 num_samples;

	/*
	 * Override sampling rate: -s sampling_rate
	 */
	if ( wavopts->SamplingRate.optChar != 0 ) {
		wfile->wavinfo.SamplingRate = wavopts->SamplingRate.optValue;
		wfile->wavinfo.bOvrSampling = 1;
	}

	/*
	 * Override mono/stereo mode: -S / -M
	 */
	if ( wavopts->Channels.optChar != 0 ) {
		wfile->wavinfo.Channels = wavopts->Channels.optValue;
		wfile->wavinfo.bOvrMode = 1;
	}

	/*
	 * Override the sample size in bits: -b bits
	 */
	if ( wavopts->DataBits.optChar != 0 ) {
		wfile->wavinfo.DataBits = wavopts->DataBits.optValue;
		wfile->wavinfo.bOvrBits = 1;
	}

        /*
         * Set the first sample:
         */
        wfile->StartSample = 0;
        num_samples = wfile->wavinfo.Samples = wfile->num_samples;
        if ( wavopts->StartSample != 0 ) {
                wfile->StartSample = wavopts->StartSample;
                wfile->wavinfo.Samples -= wfile->StartSample;
        }

        /*
         * Override # of samples if -t seconds option given:
         */
        if ( wavopts->Seconds != 0 ) {
                wfile->wavinfo.Samples = wavopts->Seconds * wfile->wavinfo.SamplingRate;
                if (wfile->StartSample+wfile->wavinfo.Samples > num_samples)
                        wfile->wavinfo.Samples = num_samples-1;
        }
}

/*
 * Close a WAVFILE
 */
int
WavClose(WAVFILE *wfile,ErrFunc erf) {
	int e = 0;				/* Returned error code */
	int channels;				/* Channels recorded in this wav file */
	u_long samplerate;			/* Sampling rate */
	int sample_bits;			/* data bit size (8/12/16) */
	u_long samples;				/* The number of samples in this file */
	u_long datastart;			/* The offset to the wav data */
	long fpos;				/* File position in bytes */

	v_erf = erf;				/* Set error reporting function */

	if ( wfile == NULL ) {
		err("%s: WAVFILE pointer is NULL!",sys_errlist[EINVAL]);
		errno = EINVAL;
		return -1;
	}

	/*
	 * If the wav file was open for write, update the actual number
	 * of samples written to the file:
	 */
	if ( wfile->rw == 'W' ) {
		fpos = lseek(wfile->fd,0L,SEEK_CUR);	/* Get out file position */
		if ( (e = WaveReadHeader(wfile->fd,&channels,&samplerate,&sample_bits,&samples,&datastart,_v_erf)) != 0 )
			err("%s:\nReading WAV header from %s",emsg,wfile->Pathname);
		else if ( lseek(wfile->fd,(long)(datastart-4),SEEK_SET) != (long)(datastart-4) )
			err("%s:\nSeeking in WAV header file %s",sys_errlist[errno],wfile->Pathname);
		else if ( write(wfile->fd,&wfile->wavinfo.Samples,sizeof wfile->wavinfo.Samples) != sizeof wfile->wavinfo.Samples )
			err("%s:\nWriting in WAV header file %s",sys_errlist[errno],wfile->Pathname);
		else	{
			/*
			 * 'data' chunk was updated OK: Now we have to update the RIFF block
			 * count. Someday very soon, a real RIFF module is going to replace
			 * this fudging.
			 */
			if ( ftruncate(wfile->fd,(size_t)fpos) )
				err("%s:\nTruncating file %s to correct size",
					sys_errlist[errno],
					wfile->Pathname);
			else if ( lseek(wfile->fd,4L,SEEK_SET) < 0L )
				err("%s:\nSeek 4 for RIFF block update of %s",
					sys_errlist[errno],
					wfile->Pathname);
			else	{
				fpos -= 8;		/* Byte count for RIFF block */
				if ( write(wfile->fd,&fpos,sizeof fpos) != sizeof fpos )
					err("%s:\nUpdate of RIFF block count in %s failed",
						sys_errlist[errno],
						wfile->Pathname);
			}
		}
	}

	if ( close(wfile->fd) < 0 ) {
		err("%s:\nClosing WAV file",sys_errlist[errno]);
		e = errno;			/* Save errno value to return */
	}

	wavfile_free(wfile);			/* Release WAVFILE structure */

	if ( (errno = e) != 0 )
		return -1;			/* Failed exit */
	return 0;				/* Successful exit */
}

/*

/*
 * Open /dev/dsp for reading or writing:
 */
DSPFILE *
OpenDSP(WAVFILE *wfile,int omode,ErrFunc erf) {
	int e;					/* Saved errno value */
	int t;					/* Work int */
	unsigned long ul;			/* Work unsigned long */
	DSPFILE *dfile = (DSPFILE *) malloc(sizeof (DSPFILE));

	v_erf = erf;				/* Set error reporting function */

	if ( dfile == NULL ) {
		err("%s: Opening DSP device",sys_errlist[errno=ENOMEM]);
		return NULL;
	}

	memset(dfile,0,sizeof *dfile);
	dfile->dspbuf = NULL;

	/*
	 * Open the device driver:
	 */
	if ( (dfile->fd = open(AUDIODEV,omode,0)) < 0 ) {
		err("%s:\nOpening audio device %s",
			sys_errlist[errno],
			AUDIODEV);
		goto errxit;
	}

        /*
         * Determine the audio device's block size.  Should be done after
         * setting sampling rate etc.
         */
        if ( ioctl(dfile->fd,SNDCTL_DSP_GETBLKSIZE,&dfile->dspblksiz) < 0 ) {
                err("%s: Optaining DSP's block size",sys_errlist[errno]);
                goto errxit;
        }

        /*
         * Check the range on the buffer sizes:
         */
        /* Minimum was 4096 but es1370 returns 1024 for 44.1kHz, 16 bit */
        /* and 64 for 8130Hz, 8 bit */
        if ( dfile->dspblksiz < 32 || dfile->dspblksiz > 65536 ) {
                err("%s: Audio block size (%d bytes)",
                        sys_errlist[errno=EINVAL],
                        (int)dfile->dspblksiz);
                goto errxit;
        }

        /*
         * Allocate a buffer to do the I/O through:
         */
        if ( (dfile->dspbuf = (char *) malloc(dfile->dspblksiz)) == NULL ) {
                err("%s: For DSP I/O buffer",sys_errlist[errno]);
                goto errxit;
        }

	/*
	 * Set the data bit size:
	 */
	t = wfile->wavinfo.DataBits;
        if ( ioctl(dfile->fd,SNDCTL_DSP_SAMPLESIZE,&t) < 0 ) {
		err("%s: Setting DSP to %u bits",sys_errlist[errno],(unsigned)t);
		goto errxit;
	}

	/*
	 * Set the mode to be Stereo or Mono:
	 */
	t = wfile->wavinfo.Channels == Stereo ? 1 : 0;
	if ( ioctl(dfile->fd,SNDCTL_DSP_STEREO,&t) < 0 ) {
		err("%s: Unable to set DSP to %s mode",
			sys_errlist[errno],
			t?"Stereo":"Mono");
		goto errxit;
	}

	/*
	 * Set the sampling rate:
	 */
	ul = wfile->wavinfo.SamplingRate;
	if ( ioctl(dfile->fd,SNDCTL_DSP_SPEED,&ul) < 0 ) {
		err("Unable to set audio sampling rate",sys_errlist[errno]);
		goto errxit;
	}

	/*
	 * Return successfully opened device:
	 */
	return dfile;				/* Return file descriptor */

	/*
	 * Failed to open/initialize properly:
	 */
errxit:	e = errno;				/* Save the errno value */

 fprintf(stdout, "error %s : \n",sys_errlist[errno]);

	if ( dfile->fd >= 0 )
		close(dfile->fd);		/* Close device */
	if ( dfile->dspbuf != NULL )
		free(dfile->dspbuf);
	free(dfile);
	errno = e;				/* Restore error code */
	return NULL;				/* Return error indication */
}

/*
 * Close the DSP device:
 */
int
CloseDSP(DSPFILE *dfile,ErrFunc erf) {
	int fd;

	v_erf = erf;				/* Set error reporting function */

	if ( dfile == NULL ) {
		err("%s: DSPFILE is not open",sys_errlist[errno=EINVAL]);
		return -1;
	}

	fd = dfile->fd;
	if ( dfile->dspbuf != NULL )
		free(dfile->dspbuf);
	free(dfile);

	if ( close(fd) ) {
		err("%s: Closing DSP fd %d",sys_errlist[errno],fd);
		return -1;
	}

	return 0;
}

/*
 * Play DSP from WAV file:
 */
int
PlayDSP(DSPFILE *dfile,WAVFILE *wfile,DSPPROC work_proc,ErrFunc erf) {
	UInt32 byte_count = (UInt32) wfile->wavinfo.Samples;
	int bytes;
	int n;
	int byte_modulo;
        int total_bytes, update_bytes;
        SVRMSG msg;

	v_erf = erf;				/* Set error reporting function */

	/*
	 * Check that the WAVFILE is open for reading:
	 */
	if ( wfile->rw != 'R' ) {
		err("%s: WAVFILE must be open for reading",sys_errlist[errno=EINVAL]);
		return -1;
	}

	/*
	 * First determine how many bytes are required for each channel's sample:
	 */
	switch ( wfile->wavinfo.DataBits ) {
	case 8 :
		byte_count = 1;
		break;
	case 16 :
		byte_count = 2;
		break;
	default :
		err("%s: Cannot process %u bit samples",
			sys_errlist[errno=EINVAL],
			(unsigned)wfile->wavinfo.DataBits);
		return -1;
	}

	/*
	 * Allow for Mono/Stereo difference:
	 */
	if ( wfile->wavinfo.Channels == Stereo )
		byte_count *= 2;		/* Twice as many bytes for stereo */
	else if ( wfile->wavinfo.Channels != Mono ) {
		err("%s: DSPFILE control block is corrupted (chan_mode)",
			sys_errlist[errno=EINVAL]);
		return -1;
	}

	byte_modulo = byte_count;				/* This many bytes per sample */
	byte_count  = wfile->wavinfo.Samples * byte_modulo;	/* Total bytes to process */
        total_bytes = byte_count;

        /* Number of bytes to write between client updates.  Must be */
        /* a multiple of dspblksiz.  */
        update_bytes = ((wfile->wavinfo.SamplingRate*byte_modulo) / (RECPLAY_UPDATES_PER_SEC*dfile->dspblksiz)) * dfile->dspblksiz;

	if ( ioctl(dfile->fd,SNDCTL_DSP_SYNC,0) != 0 )
		err("%s: ioctl(%d,SNDCTL_DSP_SYNC,0)",sys_errlist[errno]);

        /* Seek to requested start sample */
        lseek(wfile->fd,wfile->StartSample*byte_modulo,SEEK_CUR);

	for ( ; byte_count > 0 && wfile->wavinfo.DataBytes > 0; byte_count -= (UInt32) n ) {

		bytes = (int) ( byte_count > dfile->dspblksiz ? dfile->dspblksiz : byte_count );

		if ( bytes > wfile->wavinfo.DataBytes )	/* Size bigger than data chunk? */
			bytes = wfile->wavinfo.DataBytes;	/* Data chunk only has this much left */

		if ( (n = read(wfile->fd,dfile->dspbuf,bytes)) != bytes ) {
			if ( n >= 0 )
				err("Unexpected EOF reading samples from WAV file",sys_errlist[errno=EIO]);
			else	err("Reading samples from WAV file",sys_errlist[errno]);
			goto errxit;
		}
                /*
                if ((clntIPC >= 0) && !((total_bytes-byte_count) % update_bytes)) {
                        msg.msg_type = ToClnt_PlayState;
                        msg.bytes = sizeof(msg.u.toclnt_playstate);
                        msg.u.toclnt_playstate.SamplesLeft = byte_count / byte_modulo;
                        msg.u.toclnt_playstate.CurrentSample =
                          wfile->num_samples - msg.u.toclnt_playstate.SamplesLeft;
                        MsgToClient(clntIPC,&msg,0);
                }    /* Tell client playback status */

		if ( write(dfile->fd,dfile->dspbuf,n) != n ) {
			err("Writing samples to audio device",sys_errlist[errno]);
			goto errxit;
		}

		wfile->wavinfo.DataBytes -= (UInt32) bytes;	/* We have fewer bytes left to read */

		/*
		 * The work procedure function is called when operating
		 * in server mode to check for more server messages:
		 */
		if ( work_proc != NULL && work_proc(dfile) )	/* Did work_proc() return TRUE? */
			break;					/* Yes, quit playing */
	}

#if 0	/* I think this is doing a destructive flush: disabled */
	if ( ioctl(dfile->fd,SNDCTL_DSP_SYNC,0) != 0 )
		err("%s: ioctl(%d,SNDCTL_DSP_SYNC,0)",sys_errlist[errno]);
#endif
        /* Update client time display at end of sucessful play
        if (clntIPC >= 0) {
                msg.msg_type = ToClnt_PlayState;
                msg.bytes = sizeof(msg.u.toclnt_playstate);
                msg.u.toclnt_playstate.SamplesLeft = byte_count / byte_modulo;
                msg.u.toclnt_playstate.CurrentSample =
                  wfile->num_samples - msg.u.toclnt_playstate.SamplesLeft;
                MsgToClient(clntIPC,&msg,0);
        }               /* Tell client playback status */
	return 0;	/* All samples played successfully */

errxit:	return -1;	/* Indicate error return */
}




#endif
