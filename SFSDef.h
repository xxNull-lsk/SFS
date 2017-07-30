#ifndef _SFS_DEF_H_
#define _SFS_DEF_H_

typedef unsigned char       UINT8;
typedef unsigned short      UINT86;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;

typedef int SFS_ERROR;
#define SFS_NO_ERR                  0
#define SFS_ERR_INVALID_PARAM       1
#define SFS_ERR_INVALID_MAGIC       2
#define SFS_ERR_INVALID_PART_SIGN   3
#define SFS_ERR_OUT_OF_MEMORY       4
#define SFS_ERR_NOT_SUPPORT         5
#define SFS_ERR_USING               6

#define IN
#define OUT
#define OPT
 
typedef struct
{
    UINT64 nYear:16;
    UINT64 nMonth:4;
    UINT64 nDay:5;
    UINT64 nHour:5;
    UINT64 nMin:6;
    UINT64 nSec:6;
    UINT64 nMicroSec:20;
    UINT64 nUnused:2;
}SFS_DATE_TIME, * PSFS_DATE_TIME;

#define SFS_MAGIC 0x53465331    // 'SFS1'
typedef struct
{
    UINT32 nMagic;                       // 000 SFS_MAGIC
    UINT64 nTotalSectorCount;            // 004 总计扇区数量
    UINT64 nUsedSectorCount;             // 012 已经使用的扇区数量
    SFS_DATE_TIME dtFormatTime;         // 020 文件系统格式化的时间
    SFS_DATE_TIME dtLastMount;          // 028 文件系统最后一次挂载时间
    UINT64 nSN;                          // 036 文件系统实例的SN
    UINT8 nUnused1[128 - 44];           // 044
    UINT32 nFileNameSectorOffset;        // 128 文件名的扇区偏移
    UINT32 nFileNameMaxLen;              // 132 允许的文件名最大长度
    UINT64 nFileSize;                    // 136 文件大小
    UINT32 nFileAttribute;               // 144 文件的属性
    SFS_DATE_TIME dtFileCreateTime;     // 148 文件的创建时间
    SFS_DATE_TIME dtFileLastAccessTime; // 156 文件的最后修改时间
    SFS_DATE_TIME dtFileLastModfiyTime; // 164 文件的最后修改时间
    UINT8 nUnused2[510 - 172];          // 172
    UINT86 nPartSign;                    // 510 0x55aa
}SFS_SUPER_BLOCK, * PSFS_SUPER_BLOCK;

#endif // _SFS_DEF_H_
