/*
 Copyright 2024 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/thinsqlitepp/blob/main/LICENSE
*/

#ifndef HEADER_SQLITEPP_VTAB_IMPL_INCLUDED
#define HEADER_SQLITEPP_VTAB_IMPL_INCLUDED

namespace thinsqlitepp
{
    #if __cpp_designated_initializers >= 201707L
        #define SQLITEPP_DESIGNATED(name, value)  .name = value
    #else
        #define SQLITEPP_DESIGNATED(name, value)  value
    #endif

    template<class Derived>
    inline sqlite3_module * vtab<Derived>::get_module()
    {
        check_requirements();
        static sqlite3_module the_module = {
            SQLITEPP_DESIGNATED(iVersion,       4),
            SQLITEPP_DESIGNATED(xCreate,        get_create_impl()),
            SQLITEPP_DESIGNATED(xConnect,       get_connect_impl()),
            SQLITEPP_DESIGNATED(xBestIndex,     best_index_impl),
            SQLITEPP_DESIGNATED(xDisconnect,    disconnect_impl),
            SQLITEPP_DESIGNATED(xDestroy,       destroy_impl),
            SQLITEPP_DESIGNATED(xOpen,          open_impl),
            SQLITEPP_DESIGNATED(xClose,         close_impl),
            SQLITEPP_DESIGNATED(xFilter,        filter_impl),
            SQLITEPP_DESIGNATED(xNext,          next_impl),
            SQLITEPP_DESIGNATED(xEof,           eof_impl),
            SQLITEPP_DESIGNATED(xColumn,        column_impl),
            SQLITEPP_DESIGNATED(xRowid,         rowid_impl),
            SQLITEPP_DESIGNATED(xUpdate,        get_update_impl()),
            SQLITEPP_DESIGNATED(xBegin,         get_begin_impl()),
            SQLITEPP_DESIGNATED(xSync,          get_sync_impl()),
            SQLITEPP_DESIGNATED(xCommit,        get_commit_impl()),
            SQLITEPP_DESIGNATED(xRollback,      get_rollback_impl()),
            SQLITEPP_DESIGNATED(xFindFunction,  get_find_function_impl()),
            SQLITEPP_DESIGNATED(xRename,        get_rename_impl()),
            SQLITEPP_DESIGNATED(xSavepoint,     get_savepoint_impl()),
            SQLITEPP_DESIGNATED(xRelease,       get_release_impl()),
            SQLITEPP_DESIGNATED(xRollbackTo,    get_rollback_to_impl()),
            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)
            SQLITEPP_DESIGNATED(xShadowName,    get_shadow_name_impl()),
            #endif
            #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)
            SQLITEPP_DESIGNATED(xIntegrity,     get_integrity_impl()),
            #endif
        };

