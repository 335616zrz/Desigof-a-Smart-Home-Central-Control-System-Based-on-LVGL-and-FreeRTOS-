package iot.service.impl;

import iot.service.SeasonService;
import java.time.LocalDateTime;
import org.springframework.stereotype.Service;

@Service
public class SeasonServiceImpl implements SeasonService {

    @Override
    public String getCurrentSeason() {
        int currentMonth = LocalDateTime.now().getMonthValue();
        return getSeasonByMonth(currentMonth);
    }

    @Override
    public String getSeasonByMonth(int month) {
        if (month < 1 || month > 12) {
            throw new IllegalArgumentException("Invalid month: " + month);
        }

        if (month >= 6 && month <= 8) {
            return "summer";
        }
        if (month == 12 || month <= 2) {
            return "winter";
        }
        return "transition";
    }
}

