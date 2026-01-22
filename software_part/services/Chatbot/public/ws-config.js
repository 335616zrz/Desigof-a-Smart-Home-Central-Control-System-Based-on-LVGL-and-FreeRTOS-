// 如前端与后端不在同一主机/端口，请将下方地址替换为真实的 WebSocket 端点；
// 占位符未替换时不会生效，仍按当前页面同源地址连接。
const CUSTOM_WS = 'ws://<backend-host>:<port>/ws';
if (!CUSTOM_WS.includes('<backend-host>')) {
  window.CHATBOT_WS_URL = CUSTOM_WS;
}
