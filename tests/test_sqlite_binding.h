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

#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "tests/test_macros.h"
#include "modules/sqlite_binding/sqlite_binding.h"
#include <map>

namespace TestSQLiteBinding {

[[nodiscard]] static Dictionary create_dict(const std::map<String, Variant>& map) {
    Dictionary result;
    for (const auto& [key, value] : map) {
        result[key] = value;
    }
    return result;
}

TEST_CASE("[Modules][SQLiteBinding]") {
    Ref<SQLiteBinding> sqlite = memnew(SQLiteBinding);
    CHECK(sqlite->open("demo.sqlite"));

    String query =  "CREATE TABLE `fruits` "
                    "(`name` varchar(50) NOT NULL,"
                    "`price` int(11) NOT NULL,"
                    "PRIMARY KEY (`name`))";
    CHECK(sqlite->query(query));

    query = "INSERT INTO fruits VALUES (?, ?)";
    Array fruits_to_insert;
    fruits_to_insert.push_back(create_dict({{"name", "apple"}, {"price", 14}}));
    fruits_to_insert.push_back(create_dict({{"name", "orange"}, {"price", 30}}));
    fruits_to_insert.push_back(create_dict({{"name", "banana"}, {"price", 42}}));
    CHECK(sqlite->query_with_args(query, Dictionary(fruits_to_insert[0]).values()));
    CHECK(sqlite->query_with_args(query, Dictionary(fruits_to_insert[1]).values()));
    CHECK(sqlite->query_with_args(query, Dictionary(fruits_to_insert[2]).values()));

    query = "SELECT * FROM fruits";
    CHECK(sqlite->query_fetch_rows(query) == fruits_to_insert);

    query = "SELECT * FROM fruits WHERE price < ?";
    {
        Array args;
        args.push_back(15);
        Array answer;
        answer.push_back(create_dict({{"name", "apple"}, {"price", 14}}));
        CHECK(sqlite->query_fetch_rows_with_args(query, args) == answer);
    }

    query = "DROP TABLE IF EXISTS fruits";
    CHECK(sqlite->query(query));
    CHECK(sqlite->close());
}

}
