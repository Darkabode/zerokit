#ifndef __SHARED_LZO_H_
#define __SHARED_LZO_H_

/////////////////////////////////////////// lzo
#define NEED_OP(x) \
	if ((uint32_t)(op_end - op) < (uint32_t)(x))  goto output_overrun

#define NEED_IP(x) \
	if ((uint32_t)(ip_end - ip) < (uint32_t)(x))  goto input_overrun

#  define TEST_IP               1
#  define TEST_OP               1

#define M2_MAX_OFFSET   0x0800

#  define TEST_LB(m_pos)        if (m_pos < out || m_pos >= op) goto lookbehind_overrun
#  define TEST_LBO(m_pos,o)     if (m_pos < out || m_pos >= op - (o)) goto lookbehind_overrun

#define COPY4(d,s)  (*(uint32_t*)(pvoid_t)(d) = *(const uint32_t*)(const pvoid_t)(s))

#define pd(a,b)             ((uint32_t) ((a)-(b)))


/* Error codes for the compression/decompression functions. Negative
 * values are errors, positive values will be used for special but
 * normal events.
 */
#define LZO_E_OK                    0
#define LZO_E_ERROR                 (-1)
#define LZO_E_OUT_OF_MEMORY         (-2)    /* [not used right now] */
#define LZO_E_NOT_COMPRESSIBLE      (-3)    /* [not used right now] */
#define LZO_E_INPUT_OVERRUN         (-4)
#define LZO_E_OUTPUT_OVERRUN        (-5)
#define LZO_E_LOOKBEHIND_OVERRUN    (-6)
#define LZO_E_EOF_NOT_FOUND         (-7)
#define LZO_E_INPUT_NOT_CONSUMED    (-8)
#define LZO_E_NOT_YET_IMPLEMENTED   (-9)    /* [not used right now] */


int LZODecompress(const uint8_t* in, uint32_t in_len, uint8_t* out, uint32_t* out_len);

#endif // __SHARED_LZO_H_
