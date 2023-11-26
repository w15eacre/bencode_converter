#include <bencode_parser.h>
#include <config.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>

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

template <typename Dict, typename T>
void CheckDictItem(Dict& dict, std::string_view key, const T& value)
{
    auto it = dict.find(std::string{key}); // FIX ME AFTER Transparancy
    ASSERT_NE(it, std::cend(dict));
    const auto [k, v] = *it;
    ASSERT_EQ(k, key);
    ASSERT_EQ(std::get<T>(v.AsVariant()), value);
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

TEST(BencodeParserTest, ParseStringWhenParamWithoutPayload)
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
    auto parsedValue = bencode::Parse<bencode::BaseType>(TestList);
    ASSERT_FALSE(parsedValue.valueless_by_exception());

    ASSERT_EQ(
        (bencode::GetHomogeneousList<bencode::BaseType::Str, bencode::BaseType>(parsedValue)),
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

TEST(BencodeParserTest, ParseDict)
{
    constexpr std::string_view TestDict = "d4:name5:cream5:pricei100ee";

    const auto [it, value] = bencode::details::ParseDict<bencode::BaseType>(std::cbegin(TestDict), std::cend(TestDict));
    ASSERT_EQ(it, std::cend(TestDict));
    auto dict = std::get<bencode::BaseType::Dict>(value);
    CheckDictItem(dict, "name", std::string{"cream"});
    CheckDictItem(dict, "price", int64_t{100});
}

TEST(BencodeParserTest, ParseDictWhenParamsIsEmpty)
{
    constexpr std::string_view TestDict{};

    ASSERT_ANY_THROW(bencode::details::ParseDict<bencode::BaseType>(std::cbegin(TestDict), std::cend(TestDict)));
}

TEST(BencodeParserTest, ParseDictWhenParamWithoutPrefix)
{
    constexpr std::string_view TestDict = "4:name5:cream5:pricei100ee";

    ASSERT_ANY_THROW(bencode::details::ParseDict<bencode::BaseType>(std::cbegin(TestDict), std::cend(TestDict)));
}

TEST(BencodeParserTest, ParseDictWhenParamWithoutPostfix)
{
    constexpr std::string_view TestDict = "d4:name5:cream5:pricei100e";

    ASSERT_ANY_THROW(bencode::details::ParseDict<bencode::BaseType>(std::cbegin(TestDict), std::cend(TestDict)));
}

TEST(BencodeParserTest, ParseDictWhenInvalidKeyType)
{
    constexpr std::string_view TestDict = "d4:name5:creami23ei100ee";

    ASSERT_ANY_THROW(bencode::details::ParseDict<bencode::BaseType>(std::cbegin(TestDict), std::cend(TestDict)));
}

TEST(BencodeParserTest, ParseTorrentFile)
{
    const static std::filesystem::path TorrentFilePath{std::filesystem::path{test::config::ResourcesPath} / "sample.torrent"};
    ASSERT_TRUE(std::filesystem::exists(TorrentFilePath));
    ASSERT_TRUE(std::filesystem::is_regular_file(TorrentFilePath));

    std::ifstream torrentFile(TorrentFilePath, std::ios_base::in);
    ASSERT_TRUE(torrentFile.is_open());

    std::stringstream data;
    data << torrentFile.rdbuf();
    const auto& torrentFileData = data.str();

    const auto [it, value] = bencode::details::ParseDict<bencode::BaseType>(std::cbegin(torrentFileData), std::cend(torrentFileData));
    ASSERT_EQ(it, std::cend(torrentFileData));
}

TEST(BencodeParserTest, FromChars)
{
    constexpr std::string_view IntStr = "09251995";

    size_t result{};
    auto it = bencode::FromChars(std::cbegin(IntStr), std::cend(IntStr), result);
    ASSERT_EQ(it, std::cend(IntStr));

    ASSERT_EQ(result, 9251995);
}

TEST(BencodeParserTest, FromCharsWhenParamIsEmpty)
{
    constexpr std::string_view IntStr{};

    size_t result{};
    ASSERT_THROW(bencode::FromChars(std::cbegin(IntStr), std::cend(IntStr), result), std::system_error);
}

TEST(BencodeParserTest, FromCharsWhenParamIsInvalid)
{
    constexpr std::string_view IntStr = "0925a1995";

    size_t result{};
    auto it = bencode::FromChars(std::cbegin(IntStr), std::cend(IntStr), result);
    ASSERT_EQ(it, std::find_if(std::cbegin(IntStr), std::cend(IntStr), [](auto ch) {
                  return !std::isdigit(ch);
              }));

    ASSERT_EQ(result, 925);
}
