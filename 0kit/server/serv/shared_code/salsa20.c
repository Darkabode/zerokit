#include "types.h"
#include "salsa20.h"

uint32_t rot_left(uint32_t x, const int s)
{
	return (x << s) | (x >> (sizeof(uint32_t)*8 - s));
}


#define ROTATE(v, c) (rot_left(v, c))
#define XOR(v,w) ((v) ^ (w))
#define PLUS(v,w) ((uint32_t)((v) + (w)))
#define PLUSONE(v) (PLUS((v), 1))

static void salsa20_wordtobyte(uint8_t output[64], const uint32_t input[16])
{
	uint32_t x[16];
	int i;

	for (i = 0; i < 16; ++i)
		x[i] = input[i];
	for (i = 20; i > 0; i -= 2) {
		x[ 4] = XOR(x[ 4], ROTATE(PLUS(x[ 0], x[12]), 7));
		x[ 8] = XOR(x[ 8], ROTATE(PLUS(x[ 4], x[ 0]), 9));
		x[12] = XOR(x[12], ROTATE(PLUS(x[ 8], x[ 4]),13));
		x[ 0] = XOR(x[ 0], ROTATE(PLUS(x[12], x[ 8]),18));
		x[ 9] = XOR(x[ 9], ROTATE(PLUS(x[ 5], x[ 1]), 7));
		x[13] = XOR(x[13], ROTATE(PLUS(x[ 9], x[ 5]), 9));
		x[ 1] = XOR(x[ 1], ROTATE(PLUS(x[13], x[ 9]),13));
		x[ 5] = XOR(x[ 5], ROTATE(PLUS(x[ 1], x[13]),18));
		x[14] = XOR(x[14], ROTATE(PLUS(x[10], x[ 6]), 7));
		x[ 2] = XOR(x[ 2], ROTATE(PLUS(x[14], x[10]), 9));
		x[ 6] = XOR(x[ 6], ROTATE(PLUS(x[ 2], x[14]),13));
		x[10] = XOR(x[10], ROTATE(PLUS(x[ 6], x[ 2]),18));
		x[ 3] = XOR(x[ 3], ROTATE(PLUS(x[15], x[11]), 7));
		x[ 7] = XOR(x[ 7], ROTATE(PLUS(x[ 3], x[15]), 9));
		x[11] = XOR(x[11], ROTATE(PLUS(x[ 7], x[ 3]),13));
		x[15] = XOR(x[15], ROTATE(PLUS(x[11], x[ 7]),18));
		x[ 1] = XOR(x[ 1], ROTATE(PLUS(x[ 0], x[ 3]), 7));
		x[ 2] = XOR(x[ 2], ROTATE(PLUS(x[ 1], x[ 0]), 9));
		x[ 3] = XOR(x[ 3], ROTATE(PLUS(x[ 2], x[ 1]),13));
		x[ 0] = XOR(x[ 0], ROTATE(PLUS(x[ 3], x[ 2]),18));
		x[ 6] = XOR(x[ 6], ROTATE(PLUS(x[ 5], x[ 4]), 7));
		x[ 7] = XOR(x[ 7], ROTATE(PLUS(x[ 6], x[ 5]), 9));
		x[ 4] = XOR(x[ 4], ROTATE(PLUS(x[ 7], x[ 6]),13));
		x[ 5] = XOR(x[ 5], ROTATE(PLUS(x[ 4], x[ 7]),18));
		x[11] = XOR(x[11], ROTATE(PLUS(x[10], x[ 9]), 7));
		x[ 8] = XOR(x[ 8], ROTATE(PLUS(x[11], x[10]), 9));
		x[ 9] = XOR(x[ 9], ROTATE(PLUS(x[ 8], x[11]),13));
		x[10] = XOR(x[10], ROTATE(PLUS(x[ 9], x[ 8]),18));
		x[12] = XOR(x[12], ROTATE(PLUS(x[15], x[14]), 7));
		x[13] = XOR(x[13], ROTATE(PLUS(x[12], x[15]), 9));
		x[14] = XOR(x[14], ROTATE(PLUS(x[13], x[12]),13));
		x[15] = XOR(x[15], ROTATE(PLUS(x[14], x[13]),18));
	}
	for (i = 0; i < 16; ++i)
		x[i] = PLUS(x[i], input[i]);
	for (i = 0; i < 16; ++i)
		((uint32_t*)(output + 4 * i))[0] = x[i];
}

static const char sigma[16] = {0x79,0x97,0x11,0x25,0x85,0x07,0x04,0x88,0x65,0x77,0x77,0x79,0x99,0xDE,0xFA,0x11};
static const char tau[16] =   {0x0F,0x1E,0x2D,0x3C,0x4B,0x5A,0x69,0x78,0x21,0x34,0x55,0x89,0x79,0xAA,0xBB,0xCC};

void salsa20_key_setup(salsa20_context_t* x, const uint8_t* k, uint32_t kbits)
{
	static const char* constants;

	x->input[1] = *(uint32_t*)(k + 0);
	x->input[2] = *(uint32_t*)(k + 4);
	x->input[3] = *(uint32_t*)(k + 8);
	x->input[4] = *(uint32_t*)(k + 12);
	if (kbits == 256) { /* recommended */
		k += 16;
		constants = sigma;
	}
	else { /* kbits == 128 */
		constants = tau;
	}
	x->input[11] = *(uint32_t*)(k + 0);
	x->input[12] = *(uint32_t*)(k + 4);
	x->input[13] = *(uint32_t*)(k + 8);
	x->input[14] = *(uint32_t*)(k + 12);
	x->input[0] = *(uint32_t*)(constants + 0);
	x->input[5] = *(uint32_t*)(constants + 4);
	x->input[10] = *(uint32_t*)(constants + 8);
	x->input[15] = *(uint32_t*)(constants + 12);
}
/*
void Salsa20IvSetup(salsa20_context_t* x, const uint8_t* iv)
{
	x->input[6] = *(uint32_t*)(iv + 0);
	x->input[7] = *(uint32_t*)(iv + 4);
	x->input[8] = 0;
	x->input[9] = 0;
}
*/
uint8_t* salsa20_encrypt(salsa20_context_t* x, const uint8_t* m, uint8_t* c, uint32_t bytes)
{
	uint8_t output[64];
	uint8_t i;

	if (!bytes)
		return (uint8_t*)m;
	for ( ; ; ) {
		salsa20_wordtobyte(output,x->input);
		x->input[8] = PLUSONE(x->input[8]);
		if (!x->input[8]) {
			x->input[9] = PLUSONE(x->input[9]);
			/* stopping at 2^70 bytes per nonce is user's responsibility */
		}
		if (bytes <= 64) {
			for (i = 0; i < bytes; ++i)
				c[i] = m[i] ^ output[i];
			return c;
		}
		for (i = 0; i < 64; ++i)
			c[i] = m[i] ^ output[i];
		bytes -= 64;
		c += 64;
		m += 64;
	}
}

void salsa20_decrypt(salsa20_context_t* x, const uint8_t* c, uint8_t* m, uint32_t bytes)
{
	salsa20_encrypt(x, c, m, bytes);
}
