#ifndef _DRYOS_HAL_H
#define _DRYOS_HAL_H

#include <stdio.h>
#include <stdint.h>

//-------------------------------------------------------------------------------------------------------------
// Common
//-------------------------------------------------------------------------------------------------------------

int uart_printf(const char* pFormat, ...);
void printError(const char* pErrorMsg);
void reboot();

//-------------------------------------------------------------------------------------------------------------
// Sockets
//-------------------------------------------------------------------------------------------------------------

#define DRY_AF_INET 0x1
#define DRY_SOCK_STREAM 1
#define DRY_INADDR_ANY 0 // TODO: verify
#define DRY_SOL_SOCKET 0xFFFF // TODO: verify
#define DRY_SO_REUSEADDR 0x8004 // TODO: verify
#define DRY_SO_REUSEPORT 0x8005 // TODO: verify

#define socklen_t unsigned int

typedef struct {
  unsigned int s_addr;
} DryInAddr;

typedef struct {
  short sin_family;
  unsigned short sin_port;
  DryInAddr sin_addr;
} DrySockaddr_in;

typedef struct {
  unsigned short sa_family;
  char sa_data[8];
} DrySockaddr;

typedef struct {
  int lo;
  int hi;
} DryOpt_t;

uint16_t htons(uint16_t hostshort);

int* errno_get_pointer_to(void);

int socket_create(int domain, int type, int protocol);
int socket_setsockopt(int sockfd, int level, int optname, DryOpt_t* optval, socklen_t optlen);
int socket_bind(int sockfd, DrySockaddr_in* addr, socklen_t addrlen);
int socket_listen(int sockfd, int backlogl);
int socket_accept(int sockfd, DrySockaddr_in* addr, socklen_t* addrlen);
int socket_recv(int sockfd, void* buf, size_t len, int flags);
int socket_send(int sockfd, const void* buf, size_t len, int flags);

int wifiConnect();

#define O_RDONLY             00
#define O_WRONLY             01
#define O_RDWR               02
#define O_CREAT            0100 /* not fcntl */
#define O_EXCL             0200 /* not fcntl */
#define O_NOCTTY           0400 /* not fcntl */
#define O_TRUNC           01000 /* not fcntl */
#define O_APPEND          02000
#define O_NONBLOCK        04000
#define O_NDELAY        O_NONBLOCK
#define O_SYNC           010000
#define O_FSYNC          O_SYNC
#define O_ASYNC          020000

typedef struct {
  uint32_t lo;
  uint32_t hi;
} size64_t;

//-------------------------------------------------------------------------------------------------------------
// File I/O
//-------------------------------------------------------------------------------------------------------------

FILE* FIO_CreateFile(const char* pFileName);
FILE* FIO_OpenFile(const char* pFileName, uint32_t mode);
int FIO_CloseFile(FILE* pStream);
int FIO_ReadFile(FILE* pFile, void* pBuffer, size_t count);
int FIO_WriteFile(FILE* pStream, const void* ptr, size_t count);
int FIO_RemoveFile(const char* pFileName);
int _FIO_RenameFile(const char* pSrc, const char* pDst);
int FIO_GetFileSize(const char* pFileName, size64_t* pSize);

//-------------------------------------------------------------------------------------------------------------
// MD5
//-------------------------------------------------------------------------------------------------------------

typedef struct {
  uint pBitlen[2];
  uint pH[4];
  uint8_t pBuffer[64];
} Md5Ctx;

void Md5_Init(Md5Ctx* pCtx);
void Md5_Update(Md5Ctx* pCtx, void* pData, size_t size);
void Md5_Final(Md5Ctx *pCtx, uint8_t* pMd5HashOut);

int Md5_AllocAndInit(Md5Ctx** pCtx);
void Md5_FinalAndFree(Md5Ctx* pMd5Ctx, uint8_t* pMd5HashOut);

//-------------------------------------------------------------------------------------------------------------
// SHA-256
//-------------------------------------------------------------------------------------------------------------

int Sha256Init(void** ppSha256Ctx);
int ShaXUpdate(void* pCtx, void* pTransformFunction, uint8_t* pData, size_t size);
void Sha256_Transform(void* pData, uint32_t* pH);
int ShaXFinal(void* pCtx, void* pTransFormFunction, uint8_t* pFinalHash);
void ShaXFree(void** ppSha256Ctx);

#endif // _DRYOS_HAL_H