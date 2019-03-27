#ifndef __SALSA20_H_
#define __SALSA20_H_

typedef struct
{
  uint32_t input[16];
} salsa20_context_t;

void salsa20_key_setup(salsa20_context_t* x, const uint8_t* k, uint32_t kbits);
/*
void Salsa20IvSetup(salsa20_context_t* x, const uint8_t* iv)
{
	x->input[6] = *(uint32_t*)(iv + 0);
	x->input[7] = *(uint32_t*)(iv + 4);
	x->input[8] = 0;
	x->input[9] = 0;
}
*/
uint8_t* salsa20_encrypt(salsa20_context_t* x, const uint8_t* m, uint8_t* c, uint32_t bytes);
void salsa20_decrypt(salsa20_context_t* x, const uint8_t* c, uint8_t* m, uint32_t bytes);

#endif // __SALSA20_H_
