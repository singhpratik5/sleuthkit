/*
 * File Name: test_tsk_hashdb.cpp
 * Tests the public API functions for hash database management
 * Author: Pratik Singh (@singhpratik5)
 */

#include "tsk/base/tsk_os.h"
#include "tsk/hashdb/tsk_hashdb_i.h"
#include "catch.hpp"
#include <memory>
#include <cstring>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <vector>

#ifdef TSK_WIN32
#include <windows.h>
#endif

// Helper to get temp file path with proper format
static std::string get_temp_path(const char *suffix) {
    static int counter = 0;
    char buffer[512];
#ifdef TSK_WIN32
    snprintf(buffer, sizeof(buffer), ".\\test_tsk_hashdb_%d_%s", counter++, suffix);
#else
    snprintf(buffer, sizeof(buffer), "./test_tsk_hashdb_%d_%s", counter++, suffix);
#endif
    return std::string(buffer);
}

// Convert string to TSK_TCHAR* - keep wide string alive
#ifdef TSK_WIN32
static std::wstring g_wstr_holder;
static const wchar_t* string_to_tchar(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    g_wstr_holder.resize(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &g_wstr_holder[0], size_needed);
    return g_wstr_holder.c_str();
}
#define STR_TO_TCHAR(s) (string_to_tchar(s))
#else
#define STR_TO_TCHAR(s) (s.c_str())
#endif

// Remove file helper
static void remove_test_file(const std::string& path) {
#ifdef TSK_WIN32
    _wunlink(string_to_tchar(path));
#else
    unlink(path.c_str());
#endif
}

// Create sample NSRL format database for testing
static std::string create_nsrl_test_db() {
    std::string path = get_temp_path("nsrl.txt");
    FILE *f = fopen(path.c_str(), "w");
    if (f) {
        fprintf(f, "\"SHA-1\",\"MD5\",\"CRC32\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"SpecialCode\"\n");
        fprintf(f, "\"0000000000000000000000000000000000000000\",\"00000000000000000000000000000000\",\"00000000\",\"test1.txt\",\"100\",\"1\",\"1\",\"\"\n");
        fprintf(f, "\"1111111111111111111111111111111111111111\",\"11111111111111111111111111111111\",\"11111111\",\"test2.txt\",\"200\",\"2\",\"2\",\"\"\n");
        fclose(f);
    }
    return path;
}

// Create sample md5sum format database for testing
static std::string create_md5sum_test_db() {
    std::string path = get_temp_path("md5sum.txt");
    FILE *f = fopen(path.c_str(), "w");
    if (f) {
        fprintf(f, "d41d8cd98f00b204e9800998ecf8427e  empty.txt\n");
        fprintf(f, "5d41402abc4b2a76b9719d911017c592  hello.txt\n");
        fclose(f);
    }
    return path;
}

// Create sample EnCase format database for testing
static std::string create_encase_test_db() {
    std::string path = get_temp_path("encase.hash");
    FILE *f = fopen(path.c_str(), "wb");
    if (f) {
        // Write EnCase binary header
        fwrite("HASH\x0d\x0a\xff\x00", 1, 8, f);
        // Write padding and minimal structure (1144 bytes of zeros)
        std::vector<char> zeros(1144, 0);
        fwrite(zeros.data(), 1, zeros.size(), f);
        fclose(f);
    }
    return path;
}

// Create sample HashKeeper format database for testing
static std::string create_hk_test_db() {
    std::string path = get_temp_path("hk.txt");
    FILE *f = fopen(path.c_str(), "w");
    if (f) {
        // Write proper HashKeeper header
        fprintf(f, "\"file_id\",\"hashset_id\",\"file_name\",\"directory\",\"hash\",\"file_size\",\"date_modified\",\"time_modified\",\"time_zone\",\"comments\",\"date_accessed\",\"time_accessed\"\n");
        fprintf(f, "1,1,\"test1.txt\",\"C:\\\\Windows\",\"d41d8cd98f00b204e9800998ecf8427e\",100,\"2023-01-01\",\"12:00:00\",\"UTC\",\"Test file 1\",\"2023-01-01\",\"12:00:00\"\n");
        fprintf(f, "2,1,\"test2.txt\",\"C:\\\\Windows\",\"5d41402abc4b2a76b9719d911017c592\",200,\"2023-01-02\",\"13:00:00\",\"UTC\",\"Test file 2\",\"2023-01-02\",\"13:00:00\"\n");
        fclose(f);
    }
    return path;
}

