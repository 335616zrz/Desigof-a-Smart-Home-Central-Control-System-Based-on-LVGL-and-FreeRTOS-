package iot.mapper;

import iot.domain.DeviceCredential;
import java.time.LocalDateTime;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;

@Mapper
public interface DeviceCredentialMapper {

    DeviceCredential findActiveByDeviceAndKey(@Param("deviceId") String deviceId,
                                              @Param("credentialKey") String credentialKey);

    DeviceCredential findActiveByDevice(@Param("deviceId") String deviceId);

    void deactivateAll(@Param("deviceId") String deviceId);

    void insert(DeviceCredential credential);

    void markUsed(@Param("deviceId") String deviceId,
                  @Param("credentialKey") String credentialKey,
                  @Param("active") boolean active,
                  @Param("expiresAt") LocalDateTime expiresAt);
}
