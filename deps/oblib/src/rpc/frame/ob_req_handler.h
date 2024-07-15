/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef _OCEABASE_RPC_FRAME_OB_REQ_HANDLER_H_
#define _OCEABASE_RPC_FRAME_OB_REQ_HANDLER_H_

#include "rpc/frame/obi_easy_packet_handler.h"

#define EZ_ADD_CB(func) ez_handler_.func = (oceanbase::easy::func)

namespace oceanbase
{
namespace rpc
{
namespace frame
{

enum {
  MAGIC_VERSION_OFF                = 3,
  MAGIC_VERSION_MASK               = (0x7 << MAGIC_VERSION_OFF),
  MAGIC_KEEPALIVE_INDICATOR_OFF    = 6,
  MAGIC_KEEPALIVE_INDICATOR_MASK   = (1 << MAGIC_KEEPALIVE_INDICATOR_OFF),
  KEEPALIVE_DATA_TYPE_RL           = 0X1,
  KEEPALIVE_DATA_FLAG_EN_RL_OFF    = 0,
  KEEPALIVE_DATA_FLAG_EN_RL_MASK   = (1 << KEEPALIVE_DATA_FLAG_EN_RL_OFF),
  KEEPALIVE_DATA_FLAG_SET_GRP_OFF  = 1,
  KEEPALIVE_DATA_FLAG_SET_GRP_MASK = (1 << KEEPALIVE_DATA_FLAG_SET_GRP_OFF),
};

class ObReqHandler
    : public ObIEasyPacketHandler
{
public:
  ObReqHandler()
      : ez_handler_()
  {
    memset(&ez_handler_, 0, sizeof (ez_handler_));
    ez_handler_.user_data = this;
  }
  virtual ~ObReqHandler() {}

  inline easy_io_handler_pt *ez_handler()
  {
    return &ez_handler_;
  }

  /**[latte]
   *  easy_connection_t 连接
   *    每个连接上多个消息用链表连起来
   *  easy_request_t 请求
   *    多个请求组合起来形成一个完整的消息（可看作应用层的包）
   *  easy_message_t 消息
   * 
   *  子类ObMySQLHandler 和 ObRpcHandler
   */

  //[latte]从网络上读取一段字节流，按照定义的协议，解析成数据结构，供之后处理
  void *decode(easy_message_t *m); 
  //[latte]把process 的结果转换成字节流 然后把结果挂在到request的输出上
  int encode(easy_request_t *r, void *packet);
  //[latte]处理从decode中解析出的结构 可以是同步处理,也可以是异步处理
  int process(easy_request_t *r);
  int batch_process(easy_message_t *m);
  //[latte]接受TCP连接 回调该方法，可以在该事件中做密码验证等事情
  int on_connect(easy_connection_t *c);
  int on_disconnect(easy_connection_t *c);
  int new_packet(easy_connection_t *c);
  uint64_t get_packet_id(easy_connection_t *c, void *packet);
  void set_trace_info(easy_request_t *r, void *packet);
  int on_idle(easy_connection_t *c);
  void send_buf_done(easy_request_t *r);
  void sending_data(easy_connection_t *c);
  int send_data_done(easy_connection_t *c);
  int on_redispatch(easy_connection_t *c);
  int on_close(easy_connection_t *c);
  //断开连接前执行的操作
  int cleanup(easy_request_t *r, void *apacket);

public:
  static const uint8_t API_VERSION = 1;
  static const uint8_t MAGIC_HEADER_FLAG[4];
  static const uint8_t MAGIC_COMPRESS_HEADER_FLAG[4];

protected:
  easy_io_handler_pt ez_handler_;
}; // end of class ObReqHandler

} // end of namespace frame
} // end of namespace rpc
} // end of namespace oceanbase

namespace oceanbase
{
namespace easy
{

void *decode(easy_message_t *m);
int encode(easy_request_t *r, void *packet);
int process(easy_request_t *r);
int batch_process(easy_message_t *m);
int on_connect(easy_connection_t *c);
int on_disconnect(easy_connection_t *c);
int new_packet(easy_connection_t *c);
int new_keepalive_packet(easy_connection_t *c);
uint64_t get_packet_id(easy_connection_t *c, void *packet);
void set_trace_info(easy_request_t *r, void *packet);
int on_idle(easy_connection_t *c);
void send_buf_done(easy_request_t *r);
void sending_data(easy_connection_t *c);
int send_data_done(easy_connection_t *c);
int on_redispatch(easy_connection_t *c);
int on_close(easy_connection_t *c);
int cleanup(easy_request_t *r, void *apacket);


} // end of namespace easy
} // end of namespace oceanbase



#endif /* _OCEABASE_RPC_FRAME_OB_REQ_HANDLER_H_ */
