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
    pContext->pDev = pDev;
    pContext->superBlock.nMagic = SFS_MAGIC;
    pContext->superBlock.nSectorBytes = pDev->GetSectorSize(pDev);
    pContext->superBlock.dtFormatTime = SfsGetCurrDateTime();
    pContext->superBlock.nSN = (UINT64)rand() * rand();
    pContext->superBlock.nTotalSectorCount = pDev->GetSectorCount(pDev);
    pContext->superBlock.nUsedSectorCount = 2;
    pContext->superBlock.nFileNameSectorOffset = 1;
    pContext->superBlock.nFileNameMaxLen = pDev->GetSectorSize(pDev) / sizeof(wchar_t);
    pContext->superBlock.nPartSign = 0x55aa;
	err = SfsRWBytes(pContext, 0, &pContext->superBlock, sizeof(pContext->superBlock), FALSE);
    if (err != SFS_OK)
    {
        free(pContext);
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Write disk failed!");
        return err;
    }
    free(pContext);
    return SFS_OK;
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
    if (pDev->GetSectorSize(pDev) != pContext->superBlock.nSectorBytes)
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
    pContext->pDev = pDev;	
    pContext->superBlock.nSectorBytes = pDev->GetSectorSize(pDev);
	err = SfsRWBytes(pContext, 0, &pContext->superBlock, sizeof(pContext->superBlock), TRUE);
    if (err != SFS_OK)
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
    if (pContext->superBlock.nSectorBytes != pDev->GetSectorSize(pDev))
    {
        err = SFS_ERR_INVALID_PART_SIGN;
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Bad sector size!0x%x != 0x%x", pContext->superBlock.nSectorBytes, pDev->GetSectorSize(pDev));
        goto EXIT_FREE;
    }
    pContext->superBlock.dtLastMount = SfsGetCurrDateTime();
    err = SfsRWBytes(pContext, 0, &pContext->superBlock, sizeof(pContext->superBlock), FALSE);
    if (err != SFS_OK)
    {
        free(pContext);
        pDev->Log(pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Write disk failed!");
        return err;
    }
	
    *ppContext = pContext;
    return SFS_OK;
EXIT_FREE:
	if (pContext)
	{
		if (pContext->pBuf4RWBytes)
		{
			free(pContext->pBuf4RWBytes);
		}
		free(pContext);
	}
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
    
	free(pContext->pBuf4RWBytes);
    free(pContext);
    *ppContext = NULL;
    return SFS_OK;
}

SFS_ERROR SfsRWBytes(IN PSFS_CONTEXT pFs, IN UINT64 nOffset, IN OUT void * buf, IN UINT32 nBytes, IN UINT8 bIsRead)
{
	SFS_ERROR err;
    UINT32 nSectorBytes, nSectorCount;
    UINT32 nOffsetInFirstSector;
    UINT64 nSectorOffset;
    PUINT8 pWorker = (PUINT8)buf;
    if (!pFs || !pWorker)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    nSectorBytes = pFs->superBlock.nSectorBytes;
    if (!pFs->pBuf4RWBytes)
    {
		pFs->pBuf4RWBytes = (PUINT8)malloc(nSectorBytes);
		if (!pFs->pBuf4RWBytes)
		{
			pFs->pDev->Log(pFs->pDev, __FILE__, __LINE__, SFS_LOG_ERROR, SFS_ERR_OUT_OF_MEMORY, "Read write bytes failed!No memory.Size:%u", nSectorBytes);
			return SFS_ERR_OUT_OF_MEMORY;
		}
	}
    nSectorOffset = nOffset / nSectorBytes;
    nOffsetInFirstSector = nOffset % nSectorBytes;
    // Read or Write the first sector
    if (nOffsetInFirstSector)
    {
		UINT32 nRWBytes = nSectorBytes - nOffsetInFirstSector;
		if (nRWBytes > nBytes)
		{
			nBytes = nBytes;
		}
		err = pFs->pDev->ReadSector(pFs->pDev, nSectorOffset, pFs->pBuf4RWBytes, 1);
		if (err != SFS_OK)
		{
			return err;
		}
		if (bIsRead)
		{
			memcpy(buf, pFs->pBuf4RWBytes + nOffsetInFirstSector, nRWBytes);
		}
		else
		{
			memcpy(pFs->pBuf4RWBytes + nOffsetInFirstSector, pWorker, nRWBytes);
			err = pFs->pDev->WriteSector(pFs->pDev, nSectorOffset, pFs->pBuf4RWBytes, 1);
			if (err != SFS_OK)
			{
				return err;
			}
		}
		nSectorOffset++;
		nBytes -= nRWBytes;
		pWorker += nRWBytes;
    }
    
    // Read or Write the sectors which is sequential
    nSectorCount = nBytes / nSectorBytes;
    if (nSectorCount)
    {
		if (bIsRead)
		{
			err = pFs->pDev->ReadSector(pFs->pDev, nSectorOffset, pWorker, nSectorCount);
		}
		else
		{
			err = pFs->pDev->WriteSector(pFs->pDev, nSectorOffset, pWorker, nSectorCount);
		}
		if (err != SFS_OK)
		{
			return err;
		}
		nSectorOffset += nSectorCount;
		nBytes -= nSectorCount * nSectorBytes;
		pWorker += nSectorCount * nSectorBytes;
	}
	
    // Read or Write the last sector
	if (nBytes)
	{
		err = pFs->pDev->ReadSector(pFs->pDev, nSectorOffset, pFs->pBuf4RWBytes, 1);
		if (err != SFS_OK)
		{
			return err;
		}
		if (bIsRead)
		{
			memcpy(pWorker, pFs->pBuf4RWBytes, nBytes);
		}
		else
		{
			memcpy(pFs->pBuf4RWBytes, pWorker, nBytes);
			err = pFs->pDev->WriteSector(pFs->pDev, nSectorOffset, pFs->pBuf4RWBytes, 1);
		}
		if (err != SFS_OK)
		{
			return err;
		}
	}
	return SFS_OK;
}
