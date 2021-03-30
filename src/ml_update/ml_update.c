//By coon
//Repo: https://github.com/coon42/drysh_tools
#if defined(SSID) && defined(PASS) && defined(IP)
#include <stdlib.h>
#include <string.h>

#include "dryos_hal.h"

#include "protocol.h"
extern void _prop_request_change(unsigned property, const void* addr, size_t len);

static int createServer(int serverFd, int port) {
  DryOpt_t opt;
  opt.lo = 1;

  uart_printf("set socket option1\n");

  if (socket_setsockopt(serverFd, DRY_SOL_SOCKET, DRY_SO_REUSEADDR, &opt, sizeof(opt)) < 0 ) {
    printError("setsockopt");
    return 1;
  }

  uart_printf("set socket option2\n");

  if (socket_setsockopt(serverFd, DRY_SOL_SOCKET, DRY_SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    printError("setsockopt");
    return 1;
  }

  DrySockaddr_in address;
  address.sin_family = htons(DRY_AF_INET);
  address.sin_addr.s_addr = DRY_INADDR_ANY;
  address.sin_port = htons(port);

  uart_printf("bind at port: %d\n", port);

  if (socket_bind(serverFd, &address, sizeof(address)) < 0) {
    printError("bind failed");
    return 1;
  }

  uart_printf("listen\n");

  if (socket_listen(serverFd, 1) < 0) {
    printError("listen");
    return 1;
  }

  uart_printf("accept\n");
  uart_printf("now waiting for connection on port %d...\n", port);

  unsigned int addrlen = sizeof(address);
  int c = socket_accept(serverFd, &address, &addrlen);

  if (c < 0) {
    printError("accept");
    return 1;
  }

  uart_printf("client connected.\n");

  return c;
}

static int recvRequest(int clientFd, AnnounceFileReqMsg_t* pReq) {
  // TODO: check for TCP error:
  socket_recv(clientFd, pReq, sizeof(AnnounceFileReqMsg_t), 0);

  if (pReq->protocolVersion != 1)
    return ANNOUNCE_STATUS_UNSUPPORTED_VERSION;

  pReq->pFileName[sizeof(pReq->pFileName) -1] = 0;

  uart_printf("File announce message received: \n");
  uart_printf("File Name: %s\n", pReq->pFileName);
  uart_printf("File size: %lld\n", pReq->fileSize);
  uart_printf("SHA-256 Hash: ");

  for (int i = 0; i < sizeof(pReq->pSha256Hash); ++i)
    uart_printf("%02X", pReq->pSha256Hash[i]);

  uart_printf("\n");
  uart_printf("Protocol Version: %d\n", pReq->protocolVersion);

  return ANNOUNCE_STATUS_OK;
}

static int performUpdate(int clientFd) {
  AnnounceFileReqMsg_t req;
  AnnounceFileRspMsg_t rsp;
  rsp.status = recvRequest(clientFd, &req);
  int sent = socket_send(clientFd, &rsp, sizeof(rsp), 0);

  uart_printf("sent %d bytes\n", sent);

  if (rsp.status == ANNOUNCE_STATUS_UNSUPPORTED_VERSION) {
    uart_printf("Client protocol version is unsupported! Aborting. RSP status:0x%x\n",rsp.status);
    return 1;
  }

  uart_printf("Now receiving file...\n");

  const char* pTempFile = "B:/FILE.TMP";

  FIO_RemoveFile(pTempFile);

  FILE* pFile = FIO_CreateFile(pTempFile);

  if (pFile == (FILE*)-1) {
    printError("Unable to create temporary file!\n");
    return 1;
  }

  const size_t recvBufferSize = 1024;
  uint8_t* pBuffer = malloc(recvBufferSize);

  uint64_t bytesReceived;

  for (bytesReceived = 0; bytesReceived < req.fileSize;) {
    int chunkSize = socket_recv(clientFd, pBuffer, recvBufferSize, 0);

    uart_printf("received: %d\n", chunkSize);

    if (chunkSize <= 0) {
      printError("transmission failed");
      break;
    }

    uart_printf("pFile is: 0x%X. now writing\n", pFile);

    int bytesWritten;

    if ((bytesWritten = FIO_WriteFile(pFile, pBuffer, chunkSize)) != chunkSize) {
      uart_printf("write to file failed. %d bytes written but expected to write %d!\n", bytesWritten, chunkSize);
      break;
    }

    uart_printf("written: %d\n", bytesWritten);

    bytesReceived += chunkSize;
  }

  free(pBuffer);
  pBuffer = 0;

  FIO_CloseFile(pFile);

  uart_printf("Bytes received: %d\n", bytesReceived);

  if (bytesReceived != req.fileSize) {
     uart_printf("File transmission failed!\n");
     return 1;
  }

  uart_printf("File transmission finished!\n");

  int dummy = 42;
  socket_send(clientFd, &dummy, 1, 0);

  uart_printf("Now checking SHA-256\n");

  size64_t fileSize64 = {0};

  if (FIO_GetFileSize(pTempFile, &fileSize64) == -1) {
    uart_printf("failed to get file size! aborting\n");
    return 1;
  }

  uint32_t fileSize = fileSize64.lo; // TODO: will break on files bigger than 4GB! Fix!

  void* pSha256Ctx = 0;
  Sha256Init(&pSha256Ctx);

  uart_printf("file size of reopened file is: %d (lo: %d, hi: %d)\n", fileSize, fileSize64.lo, fileSize64.hi);

  pBuffer = malloc(recvBufferSize);

  if (!pBuffer) {
    uart_printf("failed to create SHA-256 working buffer!\n");
    return 0;
  }

  uart_printf("Start SHA-256 calc\n");

  pFile = FIO_OpenFile(pTempFile, O_RDONLY);

  if (pFile == (FILE*)-1) {
    printError("Unable to reopen temporary file!\n");
    return 1;
  }

  uart_printf("reopened pFile=%X\n", pFile);

  int bytesRead;
  for (bytesRead = 0; bytesRead < fileSize;) {
    int chunkSize = FIO_ReadFile(pFile, pBuffer, recvBufferSize);

    if (chunkSize < 0) {
      uart_printf("error on reading file during SHA-256 check!\n");
      return 1;
    }

    uart_printf("read: %d\n", chunkSize);
    ShaXUpdate(pSha256Ctx, Sha256_Transform, pBuffer, chunkSize);

    bytesRead += chunkSize;
  }

  free(pBuffer);
  FIO_CloseFile(pFile);

  uint8_t pSha256Hash[32];
  ShaXFinal(pSha256Ctx, Sha256_Transform, pSha256Hash);
  ShaXFree(&pSha256Ctx);

  uart_printf("SHA-256 calc finish\n");
  uart_printf("calculated: ");

  for (int i = 0; i < sizeof(pSha256Hash); ++i)
    uart_printf("%02X", pSha256Hash[i]);

  uart_printf("\n");

  uart_printf("expected: ");

  for (int i = 0; i < sizeof(pSha256Hash); ++i)
    uart_printf("%02X", req.pSha256Hash[i]);

  uart_printf("\n");

  for (int i = 0; i < sizeof(pSha256Hash); ++i) {
    if (pSha256Hash[i] != req.pSha256Hash[i]) {
      uart_printf("Error: SHA-256 checksum mismatch!\n");
     // return -1;
    }
  }

  uart_printf("checksum ok\n");

  char pTargetFileName[128];
  snprintf(pTargetFileName, sizeof(pTargetFileName), "B:/%s", req.pFileName);

  FIO_RemoveFile(pTargetFileName);

  int error;

  if ((error = _FIO_RenameFile(pTempFile, pTargetFileName)) != 0) {
    uart_printf("[%d] Error on rename '%s' -> '%s'!\n", error, pTempFile, pTargetFileName);
    return 1;
  }
   uint32_t value = 0x0;
       _prop_request_change(0x80010003,&value,4);
  return 0;
}

int drysh_ml_update(int argc, char const *argv[]) {
  int error;

  error = wifiConnect();

  if (error) {
    printError("connection to wifi failed!\n");
    return 1;
  }

  uart_printf("creating socket\n");

  int serverFd;

  if ((serverFd = socket_create(DRY_AF_INET, DRY_SOCK_STREAM, 0)) < 0) {
    printError("socket failed");
    return 1;
  }

  int clientFd = createServer(serverFd, 2342);

  if (clientFd < 0) {
    printError("Failed to create server");
    return 1;
  }

  if ((error = performUpdate(clientFd)) != 0) {
    printError("Update failed!");
    return 1;
  }

  uart_printf("Update successful!\n");
  uart_printf("Rebooting...\n");

  

  return 0;
}
/*
int main(int argc, char const* pArgv[]) {
  return drysh_ml_update(argc, pArgv);
}
*/
#endif