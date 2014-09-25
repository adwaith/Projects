/* 
 * NAME:        Adwaith Venkataraman
 * 
 * ANDREW ID:   adwaithv
 */

#define _XOPEN_SOURCE 500
#define _ATFILE_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>
#include "cloudapi.h"
#include "cloudfs.h"
#include "dedup.h"
#include "uthash.h"
#include "rabin.h"

#define UNUSED __attribute__((unused))

struct cloudfs_state state_; 

/* Function Details
 * 
 * Purpose:           Open a buffer to write contents to.
 *
 * Name:              get_buffer
 *
 * Input Arguments:   Character pointer to the buffer;
 *                    Buffer length.
 *
 * Return value:      Number of bytes written into the buffer.
 */

static FILE *outfile;
int get_buffer(const char *buffer, int bufferLength) 
{
  return fwrite(buffer, 1, bufferLength, outfile);  
}

/* Function Details
 * 
 * Purpose:           Open a buffer to read contents from.
 *
 * Name:              put_buffer
 *
 * Input Arguments:   Character pointer to the buffer;
 *                    Buffer length.
 *
 * Return value:      Number of bytes read from the buffer.
 */

static FILE *infile;
int put_buffer(char *buffer, int bufferLength) 
{
  fprintf(stdout, "put_buffer %d \n", bufferLength);
  return fread(buffer, 1, bufferLength, infile);
}

/* Function Details
 * 
 * Purpose:           Open a log file in append mode;
 *                    Log file used to print debug/log messages;
 *                    Check if open fails, then return error;
 *                    Else, open and return the pointer.
 *
 * Name:              log_open
 *
 * Input Arguments:   none
 *
 * Return value:      If succesful, file pointer to the log file;
 *                    If unsuccesful, error.
 */

FILE *log_open()
{
  logfile = fopen("/tmp/cloudfs.log", "a");
  if (logfile == NULL)
  {
    perror("logfile");
    exit(EXIT_FAILURE);
  }
  chmod("cloudfs.log", 438);
  setvbuf(logfile, NULL, _IOLBF, 0);
  return logfile;
}

/* Function Details
 * 
 * Purpose:           Prints a debug/log message to the logfile;
 *                    Given a string, it is logged to the file.
 *
 * Name:              debug_msg
 *
 * Input Arguments:   character pointer to the string to be logged.
 *
 * Return value:      none
 */

void debug_msg(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vfprintf(logfile, format, ap);
}

/* Function Details 
 *
 * Purpose:           Prints a cloudfs function error.
 *
 * Name:              cloudfs_error
 *
 * Input Arguments:   character pointer to the string to be logged.
 *
 * Return value:      negative of error number generated, but unused here.
 */

static int UNUSED cloudfs_error(char *error_str)
{
    int retval = -errno;
    debug_msg("\ncloudfs_error\tERROR %s: %s\n", error_str, strerror(errno));
    fprintf(stderr, "CloudFS Error: %s\n", error_str);
    return retval;
}

/* Function Details 
 *
 * Purpose:           Convert the path from ssd format to cloud format.
 *                    Replace / with +. 
 *
 * Name:              path_to_cloudpath
 *
 * Input Arguments:   character pointer to full path on ssd.
 *
 * Return value:      none
 */

void path_to_cloudpath(char *fpath_cloud)
{
  int i;
  for (i=0; fpath_cloud[i] != '\0'; i++)
  {
    if (fpath_cloud[i] == '/')
      fpath_cloud[i] = '+';
  }
}

/* Function Details 
 *
 * Purpose:           Given the path of a file, it appends the mount path;
 *                    Mount path prefix in this case: /mnt/ssd/
 *
 * Name:              cloudfs_get_fullpath
 *
 * Input Arguments:   character pointer to path of file, as constant;
 *                    character pointer of fullpath, to store the final path.
 *
 * Return value:      none
 */

void cloudfs_get_fullpath(const char *path, char *fullpath)
{
  strcpy(fullpath, state_.ssd_path);
  strcat(fullpath, path);
}

/* Function Details 
 *
 * Purpose:           Initialize the FUSE file system (cloudfs);
 *                    Checks if mount points are valid;
 *                    If valid, then mounts it;
 *                    Initialize cloud hostname;
 *                    Creates a bucket on the cloud;
 *                    Creates a local cache folder (only if it doesnt exist).
 *
 * Name:              cloudfs_init
 *
 * Input Arguments:   Structure object of type fuse_conn_info;
 *                    Unused here
 *
 * Return value:      void pointer with NULL as value.
 */

