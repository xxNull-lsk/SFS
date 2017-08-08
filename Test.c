#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "SFS.h" 
typedef struct
{
    int f;
    UINT32 nSectorBytes;
    UINT32 nSectorCount;
}DEV_INFO, * PDEV_INFO;

SFS_ERROR DevWriteSector(struct _SFS_DEV * pDev, UINT64 nOffset, const void* lpData, UINT32 nSectorCount)
{
    PDEV_INFO pInfo = (PDEV_INFO)pDev->pUsedData;
    if (lseek64(pInfo->f, nOffset * pInfo->nSectorBytes, SEEK_SET) < 0)
    {
        return SFS_ERR_INVALID_OFFSET;
    }
    if (write(pInfo->f, lpData, nSectorCount * pInfo->nSectorBytes) == -1)
    {
        return SFS_ERR_WRITE_SECTOR;
    }
    return SFS_OK;
}

SFS_ERROR DevReadSector(struct _SFS_DEV * pDev, UINT64 nOffset, void* lpData, UINT32 nSectorCount)
{
    PDEV_INFO pInfo = (PDEV_INFO)pDev->pUsedData;
    if (lseek64(pInfo->f, nOffset * pInfo->nSectorBytes, SEEK_SET) < 0)
    {
        return SFS_ERR_INVALID_OFFSET;
    }
    if (read(pInfo->f, lpData, nSectorCount * pInfo->nSectorBytes) == -1)
    {
        return SFS_ERR_READ_SECTOR;
    }
    return SFS_OK;
}

SFS_ERROR DevFlush(struct _SFS_DEV * pDev)
{
    PDEV_INFO pInfo = (PDEV_INFO)pDev->pUsedData;
    if (fsync(pInfo->f) == -1)
    {
        return SFS_ERR_WRITE_SECTOR;
    }
    return SFS_OK;
}

UINT64 DevGetSectorCount(struct _SFS_DEV * pDev)
{
    PDEV_INFO pInfo = (PDEV_INFO)pDev->pUsedData;
    return pInfo->nSectorCount;
}

UINT32 DevGetSectorSize(struct _SFS_DEV * pDev)
{
    PDEV_INFO pInfo = (PDEV_INFO)pDev->pUsedData;
    return pInfo->nSectorBytes;
}

void DevLog(struct _SFS_DEV * pDev, const char * lpszFileName, UINT32 nLineNO, SFS_LOG nType, SFS_ERROR err, const char * lpszFormat, ...)
{
    char szLog[1024] = {0};
    va_list ap;
    va_start(ap, lpszFormat);
    vsnprintf(szLog, sizeof(szLog), lpszFormat, ap);
    va_end(ap);
    
    switch(nType)
    {
        case SFS_LOG_ERROR:
        {
            fprintf(stderr, "Error(%d)　- %s(%u): %s\n", err, lpszFileName, nLineNO, szLog);
            break;
        }
        case SFS_LOG_WARN:
        {
            fprintf(stderr, "Warn　-　%s(%u): %s\n", lpszFileName, nLineNO, szLog);
            break;
        }
        case SFS_LOG_INFO:
        {
            fprintf(stderr, "Info　-　%s(%u): %s\n", lpszFileName, nLineNO, szLog);
            break;
        }
        case SFS_LOG_DEBUG:
        {
            fprintf(stderr, "Debug　-　%s(%u): %s\n", lpszFileName, nLineNO, szLog);
            break;
        }
        default:
        {
            fprintf(stderr, "Unknown -　%s(%u): %s\n", lpszFileName, nLineNO, szLog);
            break;
        }
    }
}

SFS_DEV g_dev = {
    .GetSectorCount = DevGetSectorCount,
    .GetSectorSize = DevGetSectorSize,
    .WriteSector = DevWriteSector,
    .ReadSector = DevReadSector,
    .Flush = DevFlush,
    .Log = DevLog,
    .pUsedData = NULL,
};

