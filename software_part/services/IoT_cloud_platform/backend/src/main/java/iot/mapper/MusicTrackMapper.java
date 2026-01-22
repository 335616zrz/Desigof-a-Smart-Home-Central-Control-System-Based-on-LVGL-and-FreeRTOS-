package iot.mapper;

import iot.domain.MusicTrack;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

import java.util.List;

@Mapper
public interface MusicTrackMapper {
    List<MusicTrack> findAll();
    List<MusicTrack> findByTitleContaining(@Param("keyword") String keyword);
    MusicTrack findById(@Param("id") Long id);
    void insert(MusicTrack track);
    void update(MusicTrack track);
    void deleteById(@Param("id") Long id);
}
