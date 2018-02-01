#define CATCH_CONFIG_MAIN
#include <stdio.h> // remove()
#include "catch.hpp"
#include "sqlite_cpp.h"

using namespace SQLite;

TEST_CASE("Basic Insert Test", "[test_insert_values]") {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");
    db.exec("INSERT INTO dillydilly VALUES ('Tom Brady', 28, 7)");
    db.exec("INSERT INTO dillydilly VALUES ('Ben Roethlisberger', 26, 14)");
    db.exec("INSERT INTO dillydilly VALUES ('Matthew Stafford', 25, 9)");
    db.exec("INSERT INTO dillydilly VALUES ('Drew Brees', 21, 7)");
    db.exec("INSERT INTO dillydilly VALUES ('Philip Rivers', 24, 10)");

    auto results = db.query("SELECT * FROM dillydilly");
    std::vector<std::string> row;
    int i = 0;
    while (results.next(row)) {
        switch (i) {
        case 0:
            REQUIRE(row == std::vector<std::string>({ "Tom Brady", "28", "7" }));
            break;
        case 1:
            REQUIRE(row == std::vector<std::string>({ "Ben Roethlisberger", "26", "14" }));
            break;
        case 2:
            REQUIRE(row == std::vector<std::string>({ "Matthew Stafford", "25", "9" }));
            break;
        case 3:
            REQUIRE(row == std::vector<std::string>({ "Drew Brees", "21", "7" }));
            break;
        case 4:
            REQUIRE(row == std::vector<std::string>({ "Philip Rivers", "24", "10" }));
            break;
        }

        i++;
    }

    // results.close();
    db.close();
    REQUIRE(remove("database.sqlite") == 0);
}

/** Test that prepared statements + variadic bind() works */
TEST_CASE("Prepared Statement Test", "[test_insert_values]") {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");
    // db.exec("BEGIN TRANSACTION");

    auto stmt = db.prepare("INSERT INTO dillydilly VALUES (?,?,?)");
    stmt.bind("Tom Brady", 28, 7);
    stmt.bind("Ben Roethlisberger", 26, 14);
    stmt.bind("Matthew Stafford", 25, 9);
    stmt.bind("Drew Brees", 21, 7);
    stmt.bind("Philip Rivers", 24, 10);
    stmt.bind(nullptr, nullptr, nullptr); // Should be stored as NULLs
    stmt.commit();

    // db.exec("COMMIT TRANSACTION");

    auto results = db.query("SELECT * FROM dillydilly");
    std::vector<std::string> row;
    int i = 0;

    REQUIRE(results.get_col_names() ==
        std::vector<std::string>({ "Player", "Touchdown", "Interception" }));
    while (results.next(row)) {
        switch (i) {
        case 0:
            REQUIRE(row == std::vector<std::string>({ "Tom Brady", "28", "7" }));
            break;
        case 1:
            REQUIRE(row == std::vector<std::string>({ "Ben Roethlisberger", "26", "14" }));
            break;
        case 2:
            REQUIRE(row == std::vector<std::string>({ "Matthew Stafford", "25", "9" }));
            break;
        case 3:
            REQUIRE(row == std::vector<std::string>({ "Drew Brees", "21", "7" }));
            break;
        case 4:
            REQUIRE(row == std::vector<std::string>({ "Philip Rivers", "24", "10" }));
            break;
        case 5:
            // Type-cast NULL values to empty strings
            REQUIRE(row == std::vector<std::string>({ "", "", "" }));
            break;
        }

        i++;
    }

    // Not necessary if db.close() works appropriately
    // stmt.close();
    // results.close();

    db.close();

    // remove() will return !0 if database hasn't been closed properly
    // and "database.sqlite" will not be deleted
    REQUIRE(remove("database.sqlite") == 0);
}

/** Test that SQLField works */
TEST_CASE("SQLField Test", "[test_sqlfield]") {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");
    // db.exec("BEGIN TRANSACTION");

    auto stmt = db.prepare("INSERT INTO dillydilly VALUES (?,?,?)");
    stmt.bind("Tom Brady", 28, 7);
    stmt.bind("Ben Roethlisberger", 26, 14);
    stmt.bind("Matthew Stafford", 25, 9);
    stmt.bind("Drew Brees", 21, 7);
    stmt.bind("Philip Rivers", 24, 10);
    stmt.bind(nullptr, nullptr, nullptr); // Should be stored as NULLs
    stmt.commit();

    // db.exec("COMMIT TRANSACTION");

    auto results = db.query("SELECT * FROM dillydilly");
    std::vector<SQLField> row;
    int i = 0;

    REQUIRE(results.get_col_names() ==
        std::vector<std::string>({ "Player", "Touchdown", "Interception" }));
    while (results.next(row)) {
        if (i <= 4) {
            REQUIRE(row[0].type() == SQLITE_TEXT);
            REQUIRE(row[1].type() == SQLITE_INTEGER);
            REQUIRE(row[2].type() == SQLITE_INTEGER);
        }

        switch (i) {
        case 0:
            REQUIRE(row[0].get<std::string>() == "Tom Brady");
            REQUIRE(row[1].get<long long int>() == 28);
            REQUIRE(row[2].get<long long int>() == 7);
            break;
        case 1:
            REQUIRE(row[0].get<std::string>() == "Ben Roethlisberger");
            REQUIRE(row[1].get<long long int>() == 26);
            REQUIRE(row[2].get<long long int>() == 14);
            break;
        case 2:
            REQUIRE(row[0].get<std::string>() == "Matthew Stafford");
            REQUIRE(row[1].get<long long int>() == 25);
            REQUIRE(row[2].get<long long int>() == 9);
            break;
        case 5:
            // NULL values
            REQUIRE(row[0].type() == SQLITE_NULL);
            REQUIRE(row[1].type() == SQLITE_NULL);
            REQUIRE(row[2].type() == SQLITE_NULL);
            break;
        }

        i++;
    }

    // Not necessary if db.close() works appropriately
    // stmt.close();
    // results.close();

    db.close();

    // remove() will return !0 if database hasn't been closed properly
    // and "database.sqlite" will not be deleted
    REQUIRE(remove("database.sqlite") == 0);
}

/** Test that iterating over empty result sets isn't dangerous */
TEST_CASE("Empty Query Test", "[test_no_results]") {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");

    auto results = db.query("SELECT * FROM dillydilly");
    std::vector<std::string> row;
    int i = 0;

    while (results.next(row)) {
        i++;
    }

    db.close();
    REQUIRE(i == 0);
    REQUIRE(remove("database.sqlite") == 0);
}