void *cloudfs_init(struct fuse_conn_info *conn UNUSED)
{
  log_open();
  cloud_init(state_.hostname);
  cloud_create_bucket("test");
  cloud_print_error();
  char cache_fpath[MAX_PATH_LEN];
  char cache_path[MAX_PATH_LEN];
  
  strcpy(cache_path, ".cache/");
  cloudfs_get_fullpath(cache_path, cache_fpath);
  
  if (access(cache_fpath, F_OK) == -1)
  {  
    mkdir(cache_fpath, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)); 
    off_t file_size = 0;
    lsetxattr(cache_fpath, "user.adw", &file_size, sizeof(off_t), 0);
  }
  
  return NULL;
}

/* Function Details 
 *
 * Purpose:           Create file if it doesnt exist;
 *                    Set extended attribute i.e., file size to zero.
 *
 * Name:              cloudfs_create
 *
 * Input Arguments:   Path of the file relative to the mount point;
 *                    Mode in which the file is to be opened;
 *                    File info structure pointer.
 *
 * Return value:      Return value when file is opened.
 */

int cloudfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  int retval = 0;
  off_t file_size = 0;
  char fpath[MAX_PATH_LEN];
  int fd;
  cloudfs_get_fullpath(path, fpath);
  fd = creat(fpath, mode);
  lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
        
  if (fd < 0)
    fd = cloudfs_error("cloudfs_create creat");
  fi->fh = fd;
  return retval;
}

/* Function Details 
 *
 * Purpose:           Clean up the file system
 *
 * Name:              cloudfs_destroy
 *
 * Input Arguments:   Pointer to the user data
 *
 * Return value:      none
 */

void cloudfs_destroy(void *data UNUSED) 
{
  cloud_destroy();
}

/* Function Details 
 *
 * Purpose:           Get the file attributes;
 *                    If file on cloud, the extended attribute (size) is used.
 *
 * Name:              cloudfs_getattr
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to stat structure object to get attributes.
 *
 * Return value:      Returns the value returned by lstat;
 *                    If lstat returns undesired value, error return.
 */

int cloudfs_getattr(const char *path, struct stat *statbuf)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);

  off_t file_size = 0;
  retval = lstat(fpath, statbuf);
  int attribute_return = lgetxattr(fpath, "user.adw", &file_size, \
    sizeof(off_t));

  if (retval != 0)
    retval = cloudfs_error("cloudfs_getattr lstat");
  
  if (attribute_return > 0 && file_size > 0)
  {
    statbuf->st_size = file_size;
  }  
  return retval;
}

/* Function Details 
 *
 * Purpose:           Get the extended file attributes
 *
 * Name:              cloudfs_getxattr
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to name of the extended attribute;
 *                    Pointer to the value to store the extended attribute;
 *                    Size of the type of extended attribute.
 *
 * Return value:      Value returned by lgetxattr;
 *                    If lgetxattr returns undesired value, error return.
 */

int cloudfs_getxattr(const char *path, const char *name, char *value, \
  size_t size)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = lgetxattr(fpath, name, value, size);
  if (retval < 0)
  {
      retval = cloudfs_error("cloudfs_getxattr lgetxattr");
  }
  return retval;
}

/* Function Details 
 *
 * Purpose:           Set the extended file attributes
 *
 * Name:              cloudfs_setxattr
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to name of the extended attribute;
 *                    Pointer to the value to store the extended attribute;
 *                    Size of the type of extended attribute;
 *                    Flags with which to store the attribute.
 *
 * Return value:      Value returned by lgetxattr;
 *                    If lsetxattr returns undesired value, error return.
 */

int cloudfs_setxattr(const char *path, const char *name, const char *value, \
  size_t size, int flags)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = lsetxattr(fpath, name, value, size, flags);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_setxattr lsetxattr");

  return retval;
}

/* Function Details 
 *
 * Purpose:           Creates a directory
 *
 * Name:              cloudfs_mkdir
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Mode in which the directory is to be created.
 *
 * Return value:      Value returned by mkdir;
 *                    If mkdir returns undesired value, error return.
 */

int cloudfs_mkdir(const char *path, mode_t mode)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = mkdir(fpath, mode);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_mkdir mkdir");

  return retval;
}

/* Function Details 
 *
 * Purpose:           Creates a file node;
 *                    If no create(), mknod() will be called.
 *
 * Name:              cloudfs_mknod
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Mode in which the node is to be created;
 *                    Device on which the node is to be created.
 *
 * Return value:      Value returned by open / close / mkinfo / mknod;
 *                    If either return undesired value, error return.
 */

