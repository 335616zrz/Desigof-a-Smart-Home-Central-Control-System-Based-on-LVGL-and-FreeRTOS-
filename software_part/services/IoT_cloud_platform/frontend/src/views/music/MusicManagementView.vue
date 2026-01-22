<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">音乐管理</p>
        <h1>音乐服务管理台</h1>
        <p class="lede">扫描 Music_Server 目录，维护 index.json，并为微信小程序转码生成 cache/*.mp3。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="info">已索引 {{ total }}</el-tag>
          <el-tag effect="plain" type="warning">未索引 {{ unindexedCount }}</el-tag>
          <el-tag effect="plain" type="success">可播放 {{ playableCount }}</el-tag>
          <span class="last-update muted">最近加载：{{ lastLoadedText }}</span>
        </div>

        <div class="app-hero__actions">
          <el-input
            v-model="keyword"
            placeholder="搜索标题/文件名…"
            clearable
            class="search-input"
            @keyup.enter="loadTracks"
            @clear="loadTracks"
          />
          <el-button plain @click="loadTracks">刷新列表</el-button>
          <el-button plain @click="loadUnindexed">扫描未索引</el-button>
          <el-button type="warning" plain @click="batchTranscode">补齐缓存</el-button>
          <el-button type="primary" @click="openCreate">手动添加</el-button>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <template #header>
        <div class="flex-between" style="align-items: center;">
          <span>预览播放</span>
          <div style="display: flex; gap: 8px; align-items: center;">
            <el-button text @click="openStatus">转码状态</el-button>
          </div>
        </div>
      </template>
      <div class="player-section">
        <p class="current-title">{{ currentTitle || '未选择曲目' }}</p>
        <audio ref="audioEl" controls preload="none" style="width: 100%; margin-top: 10px;"></audio>
      </div>
    </el-card>

    <el-card shadow="never" style="margin-top: 20px;">
      <template #header>
        <div class="flex-between" style="align-items: center;">
          <span>未索引文件</span>
          <div style="display: flex; gap: 8px; align-items: center;">
            <el-button text :loading="unindexedLoading" @click="loadUnindexed">刷新</el-button>
            <el-button
              type="primary"
              :disabled="selectedUnindexed.length === 0"
              :loading="batchAdding"
              @click="batchAddSelected"
            >
              批量添加（{{ selectedUnindexed.length }}）
            </el-button>
          </div>
        </div>
      </template>

      <TableSkeleton v-if="unindexedLoading && unindexedFiles.length === 0" :rows="6" />
      <el-table
        v-else
        :data="unindexedFiles"
        :loading="unindexedLoading"
        border
        stripe
        style="width: 100%"
        @selection-change="onUnindexedSelectionChange"
      >
        <el-table-column type="selection" width="46" />
        <el-table-column prop="filename" label="文件名" min-width="320" show-overflow-tooltip />
        <el-table-column prop="format" label="格式" width="100" />
        <el-table-column label="大小" width="120">
          <template #default="{ row }">
            <span class="muted">{{ formatBytes(row.size) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="120" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" size="small" :loading="addingOne === row.filename" @click="addOne(row.filename)">
              添加
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-card shadow="never" style="margin-top: 20px;">
      <template #header>
        <div class="flex-between" style="align-items:center;">
          <span>已索引曲目</span>
          <div style="display:flex; gap:8px; align-items:center;">
            <el-button text @click="loadTracks">刷新</el-button>
          </div>
        </div>
      </template>

      <TableSkeleton v-if="tracksLoading && tracks.length === 0" :rows="8" />
      <el-table
        v-else
        :data="tracks"
        :loading="tracksLoading"
        border
        style="width: 100%"
        stripe
      >
        <el-table-column prop="title" label="标题" min-width="220" show-overflow-tooltip />
        <el-table-column prop="filename" label="文件名" min-width="280" show-overflow-tooltip />
        <el-table-column prop="format" label="格式" width="100" />
        <el-table-column prop="duration" label="时长" width="100" />
        <el-table-column label="缓存" width="110">
          <template #default="{ row }">
            <el-tag v-if="row.cacheReady" size="small" type="success">可播放</el-tag>
            <el-tag v-else size="small" type="info">未就绪</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="url" label="可播放URL" min-width="300" show-overflow-tooltip />
        <el-table-column label="操作" width="260" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" size="small" @click="play(row)">播放</el-button>
            <el-button link size="small" @click="openEdit(row)">编辑</el-button>
            <el-button link size="small" @click="copyUrl(row.url || row.sourceUrl)">复制链接</el-button>
            <el-popconfirm title="确认从索引移除该曲目？（不会删除文件）" @confirm="remove(row.id)">
              <template #reference>
                <el-button link type="danger" size="small">删除</el-button>
              </template>
            </el-popconfirm>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-dialog
      v-model="formVisible"
      :title="editing ? '编辑曲目' : '手动添加（文件已在目录中）'"
      width="520px"
    >
      <el-form :model="form" label-width="90px">
        <el-form-item v-if="!editing" label="文件名" required>
          <el-input v-model="form.filename" placeholder="例如：128 晴天.flac" />
        </el-form-item>
        <el-form-item label="标题" required>
          <el-input v-model="form.title" placeholder="例如：晴天" />
        </el-form-item>
        <el-form-item v-if="editing" label="时长">
          <el-input v-model="form.duration" placeholder="例如：4:29" />
        </el-form-item>
        <el-form-item v-if="editing" label="源URL">
          <el-input v-model="form.sourceUrl" disabled />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="formVisible = false">取消</el-button>
        <el-button type="primary" :loading="formSubmitting" @click="submit">确定</el-button>
      </template>
    </el-dialog>

    <el-dialog v-model="statusVisible" title="转码状态" width="720px">
      <div style="display:flex; justify-content: flex-end; margin-bottom: 10px;">
        <el-button size="small" :loading="statusLoading" @click="loadStatus">刷新</el-button>
      </div>
      <el-table :data="transcodeStatus" :loading="statusLoading" border stripe style="width: 100%">
        <el-table-column prop="filename" label="文件名" min-width="360" show-overflow-tooltip />
        <el-table-column prop="status" label="状态" width="140" />
        <el-table-column prop="message" label="备注" min-width="180" show-overflow-tooltip />
      </el-table>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { ElMessage } from 'element-plus';
import TableSkeleton from '@/components/TableSkeleton.vue';
import { formatDateTime } from '@/utils/format';
import { musicApi, type MusicTrack } from '@/api/music';

interface MusicFile {
  id: string;
  filename: string;
  format: string;
  size: number;
  indexed: boolean;
}

const keyword = ref('');

const tracks = ref<MusicTrack[]>([]);
const tracksLoading = ref(false);
const lastLoaded = ref<number | null>(null);

const unindexedFiles = ref<MusicFile[]>([]);
const unindexedLoading = ref(false);
const selectedUnindexed = ref<MusicFile[]>([]);
const batchAdding = ref(false);
const addingOne = ref<string | null>(null);

const currentTitle = ref('');
const audioEl = ref<HTMLAudioElement>();

const formVisible = ref(false);
const formSubmitting = ref(false);
const editing = ref<MusicTrack | null>(null);
const form = ref({
  filename: '',
  title: '',
  duration: '' as string | undefined,
  sourceUrl: '' as string | undefined,
});

const statusVisible = ref(false);
const statusLoading = ref(false);
const transcodeStatus = ref<Array<{ filename: string; status: string; message?: string }>>([]);

const total = computed(() => tracks.value.length);
const unindexedCount = computed(() => unindexedFiles.value.length);
const playableCount = computed(() => tracks.value.filter(t => Boolean(t.url)).length);
const lastLoadedText = computed(() => (lastLoaded.value ? formatDateTime(lastLoaded.value) : '尚未加载'));

function formatBytes(size: number): string {
  if (!size || size < 0) return '-';
  const units = ['B', 'KB', 'MB', 'GB'];
  let v = size;
  let i = 0;
  while (v >= 1024 && i < units.length - 1) {
    v /= 1024;
    i += 1;
  }
  const digits = i === 0 ? 0 : i === 1 ? 1 : 2;
  return `${v.toFixed(digits)} ${units[i]}`;
}

async function loadTracks() {
  tracksLoading.value = true;
  try {
    tracks.value = await musicApi.list(keyword.value);
    lastLoaded.value = Date.now();
  } catch (e) {
    ElMessage.error('加载失败');
  } finally {
    tracksLoading.value = false;
  }
}

async function loadUnindexed() {
  unindexedLoading.value = true;
  try {
    unindexedFiles.value = await musicApi.unindexed();
  } catch (e) {
    ElMessage.error('扫描失败（需要管理员权限）');
  } finally {
    unindexedLoading.value = false;
  }
}

function onUnindexedSelectionChange(rows: MusicFile[]) {
  selectedUnindexed.value = rows;
}

async function addOne(filename: string) {
  if (!filename) return;
  addingOne.value = filename;
  try {
    await musicApi.create({ filename });
    ElMessage.success('已添加');
    await Promise.all([loadTracks(), loadUnindexed()]);
  } catch (e) {
    ElMessage.error('添加失败');
  } finally {
    addingOne.value = null;
  }
}

async function batchAddSelected() {
  if (selectedUnindexed.value.length === 0) return;
  batchAdding.value = true;
  try {
    const filenames = selectedUnindexed.value.map(f => f.filename);
    const res = await musicApi.batchAdd(filenames);
    ElMessage.success(`已添加 ${res.added} 个文件`);
    await Promise.all([loadTracks(), loadUnindexed()]);
  } catch (e) {
    ElMessage.error('批量添加失败');
  } finally {
    batchAdding.value = false;
  }
}

async function batchTranscode() {
  try {
    const res = await musicApi.batchTranscode();
    ElMessage.success(`已加入转码队列 ${res.queued} 项`);
  } catch (e) {
    ElMessage.error('触发转码失败（需要管理员权限）');
  }
}

async function loadStatus() {
  statusLoading.value = true;
  try {
    transcodeStatus.value = await musicApi.transcodeStatus();
  } catch (e) {
    ElMessage.error('获取转码状态失败（需要管理员权限）');
  } finally {
    statusLoading.value = false;
  }
}

function openStatus() {
  statusVisible.value = true;
  loadStatus();
}

function openCreate() {
  editing.value = null;
  form.value = { filename: '', title: '', duration: undefined, sourceUrl: undefined };
  formVisible.value = true;
}

function openEdit(track: MusicTrack) {
  editing.value = track;
  form.value = {
    filename: track.filename || '',
    title: track.title || '',
    duration: track.duration || undefined,
    sourceUrl: track.sourceUrl || undefined,
  };
  formVisible.value = true;
}

async function submit() {
  formSubmitting.value = true;
  try {
    if (editing.value) {
      if (!form.value.title) {
        ElMessage.warning('请填写标题');
        return;
      }
      await musicApi.update(editing.value.id, { title: form.value.title, duration: form.value.duration });
      ElMessage.success('更新成功');
    } else {
      if (!form.value.filename || !form.value.title) {
        ElMessage.warning('请填写文件名和标题');
        return;
      }
      await musicApi.create({ filename: form.value.filename, title: form.value.title });
      ElMessage.success('添加成功');
      await loadUnindexed();
    }
    formVisible.value = false;
    await loadTracks();
  } catch (e) {
    ElMessage.error('操作失败');
  } finally {
    formSubmitting.value = false;
  }
}

async function remove(id: string) {
  try {
    await musicApi.delete(id);
    ElMessage.success('已删除');
    await Promise.all([loadTracks(), loadUnindexed()]);
  } catch (e) {
    ElMessage.error('删除失败');
  }
}

function toLocalMusicUrl(url: string): string {
  // Convert https://servers.local/xxx to /music/xxx for local preview
  try {
    const u = new URL(url);
    return '/music' + u.pathname;
  } catch {
    return url;
  }
}

function play(track: MusicTrack) {
  const url = track.url || track.sourceUrl;
  if (!url) {
    ElMessage.warning('该曲目暂无可播放链接（缓存未就绪）');
    return;
  }
  if (audioEl.value) {
    audioEl.value.src = toLocalMusicUrl(url);
    audioEl.value.play();
    currentTitle.value = track.title;
  }
}

async function copyUrl(url?: string | null) {
  const text = (url || '').trim();
  if (!text) {
    ElMessage.warning('无可复制链接');
    return;
  }
  try {
    await navigator.clipboard.writeText(text);
    ElMessage.success('已复制链接');
  } catch (e) {
    ElMessage.error('复制失败');
  }
}

onMounted(async () => {
  await loadTracks();
  await loadUnindexed();
});
</script>

<style scoped>
.search-input {
  width: 320px;
}

.player-section {
  padding: 10px 0;
}

.current-title {
  font-size: 16px;
  font-weight: 500;
  color: #ffffff;
}

@media (max-width: 768px) {
  .search-input {
    width: 100%;
  }

  .app-hero__actions {
    display: flex;
    flex-direction: column;
    gap: 10px;
    width: 100%;
  }

  .app-hero__actions .el-button {
    width: 100%;
  }
}
</style>

