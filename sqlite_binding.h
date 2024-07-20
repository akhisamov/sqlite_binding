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

#pragma once

#include "core/object/ref_counted.h"

struct sqlite3;

class SQLiteBinding : public RefCounted {
    GDCLASS(SQLiteBinding, RefCounted);

    sqlite3* db_ctx = nullptr;

protected:
    static void _bind_methods();

public:
    SQLiteBinding();
    ~SQLiteBinding();

    bool open(const String& path);
    bool close();

    bool query(const String& query);
    bool query_with_args(const String& query, const Array& arguments);
    Array query_fetch_rows(const String& query);
    Array query_fetch_rows_with_args(const String& query, const Array& arguments);
};
