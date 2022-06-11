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

    String(std::basic_string<Platform::Windows::TCHAR> StandardString)
        : String_(std::move(StandardString))
    {
    }

    String(Platform::Windows::TCHAR const * CString)
        : String_(CString)
    {
    }

    template<typename ... TArguments>
    static auto Format(Platform::Windows::TCHAR const * FormatString, TArguments ... Arguments) -> String
    {
        String FormattedString = {};

        if constexpr (std::is_same<Platform::Windows::TCHAR, char>())
        {
            int SizeInBytes = std::snprintf(nullptr, 0u, FormatString, Arguments...);
            SizeInBytes++;

            FormattedString.Resize(SizeInBytes);
            std::snprintf(FormattedString.Data(), FormattedString.Length(), FormatString, Arguments...);
        }
        else if constexpr (std::is_same<Platform::Windows::TCHAR, wchar_t>())
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

    inline auto Data() -> Platform::Windows::TCHAR * const
    {
        return String_.data();
    }

    inline auto Data() const -> Platform::Windows::TCHAR const * const
    {
        return String_.data();
    }
private:
    std::basic_string<Platform::Windows::TCHAR> String_ = {};
};
