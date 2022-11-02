#pragma once

#include "CommonTypes.hpp"

#include <vector>

template<class TDataType>
class SparseVector
{
    union Node
    {
        TDataType Value = {};

        struct
        {
            uint32 NextFreeIndex = {};
        } Internal;
    };

public:
    SparseVector() = default;

    uint32 Add(TDataType const & kData)
    {
        uint32 DataIndex = {};

        if (FreeSlotCount_ > 0u)
        {
            DataIndex = FirstFreeSlotIndex_;
            FirstFreeSlotIndex_ = Data_ [FirstFreeSlotIndex_].Internal.NextFreeIndex;

            Data_ [DataIndex].Value = kData;
            FreeSlotFlags_ [DataIndex] = false;

            FreeSlotCount_--;
        }
        else
        {
            DataIndex = static_cast<uint32>(Data_.size());

            Data_.emplace_back(Node { kData });
            FreeSlotFlags_.push_back(false);
        }

        return DataIndex;
    }

    void Remove(uint32 const kIndex)
    {
        if (FreeSlotCount_ == 0u)
        {
            FirstFreeSlotIndex_ = kIndex;
        }
        else
        {
            Data_ [LastFreeSlotIndex_].Internal.NextFreeIndex = kIndex;
        }

        Data_ [kIndex].Internal.NextFreeIndex = FirstFreeSlotIndex_;

        LastFreeSlotIndex_ = kIndex;
        FreeSlotFlags_ [kIndex] = true;
        FreeSlotCount_++;
    }

    inline TDataType & operator [] (uint32 const kIndex)
    {
        return Data_ [kIndex].Value;
    }

    inline TDataType const & operator [] (uint32 const kIndex) const
    {
        return Data_ [kIndex].Value;
    }

    inline uint32 const Size() const
    {
        return Data_.size() - FreeSlotCount_;
    }

    inline uint32 const ActualSize() const
    {
        return Data_.size();
    }

private:
    std::vector<bool> FreeSlotFlags_ = {};
    std::vector<Node> Data_ = {};

    uint32 FreeSlotCount_ = {};
    uint32 FirstFreeSlotIndex_ = {};
    uint32 LastFreeSlotIndex_ = {};
};