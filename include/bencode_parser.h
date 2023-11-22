#pragma once

#include <algorithm>
#include <cctype>
#include <concepts>
#include <exception>
#include <format>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace converter::bencode {

namespace details {

constexpr char InvalidSymbol = '\0';

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
private:
    std::variant<Int, Str, List, Dict> m_variant{};
};

} // namespace details

namespace type_traits {

template <typename T>
concept BencodeTypeConcept = requires(T bencode) {
    typename T::Int;
    typename T::Str;
    typename T::List;
    typename T::Dict;

    typename T::Variant;

    requires std::convertible_to<T, typename T::Variant>;
};

template <typename Token>
concept HasTokenType = requires { typename Token::Type; };

template <typename Token>
concept HasTokenValue = requires(Token token) { requires std::same_as<decltype(token.token), char>; };

template <typename Token>
concept TokenConcept = requires {
    typename Token::BencodeType;

    std::declval<Token>() == details::InvalidSymbol;
    details::InvalidSymbol == std::declval<Token>();
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
    const char token = details::InvalidSymbol;

    bool operator==(char ch) const noexcept
    {
        return ch == token;
    }
};

template <BencodeTypeConcept BencodeType>
struct BencodeTypeTraits;

template <BencodeTypeConcept B>
struct Token<B, typename BencodeTypeTraits<B>::NoTypeToken>
{
    using BencodeType = B;
    const char token = details::InvalidSymbol;

    bool operator==(char ch) const noexcept
    {
        return token == ch;
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

    static Token<BencodeType, NoTypeToken> GetEndToken() noexcept
    {
        return {'e'};
    }

    static Token<BencodeType, IntType> GetIntToken() noexcept
    {
        return {'i'};
    }

    static Token<BencodeType, ListType> GetListToken() noexcept
    {
        return {'l'};
    };

    static Token<BencodeType, DictType> GetDictToken() noexcept
    {
        return {'d'};
    }

    static Token<BencodeType, NoTypeToken> GetSepToken() noexcept
    {
        return {':'};
    }

    static Token<BencodeType, StrType> GetStrToken() noexcept
    {
        return {};
    }
};

} // namespace type_traits

namespace details {
template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseInt(It begin, It end)
{
    if (*begin != type_traits::BencodeTypeTraits<T>::GetIntToken())
    {
        throw std::invalid_argument(std::format("Expected the bencode format: i<number>e, actualy - {}", std::string_view(begin, end)));
    }

    constexpr auto IsDigit = [](auto ch) -> bool {
        return std::isdigit(ch);
    };

    auto firstDigit = std::find_if(begin, end, IsDigit);
    if (firstDigit == end)
    {
        throw std::invalid_argument(
            std::format("Expected at least one digit, actualy - digit not found - {}", std::string_view(begin, end)));
    }

    typename type_traits::BencodeTypeTraits<T>::IntType result{};
    auto [it, error] = std::from_chars(firstDigit, end, result);
    if (error == std::errc() && it != end && *it == type_traits::BencodeTypeTraits<T>::GetEndToken())
    {
        return std::make_pair(std::next(it), T{std::move(result)});
    }

    throw std::system_error(static_cast<int>(error), std::system_category());
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseString(It begin, It end)
{
    size_t sizeOf{};
    auto [sepIt, error] = std::from_chars(begin, end, sizeOf);

    if (error != std::errc())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }

    if (*sepIt != type_traits::BencodeTypeTraits<T>::GetSepToken())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }

    auto endIt = std::next(sepIt, sizeOf + 1);

    return std::make_pair(endIt, typename type_traits::BencodeTypeTraits<T>::StrType{std::next(sepIt), endIt});
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> Parse(It begin, It end);

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> ParseList(It begin, It end)
{
    if (*begin != type_traits::BencodeTypeTraits<T>::GetListToken())
    {
        throw std::invalid_argument(std::format("Expected the bencode format: i<number>e, actualy - {}", std::string_view(begin, end)));
    }

    typename type_traits::BencodeTypeTraits<T>::ListType list{};

    auto outputIt = std::back_inserter(list);

    auto it = std::next(begin);
    while (*it != type_traits::BencodeTypeTraits<T>::GetEndToken())
    {
        auto [endIt, value] = Parse<T>(it, end);
        *outputIt = std::move(value);
        it = endIt;
    }

    return std::make_pair(end, T{list});
}

template <type_traits::BencodeTypeConcept T, std::forward_iterator It>
std::pair<It, typename type_traits::BencodeTypeTraits<T>::Variant> Parse(It begin, It end)
{
    if (*begin == type_traits::BencodeTypeTraits<T>::GetIntToken())
    {
        return ParseInt<T>(begin, end);
    }
    else if (*begin == type_traits::BencodeTypeTraits<T>::GetListToken())
    {
        return ParseList<T>(begin, end);
    }
    else if (*begin == type_traits::BencodeTypeTraits<T>::GetDictToken())
    {
        std::terminate();
    }
    else if (*begin == type_traits::BencodeTypeTraits<T>::GetStrToken())
    {
        return ParseString<T>(begin, end);
    }

    throw std::invalid_argument(std::format("Unexpected symbol - {}", *begin));
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
        throw std::invalid_argument(std::format("Failed to parse bencode: ", data));
    }

    return value;
}

} // namespace converter::bencode
