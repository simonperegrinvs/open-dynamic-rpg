#pragma once

// Stable C boundary shared by command-line and Unreal hosts. Returned strings
// remain valid until the next call on the same session. The host owns the handle.
#ifdef __cplusplus
extern "C" {
#endif

typedef struct OdrSession OdrSession;

OdrSession* odr_create(const char* definition_json);
void odr_destroy(OdrSession* session);
int odr_apply(OdrSession* session, const char* command_json);
int odr_load(OdrSession* session, const char* save_json);
const char* odr_snapshot(OdrSession* session);
const char* odr_save(OdrSession* session);
const char* odr_error(OdrSession* session);

#ifdef __cplusplus
}
#endif
