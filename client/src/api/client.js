import axios from 'axios';
import { getProtoType } from './proto';

// Helper to generate UUID
const generateRequestId = () => Math.random().toString(36).substring(2) + Date.now().toString(36);

const apiClient = axios.create({
  baseURL: '/service',
  headers: {
    'Content-Type': 'application/x-protobuf'
  },
  responseType: 'arraybuffer'
});

export const sendRequest = async (url, reqTypeName, rspTypeName, payload) => {
  try {
    const ReqType = await getProtoType(reqTypeName);
    const RspType = await getProtoType(rspTypeName);

    // Add request_id if not present
    if (!payload.request_id) {
      payload.request_id = generateRequestId();
    }

    // Verify payload
    const errMsg = ReqType.verify(payload);
    if (errMsg) throw Error(errMsg);

    // Encode
    const buffer = ReqType.encode(ReqType.create(payload)).finish();

    // Convert to Blob to ensure it is sent as binary data, not JSON
    const blob = new Blob([buffer], { type: 'application/x-protobuf' });

    // Send
    const response = await apiClient.post(url, blob);

    // Decode
    const decoded = RspType.decode(new Uint8Array(response.data));
    return RspType.toObject(decoded, {
      longs: String,
      enums: String,
      bytes: String,
    });
  } catch (error) {
    console.error(`API Error [${url}]:`, error);
    throw error;
  }
};
