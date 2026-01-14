<template>
  <div class="chat-container">
    <!-- 左侧侧边栏：导航 -->
    <div class="nav-sidebar">
      <div class="avatar-wrapper" @click="showProfile" style="cursor: pointer;">
        <el-avatar :size="40" shape="square" :src="userProfile.avatar ? 'data:image/png;base64,' + userProfile.avatar : (currentUser.avatarUrl || defaultAvatar)" />
        <div class="current-username">{{ userProfile.name || currentUser.username }}</div>
      </div>
      <div class="nav-icons">
        <el-icon 
          class="nav-icon" 
          :class="{ active: activeTab === 'chat' }" 
          @click="activeTab = 'chat'"
        ><ChatDotRound /></el-icon>
        <el-icon 
          class="nav-icon" 
          :class="{ active: activeTab === 'friends' }" 
          @click="showFriends"
        ><User /></el-icon>
        <el-icon 
          class="nav-icon" 
          :class="{ active: activeTab === 'requests' }" 
          @click="showFriendRequests"
        ><Bell /></el-icon>
      </div>
    </div>

    <!-- 中间侧边栏：会话列表、好友或申请 -->
    <div class="list-sidebar">
      <div class="search-bar">
        <el-input v-model="searchText" placeholder="搜索" prefix-icon="Search" size="small" style="flex: 1; margin-right: 5px;" />
        <el-dropdown trigger="click">
          <el-button circle size="small" :icon="Plus" />
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item :icon="User" @click="showAddFriend">添加好友</el-dropdown-item>
              <el-dropdown-item :icon="CirclePlus" @click="showCreateGroup">创建群聊</el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>

      <div v-if="activeTab === 'chat'" class="sessions-view">
        <div class="session-list">
          <div 
            v-for="session in filteredSessions" 
            :key="session.chatSessionId" 
            class="session-item"
            :class="{ active: currentSession?.chatSessionId === session.chatSessionId }"
            @click="selectSession(session)"
          >
            <el-avatar :size="40" shape="square" :src="session.avatar ? 'data:image/png;base64,' + session.avatar : defaultAvatar" />
            <div class="session-info">
              <div class="session-name">{{ session.chatSessionName || '未知' }}</div>
              <div class="session-preview">{{ getLastMessagePreview(session) }}</div>
            </div>
          </div>
          <el-empty v-if="filteredSessions.length === 0" description="暂无会话" :image-size="60" />
        </div>
      </div>

      <div v-else-if="activeTab === 'friends'" class="friends-view">
        <div class="friends-list">
          <div 
            v-for="friend in filteredFriends" 
            :key="friend.userId" 
            class="friend-item"
            @click="startChat(friend)"
          >
            <el-avatar :size="40" shape="square" :src="friend.avatar ? 'data:image/png;base64,' + friend.avatar : defaultAvatar" />
            <div class="friend-info">
              <div class="friend-name">{{ friend.name }}</div>
            </div>
          </div>
          <el-empty v-if="filteredFriends.length === 0" description="暂无好友" :image-size="60" />
        </div>
      </div>

      <div v-else-if="activeTab === 'requests'" class="requests-view">
        <div class="requests-list">
          <div v-for="req in filteredRequests" :key="req.userId" class="request-item">
            <el-avatar :size="40" shape="square" :src="req.avatar ? 'data:image/png;base64,' + req.avatar : defaultAvatar" />
            <div class="request-info">
              <div class="request-name">{{ req.name }}</div>
              <div class="request-actions">
                <el-button type="success" size="small" :icon="Check" circle @click="processRequest(req, true)" />
                <el-button type="danger" size="small" :icon="Close" circle @click="processRequest(req, false)" />
              </div>
            </div>
          </div>
          <el-empty v-if="filteredRequests.length === 0" description="暂无申请" :image-size="60" />
        </div>
      </div>
    </div>

    <!-- 右侧：聊天窗口 -->
    <div class="chat-window" v-if="currentSession">
      <div class="chat-header">
        <h3>{{ currentSession.chatSessionName }}</h3>
      </div>
      
      <div class="message-list" ref="messageListRef" @click="closeContextMenu">
        <div 
          v-for="msg in messages" 
          :key="msg.messageId" 
          class="message-item"
          :class="{ 'message-self': msg.sender.userId === currentUserId }"
        >
          <el-avatar :size="36" shape="square" :src="getSenderAvatar(msg)" class="msg-avatar" />
          <div class="msg-content-wrapper">
            <div class="msg-name" v-if="msg.sender.userId !== currentUserId">{{ msg.sender.name }}</div>
            <div class="msg-bubble" @contextmenu.prevent="onContextMenu($event, msg)">
              <template v-if="msg.message.messageType === 0 || msg.message.messageType === 'STRING'">
                {{ getMessageContent(msg) }}
              </template>
              <template v-else-if="msg.message.messageType === 1 || msg.message.messageType === 'SPEECH'">
                <div class="voice-msg-wrapper">
                  <div class="voice-msg" :class="{ 'playing': playingMessageId == msg.messageId }" @click="playVoice(msg)">
                    <div class="voice-icon-wrapper">
                      <div class="voice-wave"></div>
                      <div class="voice-wave delay-1"></div>
                      <div class="voice-wave delay-2"></div>
                    </div>
                    <span class="voice-duration">语音</span>
                  </div>
                  <div v-show="msg.recognizedText" class="recognized-text">
                    {{ msg.recognizedText }}
                  </div>
                </div>
              </template>
              <template v-else-if="msg.message.messageType === 2 || msg.message.messageType === 'IMAGE'">
                <el-image 
                  class="img-msg"
                  :src="'data:image/png;base64,' + msg.message.imageMessage.fileContent" 
                  :preview-src-list="['data:image/png;base64,' + msg.message.imageMessage.fileContent]"
                  fit="contain"
                  preview-teleported
                />
              </template>
              <template v-else-if="msg.message.messageType === 3 || msg.message.messageType === 'FILE'">
                <div class="file-msg" @click="downloadFile(msg)">
                  <el-icon class="file-icon"><Document /></el-icon>
                  <div class="file-details">
                    <div class="file-name">{{ msg.message.fileMessage.fileName }}</div>
                    <div class="file-size">{{ formatFileSize(msg.message.fileMessage.fileSize) }}</div>
                  </div>
                </div>
              </template>
              <template v-else>
                {{ getMessageContent(msg) }}
              </template>
            </div>
          </div>
        </div>
      </div>

      <div class="chat-input">
        <div class="toolbar">
          <input 
            type="file" 
            ref="fileInputRef" 
            style="display: none" 
            @change="handleFileChange"
          />
          <input 
            type="file" 
            ref="imageInputRef" 
            style="display: none" 
            accept="image/*"
            @change="handleImageChange"
          />
          <el-icon class="toolbar-icon" @click="triggerImageUpload"><Picture /></el-icon>
          <el-icon class="toolbar-icon" @click="triggerFileUpload"><Folder /></el-icon>
          <el-tooltip content="按住录音" placement="top">
            <el-icon 
              class="toolbar-icon" 
              :class="{ 'recording-active': isRecording }"
              @mousedown="startRecording"
              @mouseleave="stopRecording"
              @mouseup="stopRecording"
            ><Microphone /></el-icon>
          </el-tooltip>
          <span v-if="isRecording" class="recording-tip">正在录音...</span>
        </div>
        <textarea 
          v-model="inputMessage" 
          @keydown.enter.prevent="sendMessage"
          placeholder="请输入消息..."
        ></textarea>
        <div class="send-btn-wrapper">
          <el-button type="success" size="small" @click="sendMessage" :disabled="!inputMessage.trim()">发送</el-button>
        </div>
      </div>
    </div>
    
    <div class="empty-state" v-else>
      <el-empty description="请选择一个聊天开始沟通" />
    </div>

    <!-- 个人资料对话框 -->
    <el-dialog v-model="profileVisible" title="个人资料" width="350px" center @close="resetProfileMode">
      <div class="profile-content">
        <div class="avatar-section">
          <el-upload
            class="avatar-uploader"
            action="#"
            :show-file-list="false"
            :auto-upload="false"
            :on-change="handleAvatarChange"
          >
            <div class="avatar-edit-overlay">
              <el-avatar :size="80" shape="square" :src="editProfileForm.avatarUrl || (userProfile.avatar ? 'data:image/png;base64,' + userProfile.avatar : defaultAvatar)" />
              <div class="overlay-text">更换头像</div>
            </div>
          </el-upload>
        </div>

        <div v-if="!isEditingProfile" class="profile-info-view">
          <h3>{{ userProfile.name }}</h3>
          <p class="info-item"><span class="label">ID:</span> {{ userProfile.userId }}</p>
          <p class="info-item"><span class="label">手机号:</span> {{ userProfile.phone || '未绑定' }}</p>
          <p class="info-item"><span class="label">简介:</span> {{ userProfile.description || '暂无简介' }}</p>
          <el-button type="primary" :icon="Edit" @click="startEditProfile" style="margin-top: 20px; width: 100%;">编辑资料</el-button>
        </div>

        <div v-else class="profile-info-edit">
          <el-form :model="editProfileForm" label-width="60px">
            <el-form-item label="昵称">
              <el-input v-model="editProfileForm.name" />
            </el-form-item>
            <el-form-item label="简介">
              <el-input v-model="editProfileForm.description" type="textarea" :rows="2" />
            </el-form-item>
          </el-form>
          <div class="edit-actions">
            <el-button @click="cancelEditProfile">取消</el-button>
            <el-button type="primary" @click="saveProfile" :loading="savingProfile">保存</el-button>
          </div>
        </div>
      </div>
    </el-dialog>

    <!-- 添加好友对话框 -->
    <el-dialog v-model="addFriendVisible" title="添加好友" width="400px">
      <div class="add-friend-content">
        <el-input 
          v-model="searchUserKey" 
          placeholder="搜用户名或手机号" 
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
            <el-button type="primary" size="small" @click="addFriend(user)">添加</el-button>
          </div>
        </div>
        <div v-else-if="searchPerformed" class="no-results">
          未找到相关用户
        </div>
      </div>
    </el-dialog>

    <!-- 创建群聊对话框 -->
    <el-dialog v-model="createGroupVisible" title="发起群聊" width="450px" destroy-on-close>
      <el-form :model="createGroupForm" label-width="100px">
        <el-form-item label="群聊名称" required>
          <el-input v-model="createGroupForm.name" placeholder="请输入群聊名称" />
        </el-form-item>
        <el-form-item label="选择成员" required>
          <div class="member-selector">
            <el-checkbox-group v-model="createGroupForm.selectedMembers">
              <div v-for="friend in friends" :key="friend.userId" class="member-option">
                <el-checkbox :label="friend.userId">
                  <div class="member-item-content">
                    <el-avatar :size="24" :src="friend.avatar ? 'data:image/png;base64,' + friend.avatar : defaultAvatar" />
                    <span>{{ friend.name }}</span>
                  </div>
                </el-checkbox>
              </div>
            </el-checkbox-group>
            <el-empty v-if="friends.length === 0" description="暂无可选好友" :image-size="40" />
          </div>
        </el-form-item>
      </el-form>
      <template #footer>
        <div class="dialog-footer">
          <el-button @click="createGroupVisible = false">取消</el-button>
          <el-button type="primary" @click="createGroup" :loading="creatingGroup" :disabled="!createGroupForm.name || createGroupForm.selectedMembers.length === 0">
            创建
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 右键菜单 -->
    <div v-if="contextMenu.show" class="msg-context-menu" :style="{ top: contextMenu.y + 'px', left: contextMenu.x + 'px' }">
      <div v-if="contextMenu.msg.message.messageType === 1 || contextMenu.msg.message.messageType === 'SPEECH'" 
           class="menu-item" @click="handleRecognizeMenu">转文字</div>
      <div class="menu-item" @click="copyText(contextMenu.msg)">复制文本</div>
      <div class="menu-item delete" @click="deleteMsg(contextMenu.msg)">删除</div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, watch, reactive, computed } from 'vue';
