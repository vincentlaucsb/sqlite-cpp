## SQLite for C++

SQLite for C++ is a wrapper around the SQLite C API which makes it easier and safer to use.
Key features of this library are:

 * No manual memory management needed
 * Easy to use interface, inspired by the JDBC (but with less verbose names)
 * Memory safe

## A Small Example
Here's a small snippet demonstrating this API. Notice how it involves no manual 
wrangling with pointers? That's because this library does it all behind the scenes
in a memory-safe, low overhead way using C++ features such as RAII and smart pointers.

```
int main() {
    SQLite::Conn db("database.sqlite");
    db.exec("CREATE TABLE dillydilly (Player TEXT, Touchdown int, Interception int)");

    auto stmt = db.prepare("INSERT INTO dillydilly VALUES (?,?,?)");
    stmt.bind("Tom Brady", 28, 7);
    stmt.bind("Ben Roethlisberger", 26, 14);
    stmt.bind("Matthew Stafford", 25, 9);
    stmt.bind("Drew Brees", 21, 7);
    stmt.bind("Philip Rivers", 24, 10);
    stmt.commit(); // PreparedStatements implicitly begin transactions
                   // Errors lead to an automatic rollback

    auto results = db.query("SELECT * FROM dillydilly");
    std::vector<std::string> row;
    while (results.next()) {
        row = results.get_row();
        // Do stuff with row here
    }

    return 0;
}   // Destructors for db, stmt, results, are automatically called
```
 
## Basics
Most basic tasks can be executed with the methods and classes listed below

### Connecting to a Database
 * SQLite::Conn: To connect to a database
 * SQLite::Conn.exec(): To execute a query that doesn't return anything
 
### Preparing Statements
 * SQLite::Conn::prepare(): To prepare a statement
 * SQLite::Conn::PreparedStatement
 * SQLite::Conn::PreparedStatement::bind: To bind values to the statement
 
### Querying the Database
 * SQLite::Conn::query(): To prepare/execute a query
 * SQLite::Conn::ResultSet
 * SQLite::Conn::ResultSet::next: To advance to the next row