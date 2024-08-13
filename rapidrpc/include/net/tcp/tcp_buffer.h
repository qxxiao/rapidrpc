#ifndef RAPIDRPC_NET_TCP_TCP_BUFFER_H
#define RAPIDRPC_NET_TCP_TCP_BUFFER_H

#include <vector>

namespace rapidrpc {

class TcpBuffer {

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
     * @return the number of bytes read, always equal to data.size() if data.size = 0 when call. If len<=available, return len, else return available.
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
     */
    void moveWriteIndex(int size);

    int readIndex() const;
    int writeIndex() const;

private:
    void resizeBuffer(int size);
    void shiftBuffer();

private:
    int m_read_index{0};
    int m_write_index{0};
    int m_size{0}; // size of the buffer

    std::vector<char> m_buffer; // the buffer
};
} // namespace rapidrpc

#endif // !RAPIDRPC_NET_TCP_TCP_BUFFER_H
