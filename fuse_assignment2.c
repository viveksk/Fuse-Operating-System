/*
#########################################################################################################################################
CS 6233			
Operating Systems I
ASSIGNMENT 2 DUE DATE 12 APRIL 2014
#########################################################################################################################################

#########################################################################################################################################
Please use to following command to execute it
gcc -Wall fuse_assignment2.c `pkg-config fuse --cflags --libs` -o fuse_assignment2
#########################################################################################################################################

Referenced from:
http://fuse.sourceforge.net/ (FUSE explaination and how it interacts with host operating system)
http://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201001/homework/fuse/fuse_doc.html(Understanding of various functions and their operation)
http://fuse.sourceforge.net/doxygen/structfuse__operations.html#abac8718cdfc1ee273a44831a27393419 (structure of various parameters for implementation of function)

FUSE: Filesystem in Userspace
  
*/

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/statvfs.h>

int rootinodeno = 2;
int maximumBlocks = 10000;		//maximum number of blocks.
int maximumfssize = 1638400;	//maximum size of file in filesystem
const char* MetaDataPrefix = "/fusedata/fusedata.";
const int rootNo = 26;		//Root directory will have metadata file no as 26 

//STRUCTURE FOR PARENT TO INODE DICTIONARY IN THE DIRECTORY OF THE FILESYSTEM
struct fileName_to_inode_dict
{
	char fileStructtype[10];
	char fileName[100];
	char inodeNumber[6];
};

//STRUCTURE FOR DIRECTORIES OF FILESYSTEM
struct dirAttributes
{
	int dirSize;	//DIRECTORY SIZE
	int uId;		//User Identity for CURRENT DIRECTORY
	int gId;		//Group Identity for Current Directory
	int Mode;		//User Mode for directories
	int aTime;		//ACCESS TIME FOR DIRECTORIES
	int cTime;		//CREATION TIME FOR DIRECTORIES
	int mTime;		//MODIFIED TIME FOR DIRECTORIES..THIS VALUE IS DISPLAYED WHEN WE DO -ls
	int linkcount;	//HARD LINK COUNT
	struct fileName_to_inode_dict filetoInode[100];
};

//SUPERBLOCK FOR FILESYSTEM. THIS IS THE POINT WHERE GENERAL ATTRIBUTES OF FILESYSTEM ARE STORED. THIS IS THE FIRST(BLOCK 0) IN FILESYSTEM
struct superBlock
{
int creation_time;	//CREATION TIME.. WHEEN FIRST TIME FILESYSTEM IS MOUNTED.
int mounted;		//NUMBER OF TIMES FILESYSTEM IS MOUNTED
int devId;			//DEVICE ID
int freeStart;		//FREEBLOCK LIST STARTS FROM BLOCK 1
int freeEnd;		//FREEBLOCK LIST ENDS ON BLOCK 25 SINCE EACH BLOCK HAS 400 SUB BLOCKS IN IT.
int root;			//ROOT IS AT 26. THIS IS / ON LINUX FILESYSTEM
int maxBlocks;		//MAXIMUM NUMBER OF BLOCKS IN A FILESYSTEM
};
//{size:1033, uid:1, gid:1, mode:33261, linkcount:2, atime:1323630836, ctime:1323630836, mtime:1323630836, indirect:0 location:2444}

struct fileAttributes
{
int size;		//SIZE OF FILE
int uId;		//USER ID 
int gId;		//GROUP ID
int Mode;		//USER MODE
int linkcount;	//LINK COUNT 
int aTime;		//ACCESS TIME
int cTime;		//CREATION TIME
int mTime;		//MODIFIED TIME
int indirect;	//INDIRECT IS 0 IF SIZE_OF_DATA IS LESS THAN 4096 BUT IF IT IS GREATER THAN 4096 THEN IT IS (SIZE_OF_DATA/4096)
int location;	//LOCATION FOR DATA BLOCK
};

struct dirAttributes rootDir;
struct fileAttributes newFile;
struct dirAttributes newDir;

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Inialialization Of filesystem starts at this point. 
Implementated init function which creates 10000 blocks(fusedata files)
for i = 0 ----> superblock
for i = 1 to 25 ----> freeblock list
for i = 26 ----> root
for i = 27 to 10000 ----> data blocks
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

