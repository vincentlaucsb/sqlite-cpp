/*
,---.   ,-----.   ,--.   ,--.  ,--.           ,---. ,-----.
'   .-' '  .-.  '  |  |   `--',-'  '-. ,---.  /    |'  .--./ ,---.  ,---.
`.  `-. |  | |  |  |  |   ,--.'-.  .-'| .-. :/  '  ||  |    | .-. || .-. |
.-'    |'  '-'  '-.|  '--.|  |  |  |  \   --.'--|  |'  '--'\| '-' '| '-' '
`-----'  `-----'--'`-----'`--'  `--'   `----'   `--' `-----'|  |-' |  |-'
`--'   `--'

SQLite for C++ (https://github.com/vincentlaucsb/sqlite-cpp/)
Copyright(c) 2017 Vincent La and released under the MIT License.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/** @file */

#include "../lib/sqlite3.h"
#include <string.h>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <memory>

/** @SQLite
 */
namespace SQLite {
    /** @name Errors
     *  Classes for various runtime errors
     */
    ///@{
    /** Thrown when attempting to use the interface of a close()'d database */
    class DatabaseClosed : public std::runtime_error {
    public:
        DatabaseClosed() :runtime_error("Attempted operation on a closed database.") {};
    };

    /** Thrown when attempting to use the interface of a close()'d statement */
    class StatementClosed : public std::runtime_error {
    public:
        StatementClosed() :runtime_error("Attempted operation on a closed statement.") {};
    };

    /** Used when invalid inputs are used */
    class ValueError : public std::runtime_error {
    public:
        ValueError() :runtime_error("[Value Error] Invalid value or value(s).") {};
        ValueError(const std::string& msg) :runtime_error("[Value Error] " + msg) {};
    };

    /** Used for errors returned by the SQLite API itself */
    class SQLiteError : public std::runtime_error {
    public:
        SQLiteError(const std::string& msg) :runtime_error("[SQLite Error] " + msg) {};
    };

    /** Map error codes to error messages */
    const std::map<int, std::string> SQLITE_ERROR_MSG = {
        { 1, "SQLITE_ERROR: Generic SQLite Error" },
        { 19, "SQLITE_CONSTRAINT: SQL constrainted violated" }
    };

    const std::map<int, std::string> SQLITE_EXT_ERROR_MSG = {
        { 787, "SQLITE_CONSTRAINT_FOREIGNKEY: Foreign key constraint failed" },
        { 1555, "SQLITE_CONSTRAINT_PRIMARYKEY: Primary key constraint failed" }
    };
    
    /** Return type for SQL queries */
    class SQLField {
        struct SQLFieldConcept {
            virtual ~SQLFieldConcept() {};
            virtual size_t type() = 0;
        };

        template<typename T> struct SQLFieldModel : SQLFieldConcept {
            SQLFieldModel(const T& t) : value(t) {};
            size_t type(); /**< Return the fundamental SQLite3 type */
            T value;
        };

        std::shared_ptr<SQLFieldConcept> value;

    public:
        template<typename T> SQLField(const T& val) :
            value(new SQLFieldModel<T>(val)) {};
        template<typename T> T get() { return ((SQLFieldModel<T>*)value.get())->value; }

        size_t type() { return value.get()->type(); }
    };

    template<>
    inline size_t SQLField::SQLFieldModel<long long int>::type() { return SQLITE_INTEGER; }

    template<>
    inline size_t SQLField::SQLFieldModel<double>::type() { return SQLITE_FLOAT; }

    template<>
    inline size_t SQLField::SQLFieldModel<std::nullptr_t>::type() { return SQLITE_NULL; }

    template<>
    inline size_t SQLField::SQLFieldModel<std::string>::type() { return SQLITE_TEXT; }

    /** Wrapper over a sqlite3 pointer */
    struct conn_base {
    public:
        sqlite3* db = nullptr;

        /** Return a reference to the sqlite pointer */
        sqlite3** get_ref() {
            return &(this->db);
        }
        
        void close() {
            if (db) {
                sqlite3_close(db);
                db = nullptr;
            }
        };
        conn_base() {};
        ~conn_base() {
            this->close();
        }
    };

    /** Wrapper over a sqlite3_stmt pointer */
    struct stmt_base {
    public:
        stmt_base() {};
        ~stmt_base() {
            // Note: Calling sqlite3_finalize on a nullptr is harmless
            sqlite3_finalize(this->stmt);
        }

