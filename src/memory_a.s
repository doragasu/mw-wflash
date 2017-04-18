	.globl	memset
memset:
	move.l 12(%sp),%d0          | d0 = len
	jeq .L02

	move.l 4(%sp),%a0           | a0 = to
	move.b 11(%sp),%d1          | d1 = value

	cmpi.w #15,%d0              | len < 16 ?
	jhi .L30

	subq.w #1,%d0

.L01:
	move.b %d1,(%a0)+           | normal copy
	dbra %d0,.L01

.L02:
	rts

.L30:
	move.w %d2,-(%sp)

    move.b %d1,%d2
    lsl.w #8,%d1
    move.b %d2,%d1
    move.w %d1,%d2
    swap %d1
    move.w %d2,%d1              | d1 = value | (value << 8) | (value << 16) | (value << 24)

	move.w %a0,%d2
	btst #0,%d2                 | dst & 1 ?
	jeq .L34

	move.b %d1,(%a0)+           | align to word
	subq.w #1,%d0

.L34:
|	btst #1,%d2                 | dst & 2
|	jeq .L34b

|	move.w %d1,(%a0)+           | align to dword
|	subq.w #2,%d0

.L34b:
    move.w %d0,%d2
	lsr.w #5,%d2                | d2 = len >> 5
	jeq .L47

	subq.w #1,%d2

.L37:
	move.l %d1,(%a0)+           | fast set
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	dbra %d2,.L37

.L47:
    move.w %d0,%d2
	lsr.w #2,%d2
	andi.w #7,%d2               | d2 = (len >> 2) & 7
	jeq .L49

	subq.w #1,%d2

.L40:
	move.l %d1,(%a0)+
	dbra %d2,.L40

.L49:
	andi.w #3,%d0               | d0 = len & 3
	jeq .L29

	subq.w #1,%d0

.L43:
	move.b %d1,(%a0)+
	dbra %d0,.L43

.L29:
	move.w (%sp)+,%d2
	rts


	.globl	memsetU16
memsetU16:
	move.l 12(%sp),%d0          | d0 = len
	jeq .L53

	move.l 4(%sp),%a0           | a0 = to
	move.w 10(%sp),%d1          | d1 = value

	cmpi.w #15,%d0              | len < 16 ?
	jhi .L52

	subq.w #1,%d0

.L54:
	move.w %d1,(%a0)+           | normal copy
	dbra %d0,.L54

.L53:
	rts

.L52:
	move.w %d2,-(%sp)

    move.w %d1,%d2
    swap %d1
    move.w %d2,%d1              | d1 = value | (value << 16)

|	move.w %a0,%d2
|	btst #1,%d2                 | dst & 2 ?
|	jeq .L52b

|	move.w %d1,(%a0)+           | align to dword
|	subq.w #1,%d0

.L52b:
	btst #0,%d0                 | len & 1 ?
	jbeq .L56

	move.w %d1,(%a0)+           | align len on dword

.L56:
	lsr.w #1,%d0                | len >> 1

	move.w %d0,%d2
	lsr.w #3,%d2                | d2 = len >> 3
	jeq .L66

	subq.w #1,%d2

.L59:
	move.l %d1,(%a0)+           | fast set
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	dbra %d2,.L59

.L66:
	andi.w #7,%d0               | d0 = len & 7
	jeq .L51

	subq.w #1,%d0

.L62:
	move.l %d1,(%a0)+
	dbra %d0,.L62

.L51:
	move.w (%sp)+,%d2
	rts


	.globl	memsetU32
memsetU32:
	move.l 12(%sp),%d0          | d0 = len
	jeq .L73

	move.l 4(%sp),%a0           | a0 = to
	move.l 8(%sp),%d1           | d1 = value

	move.w %d2,-(%sp)

	move.w %d0,%d2
	lsr.w #3,%d2                | d2 = len >> 3
	jeq .L76

	subq.w #1,%d2

.L71:
	move.l %d1,(%a0)+           | fast set
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	move.l %d1,(%a0)+
	dbra %d2,.L71

.L76:
	andi.w #7,%d0               | d0 = len & 7
	jeq .L78

	subq.w #1,%d0

.L74:
	move.l %d1,(%a0)+
	dbra %d0,.L74

.L78:
	move.w (%sp)+,%d2

.L73:
	rts

