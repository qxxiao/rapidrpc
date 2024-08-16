#ifndef RAPIDRPC_NET_TCP_TCP_BUFFER_H
#define RAPIDRPC_NET_TCP_TCP_BUFFER_H

#include <vector>
#include <memory>

namespace rapidrpc {

class TcpBuffer {
public:
    using s_ptr = std::shared_ptr<TcpBuffer>;

public:
    TcpBuffer(int size);
    ~TcpBuffer();

    /**
     * @brief the number of bytes available to read
     */
    int readAvailable() const;

    /**
     * @brief the number of bytes available to write
     */
    int writeAvailable() const;

    /**
     * @brief write data to the buffer
     * @param data the data to write
     * @param len the length of the data
     * @return the number of bytes written, always equal to len
     */
    int writeToBuffer(const char *data, int len);

    /**
     * @brief read data from the buffer
     * @param data the data to read
     * @param len the length to read
     * @return the number of bytes read, min(readAvailable(), len)
     * @note 满足 data.capacity() >= len
     */
    int readFromBuffer(std::vector<char> &data, int len);

    /**
     * @brief manually move readIndex forward by size bytes
     * @param size drop the first size bytes
     */
    void moveReadIndex(int size);

    /**
     * @brief move writeIndex forward by size bytes
     * @param size the number of bytes to move, should be less than writeAvailable()
     * @note 用于操作系统读取写入socket数据后，移动指针，需要保证移动的字节数小于可写字节数（可写字节数量要预先足够）
     */
    void moveWriteIndex(int size);

    int readIndex() const;
    int writeIndex() const;

    /**
     * @brief resize the buffer
     * @note 将可读数据移到 buffer 的最前面，并将 buffer 扩大到 size
     */
    void resizeBuffer(int size);

private:
    void shiftBuffer();

private:
    int m_read_index{0};
    int m_write_index{0};
    int m_size{0}; // size of the buffer

public:
    std::vector<char> m_buffer; // the buffer
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_TCP_BUFFER_H
