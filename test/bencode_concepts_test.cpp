#include <bencode_parser.h>

#include <gtest/gtest.h>

namespace bencode = converter::bencode;
namespace type_traits = bencode::type_traits;

TEST(BencodeParserTest, BencodeConcept)
{
    ASSERT_TRUE(type_traits::BencodeTypeConcept<bencode::BaseTypeView>);
    ASSERT_TRUE(type_traits::BencodeTypeConcept<bencode::BaseType>);
}

TEST(BencodeParserTest, TypeTraitsConcept)
{
    ASSERT_TRUE(type_traits::TypeTraitsConcept<type_traits::BencodeTypeTraits<bencode::BaseType>>);
    ASSERT_TRUE(type_traits::TypeTraitsConcept<type_traits::BencodeTypeTraits<bencode::BaseTypeView>>);
}

TEST(BencodeParserTest, TokenConcept)
{
    ASSERT_TRUE(
        (type_traits::TokenConcept<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::NoTypeToken>>));
    ASSERT_TRUE(
        (type_traits::TokenConcept<type_traits::Token<bencode::BaseTypeView, type_traits::BencodeTypeTraits<bencode::BaseType>::IntType>>));
    ASSERT_TRUE(
        (type_traits::TokenConcept<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::StrType>>));
    ASSERT_TRUE((
        type_traits::TokenConcept<type_traits::Token<bencode::BaseTypeView, type_traits::BencodeTypeTraits<bencode::BaseType>::ListType>>));
    ASSERT_TRUE((
        type_traits::TokenConcept<type_traits::Token<bencode::BaseTypeView, type_traits::BencodeTypeTraits<bencode::BaseType>::DictType>>));
}

TEST(BencodeParserTest, TokenTypeConcept)
{
    ASSERT_FALSE(
        (type_traits::HasTokenType<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::NoTypeToken>>));
    ASSERT_TRUE((
        type_traits::HasTokenValue<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::NoTypeToken>>));

    ASSERT_TRUE(
        (type_traits::HasTokenType<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::IntType>>));
    ASSERT_TRUE(
        (type_traits::HasTokenValue<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::IntType>>));

    ASSERT_TRUE(
        (type_traits::HasTokenType<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::StrType>>));
    ASSERT_FALSE(
        (type_traits::HasTokenValue<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::StrType>>));

    ASSERT_TRUE(
        (type_traits::HasTokenType<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::ListType>>));
    ASSERT_TRUE(
        (type_traits::HasTokenValue<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::ListType>>));

    ASSERT_TRUE(
        (type_traits::HasTokenType<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::DictType>>));
    ASSERT_TRUE(
        (type_traits::HasTokenValue<type_traits::Token<bencode::BaseType, type_traits::BencodeTypeTraits<bencode::BaseType>::DictType>>));
}
