package iot.dto;

import jakarta.validation.constraints.NotBlank;

public class MusicTrackRequest {
    @NotBlank(message = "标题不能为空")
    private String title;

    @NotBlank(message = "URL不能为空")
    private String url;

    private Integer trackIndex;

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public Integer getTrackIndex() {
        return trackIndex;
    }

    public void setTrackIndex(Integer trackIndex) {
        this.trackIndex = trackIndex;
    }
}