int cloudfs_mknod(const char *path, mode_t mode, dev_t dev)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  if (S_ISREG(mode))
  {
    retval = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (retval < 0)
    {
        retval = cloudfs_error("cloudfs_mknod open");
    }
    else
    {
      retval = close(retval);
      if (retval < 0)
        retval = cloudfs_error("cloudfs_mknod close");
    }
  }
  else
    if (S_ISFIFO(mode))
    {
      retval = mkfifo(fpath, mode);
      if (retval < 0)
        retval = cloudfs_error("cloudfs_mknod mkfifo");
    }
    else
    {
      retval = mknod(fpath, mode, dev);
      if (retval < 0)
        retval = cloudfs_error("cloudfs_mknod mknod");
    }
  return retval;
}

/* Function Details 
 *
 * Purpose:           Opens file pointed by the path;
 *                    If file on cloud and no dedup, fetch from cloud and open.
 *
 * Name:              cloudfs_open
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    File info structure pointer.
 *
 * Return value:      Returns zero on success;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_open(const char *path, struct fuse_file_info *fi)
{
  int retval = 0;
  int fd;
  int attribute_return = 0;
  char fpath[MAX_PATH_LEN];
  char fpath_cloud[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  strcpy(fpath_cloud, fpath);
  path_to_cloudpath(fpath_cloud);

  off_t file_size = 0;
  attribute_return = lgetxattr(fpath, "user.adw", &file_size, sizeof(off_t));
  
  if (attribute_return > 0 && (file_size > 0) && state_.no_dedup == 1)
  {
    outfile = fopen(fpath, "wb");
    cloud_get_object("test", fpath_cloud, get_buffer);
    fclose(outfile);
  }

  fd = open(fpath, fi->flags);
  if (fd < 0)
    retval = cloudfs_error("cloudfs_open open");
  fi->fh = fd;
  
  return retval;
}

/* Function Details 
 *
 * Purpose:           Reads from an open file;
 *                    If file on local, simply read from that file;
 *                    If file on cloud and no dedup, get file and read;
 *                    If file on cloud and deduplified, read proxy file.
 *
 * Name:              cloudfs_read
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to buffer to read data into;
 *                    Number of bytes to read from file;
 *                    Offset within the file to read from;
 *                    File info structure pointer.
 *
 * Return value:      Number of bytes read;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_read(const char *path UNUSED, char *buf, size_t size, \
  off_t offset, struct fuse_file_info *fi)
{
  int retval = 0;
  int attribute_return = 0;
  
  off_t file_size = 0;
  char fpath[MAX_PATH_LEN];
  char fpath_cloud[MAX_PATH_LEN];

  cloudfs_get_fullpath(path, fpath);
  
  strcpy(fpath_cloud, fpath);
  path_to_cloudpath(fpath_cloud);

  attribute_return = lgetxattr(fpath, "user.adw", &file_size, sizeof(off_t));
  
  if ((state_.no_dedup == 1) || (file_size == 0))
  {
    if (file_size > 0)
    {
      outfile = fopen(fpath, "wb");
      cloud_get_object("test", fpath_cloud, get_buffer);
      fclose(outfile);
    }
    retval = pread(fi->fh, buf, size, offset);
  }
  if (state_.no_dedup == 0 && file_size > 0)
  {
    retval = read_from_proxy_file(fpath, buf, offset, size);
  }
  
  if (retval < 0)
    retval = cloudfs_error("cloudfs_read pread");

  return retval;
}

/* Function Details 
 *
 * Purpose:           Write data into an open file.
 *
 * Name:              cloudfs_write
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to buffer to read data into;
 *                    Number of bytes to write into file;
 *                    Offset within the file to write into;
 *                    File info structure pointer.
 *
 * Return value:      Number of bytes written;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_write(const char *path UNUSED, const char *buf, size_t size, \
  off_t offset, struct fuse_file_info *fi)
{
  int retval = 0;
  retval = pwrite(fi->fh, buf, size, offset);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_write pwrite");

  return retval;
}

/* Function Details 
 *
 * Purpose:           Release an open file;
 *                    If no dedup, if size greater than threshold, to cloud;
 *                                 if size less than or equal to threshold
 *                                 keep in SSD;
 *                    If dedup required, if overall size greater than threshold
 *                    rabinize file and send segments to cloud;
 *
 * Name:              cloudfs_release
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    File info structure pointer.
 *
 * Return value:      Return value from closing file;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_release(const char *path UNUSED, struct fuse_file_info *fi)
{
  int retval = 0;
  int rabin_return = 1;
  int truncate_return = 0;
  int attribute_return = 0;
  char fpath[MAX_PATH_LEN];
  char fpath_cloud[MAX_PATH_LEN];
  
  cloudfs_get_fullpath(path, fpath);
  strcpy(fpath_cloud, fpath);
  path_to_cloudpath(fpath_cloud);
  
  struct stat statbuf;
  off_t file_size = -1;
  
  retval = lstat(fpath, &statbuf);
  attribute_return = lgetxattr(fpath, "user.adw", &file_size, sizeof(off_t));
  
  if (retval < 0)
    retval = cloudfs_error("cloudfs_release lstat");
  
  if (attribute_return > 0 && state_.no_dedup == 1)
  {
    if (file_size > 0)
    {
      if (statbuf.st_size <= state_.threshold)
      {
        file_size = 0;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
        cloud_delete_object("test", fpath_cloud);
      }
      else
      {
        infile = fopen(fpath, "rb");
        file_size = statbuf.st_size;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
        cloud_put_object("test", fpath_cloud, statbuf.st_size, put_buffer);
        fclose(infile);
        truncate_return = truncate(fpath, 0);
      }
    }
    else
    {
      if (statbuf.st_size > state_.threshold)
      {
        infile = fopen(fpath, "rb");
        file_size = statbuf.st_size;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
        cloud_put_object("test", fpath_cloud, statbuf.st_size, put_buffer);
        fclose(infile);
        truncate_return = truncate(fpath, 0);
      }
      else
      {
        file_size = 0;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
      }
    }
  }
  
  else
  {
    if (file_size > 0)
    {
      if (statbuf.st_size > 0)
      {
        file_size = file_size + statbuf.st_size;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
        rabin_return = rabin(fpath, state_.avg_seg_size, \
          state_.rabin_window_size);
        truncate_return = truncate(fpath, 0);
        file_size = 0;
      }
    }
    else
    {
      if (statbuf.st_size > state_.threshold)
      {
        file_size = statbuf.st_size;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
        rabin_return = rabin(fpath, state_.avg_seg_size, \
          state_.rabin_window_size);
        truncate_return = truncate(fpath, 0);
        file_size = 0;
      }
      else
      {
        file_size = 0;
        lsetxattr(fpath, "user.adw", &file_size, sizeof(off_t), 0);
      }
    }
  }
  
  retval = close(fi->fh);
  return retval;
}

/* Function Details 
 *
 * Purpose:           Open a directory specified by the path.
 *
 * Name:              cloudfs_opendir
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    File info structure pointer.
 *
 * Return value:      Returns zero;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_opendir(const char *path, struct fuse_file_info *fi)
{
  DIR *dp;
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  dp = opendir(fpath);
  if (dp == NULL)
    retval = cloudfs_error("cloudfs_opendir opendir");

  fi->fh = (intptr_t) dp;
  return retval;
}

/* Function Details 
 *
 * Purpose:           Read the entire directory
 *
 * Name:              cloudfs_readdir
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to buffer to read data into;
 *                    Filler value;
 *                    Offset within the directory entry;
 *                    File info structure pointer.
 *
 * Return value:      Returns zero;
 *                    Else returns negative of error.
 */

