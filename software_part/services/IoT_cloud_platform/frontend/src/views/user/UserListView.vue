<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">用户管理</p>
        <h1>团队与权限</h1>
        <p class="lede">快速查看角色并执行批量操作。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="info">用户 {{ store.items.length }}</el-tag>
          <el-tag effect="plain" type="warning">管理员 {{ adminCount }}</el-tag>
        </div>

        <div class="app-hero__actions">
          <el-button type="danger" :disabled="!selected.length || !canCreateUsers" @click="onBatchDelete">批量删除</el-button>
          <el-button type="success" :disabled="!canCreateUsers" @click="openBatchCreate">批量创建</el-button>
          <el-button type="primary" :disabled="!canCreateUsers" @click="openCreate">新建用户</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><User /></el-icon></div>
            <div class="mini-meta">
              <p>总用户</p>
              <strong>{{ store.items.length }}</strong>
              <span>全部账户</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><Setting /></el-icon></div>
            <div class="mini-meta">
              <p>管理员</p>
              <strong>{{ adminCount }}</strong>
              <span>含一级/二级</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><UserFilled /></el-icon></div>
            <div class="mini-meta">
              <p>当前用户</p>
              <strong>{{ currentUsername || '-' }}</strong>
              <span>正在登录</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <template #header>
        <div class="flex-between" style="align-items:center; gap:12px; flex-wrap:wrap;">
          <span>用户列表</span>
          <div style="display:flex; gap:8px;">
            <el-button type="danger" :disabled="!selected.length || !canCreateUsers" @click="onBatchDelete">批量删除</el-button>
            <el-button type="success" :disabled="!canCreateUsers" @click="openBatchCreate">批量创建</el-button>
            <el-button type="primary" :disabled="!canCreateUsers" @click="openCreate">新建用户</el-button>
          </div>
        </div>
      </template>

      <el-table :data="store.items" border stripe @selection-change="onSelectionChange" :loading="store.loading">
        <el-table-column type="selection" width="50" />
        <el-table-column prop="username" label="用户名" width="200" />
        <el-table-column prop="email" label="邮箱" />
        <el-table-column prop="role" label="角色" width="140">
          <template #default="{ row }">
            <el-tag :type="roleType(row.role)">{{ roleLabel(row.role) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="createdAt" label="创建时间" width="200" :formatter="format" />
        <el-table-column label="操作" width="280">
          <template #default="{ row }">
            <el-popconfirm
              v-if="canUpgrade(row)"
              title="确认升级为二级管理员？"
              @confirm="upgradeUser(row.id)"
            >
              <template #reference>
                <el-button link type="primary" size="small">升级</el-button>
              </template>
            </el-popconfirm>

            <el-popconfirm
              v-if="canDowngrade(row)"
              title="确认降级为普通用户？"
              @confirm="downgradeUser(row.id)"
            >
              <template #reference>
                <el-button link type="warning" size="small">降级</el-button>
              </template>
            </el-popconfirm>

            <el-popconfirm
              title="确认删除该用户？"
              @confirm="remove(row.id)"
              :disabled="!canDelete(row)"
            >
              <template #reference>
                <el-button
                  link
                  type="danger"
                  size="small"
                  :disabled="!canDelete(row)"
                  :title="!canDelete(row) ? (row.username === currentUsername ? '不能删除当前登录用户' : '不能删除最后一个管理员') : ''"
                >
                  删除
                </el-button>
              </template>
            </el-popconfirm>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-dialog v-model="dialogVisible" title="新建用户" width="480px">
      <el-form :model="form" :rules="rules" ref="formRef" label-width="90px">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="form.username" />
        </el-form-item>
        <el-form-item label="邮箱" prop="email">
          <el-input v-model="form.email" />
        </el-form-item>
        <el-form-item label="角色" prop="role">
          <el-select v-model="form.role" placeholder="选择角色">
            <el-option label="二级管理员" value="ROLE_SECONDARY_ADMIN" />
            <el-option label="普通用户" value="ROLE_OPERATOR" />
          </el-select>
        </el-form-item>
        <el-form-item label="密码" prop="password">
          <el-input v-model="form.password" :type="showPwd ? 'text' : 'password'">
            <template #suffix>
              <el-icon class="eye" @click.stop="togglePwd">
                <View v-if="showPwd" />
                <Hide v-else />
              </el-icon>
            </template>
          </el-input>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="submit">保存</el-button>
      </template>
    </el-dialog>

    <el-dialog v-model="batchDialogVisible" title="批量创建用户" width="600px">
      <el-alert type="info" :closable="false" style="margin-bottom: 16px;">
        <p>每行输入一个用户，格式：用户名,密码,角色,邮箱</p>
        <p>角色：ROLE_SECONDARY_ADMIN（二级管理员） 或 ROLE_OPERATOR（普通用户）</p>
        <p>示例：user1,pass123,ROLE_OPERATOR,user1@example.com</p>
      </el-alert>
      <el-input
        v-model="batchText"
        type="textarea"
        :rows="10"
        placeholder="user1,pass123,ROLE_OPERATOR,user1@example.com&#10;user2,pass456,ROLE_OPERATOR,user2@example.com"
      />
      <template #footer>
        <el-button @click="batchDialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="batchSaving" @click="submitBatch">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { onMounted, reactive, ref, computed } from 'vue';
import { ElMessage } from 'element-plus';
import { User, Setting, UserFilled, View, Hide } from '@element-plus/icons-vue';
import { useUserStore } from '@/stores/user';
import { useAuthStore } from '@/stores/auth';
import { formatDateTime } from '@/utils/format';

const store = useUserStore();
const authStore = useAuthStore();
const dialogVisible = ref(false);
const batchDialogVisible = ref(false);
const saving = ref(false);
const batchSaving = ref(false);
const formRef = ref();
const showPwd = ref(false);
const selected = ref<any[]>([]);
const batchText = ref('');

const form = reactive({ username: '', email: '', role: 'ROLE_OPERATOR', password: '' });

const currentUsername = computed(() => authStore.user?.username);
const adminCount = computed(() => store.items.filter((u: any) =>
  u.role === 'ROLE_PRIMARY_ADMIN' || u.role === 'ROLE_SECONDARY_ADMIN' || u.role === 'ROLE_ADMIN'
).length);

const canCreateUsers = computed(() => authStore.user?.role === 'ROLE_PRIMARY_ADMIN' || authStore.user?.role === 'ROLE_ADMIN');

const rules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, message: '至少 3 个字符', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, message: '至少 6 位', trigger: 'blur' }
  ],
  role: [{ required: true, message: '请选择角色', trigger: 'change' }],
  email: [{ type: 'email', message: '邮箱格式不正确', trigger: 'blur' }]
};

