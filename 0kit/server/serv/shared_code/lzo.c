#include "platform.h"
#include "types.h"

#include "lzo.h"

///!!!///
int LZODecompress(const uint8_t* in, uint32_t in_len, uint8_t* out, uint32_t* out_len)
{
	uint8_t* op;
	/*const */uint8_t* ip;
	uint32_t t;
	/*const */uint8_t* m_pos;

	const uint8_t* ip_end = in + in_len;
	uint8_t* const op_end = out + *out_len;

	*out_len = 0;

	op = out;
	ip = in;

	if (*ip > 17)
	{
		t = *ip++ - 17;
		if (t < 4)
			goto match_next;
		/*assert(t > 0); */NEED_OP(t); NEED_IP(t+1);
		do *op++ = *ip++; while (--t > 0);
		goto first_literal_run;
	}

	while (TEST_IP && TEST_OP) {
		t = *ip++;
		if (t >= 16)
			goto match;
		if (t == 0) {
			NEED_IP(1);
			while (*ip == 0) {
				t += 255;
				ip++;
				NEED_IP(1);
			}
			t += 15 + *ip++;
		}
		/*assert(t > 0); */NEED_OP(t+3); NEED_IP(t+4);
			COPY4(op,ip);
			op += 4; ip += 4;
			if (--t > 0)
			{
				if (t >= 4)
				{
					do {
						COPY4(op,ip);
						op += 4; ip += 4; t -= 4;
					} while (t >= 4);
					if (t > 0) do *op++ = *ip++; while (--t > 0);
				}
				else
					do *op++ = *ip++; while (--t > 0);
			}

first_literal_run:

		t = *ip++;
		if (t >= 16)
			goto match;

		m_pos = op - (1 + M2_MAX_OFFSET);
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;
		TEST_LB(m_pos); NEED_OP(3);
		*op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
		goto match_done;

		do {
match:
			if (t >= 64) {
				m_pos = op - 1;
				m_pos -= (t >> 2) & 7;
				m_pos -= *ip++ << 3;
				t = (t >> 5) - 1;
				TEST_LB(m_pos); /*assert(t > 0); */NEED_OP(t+3-1);
				goto copy_match;
			}
			else if (t >= 32)
			{
				t &= 31;
				if (t == 0)
				{
					NEED_IP(1);
					while (*ip == 0)
					{
						t += 255;
						ip++;
						NEED_IP(1);
					}
					t += 31 + *ip++;
				}

				m_pos = op - 1;
				m_pos -= (* (const uint16_t*) (const pvoid_t) ip) >> 2;
				ip += 2;
			}
			else if (t >= 16)
			{
				m_pos = op;
				m_pos -= (t & 8) << 11;
				t &= 7;
				if (t == 0)
				{
					NEED_IP(1);
					while (*ip == 0)
					{
						t += 255;
						ip++;
						NEED_IP(1);
					}
					t += 7 + *ip++;
				}

				m_pos -= (* (const uint16_t*) (const pvoid_t) ip) >> 2;
				ip += 2;
				if (m_pos == op)
					goto eof_found;
				m_pos -= 0x4000;

			}
			else
			{
				m_pos = op - 1;
				m_pos -= t >> 2;
				m_pos -= *ip++ << 2;
				TEST_LB(m_pos); NEED_OP(2);
				*op++ = *m_pos++; *op++ = *m_pos;

				goto match_done;
			}


			TEST_LB(m_pos); /*assert(t > 0); */NEED_OP(t+3-1);
			if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
			{
				COPY4(op,m_pos);
				op += 4; m_pos += 4; t -= 4 - (3 - 1);
				do {
					COPY4(op,m_pos);
					op += 4; m_pos += 4; t -= 4;
				} while (t >= 4);
				if (t > 0) do *op++ = *m_pos++; while (--t > 0);
			}
			else {
copy_match:
				*op++ = *m_pos++; *op++ = *m_pos++;
				do *op++ = *m_pos++; while (--t > 0);
			}

match_done:
			t = ip[-2] & 3;
			if (t == 0)
				break;

match_next:
			/*assert(t > 0); assert(t < 4); */NEED_OP(t); NEED_IP(t+1);
			*op++ = *ip++;
			if (t > 1) { *op++ = *ip++; if (t > 2) { *op++ = *ip++; } }
			t = *ip++;
		} while (TEST_IP && TEST_OP);
	}

// #if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
// 	*out_len = pd(op, out);
// 	return LZO_E_EOF_NOT_FOUND;
// #endif

eof_found:
	/*assert(t == 1);*/
	*out_len = pd(op, out);
	return (ip == ip_end ? LZO_E_OK :
		(ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));

input_overrun:
	*out_len = pd(op, out);
	return LZO_E_INPUT_OVERRUN;

output_overrun:
	*out_len = pd(op, out);
	return LZO_E_OUTPUT_OVERRUN;

lookbehind_overrun:
	*out_len = pd(op, out);
	return LZO_E_LOOKBEHIND_OVERRUN;
}
