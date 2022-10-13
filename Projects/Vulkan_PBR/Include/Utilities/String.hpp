#pragma once

#include "Platform/Windows.hpp"
#include "CommonTypes.hpp"

#include <string>

class String
{
public:
    String()
        : String_()
    {
    }

    String(std::basic_string<Platform::Windows::Types::Char> StandardString)
        : String_(std::move(StandardString))
    {
    }

    String(Platform::Windows::Types::Char const * CString)
        : String_(CString)
    {
    }

    template<typename ... TArguments>
    static auto Format(Platform::Windows::Types::Char const * FormatString, TArguments ... Arguments) -> String
    {
        String FormattedString = {};

        if constexpr (std::is_same<Platform::Windows::Types::Char, char>())
        {
            int SizeInBytes = std::snprintf(nullptr, 0u, FormatString, Arguments...);
            SizeInBytes++;

            FormattedString.Resize(SizeInBytes);
            std::snprintf(FormattedString.Data(), FormattedString.Length(), FormatString, Arguments...);
        }
        else if constexpr (std::is_same<Platform::Windows::Types::Char, wchar_t>())
        {
            int const SizeInBytes = std::swprintf(nullptr, 0u, FormatString, Arguments...);
            SizeInBytes += sizeof(Platform::Windows::TCHAR);

            FormattedString.Resize(SizeInBytes / sizeof(Platform::Windows::TCHAR));
            std::swprintf(FormattedString.Data(), FormattedString.Length(), FormatString, Arguments...);
        }

        return FormattedString;
    }

    inline void Resize(uint64 NewSizeInCharacters)
    {
        String_.resize(NewSizeInCharacters);
    }

    inline auto Length() const -> uint32 const
    {
        return static_cast<uint32>(String_.size());
    }

    inline auto Data() -> Platform::Windows::Types::Char * const
    {
        return String_.data();
    }

    inline auto Data() const -> Platform::Windows::Types::Char const * const
    {
        return String_.data();
    }
private:
    std::basic_string<Platform::Windows::Types::Char> String_ = {};
};
