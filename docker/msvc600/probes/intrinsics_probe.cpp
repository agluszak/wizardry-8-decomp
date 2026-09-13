// VC6 narrow CRT intrinsic fingerprints.
//
// Compile under the first-party profile used by Wiz8.exe:
//   cl /nologo /c /DWIN32 /DNDEBUG /GX /GR- /MD /O2 /G6 /Fo intrinsics_probe.obj intrinsics_probe.cpp
//
// /O2 implies /Oi, and VC6 intrinsically expands exactly the narrow operations
// below (strlen, strcpy, strcat, strcmp, memcmp, memcpy, memset, _strset).
// Wide-character ops (wcslen/wcscpy/wcscmp/wcscat) are NOT intrinsics in VC6 -
// an expanded wide loop is authored code, a wide CRT call is not.
//
// When Ghidra shows an anonymous narrow scan/copy/compare loop in retail code,
// check it against these fingerprints before recovering an authored loop:
//
//   strlen:  or ecx,-1; xor eax,eax; repnz scasb; not ecx; dec ecx
//   strcpy:  <strlen over src>; sub edi,ecx; mov esi,edi; mov edi,dst;
//            mov edx,edi; shr ecx,2; rep movsd; and ecx,3; rep movsb
//   strcat:  <strlen over src>; <repnz scasb over dst>; dec edi;
//            shr ecx,2; rep movsd; and ecx,3; rep movsb
//   strcmp:  2-byte-at-a-time walk: mov dl,[a]; mov bl,[b]; cmp/jne;
//            test cl,cl; je; mov dl,[a+1]; mov bl,[b+1]; ...; add a,2;
//            add b,2; jne head; result via sbb eax,eax; sbb eax,-1
//   memcmp:  repz cmpsb; then je / sbb eax,eax; sbb eax,-1
//   memcpy:  shr ecx,2; rep movsd; mov ecx,n; and ecx,3; rep movsb
//   memset:  byte splat (bl,bh -> shl 16 -> mov ax,bx); shr ecx,2;
//            rep stosd; and ecx,3; rep stosb
//   _strset: <repnz scasb>; not ecx; dec ecx; <byte splat>; rep stosd/stosb

#include <string.h>

unsigned long use_strlen(const char *s) { return strlen(s); }
char *use_strcpy(char *d, const char *s) { return strcpy(d, s); }
char *use_strcat(char *d, const char *s) { return strcat(d, s); }
int use_strcmp(const char *a, const char *b) { return strcmp(a, b); }
int use_memcmp(const void *a, const void *b, unsigned long n) { return memcmp(a, b, n); }
void *use_memcpy(void *d, const void *s, unsigned long n) { return memcpy(d, s, n); }
void *use_memset(void *d, int c, unsigned long n) { return memset(d, c, n); }
char *use_strset(char *s, int c) { return _strset(s, c); }