int cloudfs_readdir(const char *path UNUSED, void *buf, \
  fuse_fill_dir_t filler, off_t offset UNUSED, struct fuse_file_info *fi)
{
  int retval = 0;
  DIR *dp;
  struct dirent *de;
  dp = (DIR *) (uintptr_t) fi->fh;
  de = readdir(dp);
  if (de == 0)
  {
    return retval;
  }
  do
  {
    if (filler(buf, de->d_name, NULL, 0) != 0)
    {
      return -ENOMEM;
    }
  }while((de = readdir(dp)) != NULL);
  return retval;
}

/* Function Details 
 *
 * Purpose:           Checks file access permissions
 *
 * Name:              cloudfs_access
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Mask value.
 *
 * Return value:      Returns permission value;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_access(const char *path, int mask)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = access(fpath, mask);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_access access");
  return retval;
}

/* Function Details 
 *
 * Purpose:           Change access and/or modification times of a file.
 *
 * Name:              cloudfs_utimens
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Structure object with time specifics.
 *
 * Return value:      Return value from utimensat;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_utimens(const char *path, const struct timespec *tv)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = utimensat(0, fpath, tv, 0);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_utimensat utime");
  return retval;
}

/* Function Details 
 *
 * Purpose:           Change permissions of a file.
 *
 * Name:              cloudfs_chmod
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Permission mode desired.
 *
 * Return value:      Return value from chmod;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_chmod(const char *path, mode_t mode)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = chmod(fpath, mode);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_chmod chmod");
  return retval;
}

/* Function Details 
 *
 * Purpose:           If no dedup and file on cloud, delete from cloud;
 *                    If deduplified and on cloud, alter hash table values;
 *                    Remove the file represented by the path.
 *
 * Name:              cloudfs_unlink
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *
 * Return value:      Return value from unlink;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_unlink(const char *path)
{
  int retval = 0;
  int attribute_return = 0;
  char fpath[MAX_PATH_LEN];
  char fpath_cloud[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  strcpy(fpath_cloud, fpath);
  path_to_cloudpath(fpath_cloud);

  off_t file_size = 0;

  char *proxy_file = NULL;
  proxy_file = strrchr(fpath, '/');
  if (proxy_file)
  {
    proxy_file++;
    if (*proxy_file == '.')
    {
      return 0;
    }
  }

  attribute_return = lgetxattr(fpath, "user.adw", &file_size, sizeof(off_t));
  
  if (attribute_return > 0 && file_size > 0 && state_.no_dedup == 1)
  {
    cloud_delete_object("test", fpath_cloud);
  }

  if (file_size > 0 && state_.no_dedup == 0)
  {
    remove_from_hash_table(fpath);
  }

  retval = unlink(fpath);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_unlink unlink");

  return retval;
}

/* Function Details 
 *
 * Purpose:           Remove a directory represented by the path.
 *
 * Name:              cloudfs_rmdir
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *
 * Return value:      Return value from rmdir;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_rmdir(const char *path)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = rmdir(fpath);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_rmdir rmdir");
  return retval;
}

/* Function Details 
 *
 * Purpose:           Change owner and group of a file.
 *
 * Name:              cloudfs_chown
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    User id;
 *                    Group id.
 *
 * Return value:      Return value from chown;
 *                    Else returns error value from cloudfs_error.
 */

