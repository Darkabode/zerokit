#ifndef __SHARED_TINF_H_
#define __SHARED_TINF_H_

typedef struct _tinf_tree
{
   uint16_t table[16];  /* table of code length counts */
   uint16_t trans[288]; /* code -> symbol translation table */
} tinf_tree_t, *ptinf_tree_t;

typedef struct _tinf_data
{
   const uint8_t* source;
   uint32_t tag;
   uint32_t bitcount;

   uint8_t* dest;
   uint32_t* destLen;

   tinf_tree_t ltree; /* dynamic length/symbol tree */
   tinf_tree_t dtree; /* dynamic distance tree */
} tinf_data_t, *ptinf_data_t;

#endif // __SHARED_TINF_H_