function showApiError(error: any, fallback: string) {
  const status = error?.response?.status;
  const data = error?.response?.data;
  const message =
    (typeof data === 'string' ? data : data?.message) ||
    error?.message ||
    fallback;

  if (status === 401) {
    ElMessage.error('登录已过期，请重新登录');
    setTimeout(() => { window.location.href = '/login'; }, 1200);
    return;
  }

  if (status === 403) {
    ElMessage.error('权限不足：当前账号无法执行该操作');
    return;
  }

  ElMessage.error(message);
}

onMounted(() => {
  store.fetchList({ page: 1, pageSize: 100 }).catch((error: any) => {
    showApiError(error, '加载用户失败');
  });
});

function roleLabel(role: string) {
  if (role === 'ROLE_PRIMARY_ADMIN') return '一级管理员';
  if (role === 'ROLE_SECONDARY_ADMIN') return '二级管理员';
  if (role === 'ROLE_ADMIN') return '管理员';
  if (role === 'ROLE_OPERATOR') return '普通用户';
  return '只读';
}
function roleType(role: string) {
  if (role === 'ROLE_PRIMARY_ADMIN') return 'danger';
  if (role === 'ROLE_SECONDARY_ADMIN') return 'warning';
  if (role === 'ROLE_ADMIN') return 'danger';
  if (role === 'ROLE_OPERATOR') return 'success';
  return 'info';
}

