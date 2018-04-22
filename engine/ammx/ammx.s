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