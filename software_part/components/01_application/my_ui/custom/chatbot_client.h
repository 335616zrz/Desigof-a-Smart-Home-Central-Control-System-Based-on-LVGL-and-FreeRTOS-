// Simple Chatbot WebSocket client for GPT screen
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

// 绑定 UI，以便在收到服务器回复时往聊天记录里追加文本
void chatbot_client_bind_ui(lv_ui *ui);

// 启动客户端（懒连接：首次发送或需要时再连接）
void chatbot_client_start(void);

// 发送一条文本消息；内部会自动连接/重连
bool chatbot_send_text(const char *utf8_text);

// 尝试中止当前会话中的 LLM/TTS（用于“打断”播报）
bool chatbot_abort(void);

#ifdef __cplusplus
}
#endif
