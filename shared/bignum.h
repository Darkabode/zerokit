#ifndef __SHARED_BIGNUM_H_
#define __SHARED_BIGNUM_H_

#define POLARSSL_ERR_MPI_FILE_IO_ERROR                     -0x0002  /**< An error occurred while reading from or writing to a file. */
#define POLARSSL_ERR_MPI_BAD_INPUT_DATA                    -0x0004  /**< Bad input parameters to function. */
#define POLARSSL_ERR_MPI_INVALID_CHARACTER                 -0x0006  /**< There is an invalid character in the digit string. */
#define POLARSSL_ERR_MPI_BUFFER_TOO_SMALL                  -0x0008  /**< The output buffer is too small to write too. */
#define POLARSSL_ERR_MPI_NEGATIVE_VALUE                    -0x000A  /**< The input arguments are negative or result in illegal output. */
#define POLARSSL_ERR_MPI_DIVISION_BY_ZERO                  -0x000C  /**< The input argument for division is zero, which is not allowed. */
#define POLARSSL_ERR_MPI_NOT_ACCEPTABLE                    -0x000E  /**< The input arguments are not acceptable. */
#define POLARSSL_ERR_MPI_MALLOC_FAILED                     -0x0010  /**< Memory allocation failed. */

#define MPI_CHK(f) if( ( ret = f ) != 0 ) goto cleanup

/*
 * Maximum size MPIs are allowed to grow to in number of limbs.
 */
#define POLARSSL_MPI_MAX_LIMBS                             10000

/*
 * Maximum window size used for modular exponentiation. Default: 6
 * Minimum value: 1. Maximum value: 6.
 *
 * Result is an array of ( 2 << POLARSSL_MPI_WINDOW_SIZE ) MPIs used
 * for the sliding window calculation. (So 64 by default)
 *
 * Reduction in size, reduces speed.
 */
#define POLARSSL_MPI_WINDOW_SIZE                           6        /**< Maximum windows size used. */

/*
 * Maximum size of MPIs allowed in bits and bytes for user-MPIs.
 * ( Default: 512 bytes => 4096 bits )
 *
 * Note: Calculations can results temporarily in larger MPIs. So the number
 * of limbs required (POLARSSL_MPI_MAX_LIMBS) is higher.
 */
#define POLARSSL_MPI_MAX_SIZE                              512      /**< Maximum number of bytes for usable MPIs. */
#define POLARSSL_MPI_MAX_BITS                              ( 8 * POLARSSL_MPI_MAX_SIZE )    /**< Maximum number of bits for usable MPIs. */

/*
 * When reading from files with mpi_read_file() the buffer should have space
 * for a (short) label, the MPI (in the provided radix), the newline
 * characters and the '\0'.
 *
 * By default we assume at least a 10 char label, a minimum radix of 10
 * (decimal) and a maximum of 4096 bit numbers (1234 decimal chars).
 */
#define POLARSSL_MPI_READ_BUFFER_SIZE                       1250   


/**
 * \brief          MPI structure
 */
typedef struct _mpi
{
    int s;        // integer sign
    uint32_t n;    // total # of limbs
    uint32_t* p;    // pointer to limbs
} mpi_t;

#ifndef MPI_INIT
#define MPI_INIT mpi_init
#endif // MPI_INIT

#ifndef MPI_FREE
#define MPI_FREE mpi_free
#endif // MPI_FREE

#ifndef MPI_GROW
#define MPI_GROW mpi_grow
#endif // MPI_GROW

#ifndef MPI_COPY
#define MPI_COPY mpi_copy
#endif // MPI_COPY

#ifndef MPI_SWAP
#define MPI_SWAP mpi_swap
#endif // MPI_SWAP

#ifndef MPI_LSET
#define MPI_LSET mpi_lset
#endif // MPI_LSET

#ifndef MPI_LSB
#define MPI_LSB mpi_lsb
#endif // MPI_LSB

