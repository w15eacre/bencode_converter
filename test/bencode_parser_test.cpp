#include <bencode_parser.h>
#include <concepts>

#include <gtest/gtest.h>

TEST(BencodeParserTest, BencodeConceptTest)
{
    ASSERT_TRUE(converter::bencode::type_traits::BencodeTypeConcept<converter::bencode::BaseTypeView>);
    ASSERT_TRUE(converter::bencode::type_traits::BencodeTypeConcept<converter::bencode::BaseType>);
}

TEST(BencodeParserTest, TypeTraitsConceptTest)
{
    ASSERT_TRUE(converter::bencode::type_traits::TypeTraitsConcept<
                converter::bencode::type_traits::BencodeTypeTraits<converter::bencode::BaseType>>);
    ASSERT_TRUE(converter::bencode::type_traits::TypeTraitsConcept<
                converter::bencode::type_traits::BencodeTypeTraits<converter::bencode::BaseTypeView>>);
}
