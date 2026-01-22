package iot.mapper;

import iot.domain.UserDeletionRequest;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import java.time.LocalDateTime;
import java.util.List;

@Mapper
public interface UserDeletionRequestMapper {

    void insert(UserDeletionRequest request);

    UserDeletionRequest findById(@Param("id") Long id);

    List<UserDeletionRequest> findAll(@Param("offset") int offset, @Param("limit") int limit);

    List<UserDeletionRequest> findByStatus(@Param("status") String status,
                                           @Param("offset") int offset,
                                           @Param("limit") int limit);

    List<UserDeletionRequest> findByRequester(@Param("requesterId") Long requesterId,
                                              @Param("offset") int offset,
                                              @Param("limit") int limit);

    long countByStatus(@Param("status") String status);

    void updateStatus(@Param("id") Long id,
                     @Param("status") String status,
                     @Param("reviewedBy") Long reviewedBy,
                     @Param("reviewedAt") LocalDateTime reviewedAt);

    void deleteById(@Param("id") Long id);
}
