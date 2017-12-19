#include "catch.hpp"
#include "sqlite_cpp.h"

TEST_CASE("Syntax Error", "[test_exec_err]") {
    SQLite::Conn db("database.sqlite");
    bool exception_raised = false;

    try {
        // Intentional typo
        db.exec("SELCT * FROM sqlite_master");
    }
    catch (SQLite::SQLiteError&) {
        exception_raised = true;
    }
    
    db.close();
    remove("database.sqlite");
}

TEST_CASE("Operation on Closed Database", "[test_closed_db]") {
    SQLite::Conn db("db2.sqlite");
    db.close();
    bool exception_raised = false;

    try {
        db.exec("SELECT * FROM sqlite_master");
    }
    catch (SQLite::DatabaseClosed&) {
        exception_raised = true;
    }

    REQUIRE(exception_raised);

    remove("db2.sqlite");
}

/** Test that double close() calls don't cause a segfault */
TEST_CASE("sqlite3* Double Free", "[test_double_close]") {
    SQLite::Conn db("db3.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");
    db.exec("INSERT INTO dillydilly VALUES ('Tom Brady', 28, 7)");
    db.close();
    REQUIRE(db.base->db == nullptr);
    db.close();
    
    remove("db3.sqlite");
}

/** Test that attempting to bind too many arguments causes an error */
TEST_CASE("Too Many Values Test", "[test_too_many_values]") {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");
    bool error_thrown = false;

    auto stmt = db.prepare("INSERT INTO dillydilly VALUES (?,?,?)");

    try {
        stmt.bind("Tom Brady", 28, 3, 7);
    }
    catch (std::runtime_error) {
        error_thrown = true;
    }

    REQUIRE(error_thrown);

    db.close();

    // remove() will return !0 if database hasn't been closed properly
    // and "database.sqlite" will not be deleted
    REQUIRE(remove("database.sqlite") == 0);
}

TEST_CASE("Primary Key Violation", "[test_pkey_constraint]") {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT PRIMARY KEY, Touchdown int, Interception int)");
    auto stmt = db.prepare("INSERT INTO dillydilly VALUES (?,?,?)");
    bool error_thrown = false;

    // Assert that primary key violation error is raised
    try {
        stmt.bind("Tom Brady", 28, 3);
        stmt.bind("Tom Brady", 28, 3);
    }
    catch(SQLite::SQLiteError& e) {
        std::string err_msg = e.what();
        std::string exp_msg = SQLite::SQLITE_EXT_ERROR_MSG.find(1555)->second;
        REQUIRE(err_msg.find(exp_msg) != std::string::npos);
        error_thrown = true;
    }

    REQUIRE(error_thrown);
    error_thrown = false;

    // Assert that statement interface doesn't work anymore
    try {
        sqlite3_stmt* ptr = stmt.get_ptr();
    }
    catch (SQLite::StatementClosed& e) {
        error_thrown = true;
    }

    REQUIRE(error_thrown);
    db.close();

    // remove() will return !0 if database hasn't been closed properly
    // and "database.sqlite" will not be deleted
    REQUIRE(remove("database.sqlite") == 0);
}