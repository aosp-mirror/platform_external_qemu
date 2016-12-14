/**
 *
 * aes.c - integrated in QEMU by Fabrice Bellard from the OpenSSL project.
 */
/*
 * rijndael-alg-fst.c
 *
 * @version 3.0 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qemu-common.h"
#include "qemu/aes.h"
#include "util/aes.h"

#ifdef QEMU_USE_INTERNAL_OPENSSL

static const u32 rcon[] = {
        0x01000000, 0x02000000, 0x04000000, 0x08000000,
        0x10000000, 0x20000000, 0x40000000, 0x80000000,
        0x1B000000, 0x36000000, /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
};

/**
 * Expand the cipher key into the encryption key schedule.
 */
int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
			AES_KEY *key) {

	u32 *rk;
   	int i = 0;
	u32 temp;

	if (!userKey || !key)
		return -1;
	if (bits != 128 && bits != 192 && bits != 256)
		return -2;

	rk = key->rd_key;

	if (bits==128)
		key->rounds = 10;
	else if (bits==192)
		key->rounds = 12;
	else
		key->rounds = 14;

	rk[0] = GETU32(userKey     );
	rk[1] = GETU32(userKey +  4);
	rk[2] = GETU32(userKey +  8);
	rk[3] = GETU32(userKey + 12);
	if (bits == 128) {
		while (1) {
			temp  = rk[3];
			rk[4] = rk[0] ^
                                (AES_Te4[(temp >> 16) & 0xff] & 0xff000000) ^
                                (AES_Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
                                (AES_Te4[(temp      ) & 0xff] & 0x0000ff00) ^
                                (AES_Te4[(temp >> 24)       ] & 0x000000ff) ^
				rcon[i];
			rk[5] = rk[1] ^ rk[4];
			rk[6] = rk[2] ^ rk[5];
			rk[7] = rk[3] ^ rk[6];
			if (++i == 10) {
				return 0;
			}
			rk += 4;
		}
	}
	rk[4] = GETU32(userKey + 16);
	rk[5] = GETU32(userKey + 20);
	if (bits == 192) {
		while (1) {
			temp = rk[ 5];
			rk[ 6] = rk[ 0] ^
                                (AES_Te4[(temp >> 16) & 0xff] & 0xff000000) ^
                                (AES_Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
                                (AES_Te4[(temp      ) & 0xff] & 0x0000ff00) ^
                                (AES_Te4[(temp >> 24)       ] & 0x000000ff) ^
				rcon[i];
			rk[ 7] = rk[ 1] ^ rk[ 6];
			rk[ 8] = rk[ 2] ^ rk[ 7];
			rk[ 9] = rk[ 3] ^ rk[ 8];
			if (++i == 8) {
				return 0;
			}
			rk[10] = rk[ 4] ^ rk[ 9];
			rk[11] = rk[ 5] ^ rk[10];
			rk += 6;
		}
	}
	rk[6] = GETU32(userKey + 24);
	rk[7] = GETU32(userKey + 28);
	if (bits == 256) {
		while (1) {
			temp = rk[ 7];
			rk[ 8] = rk[ 0] ^
                                (AES_Te4[(temp >> 16) & 0xff] & 0xff000000) ^
                                (AES_Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
                                (AES_Te4[(temp      ) & 0xff] & 0x0000ff00) ^
                                (AES_Te4[(temp >> 24)       ] & 0x000000ff) ^
				rcon[i];
			rk[ 9] = rk[ 1] ^ rk[ 8];
			rk[10] = rk[ 2] ^ rk[ 9];
			rk[11] = rk[ 3] ^ rk[10];
			if (++i == 7) {
				return 0;
			}
			temp = rk[11];
			rk[12] = rk[ 4] ^
                                (AES_Te4[(temp >> 24)       ] & 0xff000000) ^
                                (AES_Te4[(temp >> 16) & 0xff] & 0x00ff0000) ^
                                (AES_Te4[(temp >>  8) & 0xff] & 0x0000ff00) ^
                                (AES_Te4[(temp      ) & 0xff] & 0x000000ff);
			rk[13] = rk[ 5] ^ rk[12];
			rk[14] = rk[ 6] ^ rk[13];
			rk[15] = rk[ 7] ^ rk[14];

			rk += 8;
        	}
	}
	return 0;
}

/**
 * Expand the cipher key into the decryption key schedule.
 */
int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
			 AES_KEY *key) {

        u32 *rk;
	int i, j, status;
	u32 temp;

	/* first, start with an encryption schedule */
	status = AES_set_encrypt_key(userKey, bits, key);
	if (status < 0)
		return status;

	rk = key->rd_key;

	/* invert the order of the round keys: */
	for (i = 0, j = 4*(key->rounds); i < j; i += 4, j -= 4) {
		temp = rk[i    ]; rk[i    ] = rk[j    ]; rk[j    ] = temp;
		temp = rk[i + 1]; rk[i + 1] = rk[j + 1]; rk[j + 1] = temp;
		temp = rk[i + 2]; rk[i + 2] = rk[j + 2]; rk[j + 2] = temp;
		temp = rk[i + 3]; rk[i + 3] = rk[j + 3]; rk[j + 3] = temp;
	}
	/* apply the inverse MixColumn transform to all round keys but the first and the last: */
	for (i = 1; i < (key->rounds); i++) {
		rk += 4;
		rk[0] =
                        AES_Td0[AES_Te4[(rk[0] >> 24)       ] & 0xff] ^
                        AES_Td1[AES_Te4[(rk[0] >> 16) & 0xff] & 0xff] ^
                        AES_Td2[AES_Te4[(rk[0] >>  8) & 0xff] & 0xff] ^
                        AES_Td3[AES_Te4[(rk[0]      ) & 0xff] & 0xff];
		rk[1] =
                        AES_Td0[AES_Te4[(rk[1] >> 24)       ] & 0xff] ^
                        AES_Td1[AES_Te4[(rk[1] >> 16) & 0xff] & 0xff] ^
                        AES_Td2[AES_Te4[(rk[1] >>  8) & 0xff] & 0xff] ^
                        AES_Td3[AES_Te4[(rk[1]      ) & 0xff] & 0xff];
		rk[2] =
                        AES_Td0[AES_Te4[(rk[2] >> 24)       ] & 0xff] ^
                        AES_Td1[AES_Te4[(rk[2] >> 16) & 0xff] & 0xff] ^
                        AES_Td2[AES_Te4[(rk[2] >>  8) & 0xff] & 0xff] ^
                        AES_Td3[AES_Te4[(rk[2]      ) & 0xff] & 0xff];
		rk[3] =
                        AES_Td0[AES_Te4[(rk[3] >> 24)       ] & 0xff] ^
                        AES_Td1[AES_Te4[(rk[3] >> 16) & 0xff] & 0xff] ^
                        AES_Td2[AES_Te4[(rk[3] >>  8) & 0xff] & 0xff] ^
                        AES_Td3[AES_Te4[(rk[3]      ) & 0xff] & 0xff];
	}
	return 0;
}

#ifndef AES_ASM
/*
 * Encrypt a single block
 * in and out can overlap
 */
void AES_encrypt(const unsigned char *in, unsigned char *out,
		 const AES_KEY *key) {

	const u32 *rk;
	u32 s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
	int r;
#endif /* ?FULL_UNROLL */

	assert(in && out && key);
	rk = key->rd_key;

	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(in     ) ^ rk[0];
	s1 = GETU32(in +  4) ^ rk[1];
	s2 = GETU32(in +  8) ^ rk[2];
	s3 = GETU32(in + 12) ^ rk[3];
#ifdef FULL_UNROLL
	/* round 1: */
        t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[ 4];
        t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[ 5];
        t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[ 6];
        t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[ 7];
   	/* round 2: */
        s0 = AES_Te0[t0 >> 24] ^ AES_Te1[(t1 >> 16) & 0xff] ^ AES_Te2[(t2 >>  8) & 0xff] ^ AES_Te3[t3 & 0xff] ^ rk[ 8];
        s1 = AES_Te0[t1 >> 24] ^ AES_Te1[(t2 >> 16) & 0xff] ^ AES_Te2[(t3 >>  8) & 0xff] ^ AES_Te3[t0 & 0xff] ^ rk[ 9];
        s2 = AES_Te0[t2 >> 24] ^ AES_Te1[(t3 >> 16) & 0xff] ^ AES_Te2[(t0 >>  8) & 0xff] ^ AES_Te3[t1 & 0xff] ^ rk[10];
        s3 = AES_Te0[t3 >> 24] ^ AES_Te1[(t0 >> 16) & 0xff] ^ AES_Te2[(t1 >>  8) & 0xff] ^ AES_Te3[t2 & 0xff] ^ rk[11];
	/* round 3: */
        t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[12];
        t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[13];
        t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[14];
        t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[15];
   	/* round 4: */
        s0 = AES_Te0[t0 >> 24] ^ AES_Te1[(t1 >> 16) & 0xff] ^ AES_Te2[(t2 >>  8) & 0xff] ^ AES_Te3[t3 & 0xff] ^ rk[16];
        s1 = AES_Te0[t1 >> 24] ^ AES_Te1[(t2 >> 16) & 0xff] ^ AES_Te2[(t3 >>  8) & 0xff] ^ AES_Te3[t0 & 0xff] ^ rk[17];
        s2 = AES_Te0[t2 >> 24] ^ AES_Te1[(t3 >> 16) & 0xff] ^ AES_Te2[(t0 >>  8) & 0xff] ^ AES_Te3[t1 & 0xff] ^ rk[18];
        s3 = AES_Te0[t3 >> 24] ^ AES_Te1[(t0 >> 16) & 0xff] ^ AES_Te2[(t1 >>  8) & 0xff] ^ AES_Te3[t2 & 0xff] ^ rk[19];
	/* round 5: */
        t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[20];
        t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[21];
        t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[22];
        t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[23];
   	/* round 6: */
        s0 = AES_Te0[t0 >> 24] ^ AES_Te1[(t1 >> 16) & 0xff] ^ AES_Te2[(t2 >>  8) & 0xff] ^ AES_Te3[t3 & 0xff] ^ rk[24];
        s1 = AES_Te0[t1 >> 24] ^ AES_Te1[(t2 >> 16) & 0xff] ^ AES_Te2[(t3 >>  8) & 0xff] ^ AES_Te3[t0 & 0xff] ^ rk[25];
        s2 = AES_Te0[t2 >> 24] ^ AES_Te1[(t3 >> 16) & 0xff] ^ AES_Te2[(t0 >>  8) & 0xff] ^ AES_Te3[t1 & 0xff] ^ rk[26];
        s3 = AES_Te0[t3 >> 24] ^ AES_Te1[(t0 >> 16) & 0xff] ^ AES_Te2[(t1 >>  8) & 0xff] ^ AES_Te3[t2 & 0xff] ^ rk[27];
	/* round 7: */
        t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[28];
        t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[29];
        t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[30];
        t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[31];
   	/* round 8: */
        s0 = AES_Te0[t0 >> 24] ^ AES_Te1[(t1 >> 16) & 0xff] ^ AES_Te2[(t2 >>  8) & 0xff] ^ AES_Te3[t3 & 0xff] ^ rk[32];
        s1 = AES_Te0[t1 >> 24] ^ AES_Te1[(t2 >> 16) & 0xff] ^ AES_Te2[(t3 >>  8) & 0xff] ^ AES_Te3[t0 & 0xff] ^ rk[33];
        s2 = AES_Te0[t2 >> 24] ^ AES_Te1[(t3 >> 16) & 0xff] ^ AES_Te2[(t0 >>  8) & 0xff] ^ AES_Te3[t1 & 0xff] ^ rk[34];
        s3 = AES_Te0[t3 >> 24] ^ AES_Te1[(t0 >> 16) & 0xff] ^ AES_Te2[(t1 >>  8) & 0xff] ^ AES_Te3[t2 & 0xff] ^ rk[35];
	/* round 9: */
        t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[36];
        t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[37];
        t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[38];
        t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[39];
    if (key->rounds > 10) {
        /* round 10: */
        s0 = AES_Te0[t0 >> 24] ^ AES_Te1[(t1 >> 16) & 0xff] ^ AES_Te2[(t2 >>  8) & 0xff] ^ AES_Te3[t3 & 0xff] ^ rk[40];
        s1 = AES_Te0[t1 >> 24] ^ AES_Te1[(t2 >> 16) & 0xff] ^ AES_Te2[(t3 >>  8) & 0xff] ^ AES_Te3[t0 & 0xff] ^ rk[41];
        s2 = AES_Te0[t2 >> 24] ^ AES_Te1[(t3 >> 16) & 0xff] ^ AES_Te2[(t0 >>  8) & 0xff] ^ AES_Te3[t1 & 0xff] ^ rk[42];
        s3 = AES_Te0[t3 >> 24] ^ AES_Te1[(t0 >> 16) & 0xff] ^ AES_Te2[(t1 >>  8) & 0xff] ^ AES_Te3[t2 & 0xff] ^ rk[43];
        /* round 11: */
        t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[44];
        t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[45];
        t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[46];
        t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[47];
        if (key->rounds > 12) {
            /* round 12: */
            s0 = AES_Te0[t0 >> 24] ^ AES_Te1[(t1 >> 16) & 0xff] ^ AES_Te2[(t2 >>  8) & 0xff] ^ AES_Te3[t3 & 0xff] ^ rk[48];
            s1 = AES_Te0[t1 >> 24] ^ AES_Te1[(t2 >> 16) & 0xff] ^ AES_Te2[(t3 >>  8) & 0xff] ^ AES_Te3[t0 & 0xff] ^ rk[49];
            s2 = AES_Te0[t2 >> 24] ^ AES_Te1[(t3 >> 16) & 0xff] ^ AES_Te2[(t0 >>  8) & 0xff] ^ AES_Te3[t1 & 0xff] ^ rk[50];
            s3 = AES_Te0[t3 >> 24] ^ AES_Te1[(t0 >> 16) & 0xff] ^ AES_Te2[(t1 >>  8) & 0xff] ^ AES_Te3[t2 & 0xff] ^ rk[51];
            /* round 13: */
            t0 = AES_Te0[s0 >> 24] ^ AES_Te1[(s1 >> 16) & 0xff] ^ AES_Te2[(s2 >>  8) & 0xff] ^ AES_Te3[s3 & 0xff] ^ rk[52];
            t1 = AES_Te0[s1 >> 24] ^ AES_Te1[(s2 >> 16) & 0xff] ^ AES_Te2[(s3 >>  8) & 0xff] ^ AES_Te3[s0 & 0xff] ^ rk[53];
            t2 = AES_Te0[s2 >> 24] ^ AES_Te1[(s3 >> 16) & 0xff] ^ AES_Te2[(s0 >>  8) & 0xff] ^ AES_Te3[s1 & 0xff] ^ rk[54];
            t3 = AES_Te0[s3 >> 24] ^ AES_Te1[(s0 >> 16) & 0xff] ^ AES_Te2[(s1 >>  8) & 0xff] ^ AES_Te3[s2 & 0xff] ^ rk[55];
        }
    }
    rk += key->rounds << 2;
#else  /* !FULL_UNROLL */
    /*
     * Nr - 1 full rounds:
     */
    r = key->rounds >> 1;
    for (;;) {
        t0 =
            AES_Te0[(s0 >> 24)       ] ^
            AES_Te1[(s1 >> 16) & 0xff] ^
            AES_Te2[(s2 >>  8) & 0xff] ^
            AES_Te3[(s3      ) & 0xff] ^
            rk[4];
        t1 =
            AES_Te0[(s1 >> 24)       ] ^
            AES_Te1[(s2 >> 16) & 0xff] ^
            AES_Te2[(s3 >>  8) & 0xff] ^
            AES_Te3[(s0      ) & 0xff] ^
            rk[5];
        t2 =
            AES_Te0[(s2 >> 24)       ] ^
            AES_Te1[(s3 >> 16) & 0xff] ^
            AES_Te2[(s0 >>  8) & 0xff] ^
            AES_Te3[(s1      ) & 0xff] ^
            rk[6];
        t3 =
            AES_Te0[(s3 >> 24)       ] ^
            AES_Te1[(s0 >> 16) & 0xff] ^
            AES_Te2[(s1 >>  8) & 0xff] ^
            AES_Te3[(s2      ) & 0xff] ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }

        s0 =
            AES_Te0[(t0 >> 24)       ] ^
            AES_Te1[(t1 >> 16) & 0xff] ^
            AES_Te2[(t2 >>  8) & 0xff] ^
            AES_Te3[(t3      ) & 0xff] ^
            rk[0];
        s1 =
            AES_Te0[(t1 >> 24)       ] ^
            AES_Te1[(t2 >> 16) & 0xff] ^
            AES_Te2[(t3 >>  8) & 0xff] ^
            AES_Te3[(t0      ) & 0xff] ^
            rk[1];
        s2 =
            AES_Te0[(t2 >> 24)       ] ^
            AES_Te1[(t3 >> 16) & 0xff] ^
            AES_Te2[(t0 >>  8) & 0xff] ^
            AES_Te3[(t1      ) & 0xff] ^
            rk[2];
        s3 =
            AES_Te0[(t3 >> 24)       ] ^
            AES_Te1[(t0 >> 16) & 0xff] ^
            AES_Te2[(t1 >>  8) & 0xff] ^
            AES_Te3[(t2      ) & 0xff] ^
            rk[3];
    }
#endif /* ?FULL_UNROLL */
    /*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	s0 =
                (AES_Te4[(t0 >> 24)       ] & 0xff000000) ^
                (AES_Te4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Te4[(t2 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Te4[(t3      ) & 0xff] & 0x000000ff) ^
		rk[0];
	PUTU32(out     , s0);
	s1 =
                (AES_Te4[(t1 >> 24)       ] & 0xff000000) ^
                (AES_Te4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Te4[(t3 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Te4[(t0      ) & 0xff] & 0x000000ff) ^
		rk[1];
	PUTU32(out +  4, s1);
	s2 =
                (AES_Te4[(t2 >> 24)       ] & 0xff000000) ^
                (AES_Te4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Te4[(t0 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Te4[(t1      ) & 0xff] & 0x000000ff) ^
		rk[2];
	PUTU32(out +  8, s2);
	s3 =
                (AES_Te4[(t3 >> 24)       ] & 0xff000000) ^
                (AES_Te4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Te4[(t1 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Te4[(t2      ) & 0xff] & 0x000000ff) ^
		rk[3];
	PUTU32(out + 12, s3);
}

/*
 * Decrypt a single block
 * in and out can overlap
 */
void AES_decrypt(const unsigned char *in, unsigned char *out,
		 const AES_KEY *key) {

	const u32 *rk;
	u32 s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
	int r;
#endif /* ?FULL_UNROLL */

	assert(in && out && key);
	rk = key->rd_key;

	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
    s0 = GETU32(in     ) ^ rk[0];
    s1 = GETU32(in +  4) ^ rk[1];
    s2 = GETU32(in +  8) ^ rk[2];
    s3 = GETU32(in + 12) ^ rk[3];
#ifdef FULL_UNROLL
    /* round 1: */
    t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[ 4];
    t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[ 5];
    t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[ 6];
    t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[ 7];
    /* round 2: */
    s0 = AES_Td0[t0 >> 24] ^ AES_Td1[(t3 >> 16) & 0xff] ^ AES_Td2[(t2 >>  8) & 0xff] ^ AES_Td3[t1 & 0xff] ^ rk[ 8];
    s1 = AES_Td0[t1 >> 24] ^ AES_Td1[(t0 >> 16) & 0xff] ^ AES_Td2[(t3 >>  8) & 0xff] ^ AES_Td3[t2 & 0xff] ^ rk[ 9];
    s2 = AES_Td0[t2 >> 24] ^ AES_Td1[(t1 >> 16) & 0xff] ^ AES_Td2[(t0 >>  8) & 0xff] ^ AES_Td3[t3 & 0xff] ^ rk[10];
    s3 = AES_Td0[t3 >> 24] ^ AES_Td1[(t2 >> 16) & 0xff] ^ AES_Td2[(t1 >>  8) & 0xff] ^ AES_Td3[t0 & 0xff] ^ rk[11];
    /* round 3: */
    t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[12];
    t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[13];
    t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[14];
    t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[15];
    /* round 4: */
    s0 = AES_Td0[t0 >> 24] ^ AES_Td1[(t3 >> 16) & 0xff] ^ AES_Td2[(t2 >>  8) & 0xff] ^ AES_Td3[t1 & 0xff] ^ rk[16];
    s1 = AES_Td0[t1 >> 24] ^ AES_Td1[(t0 >> 16) & 0xff] ^ AES_Td2[(t3 >>  8) & 0xff] ^ AES_Td3[t2 & 0xff] ^ rk[17];
    s2 = AES_Td0[t2 >> 24] ^ AES_Td1[(t1 >> 16) & 0xff] ^ AES_Td2[(t0 >>  8) & 0xff] ^ AES_Td3[t3 & 0xff] ^ rk[18];
    s3 = AES_Td0[t3 >> 24] ^ AES_Td1[(t2 >> 16) & 0xff] ^ AES_Td2[(t1 >>  8) & 0xff] ^ AES_Td3[t0 & 0xff] ^ rk[19];
    /* round 5: */
    t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[20];
    t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[21];
    t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[22];
    t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[23];
    /* round 6: */
    s0 = AES_Td0[t0 >> 24] ^ AES_Td1[(t3 >> 16) & 0xff] ^ AES_Td2[(t2 >>  8) & 0xff] ^ AES_Td3[t1 & 0xff] ^ rk[24];
    s1 = AES_Td0[t1 >> 24] ^ AES_Td1[(t0 >> 16) & 0xff] ^ AES_Td2[(t3 >>  8) & 0xff] ^ AES_Td3[t2 & 0xff] ^ rk[25];
    s2 = AES_Td0[t2 >> 24] ^ AES_Td1[(t1 >> 16) & 0xff] ^ AES_Td2[(t0 >>  8) & 0xff] ^ AES_Td3[t3 & 0xff] ^ rk[26];
    s3 = AES_Td0[t3 >> 24] ^ AES_Td1[(t2 >> 16) & 0xff] ^ AES_Td2[(t1 >>  8) & 0xff] ^ AES_Td3[t0 & 0xff] ^ rk[27];
    /* round 7: */
    t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[28];
    t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[29];
    t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[30];
    t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[31];
    /* round 8: */
    s0 = AES_Td0[t0 >> 24] ^ AES_Td1[(t3 >> 16) & 0xff] ^ AES_Td2[(t2 >>  8) & 0xff] ^ AES_Td3[t1 & 0xff] ^ rk[32];
    s1 = AES_Td0[t1 >> 24] ^ AES_Td1[(t0 >> 16) & 0xff] ^ AES_Td2[(t3 >>  8) & 0xff] ^ AES_Td3[t2 & 0xff] ^ rk[33];
    s2 = AES_Td0[t2 >> 24] ^ AES_Td1[(t1 >> 16) & 0xff] ^ AES_Td2[(t0 >>  8) & 0xff] ^ AES_Td3[t3 & 0xff] ^ rk[34];
    s3 = AES_Td0[t3 >> 24] ^ AES_Td1[(t2 >> 16) & 0xff] ^ AES_Td2[(t1 >>  8) & 0xff] ^ AES_Td3[t0 & 0xff] ^ rk[35];
    /* round 9: */
    t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[36];
    t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[37];
    t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[38];
    t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[39];
    if (key->rounds > 10) {
        /* round 10: */
        s0 = AES_Td0[t0 >> 24] ^ AES_Td1[(t3 >> 16) & 0xff] ^ AES_Td2[(t2 >>  8) & 0xff] ^ AES_Td3[t1 & 0xff] ^ rk[40];
        s1 = AES_Td0[t1 >> 24] ^ AES_Td1[(t0 >> 16) & 0xff] ^ AES_Td2[(t3 >>  8) & 0xff] ^ AES_Td3[t2 & 0xff] ^ rk[41];
        s2 = AES_Td0[t2 >> 24] ^ AES_Td1[(t1 >> 16) & 0xff] ^ AES_Td2[(t0 >>  8) & 0xff] ^ AES_Td3[t3 & 0xff] ^ rk[42];
        s3 = AES_Td0[t3 >> 24] ^ AES_Td1[(t2 >> 16) & 0xff] ^ AES_Td2[(t1 >>  8) & 0xff] ^ AES_Td3[t0 & 0xff] ^ rk[43];
        /* round 11: */
        t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[44];
        t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[45];
        t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[46];
        t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[47];
        if (key->rounds > 12) {
            /* round 12: */
            s0 = AES_Td0[t0 >> 24] ^ AES_Td1[(t3 >> 16) & 0xff] ^ AES_Td2[(t2 >>  8) & 0xff] ^ AES_Td3[t1 & 0xff] ^ rk[48];
            s1 = AES_Td0[t1 >> 24] ^ AES_Td1[(t0 >> 16) & 0xff] ^ AES_Td2[(t3 >>  8) & 0xff] ^ AES_Td3[t2 & 0xff] ^ rk[49];
            s2 = AES_Td0[t2 >> 24] ^ AES_Td1[(t1 >> 16) & 0xff] ^ AES_Td2[(t0 >>  8) & 0xff] ^ AES_Td3[t3 & 0xff] ^ rk[50];
            s3 = AES_Td0[t3 >> 24] ^ AES_Td1[(t2 >> 16) & 0xff] ^ AES_Td2[(t1 >>  8) & 0xff] ^ AES_Td3[t0 & 0xff] ^ rk[51];
            /* round 13: */
            t0 = AES_Td0[s0 >> 24] ^ AES_Td1[(s3 >> 16) & 0xff] ^ AES_Td2[(s2 >>  8) & 0xff] ^ AES_Td3[s1 & 0xff] ^ rk[52];
            t1 = AES_Td0[s1 >> 24] ^ AES_Td1[(s0 >> 16) & 0xff] ^ AES_Td2[(s3 >>  8) & 0xff] ^ AES_Td3[s2 & 0xff] ^ rk[53];
            t2 = AES_Td0[s2 >> 24] ^ AES_Td1[(s1 >> 16) & 0xff] ^ AES_Td2[(s0 >>  8) & 0xff] ^ AES_Td3[s3 & 0xff] ^ rk[54];
            t3 = AES_Td0[s3 >> 24] ^ AES_Td1[(s2 >> 16) & 0xff] ^ AES_Td2[(s1 >>  8) & 0xff] ^ AES_Td3[s0 & 0xff] ^ rk[55];
        }
    }
	rk += key->rounds << 2;
#else  /* !FULL_UNROLL */
    /*
     * Nr - 1 full rounds:
     */
    r = key->rounds >> 1;
    for (;;) {
        t0 =
            AES_Td0[(s0 >> 24)       ] ^
            AES_Td1[(s3 >> 16) & 0xff] ^
            AES_Td2[(s2 >>  8) & 0xff] ^
            AES_Td3[(s1      ) & 0xff] ^
            rk[4];
        t1 =
            AES_Td0[(s1 >> 24)       ] ^
            AES_Td1[(s0 >> 16) & 0xff] ^
            AES_Td2[(s3 >>  8) & 0xff] ^
            AES_Td3[(s2      ) & 0xff] ^
            rk[5];
        t2 =
            AES_Td0[(s2 >> 24)       ] ^
            AES_Td1[(s1 >> 16) & 0xff] ^
            AES_Td2[(s0 >>  8) & 0xff] ^
            AES_Td3[(s3      ) & 0xff] ^
            rk[6];
        t3 =
            AES_Td0[(s3 >> 24)       ] ^
            AES_Td1[(s2 >> 16) & 0xff] ^
            AES_Td2[(s1 >>  8) & 0xff] ^
            AES_Td3[(s0      ) & 0xff] ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }

        s0 =
            AES_Td0[(t0 >> 24)       ] ^
            AES_Td1[(t3 >> 16) & 0xff] ^
            AES_Td2[(t2 >>  8) & 0xff] ^
            AES_Td3[(t1      ) & 0xff] ^
            rk[0];
        s1 =
            AES_Td0[(t1 >> 24)       ] ^
            AES_Td1[(t0 >> 16) & 0xff] ^
            AES_Td2[(t3 >>  8) & 0xff] ^
            AES_Td3[(t2      ) & 0xff] ^
            rk[1];
        s2 =
            AES_Td0[(t2 >> 24)       ] ^
            AES_Td1[(t1 >> 16) & 0xff] ^
            AES_Td2[(t0 >>  8) & 0xff] ^
            AES_Td3[(t3      ) & 0xff] ^
            rk[2];
        s3 =
            AES_Td0[(t3 >> 24)       ] ^
            AES_Td1[(t2 >> 16) & 0xff] ^
            AES_Td2[(t1 >>  8) & 0xff] ^
            AES_Td3[(t0      ) & 0xff] ^
            rk[3];
    }
#endif /* ?FULL_UNROLL */
    /*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
   	s0 =
                (AES_Td4[(t0 >> 24)       ] & 0xff000000) ^
                (AES_Td4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Td4[(t2 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Td4[(t1      ) & 0xff] & 0x000000ff) ^
   		rk[0];
	PUTU32(out     , s0);
   	s1 =
                (AES_Td4[(t1 >> 24)       ] & 0xff000000) ^
                (AES_Td4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Td4[(t3 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Td4[(t2      ) & 0xff] & 0x000000ff) ^
   		rk[1];
	PUTU32(out +  4, s1);
   	s2 =
                (AES_Td4[(t2 >> 24)       ] & 0xff000000) ^
                (AES_Td4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Td4[(t0 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Td4[(t3      ) & 0xff] & 0x000000ff) ^
   		rk[2];
	PUTU32(out +  8, s2);
   	s3 =
                (AES_Td4[(t3 >> 24)       ] & 0xff000000) ^
                (AES_Td4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
                (AES_Td4[(t1 >>  8) & 0xff] & 0x0000ff00) ^
                (AES_Td4[(t0      ) & 0xff] & 0x000000ff) ^
   		rk[3];
	PUTU32(out + 12, s3);
}

#endif /* AES_ASM */

void AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
		     const unsigned long length, const AES_KEY *key,
		     unsigned char *ivec, const int enc)
{

	unsigned long n;
	unsigned long len = length;
	unsigned char tmp[AES_BLOCK_SIZE];

	assert(in && out && key && ivec);

	if (enc) {
		while (len >= AES_BLOCK_SIZE) {
			for(n=0; n < AES_BLOCK_SIZE; ++n)
				tmp[n] = in[n] ^ ivec[n];
			AES_encrypt(tmp, out, key);
			memcpy(ivec, out, AES_BLOCK_SIZE);
			len -= AES_BLOCK_SIZE;
			in += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
		}
		if (len) {
			for(n=0; n < len; ++n)
				tmp[n] = in[n] ^ ivec[n];
			for(n=len; n < AES_BLOCK_SIZE; ++n)
				tmp[n] = ivec[n];
			AES_encrypt(tmp, tmp, key);
			memcpy(out, tmp, AES_BLOCK_SIZE);
			memcpy(ivec, tmp, AES_BLOCK_SIZE);
		}
	} else {
		while (len >= AES_BLOCK_SIZE) {
			memcpy(tmp, in, AES_BLOCK_SIZE);
			AES_decrypt(in, out, key);
			for(n=0; n < AES_BLOCK_SIZE; ++n)
				out[n] ^= ivec[n];
			memcpy(ivec, tmp, AES_BLOCK_SIZE);
			len -= AES_BLOCK_SIZE;
			in += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
		}
		if (len) {
			memcpy(tmp, in, AES_BLOCK_SIZE);
			AES_decrypt(tmp, tmp, key);
			for(n=0; n < len; ++n)
				out[n] = tmp[n] ^ ivec[n];
			memcpy(ivec, tmp, AES_BLOCK_SIZE);
		}
	}
}

#endif
