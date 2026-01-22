package iot.mapper;

import iot.domain.Device;
import java.time.LocalDateTime;
import java.util.List;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

@Mapper
public interface DeviceMapper {

    List<Device> findAll();

    List<Device> findByOwnerId(@Param("ownerId") Long ownerId);

    Device findByDeviceId(@Param("deviceId") String deviceId);

    void insert(Device device);

    void updateStatus(@Param("deviceId") String deviceId,
                      @Param("status") String status,
                      @Param("lastSeenAt") LocalDateTime lastSeenAt);

    void update(Device device);

    void deleteByDeviceId(@Param("deviceId") String deviceId);
}
