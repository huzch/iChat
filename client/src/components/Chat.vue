<template>
  <div class="chat-container">
    <!-- Left Sidebar: Navigation -->
    <div class="nav-sidebar">
      <div class="avatar-wrapper" @click="showProfile" style="cursor: pointer;">
        <el-avatar :size="40" :src="currentUser.avatarUrl || defaultAvatar" />
        <div class="current-username">{{ currentUser.username }}</div>
      </div>
      <div class="nav-icons">
        <el-icon 
          class="nav-icon" 
          :class="{ active: activeTab === 'chat' }" 
          @click="activeTab = 'chat'"
        ><ChatDotRound /></el-icon>
        <el-icon 
          class="nav-icon" 
          :class="{ active: activeTab === 'requests' }" 
          @click="showFriendRequests"
        ><User /></el-icon>
      </div>
    </div>

    <!-- Middle Sidebar: Session List or Friend Requests -->
    <div class="list-sidebar">
      <div v-if="activeTab === 'chat'">
        <div class="search-bar">
          <el-input v-model="searchText" placeholder="Search" prefix-icon="Search" size="small" style="flex: 1; margin-right: 5px;" />
          <el-button circle size="small" :icon="Plus" @click="showAddFriend" />
        </div>
        <div class="session-list">
          <div 
            v-for="session in sessions" 
            :key="session.chatSessionId" 
            class="session-item"
            :class="{ active: currentSession?.chatSessionId === session.chatSessionId }"
            @click="selectSession(session)"
          >
            <el-avatar :size="40" shape="square" :src="session.avatar ? 'data:image/png;base64,' + session.avatar : defaultAvatar" />
            <div class="session-info">
              <div class="session-name">{{ session.chatSessionName || 'Unknown' }}</div>
              <div class="session-preview">{{ getLastMessagePreview(session) }}</div>
            </div>
          </div>
        </div>
      </div>

      <div v-else-if="activeTab === 'requests'" class="requests-view">
        <div class="sidebar-header">
          <h3>Friend Requests</h3>
        </div>
        <div class="requests-list">
          <div v-for="req in friendRequests" :key="req.userId" class="request-item">
            <el-avatar :size="40" shape="square" :src="req.avatar ? 'data:image/png;base64,' + req.avatar : defaultAvatar" />
            <div class="request-info">
              <div class="request-name">{{ req.name }}</div>
              <div class="request-actions">
                <el-button type="success" size="small" :icon="Check" circle @click="processRequest(req, true)" />
                <el-button type="danger" size="small" :icon="Close" circle @click="processRequest(req, false)" />
              </div>
            </div>
          </div>
          <el-empty v-if="friendRequests.length === 0" description="No requests" :image-size="60" />
        </div>
      </div>
    </div>

    <!-- Right Area: Chat Window -->
    <div class="chat-window" v-if="currentSession">
      <div class="chat-header">
        <h3>{{ currentSession.chatSessionName }}</h3>
      </div>
      
      <div class="message-list" ref="messageListRef">
        <div 
          v-for="msg in messages" 
          :key="msg.messageId" 
          class="message-item"
          :class="{ 'message-self': msg.sender.userId === currentUser.userId }"
        >
          <el-avatar :size="36" shape="square" :src="getSenderAvatar(msg)" class="msg-avatar" />
          <div class="msg-content-wrapper">
            <div class="msg-name" v-if="msg.sender.userId !== currentUser.userId">{{ msg.sender.name }}</div>
            <div class="msg-bubble">
              {{ getMessageContent(msg) }}
            </div>
          </div>
        </div>
      </div>

      <div class="chat-input">
        <div class="toolbar">
          <el-icon><Folder /></el-icon>
          <el-icon><Microphone /></el-icon>
        </div>
        <textarea 
          v-model="inputMessage" 
          @keydown.enter.prevent="sendMessage"
          placeholder="Type a message..."
        ></textarea>
        <div class="send-btn-wrapper">
          <el-button type="success" size="small" @click="sendMessage" :disabled="!inputMessage.trim()">Send</el-button>
        </div>
      </div>
    </div>
    
    <div class="empty-state" v-else>
      <el-empty description="Select a chat to start messaging" />
    </div>

    <!-- Profile Dialog -->
    <el-dialog v-model="profileVisible" title="My Profile" width="350px" center @close="resetProfileMode">
      <div class="profile-content">
        <div class="avatar-section">
          <el-upload
            v-if="isEditingProfile"
            class="avatar-uploader"
            action="#"
            :show-file-list="false"
            :auto-upload="false"
            :on-change="handleAvatarChange"
          >
            <div class="avatar-edit-overlay">
              <el-avatar :size="80" :src="editProfileForm.avatarUrl || defaultAvatar" />
              <div class="overlay-text">Change</div>
            </div>
          </el-upload>
          <el-avatar v-else :size="80" :src="userProfile.avatar ? 'data:image/png;base64,' + userProfile.avatar : defaultAvatar" />
        </div>

        <div v-if="!isEditingProfile" class="profile-info-view">
          <h3>{{ userProfile.name }}</h3>
          <p class="info-item"><span class="label">ID:</span> {{ userProfile.userId }}</p>
          <p class="info-item"><span class="label">Phone:</span> {{ userProfile.phone || 'Not set' }}</p>
          <p class="info-item"><span class="label">Desc:</span> {{ userProfile.description || 'No description' }}</p>
          <el-button type="primary" :icon="Edit" @click="startEditProfile" style="margin-top: 20px; width: 100%;">Edit Profile</el-button>
        </div>

        <div v-else class="profile-info-edit">
          <el-form :model="editProfileForm" label-width="60px">
            <el-form-item label="Name">
              <el-input v-model="editProfileForm.name" />
            </el-form-item>
            <el-form-item label="Desc">
              <el-input v-model="editProfileForm.description" type="textarea" :rows="2" />
            </el-form-item>
          </el-form>
          <div class="edit-actions">
            <el-button @click="cancelEditProfile">Cancel</el-button>
            <el-button type="primary" @click="saveProfile" :loading="savingProfile">Save</el-button>
          </div>
        </div>
      </div>
    </el-dialog>

    <!-- Add Friend Dialog -->
    <el-dialog v-model="addFriendVisible" title="Add Friend" width="400px">
      <div class="add-friend-content">
        <el-input 
          v-model="searchUserKey" 
          placeholder="Search by username or phone" 
          class="input-with-select"
          @keyup.enter="searchUser"
        >
          <template #append>
            <el-button :icon="Search" @click="searchUser" />
          </template>
        </el-input>
        
        <div v-if="searchResults.length > 0" class="search-results">
          <div v-for="user in searchResults" :key="user.userId" class="user-result-item">
            <el-avatar :size="36" :src="user.avatar ? 'data:image/png;base64,' + user.avatar : defaultAvatar" />
            <div class="user-result-info">
              <div class="name">{{ user.name }}</div>
              <div class="id">ID: {{ user.userId }}</div>
            </div>
            <el-button type="primary" size="small" @click="addFriend(user)">Add</el-button>
          </div>
        </div>
        <div v-else-if="searchPerformed" class="no-results">
          No users found.
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, watch, reactive } from 'vue';
import { ChatDotRound, User, Search, Folder, Microphone, Plus, Edit, Check, Close } from '@element-plus/icons-vue';
import { sendRequest } from '../api/client';
import { getProtoType } from '../api/proto';
import { WebSocketClient } from '../api/ws';
import { ElMessage } from 'element-plus';

