package iot.service;

import iot.domain.MusicTrack;
import iot.dto.MusicTrackRequest;
import iot.mapper.MusicTrackMapper;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Optional;

@Service
public class MusicTrackService {
    private final MusicTrackMapper musicTrackMapper;

    public MusicTrackService(MusicTrackMapper musicTrackMapper) {
        this.musicTrackMapper = musicTrackMapper;
    }

    public List<MusicTrack> findAll() {
        return musicTrackMapper.findAll();
    }

    public List<MusicTrack> search(String keyword) {
        if (keyword == null || keyword.trim().isEmpty()) {
            return findAll();
        }
        return musicTrackMapper.findByTitleContaining(keyword.trim());
    }

    public Optional<MusicTrack> findById(Long id) {
        MusicTrack track = musicTrackMapper.findById(id);
        return Optional.ofNullable(track);
    }

    @Transactional
    public MusicTrack create(MusicTrackRequest request) {
        MusicTrack track = new MusicTrack();
        track.setTitle(request.getTitle());
        track.setUrl(request.getUrl());
        track.setTrackIndex(request.getTrackIndex());
        musicTrackMapper.insert(track);
        return track;
    }

    @Transactional
    public MusicTrack update(Long id, MusicTrackRequest request) {
        MusicTrack track = musicTrackMapper.findById(id);
        if (track == null) {
            throw new RuntimeException("音乐不存在");
        }
        track.setTitle(request.getTitle());
        track.setUrl(request.getUrl());
        track.setTrackIndex(request.getTrackIndex());
        musicTrackMapper.update(track);
        return track;
    }

    @Transactional
    public void delete(Long id) {
        musicTrackMapper.deleteById(id);
    }
}
