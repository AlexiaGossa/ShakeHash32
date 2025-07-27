#include "Platform.h"

#ifdef _WIN32
#pragma warning( disable : 4731 )
#endif


typedef unsigned char       zBYTE;
typedef unsigned int        zDWORD;
typedef unsigned __int64    zQWORD;
/*
#define M_ROR8(x,c)             (((x)>>(c))|((x)<<(8-(c))))
#define M_ROR32(x,c)            (((x)>>(c))|((x)<<(32-(c))))
#define M_ROR64(x,c)            (((x)>>(c))|((x)<<(64-(c))))

#define M_ROL32(x,c)            (((x)<<(c))|((x)>>(32-(c))))
#define M_ROL64(x,c)            (((x)<<(c))|((x)>>(64-(c))))
*/
#define M_ROL64(x,y)	_rotl64(x,y)
#define M_ROR64(x,y)	_rotr64(x,y)

#define M_ROL32(x,c)            (((x)<<(c))|((x)>>(32-(c))))
#define M_ROR32(x,c)            (((x)>>(c))|((x)<<(32-(c))))

void shakeDo64_C    ( void * pKey, int iDoCount, zQWORD * pqwY0, zQWORD * pqwY1 );
void shakeDo64_ASM  ( void * pKey, int iDoCount, zQWORD * pqwY0, zQWORD * pqwY1 );




void shakeDo64_C ( void * pKey, int iDoCount, zQWORD * pqwY0, zQWORD * pqwY1 )
{
    zQWORD  qwX;
    zQWORD  qwX0, qwX1;
    zQWORD  qwY0, qwY1;
    zDWORD  * pdwSource;
    
    pdwSource       = (zDWORD *)pKey;
    qwY0            = *pqwY0;
    qwY1            = *pqwY1;
    do {
        qwX = *pdwSource;
        pdwSource++;

        qwX0 = ( qwX ^ qwY1 ) & 0xFFFFFFFF;
        qwX1 = ( qwX ^ qwY0 ) & 0xFFFFFFFF;
        qwY0 = ( ( ( qwY0 >> 27 ) & 0xFFFFFFFF ) * 0xb11924e1 ) ^ ( qwX0 * 0x4ee6db1e );
        qwY1 = ( ( ( qwY1 >> 27 ) & 0xFFFFFFFF ) * 0x4ee6db1e ) ^ ( qwX1 * 0xb11924e1 );
        
        qwX = *pdwSource;
        pdwSource++;

        qwX0 = ( qwX ^ qwY1 ) & 0xFFFFFFFF;
        qwX1 = ( qwX ^ qwY0 ) & 0xFFFFFFFF;
        qwY0 = ( ( ( qwY0 >> 31 ) & 0xFFFFFFFF ) * 0xb11924e1 ) ^ ( qwX0 * 0x4ee6db1e );
        qwY1 = ( ( ( qwY1 >> 31 ) & 0xFFFFFFFF ) * 0x4ee6db1e ) ^ ( qwX1 * 0xb11924e1 );
        
        iDoCount--;
    } while (iDoCount);
    
    *pqwY0          = qwY0;
    *pqwY1          = qwY1;
}

//10110001000110010010010011100001 = 0xb11924e1 = 2971215073 = fibonacci n47
//01001110111001101101101100011110 = 0x4ee6db1e

__declspec(align(32)) zQWORD qwTableMulX[2] = { 0x4ee6db1e, 0xb11924e1 };
__declspec(align(32)) zQWORD qwTableMulY[2] = { 0xb11924e1, 0x4ee6db1e };

