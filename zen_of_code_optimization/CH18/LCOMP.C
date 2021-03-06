// LCOMP.C
//
// Life compiler, ver 1.3
//
// David Stafford

#include <stdio.h>
#include <stdlib.h>
#include "life.h"


#define LIST_LIMIT (46 * 138)   // when we need to use es:

int Old, New, Edge, Label;
char Buf[ 20 ];


void Next1( void )
  {
  char *Seg = "";

  if( WIDTH * HEIGHT > LIST_LIMIT )  Seg = "es:";

  printf( "mov  bp,%s[si]\n", Seg );
  printf( "add  si,2\n" );
  printf( "mov  dh,[bp+1]\n" );
  printf( "and  dh,0FEh\n" );
  printf( "jmp  dx\n" );
  }


void Next2( void )
  {
  printf( "mov  bp,es:[si]\n" );
  printf( "add  si,2\n" );
  printf( "mov  dh,[bp+1]\n" );
  printf( "or   dh,1\n" );
  printf( "jmp  dx\n" );
  }


void BuildMaps( void )
  {
  unsigned short i, j, Size, x = 0, y, N1, N2, N3, C1, C2, C3;

  printf( "_DATA segment 'DATA'\nalign 2\n" );
  printf( "public _CellMap\n" );
  printf( "_CellMap label word\n" );

  for( j = 0; j < HEIGHT; j++ )
    {
    for( i = 0; i < WIDTH; i++ )
      {
      if( i == 0 || i == WIDTH-1 || j == 0 || j == HEIGHT-1 )
        {
        printf( "dw 8000h\n" );
        }
      else
        {
        printf( "dw 0\n" );
        }
      }
    }

  printf( "ChangeCell dw 0\n" );
  printf( "_RowColMap label word\n" );

  for( j = 0; j < HEIGHT; j++ )
    {
    for( i = 0; i < WIDTH; i++ )
      {
      printf( "dw 0%02x%02xh\n", j, i * 3 );
      }
    }

  if( WIDTH * HEIGHT > LIST_LIMIT )
    {
    printf( "Change1 dw offset _CHANGE:_ChangeList1\n" );
    printf( "Change2 dw offset _CHANGE:_ChangeList2\n" );
    printf( "ends\n\n" );
    printf( "_CHANGE segment para public 'FAR_DATA'\n" );
    }
  else
    {
    printf( "Change1 dw offset DGROUP:_ChangeList1\n" );
    printf( "Change2 dw offset DGROUP:_ChangeList2\n" );
    }

  Size = WIDTH * HEIGHT + 1;

  printf( "public _ChangeList1\n_ChangeList1 label word\n" );
  printf( "dw %d dup (offset DGROUP:ChangeCell)\n", Size );
  printf( "public _ChangeList2\n_ChangeList2 label word\n" );
  printf( "dw %d dup (offset DGROUP:ChangeCell)\n", Size );
  printf( "ends\n\n" );

  printf( "_LDMAP segment para public 'FAR_DATA'\n" );

  do
    {
    // Current cell states
    C1 = (x & 0x0800) >> 11;
    C2 = (x & 0x0400) >> 10;
    C3 = (x & 0x0200) >> 9;

    // Neighbor counts
    N1 = (x & 0x01C0) >> 6;
    N2 = (x & 0x0038) >> 3;
    N3 = (x & 0x0007);

    y = x & 0x8FFF;  // Preserve all but the next generation states

    if(  C1 && ((N1 + C2 == 2) || (N1 + C2 == 3)) )
      {
      y |= 0x4000;
      }

    if( !C1 &&  (N1 + C2 == 3) )
      {
      y |= 0x4000;
      }

    if(  C2 && ((N2 + C1 + C3 == 2) || (N2 + C1 + C3 == 3)) )
      {
      y |= 0x2000;
      }

    if( !C2 &&  (N2 + C1 + C3 == 3) )
      {
      y |= 0x2000;
      }

    if(  C3 && ((N3 + C2 == 2) || (N3 + C2 == 3)) )
      {
      y |= 0x1000;
      }

    if( !C3 &&  (N3 + C2 == 3) )
      {
      y |= 0x1000;
      }

    printf( "db 0%02xh\n", y >> 8 );
    }
  while( ++x != 0 );

  printf( "ends\n\n" );
  }


