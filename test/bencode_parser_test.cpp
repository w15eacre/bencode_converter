#include <bencode_parser.h>

#include <algorithm>

#include <gtest/gtest.h>

namespace bencode = converter::bencode;
namespace type_traits = bencode::type_traits;

namespace {

template <type_traits::BencodeTypeConcept BaseType, typename O>
std::vector<O> ParseHomogeneousList(std::string_view data)
{
    const auto [it, variant] = bencode::details::ParseList<BaseType>(std::cbegin(data), std::cend(data));
    [it, data]() {
        ASSERT_EQ(it, std::cend(data));
        }();

    auto list = std::get<typename type_traits::BencodeTypeTraits<BaseType>::ListType>(variant);

    std::vector<O> result(list.size());
    std::ranges::transform(list, std::begin(result), [](const type_traits::BencodeTypeTraits<BaseType>::Variant& value) {
        return std::get<O>(value);
    });

    return result;
}

template <type_traits::BencodeTypeConcept BaseType>
type_traits::BencodeTypeTraits<BaseType>::StrType ParseString(std::string_view data)
{
    const auto [it, variant] = bencode::details::ParseString<BaseType>(std::cbegin(data), std::cend(data));
    [it, data]() {
        ASSERT_EQ(it, std::cend(data));
        }();

    return std::get<typename type_traits::BencodeTypeTraits<BaseType>::StrType>(variant);
}

} // namespace

TEST(BencodeParserTest, ParseInt)
{
    constexpr std::string_view TestInt = "i9123e";
    const auto [it, variant] = bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestInt), std::cend(TestInt));
    ASSERT_EQ(it, std::cend(TestInt));
    ASSERT_EQ(std::get<type_traits::BencodeTypeTraits<bencode::BaseType>::IntType>(variant), 9123);
}

TEST(BencodeParserTest, ParseIntWhenParamIsEmpty)
{
    constexpr std::string_view TestInt = "";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestInt), std::cend(TestInt)));
}

TEST(BencodeParserTest, ParseIntWhenParamWithoutPrefix)
{
    constexpr std::string_view TestInt = "9123e";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestInt), std::cend(TestInt)));
}

TEST(BencodeParserTest, ParseIntWhenParamWithoutPostfix)
{
    constexpr std::string_view TestInt = "i9123";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestInt), std::cend(TestInt)));
}

TEST(BencodeParserTest, ParseIntWhenParamWithoutDigit)
{
    constexpr std::string_view TestInt = "ie";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestInt), std::cend(TestInt)));
}

TEST(BencodeParserTest, ParseIntWhenInvalidParam)
{
    constexpr std::string_view TestInt = "i9423ade";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestInt), std::cend(TestInt)));

    constexpr std::string_view TestStr = "12:Hello world!";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestStr), std::cend(TestStr)));

    constexpr std::string_view TestList = "l5:jelly4:cake7:custarde";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestList), std::cend(TestList)));

    constexpr std::string_view TestDict = "d4:name5:cream5:pricei100ee";
    ASSERT_ANY_THROW(bencode::details::ParseInt<bencode::BaseType>(std::cbegin(TestDict), std::cend(TestDict)));
}

TEST(BencodeParserTest, ParseString)
{
    constexpr std::string_view TestStr = "12:hello world!";

    ASSERT_EQ(ParseString<bencode::BaseType>(TestStr), "hello world!");
    ASSERT_EQ(ParseString<bencode::BaseTypeView>(TestStr), "hello world!");
}

TEST(BencodeParserTest, ParseStringWhenParamIsEmpty)
{
    constexpr std::string_view TestStr = "";
    ASSERT_ANY_THROW(bencode::details::ParseString<bencode::BaseType>(std::cbegin(TestStr), std::cend(TestStr)));
}

TEST(BencodeParserTest, ParseStringWhenParamWithoutSize)
{
    constexpr std::string_view TestStr = "Hello World!";
    ASSERT_ANY_THROW(bencode::details::ParseString<bencode::BaseType>(std::cbegin(TestStr), std::cend(TestStr)));
}

TEST(BencodeParserTest, ParseStringWhenParamWithoutSeparator)
{
    constexpr std::string_view TestStr = "12Hello World!";
    ASSERT_ANY_THROW(bencode::details::ParseString<bencode::BaseType>(std::cbegin(TestStr), std::cend(TestStr)));
}

TEST(BencodeParserTest, ParseStringWhenInputWithoutPayload)
{
    constexpr std::string_view TestStr = "12";
    ASSERT_ANY_THROW(bencode::details::ParseString<bencode::BaseType>(std::cbegin(TestStr), std::cend(TestStr)));
}

TEST(BencodeParserTest, ParseStringWhenPayloadCorrupted)
{
    constexpr std::string_view PayloadLessExpected = "12:abcde";
    ASSERT_ANY_THROW(bencode::details::ParseString<bencode::BaseType>(std::cbegin(PayloadLessExpected), std::cend(PayloadLessExpected)));
}

TEST(BencodeParserTest, ParseStringList)
{
    constexpr std::string_view TestList = "l5:jelly4:cake7:custarde";

    ASSERT_EQ(
        (ParseHomogeneousList<bencode::BaseType, bencode::BaseType::Str>(TestList)),
        (std::vector<bencode::BaseType::Str>{"jelly", "cake", "custard"}));
}

TEST(BencodeParserTest, ParseIntList)
{
    constexpr std::string_view TestList = "li1ei2ei3ee";

    ASSERT_EQ((ParseHomogeneousList<bencode::BaseType, bencode::BaseType::Int>(TestList)), (std::vector<bencode::BaseType::Int>{1, 2, 3}));
}

TEST(BencodeParserTest, ParseListWhenParamIsEmpty)
{
    constexpr std::string_view TestList = "";
    ASSERT_ANY_THROW(bencode::details::ParseList<bencode::BaseType>(std::cbegin(TestList), std::cend(TestList)));
}

TEST(BencodeParserTest, ParseListWhenParamWithoutListPrefix)
{
    constexpr std::string_view TestList = "5:jelly4:cake7:custarde";
    ASSERT_ANY_THROW(bencode::details::ParseList<bencode::BaseType>(std::cbegin(TestList), std::cend(TestList)));
}

TEST(BencodeParserTest, ParseListWhenParamWithoutListPostfix)
{
    constexpr std::string_view TestList = "l5:jelly4:cake7:custard";
    ASSERT_ANY_THROW(bencode::details::ParseList<bencode::BaseType>(std::cbegin(TestList), std::cend(TestList)));
}

TEST(BencodeParserTest, ParseListWhenInvalidParam)
{
    constexpr std::string_view TestList = "l5:jelly4:cake8:custard";
    ASSERT_ANY_THROW(bencode::details::ParseList<bencode::BaseType>(std::cbegin(TestList), std::cend(TestList)));
}
