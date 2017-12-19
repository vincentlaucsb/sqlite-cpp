BUILD_DIR = build
SOURCES = $(wildcard src/*cpp)
TEST_SOURCES = $(wildcard tests/*.cpp)

# Debugging flags
CFLAGS = -pthread -ldl --std=c++11 -Og -g --coverage

# Compiled Objects
SQLITE3 = $(BUILD_DIR)/sqlite3.o
SQLITE_CPP = $(BUILD_DIR)/sqlite_cpp.o

# SQLite3
$(SQLITE3):
	mkdir -p $(BUILD_DIR)
	$(CC) -c -o $(BUILD_DIR)/sqlite3.o -O3 lib/sqlite3.c -pthread -ldl -Ilib/

# Main Library
$(SQLITE_CPP):
	mkdir -p $(BUILD_DIR)
	$(CC) -c -o $(SQLITE_CPP) $(SQLITE3) $(SOURCES) -Isrc/ $(CFLAGS)
	
test_sqlite: $(SQLITE3) $(SQLITE_CPP)
	$(CXX) -o test_sqlite $(TEST_SOURCES) $(SQLITE3) $(SQLITE_CPP) $(CFLAGS) -Ilib/ -Isrc/ -Itests/
	./test_sqlite
	
code_cov: test_sqlite
	mkdir -p test_results
	mv $(BUILD_DIR)/*.gcno $(BUILD_DIR)/*.gcda $(PWD)/test_results
	gcov $(SOURCES) -o test_results --relative-only
	mv *.gcov test_results
	
clean:
	rm -rf build
	rm -f test_sqlite