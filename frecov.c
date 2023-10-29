#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define ATTR_READ_ONLY	0x1
#define ATTR_HIDDEN			0x2
#define ATTR_SYSTEM			0x4
#define ATTR_VOLUME_ID  0x8
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE		0x20

#define ATTR_LONG_NAME	(ATTR_READ_ONLY | \
												ATTR_HIDDEN | \
												ATTR_SYSTEM | \
												ATTR_VOLUME_ID |)
#define ATTR_LONG_NAME_MASK 0x3F

//#define TEST

// Copied from the manual
struct fat32hdr {
  u8  BS_jmpBoot[3];
  u8  BS_OEMName[8];
  u16 BPB_BytsPerSec;
  u8  BPB_SecPerClus;
  u16 BPB_RsvdSecCnt;
  u8  BPB_NumFATs;
  u16 BPB_RootEntCnt;
  u16 BPB_TotSec16;
  u8  BPB_Media;
  u16 BPB_FATSz16;
  u16 BPB_SecPerTrk;
  u16 BPB_NumHeads;
  u32 BPB_HiddSec;
  u32 BPB_TotSec32;
  u32 BPB_FATSz32;
  u16 BPB_ExtFlags;
  u16 BPB_FSVer;
  u32 BPB_RootClus;
  u16 BPB_FSInfo;
  u16 BPB_BkBootSec;
  u8  BPB_Reserved[12];
  u8  BS_DrvNum;
  u8  BS_Reserved1;
  u8  BS_BootSig;
  u32 BS_VolID;
  u8  BS_VolLab[11];
  u8  BS_FilSysType[8];
  u8  __padding_1[420];
  u16 Signature_word;
} __attribute__((packed));

struct fat32dir	{
	u8 DIR_Name[11];
	u8 DIR_Attr;
	u8 DIR_NTRes;
	u8 DIR_CrtTimeTenth;
	u16 DIR_CrtTime;
	u16 DIR_CrtDate;
	u16 DIR_LstAccDate;
	u16 DIR_FstClusHI;
	u16 DIR_WrtTime;
	u16 DIR_WrtDate;
	u16 DIR_FstClusLO;
	u32 DIR_FileSize;
	
} __attribute__((packed));

struct fat32ldir	{
	u8 LDIR_Ord;
	u8 LDIR_Name1[10];
	u8 LDIR_Attr;
	u8 LDIR_Type;
	u8 LDIR_Chksum;
	u8 LDIR_Name2[12];
	u16 LDIR_FstClusLO;
	u8 LDIR_Name3[4];
} __attribute__((packed));

u16 ClusterOfDir[10] = {0xFFFF};
u16 ClusterOfBmpH[256] = {0xFFFF};
u8* NameArr[10] = {NULL};
u8 FileName[32] = {0};

void *map_disk(const char *fname);
void get_diskInfo(struct fat32hdr *hdr);
void get_ImageInfo(struct fat32hdr *hdr);
u8 ScanAllCluster(struct fat32hdr *hdr, u32 DataAddr);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s fs-image\n", argv[0]);
    exit(1);
  }

  setbuf(stdout, NULL);

  assert(sizeof(struct fat32hdr) == 512); // defensive

  // map disk image to memory
  struct fat32hdr *hdr = map_disk(argv[1]);
	get_diskInfo(hdr);

//	ScanAllCluster(hdr, 0x24000);

	get_ImageInfo(hdr);

  // file system traversal
  munmap(hdr, hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec);
}

void *map_disk(const char *fname) {
  int fd = open(fname, O_RDWR);

  if (fd < 0) {
    perror(fname);
    goto release;
  }

  off_t size = lseek(fd, 0, SEEK_END);
  if (size == -1) {
    perror(fname);
    goto release;
  }

  struct fat32hdr *hdr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

  if (hdr == (void *)-1) {
    goto release;
  }

  close(fd);

  if (hdr->Signature_word != 0xaa55 ||
      hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec != size) {
    fprintf(stderr, "%s: Not a FAT file image\n", fname);
    goto release;
  }
  return hdr;

release:
  if (fd > 0) {
    close(fd);
  }
  exit(1);
}

