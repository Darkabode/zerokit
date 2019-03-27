#ifndef __SHARED_VECTOR_H_
#define __SHARED_VECTOR_H_

#define cvec_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

#define cvec_t(type) struct { size_t n, m; type *a; }
#define cvec_init(v) ((v).n = (v).m = 0, (v).a = 0)
#define cvec_destroy(v) fn_memfree((v).a)
#define cvec_A(v, i) ((v).a[(i)])
#define cvec_pop(v) ((v).a[--(v).n])
#define cvec_size(v) ((v).n)
#define cvec_max(v) ((v).m)

#define cvec_resize(type, v, s)  ((v).m = (s), (v).a = (type*)fn_memrealloc((v).a, sizeof(type) * (v).m))

#define cvec_push(type, v, x) \
    if ((v).n == (v).m) { \
        (v).m = (v).m ? (v).m << 1 : 2; \
        (v).a = (type*)fn_memrealloc((v).a, sizeof(type) * (v).m); \
    } \
    (v).a[(v).n++] = (x)

#define cvec_pushp(type, v) ((((v).n == (v).m) ? ((v).m = ((v).m ? (v).m << 1 : 2), (v).a = (type*)fn_memrealloc((v).a, sizeof(type) * (v).m), 0) : 0), ((v).a + ((v).n++)))

#define cvec_add(type, v, i) ((v).m <= (size_t)(i)?						\
    ((v).m = (v).n = (i) + 1, cvec_roundup32((v).m), \
    (v).a = (type*)fn_memrealloc((v).a, sizeof(type) * (v).m), 0) \
    : (v).n <= (size_t)(i)? (v).n = (i)			\
    : 0), (v).a[(i)]

#endif // __SHARED_VECTOR_H_