const props = defineProps({
  currentUser: {
    type: Object,
    required: true
  }
});

const defaultAvatar = 'https://cube.elemecdn.com/3/7c/3ea6beec64369c2642b92c6726f1epng.png';
const activeTab = ref('chat');
const searchText = ref('');
const sessions = ref([]);
const friendRequests = ref([]);
const currentSession = ref(null);
const messages = ref([]);
const inputMessage = ref('');
const messageListRef = ref(null);
let wsClient = null;

// Profile Logic
const profileVisible = ref(false);
const userProfile = ref({});
const isEditingProfile = ref(false);
const savingProfile = ref(false);
const editProfileForm = reactive({
  name: '',
  description: '',
  avatarUrl: '',
  avatarFile: null
});

const showProfile = async () => {
  profileVisible.value = true;
  isEditingProfile.value = false;
  try {
    const rsp = await sendRequest(
      '/user/get_user_info',
      'huzch.GetUserInfoReq',
      'huzch.GetUserInfoRsp',
      {
        userId: props.currentUser.userId,
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success && rsp.userInfo) {
      userProfile.value = rsp.userInfo;
    }
  } catch (e) {
    console.error("Failed to fetch profile", e);
  }
};

const resetProfileMode = () => {
  isEditingProfile.value = false;
};

const startEditProfile = () => {
  editProfileForm.name = userProfile.value.name;
  editProfileForm.description = userProfile.value.description;
  editProfileForm.avatarUrl = userProfile.value.avatar ? 'data:image/png;base64,' + btoa(String.fromCharCode(...new Uint8Array(userProfile.value.avatar))) : defaultAvatar;
  editProfileForm.avatarFile = null;
  isEditingProfile.value = true;
};

const cancelEditProfile = () => {
  isEditingProfile.value = false;
};

const handleAvatarChange = (file) => {
  editProfileForm.avatarUrl = URL.createObjectURL(file.raw);
  const reader = new FileReader();
  reader.onload = (e) => {
    editProfileForm.avatarFile = new Uint8Array(e.target.result);
  };
  reader.readAsArrayBuffer(file.raw);
};

const saveProfile = async () => {
  savingProfile.value = true;
  try {
    // Update Name
    if (editProfileForm.name !== userProfile.value.name) {
      await sendRequest(
        '/user/set_user_name',
        'huzch.SetUserNameReq',
        'huzch.SetUserNameRsp',
        {
          userId: props.currentUser.userId,
          userName: editProfileForm.name,
          loginSessionId: props.currentUser.sessionId
        }
      );
    }

    // Update Description
    if (editProfileForm.description !== userProfile.value.description) {
      await sendRequest(
        '/user/set_user_description',
        'huzch.SetUserDescriptionReq',
        'huzch.SetUserDescriptionRsp',
        {
          userId: props.currentUser.userId,
          description: editProfileForm.description,
          loginSessionId: props.currentUser.sessionId
        }
      );
    }

    // Update Avatar
    if (editProfileForm.avatarFile) {
      await sendRequest(
        '/user/set_user_avatar',
        'huzch.SetUserAvatarReq',
        'huzch.SetUserAvatarRsp',
        {
          userId: props.currentUser.userId,
          avatar: editProfileForm.avatarFile,
          loginSessionId: props.currentUser.sessionId
        }
      );
    }

    ElMessage.success('Profile updated');
    isEditingProfile.value = false;
    // Refresh profile
    await showProfile();
    
  } catch (e) {
    console.error("Failed to update profile", e);
    ElMessage.error('Failed to update profile');
  } finally {
    savingProfile.value = false;
  }
};


// Add Friend Logic
const addFriendVisible = ref(false);
const searchUserKey = ref('');
const searchResults = ref([]);
const searchPerformed = ref(false);

const showAddFriend = () => {
  addFriendVisible.value = true;
  searchUserKey.value = '';
  searchResults.value = [];
  searchPerformed.value = false;
};

const searchUser = async () => {
  if (!searchUserKey.value.trim()) return;
  
  try {
    const rsp = await sendRequest(
      '/user/user_search',
      'huzch.UserSearchReq',
      'huzch.UserSearchRsp',
      {
        searchKey: searchUserKey.value,
        userId: props.currentUser.userId,
        loginSessionId: props.currentUser.sessionId
      }
    );
    
    searchPerformed.value = true;
    if (rsp.success) {
      searchResults.value = rsp.usersInfo || [];
    } else {
      searchResults.value = [];
    }
  } catch (e) {
    console.error("Search failed", e);
    searchResults.value = [];
  }
};

const addFriend = async (user) => {
  try {
    const rsp = await sendRequest(
      '/friend/friend_add_send',
      'huzch.FriendAddSendReq',
      'huzch.FriendAddSendRsp',
      {
        userId: props.currentUser.userId,
        respondentId: user.userId,
        loginSessionId: props.currentUser.sessionId
      }
    );
    
    if (rsp.success) {
      ElMessage.success('Friend request sent');
      addFriendVisible.value = false;
    } else {
      ElMessage.error(rsp.errmsg || 'Failed to send request');
    }
  } catch (e) {
    console.error("Add friend failed", e);
    ElMessage.error('Network error');
  }
};

// Friend Requests Logic
const showFriendRequests = async () => {
  activeTab.value = 'requests';
  await fetchFriendRequests();
};

const fetchFriendRequests = async () => {
  try {
    const rsp = await sendRequest(
      '/friend/get_requester',
      'huzch.GetRequesterReq',
      'huzch.GetRequesterRsp',
      {
        userId: props.currentUser.userId,
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success) {
      friendRequests.value = rsp.requestersInfo || [];
    }
  } catch (e) {
    console.error("Failed to fetch friend requests", e);
  }
};

const processRequest = async (req, agree) => {
  try {
    const rsp = await sendRequest(
      '/friend/friend_add_process',
      'huzch.FriendAddProcessReq',
      'huzch.FriendAddProcessRsp',
      {
        userId: props.currentUser.userId,
        requesterId: req.userId,
        agree: agree,
        loginSessionId: props.currentUser.sessionId
      }
    );
    
    if (rsp.success) {
      ElMessage.success(agree ? 'Request accepted' : 'Request rejected');
      // Refresh list
      await fetchFriendRequests();
      if (agree) {
        // If agreed, refresh sessions too as a new chat might be created
        await fetchSessions();
      }
    } else {
      ElMessage.error(rsp.errmsg || 'Failed to process request');
    }
  } catch (e) {
    console.error("Process request failed", e);
    ElMessage.error('Network error');
  }
};

// Fetch sessions on mount
onMounted(async () => {
  await fetchSessions();
  initWebSocket();
});

const initWebSocket = () => {
  // Assuming WS port is 9001 based on gateway_server.cc
  // In dev, we might need to proxy this too or connect directly if CORS allows.
  // Since vite proxy only proxies HTTP, we connect directly to 9001.
  const wsUrl = `ws://${window.location.hostname}:9001`;
  wsClient = new WebSocketClient(wsUrl, props.currentUser.sessionId, handleWsMessage);
  wsClient.connect();
};

const handleWsMessage = async (data) => {
  try {
    const NotifyMessage = await getProtoType('huzch.NotifyMessage');
    const decoded = NotifyMessage.decode(new Uint8Array(data));
    const notify = NotifyMessage.toObject(decoded, {
      enums: String,
      bytes: String,
      longs: String,
      defaults: true
    });

    console.log("Received Notify:", notify);

    if (notify.notifyType === 'FRIEND_ADD_SEND_NOTIFY' || notify.notifyType === 0) {
      ElMessage.info('You have a new friend request');
      if (activeTab.value === 'requests') {
        await fetchFriendRequests();
      }
    } else if (notify.notifyType === 'FRIEND_ADD_PROCESS_NOTIFY' || notify.notifyType === 1) {
      const info = notify.friendAddProcess;
      if (info && info.agree) {
        ElMessage.success(`${info.userInfo.name} accepted your friend request`);
        await fetchSessions();
      } else {
        ElMessage.warning(`${info.userInfo.name} rejected your friend request`);
      }
    } else if (notify.notifyType === 'FRIEND_REMOVE_NOTIFY' || notify.notifyType === 2) {
      ElMessage.info('A friend was removed');
      await fetchSessions();
    } else if (notify.notifyType === 'CHAT_SESSION_CREATE_NOTIFY' || notify.notifyType === 3) {
      // New chat session created (e.g. after friend request accepted)
      await fetchSessions();
    } else if (notify.notifyType === 'CHAT_MESSAGE_NOTIFY' || notify.notifyType === 4 ) {
      const msgInfo = notify.newMessageInfo ? notify.newMessageInfo.messageInfo : null;
      if (!msgInfo) return;

      // Update session list preview
      const session = sessions.value.find(s => s.chatSessionId === msgInfo.chatSessionId);
      if (session) {
        session.prevMessage = msgInfo;
        // Move session to top
        const index = sessions.value.indexOf(session);
        if (index > 0) {
          sessions.value.splice(index, 1);
          sessions.value.unshift(session);
        }
      }

      // If it's the current session, add to message list
      if (currentSession.value && currentSession.value.chatSessionId === msgInfo.chatSessionId) {
        // Check if message already exists (to avoid duplicate from sendMessage)
        if (!messages.value.find(m => m.messageId === msgInfo.messageId)) {
          messages.value.push(msgInfo);
          scrollToBottom();
        }
      }
    }
  } catch (e) {
    console.error("Failed to handle WS message", e);
  }
};

const fetchSessions = async () => {
  try {
    const rsp = await sendRequest(
      '/friend/get_chat_session',
      'huzch.GetChatSessionReq',
      'huzch.GetChatSessionRsp',
      {
        userId: props.currentUser.userId,
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success) {
      sessions.value = rsp.chatSessionsInfo || [];
    }
  } catch (e) {
    console.error("Failed to fetch sessions", e);
  }
};

const selectSession = async (session) => {
  currentSession.value = session;
  messages.value = []; // Clear previous messages
  await fetchMessages(session.chatSessionId);
};

const fetchMessages = async (sessionId) => {
  try {
    const rsp = await sendRequest(
      '/message/get_recent_message',
      'huzch.GetRecentMessageReq',
      'huzch.GetRecentMessageRsp',
      {
        chatSessionId: sessionId,
        msgCount: 50,
        userId: props.currentUser.userId,
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success) {
      messages.value = (rsp.messagesInfo || []).reverse();
      scrollToBottom();
    }
  } catch (e) {
    console.error("Failed to fetch messages", e);
  }
};

const sendMessage = async () => {
  if (!inputMessage.value.trim() || !currentSession.value) return;

  const content = inputMessage.value;
  inputMessage.value = ''; // Clear input immediately

  try {
    const rsp = await sendRequest(
      '/forward/new_message',
      'huzch.NewMessageReq',
      'huzch.NewMessageRsp',
      {
        userId: props.currentUser.userId,
        chatSessionId: currentSession.value.chatSessionId,
        loginSessionId: props.currentUser.sessionId,
        message: {
          messageType: 0,
          stringMessage: {
            content: content
          }
        }
      }
    );

    if (rsp.success) {
      // Optimistically append message or wait for WS?
      // Usually wait for WS or response.
      // The response contains message_info.
      if (rsp.messageInfo) {
        // Update session list preview
        const session = sessions.value.find(s => s.chatSessionId === currentSession.value.chatSessionId);
        if (session) {
          session.prevMessage = rsp.messageInfo;
          // Move to top
          const index = sessions.value.indexOf(session);
          if (index > 0) {
            sessions.value.splice(index, 1);
            sessions.value.unshift(session);
          }
        }
        messages.value.push(rsp.messageInfo);
        scrollToBottom();
      }
    } else {
      ElMessage.error(rsp.errmsg || 'Failed to send');
    }
  } catch (e) {
    console.error("Send error", e);
    ElMessage.error('Send failed');
  }
};

const scrollToBottom = () => {
  nextTick(() => {
    if (messageListRef.value) {
      messageListRef.value.scrollTop = messageListRef.value.scrollHeight;
    }
  });
};

const getLastMessagePreview = (session) => {
  if (!session.prevMessage) return '';
  const msg = session.prevMessage.message;
  if (msg.stringMessage) return msg.stringMessage.content;
  if (msg.imageMessage) return '[Image]';
  if (msg.fileMessage) return '[File]';
  if (msg.speechMessage) return '[Voice]';
  return '';
};

const getMessageContent = (msg) => {
  if (!msg.message) return '';
  if (msg.message.stringMessage) return msg.message.stringMessage.content;
  return '[Unsupported Message Type]';
};

const getSenderAvatar = (msg) => {
  if (msg.sender && msg.sender.avatar) {
    return 'data:image/png;base64,' + msg.sender.avatar;
  }
  return defaultAvatar; 
};

</script>

<style scoped>
.chat-container {
  display: flex;
  height: 100vh;
  width: 100vw;
  background-color: #f5f5f5;
  overflow: hidden;
}

.nav-sidebar {
  width: 60px;
  background-color: #2e2e2e;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding-top: 20px;
}

.avatar-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 5px;
}

.current-username {
  color: #999;
  font-size: 12px;
  text-align: center;
  max-width: 50px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.nav-icons {
  margin-top: 20px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.nav-icon {
  color: #999;
  font-size: 24px;
  cursor: pointer;
}

.nav-icon.active, .nav-icon:hover {
  color: #07c160;
}

.list-sidebar {
  width: 250px;
  background-color: #e6e6e6;
  border-right: 1px solid #d6d6d6;
  display: flex;
  flex-direction: column;
}

.search-bar {
  padding: 15px;
  background-color: #f7f7f7;
  display: flex;
  align-items: center;
}

.profile-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.search-results {
  margin-top: 20px;
}

.user-result-item {
  display: flex;
  align-items: center;
  padding: 10px;
  border-bottom: 1px solid #eee;
}

.user-result-info {
  flex: 1;
  margin-left: 10px;
}

.user-result-info .name {
  font-weight: bold;
}

.user-result-info .id {
  font-size: 12px;
  color: #999;
}

.no-results {
  margin-top: 20px;
  text-align: center;
  color: #999;
}

.session-list {
  flex: 1;
  overflow-y: auto;
}

.session-item {
  display: flex;
  padding: 12px;
  cursor: pointer;
  transition: background 0.2s;
}

.session-item:hover {
  background-color: #d9d9d9;
}

.session-item.active {
  background-color: #c6c6c6;
}

.session-info {
  margin-left: 10px;
  flex: 1;
  overflow: hidden;
}

.session-name {
  font-weight: 500;
  margin-bottom: 4px;
}

.session-preview {
  font-size: 12px;
  color: #999;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.requests-view {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.sidebar-header {
  padding: 15px;
  background-color: #f7f7f7;
  border-bottom: 1px solid #d6d6d6;
}

.sidebar-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
}

.requests-list {
  flex: 1;
  overflow-y: auto;
}

.request-item {
  display: flex;
  padding: 12px;
  border-bottom: 1px solid #dcdcdc;
  align-items: center;
}

.request-info {
  margin-left: 10px;
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.request-name {
  font-weight: 500;
}

.request-actions {
  display: flex;
  gap: 10px;
}

.chat-window {
  flex: 1;
  display: flex;
  flex-direction: column;
  background-color: #f5f5f5;
}

.chat-header {
  height: 60px;
  border-bottom: 1px solid #e7e7e7;
  display: flex;
  align-items: center;
  padding: 0 20px;
  background-color: #f5f5f5;
}

.message-list {
  flex: 1;
  padding: 20px;
  overflow-y: auto;
}

.message-item {
  display: flex;
  margin-bottom: 20px;
}

.message-self {
  flex-direction: row-reverse;
}

.msg-avatar {
  margin: 0 10px;
}

.msg-content-wrapper {
  max-width: 70%;
}

.msg-name {
  font-size: 12px;
  color: #999;
  margin-bottom: 4px;
  text-align: left;
}

.message-self .msg-name {
  text-align: right;
}

.msg-bubble {
  background-color: white;
  padding: 10px 14px;
  border-radius: 4px;
  position: relative;
  word-wrap: break-word;
}

.message-self .msg-bubble {
  background-color: #95ec69;
}

.chat-input {
  height: 150px;
  border-top: 1px solid #e7e7e7;
  background-color: white;
  display: flex;
  flex-direction: column;
}

.toolbar {
  padding: 10px 20px;
  display: flex;
  gap: 15px;
  color: #666;
}

textarea {
  flex: 1;
  border: none;
  resize: none;
  padding: 0 20px;
  outline: none;
  font-family: inherit;
}

.send-btn-wrapper {
  padding: 10px 20px;
  text-align: right;
}

.empty-state {
  flex: 1;
  display: flex;
  justify-content: center;
  align-items: center;
  background-color: #f5f5f5;
}

.profile-info-view {
  width: 100%;
  text-align: center;
}

.profile-info-edit {
  width: 100%;
}

.info-item {
  margin: 5px 0;
  color: #666;
  font-size: 14px;
}

.info-item .label {
  font-weight: bold;
  color: #333;
  margin-right: 5px;
}

.avatar-section {
  position: relative;
  cursor: pointer;
}

.avatar-edit-overlay {
  position: relative;
  display: inline-block;
}

.avatar-edit-overlay:hover .overlay-text {
  opacity: 1;
}

.overlay-text {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  color: white;
  display: flex;
  justify-content: center;
  align-items: center;
  border-radius: 50%;
  opacity: 0;
  transition: opacity 0.2s;
  font-size: 12px;
}

.edit-actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  margin-top: 20px;
}
</style>
