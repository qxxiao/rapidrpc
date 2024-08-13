#include "net/tcp/tcp_buffer.h"
#include "common/log.h"

#include <cstring>
#include <cassert>

namespace rapidrpc {

TcpBuffer::TcpBuffer(int size) : m_size(size) {
    m_buffer.reserve(size);
}

TcpBuffer::~TcpBuffer() {}

int TcpBuffer::readAvailable() const {
    return m_write_index - m_read_index;
}

// write until to the end of the buffer
int TcpBuffer::writeAvailable() const {
    return m_size - m_write_index;
}

int TcpBuffer::writeToBuffer(const char *data, int len) {
    if (len > writeAvailable()) {
        // resize the buffer
        resizeBuffer(2 * (readAvailable() + len));
    }
    // after resize, the buffer is large enough
    std::memcpy(m_buffer.data() + m_write_index, data, len);
    m_write_index += len;
    return len;
}

int TcpBuffer::readFromBuffer(std::vector<char> &data, int len) {

    if (readAvailable() <= 0 || len <= 0)
        return -1;
    if (data.capacity() < (unsigned long int)len)
        data.reserve(len);
    int read_len = std::min(len, readAvailable());
    std::memcpy(data.data(), m_buffer.data() + m_read_index, read_len);
    m_read_index += read_len;
    // ! check if shiftBuffer
    shiftBuffer();
    return read_len;
}

// always shift buffer
void TcpBuffer::resizeBuffer(int size) {
    // shift the data to the beginning of the buffer
    int read_len = readAvailable();
    // assert(size >= read_len);
    if (size < read_len) {
        ERRORLOG("TcpBuffer resizebuffer error, size < read_len");
        return;
    }
    std::vector<char> new_buffer(size);
    std::memcpy(new_buffer.data(), m_buffer.data() + m_read_index, read_len);
    m_buffer = std::move(new_buffer);
    m_write_index = read_len;
    m_read_index = 0;
    m_size = size; // note the size of the buffer
}

void TcpBuffer::shiftBuffer() {
    if (m_read_index >= m_size / 2) {
        int read_len = readAvailable();
        std::memmove(m_buffer.data(), m_buffer.data() + m_read_index, read_len);
        m_write_index = read_len;
        m_read_index = 0;
    }
}

int TcpBuffer::readIndex() const {
    return m_read_index;
}

int TcpBuffer::writeIndex() const {
    return m_write_index;
}

void TcpBuffer::moveReadIndex(int size) {
    int new_read_index = m_read_index + size;
    if (new_read_index >= m_write_index) {
        // ERRORLOG("TcpBuffer moveReadIndex error, invalid size=%d, old read_index=%d, bufferSize=%d", size,
        // m_read_index,
        //          m_size);
        m_read_index = 0;
        m_write_index = 0;
        return;
    }
    m_read_index = new_read_index;
    shiftBuffer();
}

// TODO: ??? no meaning
void TcpBuffer::moveWriteIndex(int size) {
    int new_write_index = m_write_index + size;
    if (new_write_index >= m_size) {
        ERRORLOG("moveWriteIndex error, invalid size=%d, old write_index=%d, size=%d", size, m_write_index, m_size);
        return;
    }
    m_write_index = new_write_index;
}

} // namespace rapidrpc