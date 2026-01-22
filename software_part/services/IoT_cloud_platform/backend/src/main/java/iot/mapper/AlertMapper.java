package iot.mapper;

import iot.domain.Alert;
import java.time.LocalDateTime;
import java.util.List;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

@Mapper
public interface AlertMapper {

    List<Alert> findAll(@Param("status") String status,
                        @Param("level") String level,
                        @Param("limit") Integer limit,
                        @Param("offset") Integer offset);

    Long countAll(@Param("status") String status, @Param("level") String level);

    Alert findById(@Param("id") Long id);

    List<Alert> findByDeviceId(@Param("deviceId") String deviceId);

    void insert(Alert alert);

    void updateStatus(@Param("id") Long id, @Param("status") String status);

    void acknowledge(@Param("id") Long id, @Param("acknowledgedAt") LocalDateTime acknowledgedAt);

    void close(@Param("id") Long id, @Param("closedAt") LocalDateTime closedAt);

    void deleteById(@Param("id") Long id);

    Alert findOpenByRuleAndDevice(@Param("ruleId") Long ruleId, @Param("deviceId") String deviceId);

    Long countOpenByDevice(@Param("deviceId") String deviceId);

    List<Alert> findAllForExport(@Param("status") String status,
                                 @Param("level") String level,
                                 @Param("deviceId") String deviceId,
                                 @Param("from") LocalDateTime from,
                                 @Param("to") LocalDateTime to);
}
