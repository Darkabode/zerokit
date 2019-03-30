#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define CKEYGEN_VERSION "0.3.4"

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/config.h"
#include "../../../shared_code/utils.h"
#include "../../../shared_code/havege.h"
#include "../../../shared_code/bignum.h"
#include "../../../shared_code/bn_mul.h"
#include "../../../shared_code/rsa.h"
#include "../../../shared_code/arc4.h"
#include "../../../shared_code/md5.h"
#include "../../../shared_code/sha1.h"


#define MPI_USE_FILE_IO 1
#define USE_PRIME_NUMBERS 1
#define USE_STRING_IO 1
#define USE_STRING_WRITE_IO 1
#include "../../../shared_code/bignum.c"
#include "../../../shared_code/rsa.c"

#include "../../../shared_code/havege.c"

#define KEY_SIZE 4096
#define EXPONENT 65537

#define RSA_FILE_PUBLIC "key.public"
#define RSA_FILE_PRIVATE "key.private"

int __cdecl main(int argc, char** argv)
{
    int ret = EXIT_FAILURE;
    rsa_context_t rsa;
    havege_state_t hs;
    FILE *fpub  = NULL;
    FILE *fpriv = NULL;

    havege_init(&hs);
    rsa_init(&rsa, RSA_PKCS_V15, 0);

    fprintf(stdout, "Generating the RSA key (%d-bit)... ", KEY_SIZE);
    if ((ret = rsa_gen_key(&rsa, havege_rand, &hs, KEY_SIZE, EXPONENT)) != 0) {
        fprintf(stdout, "Failed\n");
        goto exit;
    }

    fprintf(stdout, "OK\nExporting the public key in %s... ", RSA_FILE_PUBLIC);
    if ((fpub = fopen(RSA_FILE_PUBLIC, "wb+")) == NULL) {
        fprintf(stdout, "Failed\n  ! Could not open %s for writing\n\n", RSA_FILE_PUBLIC);
        goto exit;
    }

    if ((ret = mpi_write_file("N=", &rsa.N, 16, fpub)) != 0 || (ret = mpi_write_file("E=", &rsa.E, 16, fpub)) != 0) {
        fprintf(stdout, "Failed\n");
        goto exit;
    }

    fprintf(stdout,  "OK\nExporting the private key in %s... ", RSA_FILE_PRIVATE);

    if ((fpriv = fopen(RSA_FILE_PRIVATE, "wb+")) == NULL) {
        fprintf(stdout, "Failed\n  ! Could not open %s for writing\n", RSA_FILE_PRIVATE);
        goto exit;
    }

    if ((ret = mpi_write_file("N=" , &rsa.N , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("E=" , &rsa.E , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("D=" , &rsa.D , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("P=" , &rsa.P , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("Q=" , &rsa.Q , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("DP=", &rsa.DP, 16, fpriv)) != 0 ||
        (ret = mpi_write_file("DQ=", &rsa.DQ, 16, fpriv)) != 0 ||
        (ret = mpi_write_file("QP=", &rsa.QP, 16, fpriv)) != 0)
    {
        fprintf(stdout, "Failed\n");
        goto exit;
    }

	fprintf(stdout, "OK\n\n");

	ret = EXIT_SUCCESS;
exit:

    if (fpub  != NULL)
        fclose(fpub);

    if (fpriv != NULL)
        fclose(fpriv);

    rsa_free(&rsa);

    return ret;
}