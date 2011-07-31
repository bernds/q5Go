/* $Header: /cvsroot/qgo/qgo/src/wavfile.h,v 1.1 2003/04/07 10:28:54 frosla Exp $
 * Copyright:	wavfile.h (c) Erik de Castro Lopo  erikd@zip.com.au
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
 * $Log: wavfile.h,v $
 * Revision 1.1  2003/04/07 10:28:54  frosla
 * 0.0.16b3: added real sound support for Unix!!!!
 *
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.2  1997/04/14 01:03:06  wwg
 * Fixed copyright, to reflect Erik's ownership of this file.
 *
 * Revision 1.1  1997/04/14 00:58:10  wwg
 * Initial revision
 */
#ifndef _wavfile_h
#define _wavfile_h "@(#)wavfile.h $Revision: 1.1 $"

#define WW_BADOUTPUTFILE	1
#define WW_BADWRITEHEADER	2

#define WR_BADALLOC		3
#define WR_BADSEEK		4
#define WR_BADRIFF		5
#define WR_BADWAVE		6
#define WR_BADFORMAT		7
#define WR_BADFORMATSIZE	8

#define WR_NOTPCMFORMAT		9
#define WR_NODATACHUNK		10
#define WR_BADFORMATDATA	11

extern int WaveWriteHeader(int wavefile,int channels,u_long samplerate,int sampbits,u_long samples,ErrFunc erf);
extern int WaveReadHeader(int wavefile,int *channels,u_long *samplerate,int *samplebits,u_long *samples,u_long *datastart,ErrFunc erf);

#if 0
extern char *WaveFileError(int error);
#endif

#endif /* _wavfile_h_ */

/* $Source: /cvsroot/qgo/qgo/src/wavfile.h,v $ */
