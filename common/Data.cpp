/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Assertion.hpp>
#include <WCDB/Data.hpp>
#include <WCDB/Notifier.hpp>
#include <zlib.h>

namespace WCDB {

#pragma mark - Initialize
Data::Data() : UnsafeData(), m_sharedBuffer(nullptr)
{
}

Data::Data(size_t size) : UnsafeData(), m_sharedBuffer(nullptr)
{
    reset(size);
}

Data::Data(const unsigned char* buffer, size_t size)
: UnsafeData(), m_sharedBuffer(nullptr)
{
    reset(buffer, size);
}

Data::Data(const UnsafeData& unsafeData)
{
    reset(unsafeData);
}

off_t Data::getCurrentOffset() const
{
    WCTInnerAssert(m_sharedBuffer != nullptr);
    return m_sharedBuffer->data() - m_buffer;
}

size_t Data::getSharedSize() const
{
    WCTInnerAssert(m_sharedBuffer != nullptr);
    return m_sharedBuffer->size();
}

Data::Data(const std::shared_ptr<std::vector<unsigned char>>& sharedBuffer, off_t offset, size_t size)
: UnsafeData(sharedBuffer->data() + offset, size), m_sharedBuffer(sharedBuffer)
{
}

#pragma mark - Reset
bool Data::resize(size_t size)
{
    if (m_sharedBuffer != nullptr && getCurrentOffset() + size <= getSharedSize()) {
        m_size = size;
    } else {
        std::shared_ptr<std::vector<unsigned char>> oldSharedBuffer = m_sharedBuffer;
        unsigned char* oldBuffer = m_buffer;
        size_t oldSize = m_size;
        if (!reset(size)) {
            return false;
        }
        if (oldBuffer) {
            memcpy(m_buffer, oldBuffer, oldSize);
        }
    }
    return true;
}

bool Data::reset(const unsigned char* buffer, size_t size)
{
    std::shared_ptr<std::vector<unsigned char>> newSharedBuffer(
    new std::vector<unsigned char>(size));
    if (newSharedBuffer == nullptr) {
        setThreadedError(Error(Error::Code::NoMemory));
        return false;
    }
    if (buffer != nullptr) {
        memcpy(newSharedBuffer->data(), buffer, size);
    }
    m_sharedBuffer = newSharedBuffer;
    m_size = size;
    m_buffer = m_sharedBuffer->data();
    return true;
}

bool Data::reset(size_t size)
{
    return reset(nullptr, size);
}

bool Data::reset(const UnsafeData& unsafeData)
{
    return reset(unsafeData.buffer(), unsafeData.size());
}

#pragma mark - Subdata
Data Data::subdata(size_t size) const
{
    if (size == 0) {
        return emptyData();
    }
    WCTRemedialAssert(
    getCurrentOffset() + size <= getSharedSize(), "Memory cross-border", return Data(););
    return Data(m_sharedBuffer, getCurrentOffset(), size);
}

Data Data::subdata(off_t offset, size_t size) const
{
    if (size == 0) {
        return emptyData();
    }
    WCTRemedialAssert(size > 0 && getCurrentOffset() + offset + size <= getSharedSize(),
                      "Memory cross-border",
                      return Data(););
    return Data(m_sharedBuffer, getCurrentOffset() + offset, size);
}

const Data& Data::emptyData()
{
    static const Data* s_empty = new Data;
    return *s_empty;
}

} //namespace WCDB
