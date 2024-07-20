// MIT License
//
// Copyright (c) 2024 Amil Khisamov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "sqlite_binding.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"
#include "editor/project_settings_editor.h"

#include <sqlite3.h>

[[nodiscard]] static sqlite3_stmt* prepare(sqlite3* db, const char* query) {
    ERR_FAIL_COND_V(db == nullptr, nullptr);
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    ERR_FAIL_COND_V(result != SQLITE_OK, nullptr);
    return stmt;
}

static bool bind_args(sqlite3_stmt* stmt, const Array& args) {
    const int param_count = sqlite3_bind_parameter_count(stmt);
    if (param_count != args.size()) {
        print_error("Failed to bind arguments [Wrong Count]: expected " + itos(param_count) +
            ", got " + itos(args.size()));
        return false;
    }

    for (int i = 0; i < param_count; ++i) {
        int result = SQLITE_OK;
        const Variant::Type type = args[i].get_type();
        switch (type) {
        case Variant::Type::PACKED_BYTE_ARRAY:
        {
            const PackedByteArray blob = args[i];
            result = sqlite3_bind_blob(stmt, i + 1, blob.ptr(), blob.size(), SQLITE_TRANSIENT);
            break;
        }
        case Variant::Type::FLOAT:
            result = sqlite3_bind_double(stmt, i + 1, static_cast<double>(args[i]));
            break;
        case Variant::Type::INT:
            result = sqlite3_bind_int(stmt, i + 1, static_cast<int>(args[i]));
            break;
        case Variant::Type::NIL:
            result = sqlite3_bind_null(stmt, i + 1);
            break;
        case Variant::Type::STRING:
            result = sqlite3_bind_text(stmt, i + 1, String(args[i]).utf8().get_data(), -1, SQLITE_TRANSIENT);
            break;
        default:
            print_error("Unsupported type: " + itos(type));
            return false;
        }

        if (result != SQLITE_OK) {
            print_error(
                "Failed to bind argument at [" + itos(i + 1) +
                "] with type " + itos(type) +
                ", error code = " + itos(result)
            );
            return false;
        }
    }
    return true;
}

[[nodiscard]] static Dictionary fetch_row(sqlite3_stmt* stmt) {
    Dictionary result;
    const int column_count = sqlite3_column_count(stmt);
    for (int i = 0; i < column_count; ++i) {
        const char* name = sqlite3_column_name(stmt, i);
        const int type = sqlite3_column_type(stmt, i);
        switch (type) {
        case SQLITE_INTEGER:
            result[name] = sqlite3_column_int(stmt, i);
            break;
        case SQLITE_FLOAT:
            result[name] = sqlite3_column_double(stmt, i);
            break;
        case SQLITE_TEXT:
            result[name] = String(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)));
            break;
        case SQLITE_BLOB:
        {
            PackedByteArray arr;
            const int size = sqlite3_column_bytes(stmt, i);
            arr.resize(size);
            memcpy((void*)arr.ptr(), sqlite3_column_blob(stmt, i), size);
            result[name] = arr;
            break;
        }
        case SQLITE_NULL:
            break;
        default:
            print_error("Unsupported column type: " + itos(type));
            return {};
        }
    }
    return result;
}

void SQLiteBinding::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open", "path"), &SQLiteBinding::open);
    ClassDB::bind_method(D_METHOD("close"), &SQLiteBinding::close);
    ClassDB::bind_method(D_METHOD("query", "query"), &SQLiteBinding::query);
    ClassDB::bind_method(D_METHOD("query_with_args", "query", "arguments"), &SQLiteBinding::query_with_args);
    ClassDB::bind_method(D_METHOD("query_fetch_rows", "query"), &SQLiteBinding::query_fetch_rows);
    ClassDB::bind_method(D_METHOD("query_fetch_rows_with_args", "query", "arguments"), &SQLiteBinding::query_fetch_rows_with_args);
}

SQLiteBinding::SQLiteBinding() = default;
SQLiteBinding::~SQLiteBinding() {
    if (db_ctx) {
        close();
    }
}

bool SQLiteBinding::open(const String& path) {
    if (!path.strip_edges().length()) {
        return false;
    }
    const String real_path = ProjectSettings::get_singleton()->globalize_path(path.strip_edges());
    if (sqlite3_open_v2(real_path.utf8().get_data(), &db_ctx,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        print_error("Failed to open database");
        return false;
    }
    return true;
}

bool SQLiteBinding::close() {
    if (!db_ctx) {
        print_error("Database is not opened");
        return false;
    }
    if (sqlite3_close(db_ctx) != SQLITE_OK) {
        print_error("Failed to close database");
        return false;
    }
    db_ctx = nullptr;
    return true;
}

bool SQLiteBinding::query(const String& query) {
    return query_with_args(query, Array());
}

bool SQLiteBinding::query_with_args(const String& query, const Array& arguments) {
    sqlite3_stmt* stmt = prepare(db_ctx, query.utf8().get_data());
    if (stmt == nullptr) {
        return false;
    }
    if (!bind_args(stmt, arguments)) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return true;
}

Array SQLiteBinding::query_fetch_rows(const String& query) {
    return query_fetch_rows_with_args(query, Array());
}

Array SQLiteBinding::query_fetch_rows_with_args(const String& query, const Array& arguments) {
    sqlite3_stmt* stmt = prepare(db_ctx, query.utf8().get_data());
    if (stmt == nullptr) {
        return {};
    }
    if (!bind_args(stmt, arguments)) {
        sqlite3_finalize(stmt);
        return {};
    }
    Array array;
    bool done = false;
    while (!done) {
        const int result = sqlite3_step(stmt);
        switch (result) {
        case SQLITE_ROW:
            array.push_back(fetch_row(stmt));
            break;
        case SQLITE_DONE:
            done = true;
            break;
        default:
            print_error("Unsupported step result: " + itos(result));
            return {};
        }
    }
    sqlite3_finalize(stmt);
    return array;
}
