/*
 * Korean fan-translation support. See port_korean.h.
 *
 * Behaviour mirrors the patch's ARM routines (reverse-engineered from the
 * patched ROM); the data they use is read from the loaded ROM.
 */
#include "port_korean.h"

#include "port_config.h"
#include "port_rom.h"

#include <stdio.h>
#include <string.h>

/* JP-ROM code literals the patch redirects. */
#define KR_LIT_GLYPH_BANKS 0x05F104u /* gUnk_08109248 literal in sub_0805F25C */
#define KR_LIT_STYLIZED 0x05F804u    /* gUnk_081092D4 literal in sub_0805F9A0 */
#define KR_GLYPH_BANKS_PATCHED 0x08DC9F00u
#define KR_WIDE_CHECK_SITE 0x05F204u /* "cmp r0, #4" -> "cmp r0, #2" */

/* Tables inside the patch's free-space block. */
#define KR_TBL_FINAL_SPLIT 0xE52360u    /* u16[28] */
#define KR_TBL_SYLLABLES 0xE52398u      /* u16[2350], sorted Unicode syllable offsets */
#define KR_TBL_INITIAL_TO_FINAL 0xE535F4u /* u8[19] */
#define KR_TBL_DOUBLE_FINAL 0xE53607u   /* 11 x 3 bytes */
#define KR_TBL_COMPOUND_VOWEL 0xE53628u /* 7 x 3 bytes */

#define KR_SYLLABLE_COUNT 2350u
#define KR_JAMO_BASE 0x92Eu /* 19 initials, then 21 vowels at 0x941 */

static int sKorean;

static u32 Rd32(u32 off) {
    if (gRomData == NULL || off + 4 > gRomSize)
        return 0;
    return (u32)gRomData[off] | ((u32)gRomData[off + 1] << 8) | ((u32)gRomData[off + 2] << 16) |
           ((u32)gRomData[off + 3] << 24);
}

static u32 Rd16(u32 off) {
    if (gRomData == NULL || off + 2 > gRomSize)
        return 0;
    return (u32)gRomData[off] | ((u32)gRomData[off + 1] << 8);
}

void Port_Korean_Detect(void) {
    sKorean = 0;
    if (gRomRegion != ROM_REGION_JP || gRomSize < 0x1000000u)
        return;
    if (Rd32(KR_LIT_GLYPH_BANKS) == KR_GLYPH_BANKS_PATCHED && Rd16(KR_WIDE_CHECK_SITE) == 0x2802u) {
        sKorean = 1;
        fprintf(stderr, "Korean translation patch detected (BZMJ + Hangul glyph banks).\n");
    }
}

int Port_IsKoreanPatch(void) {
    return sKorean;
}

u32 Port_Kr_GlyphBankTableOffset(void) {
    return Rd32(KR_LIT_GLYPH_BANKS) - 0x08000000u;
}

u32 Port_Kr_StylizedTableOffset(void) {
    return Rd32(KR_LIT_STYLIZED) - 0x08000000u;
}

const u8* Port_Kr_RomPtr(u32 offset) {
    if (gRomData == NULL || offset >= gRomSize)
        return NULL;
    return &gRomData[offset];
}

int Port_Kr_DecodeName(const u8* name, PortKrNameEnt* out) {
    int count = 0;
    u32 i = 0;

    while (i < KR_NAME_BYTES) {
        u32 b = name[i];
        if (b == 0)
            break;
        if (b <= 0x0F) {
            if (i == KR_NAME_BYTES - 1)
                break;
            out[count].pos = (u8)i;
            out[count].len = 2;
            out[count].code = (s32)(((b - 1) << 8) | name[i + 1]);
            i += 2;
        } else {
            out[count].pos = (u8)i;
            out[count].len = 1;
            out[count].code = -1;
            i += 1;
        }
        count++;
    }
    return count;
}

void Port_Kr_FormatName(const u8* name, u8* out, u32 outSize) {
    PortKrNameEnt ent[KR_NAME_BYTES];
    int n = Port_Kr_DecodeName(name, ent);
    u32 o = 0;

    for (int i = 0; i < n && o + 4 < outSize; i++) {
        s32 code = ent[i].code;
        if (code < 0) {
            out[o++] = name[ent[i].pos];
        } else if ((u32)code >= KR_JAMO_BASE) {
            out[o++] = 0x0F;
            out[o++] = 0x46;
            out[o++] = (u8)(code - 0x2E);
        } else if ((code >> 10) == 0) {
            out[o++] = (u8)((code >> 8) + 0x0B);
            out[o++] = (u8)code;
        } else {
            out[o++] = 0x0F;
            out[o++] = (u8)((((u32)code + 0xFC00u) >> 8) + 0x40);
            out[o++] = (u8)code;
        }
    }
    out[o] = 0;
}

u32 Port_Kr_NameCodeToChar(s32 code) {
    if ((u32)code >= KR_JAMO_BASE)
        return ((u32)code - KR_JAMO_BASE) | 0xF00u;
    if ((u32)code < 0x400u)
        return (u32)code | 0x400u;
    return ((u32)code & 0xF00u) + ((u32)code & 0xFFu) + 0x500u;
}

void Port_Kr_Decompose(u32 code, u32* l, u32* v, u32* t) {
    u32 s = Rd16(KR_TBL_SYLLABLES + code * 2);
    *l = s / 588;
    s -= *l * 588;
    *v = s / 28;
    *t = s - *v * 28;
}

s32 Port_Kr_FindSyllable(u32 value) {
    s32 lo = 0;
    s32 hi = (s32)KR_SYLLABLE_COUNT - 1;
    while (lo <= hi) {
        s32 mid = (lo + hi) >> 1;
        u32 s = Rd16(KR_TBL_SYLLABLES + (u32)mid * 2);
        if (value == s)
            return mid;
        if (value > s)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}

u32 Port_Kr_FinalSplit(u32 t) {
    return Rd16(KR_TBL_FINAL_SPLIT + t * 2);
}

u32 Port_Kr_InitialToFinal(u32 l) {
    const u8* p = Port_Kr_RomPtr(KR_TBL_INITIAL_TO_FINAL + l);
    return p ? *p : 0;
}

const u8* Port_Kr_DoubleFinalTable(void) {
    return Port_Kr_RomPtr(KR_TBL_DOUBLE_FINAL);
}

const u8* Port_Kr_CompoundVowelTable(void) {
    return Port_Kr_RomPtr(KR_TBL_COMPOUND_VOWEL);
}
