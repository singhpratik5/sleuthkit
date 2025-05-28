#include "catch.hpp"
#include "tsk/fs/tsk_fs_i.h"
#include "tsk/fs/tsk_fs.h"
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>

static std::vector<std::string> printed_lines;
static int sanitized_return = 0;
static int walker_return = 0;
static int ntfs_return = 0;
static bool orphan_file = true;
static bool walker_should_find = true;

#ifndef TSK_FS_FFIND_NONE
#define TSK_FS_FFIND_NONE ((TSK_FS_FFIND_FLAG_ENUM)0)
#endif

#ifdef tsk_printf
#undef tsk_printf
#endif

extern "C" void tsk_printf(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printed_lines.push_back(std::string(buf));
}

extern "C" int tsk_print_sanitized(FILE *, const char *s) {
    printed_lines.push_back(std::string(s ? s : "(null)"));
    return sanitized_return;
}

extern "C" uint8_t tsk_fs_dir_walk(
    TSK_FS_INFO *, TSK_INUM_T, TSK_FS_DIR_WALK_FLAG_ENUM,
    TSK_FS_DIR_WALK_CB action, void *ptr)
{
    if (walker_return) return 1; //error
    if (walker_should_find) {
        // found file
        TSK_FS_FILE file = {};
        TSK_FS_NAME name = {};
        name.meta_addr = 42;
        name.flags = (TSK_FS_NAME_FLAG_ENUM)0;
        name.name = const_cast<char*>("foundfile.txt");
        file.name = &name;
        return action(&file, "/mockpath", ptr) == TSK_WALK_ERROR ? 1 : 0;
    }
    return 0;
}

extern "C" uint8_t ntfs_find_file(
    TSK_FS_INFO *, TSK_INUM_T, uint32_t, uint8_t,
    uint16_t, uint8_t, TSK_FS_DIR_WALK_FLAG_ENUM,
    TSK_FS_DIR_WALK_CB, void *)
{
    return ntfs_return;
}



extern "C" void tsk_fs_file_close(TSK_FS_FILE *) {}

// Test: Walker finds the file
TEST_CASE("File found via walker prints name and returns 0", "[ffind]") {
    printed_lines.clear();
    sanitized_return = 0;
    walker_return = 0;
    walker_should_find = true;
    ntfs_return = 1;
    TSK_FS_INFO fs = {};
    fs.root_inum = 1;
    fs.ftype = TSK_FS_TYPE_FAT12;
    uint8_t result = tsk_fs_ffind(&fs, TSK_FS_FFIND_ALL, 42, TSK_FS_ATTR_TYPE_DEFAULT, 0, 0, 0, TSK_FS_DIR_WALK_FLAG_NONE);
    REQUIRE(result == 0);
    REQUIRE(!printed_lines.empty());
    bool found = false;
    for (const auto& line : printed_lines) {
        if (line.find("foundfile.txt") != std::string::npos) found = true;
    }
    REQUIRE(found);
}

// Test: Root inode with ALLOC flag
TEST_CASE("Root inode with alloc flag prints / and returns 0", "[ffind]") {
    printed_lines.clear();
    sanitized_return = 0;
    walker_return = 0;
    walker_should_find = true;
    ntfs_return = 1;
    TSK_FS_INFO fs = {};
    fs.root_inum = 1;
    fs.ftype = TSK_FS_TYPE_FAT12;
    uint8_t result = tsk_fs_ffind(&fs, TSK_FS_FFIND_NONE, 1, TSK_FS_ATTR_TYPE_DEFAULT, 0, 0, 0, TSK_FS_DIR_WALK_FLAG_ALLOC);
    REQUIRE(result == 0);
    bool found_root = false;
    for (const auto& line : printed_lines) {
        if (line == "/\n" || line == "/") found_root = true;
    }
    REQUIRE(found_root);
}

// Test: Walker returns error, so tsk_fs_ffind should return 1 (error)
TEST_CASE("Walker returns error, tsk_fs_ffind returns 1", "[ffind]") {
    printed_lines.clear();
    sanitized_return = 0;
    walker_return = 1;
    walker_should_find = false;
    ntfs_return = 1;
    orphan_file = false;
    TSK_FS_INFO fs = {};
    fs.root_inum = 1;
    fs.ftype = TSK_FS_TYPE_FAT12;
    uint8_t result = tsk_fs_ffind(&fs, TSK_FS_FFIND_ALL, 42, TSK_FS_ATTR_TYPE_DEFAULT, 0, 0, 0, TSK_FS_DIR_WALK_FLAG_NONE);
    REQUIRE(result == 1);
}

// Test: ntfs_find_file returns error
TEST_CASE("ntfs_find_file returns error, tsk_fs_ffind returns 1", "[ffind]") {
    printed_lines.clear();
    sanitized_return = 0;
    walker_return = 0;
    walker_should_find = true;
    ntfs_return = 1;
    TSK_FS_INFO fs = {};
    fs.root_inum = 1;
    fs.ftype = TSK_FS_TYPE_NTFS;
    uint8_t result = tsk_fs_ffind(&fs, TSK_FS_FFIND_ALL, 42, TSK_FS_ATTR_TYPE_DEFAULT, 0, 0, 0, TSK_FS_DIR_WALK_FLAG_NONE);
    REQUIRE(result == 1);
}

// Test: Walker does not find the file, not FAT
TEST_CASE("File not found, not FAT, prints error", "[ffind]") {
    printed_lines.clear();
    sanitized_return = 0;
    walker_return = 0;
    walker_should_find = false; 
    ntfs_return = 1;
    orphan_file = false;
    TSK_FS_INFO fs = {};
    fs.root_inum = 1;
    fs.ftype = TSK_FS_TYPE_EXT2;
    uint8_t result = tsk_fs_ffind(&fs, TSK_FS_FFIND_NONE, 9999, TSK_FS_ATTR_TYPE_DEFAULT, 0, 0, 0, TSK_FS_DIR_WALK_FLAG_NONE);
    REQUIRE(result == 0);
    bool found_msg = false;
    for (const auto& line : printed_lines) {
        if (line.find("File name not found") != std::string::npos) found_msg = true;
    }
    REQUIRE(found_msg);
}

// Test: tsk_print_sanitized returns error
TEST_CASE("tsk_print_sanitized returns error, tsk_fs_ffind returns 1", "[ffind]") {
    printed_lines.clear();
    sanitized_return = 1;
    walker_return = 0;
    walker_should_find = true;
    ntfs_return = 1;
    TSK_FS_INFO fs = {};
    fs.root_inum = 1;
    fs.ftype = TSK_FS_TYPE_FAT12;
    uint8_t result = tsk_fs_ffind(&fs, TSK_FS_FFIND_ALL, 42, TSK_FS_ATTR_TYPE_DEFAULT, 0, 0, 0, TSK_FS_DIR_WALK_FLAG_NONE);
    REQUIRE(result == 1);
}
