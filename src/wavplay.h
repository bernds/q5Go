/* $Header: /cvsroot/qgo/qgo/src/wavplay.h,v 1.4 2006/11/06 07:09:45 yfh2 Exp $
 * Warren W. Gay VE3WWG		Sun Feb 16 18:17:17 1997
 *
 * WAVPLAY OPTION SETTINGS:
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
 * Revision 1.1  2003/04/07 10:28:54  frosla
 * 0.0.16b3: added real sound support for Unix!!!!
 *
 * Revision 1.3  1999/12/04 00:05:34  wwg
 * Fixed release number
 *
 * Revision 1.2  1999/12/04 00:01:20  wwg
 * Implement wavplay-1.4 release changes
 *
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.4  1997/04/17 23:39:26  wwg
 * Now at 1.0pl2 : attempting to fix Linux 2.0.29 compile problems
 * with the way errno and sys_errlist[] are handled for thread
 * safety.. here we removed our incompatible extern sys_errlist[]
 *
 * Revision 1.3  1997/04/17 01:59:37  wwg
 * Added version change to reflect patch level "1.0pl1"
 *
 * Revision 1.2  1997/04/17 00:27:36  wwg
 * Fixed references to the name errno. For thread safety
 * this name has been macro-ized, which I should have
 * anticipated, since I knew it was coming.
 *
 * Revision 1.1  1997/04/14 00:56:48  wwg
 * Initial revision
 *
 */



#ifndef _wavplay_h_
#define _wavplay_h_ "$Id: wavplay.h,v 1.4 2006/11/06 07:09:45 yfh2 Exp $"

#define WAVPLAY_VERSION		"1.4"


#include <stdarg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <pthread.h>




typedef void (*ErrFunc)(const char *format,va_list ap);

#include "wavfile.h"

/*
 * Default location of the wavplay server program if not overrided by the Makefile
 */
#ifndef WAVPLAYPATH
#define WAVPLAYPATH "/usr/local/bin/wavplay"	/* Default location of wavplay server */
#endif

/*
 * Default pathname for recordings if not overriden by the Makefile:
 */
#ifndef RECORD_PATH
#define RECORD_PATH "recorded.wav"
#endif

/*
 * Default lowest sampling rate unless overriden by the Makefile:
 */
#ifndef DSP_MIN
#define DSP_MIN		4000			/* Lowest acceptable sample rate */
#endif

/*
 * Default maximum sampling rate unless overrided by the Makefile:
 */
#ifndef DSP_MAX
#define DSP_MAX		48000			/* Highest acceptable sample rate */
#endif

/*
 * Default pathname of the audio device, unless overrided by the Makefile:
 */
#ifndef AUDIODEV
#define AUDIODEV	"/dev/dsp"		/* Default pathname for audio device */
#endif

/*
 * Default locking semaphore IPC Key, unless overrided by the Makefile:
 */
#ifndef AUDIOLCK
#define AUDIOLCK	0x33333333		/* Default IPC Key for semaphores */
#endif

/*
 * Short option flag characters:
 */
#define OPF_DEVICE      'd'                     /* -d device ; Override /dev/dsp default */
#define OPF_INFO	'i'			/* -i ; info mode option */
#define OPF_HELP	'h'			/* -h ; help optino */
#define OPF_QUIET	'q'			/* -q ; quiet mode */
#define OPF_SAMPRATE	's'			/* -s rate ; Sampling rate */
#define OPF_STEREO	'S'			/* -S ; Stereo */
#define OPF_MONO	'M'			/* -M ; Mono */
#define OPF_TIME	't'			/* -t seconds ; Time option */
#define OPF_DATABITS	'b'			/* -b data_bits; sample bits */
#define OPF_IPCKEY	'k'			/* -k key ; IPC Key */
#define OPF_RESET	'R'			/* -r ; reset semaphores option */
#define OPF_PLAY_LOCK	'l'			/* -l ; lock for play option */
#define OPF_PLAY_UNLOCK	'u'			/* -u ; unlock play option */
#define OPF_RECD_LOCK	'L'			/* -L ; lock for record option */
#define OPF_RECD_UNLOCK	'U'			/* -U ; unlock record option */
#define OPF_DEBUG	'x'			/* -x ; debug option */
#define OPF_VERSION	'V'			/* -V ; version and copyright */

/*
 * Types internal to wavplay, in an attempt to isolate ourselves from
 * a dependance on a particular platform.
 */
typedef unsigned char Byte;
typedef short Int16;
typedef long Int32;
typedef unsigned long UInt32;
typedef unsigned short UInt16;

/*
 * This value sets buffer sizes for temporary buffers that sprintf()
 * uses, and for copying pathnames around in. You probably don't want
 * to mess with this.
 */
