    xdef _fill_flat_texel
    xdef _pmul88_3byte

_pmul88_3byte:
    move.l 8(a7),a0
    LOAD (a0),e1
    move.l 12(a7),a0
    move.l a0,a1
    LOAD (a0),e2
    moveq #3,d0
    vperm #$48494a4b,d0,e1,e3
    vperm #$48494a4b,d0,e2,e4
    pmul88 e4,e3,e0
    vperm #$9bdf0000,d0,e0,e5
    move.l 4(a7),a0
    storec e5,d0,(a0)
    move.l a0,d0
	rts


; 
; 4(a7) uint8_t* p
; 8(a7) short d
; 12(a7) int u1
; 16(a7) int v1
; 20(a7) int w1
; 24(a7) int au
; 28(a7) int av
; 32(a7) int aw
; 36(a7) uint32_t texMaskU
; 40(a7) uint32_t texMaskV
; 44(a7) uint32_t texSizeU
; 48(a7) uint32_t* texPixels
; 52(a7) uint8_t* color
;
; additional offset because of saved registers: 24
;
_fill_flat_texel:
    movem.l d2-d7,-(a7)
    move.l 28(a7),a1    ; p in a1
    move.l 36(a7),d5    ; u1 in d5
    move.l 40(a7),d6    ; v1 in d6
    move.l 44(a7),d7    ; w1 in d7
    load #3,e4
    move.l 76(sp),a0
    load (a0),e0
    vperm #$48494a4b,e4,e0,e1   ; c in e1
    move.l 32(sp),d0    ; d0 is our loopvar
    bra .loopend
.loopstart
    ; calculate z
    move.l #$10000000,d1
    move.l d7,d2
    asr.l #8,d2
    divs.l d2,d1    ; z in d1
    moveq #24,d4   ; needed for shift right
    ; calculate tu
    move.l d5,d2
    muls.l d1,d2
    lsr.l d4,d2
    and.l 60(a7),d2     ; tu in d2
    ; calculate tv
    move.l d6,d3
    muls.l d1,d3
    lsr.l d4,d3
    and.l 64(a7),d3     ; tv in d3
    ; no idea why this is uint32_t :S
    move.l 68(a7),d4    ; texSizeU
    lsl.l d4,d3
    add.l d2,d3         ; texpixel offset in d3
    lsl.l #2,d3         ; it is a 32bit field => multiply by 4
    move.l 72(a7),d1
    add.l d3,d1
    move.l d1,a0        ; t
    ; calculate p[]
    load (a0),e0
    vperm #$48494a4b,e4,e0,e2
    pmul88 e1,e2,e0
    vperm #$9bdf0000,e4,e0,e3
    storec e3,e4,(a1)
    ; advance p
    addq.l #4,a1
    add.l 48(a7),d5
    add.l 52(a7),d6
    add.l 56(a7),d7
.loopend:
    dbra d0,.loopstart
.end:
    movem.l (a7)+,d2-d7
    rts