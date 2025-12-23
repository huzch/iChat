<template>
  <div class="app-container">
    <Login v-if="!currentUser" @login-success="handleLoginSuccess" />
    <Chat v-else :currentUser="currentUser" />
  </div>
</template>

<script setup>
import { ref } from 'vue';
import Login from './components/Login.vue';
import Chat from './components/Chat.vue';

const currentUser = ref(null);

const handleLoginSuccess = (user) => {
  currentUser.value = {
    userId: user.userId, 
    username: user.username,
    sessionId: user.sessionId,
    avatarUrl: user.avatar ? 'data:image/png;base64,' + btoa(String.fromCharCode(...new Uint8Array(user.avatar))) : ''
  };
};
</script>

<style>
body {
  margin: 0;
  padding: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
}
.app-container {
  height: 100vh;
  width: 100vw;
}
</style>
