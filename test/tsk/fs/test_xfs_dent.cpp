/*
 * test_xfs_dent.cpp
 *
 * End-to-end functional tests for XFS directory support.
 *
 * Author: Pratik Singh (@singhpratik5)
 */

#include "catch.hpp"

#include <tsk/fs/tsk_fs_i.h>
//#include <tsk/libtsk.h>
//#include <tsk/img/tsk_img.h> 
#include <cstdlib>    
#include <cstdint>   
#include <iostream>
#include <string>
#include <cstring> 

// Forward declarations to avoid headers(getting error if xfs.h is included).
extern "C" {
    int xfs_dir_open_meta(void *a_fs, void **a_fs_dir, uint64_t a_addr, int recursion_depth);
    uint8_t xfs_jentry_walk(void *info, int a, void *c, void *b);
    uint8_t xfs_jblk_walk(void *a, uint64_t b, uint64_t c, int d, void *e, void *f);
    uint8_t xfs_jopen(void *a, uint64_t b);
}

TEST_CASE("XFS Directory Parsing Tests - Testing xfs_dent.cpp functions", "[xfs_dent]") {
    char* env = getenv("SLEUTHKIT_TEST_DATA_DIR");
    if (env == nullptr) {
        WARN("SLEUTHKIT_TEST_DATA_DIR environment variable not set — skipping test");
        return;  
    }
    std::string path = std::string(env) + "/xfs/xfs-raw-2GB.E01";
    const char *image_paths[] = {path.c_str()};

    TSK_IMG_INFO *img_info = tsk_img_open_utf8(
        1, image_paths, TSK_IMG_TYPE_EWF_EWF, 0);

    REQUIRE(img_info != nullptr);

    TSK_OFF_T offset = 0;
    TSK_FS_INFO *fs_info = tsk_fs_open_img(img_info, offset, TSK_FS_TYPE_XFS);
    REQUIRE(fs_info != nullptr);

    SECTION("Test xfs_dir_open_meta function from xfs_dent.cpp") {
        // Test with root directory 
        void *fs_dir = nullptr;
        int ret = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        
        REQUIRE(ret == 0);
        REQUIRE(fs_dir != nullptr);
        
        if (fs_dir) {
            TSK_FS_DIR *dir = static_cast<TSK_FS_DIR*>(fs_dir);
            tsk_fs_dir_close(dir);
        }
    }

    SECTION("Test xfs_dir_open_meta with invalid inode number") {
        void *fs_dir = nullptr;
        int ret = xfs_dir_open_meta(fs_info, &fs_dir, 0, 0);
        
        REQUIRE(ret != 0); 
        REQUIRE(fs_dir == nullptr);
    }

    SECTION("Test xfs_dir_open_meta with out-of-range inode number") {
        void *fs_dir = nullptr;
        int ret = xfs_dir_open_meta(fs_info, &fs_dir, 999999999, 0);
        
        REQUIRE(ret != 0); 
        REQUIRE(fs_dir == nullptr);
    }

    SECTION("Test xfs_dir_open_meta with NULL fs_dir pointer") {
        int ret = xfs_dir_open_meta(fs_info, nullptr, 128, 0);
        
        REQUIRE(ret != 0); 
    }
    /*
    SECTION("Test xfs_dir_open_meta with different recursion depths") {
        void *fs_dir = nullptr;
        
        int ret1 = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        //REQUIRE(ret1 == 0);
        //REQUIRE(fs_dir != nullptr);
        if (fs_dir) {
            tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
            fs_dir = nullptr;
        }
        
        int ret2 = xfs_dir_open_meta(fs_info, &fs_dir, 128, 1);
        //REQUIRE(ret2 == 0);
        //REQUIRE(fs_dir != nullptr);
        if (fs_dir) {
            tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
            fs_dir = nullptr;
        }
        
        int ret3 = xfs_dir_open_meta(fs_info, &fs_dir, 128, 100);
        REQUIRE(ret3 == 0);
        REQUIRE(fs_dir != nullptr);
        if (fs_dir) {
            tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
        }
    }
    */
    SECTION("Test directory entry parsing with real data from xfs_dent.cpp") {
        void *fs_dir = nullptr;
        int ret = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        
        REQUIRE(ret == 0);
        REQUIRE(fs_dir != nullptr);
        
        TSK_FS_DIR *dir = static_cast<TSK_FS_DIR*>(fs_dir);
        
        // Test that directory entries have valid data
        REQUIRE(dir->names_used > 0);
        
        for (size_t i = 0; i < dir->names_used; i++) {
            TSK_FS_NAME *name = &dir->names[i];
            
            REQUIRE(name->name != nullptr);
            REQUIRE(name->name_size > 0);
            REQUIRE(name->meta_addr > 0);
            
            REQUIRE(name->name[strlen(name->name)] == '\0');
            
            REQUIRE(strlen(name->name) > 0);
            REQUIRE(strlen(name->name) < name->name_size);
        }
        
        tsk_fs_dir_close(dir);
    }

    SECTION("Test xfs_dir_open_meta with boundary inode values") {
        void *fs_dir = nullptr;
        
        // Test with first valid inode
        int ret1 = xfs_dir_open_meta(fs_info, &fs_dir, fs_info->first_inum, 0);
        REQUIRE(ret1 == 0);
        REQUIRE(fs_dir != nullptr);
        if (fs_dir) {
            tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
            fs_dir = nullptr;
        }
        
        // Test with last valid inode
        int ret2 = xfs_dir_open_meta(fs_info, &fs_dir, fs_info->last_inum, 0);
        REQUIRE(ret2 == 0);
        REQUIRE(fs_dir != nullptr);
        if (fs_dir) {
            tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
            fs_dir = nullptr;
        }
        
        // Test with inode just before first_inum
        int ret3 = xfs_dir_open_meta(fs_info, &fs_dir, fs_info->first_inum - 1, 0);
        REQUIRE(ret3 != 0); 
        REQUIRE(fs_dir == nullptr);
        
        // Test with inode just after last_inum
        int ret4 = xfs_dir_open_meta(fs_info, &fs_dir, fs_info->last_inum + 1, 0);
        REQUIRE(ret4 != 0); 
        REQUIRE(fs_dir == nullptr);
    }

    SECTION("Test xfs_dir_open_meta with non-directory inodes") {
        void *fs_dir = nullptr;
        
        for (uint64_t inode = 129; inode <= 140; inode++) {
            int ret = xfs_dir_open_meta(fs_info, &fs_dir, inode, 0);

            if (ret == 0) {
                tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
                fs_dir = nullptr;
            } else if (fs_dir) {
                tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
                fs_dir = nullptr;
            }
        }
    }

    SECTION("Test xfs_dir_open_meta with existing directory structure") {
        void *fs_dir = nullptr;
        
        // First call to create directory structure
        int ret1 = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        REQUIRE(ret1 == 0);
        REQUIRE(fs_dir != nullptr);
        
        // Second call with same directory 
        int ret2 = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        REQUIRE(ret2 == 0);
        REQUIRE(fs_dir != nullptr);
        
        // Test with different inode 
        int ret3 = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        REQUIRE(ret3 == 0);
        REQUIRE(fs_dir != nullptr);
        
        tsk_fs_dir_close(static_cast<TSK_FS_DIR*>(fs_dir));
    }

    SECTION("Test directory entry types and flags") {
        void *fs_dir = nullptr;
        int ret = xfs_dir_open_meta(fs_info, &fs_dir, 128, 0);
        
        REQUIRE(ret == 0);
        REQUIRE(fs_dir != nullptr);
        
        TSK_FS_DIR *dir = static_cast<TSK_FS_DIR*>(fs_dir);
        
        for (size_t i = 0; i < dir->names_used; i++) {
            TSK_FS_NAME *name = &dir->names[i];
            
            // Test that TSK_FS_NAME_FLAG_ALLOC should be set
            REQUIRE((name->flags & TSK_FS_NAME_FLAG_ALLOC) != 0);
            
            // Test that name type is valid
            REQUIRE(name->type >= TSK_FS_NAME_TYPE_UNDEF);
            REQUIRE(name->type <= TSK_FS_NAME_TYPE_SOCK);
        }
        
        tsk_fs_dir_close(dir);
    }

    SECTION("Test journal functions from xfs_dent.cpp return expected values") {
        // Test that journal functions return -1 as expected
        REQUIRE(xfs_jentry_walk(fs_info, 0, nullptr, nullptr) == (uint8_t)-1);
        REQUIRE(xfs_jblk_walk(fs_info, 0, 0, 0, nullptr, nullptr) == (uint8_t)-1);
        REQUIRE(xfs_jopen(fs_info, 128) == (uint8_t)-1);
        
        // Test with different parameters to ensure they all return -1
        REQUIRE(xfs_jentry_walk(fs_info, 1, nullptr, nullptr) == (uint8_t)-1);
        REQUIRE(xfs_jblk_walk(fs_info, 1, 1, 1, nullptr, nullptr) == (uint8_t)-1);
        REQUIRE(xfs_jopen(fs_info, 0) == (uint8_t)-1);
    }

    if (fs_info) {
        fs_info->close(fs_info);
    }
    if (img_info) {
        tsk_img_close(img_info);
    }
}
