;---------------------------------------------------------------------
; AUD.S
; Interface to the audio driver.
; Author:  Chris Smith
; Modifications:
;   07/05/90  Corinna Abdul
;             Changed to use .MODEL directive and simplified segments.
;             Microsoft MASM 5.1
;---------------------------------------------------------------------

;KAWA WAS HERE
;-------------
; > General cleanup -- some tabs and between-proc spacing

include	macro.i
include	audio.i
include	pmachine.i
include	intrpt.i

.CODE

inStat		byte	0


AudioDrv	proc	uses si di, function:word, qualifer:word
;Call the audio driver.
	mov	bx, audioDrv
	.if	bx
		mov	si, qualifer
		mov	ax, function
		call	dword ptr [bx]
		.endif
	ret
AudioDrv	endp


AudioServer	proc
;The timer interrupt server for audio.
	mov	al, 1
	xchg	al, cs:inStat
	.if	!al
		mov	bx, audioDrv
		.if	bx
			mov	ax, A_STAT
			call	dword ptr [bx]
		.endif
		mov	bx, cdaudioDrv
		.if	bx
			mov	ax, A_STAT
			call	dword ptr [bx]
		.endif
		dec	cs:inStat
		.endif
	ret
AudioServer	endp


CDAudioDrv	proc uses si di, function:word, qualifer:word
;Call the cdrom audio driver.
	mov	bx, cdaudioDrv
	.if	bx
		mov	si, qualifer
		mov	ax, function
		call	dword ptr [bx]
		.endif
	ret
CDAudioDrv	endp

end