import { ChatDotRound, User, Search, Folder, Microphone, Plus, Edit, Check, Close, Bell, CirclePlus, Document, Picture } from '@element-plus/icons-vue';
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
const filteredSessions = computed(() => {
  if (!searchText.value) return sessions.value;
  const keyword = searchText.value.toLowerCase();
  return sessions.value.filter(s => 
    (s.chatSessionName || '').toLowerCase().includes(keyword)
  );
});

const friends = ref([]);
const filteredFriends = computed(() => {
  if (!searchText.value) return friends.value;
  const keyword = searchText.value.toLowerCase();
  return friends.value.filter(f => 
    (f.name || '').toLowerCase().includes(keyword) || 
    (f.phone || '').toLowerCase().includes(keyword)
  );
});

const friendRequests = ref([]);
const filteredRequests = computed(() => {
  if (!searchText.value) return friendRequests.value;
  const keyword = searchText.value.toLowerCase();
  return friendRequests.value.filter(r => 
    (r.name || '').toLowerCase().includes(keyword) || 
    (r.phone || '').toLowerCase().includes(keyword)
  );
});

const currentSession = ref(null);
const messages = ref([]);
const inputMessage = ref('');
const messageListRef = ref(null);
const fileInputRef = ref(null);
const imageInputRef = ref(null);
let wsClient = null;

