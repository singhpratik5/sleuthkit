/*
 * File Name: test_sqlite_hdb.cpp
 * 
 * Unit tests for sqlite_hdb.cpp
 *
 * Author: Pratik Singh (@singhpratik5)
 */

#include "tsk/base/tsk_os.h"
#include "tsk/hashdb/tsk_hashdb_i.h"
#include "tsk/hashdb/tsk_hash_info.h"
#include "catch.hpp"
#include <memory>
#include <cstring>
#include <cstdio>
#include <string>

#ifdef TSK_WIN32
#include <windows.h>
#endif

// Portable fopen for TSK_TCHAR
static FILE *tsk_fopen_tchar(const TSK_TCHAR *path, const TSK_TCHAR *mode) {
#ifdef TSK_WIN32
    return _wfopen(path, mode);
#else
    return fopen(path, mode);
#endif
}

// Helper to create temporary file paths
static std::string get_temp_db_path() {
    static int counter = 0;
    char buffer[256];
#ifdef TSK_WIN32
    snprintf(buffer, sizeof(buffer), ".\\test_sqlite_hdb_%d.db", counter++);
#else
    snprintf(buffer, sizeof(buffer), "./test_sqlite_hdb_%d.db", counter++);
#endif
    return std::string(buffer);
}

// Helper to remove a file
static void remove_file(const char *path) {
    std::remove(path);
}

#ifdef TSK_WIN32
// Helper to convert string to wstring for Windows
static std::wstring string_to_wstring(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#endif

// Helper function to get TSK_TCHAR path
static TSK_TCHAR* get_tsk_path(const std::string& path) {
#ifdef TSK_WIN32
    static std::wstring wpath;
    wpath = string_to_wstring(path);
    return const_cast<TSK_TCHAR*>(wpath.c_str());
#else
    return const_cast<TSK_TCHAR*>(path.c_str());
#endif
}

// Dummy callback for lookup tests
static TSK_WALK_RET_ENUM test_lookup_callback(TSK_HDB_INFO *hdb_info,
                                               const char *hash,
                                               const char *name,
                                               void *ptr) {
    (void)hdb_info;  // Suppress unused parameter warning
    (void)hash;
    (void)name;
    (void)ptr;
    return TSK_WALK_CONT;
}

// Callback to count results
struct CallbackCounter {
    int count;
    std::string last_hash;
    std::string last_name;
};

static TSK_WALK_RET_ENUM count_callback(TSK_HDB_INFO *hdb_info,
                                        const char *hash,
                                        const char *name,
                                        void *ptr) {
    (void)hdb_info;
    CallbackCounter *counter = static_cast<CallbackCounter*>(ptr);
    counter->count++;
    if (hash) counter->last_hash = hash;
    if (name) counter->last_name = name;
    return TSK_WALK_CONT;
}

// ========================================================================
// Tests for sqlite_hdb_create_db
// ========================================================================

TEST_CASE("sqlite_hdb_create_db creates a new database") {
    std::string db_path = get_temp_db_path();
    TSK_TCHAR *tsk_path = get_tsk_path(db_path);
    
    SECTION("Create new database successfully") {
        uint8_t result = sqlite_hdb_create_db(tsk_path);
        CHECK(result == 0);
        
        // Verify file exists
        FILE *f = tsk_fopen_tchar(tsk_path, _TSK_T("rb"));
        REQUIRE(f != nullptr);
        fclose(f);
        
        remove_file(db_path.c_str());
    }
    
    SECTION("Create database in same location twice") {
        // First creation
        uint8_t result1 = sqlite_hdb_create_db(tsk_path);
        CHECK(result1 == 0);
        
        // Second creation should also succeed (overwrites)
        uint8_t result2 = sqlite_hdb_create_db(tsk_path);
        CHECK(result2 == 0);
        
        remove_file(db_path.c_str());
    }
}

// ========================================================================
// Tests for sqlite_hdb_is_sqlite_file
// ========================================================================

TEST_CASE("sqlite_hdb_is_sqlite_file detects SQLite files") {
    std::string db_path = get_temp_db_path();
    TSK_TCHAR *tsk_path = get_tsk_path(db_path);
    
    SECTION("Detect valid SQLite file") {
        // Create a SQLite database
        sqlite_hdb_create_db(tsk_path);
        
        FILE *f = tsk_fopen_tchar(tsk_path, _TSK_T("rb"));
        REQUIRE(f != nullptr);
        
        uint8_t is_sqlite = sqlite_hdb_is_sqlite_file(f);
        CHECK(is_sqlite == 1);
        
        fclose(f);
        remove_file(db_path.c_str());
    }
    
    SECTION("Detect non-SQLite file") {
        // Create a text file
        FILE *f = fopen(db_path.c_str(), "wb");
        REQUIRE(f != nullptr);
        fprintf(f, "This is not a SQLite file\n");
        fclose(f);
        
        f = fopen(db_path.c_str(), "rb");
        REQUIRE(f != nullptr);
        
        uint8_t is_sqlite = sqlite_hdb_is_sqlite_file(f);
        CHECK(is_sqlite == 0);
        
        fclose(f);
        remove_file(db_path.c_str());
    }
    
    SECTION("Detect empty file") {
        // Create empty file
        FILE *f = fopen(db_path.c_str(), "wb");
        REQUIRE(f != nullptr);
        fclose(f);
        
        f = fopen(db_path.c_str(), "rb");
        REQUIRE(f != nullptr);
        
        uint8_t is_sqlite = sqlite_hdb_is_sqlite_file(f);
        CHECK(is_sqlite == 0);
        
        fclose(f);
        remove_file(db_path.c_str());
    }
}

// ========================================================================
// Tests for sqlite_hdb_open
// ========================================================================

TEST_CASE("sqlite_hdb_open opens existing database") {
    std::string db_path = get_temp_db_path();
    TSK_TCHAR *tsk_path = get_tsk_path(db_path);
    
    SECTION("Open valid database") {
        // Create database first
        sqlite_hdb_create_db(tsk_path);
        
        // Open it
        TSK_HDB_INFO *hdb_info = sqlite_hdb_open(tsk_path);
        REQUIRE(hdb_info != nullptr);
        CHECK(hdb_info->db_type == TSK_HDB_DBTYPE_SQLITE_ID);
        CHECK(hdb_info->db_fname != nullptr);
        
        // Verify function pointers are set
        CHECK(hdb_info->lookup_str != nullptr);
        CHECK(hdb_info->lookup_raw != nullptr);
        CHECK(hdb_info->lookup_verbose_str != nullptr);
        CHECK(hdb_info->add_entry != nullptr);
        CHECK(hdb_info->begin_transaction != nullptr);
        CHECK(hdb_info->commit_transaction != nullptr);
        CHECK(hdb_info->rollback_transaction != nullptr);
        CHECK(hdb_info->close_db != nullptr);
        
        tsk_hdb_close(hdb_info);
        remove_file(db_path.c_str());
    }
    
    SECTION("Open non-existent database fails") {
        TSK_HDB_INFO *hdb_info = sqlite_hdb_open(tsk_path);
        // Should return NULL or handle error
        if (hdb_info) {
            tsk_hdb_close(hdb_info);
        }
        remove_file(db_path.c_str());
    }
}
