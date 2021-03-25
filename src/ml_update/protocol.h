
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include <stdint.h>

typedef enum {
  ANNOUNCE_STATUS_OK = 0,
  ANNOUNCE_STATUS_UNSUPPORTED_VERSION = 1,
} AnnounceStatus_t;

typedef struct {
  char pFileName[64];
  uint64_t fileSize;
  uint8_t pSha256Hash[32];
  int protocolVersion;
} AnnounceFileReqMsg_t;

typedef struct {
  uint32_t status;
} AnnounceFileRspMsg_t;

#endif // _PROTOCOL_H