void GetUpAndDown( void )
  {
  printf( "mov  ax,[bp+_RowColMap-_CellMap]\n" );
  printf( "or   ah,ah\n" );
  printf( "mov  dx,%d\n", DOWN );
  printf( "mov  cx,%d\n", WRAPUP );
  printf( "jz   short D%d\n", Label );
  printf( "cmp  ah,%d\n", HEIGHT - 1 );
  printf( "mov  cx,%d\n", UP );
  printf( "jb   short D%d\n", Label );
  printf( "mov  dx,%d\n", WRAPDOWN );
  printf( "D%d:\n", Label );
  }


void FirstPass( void )
  {
  char *Op;
  unsigned short UpDown = 0;

  printf( "org 0%02x00h\n", (Edge << 7) + (New << 4) + (Old << 1) );

  // reset cell
  printf( "xor  byte ptr [bp+1],0%02xh\n", (New ^ Old) << 1 );

  // get the screen address and update the display
  #ifndef NODRAW
  printf( "mov  al,160\n" );
  printf( "mov  bx,[bp+_RowColMap-_CellMap]\n" );
  printf( "mul  bh\n" );
  printf( "add  ax,ax\n" );
  printf( "mov  bh,0\n" );
  printf( "add  bx,ax\n" );    // bx = screen offset

  if( ((New ^ Old) & 6) == 6 )
    {
    printf( "mov  word ptr fs:[bx],0%02x%02xh\n",
            (New & 2) ? 15 : 0,
            (New & 4) ? 15 : 0 );

    if( (New ^ Old) & 1 )
      {
      printf( "mov  byte ptr fs:[bx+2],%s\n",
              (New & 1) ? "15" : "dl" );
      }
    }
  else
    {
    if( ((New ^ Old) & 3) == 3 )
      {
      printf( "mov  word ptr fs:[bx+1],0%02x%02xh\n",
              (New & 1) ? 15 : 0,
              (New & 2) ? 15 : 0 );
      }
    else
      {
      if( (New ^ Old) & 2 )
        {
        printf( "mov  byte ptr fs:[bx+1],%s\n",
                (New & 2) ? "15" : "dl" );
        }

      if( (New ^ Old) & 1 )
        {
        printf( "mov  byte ptr fs:[bx+2],%s\n",
                (New & 1) ? "15" : "dl" );
        }
      }

    if( (New ^ Old) & 4 )
      {
      printf( "mov  byte ptr fs:[bx],%s\n",
              (New & 4) ? "15" : "dl" );
      }
    }
  #endif

  if( (New ^ Old) & 4 )  UpDown += (New & 4) ? 0x48 : -0x48;
  if( (New ^ Old) & 2 )  UpDown += (New & 2) ? 0x49 : -0x49;
  if( (New ^ Old) & 1 )  UpDown += (New & 1) ? 0x09 : -0x09;

  if( Edge )
    {
    GetUpAndDown();  // ah = row, al = col, cx = up, dx = down

    if( (New ^ Old) & 4 )
      {
      printf( "mov  di,%d\n", WRAPLEFT );      // di = left
      printf( "cmp  al,0\n" );
      printf( "je   short L%d\n", Label );
      printf( "mov  di,%d\n", LEFT );
      printf( "L%d:\n", Label );

      if( New & 4 )  Op = "inc";
      else           Op = "dec";

      printf( "%s  word ptr [bp+di]\n", Op );
      printf( "add  di,cx\n" );
      printf( "%s  word ptr [bp+di]\n", Op );
      printf( "sub  di,cx\n" );
      printf( "add  di,dx\n" );
      printf( "%s  word ptr [bp+di]\n", Op );
      }

    if( (New ^ Old) & 1 )
      {
      printf( "mov  di,%d\n", WRAPRIGHT );      // di = right
      printf( "cmp  al,%d\n", (WIDTH - 1) * 3 );
      printf( "je   short R%d\n", Label );
      printf( "mov  di,%d\n", RIGHT );
      printf( "R%d:\n", Label );

      if( New & 1 )  Op = "add";
      else           Op = "sub";

      printf( "%s   word ptr [bp+di],40h\n", Op );
      printf( "add  di,cx\n" );
      printf( "%s   word ptr [bp+di],40h\n", Op );
      printf( "sub  di,cx\n" );
      printf( "add  di,dx\n" );
      printf( "%s   word ptr [bp+di],40h\n", Op );
      }

    printf( "mov  di,cx\n" );
    printf( "add  word ptr [bp+di],%d\n", UpDown );
    printf( "mov  di,dx\n" );
    printf( "add  word ptr [bp+di],%d\n", UpDown );

    printf( "mov  dl,0\n" );
    }
  else
    {
    if( (New ^ Old) & 4 )
      {
      if( New & 4 )  Op = "inc";
      else           Op = "dec";

      printf( "%s  byte ptr [bp+%d]\n", Op, LEFT );
      printf( "%s  byte ptr [bp+%d]\n", Op, UPPERLEFT );
      printf( "%s  byte ptr [bp+%d]\n", Op, LOWERLEFT );
      }

    if( (New ^ Old) & 1 )
      {
      if( New & 1 )  Op = "add";
      else           Op = "sub";

      printf( "%s  word ptr [bp+%d],40h\n", Op, RIGHT );
      printf( "%s  word ptr [bp+%d],40h\n", Op, UPPERRIGHT );
      printf( "%s  word ptr [bp+%d],40h\n", Op, LOWERRIGHT );
      }

    if( abs( UpDown ) > 1 )
      {
      printf( "add  word ptr [bp+%d],%d\n", UP, UpDown );
      printf( "add  word ptr [bp+%d],%d\n", DOWN, UpDown );
      }
    else
      {
      if( UpDown == 1 )  Op = "inc";
      else               Op = "dec";

      printf( "%s  byte ptr [bp+%d]\n", Op, UP   );
      printf( "%s  byte ptr [bp+%d]\n", Op, DOWN );
      }
    }

  Next1();
  }


