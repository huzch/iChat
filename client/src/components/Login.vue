<template>
  <div class="login-container">
    <div class="login-card">
      <h2>{{ isRegister ? 'iChat Register' : 'iChat Login' }}</h2>
      <el-form :model="form" label-width="0">
        <el-form-item>
          <el-input v-model="form.username" placeholder="Username" prefix-icon="User" />
        </el-form-item>
        <el-form-item>
          <el-input v-model="form.password" type="password" placeholder="Password" prefix-icon="Lock" show-password />
        </el-form-item>
        <el-form-item v-if="isRegister">
          <el-input v-model="form.confirmPassword" type="password" placeholder="Confirm Password" prefix-icon="Lock" show-password />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" class="login-btn" :loading="loading" @click="handleSubmit">
            {{ isRegister ? 'Register' : 'Log In' }}
          </el-button>
        </el-form-item>
        <div class="toggle-mode">
          <el-link type="primary" @click="toggleMode">
            {{ isRegister ? 'Already have an account? Log In' : 'No account? Register now' }}
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
      // Login successful, now we need to fetch the real user_id
      // Because UserLogin only returns session_id, but other APIs need user_id (UUID)
      // We use UserSearch to find our own user_id by username
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
            ElMessage.success('Login successful');
            emit('login-success', {
              username: me.name,
              userId: me.userId, // The real UUID
              sessionId: rsp.loginSessionId,
              avatar: me.avatar
            });
            return;
          }
        }
        throw new Error("Failed to retrieve user info");
      } catch (e) {
        console.error("Failed to fetch user details", e);
        ElMessage.warning('Login successful but failed to load user profile. Some features may not work.');
        // Fallback: use username as userId (might fail for some APIs)
        emit('login-success', {
          username: form.username,
          userId: form.username, 
          sessionId: rsp.loginSessionId
        });
      }
    } else {
      ElMessage.error(rsp.errmsg || 'Login failed');
    }
  } catch (error) {
    ElMessage.error('Network error or server unavailable');
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