/*
 S = ((Y0 >> 27)[31-0] * A) ^ ((E ^ Y1)[31-0] * /A)
 
 si Y0 = 1<<27 et E^Y1 = 1
 S = A ^ /A
 
 si Y0 = (E^Y1)<<27
 S = (Y0>>27)*A ^ (Y0>>27)* /A
 S = (Y0>>27) * (A ^ /A)
 S = /0
 
 
 S = ((Y0 >> 27)[31-0] * A) ^ ((E ^ Y0)[31-0] * /A)
 
 Hypothèse : 
 (Y0>>27)[31-0] peut-il être égal à (E^Y0)[31-0]
 
 
 
*/
void shakeDo128_ASM ( void * pKey, int iDoCount, zQWORD * pqwY )
{
    __asm {
            push    esi
            push    ecx
            push    eax
    
            mov     ecx, iDoCount
            mov     esi, pKey
            
            //On charge Y dans xmm2 et xmm6
            mov     eax, pqwY
            movups  xmm2, [eax]    
    
            //Initialise X
            pxor    xmm0, xmm0
            pxor    xmm1, xmm1
            pxor    xmm5, xmm5
            pxor    xmm6, xmm6
            
            //Récupération des multiplicateurs            
            movaps  xmm3, qwTableMulX                                           
            movaps  xmm4, qwTableMulY
                                                                                        //XMM0 = 0
                                                                                        //XMM1 = 0
                                                                                        //XMM2[127-64] = qwY1
                                                                                        //XMM2[ 63- 0] = qwY0
        __loop:
    
            //STEP #01 : On charge X(t) et X(t+1) dans xmm0 et xmm1
            movss   xmm0, [esi]                                                         //xmm0[31-0] = E(t)
            movss   xmm1, [esi+4]                                                       //xmm1[31-0] = E(t+1)
            movlhps xmm0, xmm0                                                          //xmm0[95-64] = E(t)
            movlhps xmm1, xmm1                                                          //xmm1[95-64] = E(t+1

            //STEP #02 : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm0, xmm2
            shufps  xmm0, xmm0, 10
    
            //STEP #03 : On va appliquer une paire de décalage sur Y0 et Y1 (27=best)
            psrlq   xmm2, 27 //27     //29 //27, 31, 29, 33
            
            //STEP #04           
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm0, xmm3 //qwTableMulX
            pxor    xmm2, xmm0
            
            //STEP #05 (eq #02) : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm1, xmm2
            shufps  xmm1, xmm1, 10
    
            //STEP #06 (eq #03) : On va appliquer une paire de décalage sur Y0 et Y1 (31=best)
            psrlq   xmm2, 33 //31   //27, 31, 29, 33
            
            //STEP #07 (eq #04)
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm1, xmm3 //qwTableMulX
            pxor    xmm2, xmm1

            //STEP #01 : On charge X(t) et X(t+1) dans xmm0 et xmm1
            movss   xmm5, [esi+8]                                                       //xmm0[31-0] = E(t)
            movss   xmm5, [esi+12]                                                      //xmm1[31-0] = E(t+1)
            movlhps xmm5, xmm5                                                          //xmm0[95-64] = E(t)
            movlhps xmm6, xmm6                                                          //xmm1[95-64] = E(t+1

            //STEP #02 : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm5, xmm2
            shufps  xmm5, xmm5, 10
    
            //STEP #03 : On va appliquer une paire de décalage sur Y0 et Y1 (27=best)
            psrlq   xmm2, 31 //27     //29 //27, 31, 29, 33
            
            //STEP #04           
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm5, xmm3 //qwTableMulX
            pxor    xmm2, xmm5
            
            //STEP #05 (eq #02) : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm6, xmm2
            shufps  xmm6, xmm6, 10
    
            //STEP #06 (eq #03) : On va appliquer une paire de décalage sur Y0 et Y1 (31=best)
            psrlq   xmm2, 32 //31   //27, 31, 29, 33
            
            //STEP #07 (eq #04)
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm6, xmm3 //qwTableMulX
            pxor    xmm2, xmm6
            
            add     esi, 16
            sub     ecx, 1
            jnz     __loop
            
            movups  [eax], xmm2
            
            pop     eax
            pop     ecx
            pop     esi
    }
}



void shakeDo64_ASM ( void * pKey, int iDoCount, zQWORD * pqwY, zDWORD dwShiftA, zDWORD dwShiftB )
{
    __asm {
            push    esi
            push    ecx
            push    eax
    
            mov     ecx, iDoCount
            mov     esi, pKey
            
            //On charge Y dans xmm2 et xmm6
            mov     eax, pqwY
            movups  xmm2, [eax]    
    
            //Initialise X
            pxor    xmm0, xmm0
            pxor    xmm1, xmm1
            
            //Récupération des multiplicateurs            
            movaps  xmm3, qwTableMulX                                           
            movaps  xmm4, qwTableMulY
            
            movd    xmm6, dwShiftA 
            movd    xmm7, dwShiftB
                                                                                        //XMM0 = 0
                                                                                        //XMM1 = 0
                                                                                        //XMM2[127-64] = qwY1
                                                                                        //XMM2[ 63- 0] = qwY0
        __loop:
    
            //STEP #01 : On charge X(t) et X(t+1) dans xmm0 et xmm1
            movd    xmm0, [esi]                                                         //xmm0[31-0] = E(t)
            movd    xmm1, [esi+4]                                                       //xmm1[31-0] = E(t+1)
            movlhps xmm0, xmm0                                                          //xmm0[95-64] = E(t)
            movlhps xmm1, xmm1                                                          //xmm1[95-64] = E(t+1

            //STEP #02 : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm0, xmm2
            shufps  xmm0, xmm0, 10
    
            //STEP #03 : On va appliquer une paire de décalage sur Y0 et Y1 (27=best)
            psrlq   xmm2, xmm6 //27     //29 //27, 31, 29, 33
            
            //STEP #04           
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm0, xmm3 //qwTableMulX
            pxor    xmm2, xmm0
            
            //STEP #05 (eq #02) : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm1, xmm2
            shufps  xmm1, xmm1, 10
    
            //STEP #06 (eq #03) : On va appliquer une paire de décalage sur Y0 et Y1 (31=best)
            psrlq   xmm2, xmm7 //31   //27, 31, 29, 33
            
            //STEP #07 (eq #04)
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm1, xmm3 //qwTableMulX
            pxor    xmm2, xmm1
            
            add     esi, 8
            sub     ecx, 1
            jnz     __loop
            
            movups  [eax], xmm2
            
            pop     eax
            pop     ecx
            pop     esi
    }
}





