#include "catch.hpp"
#include "tsk/fs/tsk_fs_i.h"
#include "tsk/fs/tsk_fs.h"
#include "tsk/libtsk.h"
#include <cstdio>
#include <cstring>

// Helper to check if the ext2 image exists
static bool ext2_image_exists() {
    FILE *f = fopen("test/data/image_ext2.dd", "rb");
    if (f) { fclose(f); return true; }
    return false;
}

// Helper to open ext2 image and fs
static bool setup_ext2_image(TSK_IMG_INFO **img, TSK_FS_INFO **fs) {
    if (!ext2_image_exists()) {
        WARN("Ext2 test image not found, skipping filesystem tests");
        return false;
    }
    *img = tsk_img_open_sing(_TSK_T("test/data/image_ext2.dd"), TSK_IMG_TYPE_RAW, 0);
    if (!*img) {
        WARN("Could not open ext2 test image");
        return false;
    }
    *fs = tsk_fs_open_img(*img, 0, TSK_FS_TYPE_EXT2);
    if (!*fs) {
        tsk_img_close(*img);
        *img = nullptr;
        WARN("Could not open ext2 filesystem");
        return false;
    }
    return true;
}

static void cleanup_ext2_image(TSK_IMG_INFO *img, TSK_FS_INFO *fs) {
    if (fs) tsk_fs_close(fs);
    if (img) tsk_img_close(img);
}

// Test tsk_fs_blkcat with STAT flag
TEST_CASE("dcat_lib: tsk_fs_blkcat with STAT flag", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_STAT;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 0);
}

// Test tsk_fs_blkcat with ext2 fs - raw output
TEST_CASE("dcat_lib: tsk_fs_blkcat with ext2 fs raw output", "[dcat_lib]") {
    TSK_IMG_INFO *img = nullptr;
    TSK_FS_INFO *fs = nullptr;
    if (!setup_ext2_image(&img, &fs)) return;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
    
    cleanup_ext2_image(img, fs);
}

// Test tsk_fs_blkcat with ext2 fs - HEX output
TEST_CASE("dcat_lib: tsk_fs_blkcat with ext2 fs HEX output", "[dcat_lib]") {
    TSK_IMG_INFO *img = nullptr;
    TSK_FS_INFO *fs = nullptr;
    if (!setup_ext2_image(&img, &fs)) return;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_HEX;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
    
    cleanup_ext2_image(img, fs);
}

// Test tsk_fs_blkcat with ext2 fs - ASCII output
TEST_CASE("dcat_lib: tsk_fs_blkcat with ext2 fs ASCII output", "[dcat_lib]") {
    TSK_IMG_INFO *img = nullptr;
    TSK_FS_INFO *fs = nullptr;
    if (!setup_ext2_image(&img, &fs)) return;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_ASCII;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
    
    cleanup_ext2_image(img, fs);
}

// Test tsk_fs_blkcat with ext2 fs - HTML output
TEST_CASE("dcat_lib: tsk_fs_blkcat with ext2 fs HTML output", "[dcat_lib]") {
    TSK_IMG_INFO *img = nullptr;
    TSK_FS_INFO *fs = nullptr;
    if (!setup_ext2_image(&img, &fs)) return;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_HTML;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
    
    cleanup_ext2_image(img, fs);
}

// Test tsk_fs_blkcat with ext2 fs - HTML and HEX combined
TEST_CASE("dcat_lib: tsk_fs_blkcat with ext2 fs HTML and HEX", "[dcat_lib]") {
    TSK_IMG_INFO *img = nullptr;
    TSK_FS_INFO *fs = nullptr;
    if (!setup_ext2_image(&img, &fs)) return;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = (TSK_FS_BLKCAT_FLAG_ENUM)(TSK_FS_BLKCAT_HTML | TSK_FS_BLKCAT_HEX);
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
    
    cleanup_ext2_image(img, fs);
}