#define MAXTXTLEN	2048			/* Must allow for long pathnames */

/*
 * These are the wavplay command operation modes.
 */
typedef enum {
	OprNoMode=0,				/* No mode given (not determined yet) */
	OprRecord=1,				/* wavplay command is in "Record Mode" */
	OprPlay=2,				/* wavplay command is in "Play Mode" */
	OprServer=3				/* wavplay is accting in "Server Mode" */
} OprMode;

/*
 * This enumerated type, selects between monophonic sound and
 * stereo sound (1 or 2 channels).
 */
typedef enum {
	Mono,					/* Monophonic sound (1 channel) */
	Stereo					/* Stereo sound (2 channels) */
} Chan;

/*
 * This type is used for those options that can be one or another
 * option flags (represented as the ASCII character), or no option
 * flags at all (zero value, ie. 0x00).
 */
typedef struct {
	char	optChar;			/* Option character */
} FlgOpt;

/*
 * This type represents any unsigned 32 bit option value, if the member
 * optChar is non-zero (usually holds the ASCII flag option character).
 * If optChar is zero, then no value is present (or specified).
 */
typedef struct {
	char	optChar;			/* Zero if not valid, else non-zero if active */
	UInt32	optValue;			/* The unsigned 32 bit value if optChar is true */
} U32Opt;

typedef struct {
	char	optChar;			/* Zero if not valid, else non-zero if active */
	UInt16	optValue;			/* The unsigned 16 bit value if optChar is true */
} U16Opt;

typedef struct {
	char	optChar;			/* Zero if not valid, else non-zero if active */
	Chan	optValue;			/* The enumerated value for Stereo/Mono */
} ChnOpt;

/*
 * This structure holds the command line options for the wavplay command.
 * It is also used to pass option values around (in server mode etc.)
 */
typedef struct {
	key_t	IPCKey;				/* Default IPC Key for lock */
	OprMode	Mode;				/* Operation Mode: OprRecord or OprPlay */
	FlgOpt	PlayLock;			/* -l or -u option flag */
	FlgOpt	RecdLock;			/* -L or -U option flag */
	char	bResetLocks;			/* True if semaphores are to be reset */
	char	bQuietMode;			/* True if quiet mode is requested */
	char	bInfoMode;			/* True if only wav header info is to be printed */
	U32Opt	SamplingRate;			/* -s rate ; sampling rate in Hz */
	ChnOpt	Channels;			/* -S ; or no -S option (stereo/mono respectively) */
	U16Opt	DataBits;			/* -b bits ; number of bits per sample */
	UInt32	Seconds;			/* Time limited to this many seconds, else zero */
        UInt32  StartSample;                    /* Sample to start playback with */
	int	ipc;				/* Semaphore IPC ID */
} WavPlayOpts;

/*
 * These values represent values found in/or destined for a
 * WAV file.
 */
typedef struct {
	UInt32	SamplingRate;			/* Sampling rate in Hz */
	Chan	Channels;			/* Mono or Stereo */
	UInt32	Samples;			/* Sample count */	
	UInt16	DataBits;			/* Sample bit size (8/12/16) */
	UInt32	DataStart;			/* Offset to wav data */
	UInt32	DataBytes;			/* Data bytes in current chunk */
	char	bOvrSampling;			/* True if sampling_rate overrided */
	char	bOvrMode;			/* True if chan_mode overrided */
	char	bOvrBits;			/* True if data_bits is overrided */
} WAVINF;

/*
 * This structure manages an open WAV file.
 */
typedef struct {
	char	rw;				/* 'R' for read, 'W' for write */
	char	*Pathname;			/* Pathname of wav file */
	int	fd;				/* Open file descriptor or -1 */
	WAVINF	wavinfo;			/* WAV file hdr info */
        UInt32  num_samples;                    /* Total number of samples */
        UInt32  StartSample;                    /* First sample to play */
} WAVFILE;

/*
 * This macro is used to return the system file descriptor
 * associated with the open WAV file, given a (WAVFILE *).
 */
#define WAV_FD(wfile) (wfile->fd)		/* Return file descriptor */

/*
 * This structure manages an opened DSP device.
 */
typedef struct {
	int	fd;				/* Open fd of /dev/dsp */
	int	dspblksiz;			/* Size of the DSP buffer */
	char	*dspbuf;			/* The buffer */
} DSPFILE;

/*
 * This structure manages server information and state:
 */