static void* vk_init(struct fuse_conn_info *conn)
{
	int fileNo;
	char fName[64];
	int c_time,mount;
	//learned about opening multiple files from :
	//http://cboard.cprogramming.com/c-programming/132421-need-help-opening-multiple-files-c-programming.html
	FILE *infile;		
	for(fileNo = 0; fileNo < 1; fileNo++)
	{   sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
		struct superBlock sBlock;
		sBlock.creation_time = (int)time(NULL);
		sBlock.mounted = 1;
		sBlock.devId = 20;
		sBlock.freeStart = 1;
		sBlock.freeEnd = 25;
		sBlock.root = 26;
		sBlock.maxBlocks =10000;
// IF SUPERBLOCK ALREADY EXIST DONT CREATE A NEW BUT ONLY INCREMENT MOUNTED SECTION
		if( access( fName, F_OK ) != 0)
		{	
			infile = fopen(fName, "w");
			fprintf(infile, "%d,%d,%d,%d,%d,%d,%d\n",sBlock.creation_time,sBlock.mounted,sBlock.devId,sBlock.freeStart,sBlock.freeEnd,sBlock.root,sBlock.maxBlocks);
		
		}
		else
		{	
			infile = fopen(fName, "r");
			fscanf(infile,"%d,%d",&c_time,&mount); 
			fclose(infile);
			mount ++;
			infile = fopen(fName, "w");
			fprintf(infile, "%d,%d,%d,%d,%d,%d,%d\n",c_time,mount,sBlock.devId,sBlock.freeStart,sBlock.freeEnd,sBlock.root,sBlock.maxBlocks);
		}

		fclose(infile);

	}
	for(fileNo = 1; fileNo < 2; fileNo++)
	{	
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
//IF FREE BLOCK LIST ALREADY EXIST DO NOTHING
		if( access( fName, F_OK ) != 0)
		{
    		infile = fopen(fName, "w");
			int freeBlockNumber;
			for(freeBlockNumber = 27; freeBlockNumber < 400; freeBlockNumber++)
			{
				fprintf(infile,"%d,", freeBlockNumber);	
			}
			fclose(infile);
		}
		else
		{
			//
		}		
	}
	for(fileNo = 2; fileNo < 26; fileNo++)
	{
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
		if( access( fName, F_OK ) != 0)
		{
    		infile = fopen(fName, "w");
			int freeBlockNumber;
			for(freeBlockNumber = (fileNo-1)*400; freeBlockNumber < fileNo*400; freeBlockNumber++)
			{
				fprintf(infile,"%d,", freeBlockNumber);	
			}
			fclose(infile);
		}
		else
		{
			//
		}	
	}

// INTIALIZING THE ROOT DIRECTORY BLOCK WITH INITIAL ATTRIBUTES

	for(fileNo = 26; fileNo < 27; fileNo++)
	{
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);		
		if( access( fName, F_OK ) != 0)
		{
    		infile = fopen(fName, "w");
			rootDir.dirSize=50;
			rootDir.uId = getuid();
			rootDir.gId = getgid();
			rootDir.Mode = S_IFDIR | 0755;
			rootDir.aTime = (int)time(NULL);
			rootDir.cTime = (int)time(NULL);
			rootDir.mTime = (int)time(NULL);
			rootDir.linkcount = 2;
			strcpy(rootDir.filetoInode[0].fileStructtype,"d");
			strcpy(rootDir.filetoInode[0].fileName,".");
			strcpy(rootDir.filetoInode[0].inodeNumber,"26");
			strcpy(rootDir.filetoInode[1].fileStructtype,"d");
			strcpy(rootDir.filetoInode[1].fileName,"..");
			strcpy(rootDir.filetoInode[1].inodeNumber,"26");
			fprintf(infile,"%d %d %d %d %d %d %d %d %s %s %s %s %s %s",
				rootDir.dirSize,rootDir.uId,rootDir.gId,rootDir.Mode,rootDir.aTime,rootDir.cTime,rootDir.mTime,rootDir.linkcount,rootDir.filetoInode[0].fileStructtype,rootDir.filetoInode[0].fileName,
				rootDir.filetoInode[0].inodeNumber,rootDir.filetoInode[1].fileStructtype,rootDir.filetoInode[1].fileName,rootDir.filetoInode[1].inodeNumber);
			fclose(infile);
		}
		else
		{	
//IF ROOT ALREADY EXISTS THEN COPY STRUCT INFORMATION AS THIS WILL BE USED TO LOAD PREVIOUS FILES AND DIRECTORIES OF A FILESYSTEM
			char chars[100],buffer[100];
			int i = 0;
			infile = fopen(fName, "r");
			while (!feof(infile))
			{	
				if (i == 8)
					{
						break;
					}	
				fscanf(infile,"%s",chars);
				buffer[i] = atoi(chars);
				i++;
			}
			rootDir.dirSize=buffer[0];
			rootDir.uId = buffer[1];
			rootDir.gId = buffer[2];
			rootDir.Mode = buffer[3];
			rootDir.aTime = buffer[4];
			rootDir.cTime = buffer[5];
			rootDir.mTime = buffer[6];
			rootDir.linkcount = buffer[7];

			char buffer1[100][100];
			int j = 0;
			while (!feof(infile))
			{	
				fscanf(infile,"%s",chars);
				strcpy(buffer1[j],chars);
				rootinodeno = (int)j/3;
				j++;
			}
			rootinodeno++;
			int count = 0;
			int counter;
			counter = 0;
			while(count<j)
			{	
				strcpy(rootDir.filetoInode[counter].fileStructtype,buffer1[count]);
				count++;
				strcpy(rootDir.filetoInode[counter].fileName,buffer1[count]);
				count ++;
				strcpy(rootDir.filetoInode[counter].inodeNumber,buffer1[count]);
				count ++;
				counter++;
			}
		fclose(infile);

		}		
	}	