#define SHAKEHASH_ASM


void ShakeHash32_ProcessQWORD ( void * pKey, int iDoCount, zQWORD * pqwY )
{
    __asm {
            push    esi
            push    ecx
            push    eax
    
            mov     ecx, iDoCount
            mov     esi, pKey
            
            //On charge Y dans xmm2 et xmm6
            mov     eax, pqwY
            movups  xmm2, [eax]    
    
            //Récupération des multiplicateurs
            movaps  xmm3, qwTableMulX
            movaps  xmm4, qwTableMulY
                //XMM0 = 0
                //XMM1 = 0
                //XMM2[127-64] = qwY1
                //XMM2[ 63- 0] = qwY0
        __loop:
    
            //STEP #01 : On charge X(t) et X(t+1) dans xmm0 et xmm1
            movq    xmm7, qword ptr [esi]
            pshufd  xmm0, xmm7, 00000000b
            
            
            /*
            
            movd   xmm0, [esi]                                                          //xmm0[31-0] = E(t)
            movd   xmm1, [esi+4]                                                        //xmm1[31-0] = E(t+1)
            movlhps xmm0, xmm0                                                          //xmm0[95-64] = E(t)
            movlhps xmm1, xmm1                                                          //xmm1[95-64] = E(t+1
            */

            //STEP #02 : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm0, xmm2
            shufps  xmm0, xmm0, 10
    
            //STEP #03 : On va appliquer une paire de décalage sur Y0 et Y1 (27=best)
            psrlq   xmm2, 30
            
            //Prepare X(t+1)
            pshufd  xmm1, xmm7, 01010101b
            
            //STEP #04           
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm0, xmm3 //qwTableMulX
            pxor    xmm2, xmm0
            
            //STEP #05 (eq #02) : On applique X xor Y et on permute X0 <=> X1
            pxor    xmm1, xmm2
            shufps  xmm1, xmm1, 10
    
            //STEP #06 (eq #03) : On va appliquer une paire de décalage sur Y0 et Y1 (31=best)
            psrlq   xmm2, 30
            
            //STEP #07 (eq #04)
            pmuludq xmm2, xmm4 //qwTableMulY
            pmuludq xmm1, xmm3 //qwTableMulX
            pxor    xmm2, xmm1
            
            add     esi, 8
            sub     ecx, 1
            jnz     __loop
            
            movups  [eax], xmm2
            
            pop     eax
            pop     ecx
            pop     esi
    }
}





