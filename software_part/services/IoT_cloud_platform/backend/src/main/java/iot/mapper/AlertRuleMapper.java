package iot.mapper;

import iot.domain.AlertRule;
import java.util.List;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

@Mapper
public interface AlertRuleMapper {

    List<AlertRule> findAll(@Param("enabled") Boolean enabled);

    AlertRule findById(@Param("id") Long id);

    List<AlertRule> findByDeviceId(@Param("deviceId") String deviceId);

    List<AlertRule> findEnabledRules();

    void insert(AlertRule rule);

    void update(AlertRule rule);

    void deleteById(@Param("id") Long id);

    void updateEnabled(@Param("id") Long id, @Param("enabled") Boolean enabled);

    List<AlertRule> findActiveRulesBySeason(@Param("season") String season,
                                            @Param("enabled") Boolean enabled);

    List<AlertRule> findSystemTemplates(@Param("season") String season);

    List<AlertRule> findUserRules(@Param("deviceId") String deviceId);

    int deleteByIdIfNotSystem(@Param("id") Long id);
}