//INILIALIZATION OF FILE DATA BLOCKS
	for(fileNo = 27; fileNo < maximumBlocks; fileNo++)
	{
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
		if( access( fName, F_OK ) != 0)
		{
    		infile = fopen(fName, "w");
			fprintf(infile, "%04096d", 0);
			fclose(infile);
		}
		else
		{
			//
		}	
	}
	
return 0;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
This function is a fuction which helps other function in file operation. This Function is used by mkdir,readdir, create,read...
All the attributes are returned by statfs is populated 
[Return file attributes. The "stat" structure is described in detail in the stat(2) manual page. 
	For the given pathname, this should fill in the elements of the "stat" structure.
	If a field is meaningless or semi-meaningless (e.g., st_ino) then it should be set to 0 or given a "reasonable" value.
	This call is pretty much required for a usable filesystem.] 
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	int i;
	int flag;
	flag = 0;

	if (strcmp(path, "/") == 0) 
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = rootinodeno;
		flag =1;
	} 
	for(i=2;i<=rootinodeno;i++)
	{
		if (strcmp(path+1, rootDir.filetoInode[i].fileName) == 0) 
		{	
			if(strcmp("d",rootDir.filetoInode[i].fileStructtype) == 0)
			{
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				stbuf->st_size = 4096;
				stbuf->st_mtime = time(NULL);
				stbuf->st_uid = getuid();
				stbuf->st_gid = getgid();
			}
			else if(strcmp("f",rootDir.filetoInode[i].fileStructtype) == 0)
			{
				stbuf->st_mode = S_IFREG | 0444;
				stbuf->st_nlink = 2;
				stbuf->st_mtime = time(NULL);
				stbuf->st_uid = getuid();
				stbuf->st_gid = getgid();
				//stbuf->st_size = 4096;
			}
			flag = 1;
	}
	}
	if(flag == 0)
		{
			res = -ENOENT;
		}
return res;

}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
MKDIR FUNCTION IS IMPLEMENTED:
FIRST WE LOOK FOR FREE FILE BLOCK.---------->freeBlock()
		IT USES FILESIZE CHECKING METHOD WHICH RETURNS THE SIZE USED TO CHECK IF THERE ARE BLOCK(1 TO 25) fileSize(FILE *freeblkfile)
FREEBLOCK IS USED TO WRITE TO THAT FREEBLOCK --------> writeDir(int DirNo)
REMOVE NEWLY ALLOCATED FREEBLOCK FROM FREE_BLOCK_LIST
UPDATE PARENT TO INODE DICTIONARY

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int fileSize(FILE *freeblkfile)
{
	size_t fileSize;
	int seekPosition = ftell(freeblkfile);
	fseek(freeblkfile,0,SEEK_END);
	fileSize= ftell(freeblkfile);
	fseek(freeblkfile,seekPosition,SEEK_SET);

return (int)(fileSize-1);
}

static void writeDir(int DirNo)
{	
	char fName[64],current_dir[100],parent_dir[100];
	FILE *newDirectory;
	int parent = 26;
	sprintf(fName,"%s%d",MetaDataPrefix,DirNo);
	newDirectory = fopen(fName,"w");
		snprintf(current_dir,10,"%d",DirNo);
		snprintf(parent_dir,10,"%d",parent);
		newDir.dirSize=50;
		newDir.uId = getuid();
		newDir.gId = getgid();
		newDir.Mode = S_IFDIR | 0755;
		newDir.aTime = (int)time(NULL);
		newDir.cTime = (int)time(NULL);
		newDir.mTime = (int)time(NULL);
		newDir.linkcount = 2;
		strcpy(newDir.filetoInode[0].fileStructtype,"d");
		strcpy(newDir.filetoInode[0].fileName,".");
		strcpy(newDir.filetoInode[0].inodeNumber,current_dir);
		strcpy(newDir.filetoInode[1].fileStructtype,"d");
		strcpy(newDir.filetoInode[1].fileName,"..");
		strcpy(newDir.filetoInode[1].inodeNumber,parent_dir);
		fprintf(newDirectory,"%d %d %d %d %d %d %d %d %s %s %s %s %s %s",
			newDir.dirSize,newDir.uId,newDir.gId,newDir.Mode,newDir.aTime,newDir.cTime,newDir.mTime,newDir.linkcount,newDir.filetoInode[0].fileStructtype,newDir.filetoInode[0].fileName,
			newDir.filetoInode[0].inodeNumber,newDir.filetoInode[1].fileStructtype,newDir.filetoInode[1].fileName,newDir.filetoInode[1].inodeNumber);
	fclose(newDirectory);

}