u8 ChkSum(u8* pFcbName)
{
	u16 FcbNameLen;
	u8 Sum;

	Sum = 0;
	for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--)
	{
		// NOTE: The operation is unsigned char rotate right
		Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
	}
	return (Sum);
}

u8 ScanAllCluster(struct fat32hdr *hdr, u32 DataAddr)
{
	u32 offset = 0;
	u32 TotalBytes = hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec;
	u8* DataPtr = (u8*)hdr;
	u8 ArrIdx = 0, ArrIdxB = 0;

#ifdef TEST	
	printf("%x %x\r\n", DataAddr, TotalBytes);
#endif

	while ((DataAddr) < TotalBytes)
	{
		if ((*(DataPtr + DataAddr + 11) & 0xC0) == 0)
		{
			if (*(DataPtr + DataAddr + 11) == 0xF)
			{
				// Check sum
				if (*(DataPtr + DataAddr + 13) == ChkSum(DataPtr + DataAddr + (*(DataPtr + DataAddr) & 0xF)*0x20))
				{
				  ClusterOfDir[ArrIdx++] = offset/0x1000 + 2;	
				}
			}
			else if ((*(DataPtr + DataAddr) == 0x2E) && \
				(*(DataPtr + DataAddr + 11) == 0x10))
			{
				  ClusterOfDir[ArrIdx++] = offset/0x1000 + 2;	
			}
			else if ((*(DataPtr + DataAddr + 11) == 0x20) && \
				(*(DataPtr + DataAddr + 20) == 0))
			{
				  ClusterOfDir[ArrIdx++] = offset/0x1000 + 2;	
			}
			else if ((*(DataPtr + DataAddr) == 0x42) && \
					(*(DataPtr + DataAddr + 1) == 0x4d))
			{
				ClusterOfBmpH[ArrIdxB++] = offset / 0x1000;
			}
		}
		
		DataAddr += 0x1000;
		offset += 0x1000;
	}

#ifdef TEST
	printf("Dir cluster: ");
	for (u16 i = 0; i < ArrIdx; i++)
	{
		printf("%x ", ClusterOfDir[i]); 
	}
	printf("\r\n");

	printf("BMP header cluster: ");
	for (u16 i = 0; i < ArrIdxB; i++)
	{
		printf("%x ", ClusterOfBmpH[i]);
	}
#endif
	return ArrIdx;
}

void BuildBmpFile(struct fat32hdr *hdr, u16 FirstCluster, u32 FileSize)
{
	u8* DataPtr = (u8*)hdr;
	FILE *fd;
	u32 DirOffset =	(hdr->BPB_RsvdSecCnt + hdr->BPB_FATSz32 * hdr->BPB_NumFATs) * hdr->BPB_BytsPerSec;
	u32 BytePerCluster = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
	u32 Data = DirOffset + (FirstCluster - 2) * BytePerCluster; 
	char File[64] = {0};
	u8 buf[256] = {0};

	memcpy(File, "./temp_bmp/", sizeof ("./temp_bmp/"));
	memcpy(&File[11], FileName, sizeof(FileName));

#ifdef TEST
	printf("FirstClus:%x\r\n", FirstCluster);
	printf("FileSize:%x\r\n", FileSize);
	printf("DataAddr:%x\r\n", Data);
#endif

	fd = fopen((const char*)File, "wb");
	if (fd < 0)
	{
		printf("%s can't open!!!\r\n", FileName);
	}
	else
	{
		fwrite((unsigned char*)((u8*)hdr + Data), sizeof(u8), FileSize, fd);
	}
	fclose(fd);
	
	memcpy (File, "sha1sum ./temp_bmp/", sizeof("shasum ./temp_bmp/"));
	memcpy(&File[19], FileName, sizeof(FileName));

#ifdef TEST
	printf("%s\r\n", File);
#endif

	fd = popen((const char*)File, "r");
	//panic_on(fd < 0, "popen");
	if (fd < 0)
	{
		printf("can't open fd\r\n");
		while(1);
	}
	fscanf(fd, "%s", buf);
	printf("%s ", buf);
	printf("%s\r\n", FileName);
	return;
}

