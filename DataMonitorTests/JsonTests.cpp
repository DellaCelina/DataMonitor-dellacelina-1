#include "datamonitor/Json.h"

#include "TestFramework.h"

using datamonitor::JsonValue;

TEST(Json_ParsesLiterals) {
    CHECK(JsonValue::Parse("null").IsNull());
    CHECK(JsonValue::Parse("true").AsBool() == true);
    CHECK(JsonValue::Parse("false").AsBool() == false);
}

TEST(Json_ParsesNumbers) {
    CHECK_EQ(JsonValue::Parse("42").AsDouble(), 42.0);
    CHECK_EQ(JsonValue::Parse("-17").AsDouble(), -17.0);
    CHECK_EQ(JsonValue::Parse("3.14").AsDouble(), 3.14);
    CHECK_EQ(JsonValue::Parse("-0.5").AsDouble(), -0.5);
    CHECK_EQ(JsonValue::Parse("1e3").AsDouble(), 1000.0);
    CHECK_EQ(JsonValue::Parse("2.5E2").AsDouble(), 250.0);
}

TEST(Json_ParsesStringsWithEscapes) {
    JsonValue v = JsonValue::Parse(R"("hello\nworld\t\"quoted\"\\slash")");
    CHECK_EQ(v.AsString(), std::string("hello\nworld\t\"quoted\"\\slash"));
}

TEST(Json_ParsesUnicodeEscape) {
    // 가 is the Korean syllable "가".
    JsonValue v = JsonValue::Parse(R"("가나")");
    CHECK_EQ(v.AsString(), std::string("가나"));
}

TEST(Json_ParsesArraysAndObjectsNested) {
    JsonValue v = JsonValue::Parse(R"({"a":[1,2,3],"b":{"c":true,"d":null},"e":"text"})");
    CHECK(v.IsObject());
    CHECK_EQ(v["a"].AsArray().size(), static_cast<size_t>(3));
    CHECK_EQ(v["a"][0].AsDouble(), 1.0);
    CHECK(v["b"]["c"].AsBool() == true);
    CHECK(v["b"]["d"].IsNull());
    CHECK_EQ(v["e"].AsString(), std::string("text"));
}

TEST(Json_RoundTripsObject) {
    JsonValue original = JsonValue::MakeObject();
    original["name"] = JsonValue(std::string("샘플-A"));
    original["count"] = JsonValue(7);
    original["ratio"] = JsonValue(0.925);
    original["active"] = JsonValue(true);
    original["nothing"] = JsonValue(nullptr);
    JsonValue::Array tags;
    tags.push_back(JsonValue(std::string("x")));
    tags.push_back(JsonValue(std::string("y\"z")));
    original["tags"] = JsonValue(tags);

    std::string dumped = original.Dump(true);
    JsonValue reparsed = JsonValue::Parse(dumped);
    CHECK(reparsed == original);
}

TEST(Json_RoundTripsArrayOfObjects) {
    JsonValue::Array array;
    for (int i = 0; i < 3; ++i) {
        JsonValue obj = JsonValue::MakeObject();
        obj["id"] = JsonValue(std::to_string(i));
        obj["value"] = JsonValue(i * 1.5);
        array.push_back(obj);
    }
    JsonValue original(array);
    JsonValue reparsed = JsonValue::Parse(original.Dump(false));
    CHECK(reparsed == original);
}

TEST(Json_EmptyArrayAndObjectRoundTrip) {
    JsonValue emptyArray = JsonValue::MakeArray();
    JsonValue emptyObject = JsonValue::MakeObject();
    CHECK(JsonValue::Parse(emptyArray.Dump()) == emptyArray);
    CHECK(JsonValue::Parse(emptyObject.Dump()) == emptyObject);
}

TEST(Json_TypeMismatchThrows) {
    JsonValue number(5.0);
    CHECK_THROWS(number.AsString());
    CHECK_THROWS(number.AsArray());
    CHECK_THROWS(number.AsObject());

    JsonValue str(std::string("x"));
    CHECK_THROWS(str.AsDouble());
    CHECK_THROWS(str.AsBool());
}

TEST(Json_MalformedInputThrows) {
    CHECK_THROWS(JsonValue::Parse("{"));
    CHECK_THROWS(JsonValue::Parse("[1,2"));
    CHECK_THROWS(JsonValue::Parse("nul"));
    CHECK_THROWS(JsonValue::Parse(""));
    CHECK_THROWS(JsonValue::Parse("{\"a\":}"));
}

TEST(Json_MergeFromOverwritesOnlyGivenFields) {
    JsonValue base = JsonValue::MakeObject();
    base["a"] = JsonValue(1);
    base["b"] = JsonValue(std::string("keep"));

    JsonValue patch = JsonValue::MakeObject();
    patch["a"] = JsonValue(99);

    base.MergeFrom(patch);
    CHECK_EQ(base["a"].AsDouble(), 99.0);
    CHECK_EQ(base["b"].AsString(), std::string("keep"));
}
