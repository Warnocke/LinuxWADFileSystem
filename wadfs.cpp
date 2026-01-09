#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <vector>
#include "../libWad/Wad.h"

using namespace std;

static Wad* wad;
int wad_getattr(const char *path, struct stat *stbuf);
int wad_mknod(const char *path, mode_t mode, dev_t rdev);
int wad_mkdir(const char *path, mode_t mode);
int wad_do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int wad_do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int wad_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int main(int argc, char** argv){
    if(argc < 3){
        cout << "\nInvalid use!\n";
        return -1;
    }

    int ind = -1;
    for(int i = 1; i < argc - 1; i++){
        if(argv[i][0] != '-'){
            ind = i;
            break;
        }
    }

    if(ind == -1){
        cerr << "Could not find WAD file.\n";
        return -1;
    }

    wad = Wad::loadWad(argv[ind]);

    static struct fuse_operations operations = {};
    operations.getattr  = wad_getattr;
    operations.mknod    = wad_mknod;
    operations.mkdir    = wad_mkdir;
    operations.read     = wad_do_read;
    operations.write    = wad_do_write;
    operations.readdir  = wad_readdir;

    for(int i = ind; i < argc - 1; i++){
        argv[i] = argv[i + 1];
    }
    argc--;

    return fuse_main(argc, argv, &operations, NULL);
}


// Get file attributes, very much inspired by source 3 (0777 permissions for read write execute)
int wad_getattr(const char *path, struct stat *stbuf){
    memset(stbuf, 0, sizeof(struct stat));

    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_atime = time(NULL);
    stbuf->st_mtime = time(NULL);

    if(strcmp(path, "/") == 0){
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        return 0;
    } else if(wad->isDirectory(string(path))){
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        return 0;
    } else if (wad->isContent(string(path))){
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = wad->getSize(string(path));
        return 0;
    } else {
        return -ENOENT;
    }
}

// Create a file node, uses createFile method from Wad class
int wad_mknod(const char *path, mode_t mode, dev_t rdev){
    wad->createFile(string(path));
    if(wad->isContent(string(path))){return 0;}
    return -ENOENT;
}
// Create a directory, uses createDirectory method from Wad class
int wad_mkdir(const char *path, mode_t mode){
    wad->createDirectory(string(path));
    if(wad->isDirectory(string(path))){return 0;}
    return -ENOENT;
}
// Read data from a file, uses getContents method from Wad class
int wad_do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    int ret = wad->getContents(string(path), buf, size, offset);

    if (ret == -1) {
        if (wad->isDirectory(string(path))) 
            return -EISDIR;
        return -ENOENT;
    }

    return ret;
}
// Write data to a file, uses writeToFile method from Wad class
int wad_do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    int ret = wad->writeToFile(string(path), buf, size, offset);

    if (ret == -1) {
        if (wad->isDirectory(string(path)))
            return -EISDIR;
        return -ENOENT;
    }

    return ret;
}

// Directory reading function, uses getDirectory method from Wad class. FUSE requires . (dir) .. (parent dir) to be added manually
int wad_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    if(strcmp(path,"/") == 0){
        vector<string> children;
        wad->getDirectory("/", &children);
        for(const string& child : children){
            filler(buf, child.c_str(), NULL, 0);
        }
        return 0;
    } else if(wad->isDirectory(string(path))){
        vector<string> children;
        wad->getDirectory(string(path), &children);
        for(const string& child : children){
            filler(buf, child.c_str(), NULL, 0);
        }
        return 0;
    } else {
        return -ENOENT;
    }
}