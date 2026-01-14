import { getProtoType } from './proto';

export class WebSocketClient {
  constructor(url, sessionId, onMessage) {
    this.url = url;
    this.sessionId = sessionId;
    this.onMessage = onMessage;
    this.ws = null;
    this.isConnected = false;
  }

  connect() {
    this.ws = new WebSocket(this.url);
    this.ws.binaryType = 'arraybuffer';

    this.ws.onopen = async () => {
      console.log('WebSocket connected');
      this.isConnected = true;
      await this.authenticate();
    };

    this.ws.onmessage = async (event) => {
      // 处理接收到的消息
      if (this.onMessage) {
        this.onMessage(event.data);
      }
    };

    this.ws.onclose = () => {
      console.log('WebSocket 连接断开');
      this.isConnected = false;
    };

    this.ws.onerror = (error) => {
      console.error('WebSocket 错误', error);
    };
  }

  async authenticate() {
    const AuthReq = await getProtoType('huzch.ClientAuthenticationReq');
    const payload = {
      requestId: Date.now().toString(),
      loginSessionId: this.sessionId
    };
    const buffer = AuthReq.encode(AuthReq.create(payload)).finish();
    // 以 Blob 形式发送确保二进制传输
    this.ws.send(new Blob([buffer]));
  }

  send(data) {
    if (this.isConnected) {
      this.ws.send(data);
    }
  }
}
