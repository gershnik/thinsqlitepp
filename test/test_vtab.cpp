#include <doctest.h>
#include "mock_sqlite.hpp"

#include <thinsqlitepp/vtab.hpp>
#include <thinsqlitepp/memory.hpp>
#include <thinsqlitepp/database.hpp>

using namespace thinsqlitepp;

namespace
{
    class vector_table : public vtab<vector_table>
    {
    public:
        struct entry
        {
            sqlite3_int64 rowid;
            std::string value;

            bool operator==(const entry & rhs) const 
                { return this->rowid == rhs.rowid && this->value == rhs.value; }
            bool operator!=(const entry & rhs) const 
                { return !(*this == rhs); }
        };
        
        using constructor_data_type = std::vector<entry> *;

    public:
        vector_table(
        #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 9, 0)    
            connect_t, 
        #endif    
        database * db, std::vector<entry> * data, int /*argc*/, const char * const * /*argv*/):
            _entries(data)
        {
            db->declare_vtab("CREATE TABLE _ (name TEXT)");
        #ifdef SQLITE_VTAB_DIRECTONLY
            db->vtab_config<SQLITE_VTAB_DIRECTONLY>();
        #endif
        }

        class cursor : public vtab::cursor
        {
        public:
            using vtab::cursor::cursor;
            
            bool eof() const noexcept
                { return _current == _end; }
            
            void next()
                { ++_current; }

            sqlite_int64 rowid() const
                { return _current->rowid; }

            void column(context & ctxt, int idx) const
            {
                if (idx != 0)
                    std::terminate();
                ctxt.result(_current->value);
            }

            void filter(int /*idx*/, int /*argc*/, value ** /*argv*/)
            {
                _current = owner()->_entries->begin();
                _end = owner()->_entries->end();
            }
        private:
            std::vector<entry>::const_iterator _current;
            std::vector<entry>::const_iterator _end;
        };

    private:
        std::vector<entry> * _entries;
    };

    template<class LHS, class RHS>
    bool equalRanges(const LHS & lhs, const RHS & rhs)
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
}

TEST_SUITE_BEGIN("vtab");

TEST_CASE( "basics" ) {

    auto db = database::open("foo.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);

    std::vector<vector_table::entry> entries = {
        { 1, "haha"},
        { 2, "hoho"}
    };
    vector_table::create_module(*db, "blah", &entries);

    #if SQLITE_VERSION_NUMBER < SQLITEPP_SQLITE_VERSION(3, 9, 0)  
    db->exec("CREATE VIRTUAL TABLE blah USING blah");
    #endif

    std::vector<vector_table::entry> selected;
    db->exec("SELECT rowid, * FROM blah", [&selected] (int, row r) noexcept {
        selected.push_back(vector_table::entry{r[0].value<int64_t>(), std::string(r[1].value<std::string_view>())});
        return true;
    });
    CHECK(equalRanges(selected, entries));

    selected.clear();
    db->exec("SELECT rowid, * FROM blah WHERE rowid = 2", [&selected] (int, row r) noexcept {
        selected.push_back(vector_table::entry{r[0].value<int64_t>(), std::string(r[1].value<std::string_view>())});
        return true;
    });
    CHECK(equalRanges(selected, std::vector<vector_table::entry>{{2, "hoho"}}));
    
    selected.clear();
    db->exec("SELECT rowid, * FROM blah WHERE name = 'hoho'", [&selected] (int, row r) noexcept {
        selected.push_back(vector_table::entry{r[0].value<int64_t>(), std::string(r[1].value<std::string_view>())});
        return true;
    });
    CHECK(equalRanges(selected, std::vector<vector_table::entry>{{2, "hoho"}}));

    #if SQLITE_VERSION_NUMBER < SQLITEPP_SQLITE_VERSION(3, 9, 0)  
    db->exec("DROP TABLE blah");
    #endif
}


TEST_SUITE_END();