typedef struct {
	UInt32	SamplingRate;			/* Sampling rate in Hz */
	Chan	Channels;			/* Mono or Stereo */
	UInt32	Samples;			/* Sample count */	
	UInt16	DataBits;			/* Sample bit size (8/12/16) */
	char	WavType[16];			/* "PCM" */
	char	bOvrSampling;			/* True if sampling is overrided */
	char	bOvrMode;			/* True if mode is overrided */
	char	bOvrBits;			/* True if bits is overrided */
} SVRINF;

/*
 * This is the function type that is called between blocks
 * of I/O with the DSP.
 */
typedef int (*DSPPROC)(DSPFILE *dfile);		/* DSP work procedure */

/*
 * Client/Server Message Types. These definitions must be coordinated with
 * source module msg.c, function msg_name(), for their corresponding
 * message texts.
 */
typedef enum {
	ToClnt_Fatal=0,				/* Fatal server error */
	ToClnt_Ready=1,				/* Tell client that server is ready */
	ToSvr_Bye=2,				/* Client tells server to exit */
	ToSvr_Path=3,				/* Client tells server a pathname */
	ToClnt_Path=4,				/* Client tells server a pathname */
	ToClnt_Stat=5,				/* Response: Svr->Clnt to ToSvr_Path */
	ToClnt_WavInfo=6,			/* Server tells client wav info */
	ToSvr_Play=7,				/* Client tells server to play */
	ToSvr_Pause=8,				/* Tell server to pause */
	ToSvr_Stop=9,				/* Tell server to stop */
	ToSvr_Bits=10,				/* Tell server to use 8 bits */
	ToClnt_Bits=11,				/* Server tells what bit setting is in effect */
	ToClnt_Settings=12,			/* Current server settings */
	ToSvr_SamplingRate=13,			/* Tell server new sampling rate */
	ToSvr_Restore=14,			/* Clear overrides: restore original settings */
	ToSvr_Chan=15,				/* Change Stereo/Mono mode */
	ToSvr_Record=16,			/* Tell server to start recording */
	ToSvr_Debug=17,				/* Tell server debug mode setting */
	ToClnt_ErrMsg=18,			/* Pass back to client, an error message string */
	ToSvr_SemReset=19,			/* Reset locking semaphores */
        ToSvr_StartSample=20,                   /* Start playback at requested sample */
        ToClnt_PlayState=21,                    /* Playback status */
        ToClnt_RecState=22,                     /* Record status */
        MSGTYP_Last=23                         /* This is not really a message type */
} MSGTYP;

/*
 * Client/Server Message Structure: This consists of a common header
 * component, and then a union of specific format variations according
 * to the message type.
 */
typedef struct {
	long	type;				/* Message Type: 1=server, 0=client */
	MSGTYP	msg_type;			/* Client/Server message type */
	UInt16	bytes;				/* Byte length of the union */
	union	{

		/*
		 * Message from server to client, to convey a fatal error that
		 * has occured in the server.
		 */
		struct	{
			int	Errno;		/* Error code */
			char	msg[128];	/* Error message text */
		} toclnt_fatal;

		/*
		 * Message from the X client, to the server, to indicate that
		 * a new pathname is to be referenced.
		 */
		struct	{
			char	path[1024];	/* Pathname */
		} tosvr_path;			/* Tell server a pathname */

		/*
		 * Message from the server to the X client, to indicate
		 * that the indicated pathname has been accepted and
		 * ready (the pathname may be canonicalized at some future
		 * revision of the server)
		 */
		struct	{
			char	path[1024];	/* Pathname */
		} toclnt_path;			/* ..from server as confirmation */

		/*
		 * Message to X client, from the server, indicating a
		 * stat() error when Errno != 0, or stat() information
		 * when Errno == 0.
		 */
		struct	{
			int	Errno;		/* Zero if OK, else errno from stat() */
			struct stat sbuf;	/* Path's stat info */
		} toclnt_stat;

		/*
		 * Message to X client, from server, indicating an
		 * error if errno != 0, or successfully obtained
		 * WAV file info if errno == 0.
		 */
		struct	{
			int	Errno;		/* Zero if OK, else errno value */
			WAVINF	wavinfo;	/* WAV file info */
			char	errmsg[256];	/* Error message */
		} toclnt_wavinfo;

		/*
		 * Message from X client, to server, indicating how
		 * many bits per sample to use (request).
		 */
		struct	{
			int	DataBits;	/* 8/12/16 bit override requested */
		} tosvr_bits;

		/*
		 * Message to X client, from server, indicating
		 * the accepted number of bits per sample (response
		 * to request).
		 */
		struct	{
			int	DataBits;	/* Server says this # of bits in effect */
		} toclnt_bits;

		/*
		 * Message to X client, from server, indicating the
		 * current server settings.
		 */
		SVRINF	toclnt_settings;	/* Current server settings */

		/*
		 * Message from X client, to server, indicating the
		 * requested sampling rate to use (request).
		 */
		struct S_SvrSampRate {
			UInt32	SamplingRate;	/* In Hz */
		} tosvr_sampling_rate;

		/*
		 * Message from X client, to server, indicating the
		 * number of channels to use (request).
		 */
		struct	{
			Chan	Channels;	/* New channel mode: Stereo/Mono */
		} tosvr_chan;

                /*
                 * Message from X client, to server, indicating the sample to
                 * start playback at (request).
                 */
                struct  {
                        UInt32  StartSample;    /* New origin */
                } tosvr_start_sample;

		/*
		 * Message from X client, to server, indicating the
		 * channels to use, the sampling rate to use, and
		 * the data bits per channel to use (request).
		 */
		struct	{
			Chan	Channels;	/* Stereo or Mono */
			UInt32	SamplingRate;	/* Start recording at this rate */
			UInt16	DataBits;	/* 8/12/16 data bits */
		} tosvr_record;

		/*
		 * Message from X client to server, to set the server's
		 * debug mode global (cmdopt_x).
		 */
		struct	{
			char	bDebugMode;	/* True if debug mode set, else not debug mode */
		} tosvr_debug;

		/*
		 * Message from server to client, to convey a NON-fatal error that
		 * has occured in the server.
		 */
		struct	{
			int	Errno;		/* Error code */
			char	msg[512];	/* Error message text */
		} toclnt_errmsg;

                /*
                 * Message from server to client with playback status.
                 */
                struct {
                        int     CurrentSample;  /* Currently playing sample */
                        int     SamplesLeft;    /* Samples left */
                } toclnt_playstate;

                /*
                 * Message from server to client with record status.
                 */
                struct {
                        int     bytes_written;  /* Number of bytes sampled */
                        int     num_samples;    /* Samples taken */
                } toclnt_recstate;
	} u;					/* The message union of all message types */
} SVRMSG;

