#include <mongoose/mongoose.h>
#include <plog/Log.h>
#include <psa/crypto.h>

extern "C" void mg_random(void *buf, size_t len) {
    int ret;
    ret = psa_generate_random((unsigned char *)buf, len);
    if(0 != ret) {
        PLOG_FATAL << "Cannot generate random number.";
        PLOG_INFO << "Error value: " << ret;
        exit(1);
    }
}