#ifndef MPI_MSB
#define MPI_MSB mpi_msb
#endif // MPI_MSB

#ifndef MPI_SIZE
#define MPI_SIZE mpi_size
#endif // MPI_SIZE

#ifndef MPI_GET_DIGIT
#define MPI_GET_DIGIT mpi_get_digit
#endif // MPI_GET_DIGIT

#ifndef MPI_MUL_HLP
#define MPI_MUL_HLP mpi_mul_hlp
#endif // MPI_MUL_HLP

#ifndef MPI_MUL_MPI
#define MPI_MUL_MPI mpi_mul_mpi
#endif // MPI_MUL_MPI

#ifndef MPI_MUL_INT
#define MPI_MUL_INT mpi_mul_int
#endif // MPI_MUL_INT

#ifndef MPI_CMP_ABS
#define MPI_CMP_ABS mpi_cmp_abs
#endif // MPI_CMP_ABS

#ifndef MPI_SUB_HLP
#define MPI_SUB_HLP mpi_sub_hlp
#endif // MPI_SUB_HLP

#ifndef MPI_SUB_ABS
#define MPI_SUB_ABS mpi_sub_abs
#endif // MPI_SUB_ABS

#ifndef MPI_ADD_ABS
#define MPI_ADD_ABS mpi_add_abs
#endif // MPI_ADD_ABS

#ifndef MPI_ADD_MPI
#define MPI_ADD_MPI mpi_add_mpi
#endif // MPI_ADD_MPI

#ifndef MPI_ADD_INT
#define MPI_ADD_INT mpi_add_int
#endif // MPI_ADD_INT

#ifndef MPI_SUB_MPI
#define MPI_SUB_MPI mpi_sub_mpi
#endif // MPI_SUB_MPI

#ifndef MPI_SUB_INT
#define MPI_SUB_INT mpi_sub_int
#endif // MPI_SUB_INT

#ifndef MPI_MOD_INT
#define MPI_MOD_INT mpi_mod_int
#endif // MPI_MOD_INT

#ifndef MPI_CMP_MPI
#define MPI_CMP_MPI mpi_cmp_mpi
#endif // MPI_CMP_MPI

#ifndef MPI_CMP_INT
#define MPI_CMP_INT mpi_cmp_int
#endif // MPI_CMP_INT

#ifndef MPI_SHIFT_L
#define MPI_SHIFT_L mpi_shift_l
#endif // MPI_SHIFT_L

#ifndef MPI_SHIFT_R
#define MPI_SHIFT_R mpi_shift_r
#endif // MPI_SHIFT_R

#ifndef MPI_DIV_MPI
#define MPI_DIV_MPI mpi_div_mpi
#endif // MPI_DIV_MPI

#ifndef MPI_DIV_INT
#define MPI_DIV_INT mpi_div_int
#endif // MPI_DIV_INT

#ifndef MPI_MOD_MPI
#define MPI_MOD_MPI mpi_mod_mpi
#endif // MPI_MOD_MPI

#ifndef MPI_MONTG_INIT
#define MPI_MONTG_INIT mpi_montg_init
#endif // MPI_MONTG_INIT

#ifndef MPI_MONTMUL
#define MPI_MONTMUL mpi_montmul
#endif // MPI_MONTMUL

#ifndef MPI_MONTRED
#define MPI_MONTRED mpi_montred
#endif // MPI_MONTRED

#ifndef MPI_EXP_MOD
#define MPI_EXP_MOD mpi_exp_mod
#endif // MPI_EXP_MOD

#ifndef MPI_GCD
#define MPI_GCD mpi_gcd
#endif // MPI_GCD

#ifndef MPI_READ_BINARY
#define MPI_READ_BINARY mpi_read_binary
#endif // MPI_READ_BINARY

#ifndef MPI_WRITE_BINARY
#define MPI_WRITE_BINARY mpi_write_binary
#endif // MPI_WRITE_BINARY

#endif // __SHARED_BIGNUM_H_
