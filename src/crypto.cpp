extern "C" {
#include <mbedtls/psa_util.h>
}
#include <mongoose/mongoose.h>
#include <plog/Log.h>

extern "C" void mg_random(void *buf, size_t len) {
    int ret = psa_crypto_init();
    if(PSA_SUCCESS != ret) {
        PLOG_FATAL << "Cannot initialize PSA crypto.";
        PLOG_INFO << "Error value: " << ret;
        exit(1);
    }

    ret = mbedtls_psa_get_random(NULL, (unsigned char *)buf, len);
    if(0 != ret) {
        PLOG_FATAL << "Cannot generate random number.";
        PLOG_INFO << "Error value: " << ret;
        exit(1);
    }
}