void addblockno(int fileNo)
{	
	char fName[64];
	int BlockNo,i;
	BlockNo = (fileNo / 400)+ 1;
	FILE *file;
	sprintf(fName,"%s%d",MetaDataPrefix,BlockNo); 
	file=fopen(fName,"a+");
		fprintf(file, "%d,", fileNo);
	fclose(file);
	sprintf(fName,"%s%d",MetaDataPrefix,fileNo); 
	file=fopen(fName,"w");
		for(i=1;i<=4096;i++)
		{
		fprintf(file, "%d",0);
		}
	fclose(file);
}

void removedirno(int fileNo)
{
	char fName[64];
	FILE *file;
	sprintf(fName,"%s%d",MetaDataPrefix,fileNo); 
	file=fopen(fName,"r"); 
	char ch; 
	int flag = 0; 
	int i = 0; 
	char chars[4096]; 
	while((ch = fgetc(file)) != EOF ) 
	{
		if((ch != ',') && (flag == 0)) 
		{ 
			continue;
		} 
		if((ch ==',') && (flag == 0)) 
		{ 
			flag = 1; 
			continue; 
		} 
		chars[i] = ch; 
		i++; 
	}
	chars[i] = '\0';
	fclose(file);

	file = fopen(fName,"w"); 
		fputs(chars,file);
	fclose(file);
}

static int freeBlock()
{	
	char fName[64];
	int fileNo;
	int freeBlkSize;
	for(fileNo = 1;fileNo < 26;fileNo++)
	{
		FILE *freeblkfile;
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
		freeblkfile = fopen(fName, "r");
		freeBlkSize = fileSize(freeblkfile);
		
		if (freeBlkSize < 1)
		{
			fclose(freeblkfile);
			continue;
		}	
		else
		{
			char ch; 
			int flag = 0; 
			int i = 0; 
			char chars[4096]; 
			while((ch = fgetc(freeblkfile)) != EOF ) 
			{
				if((ch != ',') && (flag == 0)) 
				{ 
					chars[i] = ch;
					i++; 
					continue;
				} 
				if((ch ==',') && (flag == 0)) 
				{ 
					flag = 1; 
					break;
				} 
			}
		fclose(freeblkfile);
		writeDir((atoi(chars)));
		removedirno(fileNo);
		return (atoi(chars));
		}
	}	
	return 0;
}

