#ifndef PORT_KOREAN_H
#define PORT_KOREAN_H

/*
 * Korean fan-translation support ("Fushigi no Boushi (K)" ver1.0/1.1, built on the
 * JP BZMJ ROM).
 *
 * The translation patch replaces ARM code in the ROM (glyph-bank lookup,
 * control-code decoding, a Hangul name-entry keyboard, name rendering). The
 * native port never executes ROM code, so those behaviours are re-implemented
 * here in C. All tables (glyph banks, syllable list, jamo composition tables,
 * keyboard tilemap/tiles) are read from the user's own patched ROM at runtime;
 * nothing from the patch is embedded in the port.
 */

#include "global.h"

#define KR_NAME_BYTES 8 /* the patch uses name[6] plus the 2 filler bytes after it */

typedef struct {
    u8 pos; /* byte offset in the name buffer */
    u8 len; /* 1 = raw single-byte char, 2 = Hangul/jamo pair */
    s32 code; /* -1 for raw chars, otherwise Korean glyph code (0..0x955) */
} PortKrNameEnt;

void Port_Korean_Detect(void);
int Port_IsKoreanPatch(void);

/* ROM offsets of the patch's relocated tables (valid only when patched). */
u32 Port_Kr_GlyphBankTableOffset(void);
u32 Port_Kr_StylizedTableOffset(void);

int Port_Kr_DecodeName(const u8* name, PortKrNameEnt* out);
/* Converts a save-file name into a text-engine string (control codes 0x0B..0x0F). */
void Port_Kr_FormatName(const u8* name, u8* out, u32 outSize);
/* Name code -> glyph character code for direct sub_0805F7DC() drawing. */
u32 Port_Kr_NameCodeToChar(s32 code);

/* Hangul composition helpers (tables come from the ROM). */
void Port_Kr_Decompose(u32 code, u32* l, u32* v, u32* t);
s32 Port_Kr_FindSyllable(u32 value);
u32 Port_Kr_FinalSplit(u32 t);      /* (keptFinal << 5) | movedInitial */
u32 Port_Kr_InitialToFinal(u32 l);  /* 0 = cannot be a final */
const u8* Port_Kr_DoubleFinalTable(void);   /* 11 x {final, initial, result} */
const u8* Port_Kr_CompoundVowelTable(void); /* 7 x {vowel, vowel, result} */
const u8* Port_Kr_RomPtr(u32 offset);

/* Name-entry keyboard (file select). */
u32 Port_Kr_NameEntryAction(u32 action, u32 page);
void Port_Kr_ShowKeyboardPage(void);
void Port_Kr_UpdateNameCursor(void);

#endif
