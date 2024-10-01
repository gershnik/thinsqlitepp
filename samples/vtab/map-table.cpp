#include <iostream>
#include <string_view>
#include <map>
#include <string>
#include <stddef.h>

#include <thinsqlitepp/vtab.hpp>
#include <thinsqlitepp/database.hpp>


using namespace thinsqlitepp;

class map_table : public vtab<map_table> {
public:
    using map_type = std::map<int, std::string>;
    using map_range = std::pair<map_type::iterator, map_type::iterator>;
    using map_indexer = map_range (map_type &, map_type::iterator, map_type::iterator, map_type::key_type);

    //variable length data to pass between best_index() and filter()
    struct comparisons_array : public sqlite_allocated {
        size_t count = 0;
        map_indexer * indexer[1];
    };
    //later we will want SQLite to deallocate comparisons_array rather than handle it ourselves
    //so it needs to be trivially_destructible. Let's make sure
    static_assert(std::is_trivially_destructible_v<comparisons_array>);
    
    //allocate variable length index data
    static comparisons_array * allocate_index_data(size_t entry_count) {
        size_t alloc_size = offsetof(comparisons_array, indexer) + sizeof(comparisons_array) * entry_count;
        auto ret = (comparisons_array *)comparisons_array::operator new(alloc_size);
        ret->count = 0;
        return ret;
    }
    
    //convert SQLite constraint operation to map range lookup
    static map_indexer * find_indexer(int op) {
        struct comparator {
            bool operator()(const map_type::value_type & lhs, map_type::key_type rhs) const
                { return comp(lhs.first, rhs); }
            bool operator()(map_type::key_type lhs, const map_type::value_type & rhs) const
                { return comp(lhs, rhs.first); }
            map_type::key_compare comp;
        };
        switch (op) {
            case SQLITE_INDEX_CONSTRAINT_EQ:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return std::equal_range(begin, end, val, comparator{map.key_comp()});
                };
            case SQLITE_INDEX_CONSTRAINT_GT:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{std::upper_bound(begin, end, val, comparator{map.key_comp()}), end};
                };
            case SQLITE_INDEX_CONSTRAINT_LE:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{begin, std::upper_bound(begin, end, val, comparator{map.key_comp()})};
                };
            case SQLITE_INDEX_CONSTRAINT_LT:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{begin, std::lower_bound(begin, end, val, comparator{map.key_comp()})};
                };
            case SQLITE_INDEX_CONSTRAINT_GE:
                return [](map_type & map, map_type::iterator begin, map_type::iterator end, map_type::key_type val) {
                    return map_range{std::lower_bound(begin, end, val, comparator{map.key_comp()}), end};
                };
        }
        return nullptr;
    }

public:
    
    using constructor_data_type = map_type *;
    using index_data_type =  comparisons_array *;
    
    map_table(connect_t, database * db, map_type * map, int argc, const char * const * argv):
        _map(map) {
        db->declare_vtab(R"_(
         CREATE TABLE this_name_is_ignored (
                         key INTEGER PRIMARY KEY,
                         value TEXT)
        )_");
    }

    bool best_index(index_info<index_data_type> & info) const {
        auto constraints = info.constraints();
        auto usages = info.constraints_usage();
        auto orderbys = info.orderbys();

        //first let's check if already provide the required order, if any
        bool properly_ordered = true;
        for (auto order: orderbys) {
            if (order.iColumn != 0 && order.iColumn != -1)
                properly_ordered = false;
            else if (order.desc)
                properly_ordered = false;
        }
        info.set_order_by_consumed(properly_ordered);


        //next check if we can handle the constraints        
        if (!constraints.empty()) {
            std::unique_ptr<comparisons_array> comparisons{allocate_index_data(constraints.size())};
            for (size_t i = 0; i < constraints.size(); ++i) {
                auto & constraint = constraints[i];
                auto & usage = usages[i];

                //skip unused constraints or constraints not on key/rowid column
                if (!constraint.usable || (constraint.iColumn != 0 && constraint.iColumn != -1))
                    continue;
            
                if (auto indexer = find_indexer(constraint.op)) {
                    //if we can handle the operation using map lookup add the
                    //indexer to the data
                    comparisons->indexer[comparisons->count] = indexer;
                    //set argvIndex so the value to compare to is given to filter()
                    usage.argvIndex = int(++comparisons->count);
                    //and tell SQLite not to check this condition itself
                    usage.omit = true;
                }
            }
            if (comparisons->count) {
                //we don't use index_number but still let's set it to non-default value
                info.set_index_number(1);
                //move our collected indexers data into info
                info.set_index_data(std::move(comparisons));
                //set the cost to 0 - we can handle it best
                info.set_estimated_cost(0);
                return true;
            } 
        }
        
        //if we have no indexers to use set a high but not infinite cost and let SQLite do the filtering
        info.set_estimated_cost((double)2147483647);
        return true;
    }

    class cursor : public vtab::cursor {
    public:
        //inherit base constructor
        using vtab::cursor::cursor;

        void filter(int index_num, comparisons_array * index_data, int argc, value ** argv) {
            
            auto & map = *(owner()->_map);

            //initialize our range to the full map
            _current = map.begin();
            _end = map.end();

            //if we have indexers available, restrict the range
            //to satisfy all of them
            if (index_data) {
                for (size_t i = 0; i < index_data->count; ++i) {
                    auto val = argv[i]->get<map_type::key_type>();
                    std::tie(_current, _end) = index_data->indexer[i](map, _current, _end, val);
                }
            }
        }

        bool eof() const noexcept
            { return _current == _end; }
        
        void next() 
            { ++_current; }

        sqlite_int64 rowid() const
            { return sqlite_int64(_current->first); }

        void column(context & ctxt, int idx) const {
            if (idx == 0)
                ctxt.result(_current->first);
            else
                ctxt.result(_current->second);
        }

    private:
        map_type::iterator _current;
        map_type::iterator _end;
    };

private:
    map_type * _map;
};

int main() {
    auto db = database::open("sample.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    std::map<int, std::string> map{
        {74, "a"},
        {42, "b"},
        {50, "c"},
        {80, "d"}
    };
    map_table::create_module(*db, "map_table_module", &map);

    db->exec("SELECT key, value FROM map_table_module WHERE key > 50 ORDER BY key ASC", [] (row r) {
        std::cout << r[0].value<int>() << ": " << r[1].value<std::string_view>() << '\n';
        return true;
    });
}