void ShowFileName(u8** Name, u8 length)
{
	u8 NameLen[3] = {4, 12, 10};

	if (length % 3 != 1)
	{
		printf("Get name wrong!\r\n");
		while(1);
	}
	if (length == 1)
	{
		printf("%s", Name[0]);
	}
	else
	{
		//for (u8 i = length - 2; i != 0; i--)
		u8 i = length - 2;
		u8 k = 0;
		while(1)
		{
			for (u8 j = 0; j < NameLen[i % 3]; j++)
			{
				if ((Name[i][j] != 0) && (Name[i][j] != 0xFF))
				{
#ifdef TEST
					printf("%c", Name[i][j]);
#endif
					FileName[k++] = Name[i][j];
				}
			}	
			if (i == 0)
				break;
			i--;
		}
		FileName[k] = '\0';
	}
#ifdef TEST
	printf("\r\n");
#endif
}

void get_ImageInfo(struct fat32hdr *hdr)
{
	u8* DiskAddr = (u8*)hdr;
	u32 DirOffset =	(hdr->BPB_RsvdSecCnt + hdr->BPB_FATSz32 * hdr->BPB_NumFATs) * hdr->BPB_BytsPerSec;
	u32 BytePerCluster = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
	u32 DataBaseAddr;

  struct fat32dir* dir;
  struct fat32ldir* ldir;
	
	u8 dir_attr, NameIdx = 0, i = 0;
  u8 ldir_ord;
  u8 Name[0x100] = {0};
	u16 offset = 0;
	u8 TotalDirClus = ScanAllCluster(hdr, 0x24000);

	dir_attr = *(DiskAddr + DirOffset + 11);
	DataBaseAddr = DirOffset;

	while (i < TotalDirClus)
	{	
		DirOffset = (ClusterOfDir[i] - 2) * BytePerCluster + DataBaseAddr; 
		offset = 0;

		while (offset < BytePerCluster)
		{
				dir_attr = *(DiskAddr + DirOffset + 11);
				//printf("%x ", DirOffset);

				if ((dir_attr & 0x10) == 0x10)
				{
					//dirtory, continue
					dir = (struct fat32dir*)(DiskAddr + DirOffset);
					printf("%s\r\n", dir->DIR_Name);
				}
				else if ((dir_attr & 0xF) == 0xF)
				{
					//long name file
				  ldir = (struct fat32ldir*)(DiskAddr + DirOffset);
					NameArr[NameIdx++] = ldir->LDIR_Name3;
					NameArr[NameIdx++] = ldir->LDIR_Name2;
					NameArr[NameIdx++] = ldir->LDIR_Name1;
				}
				else if ((dir_attr & 0x3F) != 0)
				{
					dir = (struct fat32dir*)(DiskAddr + DirOffset);
					if (dir->DIR_Name[0] != 0xE5)
					{
						NameArr[NameIdx++] = dir->DIR_Name;
						ShowFileName(NameArr, NameIdx);
						BuildBmpFile(hdr, dir->DIR_FstClusLO | (dir->DIR_FstClusHI << 16), dir->DIR_FileSize);
#ifdef TEST
						return;
#endif
					}
					NameIdx = 0;
				}
				DirOffset += sizeof(struct fat32dir);
				offset += sizeof(struct fat32dir);
		}
		i++;
	}
	return;
		
}

void get_diskInfo(struct fat32hdr *hdr)
{	
 	// TODO: frecov
	int DirOffset =	(hdr->BPB_RsvdSecCnt + hdr->BPB_FATSz32 * hdr->BPB_NumFATs) * hdr->BPB_BytsPerSec;
	int SecPerCluster = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;

	printf("BPB_TotSec32 %x\r\nBPB_BytsPerSec %x\r\n", hdr->BPB_TotSec32, hdr->BPB_BytsPerSec);
	
	printf("BPB_RsvdSecCnt %x \r\nBPB_FATSz32 %x BPB_NumFATs %x\r\n", hdr->BPB_RsvdSecCnt, hdr->BPB_FATSz32, hdr->BPB_NumFATs);

 	printf("DirOffset %x SecPerClus %x\r\n", DirOffset, SecPerCluster);
	printf("%x %x %x\r\n", *(int*)hdr, *(int*)((char*)hdr + DirOffset), *(int*)((char*)hdr + DirOffset + 0x1000));
  printf("RootClus %x\r\n", hdr->BPB_RootClus);
	printf("HiddSec %x\r\n", hdr->BPB_HiddSec);
  return;
}