void Test( char *Offset, char *Str )
  {
  printf( "mov  bx,[bp+%s]\n", Offset );
  printf( "cmp  bh,[bx]\n" );
  printf( "jnz  short FIX_%s%d\n", Str, Label );
  printf( "%s%d:\n", Str, Label );
  }


void Fix( char *Offset, char *Str, int JumpBack )
  {
  printf( "FIX_%s%d:\n", Str, Label );
  printf( "mov  bh,[bx]\n" );
  printf( "mov  [bp+%s],bx\n", Offset );

  if( *Offset != '0' )  printf( "lea  ax,[bp+%s]\n", Offset );
  else                  printf( "mov  ax,bp\n" );

  printf( "stosw\n" );

  if( JumpBack )  printf( "jmp  short %s%d\n", Str, Label );
  }


void SecondPass( void )
  {
  printf( "org 0%02x00h\n",
          (Edge << 7) + (New << 4) + (Old << 1) + 1 );

  if( Edge )
    {
    // finished with second pass
    if( New == 7 && Old == 0 )
      {
      printf( "cmp  bp,offset DGROUP:ChangeCell\n" );
      printf( "jne  short NotEnd\n" );
      printf( "mov  word ptr es:[di],offset DGROUP:ChangeCell\n" );
      printf( "pop  di si bp ds\n" );
      printf( "mov  ChangeCell,0\n" );
      printf( "retf\n" );
      printf( "NotEnd:\n" );
      }

    GetUpAndDown();  // ah = row, al = col, cx = up, dx = down

    printf( "push si\n" );
    printf( "mov  si,%d\n", WRAPLEFT );    // si = left
    printf( "cmp  al,0\n" );
    printf( "je   short L%d\n", Label );
    printf( "mov  si,%d\n", LEFT );
    printf( "L%d:\n", Label );

    Test( "si", "LEFT" );
    printf( "add  si,cx\n" );
    Test( "si", "UPPERLEFT" );
    printf( "sub  si,cx\n" );
    printf( "add  si,dx\n" );
    Test( "si", "LOWERLEFT" );

    printf( "mov  si,cx\n" );
    Test( "si", "UP" );
    printf( "mov  si,dx\n" );
    Test( "si", "DOWN" );

    printf( "cmp  byte ptr [bp+_RowColMap-_CellMap],%d\n",
            (WIDTH - 1) * 3 );

    printf( "mov  si,%d\n", WRAPRIGHT );    // si = right
    printf( "je   short R%d\n", Label );
    printf( "mov  si,%d\n", RIGHT );
    printf( "R%d:\n", Label );

    Test( "si", "RIGHT" );
    printf( "add  si,cx\n" );
    Test( "si", "UPPERRIGHT" );
    printf( "sub  si,cx\n" );
    printf( "add  si,dx\n" );
    Test( "si", "LOWERRIGHT" );
    }
  else
    {
    Test( itoa( LEFT, Buf, 10 ), "LEFT" );
    Test( itoa( UPPERLEFT, Buf, 10 ), "UPPERLEFT" );
    Test( itoa( LOWERLEFT, Buf, 10 ), "LOWERLEFT" );
    Test( itoa( UP, Buf, 10 ), "UP" );
    Test( itoa( DOWN, Buf, 10 ), "DOWN" );
    Test( itoa( RIGHT, Buf, 10 ), "RIGHT" );
    Test( itoa( UPPERRIGHT, Buf, 10 ), "UPPERRIGHT" );
    Test( itoa( LOWERRIGHT, Buf, 10 ), "LOWERRIGHT" );
    }

  if( New == Old )  Test( "0", "CENTER" );

  if( Edge )  printf( "pop  si\n" "mov  dl,0\n" );

  Next2();

  if( Edge )
    {
    Fix( "si", "LEFT",       1 );
    Fix( "si", "UPPERLEFT",  1 );
    Fix( "si", "LOWERLEFT",  1 );
    Fix( "si", "UP",         1 );
    Fix( "si", "DOWN",       1 );
    Fix( "si", "RIGHT",      1 );
    Fix( "si", "UPPERRIGHT", 1 );
    Fix( "si", "LOWERRIGHT", New == Old );
    }
  else
    {
    Fix( itoa( LEFT, Buf, 10 ),       "LEFT",       1 );
    Fix( itoa( UPPERLEFT, Buf, 10 ),  "UPPERLEFT",  1 );
    Fix( itoa( LOWERLEFT, Buf, 10 ),  "LOWERLEFT",  1 );
    Fix( itoa( UP, Buf, 10 ),         "UP",         1 );
    Fix( itoa( DOWN, Buf, 10 ),       "DOWN",       1 );
    Fix( itoa( RIGHT, Buf, 10 ),      "RIGHT",      1 );
    Fix( itoa( UPPERRIGHT, Buf, 10 ), "UPPERRIGHT", 1 );
    Fix( itoa( LOWERRIGHT, Buf, 10 ), "LOWERRIGHT", New == Old );
    }

  if( New == Old )  Fix( "0", "CENTER", 0 );

  if( Edge )  printf( "pop  si\n" "mov  dl,0\n" );

  Next2();
  }


