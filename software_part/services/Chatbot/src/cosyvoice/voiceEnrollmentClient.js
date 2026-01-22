const axios = require('axios');

const CUSTOMIZATION_URL = 'https://dashscope.aliyuncs.com/api/v1/services/audio/tts/customization';

const callVoiceEnrollment = async ({ apiKey, input, timeoutMs }) => {
  if (!apiKey) {
    const error = new Error('未配置 DashScope API Key（COSYVOICE_API_KEY 或 DASHSCOPE_API_KEY）。');
    error.code = 'NO_API_KEY';
    throw error;
  }
  if (!input || typeof input !== 'object') {
    const error = new Error('请求参数 input 格式错误。');
    error.code = 'INVALID_INPUT';
    throw error;
  }

  try {
    const response = await axios.post(
      CUSTOMIZATION_URL,
      {
        model: 'voice-enrollment',
        input,
      },
      {
        timeout: timeoutMs,
        headers: {
          Authorization: `Bearer ${apiKey}`,
          'Content-Type': 'application/json',
        },
      }
    );

    return response.data;
  } catch (error) {
    // Avoid accidentally leaking credentials if callers log the axios error object.
    if (error?.config?.headers) {
      try {
        const headers = error.config.headers;
        if (typeof headers.Authorization === 'string') headers.Authorization = 'Bearer ***';
        if (typeof headers.authorization === 'string') headers.authorization = 'Bearer ***';
      } catch {
        // ignore
      }
    }
    throw error;
  }
};

const createVoice = async (
  { apiKey, targetModel, prefix, url, languageHints },
  { timeoutMs } = {}
) => {
  const input = {
    action: 'create_voice',
    target_model: targetModel,
    prefix,
    url,
  };
  if (Array.isArray(languageHints) && languageHints.length) {
    input.language_hints = languageHints;
  }
  return callVoiceEnrollment({ apiKey, input, timeoutMs });
};

const listVoices = async ({ apiKey, prefix, pageIndex, pageSize }, { timeoutMs } = {}) => {
  const input = {
    action: 'list_voice',
    prefix,
    page_index: pageIndex,
    page_size: pageSize,
  };
  return callVoiceEnrollment({ apiKey, input, timeoutMs });
};

const queryVoice = async ({ apiKey, voiceId }, { timeoutMs } = {}) => {
  const input = {
    action: 'query_voice',
    voice_id: voiceId,
  };
  return callVoiceEnrollment({ apiKey, input, timeoutMs });
};

const updateVoice = async ({ apiKey, voiceId, url }, { timeoutMs } = {}) => {
  const input = {
    action: 'update_voice',
    voice_id: voiceId,
    url,
  };
  return callVoiceEnrollment({ apiKey, input, timeoutMs });
};

const deleteVoice = async ({ apiKey, voiceId }, { timeoutMs } = {}) => {
  const input = {
    action: 'delete_voice',
    voice_id: voiceId,
  };
  return callVoiceEnrollment({ apiKey, input, timeoutMs });
};

module.exports = {
  CUSTOMIZATION_URL,
  createVoice,
  listVoices,
  queryVoice,
  updateVoice,
  deleteVoice,
};
