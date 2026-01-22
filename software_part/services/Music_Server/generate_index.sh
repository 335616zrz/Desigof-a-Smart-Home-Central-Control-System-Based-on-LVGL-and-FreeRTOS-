#!/bin/bash
# 自动生成音乐索引文件 (index.json)
# 使用方法：在 Music_Server 目录下直接运行 ./generate_index.sh

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== 音乐索引生成器 ===${NC}"

# 1. 检测当前目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MUSIC_DIR="$SCRIPT_DIR"

echo "当前目录: $MUSIC_DIR"

# 2. 检测项目根目录
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

if [ ! -f "$PROJECT_ROOT/tools/my_music_tools/generate_music_index.py" ]; then
    echo -e "${RED}错误: 找不到 generate_music_index.py 脚本${NC}"
    echo "预期路径: $PROJECT_ROOT/tools/my_music_tools/generate_music_index.py"
    echo -e "${YELLOW}提示: 请确保在正确的项目目录下运行此脚本${NC}"
    exit 1
fi

# 3. 统计音乐文件数量
MUSIC_COUNT=$(find "$MUSIC_DIR" -maxdepth 1 -type f \( -name "*.mp3" -o -name "*.wav" -o -name "*.m4a" -o -name "*.flac" -o -name "*.aac" -o -name "*.ogg" \) | wc -l)

if [ "$MUSIC_COUNT" -eq 0 ]; then
    echo -e "${YELLOW}警告: 当前目录没有找到音乐文件！${NC}"
    echo "支持的格式: *.mp3, *.wav, *.m4a, *.flac, *.aac, *.ogg"
    echo ""
    echo "请先添加音乐文件到 $MUSIC_DIR"
    echo "例如: cp /path/to/your/music/*.mp3 $MUSIC_DIR/"
    exit 1
fi

echo -e "${GREEN}找到 $MUSIC_COUNT 个音乐文件${NC}"

# 4. 配置基础 URL
if [ -z "$MUSIC_BASE_URL" ]; then
    MUSIC_BASE_URL="https://music-server.local/"
    echo -e "${YELLOW}使用默认 MUSIC_BASE_URL: $MUSIC_BASE_URL${NC}"
    echo "如需自定义，请设置环境变量: MUSIC_BASE_URL=https://your-server.local/ $0"
fi

# 5. 运行 Python 生成脚本
echo ""
echo -e "${GREEN}正在生成索引文件...${NC}"

cd "$PROJECT_ROOT"

MUSIC_BASE_URL="$MUSIC_BASE_URL" python3 tools/my_music_tools/generate_music_index.py

# 6. 验证生成结果
INDEX_FILE="$MUSIC_DIR/index.json"

if [ ! -f "$INDEX_FILE" ]; then
    echo -e "${RED}错误: index.json 生成失败${NC}"
    exit 1
fi

# 7. 显示统计信息
INDEXED_COUNT=$(jq '. | length' "$INDEX_FILE")
INDEX_SIZE=$(du -h "$INDEX_FILE" | cut -f1)

echo ""
echo -e "${GREEN}=== 生成完成 ===${NC}"
echo "索引文件: $INDEX_FILE"
echo "索引大小: $INDEX_SIZE"
echo "音乐数量: $INDEXED_COUNT 首"
echo ""

# 8. 显示前 5 首歌曲（如果有 jq）
if command -v jq &> /dev/null; then
    echo -e "${GREEN}前 5 首歌曲预览:${NC}"
    jq -r '.[0:5] | .[] | "  - \(.title // .filename)"' "$INDEX_FILE"
    echo ""
else
    echo -e "${YELLOW}提示: 安装 jq 工具可以预览索引内容${NC}"
    echo "安装命令: sudo apt install jq"
    echo ""
fi

# 9. 提示后续操作
echo -e "${GREEN}下一步:${NC}"
echo "1. 验证索引文件: jq '.[0:5]' $INDEX_FILE"
echo "2. 重启 Caddy 服务: sudo systemctl reload caddy"
echo "3. 测试访问: curl https://music-server.local/index.json"
echo "4. 访问管理界面: https://music-server.local/admin/"
echo ""

echo -e "${GREEN}✓ 完成！${NC}"
