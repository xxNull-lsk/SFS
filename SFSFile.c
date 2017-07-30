#include "SFS.h"

SFS_ERROR SfsFileOpen(IN PSFS_CONTEXT* ppContext, OUT PSFS_FILE_CONTEXT * ppFile)
{
    
}

SFS_ERROR SfsFileClose(IN PSFS_FILE_CONTEXT * ppFile)
{
    
}

SFS_ERROR SfsFileRead(IN PSFS_FILE_CONTEXT * pFile, void * buf, UINT32 nBytes)
{
}

SFS_ERROR SfsFileWrite(IN PSFS_FILE_CONTEXT * pFile, const void * buf, UINT32 nBytes)
{
}

UINT64 SfsFileSeek(IN PSFS_FILE_CONTEXT * pFile, UINT64 offset, SFS_WENCE whence)
{
}
