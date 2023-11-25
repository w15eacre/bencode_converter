#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <charconv>
#include <concepts>
#include <exception>
#include <map>
#include <variant>
#include <vector>

namespace converter::bencode {

template <typename... Args>
constexpr std::convertible_to<std::string_view> auto Format(fmt::format_string<Args...> pattern, Args&&... args)
{
    return fmt::format(pattern, std::forward<Args>(args)...);
}

template <std::forward_iterator It, typename T>
It FromChars(It begin, It end, T& result)
{
    std::string_view tmp{begin, end};
    auto [it, error] = std::from_chars(tmp.data(), tmp.data() + tmp.size(), result);
    if (error != std::errc())
    {
        throw std::system_error(std::make_error_code(error));
    }

    return std::next(begin, std::distance(&*begin, &*it));
}

namespace type_traits {

constexpr char InvalidSymbol = '\0';

template <typename T>
concept BencodeTypeConcept = requires(T bencode) {
    typename T::Int;
    typename T::Str;
    typename T::List;
    typename T::Dict;

    typename T::Variant;

    requires std::convertible_to<T, typename T::Variant>;
    {
        bencode.AsVariant()
    } -> std::convertible_to<typename T::Variant>;
};

template <typename T>
concept BencodeListConcept = requires(T list) {
    requires BencodeTypeConcept<typename T::value_type>;
    std::back_inserter(list);
};

template <typename T>
concept BencodeDictConcept = requires(T dict) {
    requires BencodeTypeConcept<typename T::mapped_type>;
    requires std::same_as<typename T::key_type, typename T::mapped_type::Str>;
    dict.insert(std::declval<typename T::value_type>());
};

template <typename Token>
concept HasTokenType = requires { typename Token::Type; };

template <typename Token>
concept HasTokenValue = requires(Token token) { requires std::same_as<std::remove_cvref_t<decltype(token.Token)>, char>; };

template <typename Token>
concept TokenConcept = requires {
    typename Token::BencodeType;

    std::declval<Token>() == InvalidSymbol;
    InvalidSymbol == std::declval<Token>();
};

template <typename TypeTraits>
concept TypeTraitsConcept = requires {
    {
        TypeTraits::GetIntToken()
    } -> TokenConcept;
    {
        TypeTraits::GetListToken()
    } -> TokenConcept;

    {
        TypeTraits::GetDictToken()
    } -> TokenConcept;

    {
        TypeTraits::GetSepToken()
    } -> TokenConcept;

    {
        TypeTraits::GetStrToken()
    } -> TokenConcept;
};

template <BencodeTypeConcept B, typename T>
struct Token
{
    using BencodeType = B;
    using Type = T;
    const char Token = InvalidSymbol;

    bool operator==(char ch) const noexcept
    {
        return ch == Token;
    }
};

template <BencodeTypeConcept BencodeType>
struct BencodeTypeTraits;

template <BencodeTypeConcept B>
struct Token<B, typename BencodeTypeTraits<B>::NoTypeToken>
{
    using BencodeType = B;
    const char Token = InvalidSymbol;

    bool operator==(char ch) const noexcept
    {
        return Token == ch;
    }
};

template <BencodeTypeConcept B>
struct Token<B, typename BencodeTypeTraits<B>::StrType>
{
    using BencodeType = B;
    using Type = std::string;

    bool operator==(char ch) const noexcept
    {
        return std::isdigit(ch);
    }
};

template <BencodeTypeConcept BencodeType>
struct BencodeTypeTraits
{
    using IntType = BencodeType::Int;
    using StrType = BencodeType::Str;
    using ListType = BencodeType::List;
    using DictType = BencodeType::Dict;
    using NoTypeToken = void;

    using Variant = std::variant<IntType, StrType, ListType, DictType>;

    constexpr static Token<BencodeType, NoTypeToken> GetEndToken() noexcept
    {
        return {'e'};
    }

    constexpr static Token<BencodeType, IntType> GetIntToken() noexcept
    {
        return {'i'};
    }

    constexpr static Token<BencodeType, ListType> GetListToken() noexcept
    {
        return {'l'};
    };

    constexpr static Token<BencodeType, DictType> GetDictToken() noexcept
    {
        return {'d'};
    }