int cloudfs_chown(const char *path, uid_t uid, gid_t gid)
{
  int retval = 0;
  char fpath[MAX_PATH_LEN];
  cloudfs_get_fullpath(path, fpath);
  retval = chown(fpath, uid, gid);
  if (retval < 0)
    retval = cloudfs_error("cloudfs_chown chown");
  return retval;
}

/*
 * Functions supported by cloudfs 
 */

static 
struct fuse_operations cloudfs_operations = {
    .init           = cloudfs_init,
    .getattr        = cloudfs_getattr,
    .getxattr       = cloudfs_getxattr,
    .setxattr       = cloudfs_setxattr,
    .mkdir          = cloudfs_mkdir,
    .mknod          = cloudfs_mknod,
    .open           = cloudfs_open,
    .read           = cloudfs_read,
    .write          = cloudfs_write,
    .release        = cloudfs_release,
    .opendir        = cloudfs_opendir,
    .readdir        = cloudfs_readdir,
    .destroy        = cloudfs_destroy,
    .access         = cloudfs_access,
    .utimens        = cloudfs_utimens,
    .chmod          = cloudfs_chmod,
    .unlink         = cloudfs_unlink,
    .rmdir          = cloudfs_rmdir,
    .chown          = cloudfs_chown,
    .create         = cloudfs_create
};

/* Function Details 
 *
 * Purpose:           Initialized the fuse system;
 *                    Retrieves the hash table from the SSD.
 *
 * Name:              cloudfs_start
 *
 * Input Arguments:   State structure object;
 *                    Character pointer to the Fuse runtime name.
 *
 * Return value:      Return value from fuse_main.
 */

int cloudfs_start(struct cloudfs_state *state,
                  const char* fuse_runtime_name) {

  sleep(4);
  int argc = 0;
  char* argv[10];
  argv[argc] = (char *) malloc(128 * sizeof(char));
  strcpy(argv[argc++], fuse_runtime_name);
  argv[argc] = (char *) malloc(1024 * sizeof(char));
  strcpy(argv[argc++], state->fuse_path);
  argv[argc++] = "-s"; // set the fuse mode to single thread
  //argv[argc++] = "-f"; // run fuse in foreground 
  state_  = *state;
  
  retrieve_hash_table_from_ssd();

  int fuse_stat = fuse_main(argc, argv, &cloudfs_operations, NULL);
    
  return fuse_stat;
}