#pragma once

#include "CommonTypes.hpp"

namespace Utilities
{
    class IteratorBase
    {
    public:
        IteratorBase operator ++ ()
        {
            ++CurrentIndex_;
            return *this;
        }

        bool const operator == (IteratorBase const kOther)
        {
            return CurrentIndex_ == kOther.CurrentIndex_;
        }

        bool const operator != (IteratorBase const kOther)
        {
            return CurrentIndex_ != kOther.CurrentIndex_;
        }

    protected:
        explicit IteratorBase(uint32 const kIndex)
        : CurrentIndex_ { kIndex }
        {
        }

    protected:
        uint32 CurrentIndex_;
    };
}