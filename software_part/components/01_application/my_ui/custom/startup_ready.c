/**
 * 启动阶段就绪同步（Wi-Fi / Chatbot / Music）
 *
 * 设计目标：
 * - 由各自模块在“首轮初始化完成”时置位 READY bit
 * - 启动动画 / splash 线程可以阻塞等待这些 bit（带总超时）再进入首页
 */

#include "startup_ready.h"

static EventGroupHandle_t s_group = NULL;

EventGroupHandle_t startup_ready_group(void)
{
    if (!s_group) {
        s_group = xEventGroupCreate();
    }
    return s_group;
}

