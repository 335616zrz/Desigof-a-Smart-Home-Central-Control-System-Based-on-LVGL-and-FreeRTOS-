package iot.dto;

public class SeasonResponse {

    private String season;
    private String seasonName;

    public SeasonResponse() {
    }

    public SeasonResponse(String season, String seasonName) {
        this.season = season;
        this.seasonName = seasonName;
    }

    public String getSeason() {
        return season;
    }

    public void setSeason(String season) {
        this.season = season;
    }

    public String getSeasonName() {
        return seasonName;
    }

    public void setSeasonName(String seasonName) {
        this.seasonName = seasonName;
    }
}