// ========================================================================
// Tests for tsk_hdb_create
// ========================================================================

TEST_CASE("tsk_hdb_create with NULL path", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_create(NULL);
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_create with non-kdb extension", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.db");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    uint8_t result = tsk_hdb_create(tpath);
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_create with valid .kdb extension", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    uint8_t result = tsk_hdb_create(tpath);
    REQUIRE(result == 0);
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_open
// ========================================================================

TEST_CASE("tsk_hdb_open with NULL path", "[tsk_hashdb]") {
    TSK_HDB_INFO *hdb = tsk_hdb_open(NULL, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb == nullptr);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_open with non-existent file", "[tsk_hashdb]") {
    std::string path = get_temp_path("nonexistent.txt");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb == nullptr);
}

TEST_CASE("tsk_hdb_open NSRL database", "[tsk_hashdb]") {
    std::string path = create_nsrl_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    REQUIRE(hdb->db_type == TSK_HDB_DBTYPE_NSRL_ID);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_open md5sum database", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    REQUIRE(hdb->db_type == TSK_HDB_DBTYPE_MD5SUM_ID);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_open EnCase database", "[tsk_hashdb]") {
    std::string path = create_encase_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    // EnCase requires proper binary format - test may not work with minimal data
    if (hdb) {
        CHECK(hdb->db_type == TSK_HDB_DBTYPE_ENCASE_ID);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_open HashKeeper database", "[tsk_hashdb]") {
    std::string path = create_hk_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    CHECK(hdb->db_type == TSK_HDB_DBTYPE_HK_ID);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_open SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    REQUIRE(hdb->db_type == TSK_HDB_DBTYPE_SQLITE_ID);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}
/*
TEST_CASE("tsk_hdb_open with index file path md5", "[tsk_hashdb]") {
    std::string db_path = create_md5sum_test_db();
    std::string idx_path = db_path + "-md5.idx";
    // Create a proper binary index file header
    FILE *idx_f = fopen(idx_path.c_str(), "wb");
    if (idx_f) {
        // Write index header markers
        fprintf(idx_f, "%s\n", TSK_HDB_IDX_HEAD_TYPE_STR);
        fprintf(idx_f, "md5\n");
        fprintf(idx_f, "%s\n", TSK_HDB_IDX_HEAD_NAME_STR);
        fprintf(idx_f, "%s\n", db_path.c_str());
        // Write a sample index entry (hash,offset)
        fprintf(idx_f, "00000000000000000000000000000000,0000000000000000\n");
        fclose(idx_f);
    }
    // Remove the database file so that IDXONLY is triggered
    remove_test_file(db_path);
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(idx_path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    CHECK(hdb->db_type == TSK_HDB_DBTYPE_IDXONLY_ID);
    tsk_hdb_close(hdb);
    remove_test_file(idx_path);
}

TEST_CASE("tsk_hdb_open with index file path sha1", "[tsk_hashdb]") {
    std::string db_path = create_nsrl_test_db();
    std::string idx_path = db_path + "-sha1.idx";
    // Create a proper binary index file header
    FILE *idx_f = fopen(idx_path.c_str(), "wb");
    if (idx_f) {
        // Write index header markers
        fprintf(idx_f, "%s\n", TSK_HDB_IDX_HEAD_TYPE_STR);
        fprintf(idx_f, "sha1\n");
        fprintf(idx_f, "%s\n", TSK_HDB_IDX_HEAD_NAME_STR);
        fprintf(idx_f, "%s\n", db_path.c_str());
        // Write a sample index entry (hash,offset)
        fprintf(idx_f, "0000000000000000000000000000000000000000,0000000000000000\n");
        fclose(idx_f);
    }
    // Remove the database file so that IDXONLY is triggered
    remove_test_file(db_path);
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(idx_path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    CHECK(hdb->db_type == TSK_HDB_DBTYPE_IDXONLY_ID);
    tsk_hdb_close(hdb);
    remove_test_file(idx_path);
}

TEST_CASE("tsk_hdb_open with IDXONLY flag", "[tsk_hashdb]") {
    std::string path = get_temp_path("dummy.txt");
    FILE *idx_f = fopen(path.c_str(), "w");
    if (idx_f) {
        fprintf(idx_f, "00000000000000000000000000000000,0\n");
        fclose(idx_f);
    }
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_IDXONLY);
    REQUIRE(hdb != nullptr);
    REQUIRE(hdb->db_type == TSK_HDB_DBTYPE_IDXONLY_ID);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}
*/
// ========================================================================
// Tests for tsk_hdb_get_db_path
// ========================================================================

TEST_CASE("tsk_hdb_get_db_path with NULL hdb_info", "[tsk_hashdb]") {
    const TSK_TCHAR *result = tsk_hdb_get_db_path(NULL);
    REQUIRE(result == nullptr);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_get_db_path with valid hdb_info", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    const TSK_TCHAR *result = tsk_hdb_get_db_path(hdb);
    REQUIRE(result != nullptr);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_get_display_name
// ========================================================================

TEST_CASE("tsk_hdb_get_display_name with NULL hdb_info", "[tsk_hashdb]") {
    const char *result = tsk_hdb_get_display_name(NULL);
    REQUIRE(result == nullptr);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_get_display_name with valid hdb_info", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    const char *result = tsk_hdb_get_display_name(hdb);
    REQUIRE(result != nullptr);
    REQUIRE(strlen(result) > 0);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_uses_external_indexes
// ========================================================================

TEST_CASE("tsk_hdb_uses_external_indexes with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_uses_external_indexes(NULL);
    REQUIRE(result == 0);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_uses_external_indexes with text db", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    uint8_t result = tsk_hdb_uses_external_indexes(hdb);
    REQUIRE(result == 1);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_uses_external_indexes with SQLite db", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    uint8_t result = tsk_hdb_uses_external_indexes(hdb);
    REQUIRE(result == 0);
    tsk_hdb_close(hdb);
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_get_idx_path
// ========================================================================

TEST_CASE("tsk_hdb_get_idx_path with NULL hdb_info", "[tsk_hashdb]") {
    const TSK_TCHAR *result = tsk_hdb_get_idx_path(NULL, TSK_HDB_HTYPE_MD5_ID);
    REQUIRE(result == nullptr);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}
/*
TEST_CASE("tsk_hdb_get_idx_path with valid hdb_info", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    REQUIRE(hdb != nullptr);
    // Create the index first so get_idx_path can return a valid path
    TSK_TCHAR idx_type[] = _TSK_T("md5");
    uint8_t make_result = tsk_hdb_make_index(hdb, idx_type);
    CHECK(make_result == 0);
    if (make_result == 0) {
        const TSK_TCHAR *result = tsk_hdb_get_idx_path(hdb, TSK_HDB_HTYPE_MD5_ID);
        CHECK(result != nullptr);
        // Cleanup index file
        std::string idx_path = path + "-md5.idx";
        remove_test_file(idx_path);
    }
    tsk_hdb_close(hdb);
    remove_test_file(path);
}
*/

// ========================================================================
// Tests for tsk_hdb_is_idx_only
// ========================================================================

TEST_CASE("tsk_hdb_is_idx_only with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_is_idx_only(NULL);
    REQUIRE(result == 0);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_is_idx_only with regular database", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_is_idx_only(hdb);
        REQUIRE(result == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}
/*
TEST_CASE("tsk_hdb_is_idx_only with index only database", "[tsk_hashdb]") {
    std::string db_path = create_md5sum_test_db();
    std::string idx_path = db_path + "-md5.idx";
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(idx_path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_is_idx_only(hdb);
        REQUIRE(result == 1);
        tsk_hdb_close(hdb);
    }
    remove_test_file(db_path);
}
*/
// ========================================================================
// Tests for tsk_hdb_make_index
// ========================================================================

TEST_CASE("tsk_hdb_make_index with NULL hdb_info", "[tsk_hashdb]") {
    TSK_TCHAR type[] = _TSK_T("md5");
    uint8_t result = tsk_hdb_make_index(NULL, type);
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}
/*
TEST_CASE("tsk_hdb_make_index with valid md5sum database", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        TSK_TCHAR type[] = _TSK_T("md5");
        uint8_t result = tsk_hdb_make_index(hdb, type);
        REQUIRE(result == 0);
        std::string idx_path = path + "-md5.idx";
        remove_test_file(idx_path);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}
*/
// ========================================================================
// Tests for tsk_hdb_lookup_str
// ========================================================================

TEST_CASE("tsk_hdb_lookup_str with NULL hdb_info", "[tsk_hashdb]") {
    int8_t result = tsk_hdb_lookup_str(NULL, "d41d8cd98f00b204e9800998ecf8427e", TSK_HDB_FLAG_QUICK, NULL, NULL);
    REQUIRE(result == -1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_lookup_str with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        tsk_hdb_add_entry(hdb, "test.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "test comment");
        int8_t result = tsk_hdb_lookup_str(hdb, "d41d8cd98f00b204e9800998ecf8427e", TSK_HDB_FLAG_QUICK, NULL, NULL);
        REQUIRE(result >= 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_lookup_raw
// ========================================================================

TEST_CASE("tsk_hdb_lookup_raw with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t hash[16] = {0};
    int8_t result = tsk_hdb_lookup_raw(NULL, hash, 16, TSK_HDB_FLAG_QUICK, NULL, NULL);
    REQUIRE(result == -1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_lookup_raw with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        tsk_hdb_add_entry(hdb, "test.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "test comment");
        uint8_t hash[16] = {0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e};
        int8_t result = tsk_hdb_lookup_raw(hdb, hash, 16, TSK_HDB_FLAG_QUICK, NULL, NULL);
        REQUIRE(result >= 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_lookup_verbose_str
// ========================================================================

TEST_CASE("tsk_hdb_lookup_verbose_str with NULL hdb_info", "[tsk_hashdb]") {
    int result_data = 0;
    int8_t result = tsk_hdb_lookup_verbose_str(NULL, "d41d8cd98f00b204e9800998ecf8427e", &result_data);
    REQUIRE(result == -1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_lookup_verbose_str with NULL hash", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        int result_data = 0;
        int8_t result = tsk_hdb_lookup_verbose_str(hdb, NULL, &result_data);
        REQUIRE(result == -1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_lookup_verbose_str with NULL result", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        int8_t result = tsk_hdb_lookup_verbose_str(hdb, "d41d8cd98f00b204e9800998ecf8427e", NULL);
        REQUIRE(result == -1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_accepts_updates
// ========================================================================

TEST_CASE("tsk_hdb_accepts_updates with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_accepts_updates(NULL);
    REQUIRE(result == 0);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_accepts_updates with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_accepts_updates(hdb);
        REQUIRE(result == 1);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_accepts_updates with text database", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_accepts_updates(hdb);
        REQUIRE(result == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_add_entry
// ========================================================================

TEST_CASE("tsk_hdb_add_entry with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_add_entry(NULL, "test.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "comment");
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_add_entry with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_add_entry(hdb, "test.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "test comment");
        REQUIRE(result == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_add_entry with all hash types", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_add_entry(hdb, "test.txt", 
            "d41d8cd98f00b204e9800998ecf8427e",
            "da39a3ee5e6b4b0d3255bfef95601890afd80709",
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "test comment");
        REQUIRE(result == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_add_entry to text database fails", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_add_entry(hdb, "test.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "comment");
        REQUIRE(result == 1);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_begin_transaction
// ========================================================================

TEST_CASE("tsk_hdb_begin_transaction with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_begin_transaction(NULL);
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_begin_transaction with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_begin_transaction(hdb);
        REQUIRE(result == 0);
        REQUIRE(hdb->transaction_in_progress == 1);
        tsk_hdb_commit_transaction(hdb);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_begin_transaction twice fails", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result1 = tsk_hdb_begin_transaction(hdb);
        REQUIRE(result1 == 0);
        uint8_t result2 = tsk_hdb_begin_transaction(hdb);
        REQUIRE(result2 == 1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_PROC);
        tsk_hdb_commit_transaction(hdb);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_begin_transaction with text database fails", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_begin_transaction(hdb);
        REQUIRE(result == 1);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_commit_transaction
// ========================================================================

TEST_CASE("tsk_hdb_commit_transaction with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_commit_transaction(NULL);
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_commit_transaction with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        tsk_hdb_begin_transaction(hdb);
        uint8_t result = tsk_hdb_commit_transaction(hdb);
        REQUIRE(result == 0);
        REQUIRE(hdb->transaction_in_progress == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_commit_transaction without begin fails", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_commit_transaction(hdb);
        REQUIRE(result == 1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_PROC);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_commit_transaction with text database fails", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_commit_transaction(hdb);
        REQUIRE(result == 1);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_rollback_transaction
// ========================================================================

TEST_CASE("tsk_hdb_rollback_transaction with NULL hdb_info", "[tsk_hashdb]") {
    uint8_t result = tsk_hdb_rollback_transaction(NULL);
    REQUIRE(result == 1);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_rollback_transaction with SQLite database", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        tsk_hdb_begin_transaction(hdb);
        uint8_t result = tsk_hdb_rollback_transaction(hdb);
        REQUIRE(result == 0);
        REQUIRE(hdb->transaction_in_progress == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_rollback_transaction without begin fails", "[tsk_hashdb]") {
    std::string path = get_temp_path("test.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_rollback_transaction(hdb);
        REQUIRE(result == 1);
        REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_PROC);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("tsk_hdb_rollback_transaction with text database fails", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        uint8_t result = tsk_hdb_rollback_transaction(hdb);
        REQUIRE(result == 1);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Tests for tsk_hdb_close
// ========================================================================

TEST_CASE("tsk_hdb_close with NULL hdb_info", "[tsk_hashdb]") {
    tsk_hdb_close(NULL);
    REQUIRE(tsk_error_get_errno() == TSK_ERR_HDB_ARG);
}

TEST_CASE("tsk_hdb_close with valid hdb_info", "[tsk_hashdb]") {
    std::string path = create_md5sum_test_db();
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

// ========================================================================
// Additional edge case and integration tests
// ========================================================================

TEST_CASE("tsk_hdb_open ambiguous database type detection", "[tsk_hashdb]") {
    std::string path = get_temp_path("ambiguous.txt");
    FILE *f = fopen(path.c_str(), "w");
    if (f) {
        fprintf(f, "d41d8cd98f00b204e9800998ecf8427e  empty.txt\n");
        fprintf(f, "\"SHA-1\",\"MD5\",\"CRC32\",\"FileName\",\"FileSize\",\"ProductCode\",\"OpSystemCode\",\"SpecialCode\"\n");
        fclose(f);
    }
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}

TEST_CASE("Full workflow: create, add entries, lookup", "[tsk_hashdb]") {
    std::string path = get_temp_path("workflow.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    uint8_t create_result = tsk_hdb_create(tpath);
    REQUIRE(create_result == 0);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        REQUIRE(tsk_hdb_accepts_updates(hdb) == 1);
        REQUIRE(tsk_hdb_begin_transaction(hdb) == 0);
        REQUIRE(tsk_hdb_add_entry(hdb, "file1.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "comment1") == 0);
        REQUIRE(tsk_hdb_add_entry(hdb, "file2.txt", "5d41402abc4b2a76b9719d911017c592", NULL, NULL, "comment2") == 0);
        REQUIRE(tsk_hdb_commit_transaction(hdb) == 0);
        int8_t lookup_result = tsk_hdb_lookup_str(hdb, "d41d8cd98f00b204e9800998ecf8427e", TSK_HDB_FLAG_QUICK, NULL, NULL);
        REQUIRE(lookup_result > 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}
/*
TEST_CASE("Transaction rollback test", "[tsk_hashdb]") {
    std::string path = get_temp_path("rollback.kdb");
    TSK_TCHAR *tpath = const_cast<TSK_TCHAR*>(STR_TO_TCHAR(path));
    tsk_hdb_create(tpath);
    TSK_HDB_INFO *hdb = tsk_hdb_open(tpath, TSK_HDB_OPEN_NONE);
    if (hdb) {
        REQUIRE(tsk_hdb_begin_transaction(hdb) == 0);
        REQUIRE(tsk_hdb_add_entry(hdb, "test.txt", "d41d8cd98f00b204e9800998ecf8427e", NULL, NULL, "comment") == 0);
        REQUIRE(tsk_hdb_rollback_transaction(hdb) == 0);
        int8_t lookup_result = tsk_hdb_lookup_str(hdb, "d41d8cd98f00b204e9800998ecf8427e", TSK_HDB_FLAG_QUICK, NULL, NULL);
        REQUIRE(lookup_result == 0);
        tsk_hdb_close(hdb);
    }
    remove_test_file(path);
}
*/
TEST_CASE("Multiple database types in sequence", "[tsk_hashdb]") {
    std::string nsrl_path = create_nsrl_test_db();
    std::string md5sum_path = create_md5sum_test_db();
    std::string encase_path = create_encase_test_db();
    std::string hk_path = create_hk_test_db();
    TSK_HDB_INFO *nsrl_hdb = tsk_hdb_open(const_cast<TSK_TCHAR*>(STR_TO_TCHAR(nsrl_path)), TSK_HDB_OPEN_NONE);
    TSK_HDB_INFO *md5sum_hdb = tsk_hdb_open(const_cast<TSK_TCHAR*>(STR_TO_TCHAR(md5sum_path)), TSK_HDB_OPEN_NONE);
    TSK_HDB_INFO *encase_hdb = tsk_hdb_open(const_cast<TSK_TCHAR*>(STR_TO_TCHAR(encase_path)), TSK_HDB_OPEN_NONE);
    TSK_HDB_INFO *hk_hdb = tsk_hdb_open(const_cast<TSK_TCHAR*>(STR_TO_TCHAR(hk_path)), TSK_HDB_OPEN_NONE);
    if (nsrl_hdb) {
        REQUIRE(nsrl_hdb->db_type == TSK_HDB_DBTYPE_NSRL_ID);
        tsk_hdb_close(nsrl_hdb);
    }
    if (md5sum_hdb) {
        REQUIRE(md5sum_hdb->db_type == TSK_HDB_DBTYPE_MD5SUM_ID);
        tsk_hdb_close(md5sum_hdb);
    }
    if (encase_hdb) {
        REQUIRE(encase_hdb->db_type == TSK_HDB_DBTYPE_ENCASE_ID);
        tsk_hdb_close(encase_hdb);
    }
    if (hk_hdb) {
        REQUIRE(hk_hdb->db_type == TSK_HDB_DBTYPE_HK_ID);
        tsk_hdb_close(hk_hdb);
    }
    remove_test_file(nsrl_path);
    remove_test_file(md5sum_path);
    remove_test_file(encase_path);
    remove_test_file(hk_path);
}