        void close() {
            if (stmt) {
                sqlite3_finalize(stmt);
                stmt = nullptr;
            }
        }

        sqlite3_stmt* stmt = nullptr;
    };

    /** Connection to a SQLite database */
    class Conn {

        /** An interface for executing and iterating through SQL statements */
        class PreparedStatement {
        public:
            PreparedStatement(Conn& conn, const std::string& stmt);
            
            /** @name Binding Values */
            ///@{
            template<typename... Args>
            void bind(Args... args) {
                /** Bind any number of arguments to the statement and
                 *  automatically call next()
                 *
                 *  #### Safety
                 *  If too many arguments are bound, an error will be thrown at runtime.
                 */

                if (sizeof...(Args) > this->params) {
                    throw ValueError("Too many arguments to bind() " +
                        std::to_string(this->params) + " expected " + std::to_string(sizeof...(Args)) + " specified");
                }

                size_t i = 0;
                _bind_many(i, args...); // Recurse through templates
                this->next();
            }

            template<typename T>
            void bind(const size_t i, const T value) {
                bind<T>(i, value);
            }
            ///@}

            sqlite3_stmt* get_ptr();
            void commit();
            void next();
            void close() noexcept;

        protected:
            int params;
            Conn* conn;
            std::shared_ptr<stmt_base> base = std::make_shared<stmt_base>();
            const char * unused;

        private:
            /** @name Variadic bind() Helpers */
            ///@{
            template<typename T>
            void _bind_many(size_t i, T value) {
                bind<T>(i, value);
                i++;
            }

            template<typename T, typename... Args>
            void _bind_many(size_t i, T value, Args... args) {
                bind<T>(i, value);
                i++;
                _bind_many(i, args...); // Recurse through templates
            }
            ///@}
        };

        /** Class for representing results from a SQL query */
        class ResultSet : PreparedStatement {
        public:
            std::vector<std::string> get_col_names();
            std::vector<std::string> get_row();
            std::vector<SQLField> get_values();
            int num_cols();
            bool next();
            using PreparedStatement::close;
            using PreparedStatement::PreparedStatement;
        };

    public:
        Conn(const char * db_name);
        Conn(const std::string db_name);
        ~Conn();
        void exec(const std::string query);
        Conn::PreparedStatement prepare(const std::string& stmt);
        Conn::ResultSet query(const std::string& stmt);
        void close() noexcept;

        sqlite3* get_ptr();
        std::shared_ptr<conn_base> base =
            std::make_shared<conn_base>(); /** Database handle */
        char * error_message = nullptr;    /** Buffer for error messages */
    private:
        std::queue<PreparedStatement*> stmts; /** Keep track of prepared statements
                                               *  so we can dealloc them on close() */
    };

    void throw_sqlite_error(const int& error_code,
        const int& ext_error_code=-1);
    ///@}
    
    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const char* value) {
        sqlite3_bind_text(
            this->get_ptr(),    // Pointer to prepared statement
            i + 1,              // Index of parameter to set
            value,              // Value to bind
            strlen(value),      // Size of string in bytes
            SQLITE_TRANSIENT);  // String destructor
    }

    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const std::string value) {
        /** Bind text values to the statement
        *
        *  **Note:** This function is zero-indexed while
        *  sqlite3_bind_* is 1-indexed
        */
        sqlite3_bind_text(
            this->get_ptr(),    // Pointer to prepared statement
            i + 1,              // Index of parameter to set
            value.c_str(),      // Value to bind
            value.size(),       // Size of string in bytes
            SQLITE_TRANSIENT);  // String destructor
    }

    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const int value) {
        /** Bind integer values to the statement */
        sqlite3_bind_int(this->get_ptr(), i + 1, value);
    }

    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const long int value) {
        /** Bind integer values to the statement */
        sqlite3_bind_int64(this->get_ptr(), i + 1, value);
    }

    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const long long int value) {
        /** Bind integer values to the statement */
        sqlite3_bind_int64(this->get_ptr(), i + 1, value);
    }

    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const double value) {
        /** Bind floating point values to the statement */
        sqlite3_bind_double(this->get_ptr(), i + 1, value);
    }

    template<>
    inline void Conn::PreparedStatement::bind(const size_t i, const std::nullptr_t value) {
        sqlite3_bind_null(this->get_ptr(), i + 1);
    }
}