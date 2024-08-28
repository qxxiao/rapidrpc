#include "rapidrpc/common/msg_id_util.h"

#include <unistd.h>
#include <fcntl.h>

namespace rapidrpc {

static int g_msg_id_length = 20; // 消息 ID 的长度
static int g_random_fd = -1;     // 随机数文件描述符

// 每个线程的消息 ID 单独，线程内部递增
static thread_local std::string t_msg_id_no;
static thread_local std::string t_max_msg_id_no(g_msg_id_length, '9');

std::string MsgIdUtil::GenMsgId() {
    // 生成一个随机的消息 ID
    if (t_msg_id_no.empty() || t_msg_id_no == t_max_msg_id_no) {
        if (g_random_fd == -1) {
            g_random_fd = open("/dev/urandom", O_RDONLY);
        }
        // 读取随机数
        std::string random_str(g_msg_id_length, 0); // 20 个字节
        int rt = read(g_random_fd, &random_str[0], g_msg_id_length);
        if (rt != g_msg_id_length) {
            return "";
        }
        // to 0-9
        for (int i = 0; i < g_msg_id_length; i++) {
            random_str[i] = (static_cast<uint8_t>(random_str[i])) % 10 + '0';
        }
        t_msg_id_no = random_str;
    }
    else {
        // +1
        int carry = 1;
        for (int i = g_msg_id_length - 1; i >= 0; i--) {
            int num = t_msg_id_no[i] - '0' + carry;
            if (num >= 10) {
                t_msg_id_no[i] = '0';
                carry = 1;
            }
            else {
                t_msg_id_no[i] = num + '0';
                carry = 0;
                break;
            }
        }
    }
    return t_msg_id_no;
}

} // namespace rapidrpc