SFS_ERROR TestFormat()
{
    SFS_ERROR err = SfsFormat(&g_dev);
    if (err)
    {
        g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Format failed!");
        return err;
    }
    return SFS_OK;
}

void DumpSuperBlock(SFS_CONTEXT* pContext, UINT32 nLineNO)
{
    printf("--------------------%u--------------------------\n"
            " nTotalSectorCount:        %llu\n"
            " nUsedSectorCount:         %llu\n"
            " nSN:                      0x%llx\n"
            " nSectorBytes:             %u\n"
            " nFileNameSectorOffset:    %u\n"
            " nFileNameMaxLen:          %u\n"
            " nFileSize:                %llu\n"
            " nFileAttribute:           %u\n"
            " nFileDataSectorOffset:    %u\n",
            nLineNO,
        pContext->superBlock.nTotalSectorCount,
        pContext->superBlock.nUsedSectorCount,
        pContext->superBlock.nSN,
        pContext->superBlock.nSectorBytes,
        pContext->superBlock.nFileNameSectorOffset,
        pContext->superBlock.nFileNameMaxLen,
        pContext->superBlock.nFileSize,
        pContext->superBlock.nFileAttribute,
        pContext->superBlock.nFileDataSectorOffset);
}

SFS_ERROR TestFile(SFS_CONTEXT* pContext)
{
    PSFS_FILE_CONTEXT pFile;
    SFS_ERROR err = SfsFileOpen(pContext, &pFile);
    if (err)
    {
        g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Open file failed!");
        return err;
    }
    char szText[1024] = "This is a test!";
    char szText2[1024];
    UINT32 i;
    for (i = 0; i < 10; i++)
    {
        UINT64 off = SfsFileSeek(pFile, i * 100, SFS_SEEK_SET);
        if (off == (UINT64)-1)
        {
            g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Seek file failed!");
            return err;
        }
        err = SfsFileWrite(pFile, szText, strlen(szText));
        if (err)
        {
            g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Write file failed!");
            return err;
        }
        DumpSuperBlock(pContext, i);
        SfsFileSeek(pFile, i * 100, SFS_SEEK_SET);
        err = SfsFileRead(pFile, szText2, strlen(szText));
        if (err)
        {
            g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Read file failed!");
            return err;
        }
    }
    
    SfsFileClose(&pFile);
    
    return SFS_OK;
}

int main(int argc, char * argv[])
{
    SFS_CONTEXT* pContext;
    SFS_ERROR err;
    DEV_INFO devInfo;
    devInfo.nSectorCount = 1024 * 200;
    devInfo.nSectorBytes = 512;
    devInfo.f = open("sfs.dat", O_CREAT|O_TRUNC|O_RDWR, S_IFMT|S_IREAD|S_IWRITE);
    if (devInfo.f == -1)
    {
        fprintf(stderr, "Create file failed!errno:%d\n", errno);
        return errno;
    }
    g_dev.pUsedData = &devInfo;
    if (ftruncate(devInfo.f, devInfo.nSectorCount * devInfo.nSectorBytes) == -1)
    {
        fprintf(stderr, "Set file size failed!errno:%d\n", errno);
        return errno;
    }
    err = TestFormat();
    if (err)
    {
        g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Format failed!");
    }
    err = SfsMount(&g_dev, &pContext);
    if (err)
    {
        g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Mount failed!");
        return err;
    }

    DumpSuperBlock(pContext, __LINE__);
    err = TestFile(pContext);
    if (err)
    {
        g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Test file failed!");
        return err;
    }
    DumpSuperBlock(pContext, __LINE__);
    
    err = SfsUmount(&pContext);
    if (err)
    {
        g_dev.Log(&g_dev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Umount failed!");
        return err;
    }
    
    printf("Test finished!\n");
    close(devInfo.f);
    return 0;
}