        return &the_module;
    }

    #undef SQLITEPP_DESIGNATED

    //Detect vtab related things existing in a class
    struct vtab_detector
    {
    public:
        template<class T> static constexpr bool has_common_constructor = std::is_void_v<typename T::constructor_data_type> ?
                                                                                 std::is_constructible_v<T, 
                                                                                                         database *, 
                                                                                                         int, 
                                                                                                         const char * const *> :
                                                                                 std::is_constructible_v<T, 
                                                                                                         database *, 
                                                                                                         typename T::constructor_data_type, 
                                                                                                         int, 
                                                                                                         const char * const *>;

        template<class T> static constexpr bool has_create_constructor = std::is_void_v<typename T::constructor_data_type> ?
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::create_t, 
                                                                                                        database *, 
                                                                                                        int, 
                                                                                                        const char * const *> :
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::create_t, 
                                                                                                        database *, 
                                                                                                        typename T::constructor_data_type, 
                                                                                                        int, 
                                                                                                        const char * const *>;
        template<class T> static constexpr bool has_connect_constructor = std::is_void_v<typename T::constructor_data_type> ?
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::connect_t, 
                                                                                                        database *, 
                                                                                                        int, 
                                                                                                        const char * const *> :
                                                                                std::is_constructible_v<T, 
                                                                                                        typename T::connect_t, 
                                                                                                        database *, 
                                                                                                        typename T::constructor_data_type, 
                                                                                                        int, 
                                                                                                        const char * const *>;

        SQLITEPP_STATIC_METHOD_DETECTOR(void, disconnect, std::unique_ptr<T>{});
        SQLITEPP_STATIC_METHOD_DETECTOR(void, destroy, std::unique_ptr<T>{});

        SQLITEPP_METHOD_DETECTOR(int64_t, update, int{}, (value **)nullptr);
        SQLITEPP_METHOD_DETECTOR(int, find_function, int{}, 
                                                     (const char *)nullptr, 
                                                     (void (**)(context*,int,value**) noexcept)nullptr,
                                                     (void **)nullptr);
        SQLITEPP_METHOD_DETECTOR_0(void, begin);
        SQLITEPP_METHOD_DETECTOR_0(void, sync);
        SQLITEPP_METHOD_DETECTOR_0(void, commit);
        SQLITEPP_METHOD_DETECTOR_0(void, rollback);
        SQLITEPP_METHOD_DETECTOR(void, rename, (const char *)nullptr);
        SQLITEPP_METHOD_DETECTOR(void, savepoint, int{});
        SQLITEPP_METHOD_DETECTOR(void, release, int{});
        SQLITEPP_METHOD_DETECTOR(void, rollback_to, int{});
        SQLITEPP_STATIC_METHOD_DETECTOR(bool, shadow_name, (const char *)nullptr);
        SQLITEPP_METHOD_DETECTOR(allocated_string, integrity, (const char *)nullptr, (const char *)nullptr, int{});
    };

    #if __cpp_concepts >= 201907L && __cpp_lib_concepts >= 202002L

        template<class T>
        concept is_vtab = 
            std::is_base_of_v<vtab<T>, T> && 
            (std::is_void_v<typename T::constructor_data_type> || std::is_pointer_v<typename T::constructor_data_type>) &&
            (std::is_void_v<typename T::index_data_type> || 
                (std::is_pointer_v<typename T::index_data_type> && std::is_trivially_destructible_v<typename T::index_data_type>)) &&
            std::is_base_of_v<typename vtab<T>::cursor, typename T::cursor> &&
            requires(T obj, const T cobj, index_info<typename T::index_data_type> & index) 
            {
                { cobj.best_index(index) } -> std::convertible_to<bool>;
                { obj.open() } -> std::convertible_to<std::unique_ptr<typename T::cursor>>;
            } &&
            requires(typename T::cursor & cur, const typename T::cursor & ccur, context & ctxt)
            {
                { ccur.eof() } noexcept -> std::convertible_to<bool>;
                requires 
                    (
                        !std::is_void_v<typename T::index_data_type> && 
                        requires { { cur.filter(int{}, (typename T::index_data_type *)nullptr, int{}, (value **)nullptr) } -> std::same_as<void>; }
                    ) || (
                        std::is_void_v<typename T::index_data_type> && 
                        requires { { cur.filter(int{}, int{}, (value **)nullptr) } -> std::same_as<void>; }
                    );
                { cur.next() } -> std::same_as<void>;
                { ccur.column(ctxt, int{}) } -> std::same_as<void>;
                { ccur.rowid() } -> std::same_as<int64_t>;
            };

    #endif

    template<class Derived>
    constexpr void vtab<Derived>::check_requirements()
    {
        #if __cpp_concepts >= 201907L && __cpp_lib_concepts >= 202002L

            static_assert(is_vtab<Derived>);

        #endif

        if constexpr (vtab_detector::has_common_constructor<Derived>)
        {
            static_assert(!vtab_detector::has_create_constructor<Derived>,
                            "if you declare a common constructor you cannot also have a constructor that takes create_t");
            static_assert(!vtab_detector::has_connect_constructor<Derived>,
                            "if you declare a common constructor you cannot also have a constructor that takes connect_t");
        }
        else
        {
            static_assert(vtab_detector::has_connect_constructor<Derived>,
                          "you must declare either a constructor that takes a connect_t OR a common constructor");

            #if SQLITE_VERSION_NUMBER < SQLITEPP_SQLITE_VERSION(3, 9, 0)

                static_assert(vtab_detector::has_create_constructor<Derived>,
                              "you must declare either a constructor that takes a create_t OR a common constructor");

            #endif
        }

        if constexpr (vtab_detector::has_disconnect<Derived>) 
            static_assert(vtab_detector::has_noexcept_disconnect<Derived>, "disconnect() must be noexcept");

        if constexpr (vtab_detector::has_destroy<Derived>) 
            static_assert(vtab_detector::has_noexcept_destroy<Derived>, "destroy() must be noexcept");
        
        if constexpr (vtab_detector::has_find_function<Derived>) 
            static_assert(vtab_detector::has_noexcept_find_function<Derived>, "find_function() must be noexcept");

        if constexpr (vtab_detector::has_shadow_name<Derived>) 
            static_assert(vtab_detector::has_noexcept_shadow_name<Derived>, "shadow_name() must be noexcept");

        static_assert(std::is_base_of_v<vtab<Derived>::cursor, typename Derived::cursor>,
                      "Derived::cursor type must derive from vtab<Derived>::cursor");
    }

    template<class Derived>
    int vtab<Derived>::create_impl(sqlite3 * db, [[maybe_unused]] void * aux, int argc, const char * const * argv, sqlite3_vtab ** pp_vtab, char ** err)
    {
        try 
        {
            if constexpr (vtab_detector::has_common_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else if constexpr (vtab_detector::has_create_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(create_t{}, database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(create_t{}, database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else
                static_assert(dependent_false<Derived>, "neither required constructor form is present");
            return SQLITE_OK;
        }
        catch(exception & ex) 
        {
            auto message = ex.error().extract_message();
            *err = (char *)message.release();
        }
        catch(std::exception & ex)
        {
            auto message = ex.what();
            const auto len = strlen(message) + 1;
            if (char * const ret = (char *)sqlite_allocate_nothrow(len))
            {
                memcpy(ret, message, len);
                *err = ret;
            }
        }
        return SQLITE_ERROR;
    }

    template<class Derived>
    int vtab<Derived>::connect_impl(sqlite3 * db, [[maybe_unused]] void * aux, int argc, const char * const * argv, sqlite3_vtab ** pp_vtab, char ** err)
    {
        try 
        {
            if constexpr (vtab_detector::has_common_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else if constexpr(vtab_detector::has_connect_constructor<Derived>)
                if constexpr (std::is_void_v<typename Derived::constructor_data_type>)
                    *pp_vtab = new Derived(connect_t{}, database::from(db), argc, argv);
                else
                    *pp_vtab = new Derived(connect_t{}, database::from(db), (typename Derived::constructor_data_type)aux, argc, argv);
            else
                static_assert(dependent_false<Derived>, "neither required constructor form is present");
            return SQLITE_OK;
        }
        catch(exception & ex) 
        {
            auto message = ex.error().extract_message();
            *err = (char *)message.release();
        }
        catch(std::exception & ex)
        {
            auto message = ex.what();
            const auto len = strlen(message) + 1;
            if (char * const ret = (char *)sqlite_allocate_nothrow(len))
            {
                memcpy(ret, message, len);
                *err = ret;
            }
        }
        return SQLITE_ERROR;
    }

    template<class Derived>
    constexpr decltype(sqlite3_module::xCreate) vtab<Derived>::get_create_impl() 
    {
        if constexpr (vtab_detector::has_common_constructor<Derived> || vtab_detector::has_create_constructor<Derived>)
        {
            return create_impl;
        }
        else 
        {
            return nullptr;
        }
    }

    template<class Derived>
    constexpr decltype(sqlite3_module::xCreate) vtab<Derived>::get_connect_impl() 
    {
        if constexpr (vtab_detector::has_common_constructor<Derived>)
        {
            return create_impl;
        }
        else
        {
            return connect_impl;
        }
    }

    #define SQLITEPP_BEGIN_CALLBACK try
    #define SQLITEPP_END_CALLBACK   catch(exception & ex) { \
                                        me->set_error_message(ex); \
                                        return ex.extended_error_code(); \
                                    } catch(std::exception & ex) { \
                                        me->set_error_message(ex); \
                                        return SQLITE_ERROR; \
                                    }

    template<class Derived>
    int vtab<Derived>::best_index_impl(sqlite3_vtab * vtab, sqlite3_index_info * info)
    {
        const auto * me = static_cast<Derived *>(vtab);
        auto myinfo = index_info<typename Derived::index_data_type>::from(info);
        SQLITEPP_BEGIN_CALLBACK 
        {
            bool res = me->best_index(*myinfo);
            return res ? SQLITE_OK : SQLITE_CONSTRAINT;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::disconnect_impl(sqlite3_vtab * vtab)
    {
        auto me = std::unique_ptr<Derived>(static_cast<Derived *>(vtab));

        if constexpr (vtab_detector::has_disconnect<Derived>) {
            Derived::disconnect(std::move(me));
        }
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::destroy_impl(sqlite3_vtab * vtab)
    {
        auto me = std::unique_ptr<Derived>(static_cast<Derived *>(vtab));

        if constexpr (vtab_detector::has_destroy<Derived>) {
            Derived::destroy(std::move(me));
        }
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::open_impl(sqlite3_vtab * vtab, sqlite3_vtab_cursor ** cursor)
    {
        auto me = static_cast<Derived *>(vtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            std::unique_ptr<typename Derived::cursor> res = me->open();
            *cursor = res.release()->c_ptr();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::close_impl(sqlite3_vtab_cursor * cursor)
    {
        auto me = static_cast<typename Derived::cursor *>(cursor);
        delete me;
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::eof_impl(sqlite3_vtab_cursor * cursor)
    {
        auto me = static_cast<typename Derived::cursor *>(cursor);

        static_assert(noexcept(me->eof()), "cursor's eof() must be noexcept");

        bool res = me->eof();
        return res;
    }

    template<class Derived>
    int vtab<Derived>::filter_impl(sqlite3_vtab_cursor * cursor, int idx_num, const char * idx_str,
                                   int argc, sqlite3_value ** argv)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            if constexpr (std::is_void_v<typename Derived::index_data_type>)
                me_cursor->filter(idx_num, argc, (value **)argv);
            else
                me_cursor->filter(idx_num, (typename Derived::index_data_type)idx_str, 
                                  argc, (value **)argv);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::next_impl(sqlite3_vtab_cursor * cursor)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            me_cursor->next();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::column_impl(sqlite3_vtab_cursor * cursor, sqlite3_context * ctxt, int n)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);
        auto myctxt = context::from(ctxt);

        SQLITEPP_BEGIN_CALLBACK
        {
            me_cursor->column(*myctxt, n);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::rowid_impl(sqlite3_vtab_cursor * cursor, sqlite_int64 * rowid)
    {
        auto me_cursor = static_cast<typename Derived::cursor *>(cursor);
        auto me = static_cast<Derived *>(cursor->pVtab);

        SQLITEPP_BEGIN_CALLBACK
        {
            *rowid = sqlite_int64(me_cursor->rowid());
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::update_impl(sqlite3_vtab * vtab, int argc, sqlite3_value ** argv, sqlite_int64 * rowid)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            *rowid = sqlite_int64(me->update(argc, (value **)argv));
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::find_function_impl(sqlite3_vtab * vtab, int n_arg, const char * name, 
                                          void (**func)(sqlite3_context*,int,sqlite3_value**), void ** args)
    {
        auto me = static_cast<Derived *>(vtab);
        return me->find_function(n_arg, name, (void (**)(context*,int,value**) noexcept)func, args);
    }
        
    template<class Derived>
    int vtab<Derived>::begin_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->begin();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK

    }

    template<class Derived>
    int vtab<Derived>::sync_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->sync();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK

    }

    template<class Derived>
    int vtab<Derived>::commit_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->commit();
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK

    }

    template<class Derived>
    int vtab<Derived>::rollback_impl(sqlite3_vtab * vtab)
    {
        auto me = static_cast<Derived *>(vtab);
        me->rollback();
        return SQLITE_OK;
    }

    template<class Derived>
    int vtab<Derived>::rename_impl(sqlite3_vtab * vtab, const char * new_name)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->rename(new_name);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::savepoint_impl(sqlite3_vtab * vtab, int point)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->savepoint(point);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::release_impl(sqlite3_vtab * vtab, int point )
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->release(point);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    template<class Derived>
    int vtab<Derived>::rollback_to_impl(sqlite3_vtab * vtab, int point)
    {
        auto me = static_cast<Derived *>(vtab);
        SQLITEPP_BEGIN_CALLBACK
        {
            me->rollback_to(point);
            return SQLITE_OK;
        }
        SQLITEPP_END_CALLBACK
    }

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)

        template<class Derived>
        int vtab<Derived>::shadow_name_impl(const char * name) 
        {
            return Derived::shadow_name(name);
        }
    #endif

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)

        template<class Derived>
        int vtab<Derived>::integrity_impl(sqlite3_vtab * vtab, const char * schema,
                                          const char * table_name, int flags, char ** err)
        {
            auto me = static_cast<Derived *>(vtab);
            SQLITEPP_BEGIN_CALLBACK
            {
                auto message =  me->integrity(schema, table_name, flags);
                *err = message.release();
                return SQLITE_OK;
            }
            SQLITEPP_END_CALLBACK
        }

    #endif

    

    #undef SQLITEPP_END_CALLBACK
    #undef SQLITEPP_BEGIN_CALLBACK

    #define SQLITEPP_SIMPLE_GET_IMPL(xname, name) \
    template<class Derived> \
    constexpr decltype(sqlite3_module::xname) vtab<Derived>::get_##name##_impl() \
    { \
        if constexpr (vtab_detector::has_##name<Derived>) \
            return name##_impl; \
        else \
            return nullptr; \
    }

    SQLITEPP_SIMPLE_GET_IMPL(xUpdate, update)
    SQLITEPP_SIMPLE_GET_IMPL(xFindFunction, find_function)
    SQLITEPP_SIMPLE_GET_IMPL(xBegin, begin)
    SQLITEPP_SIMPLE_GET_IMPL(xSync, sync)
    SQLITEPP_SIMPLE_GET_IMPL(xCommit, commit)
    SQLITEPP_SIMPLE_GET_IMPL(xRollback, rollback)
    SQLITEPP_SIMPLE_GET_IMPL(xRename, rename)
    SQLITEPP_SIMPLE_GET_IMPL(xSavepoint, savepoint)
    SQLITEPP_SIMPLE_GET_IMPL(xRelease, release)
    SQLITEPP_SIMPLE_GET_IMPL(xRollbackTo, rollback_to)

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 26, 0)
        SQLITEPP_SIMPLE_GET_IMPL(xShadowName, shadow_name)
    #endif

    #if SQLITE_VERSION_NUMBER >= SQLITEPP_SQLITE_VERSION(3, 44, 0)
        SQLITEPP_SIMPLE_GET_IMPL(xIntegrity, integrity)
    #endif

    #undef SQLITEPP_SIMPLE_GET_IMPL


    
}


#endif