// 个人资料显隐
const profileVisible = ref(false);
const userProfile = ref({});
const currentUserId = computed(() => userProfile.value?.userId || props.currentUser.userId);
const isEditingProfile = ref(false);
const savingProfile = ref(false);
const editProfileForm = reactive({
  name: '',
  description: '',
  avatarUrl: '',
  avatarFile: null
});

// 文件发送逻辑
const triggerFileUpload = () => {
  fileInputRef.value.click();
};

const handleFileChange = async (event) => {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = async (e) => {
    const arrayBuffer = e.target.result;
    const uint8Array = new Uint8Array(arrayBuffer);
    await sendFileMessage(file.name, file.size, uint8Array);
  };
  reader.readAsArrayBuffer(file);
  // 重置 input 以允许再次选择同一文件
  event.target.value = '';
};

const triggerImageUpload = () => {
  imageInputRef.value.click();
};

const handleImageChange = async (event) => {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = async (e) => {
    const uint8Array = new Uint8Array(e.target.result);
    await sendImageMessage(uint8Array);
  };
  reader.readAsArrayBuffer(file);
  event.target.value = '';
};

const sendImageMessage = async (content) => {
  if (!currentSession.value) return;
  try {
    const rsp = await sendRequest(
      '/forward/new_message',
      'huzch.NewMessageReq',
      'huzch.NewMessageRsp',
      {
        chatSessionId: currentSession.value.chatSessionId,
        userId: currentUserId.value,
        message: {
          messageType: 2, // IMAGE
          imageMessage: {
            fileContent: content
          }
        },
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (!rsp.success) {
      ElMessage.error('图片发送失败');
    }
  } catch (e) {
    console.error("发送图片失败", e);
  }
};

const sendFileMessage = async (name, size, content) => {
  if (!currentSession.value) return;

  try {
    const rsp = await sendRequest(
      '/forward/new_message',
      'huzch.NewMessageReq',
      'huzch.NewMessageRsp',
      {
        userId: currentUserId.value,
        chatSessionId: currentSession.value.chatSessionId,
        loginSessionId: props.currentUser.sessionId,
        message: {
          messageType: 3, // FILE
          fileMessage: {
            fileName: name,
            fileSize: size,
            fileContent: content
          }
        }
      }
    );
    if (rsp.success) {
      await fetchMessages(currentSession.value.chatSessionId);
    } else {
      ElMessage.error(rsp.errmsg || '发送文件失败');
    }
  } catch (e) {
    console.error("发送文件失败", e);
    ElMessage.error('网络错误');
  }
};

const downloadFile = async (msg) => {
  const fileMsg = msg.message.fileMessage;
  if (!fileMsg) return;

  let content = fileMsg.fileContent;
  
  // 如果消息中不含内容，则需要从文件服务下载
  if (!content && fileMsg.fileId) {
    try {
      const rsp = await sendRequest(
        '/file/get_single_file',
        'huzch.GetSingleFileReq',
        'huzch.GetSingleFileRsp',
        {
          fileId: fileMsg.fileId,
          userId: currentUserId.value,
          loginSessionId: props.currentUser.sessionId
        }
      );
      if (rsp.success && rsp.fileData) {
        content = rsp.fileData.fileContent;
      } else {
        ElMessage.error('无法下载文件');
        return;
      }
    } catch (e) {
      console.error("下载文件失败", e);
      return;
    }
  }

  if (!content) return;

  let blob;
  if (typeof content === 'string') {
    const binary = atob(content);
    const array = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      array[i] = binary.charCodeAt(i);
    }
    blob = new Blob([array]);
  } else {
    blob = new Blob([content]);
  }

  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = fileMsg.fileName || 'download';
  a.click();
  URL.revokeObjectURL(url);
};

const formatFileSize = (size) => {
  if (size < 1024) return size + ' B';
  if (size < 1024 * 1024) return (size / 1024).toFixed(2) + ' KB';
  return (size / (1024 * 1024)).toFixed(2) + ' MB';
};

// 录音相关
const isRecording = ref(false);
let mediaRecorder = null;
let audioChunks = [];
const playingMessageId = ref(null);

// 右键菜单状态
const contextMenu = reactive({
  show: false,
  x: 0,
  y: 0,
  msg: null
});

const onContextMenu = (event, msg) => {
  contextMenu.show = true;
  contextMenu.x = event.clientX;
  contextMenu.y = event.clientY;
  contextMenu.msg = msg;
};

const closeContextMenu = () => {
  contextMenu.show = false;
};

const handleRecognizeMenu = () => {
  if (contextMenu.msg) {
    recognizeSpeech(contextMenu.msg);
  }
  closeContextMenu();
};

const copyText = (msg) => {
  const text = getMessageContent(msg);
  navigator.clipboard.writeText(text);
  ElMessage.success('已复制到剪贴板');
  closeContextMenu();
};

const deleteMsg = (msg) => {
  messages.value = messages.value.filter(m => m.messageId !== msg.messageId);
  closeContextMenu();
};

const startRecording = async () => {
  try {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
    mediaRecorder = new MediaRecorder(stream);
    audioChunks = [];
    mediaRecorder.ondataavailable = (event) => {
      audioChunks.push(event.data);
    };
    mediaRecorder.onstop = async () => {
      const audioBlob = new Blob(audioChunks, { type: 'audio/webm' });
      await sendVoiceMessage(audioBlob);
    };
    mediaRecorder.start();
    isRecording.value = true;
  } catch (err) {
    console.error("麦克风启动失败", err);
    ElMessage.error("麦克风不可用");
  }
};

const stopRecording = () => {
  if (mediaRecorder && isRecording.value) {
    mediaRecorder.stop();
    isRecording.value = false;
    mediaRecorder.stream.getTracks().forEach(track => track.stop());
  }
};

const sendVoiceMessage = async (blob) => {
  if (!currentSession.value) return;
  
  const arrayBuffer = await blob.arrayBuffer();
  const uint8Array = new Uint8Array(arrayBuffer);

  try {
    const rsp = await sendRequest(
      '/forward/new_message',
      'huzch.NewMessageReq',
      'huzch.NewMessageRsp',
      {
        userId: currentUserId.value,
        chatSessionId: currentSession.value.chatSessionId,
        loginSessionId: props.currentUser.sessionId,
        message: {
          messageType: 1, // SPEECH
          speechMessage: {
            fileContent: uint8Array
          }
        }
      }
    );
    if (rsp.success) {
      await fetchMessages(currentSession.value.chatSessionId);
    } else {
      ElMessage.error(rsp.errmsg || '发送语音失败');
    }
  } catch (e) {
    console.error("发送语音失败", e);
    ElMessage.error('网络错误');
  }
};

const playVoice = async (msg) => {
  // 立即开始动画，增加交互反馈
  playingMessageId.value = msg.messageId;
  
  let content = msg.message.speechMessage?.fileContent;
  const fileId = msg.message.speechMessage?.fileId;

  // 如果没有内容但有 fileId，则去文件服务下载
  if (!content && fileId) {
    try {
      const rsp = await sendRequest(
        '/file/get_single_file',
        'huzch.GetSingleFileReq',
        'huzch.GetSingleFileRsp',
        {
          fileId: fileId,
          userId: currentUserId.value,
          loginSessionId: props.currentUser.sessionId
        }
      );
      if (rsp.success && rsp.fileData) {
        content = rsp.fileData.fileContent;
        msg.message.speechMessage.fileContent = content; // 缓存
      }
    } catch (e) {
      console.error("下载语音文件失败", e);
    }
  }

  if (!content) {
    playingMessageId.value = null;
    ElMessage.warning('语音内容加载失败');
    return;
  }
  
  let blob;
  try {
    if (typeof content === 'string') {
      const binary = atob(content);
      const array = new Uint8Array(binary.length);
      for (let i = 0; i < binary.length; i++) array[i] = binary.charCodeAt(i);
      blob = new Blob([array], { type: 'audio/webm' });
    } else if (content instanceof Uint8Array || ArrayBuffer.isView(content)) {
      blob = new Blob([content], { type: 'audio/webm' });
    } else if (content.data && Array.isArray(content.data)) {
      // 处理某些序列化后的对象格式 {type: 'Buffer', data: [...]}
      blob = new Blob([new Uint8Array(content.data)], { type: 'audio/webm' });
    } else {
      blob = new Blob([content], { type: 'audio/webm' });
    }
  } catch (e) {
    console.error("处理语音数据失败", e);
    playingMessageId.value = null;
    return;
  }
  
  const url = URL.createObjectURL(blob);
  const audio = new Audio(url);
  
  audio.onended = () => {
    playingMessageId.value = null;
    URL.revokeObjectURL(url);
  };
  audio.onerror = () => {
    playingMessageId.value = null;
    URL.revokeObjectURL(url);
    ElMessage.error('播放失败');
  };
  
  try {
    await audio.play();
  } catch (err) {
    console.error("播放被拦截", err);
    // 某些浏览器在异步操作后会拦截 play()
    // 如果拦截了，点击一下可能还是没声音，但至少动画会消失
    playingMessageId.value = null;
    ElMessage.info('请再次点击播放（浏览器安全限制）');
  }
};

const recognizeSpeech = async (msg) => {
  let content = msg.message.speechMessage?.fileContent;
  
  // 如果没有内容但有 fileId，则去文件服务下载
  const fileId = msg.message.speechMessage?.fileId;
  if (!content && fileId) {
    try {
      const rsp = await sendRequest(
        '/file/get_single_file',
        'huzch.GetSingleFileReq',
        'huzch.GetSingleFileRsp',
        {
          fileId: fileId,
          userId: currentUserId.value,
          loginSessionId: props.currentUser.sessionId
        }
      );
      if (rsp.success && rsp.fileData) {
        content = rsp.fileData.fileContent;
        msg.message.speechMessage.fileContent = content;
      }
    } catch (e) {
      console.error("下载语音用于识别失败", e);
    }
  }

  if (!content) return;

  let uint8Array;
  if (typeof content === 'string') {
    const binary = atob(content);
    uint8Array = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
        uint8Array[i] = binary.charCodeAt(i);
    }
  } else {
    uint8Array = new Uint8Array(content);
  }

  try {
    const rsp = await sendRequest(
      '/speech/speech_recognize',
      'huzch.SpeechRecognizeReq',
      'huzch.SpeechRecognizeRsp',
      {
        speechContent: uint8Array,
        userId: currentUserId.value,
        loginSessionId: props.currentUser.sessionId
      }
    );

    if (rsp.success && rsp.recognitionResult) {
      msg.recognizedText = rsp.recognitionResult;
    } else {
      ElMessage.error(rsp.errmsg || '识别失败');
    }
  } catch (e) {
    console.error("语音识别失败", e);
    ElMessage.error('识别服务不可用');
  }
};

