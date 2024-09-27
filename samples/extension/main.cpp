#include <iostream>

#include <thinsqlitepp/database.hpp>


using namespace thinsqlitepp;

int main() {
    auto db = database::open("sample.db", SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX);
    db->enable_load_extension(true);
    db->load_extension("libsample-extension");

    db->exec("SELECT sample_function(5)", [] (int, row r) noexcept {
        std::cout << r[0].value<int>() << '\n';
        return true;
    });
}
