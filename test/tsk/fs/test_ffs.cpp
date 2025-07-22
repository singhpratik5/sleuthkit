#include "catch.hpp"
#include "tsk/fs/tsk_fs_i.h"
#include "tsk/fs/tsk_ffs.h"
#include "tsk/fs/tsk_fs.h"
#include <string>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>

#define SLEUTHKIT_TEST_DATA_DIR "SLEUTHKIT_TEST_DATA_DIR"

// Setting test data directory
std::string get_test_data_dir() {
    const char* env = std::getenv(SLEUTHKIT_TEST_DATA_DIR);
    if (!env) {
        std::cerr << SLEUTHKIT_TEST_DATA_DIR << " not set.\n";
        exit(77);  // SKIP exit code 
    }
    return std::string(env);
}

// Test ffs_open with valid UFS image
TEST_CASE("ffs_open_valid_ufs", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    SECTION("open with FFS1 type") {
        TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
        if (fs) {
            REQUIRE(fs->ftype == TSK_FS_TYPE_FFS1);
            REQUIRE(fs->block_count > 0);
            REQUIRE(fs->block_size > 0);
            REQUIRE(fs->root_inum == FFS_ROOTINO);
            REQUIRE(fs->first_inum == FFS_FIRSTINO);
            fs->close(fs);
        }
    }
    
    SECTION("open with FFS2 type") {
        TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS2, nullptr, 0);
        if (fs) {
            REQUIRE(fs->ftype == TSK_FS_TYPE_FFS2);
            REQUIRE(fs->block_count > 0);
            REQUIRE(fs->block_size > 0);
            fs->close(fs);
        }
    }
    
    tsk_img_close(img);
}

// Test ffs_open error conditions
TEST_CASE("ffs_open_error_conditions", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    SECTION("invalid fs type") {
        TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_NTFS, nullptr, 0);
        REQUIRE(fs == nullptr);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_FS_ARG);
        REQUIRE(std::string(tsk_error_get_errstr()).find("Invalid FS Type") != std::string::npos);
    }
    
    SECTION("zero sector size") {
        img->sector_size = 0;
        TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
        REQUIRE(fs == nullptr);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_FS_ARG);
        REQUIRE(std::string(tsk_error_get_errstr()).find("sector size is 0") != std::string::npos);
    }
    
    tsk_img_close(img);
}

