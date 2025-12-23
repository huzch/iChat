import protobuf from 'protobufjs';

// Cache the root object
let root = null;

export const loadProto = async () => {
  if (root) return root;
  
  const files = [
    '/proto/base.proto',
    '/proto/user.proto',
    '/proto/gateway.proto',
    '/proto/friend.proto',
    '/proto/message.proto',
    '/proto/file.proto',
    '/proto/speech.proto',
    '/proto/notify.proto',
    '/proto/forward.proto'
  ];

  try {
    root = await protobuf.load(files);
    return root;
  } catch (err) {
    console.error("Failed to load protos", err);
    throw err;
  }
};

export const getProtoType = async (typeName) => {
  const r = await loadProto();
  return r.lookupType(typeName);
};
