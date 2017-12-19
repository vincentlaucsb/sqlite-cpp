#include "sqlite_cpp.h"

namespace SQLite {
    /** @file
     *  Blah
     */

    void throw_sqlite_error(const int& error_code, const int& ext_error_code) {
        auto error_msg = SQLITE_ERROR_MSG.find(error_code);
        auto ext_error_msg = SQLITE_EXT_ERROR_MSG.find(ext_error_code);

        if (error_msg != SQLITE_ERROR_MSG.end()) {
            if (ext_error_msg != SQLITE_EXT_ERROR_MSG.end())
                throw SQLiteError(ext_error_msg->second);
            else
                throw SQLiteError(error_msg->second);
        }
        else {
            throw SQLiteError("Code " + std::to_string(error_code));
        }
    }
    
    Conn::Conn(const char * db_name) {
        /** Open a connection to a SQLite3 database
         *  @param[in] db_name Path to SQLite3 database
         */
        if (sqlite3_open(db_name, this->base->get_ref()))
            throw SQLiteError("Failed to open database");
    };

    Conn::Conn(const std::string db_name) {
        /** Open a connection to a SQLite3 database
         *  @param[in] db_name Path to SQLite3 database
         */
        if (sqlite3_open(db_name.c_str(), this->base->get_ref()))
            throw SQLiteError("Failed to open database");
    };

    void Conn::exec(const std::string query) {
        /** Execute a query that doesn't return anything
         *  @param[in] query A SQL query
         */

        if (sqlite3_exec(this->get_ptr(),
            (const char*)query.c_str(),
            0,  // Callback
            0,  // Arg to callback
            &error_message))
            throw SQLiteError(error_message);
    }

    sqlite3* Conn::get_ptr() {
        /**
         * Return a raw pointer to the sqlite3 handle.
         * 
         * #### Memory Safety
         * This function will never return invalid pointers. A runtime_error is thrown
         * if this is used after Conn::close() has been called.
         */

        if (this->base->db) {
            return this->base->db;
        }
        else { // nullptr
            throw DatabaseClosed();
        }
    }

    void Conn::close() noexcept {
        /** Close the active database connection. **If there are active
         *  prepared statements using this connection, they are also
         *  closed.** Attempting to use the database or any associated
         *  statements after calling close() will throw an error.
         *
         *  In most cases, calling this method explicitly is not necessary
         *  because it gets called automatically when Conn goes out of 
         *  scope.
         *
         *  #### Memory/Exception Safety
         *  This method will never throw an exception. Calling close() after the first
         *  time will harmlessly do nothing.
         **/

        // https://sqlite.org/c3ref/close.html

        PreparedStatement* stmt;

        /* Deallocate prepared statements
         * Note: This should be completely harmless,
         * even if some of the stmts are nullptrs/have been closed
         */
        while (!this->stmts.empty()) {
            stmt = this->stmts.front();
            stmt->close();
            this->stmts.pop();
        }

        this->base->close();
    }

    //
    // PreparedStatement
    //

    Conn::PreparedStatement::PreparedStatement(
        Conn& conn, const std::string& stmt) {
        /** Prepare a SQL statement
         *  @param[in]  conn An active SQLite connection
         *  @param[out] stmt A SQL query that should be prepared
         */

        this->conn = &conn;
        conn.stmts.push(this);
        int result = sqlite3_prepare_v2(
            this->conn->get_ptr(),       /* Database handle */
            (const char *)stmt.c_str(),  /* SQL statement, UTF-8 encoded */
            stmt.size(),                 /* Maximum length of zSql in bytes. */
            &(this->base->stmt),         /* OUT: Statement handle */
            &(this->unused)              /* OUT: Pointer to unused portion of zSql */
        );

        // explain_sqlite_error(result);

        this->params = sqlite3_bind_parameter_count(this->get_ptr());
    }


    sqlite3_stmt* Conn::PreparedStatement::get_ptr() {
        /** Get a raw pointer to the underlying sqlite3_stmt */
        if (this->base->stmt) {
            return this->base->stmt;
        }
        else { // nullptr
            throw StatementClosed();
        }
    }
    
    Conn::PreparedStatement Conn::prepare(const std::string& stmt) {
        /** Prepare a query for execution */
        this->exec("BEGIN TRANSACTION");
        return Conn::PreparedStatement(*this, stmt);
    }

    Conn::ResultSet Conn::query(const std::string& stmt) {
        /** Return a query */
        return Conn::ResultSet(*this, stmt);
    }

    void Conn::PreparedStatement::commit() {
        /** End a transaction */
        this->conn->exec("END TRANSACTION");
        this->close();
    }

    void Conn::PreparedStatement::close() noexcept {
        /** Close the prepared statement */
        this->base->close();
    }

    void Conn::PreparedStatement::next() {
        /** Call after bind()-ing values to execute statement */

        int result = sqlite3_step(this->get_ptr());
        int ext_res = sqlite3_extended_errcode(this->conn->get_ptr());
        if (result != 101 || sqlite3_reset(this->get_ptr()) != 0) {
            // Rollback transaction on failure
            this->conn->exec("ROLLBACK");
            this->base->close();
            throw_sqlite_error(result, ext_res);
        }
    }

    //
    // SQLiteResultSet
    // 

    std::vector<std::string> Conn::ResultSet::get_col_names() {
        /** Retrieve the column names of a SQL query result */
        std::vector<std::string> ret;
        int col_size = this->num_cols();
        for (int i = 0; i < col_size; i++)
            ret.push_back(sqlite3_column_name(this->get_ptr(), i));

        return ret;
    }

    std::vector<std::string> Conn::ResultSet::get_row() {
        /** After calling next_result(), use this to type-cast
         *  the next row from a query into a string vector
         */
        std::vector<std::string> ret;
        int col_size = this->num_cols();

        for (int i = 0; i < col_size; i++) {
            ret.push_back(std::string((char *)
                sqlite3_column_text(this->get_ptr(), i)));
        }

        return ret;
    }

    int Conn::ResultSet::num_cols() {
        /** Returns the number of columns in a SQL query result */
        return sqlite3_column_count(this->get_ptr());
    }

    bool Conn::ResultSet::next() {
        /** Retrieves the next row from the a SQL result set,
         *  or returns False if we're done
         */

        /* 100 --> More rows are available
        * 101 --> Done
        */
        return (sqlite3_step(this->get_ptr()) == 100);
    }
}