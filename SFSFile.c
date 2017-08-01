#include "SFS.h"

SFS_ERROR SfsFileOpen(IN PSFS_CONTEXT pFs, OUT PSFS_FILE_CONTEXT * ppFile)
{
    PSFS_FILE_CONTEXT pFile = NULL;
    if (!pFs)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    if (pFs->pFile)
    {
        return SFS_ERR_ALREADY_OPENED;
    }
    if (pFs->superBlock.nMagic != SFS_MAGIC)
    {
        return SFS_ERR_UNMOUNT;
    }
    pFile = (PSFS_FILE_CONTEXT)malloc(sizeof(SFS_FILE_CONTEXT));
    if (!pFile)
    {
        return SFS_ERR_OUT_OF_MEMORY;
    }
    memset(pFile, 0, sizeof(SFS_FILE_CONTEXT));
    pFs->pFile = pFile;
    pFile->pFs = pFs;
    *ppFile = pFile;
    return SFS_OK;
}

SFS_ERROR SfsFileClose(IN PSFS_FILE_CONTEXT * ppFile)
{
    if (!ppFile)
    {
        return SFS_OK;
    }
    if (!(*ppFile)->pFs ||
        (*ppFile)->pFs->pFile != *ppFile)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    (*ppFile)->pFs->pFile = NULL;
    free(*ppFile);
    *ppFile = NULL;
    return SFS_OK;
}

SFS_ERROR SfsFileRead(IN PSFS_FILE_CONTEXT pFile, OUT void * buf, IN UINT32 nBytes)
{
    SFS_ERROR err;
    UINT64 nOffset;
    if (!pFile ||
        !pFile->pFs ||
        pFile->pFs->pFile != pFile)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    nOffset = pFile->nCurrOffset + pFile->pFs->superBlock.nFileDataSectorOffset * pFile->pFs->superBlock.nSectorBytes;
    if (nOffset >= pFile->pFs->superBlock.nFileSize)
    {
        return SFS_ERR_INVALID_OFFSET;
    }
    if (nBytes > pFile->pFs->superBlock.nFileSize - nOffset)
    {
        return SFS_ERR_INVALID_LEN;
    }
    err = SfsRWBytes(pFile->pFs, nOffset, buf, nBytes, TRUE);
    if (err != SFS_OK)
    {
        return err;
    }
    pFile->nCurrOffset = nOffset + nBytes;
    return SFS_OK;
}

SFS_ERROR SfsFileWrite(IN PSFS_FILE_CONTEXT pFile, const void * buf, UINT32 nBytes)
{
    SFS_ERROR err;
    UINT64 nOffset;
    if (!pFile ||
        !pFile->pFs ||
        pFile->pFs->pFile != pFile)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    nOffset = pFile->nCurrOffset + pFile->pFs->superBlock.nFileDataSectorOffset * pFile->pFs->superBlock.nSectorBytes;
    if (nOffset + nBytes > pFile->pFs->superBlock.nFileSize)
    {
        err = SfsFileSetFileSize(pFile, nOffset + nBytes);
        if (err != SFS_OK)
        {
            return err;
        }
    }
    err = SfsRWBytes(pFile->pFs, nOffset, (void *)buf, nBytes, FALSE);
    if (err != SFS_OK)
    {
        return err;
    }
    pFile->nCurrOffset = nOffset + nBytes;
    return SFS_OK;
}

UINT64 SfsFileSeek(IN PSFS_FILE_CONTEXT pFile, UINT64 offset, SFS_WENCE whence)
{
    if (!pFile ||
        !pFile->pFs ||
        pFile->pFs->pFile != pFile)
    {
        return (UINT64)-1;
    }
    switch(whence)
    {
        case SFS_SEEK_SET:
        {
            pFile->nCurrOffset = offset;
            break;
        }
        case SFS_SEEK_CUR:
        {
            pFile->nCurrOffset += offset;
            break;
        }
        case SFS_SEEK_END:
        {
            if (offset >= pFile->pFs->superBlock.nFileSize)
            {
                pFile->nCurrOffset = 0;
            }
            else
            {
                pFile->nCurrOffset = pFile->pFs->superBlock.nFileSize - offset - 1;
            }
            break;
        }
        default:
        {
            return (UINT64)-1;
        }
    }
    return pFile->nCurrOffset;
}

SFS_ERROR SfsFileSetFileSize(IN PSFS_FILE_CONTEXT pFile, UINT64 nSize)
{
    SFS_ERROR err;
    if (!pFile ||
        !pFile->pFs ||
        pFile->pFs->pFile != pFile)
    {
        return SFS_ERR_INVALID_PARAM;
    }
    pFile->pFs->superBlock.nUsedSectorCount = pFile->pFs->superBlock.nFileDataSectorOffset + (nSize + pFile->pFs->superBlock.nSectorBytes - 1) / pFile->pFs->superBlock.nSectorBytes;

    pFile->pFs->superBlock.nFileSize = nSize;
    err = SfsRWBytes(pFile->pFs, 0, &pFile->pFs->superBlock, sizeof(pFile->pFs->superBlock), FALSE);
    if (err != SFS_OK)
    {
        pFile->pFs->pDev->Log(pFile->pFs->pDev, __FILE__, __LINE__, SFS_LOG_ERROR, err, "Write disk failed!");
        return err;
    }
    return SFS_OK;
}