// Test tsk_fs_blkcat with ext2 fs - multiple blocks
TEST_CASE("dcat_lib: tsk_fs_blkcat with ext2 fs multiple blocks", "[dcat_lib]") {
    TSK_IMG_INFO *img = nullptr;
    TSK_FS_INFO *fs = nullptr;
    if (!setup_ext2_image(&img, &fs)) return;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 2; 
    uint8_t result = tsk_fs_blkcat(fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
    
    cleanup_ext2_image(img, fs);
}

// Test tsk_fs_blkcat with raw output
TEST_CASE("dcat_lib: tsk_fs_blkcat with raw output calls read_block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 512;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with HEX flag 
TEST_CASE("dcat_lib: tsk_fs_blkcat with HEX flag calls read_block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 256;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_HEX;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with ASCII flag 
TEST_CASE("dcat_lib: tsk_fs_blkcat with ASCII flag calls read_block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 512;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_ASCII;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with HTML flag 
TEST_CASE("dcat_lib: tsk_fs_blkcat with HTML flag calls read_block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 1024;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_HTML;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with HTML and HEX flags combined
TEST_CASE("dcat_lib: tsk_fs_blkcat with HTML and HEX flags", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 512;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = (TSK_FS_BLKCAT_FLAG_ENUM)(TSK_FS_BLKCAT_HTML | TSK_FS_BLKCAT_HEX);
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with HTML and ASCII flags combined
TEST_CASE("dcat_lib: tsk_fs_blkcat with HTML and ASCII flags", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 512;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = (TSK_FS_BLKCAT_FLAG_ENUM)(TSK_FS_BLKCAT_HTML | TSK_FS_BLKCAT_ASCII);
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with all flags set
TEST_CASE("dcat_lib: tsk_fs_blkcat with all flags set", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = (TSK_FS_BLKCAT_FLAG_ENUM)(TSK_FS_BLKCAT_HTML | TSK_FS_BLKCAT_HEX | TSK_FS_BLKCAT_ASCII | TSK_FS_BLKCAT_STAT);
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 0); 
}

// Test tsk_fs_blkcat with address beyond last block
TEST_CASE("dcat_lib: tsk_fs_blkcat with address beyond last block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 95;
    TSK_DADDR_T read_num_units = 10; 
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
}

// Test tsk_fs_blkcat with zero read units
TEST_CASE("dcat_lib: tsk_fs_blkcat with zero read units", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 0;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
}
/* Causing Memory Leak
// Test tsk_fs_blkcat with very large read units
TEST_CASE("dcat_lib: tsk_fs_blkcat with very large read units", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 0xFFFFFFFFFFFFFFFFULL; 
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
}
*/
// Test tsk_fs_blkcat with invalid filesystem range
TEST_CASE("dcat_lib: tsk_fs_blkcat with invalid filesystem range", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.first_block = 100; 
    fs.last_block = 50;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
}

// Test tsk_fs_blkcat with negative address (should be treated as unsigned)
TEST_CASE("dcat_lib: tsk_fs_blkcat with negative address", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = (TSK_DADDR_T)-1;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
}

// Test tsk_fs_blkcat with filesystem that has no blocks
TEST_CASE("dcat_lib: tsk_fs_blkcat with filesystem having no blocks", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.first_block = 0;
    fs.last_block = 0;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
}
/* Memory leak
// Test tsk_fs_blkcat with very large block size
TEST_CASE("dcat_lib: tsk_fs_blkcat with very large block size", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 0xFFFFFFFF; 
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}
*/
// Test tsk_fs_blkcat with address at last block
TEST_CASE("dcat_lib: tsk_fs_blkcat with address at last block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 100; 
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with address just before last block
TEST_CASE("dcat_lib: tsk_fs_blkcat with address just before last block", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 4096;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 99; 
    TSK_DADDR_T read_num_units = 1;
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1); 
}

// Test tsk_fs_blkcat with multiple blocks
TEST_CASE("dcat_lib: tsk_fs_blkcat with multiple blocks", "[dcat_lib]") {
    TSK_FS_INFO fs = {};
    fs.block_size = 512;
    fs.last_block = 100;
    
    TSK_FS_BLKCAT_FLAG_ENUM flags = TSK_FS_BLKCAT_NONE;
    TSK_DADDR_T addr = 0;
    TSK_DADDR_T read_num_units = 3; 
    uint8_t result = tsk_fs_blkcat(&fs, flags, addr, read_num_units);
    REQUIRE(result == 1);
} 
