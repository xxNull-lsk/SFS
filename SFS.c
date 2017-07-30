#include "SFS.h"

SFS_DATE_TIME SfsGetCurrDateTime()
{
    struct timeval t;
    SFS_DATE_TIME dtNow;
    time_t tNow;
    struct tm *gmtNow;
    tNow = time(NULL);
    gmtNow = gmtime(&tNow);
    memset(&dtNow, 0, sizeof(dtNow));
    dtNow.nYear = gmtNow->tm_year;
    dtNow.nMonth = gmtNow->tm_mon;
    dtNow.nDay = gmtNow->tm_mday;
    dtNow.nHour = gmtNow->tm_hour;
    dtNow.nMin = gmtNow->tm_min;
    dtNow.nSec = gmtNow->tm_sec;
    gettimeofday(&t, NULL);
    dtNow.nMicroSec = t.tv_usec % 1000000;
    return dtNow;
}

SFS_ERROR SfsFormat(IN PSFS_DEV pDev)
{
    SFS_ERROR err;
    PSFS_CONTEXT pContext = NULL;
    if (!pDev)
    {
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_INVALID_PARAM, "Format failed!");
        return SFS_ERR_INVALID_PARAM;
    }
    if (pDev->GetSectorSize(pDev) != 512)
    {
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_NOT_SUPPORT, "Format failed!");
        return SFS_ERR_NOT_SUPPORT;
    }
    srand((unsigned)time(NULL));
    pContext = (PSFS_CONTEXT)malloc(sizeof(SFS_CONTEXT));
    if (!pContext)
    {
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_OUT_OF_MEMORY, "Format failed!No memory.Size:%u", sizeof(SFS_CONTEXT));
        return SFS_ERR_OUT_OF_MEMORY;
    }
    memset(pContext, 0, sizeof(SFS_CONTEXT));
    pContext->superBlock.nMagic = SFS_MAGIC;
    pContext->superBlock.dtFormatTime = SfsGetCurrDateTime();
    pContext->superBlock.nSN = (UINT64)rand() * rand();
    pContext->superBlock.nTotalSectorCount = pDev->GetSectorCount(pDev);
    pContext->superBlock.nUsedSectorCount = 2;
    pContext->superBlock.nFileNameSectorOffset = 1;
    pContext->superBlock.nFileNameMaxLen = pDev->GetSectorSize(pDev) / sizeof(wchar_t);
    pContext->superBlock.nPartSign = 0x55aa;
    err = pDev->WriteSector(pDev, 0, &pContext->superBlock, 1);
    if (err != SFS_NO_ERR)
    {
        free(pContext);
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Write disk failed!");
        return err;
    }
    free(pContext);
    return SFS_NO_ERR;
}

SFS_ERROR SfsMount(IN PSFS_DEV pDev, OUT PSFS_CONTEXT* ppContext)
{
    SFS_ERROR err;
    PSFS_CONTEXT pContext = NULL;
    if (!pDev || !ppContext)
    {
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_INVALID_PARAM, "Mount failed!");
        return SFS_ERR_INVALID_PARAM;
    }
    *ppContext = NULL;
    if (pDev->GetSectorSize(pDev) != 512)
    {
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_NOT_SUPPORT, "Mount failed!");
        return SFS_ERR_NOT_SUPPORT;
    }
    pContext = (PSFS_CONTEXT)malloc(sizeof(SFS_CONTEXT));
    if (!pContext)
    {
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_OUT_OF_MEMORY, "Mount failed!No memory.Size:%u", sizeof(SFS_CONTEXT));
        return SFS_ERR_OUT_OF_MEMORY;
    }
    memset(pContext, 0, sizeof(SFS_CONTEXT));
    err = pDev->ReadSector(pDev, 0, &pContext->superBlock, 1);
    if (err != SFS_NO_ERR)
    {
        free(pContext);
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Read disk failed!");
        goto EXIT_FREE;
    }
    if (pContext->superBlock.nMagic != SFS_MAGIC)
    {
        err = SFS_ERR_INVALID_MAGIC;
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Bad magic!0x%lx != 0x%lx", pContext->superBlock.nMagic, SFS_MAGIC);
        goto EXIT_FREE;
    }
    if (pContext->superBlock.nPartSign != 0x55aa)
    {
        err = SFS_ERR_INVALID_PART_SIGN;
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Bad PartSign!0x%x != 0x%x", pContext->superBlock.nPartSign, 0x55aa);
        goto EXIT_FREE;
    }
    pContext->superBlock.dtLastMount = SfsGetCurrDateTime();
    err = pDev->WriteSector(pDev, 0, &pContext->superBlock, 1);
    if (err != SFS_NO_ERR)
    {
        free(pContext);
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Write disk failed!");
        return err;
    }
    *ppContext = pContext;
    return SFS_NO_ERR;
EXIT_FREE:
    free(pContext);
    return err;
}

SFS_ERROR SfsUmount(IN PSFS_CONTEXT* ppContext)
{
    PSFS_CONTEXT pContext = *ppContext;
    if (!ppContext)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    if (pContext->pFile)
    {
        pContext->pDev->Log(pContext->pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_USING, "Umount failed!");
		return SFS_ERR_USING;
	}
    
    free(pContext);
    *ppContext = NULL;
    return SFS_NO_ERR;
}
