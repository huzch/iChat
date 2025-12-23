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
      // Handle incoming messages
      // We need to know the message type. 
      // Usually, the backend sends a specific proto.
      // Based on gateway.proto, it might be just forwarding messages.
      // Let's assume it sends 'huzch.NotifyMessage' or similar.
      // I need to check notify.proto.
      if (this.onMessage) {
        this.onMessage(event.data);
      }
    };

    this.ws.onclose = () => {
      console.log('WebSocket disconnected');
      this.isConnected = false;
    };

    this.ws.onerror = (error) => {
      console.error('WebSocket error', error);
    };
  }

  async authenticate() {
    const AuthReq = await getProtoType('huzch.ClientAuthenticationReq');
    const payload = {
      requestId: Date.now().toString(),
      loginSessionId: this.sessionId
    };
    const buffer = AuthReq.encode(AuthReq.create(payload)).finish();
    // Send as Blob to ensure binary transmission
    this.ws.send(new Blob([buffer]));
  }

  send(data) {
    if (this.isConnected) {
      this.ws.send(data);
    }
  }
}
