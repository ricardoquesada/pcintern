;  QSCAN3.ASM
;  David Stafford

        COMMENT $
 
How it works
------------
The idea is to go through the buffer fetching each letter-pair (words
rather than bytes).  The carry flag indicates whether we are
currently in a (text)word or not.  The letter-pair fetched from the
buffer is converted to a 16-bit address by shifting it left one bit
(losing the high bit of the second character) and putting the carry
flag in the low bit.  The high bit of the count register is set to
1.  Then the count register is added to the byte found at the given
address in a large (64K, naturally) table.  The byte at the given
address will contain a 1 in the high bit if the last character of the
letter-pair is a word-letter (alphanumeric or apostrophe).  This will
set the carry flag since the high bit of the count register is also a
1. The low bit of the byte found at the given address will be one if
the second character of the previous letter-pair was a word-letter
and the first character of this letter-pair is not a word-letter. It
will also be 1 if the first character of this letter-pair is a
word-letter but the second character is not.  This process is
repeated.  Finally, the carry flag is saved to indicate the final
in-a-word/not-in-a-word status.  The count register is masked to
remove the high bit and the count of words remains in the count
register.
 
Sound complicated?  You're right!  But it's fast!
 
The beauty of this method is that no jumps are required, the
operations are fast, it requires only one table and the process can
be repeated (unrolled) many times.  QSCAN3 can read 256 bytes without
jumping.

        COMMEND $

                .model small
                .code
 
Test1           macro   x,y                     ;9 or 10 bytes
Addr&x:         mov     di,[bp+y]               ;3 or 4 bytes
                adc     di,di
                or      ax,si
                add     al,[di]
                endm
 
Test2           macro   x,y                     ;7 or 8 bytes
Addr&x:         mov     di,[bp+y]               ;3 or 4 bytes
                adc     di,di
                add     ah,[di]
                endm
 
Scan            =       128                     ;scan 256 bytes at a time
 
Buffer          =       4                       ;parms
BufferLength    =       6
CharFlag        =       8
WordCount       =       10
 
                public _ScanBuffer
_ScanBuffer     proc near
                push    bp
                mov     bp,sp
                push    si
                push    di
 
                xor     cx,cx
                mov     si,[bp+Buffer]          ;si = text buffer
                mov     ax,[bp+BufferLength]    ;dx = length in bytes
                shr     ax,1                    ;dx = length in words
                jnz     NormalBuf
OneByteBuf:
                mov     ax,seg WordTable
                mov     es,ax
 
                mov     di,[bp+CharFlag]
                mov     bh,[di]                 ;bh = old CharFlag
                mov     bl,[si]                 ;bl = character
                add     bh,'A'-1                ;make bh into character
                add     bx,bx                   ;prepare to index
                mov     al,es:[bx]
                cbw                             ;get hi bit in ah (then bh)
                shr     al,1                    ;get low bit
                adc     cx,cx                   ;cx = 0 or 1
                xchg    ax,bx
                jmp     CleanUp
NormalBuf:
                push    bp                      ;(1)
                pushf                           ;(2)
 
                cwd                             ;dx = 0
                mov     cl,Scan
                div     cx
                or      dx,dx                   ;remainder?
                jz      StartAtTheTop           ;nope, do the whole banana
 
                sub     cx,dx
                sub     si,cx                   ;adjust buf pointer
                sub     si,cx
                inc     ax                      ;adjust for partial read
 
StartAtTheTop:  mov     bx,dx                   ;get index for start...
                shl     bx,1
                mov     di,LoopEntry[bx]        ;...address in di
 
                xchg    dx,ax                   ;dx is the loop counter
                xor     cx,cx                   ;total word count
 
                mov     bx,[bp+CharFlag]
                mov     bl,[bx]                 ;bl = old CharFlag
 
                mov     bp,seg WordTable
                mov     ds,bp
 
                mov     bp,si                   ;scan buffer with bp
                mov     si,8080h                ;hi bits
                mov     ax,si                   ;init local word counter
 
                shr     bl,1                    ;carry = old CharFlag
                jmp     di
 
                align   2
Top:            add     bx,bx                   ;restore carry
n               =       0
                rept    Scan/2
                Test1   %n,%n*2
                Test2   %n+1,%n*2+2
n               =       n+2
                endm
EndCount:
                sbb     bx,bx                   ;save carry
 
if              Scan ge 128                     ;because al+ah may equal 128!
                or      ax,si
                add     al,ah
                mov     ah,0
else
                add     al,ah
                and     ax,7fh                  ;mask
endif
                add     cx,ax                   ;update word count
                mov     ax,si
                add     bp,Scan*2
 
                dec     dx                      ;any left?
                jng     Quit
                jmp     Top
 
Quit:           popf                            ;(2) even or odd buffer?
                jnc     ItsEven
 
                clc
                Test1   Odd,-1
                sbb     bx,bx                   ;save carry
                shr     ax,1
                adc     cx,0
ItsEven:
                push    ss                      ;restore ds
                pop     ds
                pop     bp                      ;(1)
CleanUp:
                mov     si,[bp+WordCount]
                add     [si],cx
                adc     word ptr [si+2],0
 
                and     bh,1                    ;save only the carry flag
                mov     si,[bp+CharFlag]
                mov     [si],bh
 
                pop     di
                pop     si
                pop     bp
                ret
_ScanBuffer     endp
 
                .data
Address         macro   X
                dw      Addr&X
                endm
 
LoopEntry       label word
n               =       Scan
                REPT Scan
                Address %n MOD Scan
n               =       n - 1
                ENDM
 
                .fardata WordTable
include         qscan3.inc                      ;built by MAKETAB
                end