void main( void )
  {
  char *Seg = "ds";

  BuildMaps();

  printf( "DGROUP group _DATA\n" );
  printf( "LIFE segment 'CODE'\n" );
  printf( "assume cs:LIFE,ds:DGROUP,ss:DGROUP,es:NOTHING\n" );
  printf( ".386C\n" "public _NextGen\n\n" );

  for( Edge = 0; Edge <= 1; Edge++ )
    {
    for( New = 0; New < 8; New++ )
      {
      for( Old = 0; Old < 8; Old++ )
        {
        if( New != Old )  FirstPass();  Label++;
        SecondPass();                   Label++;
        }
      }
    }

  // finished with first pass
  printf( "org  0\n" );
  printf( "mov  si,Change1\n" );
  printf( "mov  di,Change2\n" );
  printf( "mov  Change1,di\n" );
  printf( "mov  Change2,si\n" );
  printf( "mov  ChangeCell,0F000h\n" );
  printf( "mov  ax,seg _LDMAP\n" );
  printf( "mov  ds,ax\n" );
  Next2();

  // entry point
  printf( "_NextGen: push ds bp si di\n" "cld\n" );

  if( WIDTH * HEIGHT > LIST_LIMIT )  Seg = "seg _CHANGE";

  printf( "mov  ax,%s\n", Seg );
  printf( "mov  es,ax\n" );

  #ifndef NODRAW
  printf( "mov  ax,0A000h\n" );
  printf( "mov  fs,ax\n" );
  #endif

  printf( "mov  si,Change1\n" );
  printf( "mov  dl,0\n" );
  Next1();

  printf( "LIFE ends\nend\n" );
  }