static void parent_inode(const char* parent,int DirNo)
{
	FILE *parentdir;
	char fName[64],buffer[64];
	sprintf(buffer,"%d",DirNo);
	sprintf(fName,"%s%d",MetaDataPrefix,26);
		strcpy(rootDir.filetoInode[rootinodeno].fileStructtype,"d");
		strcpy(rootDir.filetoInode[rootinodeno].fileName,parent+1);
		strcpy(rootDir.filetoInode[rootinodeno].inodeNumber,buffer);
	parentdir = fopen(fName,"a+");
	fprintf(parentdir," %s %s %s",rootDir.filetoInode[rootinodeno].fileStructtype,rootDir.filetoInode[rootinodeno].fileName,rootDir.filetoInode[rootinodeno].inodeNumber);
	fclose(parentdir);
	rootinodeno++;	
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Implementation
This function creates a directory.
	Find a free block
	write metadata to free block and update same in root directory.
For traversal (after cd) we need to calculate absolute path and use the same function after that. but my absolute path function had a compilation error.
I am using strtok to check for "/" in the new path.
use the first  token and compare it with parent_to_inode table.
Use the inode_number(block number) as parent and update details(parenttoinode dict)
This will help getattr to obtain appropriate attributes.
Used the functions above to call and get the operation done
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_mkdir(const char* path, mode_t mode)
{	
	int DirNo;
	DirNo = freeBlock();
	parent_inode(path,DirNo);
	return 0;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::opendir)(const char *, struct fuse_file_info *)
Open directory
Unless the 'default_permissions' mount option is given, this method should check if opendir is permitted for this directory.
Optionally opendir may also return an arbitrary filehandle in the fuse_file_info structure, which will be passed to readdir, closedir and fsyncdir.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_opendir(const char* path,struct fuse_file_info* fi)
{
	int i;
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	if (strcmp(path, "/") == 0)
		return (fi->fh);

	for(i=2 ; i<rootinodeno;i++)
		{
			if(strcmp(rootDir.filetoInode[i].fileName,path+1) == 0)
			{	if(strcmp(rootDir.filetoInode[i].fileStructtype,"d") == 0)
				return (fi->fh);
			}
		}
	return -ENOENT;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::release)(const char *, struct fuse_file_info *)
Release an open file
Release is called when there are no more references to an open file: all file descriptors are closed and all memory mappings are unmapped.
For every open() call there will be exactly one release() call with the same flags and file descriptor. It is possible to have a file opened more than once, in which case only the last release will mean, that no more reads/writes will happen on the file. 
The return value of release is ignored.

-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_release(const char *path, struct fuse_file_info *fi)
{
	int i;
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	if (strcmp(path, "/") == 0)
		return (fi->fh);

	for(i=2 ; i<rootinodeno;i++)
		{
			if(strcmp(rootDir.filetoInode[i].fileName,path+1) == 0)
			{	if(strcmp(rootDir.filetoInode[i].fileStructtype,"f") == 0)
				return (fi->fh);
			}
		}
	return -ENOENT;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::releasedir)(const char *, struct fuse_file_info *)
Release directory


-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_releasedir(const char *path, struct fuse_file_info *fi)
{
	int i;
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	if (strcmp(path, "/") == 0)
		return (fi->fh);

	for(i=2 ; i<rootinodeno;i++)
		{
			if(strcmp(rootDir.filetoInode[i].fileName,path+1) == 0)
			{	if(strcmp(rootDir.filetoInode[i].fileStructtype,"d") == 0)
				return (fi->fh);
			}
		}
	return -ENOENT;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void(* fuse_operations::destroy)(void *)
Clean up filesystem
Called on filesystem exit
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void vk_destroy(void* userdata)
{
	fprintf(stderr, "Destroy function called\n");
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::readdir)(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *)
Read directory
The filesystem may choose between two modes of operation:
1) The readdir implementation ignores the offset parameter, and passes zero to the filler function's offset. 
The filler function will not return '1' (unless an error happens), so the whole directory is read in a single readdir operation.
2) The readdir implementation keeps track of the offsets of the directory entries. 
It uses the offset parameter and always passes non-zero offset to the filler function.
When the buffer is full (or an error happens) the filler function will return '1'.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	int i;
		if (strcmp(path, "/") != 0)
		{
			return -ENOENT;
		}
		filler(buf,".", NULL, 0);
		filler(buf,"..",NULL, 0);
		for(i=2;i < rootinodeno; i++)
		{ 
			filler(buf, rootDir.filetoInode[i].fileName,NULL,0);
		}

	return 0;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::open)(const char *, struct fuse_file_info *)
File open operation
No creation (O_CREAT, O_EXCL) and by default also no truncation (O_TRUNC) flags will be passed to open(). If an application specifies O_TRUNC, fuse first calls truncate() and then open().
Only if 'atomic_o_trunc' has been specified and kernel version is 2.6.24 or later, O_TRUNC is passed on to open.
Unless the 'default_permissions' mount option is given, open should check if the operation is permitted for the given flags.
Optionally open may also return an arbitrary filehandle in the fuse_file_info structure, which will be passed to all file operations.
IMPLEMENTATION:
Find if file exist.
if it exist return the file handler
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_open(const char *path, struct fuse_file_info *fi)
{
	int i;
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	
	if (strcmp(path, "/") == 0)
		return (fi->fh);

	for(i=2 ; i<rootinodeno;i++)
		{
			if(strcmp(rootDir.filetoInode[i].fileName,path+1) == 0)
			{	if(strcmp(rootDir.filetoInode[i].fileStructtype,"f") == 0)
				return (fi->fh);
			}
		}
	return -ENOENT;

}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Read function Implementation:
Fild file according to path in filesystem
open the file in read mode

int(* fuse_operations::read)(const char *, char *, size_t, off_t, struct fuse_file_info *)
Read data from an open file
Read should return exactly the number of bytes requested except on EOF or error, otherwise the rest of the data will be substituted with zeroes.
An exception to this is when the 'direct_io' mount option is specified, in which case the return value of the read system call will reflect the return value of this operation.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	char fileNo[10];
	char fName[64];
	int i,str1,str2,str3,str4,str5,str6,str7,str8,str9,loc;

	for(i=2 ; i<rootinodeno;i++)
		{
			if(strcmp(rootDir.filetoInode[i].fileName,path+1) == 0)
			{	
				strcpy(fileNo,rootDir.filetoInode[i].inodeNumber);
				break;
			}
		}
	FILE *new_File;
	sprintf(fName,"%s%s",MetaDataPrefix,fileNo);
	new_File = fopen(fName,"r");
	fscanf(new_File,"%d %d %d %d %d %d %d %d %d %d",&str1,&str2,&str3,&str4,&str5,&str6,&str7,&str8,&str9,&loc);
	fclose(new_File);
	sprintf(fName,"%s%d",MetaDataPrefix,loc);
	new_File = fopen(fName,"r");
	int file_size =fileSize(new_File);
	char *data = malloc(file_size+1);
	fread(data,file_size,1,new_File);
	fclose(new_File);
	len = strlen(data);
	if (offset < len) {
		if (offset + file_size > len)
			file_size = len - offset;
		memcpy(buf, data + offset, file_size);
		//printf("%s\n", buf);
	} else
		file_size = 0;

	return file_size;
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Create Function
{size:1033, uid:1, gid:1, mode:33261, linkcount:2, atime:1323630836, ctime:1323630836, mtime:1323630836, indirect:0 location:2444}
 Function for create
