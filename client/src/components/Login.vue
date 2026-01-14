<template>
  <div class="login-container">
    <div class="login-card">
      <div class="mode-tabs">
        <div 
          class="mode-tab" 
          :class="{ active: loginMode === 'password' }" 
          @click="loginMode = 'password'"
        >{{ isRegister ? '用户名注册' : '密码登录' }}</div>
        <div 
          class="mode-tab" 
          :class="{ active: loginMode === 'phone' }" 
          @click="loginMode = 'phone'"
        >{{ isRegister ? '手机号注册' : '手机号登录' }}</div>
      </div>
      
      <el-form :model="form" label-width="0" style="margin-top: 20px;">
        <template v-if="loginMode === 'password'">
          <el-form-item>
            <el-input v-model="form.username" placeholder="用户名" prefix-icon="User" />
          </el-form-item>
          <el-form-item>
            <el-input v-model="form.password" type="password" placeholder="密码" prefix-icon="Lock" show-password />
          </el-form-item>
          <el-form-item v-if="isRegister">
            <el-input v-model="form.confirmPassword" type="password" placeholder="确认密码" prefix-icon="Lock" show-password />
          </el-form-item>
        </template>

        <template v-else>
          <el-form-item>
            <el-input v-model="form.phone" placeholder="手机号" prefix-icon="Iphone" />
          </el-form-item>
          <el-form-item>
            <div style="display: flex; width: 100%; gap: 10px;">
              <el-input v-model="form.verifyCode" placeholder="验证码" prefix-icon="Ticket" style="flex: 1;" />
              <el-button @click="getVerifyCode" :disabled="!!countdown">{{ countdown ? `${countdown}s` : '获取验证码' }}</el-button>
            </div>
          </el-form-item>
        </template>

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
import { User, Lock, Iphone, Ticket } from '@element-plus/icons-vue';
import { sendRequest } from '../api/client';
import { ElMessage } from 'element-plus';

const emit = defineEmits(['login-success']);

const isRegister = ref(false);
const loginMode = ref('password'); // 'password' or 'phone'
const countdown = ref(0);
const verifyCodeId = ref('');
let timer = null;

const form = reactive({
  username: '',
  password: '',
  confirmPassword: '',
  phone: '',
  verifyCode: ''
});

const loading = ref(false);

const toggleMode = () => {
  isRegister.value = !isRegister.value;
  form.password = '';
  form.confirmPassword = '';
  form.verifyCode = '';
  // 切换模式时重置验证码倒计时
  if (timer) {
    clearInterval(timer);
    timer = null;
  }
  countdown.value = 0;
};

const getVerifyCode = async () => {
  if (!/^1[3-9]\d{9}$/.test(form.phone)) {
    ElMessage.warning('请输入有效的手机号');
    return;
  }

  console.log("phone:", form.phone);
  
  try {
    const rsp = await sendRequest(
      '/user/get_phone_verify_code',
      'huzch.GetPhoneVerifyCodeReq',
      'huzch.GetPhoneVerifyCodeRsp',
      { phone_number: form.phone }
    );
    if (rsp.success) {
      ElMessage.success('验证码已发送');
      verifyCodeId.value = rsp.verifyCodeId;
      countdown.value = 60;
      if (timer) clearInterval(timer);
      timer = setInterval(() => {
        countdown.value--;
        if (countdown.value <= 0) {
          clearInterval(timer);
          timer = null;
        }
      }, 1000);
    } else {
      ElMessage.error(rsp.errmsg || '发送失败');
    }
  } catch (e) {
    ElMessage.error('服务器错误');
  }
};

const handleSubmit = async () => {
  if (isRegister.value) {
    if (loginMode.value === 'password') {
      await handleRegister();
    } else {
      await handlePhoneRegister();
    }
  } else {
    if (loginMode.value === 'password') {
      await handleLogin();
    } else {
      await handlePhoneLogin();
    }
  }
};

const handlePhoneRegister = async () => {
  if (!form.phone || !form.verifyCode) {
    ElMessage.warning('请输入手机号和验证码');
    return;
  }
  loading.value = true;
  try {
    const rsp = await sendRequest(
      '/user/phone_register',
      'huzch.PhoneRegisterReq',
      'huzch.PhoneRegisterRsp',
      {
        phoneNumber: form.phone,
        verifyCodeId: verifyCodeId.value,
        verifyCode: form.verifyCode
      }
    );
    if (rsp.success) {
      ElMessage.success('注册成功，请登录');
      isRegister.value = false;
    } else {
      ElMessage.error(rsp.errmsg || '注册失败');
    }
  } catch (e) {
    ElMessage.error('服务不可用');
  } finally {
    loading.value = false;
  }
};

const handlePhoneLogin = async () => {
  if (!form.phone || !form.verifyCode) {
    ElMessage.warning('请输入手机号和验证码');
    return;
  }
  loading.value = true;
  try {
    const rsp = await sendRequest(
      '/user/phone_login',
      'huzch.PhoneLoginReq',
      'huzch.PhoneLoginRsp',
      {
        phoneNumber: form.phone,
        verifyCodeId: verifyCodeId.value,
        verifyCode: form.verifyCode
      }
    );
    if (rsp.success) {
      await fetchAndEmitInfo(rsp.loginSessionId, form.phone);
    } else {
      ElMessage.error(rsp.errmsg || '登录失败');
    }
  } catch (e) {
    ElMessage.error('服务不可用');
  } finally {
    loading.value = false;
  }
};

const fetchAndEmitInfo = async (sessionId, searchKey) => {
  try {
    const searchRsp = await sendRequest(
      '/user/user_search',
      'huzch.UserSearchReq',
      'huzch.UserSearchRsp',
      {
        searchKey: searchKey,
        loginSessionId: sessionId
      }
    );

    if (searchRsp.success && searchRsp.usersInfo) {
      // 匹配名字或手机号
      const me = searchRsp.usersInfo.find(u => u.name === searchKey || u.phone === searchKey);
      if (me) {
        ElMessage.success('登录成功');
        emit('login-success', {
          username: me.name,
          userId: me.userId,
          sessionId: sessionId,
          avatar: me.avatar
        });
        return;
      }
    }
    throw new Error("获取用户信息失败");
  } catch (e) {
    console.error("获取用户信息失败", e);
    emit('login-success', {
      username: searchKey,
      userId: searchKey,
      sessionId: sessionId
    });
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
      await fetchAndEmitInfo(rsp.loginSessionId, form.username);
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

.mode-tabs {
  display: flex;
  border-bottom: 1px solid #eee;
  margin-bottom: 10px;
}

.mode-tab {
  flex: 1;
  padding: 10px;
  cursor: pointer;
  color: #666;
  font-size: 14px;
  transition: all 0.3s;
}

.mode-tab:hover {
  color: #07c160;
}

.mode-tab.active {
  color: #07c160;
  border-bottom: 2px solid #07c160;
  font-weight: bold;
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
