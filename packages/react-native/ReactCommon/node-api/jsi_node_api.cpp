#include <napi.h>

NAPI_EXTERN NAPI_NO_RETURN void NAPI_CDECL
napi_fatal_error(const char* location,
                 size_t location_len,
                 const char* message,
                 size_t message_len) {
  printf("NAPI FATAL ERROR: %s at %s", message, location);
  abort();
}
