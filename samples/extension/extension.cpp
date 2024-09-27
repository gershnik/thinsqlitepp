#define THINSQLITEPP_BUILDING_EXTENSION 1
#include <thinsqlitepp/database.hpp>

using namespace thinsqlitepp;


SQLITE_EXTENSION_INIT1


void sample_function(context * ctxt, int /*argc*/, value ** argv) noexcept {
    auto arg = argv[0]->get<int>();
    ctxt->result(arg + 17);
}


extern "C"
#if defined(_WIN32)
__declspec(dllexport)
#elif defined(__GNUC__)
[[gnu::visibility("default")]]
#endif
int sqlite3_sampleextension_init(database * db, char ** pzErrMsg, const sqlite3_api_routines * pApi){
    SQLITE_EXTENSION_INIT2(pApi);
  
    try{

        db->create_function("sample_function", 1, SQLITE_UTF8, &sample_function, nullptr);
        return SQLITE_OK;
    }
    catch(exception & ex) {
        *pzErrMsg = const_cast<char *>(ex.error().extract_message().release());
        return ex.extended_error_code();
    }
    catch(std::exception & ex) {
        auto what = ex.what();
        auto len = strlen(what) + 1;
        if (auto message = (const char *)sqlite3_malloc(len)) {
            memcpy(*pzErrMsg, what, len);
        }
        return SQLITE_ERROR;
    }
}

