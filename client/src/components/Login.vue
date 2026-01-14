<template>
  <div class="login-container">
    <div class="login-card">
      <h2>{{ isRegister ? 'iChat 注册' : 'iChat 登录' }}</h2>
      <el-form :model="form" label-width="0">
        <el-form-item>
          <el-input v-model="form.username" placeholder="用户名" prefix-icon="User" />
        </el-form-item>
        <el-form-item>
          <el-input v-model="form.password" type="password" placeholder="密码" prefix-icon="Lock" show-password />
        </el-form-item>
        <el-form-item v-if="isRegister">
          <el-input v-model="form.confirmPassword" type="password" placeholder="确认密码" prefix-icon="Lock" show-password />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" class="login-btn" :loading="loading" @click="handleSubmit">
            {{ isRegister ? '立即注册' : '登录' }}
          </el-button>
        </el-form-item>
        <div class="toggle-mode">
          <el-link type="primary" @click="toggleMode">
            {{ isRegister ? '已有账号？去登录' : '没有账号？立即注册' }}
          </el-link>
        </div>
      </el-form>
    </div>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue';
import { User, Lock } from '@element-plus/icons-vue';
import { sendRequest } from '../api/client';
import { ElMessage } from 'element-plus';

const emit = defineEmits(['login-success']);

const isRegister = ref(false);
const form = reactive({
  username: '',
  password: '',
  confirmPassword: ''
});

const loading = ref(false);

const toggleMode = () => {
  isRegister.value = !isRegister.value;
  form.password = '';
  form.confirmPassword = '';
};

const handleSubmit = async () => {
  if (isRegister.value) {
    await handleRegister();
  } else {
    await handleLogin();
  }
};

const handleRegister = async () => {
  if (!form.username || !form.password) {
    ElMessage.warning('Please enter username and password');
    return;
  }
  if (form.password !== form.confirmPassword) {
    ElMessage.warning('Passwords do not match');
    return;
  }

  loading.value = true;
  try {
    const rsp = await sendRequest(
      '/user/user_register',
      'huzch.UserRegisterReq',
      'huzch.UserRegisterRsp',
      {
        userName: form.username,
        password: form.password
      }
    );

    if (rsp.success) {
      ElMessage.success('Registration successful! Please log in.');
      isRegister.value = false;
      form.password = '';
      form.confirmPassword = '';
    } else {
      ElMessage.error(rsp.errmsg || 'Registration failed');
    }
  } catch (error) {
    ElMessage.error('Network error or server unavailable');
  } finally {
    loading.value = false;
  }
};

const handleLogin = async () => {
  if (!form.username || !form.password) {
    ElMessage.warning('Please enter username and password');
    return;
  }

  loading.value = true;
  try {
    const rsp = await sendRequest(
      '/user/user_login',
      'huzch.UserLoginReq',
      'huzch.UserLoginRsp',
      {
        userName: form.username,
        password: form.password
      }
    );

    if (rsp.success) {
      // 登录成功，获取真实的 user_id
      // 因为 UserLogin 仅返回 session_id，其他接口需要 UUID 格式的 user_id
      // 通过 UserSearch 根据用户名搜索获取自己的 user_id
      try {
        const searchRsp = await sendRequest(
          '/user/user_search',
          'huzch.UserSearchReq',
          'huzch.UserSearchRsp',
          {
            searchKey: form.username,
            loginSessionId: rsp.loginSessionId
          }
        );

        if (searchRsp.success && searchRsp.usersInfo) {
          const me = searchRsp.usersInfo.find(u => u.name === form.username);
          if (me) {
            ElMessage.success('登录成功');
            emit('login-success', {
              username: me.name,
              userId: me.userId, // 真实的 UUID
              sessionId: rsp.loginSessionId,
              avatar: me.avatar
            });
            return;
          }
        }
        throw new Error("获取用户信息失败");
      } catch (e) {
        console.error("获取用户信息失败", e);
        ElMessage.warning('登录成功但加载详细资料失败，部分功能可能受限');
        // 兜底方案：使用用户名作为 userId（部分 API 可能失败）
        emit('login-success', {
          username: form.username,
          userId: form.username, 
          sessionId: rsp.loginSessionId
        });
      }
    } else {
      ElMessage.error(rsp.errmsg || '登录失败');
    }
  } catch (error) {
    ElMessage.error('网络错误或服务器不可用');
  } finally {
    loading.value = false;
  }
};
</script>

<style scoped>
.login-container {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100vh;
  background-color: #f5f5f5;
}

.login-card {
  width: 350px;
  padding: 40px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  text-align: center;
}

h2 {
  margin-bottom: 30px;
  color: #333;
}

.login-btn {
  width: 100%;
}

.toggle-mode {
  margin-top: 15px;
  text-align: center;
}
</style>