FIND FREE BLOCK AND RETURN IT TO WRITE METH. THIS FREE BLOCK WILL BE USED TO WRITE DATA ASSOCIATED WITH CURRENT BLOCK
-------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int freefile_Block()
{	
	char fName[64];
	int fileNo;
	int freeBlkSize;
	for(fileNo = 1;fileNo < 26;fileNo++)
	{
		FILE *freeblkfile;
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
		freeblkfile = fopen(fName, "r");
		freeBlkSize = fileSize(freeblkfile);
		
		if (freeBlkSize < 1)
		{
			fclose(freeblkfile);
			continue;
		}	
		else
		{
			char ch; 
			int flag = 0; 
			int i = 0; 
			char chars[4096]; 
			while((ch = fgetc(freeblkfile)) != EOF ) 
			{
				if((ch != ',') && (flag == 0)) 
				{ 
					chars[i] = ch;
					i++; 
					continue;
				} 
				if((ch ==',') && (flag == 0)) 
				{ 
					flag = 1; 
					break;
				} 
			}
		fclose(freeblkfile);
		removedirno(fileNo);
	return (atoi(chars));
		}
	}	
return 0;
}

static int writeFile(int freeBlockNumber)
{	//fprintf(stderr, "writeFile\n");
	char fName[64],current_file[100],parent_dir[100];
	FILE *new_File;
	int parent = 26;
	sprintf(fName,"%s%d",MetaDataPrefix,freeBlockNumber);
	//fprintf(stderr, "%s\n",fName );
	snprintf(current_file,10,"%d",freeBlockNumber);
	snprintf(parent_dir,10,"%d",parent);
			newFile.size = 0;
			newFile.uId = getuid();
			newFile.gId = getgid();
			newFile.Mode = S_IFREG | 0444;
			newFile.aTime = (int)time(NULL);
			newFile.cTime = (int)time(NULL);
			newFile.mTime = (int)time(NULL);
			newFile.linkcount = 2;
			newFile.indirect = 0;
			int filedatano = freefile_Block();
			newFile.location = filedatano;
			new_File = fopen(fName,"w");		
			fprintf(new_File,"%d %d %d %d %d %d %d %d %d %d",
				newFile.size,newFile.uId,newFile.gId,newFile.Mode,newFile.aTime,newFile.cTime,newFile.mTime,newFile.linkcount,newFile.indirect,newFile.location);
	fclose(new_File);
	return 0;
}


/*
FIND FREE BLOCK FROM FREE BLOCK LIST
REMOVE NEW BLOCK FROM FREE BLOCK LIST
WRITE ATTRIBUTES OF FILE TO NEW BLOCK
*/
static int freefileBlock()
{	
	char fName[64];
	int fileNo;
	int freeBlkSize;
	for(fileNo = 1;fileNo < 26;fileNo++)
	{
		FILE *freeblkfile;
		sprintf(fName,"%s%d",MetaDataPrefix,fileNo);
		freeblkfile = fopen(fName, "r");
		freeBlkSize = fileSize(freeblkfile);
		
		if (freeBlkSize < 1)
		{
			fclose(freeblkfile);
			continue;
		}	
		else
		{
			char ch; 
			int flag = 0; 
			int i = 0; 
			char chars[4096]; 
			while((ch = fgetc(freeblkfile)) != EOF ) 
			{
				if((ch != ',') && (flag == 0)) 
				{ 
					chars[i] = ch;
					i++; 
					continue;
				} 
				if((ch ==',') && (flag == 0)) 
				{ 
					flag = 1; 
					break;
				} 
			}
			fclose(freeblkfile);
			removedirno(fileNo);
			writeFile((atoi(chars)));
			return (atoi(chars));
		}
	}	
	return 0;
}

