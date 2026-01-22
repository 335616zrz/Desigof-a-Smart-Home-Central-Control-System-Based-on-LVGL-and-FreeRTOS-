import api from './http';

export interface MusicTrack {
  /** URL-encoded filename; safe as a path segment */
  id: string;
  title: string;
  /** Playable URL (mp3/m4a/aac direct, other formats -> cached mp3 when ready). */
  url: string | null;
  duration?: string | null;
  format?: string | null;
  filename?: string | null;
  sourceUrl?: string | null;
  cacheReady?: boolean;
}

export interface MusicTrackRequest {
  filename: string;
  title?: string;
}

export const musicApi = {
  async list(keyword?: string): Promise<MusicTrack[]> {
    const { data } = await api.get<MusicTrack[]>('/music', {
      params: keyword ? { keyword } : undefined,
    });
    return data;
  },

  async get(id: string): Promise<MusicTrack> {
    const { data } = await api.get<MusicTrack>(`/music/${id}`);
    return data;
  },

  async create(payload: MusicTrackRequest): Promise<MusicTrack> {
    const { data } = await api.post<MusicTrack>('/music', payload);
    return data;
  },

  async update(id: string, payload: { title?: string; duration?: string }): Promise<MusicTrack> {
    const { data } = await api.put<MusicTrack>(`/music/${id}`, payload);
    return data;
  },

  async delete(id: string): Promise<void> {
    await api.delete(`/music/${id}`);
  },

  async files(): Promise<Array<{ id: string; filename: string; format: string; size: number; indexed: boolean }>> {
    const { data } = await api.get<Array<{ id: string; filename: string; format: string; size: number; indexed: boolean }>>('/music/files');
    return data;
  },

  async unindexed(): Promise<Array<{ id: string; filename: string; format: string; size: number; indexed: boolean }>> {
    const { data } = await api.get<Array<{ id: string; filename: string; format: string; size: number; indexed: boolean }>>('/music/unindexed');
    return data;
  },

  async batchAdd(filenames: string[]): Promise<{ added: number; errors: Record<string, string> }> {
    const { data } = await api.post<{ added: number; errors: Record<string, string> }>('/music/batch-add', filenames);
    return data;
  },

  async batchTranscode(): Promise<{ queued: number }> {
    const { data } = await api.post<{ queued: number }>('/music/batch-transcode');
    return data;
  },

  async transcodeStatus(): Promise<Array<{ filename: string; status: string; message?: string }>> {
    const { data } = await api.get<Array<{ filename: string; status: string; message?: string }>>('/music/transcode-status');
    return data;
  }
};