// 获取用户详细信息
const fetchUserProfile = async () => {
  try {
    const rsp = await sendRequest(
      '/user/get_user_info',
      'huzch.GetUserInfoReq',
      'huzch.GetUserInfoRsp',
      {
        userId: currentUserId.value, // 使用原始 ID 查找
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success && rsp.userInfo) {
      userProfile.value = rsp.userInfo;
      console.log("真实 userId 已确立:", userProfile.value.userId);
    }
  } catch (e) {
    console.error("确立真实 userId 失败", e);
  }
};

// 显示个人资料
const showProfile = async () => {
  profileVisible.value = true;
  isEditingProfile.value = false;
  await fetchUserProfile();
};

const resetProfileMode = () => {
  isEditingProfile.value = false;
};

// 开始编辑资料
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
    if (!isEditingProfile.value) {
      isEditingProfile.value = true;
      editProfileForm.name = userProfile.value.name;
      editProfileForm.description = userProfile.value.description;
    }
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
          userId: currentUserId.value,
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
          userId: currentUserId.value,
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
          userId: currentUserId.value,
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

// Create Group Logic
const createGroupVisible = ref(false);
const creatingGroup = ref(false);
const createGroupForm = reactive({
  name: '',
  selectedMembers: []
});

const showCreateGroup = async () => {
  createGroupForm.name = '';
  createGroupForm.selectedMembers = [];
  createGroupVisible.value = true;
  // Ensure friends are loaded
  if (friends.value.length === 0) {
    await fetchFriends();
  }
};

const createGroup = async () => {
  creatingGroup.value = true;
  try {
    const members = [...createGroupForm.selectedMembers, currentUserId.value];
    // console.log("Creating group with members:", members);
    // console.log("userid:", currentUserId.value);
    const rsp = await sendRequest(
      '/friend/chat_session_create',
      'huzch.ChatSessionCreateReq',
      'huzch.ChatSessionCreateRsp',
      {
        chatSessionName: createGroupForm.name,
        userId: currentUserId.value,
        loginSessionId: props.currentUser.sessionId,
        membersId: members
      }
    );

    if (rsp.success) {
      ElMessage.success('Group created');
      createGroupVisible.value = false;
      await fetchSessions();
      if (rsp.chatSessionInfo) {
        selectSession(rsp.chatSessionInfo);
      }
    } else {
      ElMessage.error(rsp.errmsg || 'Failed to create group');
    }
  } catch (e) {
    console.error("Create group failed", e);
    ElMessage.error('Network error');
  } finally {
    creatingGroup.value = false;
  }
};

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
        userId: currentUserId.value,
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
        userId: currentUserId.value,
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

// Friends Logic
const showFriends = async () => {
  activeTab.value = 'friends';
  await fetchFriends();
};

const fetchFriends = async () => {
  try {
    const rsp = await sendRequest(
      '/friend/get_friend',
      'huzch.GetFriendReq',
      'huzch.GetFriendRsp',
      {
        userId: currentUserId.value,
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success) {
      friends.value = rsp.friendsInfo || [];
    }
  } catch (e) {
    console.error("Failed to fetch friends", e);
  }
};

const startChat = async (friend) => {
  // Find session with this friend
  const session = sessions.value.find(s => s.singleChatFriendId === friend.userId);
  if (session) {
    activeTab.value = 'chat';
    selectSession(session);
  } else {
    // If session doesn't exist, we might need to create it, 
    // but the system creates it on friend acceptance.
    // If it's missing from 'sessions', maybe we need to refresh sessions.
    await fetchSessions();
    const refreshedSession = sessions.value.find(s => s.singleChatFriendId === friend.userId);
    if (refreshedSession) {
      activeTab.value = 'chat';
      selectSession(refreshedSession);
    } else {
      ElMessage.warning('Chat session not found');
    }
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
        userId: currentUserId.value,
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
        userId: currentUserId.value,
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
        // If agreed, refresh sessions and friends
        await fetchSessions();
        if (activeTab.value === 'friends') {
          await fetchFriends();
        }
      }
    } else {
      ElMessage.error(rsp.errmsg || 'Failed to process request');
    }
  } catch (e) {
    console.error("Process request failed", e);
    ElMessage.error('Network error');
  }
};