    constexpr static Token<BencodeType, NoTypeToken> GetSepToken() noexcept
    {
        return {':'};
    }

    constexpr static Token<BencodeType, StrType> GetStrToken() noexcept
    {
        return {};
    }
};

} // namespace type_traits

namespace details {

template <typename I, typename S, template <typename...> typename L, template <typename...> typename D>
class BencodeType
{
public:
    using Int = I;
    using Str = S;
    using List = L<BencodeType>;
    using Dict = D<Str, BencodeType>;

    using Variant = std::variant<Int, Str, List, Dict>;

    template <typename T>
    explicit BencodeType(T&& value)
    {
        m_variant = std::forward<T>(value);
    }

    BencodeType(BencodeType::Variant&& variant) noexcept(std::is_nothrow_move_assignable_v<Variant>)
    {
        m_variant = std::move(variant);
    }

    operator Variant() const noexcept
    {
        return m_variant;
    }

    Variant AsVariant() const&
    {
        return m_variant;
    }

    Variant AsVariant() const&&
    {
        return std::move(m_variant);
    }
private:
    std::variant<Int, Str, List, Dict> m_variant{};
};

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseInt(It begin, It end)
{
    try
    {
        if (begin == end)
        {
            throw std::invalid_argument("The input value must not be empty.");
        }

        constexpr type_traits::TokenConcept auto IntToken = type_traits::BencodeTypeTraits<T>::GetIntToken();
        if (*begin != IntToken)
        {
            throw std::invalid_argument(Format("Expected the first letter '{}', actually: {}", IntToken.Token, *begin));
        }

        constexpr auto IsDigit = [](auto ch) -> bool {
            return std::isdigit(ch);
        };

        auto firstDigit = std::find_if(begin, end, IsDigit);
        if (firstDigit == end)
        {
            throw std::invalid_argument("Expected at least one digit, actualy - digit not found");
        }

        typename type_traits::BencodeTypeTraits<T>::IntType result{};
        auto it = FromChars(firstDigit, end, result);
        if (it == end || *it != type_traits::BencodeTypeTraits<T>::GetEndToken())
        {
            throw std::invalid_argument("The int ended unexpectedly");
        }

        return std::make_pair(std::next(it), T{std::move(result)});
    }
    catch (...)
    {
        std::throw_with_nested(
            std::runtime_error(Format("{}:{}:Failed to parse int value: {}", __FUNCTION__, __LINE__, std::string_view(begin, end))));
    }
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseString(It begin, It end)
{
    try
    {
        if (begin == end)
        {
            throw std::invalid_argument("The input value must not be empty.");
        }

        size_t sizeOf{};
        auto sepIt = FromChars(begin, end, sizeOf);

        if (sepIt == end)
        {
            throw std::invalid_argument("Separate token not found");
        }

        constexpr type_traits::TokenConcept auto SepToken = type_traits::BencodeTypeTraits<T>::GetSepToken();
        if (*sepIt != SepToken)
        {
            throw std::invalid_argument(Format("Expected '{}' token, actually: {}", SepToken.Token, *sepIt));
        }

        auto endIt = std::next(sepIt, sizeOf + 1);
        if (endIt > end)
        {
            throw std::invalid_argument(Format("Incomplete payload"));
        }

        return std::make_pair(endIt, typename type_traits::BencodeTypeTraits<T>::StrType{std::next(sepIt), endIt});
    }
    catch (...)
    {
        std::throw_with_nested(
            std::runtime_error(Format("{}:{}:Failed to parse string value: {}", __FUNCTION__, __LINE__, std::string_view(begin, end))));
    }
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> Parse(It begin, It end);

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseList(It begin, It end)
{
    try
    {
        if (begin == end)
        {
            throw std::invalid_argument("The input value must not be empty.");
        }

        constexpr type_traits::TokenConcept auto ListToken = type_traits::BencodeTypeTraits<T>::GetListToken();
        if (*begin != ListToken)
        {
            throw std::invalid_argument(
                Format("Expected the first letter '{}', actually {}", ListToken.Token, std::string_view(begin, end)));
        }

        type_traits::BencodeListConcept auto list = typename type_traits::BencodeTypeTraits<T>::ListType{};

        auto outputIt = std::back_inserter(list);

        auto it = std::next(begin);
        if (it == end)
        {
            throw std::invalid_argument("The list ended unexpectedly");
        }

        constexpr type_traits::TokenConcept auto EndToken = type_traits::BencodeTypeTraits<T>::GetEndToken();
        while (*it != EndToken)
        {
            auto [endIt, value] = Parse<T>(it, end);
            *outputIt = std::move(value);
            it = endIt;
        }

        return std::make_pair(end, T{list});
    }
    catch (...)
    {
        std::throw_with_nested(
            std::runtime_error(Format("{}:{}:Failed to parse list value: {}", __FUNCTION__, __LINE__, std::string_view(begin, end))));
    }
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseDict(It begin, It end)
{
    try
    {
        if (begin == end)
        {
            throw std::invalid_argument("The input value must not be empty.");
        }

        constexpr type_traits::TokenConcept auto DictToken = type_traits::BencodeTypeTraits<T>::GetDictToken();
        if (*begin != DictToken)
        {
            throw std::invalid_argument(
                Format("Expected the first letter '{}', actually {}", DictToken.Token, std::string_view(begin, end)));
        }

        type_traits::BencodeDictConcept auto dict = typename type_traits::BencodeTypeTraits<T>::DictType{};

        constexpr type_traits::TokenConcept auto EndToken = type_traits::BencodeTypeTraits<T>::GetEndToken();

        std::advance(begin, 1);
        while (*begin != EndToken)
        {
            const auto [keyEndIt, keyVariant] = ParseString<T>(begin, end);
            const auto [valueEndIt, valueVariant] = Parse<T>(keyEndIt, end);

            dict.insert(std::pair<typename type_traits::BencodeTypeTraits<T>::StrType, T>(
                std::get<typename type_traits::BencodeTypeTraits<T>::StrType>(keyVariant), valueVariant));

            begin = valueEndIt;
        }

        return std::make_pair(std::next(begin), std::move(dict));
    }
    catch (...)
    {
        std::throw_with_nested(
            std::runtime_error(Format("{}:{}:Failed to parse dict value: {}", __FUNCTION__, __LINE__, std::string_view(begin, end))));
    }
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> Parse(It begin, It end)
{
    try
    {
        if (begin == end)
        {
            throw std::invalid_argument("The input value must not be empty.");
        }

        if (*begin == type_traits::BencodeTypeTraits<T>::GetIntToken())
        {
            return ParseInt<T>(begin, end);
        }

        if (*begin == type_traits::BencodeTypeTraits<T>::GetListToken())
        {
            return ParseList<T>(begin, end);
        }

        if (*begin == type_traits::BencodeTypeTraits<T>::GetDictToken())
        {
            return ParseDict<T>(begin, end);
        }

        if (*begin == type_traits::BencodeTypeTraits<T>::GetStrToken())
        {
            return ParseString<T>(begin, end);
        }

        throw std::invalid_argument(Format("Unexpected symbol: {}", *begin));
    }
    catch (...)
    {
        std::throw_with_nested(
            std::runtime_error(Format("{}:{}:Failed to parse Bencode value: {}", __FUNCTION__, __LINE__, std::string_view(begin, end))));
    }
}

} // namespace details

using BaseType = details::BencodeType<int64_t, std::string, std::vector, std::map>;
using BaseTypeView = details::BencodeType<int64_t, std::string_view, std::vector, std::map>;
using BenCodeVariant = type_traits::BencodeTypeTraits<BaseType>::Variant;
using BenCodeVariantView = type_traits::BencodeTypeTraits<BaseTypeView>::Variant;

template <type_traits::BencodeTypeConcept T>
type_traits::BencodeTypeTraits<T>::Variant Parse(std::string_view data)
{
    auto [it, value] = details::Parse<T>(std::cbegin(data), std::cend(data));
    if (it != std::cend(data))
    {
        throw std::runtime_error(
            Format("Failed to parse bencode value: Unparsed data starts with {}: {}", std::distance(data.cbegin(), it), data));
    }

    return value;
}

} // namespace converter::bencode
