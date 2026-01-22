package iot.service;

import iot.service.impl.SeasonServiceImpl;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

class SeasonServiceTest {

    private final SeasonService seasonService = new SeasonServiceImpl();

    @Test
    void testSummerSeason() {
        assertEquals("summer", seasonService.getSeasonByMonth(6));
        assertEquals("summer", seasonService.getSeasonByMonth(7));
        assertEquals("summer", seasonService.getSeasonByMonth(8));
    }

    @Test
    void testWinterSeason() {
        assertEquals("winter", seasonService.getSeasonByMonth(12));
        assertEquals("winter", seasonService.getSeasonByMonth(1));
        assertEquals("winter", seasonService.getSeasonByMonth(2));
    }

    @Test
    void testTransitionSeason() {
        assertEquals("transition", seasonService.getSeasonByMonth(3));
        assertEquals("transition", seasonService.getSeasonByMonth(4));
        assertEquals("transition", seasonService.getSeasonByMonth(5));
        assertEquals("transition", seasonService.getSeasonByMonth(9));
        assertEquals("transition", seasonService.getSeasonByMonth(10));
        assertEquals("transition", seasonService.getSeasonByMonth(11));
    }

    @Test
    void testInvalidMonth() {
        assertThrows(IllegalArgumentException.class, () -> seasonService.getSeasonByMonth(0));
        assertThrows(IllegalArgumentException.class, () -> seasonService.getSeasonByMonth(13));
    }
}