// 挂载时获取会话
onMounted(async () => {
  await fetchUserProfile(); // 先确立真实 UUID
  await fetchSessions();
  initWebSocket();
});

const initWebSocket = () => {
  // 建立 WebSocket 连接
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

    console.log("收到通知:", notify);

    if (notify.notifyType === 'FRIEND_ADD_SEND_NOTIFY' || notify.notifyType === 0) {
      ElMessage.info('收到新的好友申请');
      if (activeTab.value === 'requests') {
        await fetchFriendRequests();
      }
    } else if (notify.notifyType === 'FRIEND_ADD_PROCESS_NOTIFY' || notify.notifyType === 1) {
      const info = notify.friendAddProcess;
      if (info && info.agree) {
        ElMessage.success(`${info.userInfo.name} 通过了你的好友申请`);
        await fetchSessions();
        if (activeTab.value === 'friends') {
          await fetchFriends();
        }
      } else {
        ElMessage.warning(`${info.userInfo.name} 拒绝了你的好友申请`);
      }
    } else if (notify.notifyType === 'FRIEND_REMOVE_NOTIFY' || notify.notifyType === 2) {
      ElMessage.info('好友已被移除');
      await fetchSessions();
    } else if (notify.notifyType === 'CHAT_SESSION_CREATE_NOTIFY' || notify.notifyType === 3) {
      // 新会话创建通知（例如好友申请通过后）
      await fetchSessions();
    } else if (notify.notifyType === 'CHAT_MESSAGE_NOTIFY' || notify.notifyType === 4 ) {
      const msgInfo = notify.newMessageInfo ? notify.newMessageInfo.messageInfo : null;
      if (!msgInfo) return;

      // 更新会话列表预览
      const session = sessions.value.find(s => s.chatSessionId === msgInfo.chatSessionId);
      if (session) {
        session.prevMessage = msgInfo;
        // 将会话移至顶部
        const index = sessions.value.indexOf(session);
        if (index > 0) {
          sessions.value.splice(index, 1);
          sessions.value.unshift(session);
        }
      }

      // 如果是当前打开的会话，添加到消息列表
      if (currentSession.value && currentSession.value.chatSessionId === msgInfo.chatSessionId) {
        // 检查消息是否已存在（避免 sendMessage 后的重复）
        if (!messages.value.find(m => m.messageId === msgInfo.messageId)) {
          messages.value.push(msgInfo);
          scrollToBottom();
        }
      }
    }
  } catch (e) {
    console.error("处理 WS 消息失败", e);
  }
};