void ShakeHash32 ( const void * key, int len, uint32_t seed, void * out )
{
    zQWORD  qwY[4];
    zBYTE   * pbSource;
#ifndef SHAKEHASH_ASM
    zDWORD  dwSizeRemain;
#endif

    //Init Y2 and Y3 base registers
#ifndef SHAKEHASH_ASM
    qwY[2] = ((zQWORD)seed ^ 0x4ee6db1e ) * 0xb11924e1;
    qwY[3] = ((zQWORD) len ^ 0xb11924e1 ) * 0x4ee6db1e;
    qwY[2] = (( (qwY[2]>>17) ^ 0xb11924e1 ) * 0x4ee6db1e ) + qwY[3];
    qwY[3] = (( (qwY[3]>>17) ^ 0x4ee6db1e ) * 0xb11924e1 ) + qwY[2];
    qwY[0] = qwY[2];
    qwY[1] = qwY[3];
#else
    __asm {
        lea         edi, qwY
        movd        xmm0, len
        movd        xmm1, seed
        movaps      xmm3, qwTableMulX                                           
        movlhps     xmm0, xmm0
        movaps      xmm4, qwTableMulY
        movss       xmm0, xmm1
        
        pxor        xmm0, xmm3
        pmuludq     xmm0, xmm4
        
        movaps      xmm1, xmm0
        psrlq       xmm0, 17
        pxor        xmm0, xmm4
        pmuludq     xmm0, xmm3
        shufps      xmm1, xmm1, 10
        
        paddq       xmm0, xmm1
        
        movups      [edi], xmm0
        movups      [edi+16], xmm0
    }        
#endif

    //Source data
    pbSource        = (zBYTE *)key;
    
    //Process using input block of 8-byte
    if (len>=8)
        ShakeHash32_ProcessQWORD ( pbSource, len>>3, qwY+2 );

    //Process remaining input data
    if (len&7)
    {
#ifndef SHAKEHASH_ASM    
        dwSizeRemain    = len;
        pbSource        += dwSizeRemain & 0xFFFFFFF8;
        dwSizeRemain    &= 7;
        qwY[0]          = qwY[2];
        qwY[1]          = qwY[3];
        memcpy ( qwY, pbSource, dwSizeRemain );
#else        
        __asm {
            lea     edi, qwY
            mov     esi, len
            
            mov     ecx, len
            and     esi, 0xFFFFFFF8            
            
            movups  xmm0, [edi+16]

            and     ecx, 7
            add     esi, pbSource
            
            movups  [edi], xmm0
            
            cld
            rep     movsb
        }
#endif
        ShakeHash32_ProcessQWORD ( qwY, 2, qwY+2 );


#ifndef SHAKEHASH_ASM    
        qwY[0] = qwY[2];
        qwY[1] = qwY[3];
#else        
        __asm {
            lea     edi, qwY
            movups  xmm0, [edi+16]
            movups  [edi], xmm0
        }
#endif
        
        
    }
    if (len<=16) ShakeHash32_ProcessQWORD  ( qwY, 4, qwY+2 );
    else         ShakeHash32_ProcessQWORD  ( qwY+2, 2, qwY+2 );
    
    *(uint32_t*)out = (zDWORD)(  (qwY[2] * qwY[3])>>32 );
}
    






long    shakeShiftA=27;
long    shakeShiftB=31;
    
void hybrid ( const void * key, int len, uint32_t seed, void * out )
{
    zQWORD  qwY[4];
    zBYTE   * pbSource;

    /*
    qwY[2] = ((zQWORD)seed ^ 0x4ee6db1e ) * 0xb11924e1;
    qwY[3] = ((zQWORD) len ^ 0xb11924e1 ) * 0x4ee6db1e;
    qwY[2] = (( (qwY[2]>>17) ^ 0xb11924e1 ) * 0x4ee6db1e ) + qwY[3];
    qwY[3] = (( (qwY[3]>>17) ^ 0x4ee6db1e ) * 0xb11924e1 ) + qwY[2];
    */
    
    __asm {
        lea         edi, qwY
        movd        xmm0, len
        movd        xmm1, seed
        movaps      xmm3, qwTableMulX                                           
        movlhps     xmm0, xmm0
        movaps      xmm4, qwTableMulY
        movss       xmm0, xmm1
        
        pxor        xmm0, xmm3
        pmuludq     xmm0, xmm4
        
        movaps      xmm1, xmm0
        psrlq       xmm0, 17
        pxor        xmm0, xmm4
        pmuludq     xmm0, xmm3
        shufps      xmm1, xmm1, 10
        
        paddq       xmm0, xmm1
        
        movups      [edi], xmm0
        movups      [edi+16], xmm0
    }        
    
    pbSource        = (zBYTE *)key;
    
    if (len>=8)
        shakeDo64_ASM ( pbSource, len>>3, qwY+2, shakeShiftA, shakeShiftB );
        
    if (len&7)
    {
        //On se déplace à la fin des données    
        //pbSource += dwSizeRemain & 0xFFFFFFF8;
        //dwSizeRemain &= 7;
        //qwY[0] = qwY[2] ^ qwY[3];
        
        __asm {
            mov     esi, len
            lea     edi, qwY
            
            mov     ecx, len
            and     esi, 0xFFFFFFF8
            and     ecx, 7
            
            add     esi, pbSource
            
            cld
            rep     movsb
        }
        
        shakeDo64_ASM ( qwY, 2, qwY+2, shakeShiftA, shakeShiftB );
    }
    if (len<=16) shakeDo64_ASM ( qwY, 4, qwY+2, shakeShiftA, shakeShiftB );
    else         shakeDo64_ASM ( qwY+2, 2, qwY+2, shakeShiftA, shakeShiftB );
    
    *(uint32_t*)out = (zDWORD)(  (qwY[2] * qwY[3])>>32 );
}
    

