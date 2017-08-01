#ifndef _SFS_H_
#define _SFS_H_
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "SFSDef.h"
typedef enum _SFS_LOG
{
    SFS_LOG_ERROR = 1,
    SFS_LOG_WARN,
    SFS_LOG_INFO,
    SFS_LOG_DEBUG
}SFS_LOG;
struct _SFS_DEV;
typedef SFS_ERROR (* PFN_WriteSector)(struct _SFS_DEV * pDev, UINT64 nOffset, const void* lpData, UINT32 nSectorCount);
typedef SFS_ERROR (* PFN_ReadSector)(struct _SFS_DEV * pDev, UINT64 nOffset, void* lpData, UINT32 nSectorCount);
typedef SFS_ERROR (* PFN_Flush)(struct _SFS_DEV * pDev);
typedef UINT64 (* PFN_GetSectorCount)(struct _SFS_DEV * pDev);
typedef UINT32 (* PFN_GetSectorSize)(struct _SFS_DEV * pDev);
typedef UINT32 (* PFN_Log)(struct _SFS_DEV * pDev, const char * lpszFileName, UINT32 nLineNO, SFS_LOG nType, SFS_ERROR err, const char * lpszFormat, ...);

typedef struct _SFS_DEV{
    PFN_GetSectorCount      GetSectorCount;
    PFN_GetSectorSize       GetSectorSize;
    PFN_WriteSector         WriteSector;
    PFN_ReadSector          ReadSector;
    PFN_Flush               Flush;
    PFN_Log                 Log;
    void *                  pUsedData;
}SFS_DEV, * PSFS_DEV;

struct _SFS_FILE_CONTEXT;
typedef struct _SFS_CONTEXT
{
    SFS_SUPER_BLOCK            superBlock;
    PSFS_DEV                   pDev;
    PUINT8                     pBuf4RWBytes;
    struct _SFS_FILE_CONTEXT*  pFile;
}SFS_CONTEXT, * PSFS_CONTEXT;

#define SFS_FILE_MODE_READ      0x01
#define SFS_FILE_MODE_WRITE     0x02
typedef struct _SFS_FILE_CONTEXT
{
    PSFS_CONTEXT    pFs;
    UINT64          nCurrOffset;
}SFS_FILE_CONTEXT, * PSFS_FILE_CONTEXT;

SFS_DATE_TIME SfsGetCurrDateTime();

SFS_ERROR SfsFormat(IN PSFS_DEV pDev);
SFS_ERROR SfsMount(IN PSFS_DEV pDev, OUT PSFS_CONTEXT* ppContext);
SFS_ERROR SfsUmount(IN PSFS_CONTEXT* ppContext);

typedef enum _SFS_WENCE
{
    SFS_SEEK_SET = 0,
    SFS_SEEK_CUR = 1,
    SFS_SEEK_END = 2
}SFS_WENCE;

SFS_ERROR SfsFileOpen(IN PSFS_CONTEXT pFs, OUT PSFS_FILE_CONTEXT * ppFile);
SFS_ERROR SfsFileClose(IN PSFS_FILE_CONTEXT * ppFile);
SFS_ERROR SfsFileRead(IN PSFS_FILE_CONTEXT pFile, void * buf, UINT32 nBytes);
SFS_ERROR SfsFileWrite(IN PSFS_FILE_CONTEXT pFile, const void * buf, UINT32 nBytes);
UINT64 SfsFileSeek(IN PSFS_FILE_CONTEXT pFile, UINT64 offset, SFS_WENCE whence);
SFS_ERROR SfsFileSetFileSize(IN PSFS_FILE_CONTEXT pFile, UINT64 nSize);

SFS_ERROR SfsRWBytes(IN PSFS_CONTEXT pFs, IN UINT64 nOffset, IN OUT void * buf, IN UINT32 nBytes, IN UINT8 bIsRead);
#endif // _SFS_H_
