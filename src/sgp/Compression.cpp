/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Add matching markers for retained SGP functions and globals.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "MemMan.h"
#include "debug.h"
#include "zlib.h"


// mem allocation functions for ZLIB's purposes

// FUNCTION: WIZ8 0x00415820
voidpf ZAlloc( voidpf opaque, uInt items, uInt size )
{
  return( MemAlloc( items * size ) );
}

// FUNCTION: WIZ8 0x00415840
void ZFree( voidpf opaque, voidpf address )
{
	MemFree( address );
}

// FUNCTION: WIZ8 0x00415850
PTR DecompressInit( BYTE * pCompressedData, UINT32 uiDataSize )
{
	z_stream *	pZStream;
	int					iZRetCode;

	// allocate memory for the z_stream struct
	pZStream = (z_stream *)MemAlloc( sizeof( z_stream ) );
	if( pZStream == NULL )
	{ // out of memory!
		return( NULL );
	}

	// initial defines
	pZStream->zalloc = ZAlloc;
	pZStream->zfree = ZFree;
	pZStream->opaque = NULL;

	// call the ZLIB init routine
	iZRetCode = inflateInit( pZStream );
	if( iZRetCode != Z_OK )
	{ // ZLIB init error!
		MemFree( pZStream );
		return( NULL );
	}

	// set up our parameters
	pZStream->next_in = pCompressedData;
	pZStream->avail_in = uiDataSize;
	return( (PTR) pZStream );
}

// FUNCTION: WIZ8 0x004158b0
UINT32 Decompress( PTR pDecompPtr, BYTE * pBuffer, UINT32 uiBufferLen )
{
	int					iZRetCode;
	z_stream *	pZStream = (z_stream *) pDecompPtr;

	// these assertions is in here to ensure that we get passed a proper z_stream pointer
	Assert( pZStream != NULL );
	Assert( pZStream->zalloc == ZAlloc );

	if (pZStream->avail_in == 0)
	{ // There is nothing left to decompress!
		return( 0 );
	}

	// set up the z_stream with our parameters
	pZStream->next_out = pBuffer;
	pZStream->avail_out = uiBufferLen;

	// decompress!
	iZRetCode = inflate( pZStream, Z_PARTIAL_FLUSH );
	Assert( iZRetCode == Z_OK || iZRetCode == Z_STREAM_END );

	return( uiBufferLen - pZStream->avail_out );
}

// FUNCTION: WIZ8 0x004158f0
void DecompressFini( PTR pDecompPtr )
{
	z_stream *	pZStream = (z_stream *) pDecompPtr;

	// these assertions is in here to ensure that we get passed a proper z_stream pointer
	Assert( pZStream != NULL );
	Assert( pZStream->zalloc == ZAlloc );

	inflateEnd( pZStream );
	MemFree( pZStream );
}

