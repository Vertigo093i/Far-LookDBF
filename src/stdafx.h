// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


#if NTDDI_VERSION >= NTDDI_WIN2K
#ifdef RtlMoveMemory
#undef RtlMoveMemory
extern "C" NTSYSAPI VOID NTAPI RtlMoveMemory(__out_bcount_full(Length) VOID UNALIGNED *Destination, __in_bcount(Length) CONST VOID UNALIGNED *Source, __in SIZE_T Length);
#endif

#ifdef RtlCopyMemory
#undef RtlCopyMemory
#if defined(_M_AMD64)
extern "C" NTSYSAPI VOID NTAPI RtlCopyMemory(__out_bcount_full(Length) VOID UNALIGNED *Destination, __in_bcount(Length) CONST VOID UNALIGNED *Source, __in SIZE_T Length);
#else
#define RtlCopyMemory RtlMoveMemory
#endif
#endif

#ifdef RtlFillMemory
#undef RtlFillMemory
extern "C" NTSYSAPI VOID NTAPI RtlFillMemory(__out_bcount_full(Length) VOID UNALIGNED *Destination, __in SIZE_T Length, __in BYTE Fill);
#endif

#ifdef RtlZeroMemory
#undef RtlZeroMemory
extern "C" NTSYSAPI VOID NTAPI RtlZeroMemory(__out_bcount_full(Length) VOID UNALIGNED *Destination, __in SIZE_T Length);
#endif
#endif /* NTDDI_VERSION >= NTDDI_WIN2K */


#include <plugin.hpp>
