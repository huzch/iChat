import axios from 'axios';
import { getProtoType } from './proto';

// 生成 Request ID 的辅助函数
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

    // 如果没有则添加 request_id
    if (!payload.request_id) {
      payload.request_id = generateRequestId();
    }

    // 校验负载
    const errMsg = ReqType.verify(payload);
    if (errMsg) throw Error(errMsg);

    // 编码
    const buffer = ReqType.encode(ReqType.create(payload)).finish();

    // 转换为 Blob 确保作为二进制数据发送
    const blob = new Blob([buffer], { type: 'application/x-protobuf' });

    // 发送
    const response = await apiClient.post(url, blob);

    // 解码
    const decoded = RspType.decode(new Uint8Array(response.data));
    return RspType.toObject(decoded, {
      longs: String,
      enums: String,
      bytes: String,
    });
  } catch (error) {
    console.error(`API 错误 [${url}]:`, error);
    throw error;
  }
};
