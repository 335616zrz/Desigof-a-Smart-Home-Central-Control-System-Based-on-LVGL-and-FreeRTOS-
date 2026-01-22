package iot.service;

/**
 * Season detection service for seasonal alert rule filtering.
 *
 * <p>Season definition:
 * <ul>
 *   <li>summer: 6-8</li>
 *   <li>winter: 12-2</li>
 *   <li>transition: 3-5 and 9-11</li>
 * </ul>
 */
public interface SeasonService {

    /**
     * Get current season based on current month.
     * @return "summer", "winter", "transition"
     */
    String getCurrentSeason();

    /**
     * Get season by month.
     * @param month month value (1-12)
     * @return "summer", "winter", "transition"
     */
    String getSeasonByMonth(int month);
}