// Test ffs_inode_walk functionality
TEST_CASE("ffs_inode_walk_basic", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    int inode_count = 0;
    auto callback = [](TSK_FS_FILE* fs_file, void* ptr) -> TSK_WALK_RET_ENUM {
        (void)fs_file;
        int* count = static_cast<int*>(ptr);
        (*count)++;
        return TSK_WALK_CONT;
    };
    
    SECTION("walk all inodes") {
        uint8_t result = ffs_inode_walk(fs, fs->first_inum, fs->last_inum, 
                                       (TSK_FS_META_FLAG_ENUM)0, callback, &inode_count);
        REQUIRE(result == 0);
        REQUIRE(inode_count > 0);
    }
    
    SECTION("walk with allocated flag") {
        uint8_t result = ffs_inode_walk(fs, fs->first_inum, fs->last_inum, 
                                       TSK_FS_META_FLAG_ALLOC, callback, &inode_count);
        REQUIRE(result == 0);
    }
    
    SECTION("walk with unallocated flag") {
        uint8_t result = ffs_inode_walk(fs, fs->first_inum, fs->last_inum, 
                                       TSK_FS_META_FLAG_UNALLOC, callback, &inode_count);
        REQUIRE(result == 0);
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test ffs_block_walk functionality
TEST_CASE("ffs_block_walk_basic", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    int block_count = 0;
    auto callback = [](const TSK_FS_BLOCK* fs_block, void* ptr) -> TSK_WALK_RET_ENUM {
        (void)fs_block;
        int* count = static_cast<int*>(ptr);
        (*count)++;
        return TSK_WALK_CONT;
    };
    
    SECTION("walk first few blocks") {
        TSK_DADDR_T start = fs->first_block;
        TSK_DADDR_T end = std::min(start + 10, fs->last_block);
        uint8_t result = ffs_block_walk(fs, start, end, TSK_FS_BLOCK_WALK_FLAG_NONE, 
                                       callback, &block_count);
        REQUIRE(result == 0);
        REQUIRE(block_count > 0);
    }
    
    SECTION("walk with allocated flag") {
        uint8_t result = ffs_block_walk(fs, fs->first_block, fs->last_block, 
                                       TSK_FS_BLOCK_WALK_FLAG_ALLOC, callback, &block_count);
        REQUIRE(result == 0);
    }
    
    SECTION("walk with unallocated flag") {
        uint8_t result = ffs_block_walk(fs, fs->first_block, fs->last_block, 
                                       TSK_FS_BLOCK_WALK_FLAG_UNALLOC, callback, &block_count);
        REQUIRE(result == 0);
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test ffs_block_getflags for various blocks
TEST_CASE("ffs_block_getflags_comprehensive", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("first block") {
        TSK_FS_BLOCK_FLAG_ENUM flags = ffs_block_getflags(fs, fs->first_block);
        REQUIRE((flags & (TSK_FS_BLOCK_FLAG_ALLOC | TSK_FS_BLOCK_FLAG_UNALLOC)) != 0);
    }
    
    SECTION("last block") {
        TSK_FS_BLOCK_FLAG_ENUM flags = ffs_block_getflags(fs, fs->last_block);
        REQUIRE((flags & (TSK_FS_BLOCK_FLAG_ALLOC | TSK_FS_BLOCK_FLAG_UNALLOC)) != 0);
    }
    
    SECTION("middle block") {
        TSK_DADDR_T middle = (fs->first_block + fs->last_block) / 2;
        TSK_FS_BLOCK_FLAG_ENUM flags = ffs_block_getflags(fs, middle);
        REQUIRE((flags & (TSK_FS_BLOCK_FLAG_ALLOC | TSK_FS_BLOCK_FLAG_UNALLOC)) != 0);
    }
    
    SECTION("invalid block number") {
        TSK_FS_BLOCK_FLAG_ENUM flags = ffs_block_getflags(fs, fs->last_block + 1000);
        REQUIRE(flags == TSK_FS_BLOCK_FLAG_UNALLOC);
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test ffs_fsstat functionality
TEST_CASE("ffs_fsstat_basic", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("fsstat to null file") {
        uint8_t result = fs->fsstat(fs, nullptr);
        REQUIRE(result == 0);
    }
    
    SECTION("fsstat to temporary file") {
        FILE* temp_file = tmpfile();
        if (temp_file) {
            uint8_t result = fs->fsstat(fs, temp_file);
            REQUIRE(result == 0);
            fclose(temp_file);
        }
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test ffs_istat functionality
TEST_CASE("ffs_istat_basic", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("istat root inode") {
        uint8_t result = fs->istat(fs, TSK_FS_ISTAT_NONE, nullptr, fs->root_inum, 0, 0);
        REQUIRE(result == 0);
    }
    
    SECTION("istat with runlist flag") {
        uint8_t result = fs->istat(fs, TSK_FS_ISTAT_RUNLIST, nullptr, fs->root_inum, 0, 0);
        REQUIRE(result == 0);
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test ffs_fscheck functionality
TEST_CASE("ffs_fscheck_basic", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("fscheck to null file") {
        uint8_t result = fs->fscheck(fs, nullptr);
        REQUIRE(result == 0);
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test inode lookup functionality
TEST_CASE("ffs_inode_lookup_basic", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("lookup root inode") {
        TSK_FS_FILE* fs_file = tsk_fs_file_open_meta(fs, nullptr, fs->root_inum);
        if (fs_file) {
            REQUIRE(fs_file->meta != nullptr);
            REQUIRE(fs_file->meta->type == TSK_FS_META_TYPE_DIR);
            tsk_fs_file_close(fs_file);
        }
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test directory operations
TEST_CASE("ffs_directory_operations", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("open root directory") {
        TSK_FS_DIR* fs_dir = tsk_fs_dir_open_meta(fs, fs->root_inum);
        if (fs_dir) {
            REQUIRE(fs_dir->fs_file != nullptr);
            REQUIRE(fs_dir->fs_file->meta != nullptr);
            REQUIRE(fs_dir->fs_file->meta->type == TSK_FS_META_TYPE_DIR);
            tsk_fs_dir_close(fs_dir);
        }
    }
    
    fs->close(fs);
    tsk_img_close(img);
}

// Test journal functions (should return error for UFS)
TEST_CASE("ffs_journal_functions", "[ffs]") {
    std::string test_data_dir = get_test_data_dir();
    std::string ufs_image = test_data_dir + "/ufs/image.E01";
    
    TSK_IMG_INFO* img = tsk_img_open_utf8_sing(ufs_image.c_str(), TSK_IMG_TYPE_DETECT, 0);
    if (!img) {
        WARN("Could not open UFS image: " << ufs_image << ". Skipping test.");
        return;
    }
    
    TSK_FS_INFO* fs = ffs_open(img, 0, TSK_FS_TYPE_FFS1, nullptr, 0);
    if (!fs) {
        tsk_img_close(img);
        WARN("Could not open FFS filesystem. Skipping test.");
        return;
    }
    
    SECTION("jopen should fail for UFS") {
        uint8_t result = ffs_jopen(fs, 0);
        REQUIRE(result == 1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_FS_UNSUPFUNC);
    }
    
    SECTION("jentry_walk should fail for UFS") {
        uint8_t result = ffs_jentry_walk(fs, 0, nullptr, nullptr);
        REQUIRE(result == 1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_FS_UNSUPFUNC);
    }
    
    SECTION("jblk_walk should fail for UFS") {
        uint8_t result = ffs_jblk_walk(fs, 0, 1, 0, nullptr, nullptr);
        REQUIRE(result == 1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_FS_UNSUPFUNC);
    }
    
    fs->close(fs);
    tsk_img_close(img);
}