extern char *ProcTerm(int procstat);

extern int MsgCreate(void);
extern int MsgSend(int ipcid,SVRMSG *msg,int flags,long msgtype);
extern int MsgRecv(int ipcid,SVRMSG *msg,int flags,long msgtype);
extern int MsgClose(int ipcid);
extern char *msg_name(MSGTYP mtyp);
extern void msg_dump(const char *desc,MSGTYP mtyp);

#define MSGNO_CLNT	3L
#define MSGNO_SRVR	2L
#define MsgToClient(ipcid,msg,flags) MsgSend(ipcid,msg,flags,MSGNO_CLNT)
#define MsgToServer(ipcid,msg,flags) MsgSend(ipcid,msg,flags,MSGNO_SRVR)
#define MsgFromClient(ipcid,msg,flags) MsgRecv(ipcid,msg,flags,MSGNO_SRVR)
#define MsgFromServer(ipcid,msg,flags) MsgRecv(ipcid,msg,flags,MSGNO_CLNT)

extern int OpenDSPLocks(key_t LockIPCKey,int SemUndoFlag,ErrFunc erf);
extern int LockDSP(int ipc,int playrecx,ErrFunc erf,unsigned timeout_secs);
extern int UnlockDSP(int ipc,int playrecx,ErrFunc erf);

extern WAVFILE *WavOpenForRead(const char *Pathname,ErrFunc erf);
extern WAVFILE *WavOpenForWrite(const char *Pathname,OprMode m,UInt32 sample_rate,UInt16 bits,UInt32 samples,ErrFunc erf);
extern void WavReadOverrides(WAVFILE *wfile,WavPlayOpts *wavopts);
extern int WavClose(WAVFILE *wfile,ErrFunc erf);

extern DSPFILE *OpenDSP(WAVFILE *wfile,int omode,ErrFunc erf);
extern int PlayDSP(DSPFILE *dfile,WAVFILE *wfile,DSPPROC work_proc,ErrFunc erf);
extern int RecordDSP(DSPFILE *dfile,WAVFILE *wfile,UInt32 samples,DSPPROC work_proc,ErrFunc erf);
extern int CloseDSP(DSPFILE *dfile,ErrFunc erf);

extern int recplay(WavPlayOpts *wavopts,char **argv,ErrFunc erf);
/*/extern int wavplay(WavPlayOpts *wavopts,char *argv,ErrFunc erf);*/
extern int wavplay(char *argv,ErrFunc erf);
extern int wavrecd(WavPlayOpts *wavopts,char *Pathname,ErrFunc erf);

extern void RegisterSigHandlers(void);

/*/extern void play(char *Pathname);*/

#endif /* _wavplay_h_ */




/* $Source: /cvsroot/qgo/qgo/src/wavplay.h,v $ */