static void parent_inode_file(const char* parent,int datablock)
{
	FILE *parentdir;
	char fName[64],buffer[64];
	sprintf (buffer,"%d",datablock);
	sprintf(fName,"%s%d",MetaDataPrefix,26);
		strcpy(rootDir.filetoInode[rootinodeno].fileStructtype,"f");
		strcpy(rootDir.filetoInode[rootinodeno].fileName,parent+1);
		strcpy(rootDir.filetoInode[rootinodeno].inodeNumber,buffer);
	parentdir = fopen(fName,"a+");
	fprintf(parentdir," %s %s %s",rootDir.filetoInode[rootinodeno].fileStructtype,rootDir.filetoInode[rootinodeno].fileName,rootDir.filetoInode[rootinodeno].inodeNumber);
	fclose(parentdir);
	rootinodeno++;	
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::create)(const char *, mode_t, struct fuse_file_info *)
Create and open a file
If the file does not exist, first create it with the specified mode, and then open it.
If this method is not implemented or under Linux kernel versions earlier than 2.6.15, the mknod() and open() methods will be called instead.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

static int vk_create(const char *path, mode_t mode, struct fuse_file_info * fi) 
{
	int freeBlockNumber;
	freeBlockNumber = freefileBlock();
	parent_inode_file(path,freeBlockNumber);
	return 0;
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::write)(const char *, const char *, size_t, off_t, struct fuse_file_info *)
Write data to an open file
Write should return exactly the number of bytes requested except on error. An exception to this is when the 'direct_io' mount option is specified (see read operation).
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

static int vk_write(const char * path, const char * buf, size_t buff_size, off_t offset, struct fuse_file_info * fi) 
{
	char fileNo[10];
	char fName[64];
	int i,str1,str2,str3,str4,str5,str6,str7,str8,str9,loc;
	for(i=2 ; i<rootinodeno;i++)
		{
			if(strcmp(rootDir.filetoInode[i].fileName,path + 1) == 0 && strcmp(rootDir.filetoInode[i].fileStructtype,"f")==0)
			{	
				strcpy(fileNo,rootDir.filetoInode[i].inodeNumber);
				break;
			}
		}
		/*Could not implement it but have just mentioned the logic.
	if(size > 4096)
	{
	int no_of blocks;
	no_of blocks = size/4096;
	no_of_loc = no_of_blocks;
	indirect = no_of_blocks;
	for(count = 0;count <=no_of_blocks+1;count++)
	{
	loc =findfreeblocks()
	} 
	Use this loc to write
	for (ch = 1;ch <4096;ch++){
	writeFile();
	}
	}
	*/
	FILE *new_File;
	sprintf(fName,"%s%s",MetaDataPrefix,fileNo);
	new_File = fopen(fName,"r");
	fscanf(new_File,"%d %d %d %d %d %d %d %d %d %d",&str1,&str2,&str3,&str4,&str5,&str6,&str7,&str8,&str9,&loc);
	fclose(new_File);
	sprintf(fName,"%s%d",MetaDataPrefix,loc);

	new_File = fopen(fName,"w");
	fprintf(new_File, "%s", buf);
	fclose(new_File);

	return sizeof(buf);
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::unlink)(const char *)
Remove a file