const fetchSessions = async () => {
  try {
    const rsp = await sendRequest(
      '/friend/get_chat_session',
      'huzch.GetChatSessionReq',
      'huzch.GetChatSessionRsp',
      {
        userId: currentUserId.value,
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
        userId: currentUserId.value,
        loginSessionId: props.currentUser.sessionId
      }
    );
    if (rsp.success) {
      messages.value = rsp.messagesInfo || [];
      scrollToBottom();
    }
  } catch (e) {
    console.error("Failed to fetch messages", e);
  }
};

const sendMessage = async () => {
  if (!inputMessage.value.trim() || !currentSession.value) return;

  const content = inputMessage.value;
  inputMessage.value = ''; // 立即清空输入框

  // 乐观 UI 更新：在服务器响应前先将消息推送到本地列表
  const optimisticMsg = {
    messageId: 'temp-' + Date.now(),
    chatSessionId: currentSession.value.chatSessionId,
    timestamp: Math.floor(Date.now() / 1000),
    sender: {
      userId: currentUserId.value,
      name: userProfile.value.name || props.currentUser.username,
      avatar: props.currentUser.avatarUrl ? props.currentUser.avatarUrl.replace('data:image/png;base64,', '') : null
    },
    message: {
      messageType: 0,
      stringMessage: {
        content: content
      }
    }
  };

  messages.value.push(optimisticMsg);

  // 立即更新侧边栏预览
  const session = sessions.value.find(s => s.chatSessionId === currentSession.value.chatSessionId);
  if (session) {
    session.prevMessage = optimisticMsg;
    // 移至顶部
    const index = sessions.value.indexOf(session);
    if (index > 0) {
      sessions.value.splice(index, 1);
      sessions.value.unshift(session);
    }
  }
  scrollToBottom();

  try {
    const rsp = await sendRequest(
      '/forward/new_message',
      'huzch.NewMessageReq',
      'huzch.NewMessageRsp',
      {
        userId: currentUserId.value,
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

    if (!rsp.success) {
      // 发送失败则移除乐观 UI 消息
      messages.value = messages.value.filter(m => m.messageId !== optimisticMsg.messageId);
      ElMessage.error(rsp.errmsg || '发送失败');
    }
  } catch (e) {
    console.error("发送错误", e);
    // 网络错误也移除消息
    messages.value = messages.value.filter(m => m.messageId !== optimisticMsg.messageId);
    ElMessage.error('网络错误，发送失败');
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
  if (msg.imageMessage) return '[图片]';
  if (msg.fileMessage) return '[文件]';
  if (msg.speechMessage) return '[语音]';
  return '';
};

const getMessageContent = (msg) => {
  if (!msg.message) return '';
  if (msg.message.stringMessage) return msg.message.stringMessage.content;
  return '[暂不支持的消息类型]';
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

.member-selector {
  border: 1px solid #dcdcdc;
  border-radius: 4px;
  max-height: 200px;
  overflow-y: auto;
  padding: 10px;
}

.member-option {
  margin-bottom: 5px;
}

.member-item-content {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-left: 8px;
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
  color: #000;
}

.session-preview {
  font-size: 12px;
  color: #999;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.sessions-view, .friends-view, .requests-view {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.friends-list {
  flex: 1;
  overflow-y: auto;
}

.friend-item {
  display: flex;
  padding: 12px;
  cursor: pointer;
  border-bottom: 1px solid #dcdcdc;
  align-items: center;
  transition: background 0.2s;
}

.friend-item:hover {
  background-color: #d9d9d9;
}

.friend-info {
  margin-left: 10px;
  flex: 1;
}

.friend-name {
  font-weight: 500;
  color: #000;
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
  color: #000;
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

.chat-header h3 {
  color: #000;
  margin: 0;
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
  background-color: #95ec69;
  color: #000;
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
  background-color: #f5f5f5;
  display: flex;
  flex-direction: column;
}

.toolbar {
  padding: 10px 20px;
  display: flex;
  gap: 15px;
  color: #666;
  align-items: center;
}

.toolbar-icon {
  cursor: pointer;
  font-size: 20px;
  transition: color 0.2s;
}

.toolbar-icon:hover {
  color: #07c160;
}

.recording-active {
  color: #f56c6c !important;
  animation: pulse 1.5s infinite;
}

.recording-tip {
  font-size: 12px;
  color: #f56c6c;
  margin-left: 5px;
}

@keyframes pulse {
  0% { opacity: 1; }
  50% { opacity: 0.5; }
  100% { opacity: 1; }
}

.voice-msg {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  min-width: 60px;
  max-width: 150px;
  height: 24px;
}

.voice-icon-wrapper {
  display: flex;
  align-items: center;
  gap: 2px;
  margin-right: 5px;
}

.voice-wave {
  width: 3px;
  height: 8px;
  background-color: #333;
  border-radius: 1px;
}

.voice-msg.playing .voice-wave {
  animation: voice-pulse 0.8s infinite ease-in-out;
}

.delay-1 { animation-delay: 0.15s !important; }
.delay-2 { animation-delay: 0.3s !important; }

@keyframes voice-pulse {
  0% { height: 6px; }
  50% { height: 16px; }
  100% { height: 6px; }
}

.voice-duration {
  font-size: 13px;
  color: #333;
}

.voice-msg-wrapper {
  display: flex;
  flex-direction: column;
}

.recognized-text {
  font-size: 13px;
  color: #333;
  padding: 8px;
  background: rgba(0, 0, 0, 0.05);
  border-radius: 4px;
  margin-top: 8px;
  word-break: break-all;
  border-left: 3px solid #07c160;
}

/* 右键菜单样式 */
.msg-context-menu {
  position: fixed;
  background: white;
  border: 1px solid #ddd;
  border-radius: 4px;
  box-shadow: 0 2px 10px rgba(0,0,0,0.1);
  padding: 5px 0;
  z-index: 9999;
  min-width: 100px;
}

.menu-item {
  padding: 8px 15px;
  font-size: 14px;
  cursor: pointer;
  transition: background 0.2s;
  color: #000;
}

.menu-item:hover {
  background: #f5f5f5;
}

.menu-item.delete {
  color: #f56c6c;
}

.voice-msg .el-icon {
  font-size: 18px;
}

.file-msg {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 8px;
  background: rgba(255, 255, 255, 0.8);
  border-radius: 6px;
  cursor: pointer;
  border: 1px solid #eee;
  min-width: 200px;
}

.file-icon {
  font-size: 32px;
  color: #409eff;
}

.file-details {
  flex: 1;
  overflow: hidden;
}

.file-name {
  font-size: 14px;
  font-weight: 500;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  color: #333;
}

.file-size {
  font-size: 12px;
  color: #999;
  margin-top: 2px;
}

.img-msg {
  max-width: 200px;
  max-height: 200px;
  border-radius: 4px;
  cursor: pointer;
  display: block;
}

textarea {
  flex: 1;
  border: none;
  resize: none;
  padding: 0 20px;
  outline: none;
  font-family: inherit;
  background-color: transparent;
  color: #000;
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