function format(_: unknown, __: unknown, value: string) {
  return formatDateTime(value);
}

function onSelectionChange(rows: any[]) {
  selected.value = rows;
}

function openCreate() {
  Object.assign(form, { username: '', email: '', role: 'ROLE_OPERATOR', password: '' });
  dialogVisible.value = true;
}

function openBatchCreate() {
  batchText.value = '';
  batchDialogVisible.value = true;
}

function togglePwd() {
  showPwd.value = !showPwd.value;
}

function canDelete(user: any): boolean {
  if (user.username === currentUsername.value) return false;
  if (user.isProtected) return false;
  const isAnyAdmin = user.role === 'ROLE_PRIMARY_ADMIN' || user.role === 'ROLE_SECONDARY_ADMIN' || user.role === 'ROLE_ADMIN';
  if (isAnyAdmin && adminCount.value <= 1) return false;
  return true;
}

function canUpgrade(user: any): boolean {
  if (authStore.user?.role !== 'ROLE_PRIMARY_ADMIN') return false;
  return user.role === 'ROLE_OPERATOR';
}

function canDowngrade(user: any): boolean {
  if (authStore.user?.role !== 'ROLE_PRIMARY_ADMIN') return false;
  return user.role === 'ROLE_SECONDARY_ADMIN';
}

async function submit() {
  const valid = await formRef.value?.validate();
  if (!valid) return;
  saving.value = true;
  try {
    await store.create({ ...form });
    dialogVisible.value = false;
  } catch (error: any) {
    showApiError(error, '创建用户失败');
  } finally {
    saving.value = false;
  }
}

function parseBatchText(text: string) {
  const lines = text
    .split(/\r?\n/)
    .map(line => line.trim())
    .filter(Boolean);

  const users = lines.map((line, index) => {
    const parts = line.split(',').map(p => p.trim());
    const [username, password, role, email] = parts;
    if (!username || !password || !role) {
      throw new Error(`第 ${index + 1} 行格式错误：需要 用户名,密码,角色[,邮箱]`);
    }
    return {
      username,
      password,
      role,
      email: email || undefined
    };
  });

  return users;
}

async function submitBatch() {
  if (!batchText.value.trim()) return;
  batchSaving.value = true;
  try {
    if (!canCreateUsers.value) {
      ElMessage.error('权限不足：只有一级管理员/管理员可批量创建用户');
      return;
    }
    const users = parseBatchText(batchText.value.trim());
    await store.batchCreate(users);
    batchDialogVisible.value = false;
  } catch (error: any) {
    showApiError(error, '批量创建失败');
  } finally {
    batchSaving.value = false;
  }
}

async function remove(id: number) {
  try {
    await store.remove(id);
  } catch (error: any) {
    showApiError(error, '删除用户失败');
  }
}

async function onBatchDelete() {
  if (!selected.value.length) return;
  try {
    if (!canCreateUsers.value) {
      ElMessage.error('权限不足：只有一级管理员/管理员可批量删除用户');
      return;
    }
    await store.removeBatch(selected.value.map((u: any) => u.id));
    selected.value = [];
  } catch (error: any) {
    showApiError(error, '批量删除失败');
  }
}

async function upgradeUser(id: number) {
  try {
    await store.upgrade(id);
  } catch (error: any) {
    showApiError(error, '升级失败');
  }
}

async function downgradeUser(id: number) {
  try {
    await store.downgrade(id);
  } catch (error: any) {
    showApiError(error, '降级失败');
  }
}
</script>

<style scoped>
.eye {
  cursor: pointer;
}
</style>