Implementation:
find file in the filesystem
find assocaited data file
delete data file
remove a file from root dirctory
update parent_inode table
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
int vk_unlink(const char * path) {
	int i,j,BlockNo,str1,str2,str3,str4,str5,str6,str7,str8,str9,loc;
	char fName[64];
	for(i=2 ; i<rootinodeno;i++)
	{
		if(strcmp(rootDir.filetoInode[i].fileName,path + 1) == 0)
		{	
			BlockNo = atoi(rootDir.filetoInode[i].inodeNumber);
			FILE *data_File;
			sprintf(fName,"%s%d",MetaDataPrefix,BlockNo);
			data_File = fopen(fName,"w");
			fscanf(data_File,"%d %d %d %d %d %d %d %d %d %d",&str1,&str2,&str3,&str4,&str5,&str6,&str7,&str8,&str9,&loc);
			fclose(data_File);
			addblockno(BlockNo);
			addblockno(loc);
			for(j= i+1 ; j <=rootinodeno ;j++)
			{
				strcpy(rootDir.filetoInode[i].fileName,rootDir.filetoInode[j].fileName);
				strcpy(rootDir.filetoInode[i].inodeNumber,rootDir.filetoInode[j].inodeNumber);
				strcpy(rootDir.filetoInode[i].fileStructtype,rootDir.filetoInode[j].fileStructtype);
			}
			rootinodeno--;
		break;
		}
	}
	FILE *rootfile;
	sprintf(fName,"%s%d",MetaDataPrefix,26);
	rootfile = fopen(fName,"w");
	rootDir.dirSize=50;
	rootDir.uId = getuid();
	rootDir.gId = getgid();
	rootDir.Mode = S_IFDIR | 0755;
	rootDir.aTime = (int)time(NULL);
	rootDir.cTime = (int)time(NULL);
	rootDir.mTime = (int)time(NULL);
	rootDir.linkcount = 2;
	fprintf(rootfile,"%d %d %d %d %d %d %d %d",rootDir.dirSize,rootDir.uId,rootDir.gId,rootDir.Mode,rootDir.aTime,rootDir.cTime,rootDir.mTime,rootDir.linkcount);
	fclose(rootfile);
	rootfile = fopen(fName,"a+");
	for(i=0 ; i < rootinodeno;i++)
	{
		fprintf(rootfile,"%s %s %s",rootDir.filetoInode[i].fileStructtype,rootDir.filetoInode[i].fileName,rootDir.filetoInode[i].inodeNumber);
	}
	fclose(rootfile);

return 0;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
int(* fuse_operations::link)(const char *, const char *)
Create a hard link to a file
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_link(const char* from,const char * to)
{
	int i,og_file_no;
	char og_file[100];
	char link_file[100];
	strcpy(og_file,from+1);
	strcpy(link_file,to);
	char fName[64];
	int file_size;
	for(i=2 ; i<rootinodeno;i++)
	{	
		if(strcmp(rootDir.filetoInode[i].fileName,og_file) == 0 && strcmp(rootDir.filetoInode[i].fileStructtype,"f") == 0 )
		{	
			fprintf(stderr, "%s\n", rootDir.filetoInode[i].fileName);
			fprintf(stderr, "%s\n", og_file);
			og_file_no = atoi(rootDir.filetoInode[i].inodeNumber);
			FILE *inode_File;
			sprintf(fName,"%s%d",MetaDataPrefix,og_file_no);
			inode_File = fopen(fName,"r");
			file_size =fileSize(inode_File);
			char *data = malloc(file_size + 2);
			fread(data,file_size,1,inode_File);
			fclose(inode_File);
			int link_file_no = freefile_Block();
			parent_inode_file(link_file,link_file_no);
			sprintf(fName,"%s%d",MetaDataPrefix,link_file_no);
			inode_File = fopen(fName,"w");
			fprintf(inode_File,"%s",data);
			fclose(inode_File);
		return 0;
		}
	}
return -ENOENT;	
	
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Rename function is used to change name of file in the filesystem
Implementation:
find file if it exist in the filesystem
update the file name to new name
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_rename(const char *from, const char *to)
{
	int i;
	char og_file[100];
	char new_file[100];
	char fName[64];
	strcpy(og_file,from+1);
	strcpy(new_file,to+1);
	for(i=2 ; i<rootinodeno;i++)
	{
		if(strcmp(rootDir.filetoInode[i].fileName,og_file) == 0 && strcmp(rootDir.filetoInode[i].fileStructtype,"f") == 0 )
		{
			strcpy(rootDir.filetoInode[i].fileName,new_file);
			FILE *rootfile;
			sprintf(fName,"%s%d",MetaDataPrefix,26);
			rootfile = fopen(fName,"w");
			rootDir.dirSize=50;
			rootDir.uId = getuid();
			rootDir.gId = getgid();
			rootDir.Mode = S_IFDIR | 0755;
			rootDir.aTime = (int)time(NULL);
			rootDir.cTime = (int)time(NULL);
			rootDir.mTime = (int)time(NULL);
			rootDir.linkcount = 2;
			fprintf(rootfile,"%d %d %d %d %d %d %d %d",rootDir.dirSize,rootDir.uId,rootDir.gId,rootDir.Mode,rootDir.aTime,rootDir.cTime,rootDir.mTime,rootDir.linkcount);
			fclose(rootfile);
			rootfile = fopen(fName,"a+");
			for(i=0 ; i < rootinodeno;i++)
			{
				fprintf(rootfile,"%s %s %s",rootDir.filetoInode[i].fileStructtype,rootDir.filetoInode[i].fileName,rootDir.filetoInode[i].inodeNumber);
			}
			fclose(rootfile);
			return 0;
		}
	}	
return -ENOENT;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
Statfs returns attributes of files and directories in a filesystem. These attributes are used by getattr.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
static int vk_statfs(const char *path, struct statvfs *buf)
{	
	 unsigned long  f_bsize = 4096;
	 return 0;
	 return f_bsize;

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------

static struct fuse_operations vk_oper = {
	.getattr	= vk_getattr,
	.readdir	= vk_readdir,
	.open		= vk_open,
	.read		= vk_read,
	.init		= vk_init,
	.mkdir 		= vk_mkdir,
	.destroy	= vk_destroy,
	.opendir	= vk_opendir,
	.create 	= vk_create,
	.write 		= vk_write,
	.unlink 	= vk_unlink,
	.link 		= vk_link,
	.release 	= vk_release,
	.releasedir = vk_releasedir,
	.rename 	= vk_rename,
	.statfs		= vk_statfs,
};

int main(int argc, char *argv[])
{
	int ret;
	fprintf(stderr, "about to call fuse_main\n");
	ret= fuse_main(argc, argv, &vk_oper, NULL);
	fprintf(stderr, "fuse_main returned ");
	return ret;
}