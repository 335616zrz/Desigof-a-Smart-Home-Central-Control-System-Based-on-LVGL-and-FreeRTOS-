package iot.mapper;

import iot.domain.User;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import java.util.List;

@Mapper
public interface UserMapper {

    User findByUsername(@Param("username") String username);

    void insert(User user);

    List<User> findAll(@Param("offset") int offset, @Param("limit") int limit);

    long countAll();

    void deleteById(@Param("id") Long id);

    void deleteByIds(@Param("ids") List<Long> ids);

    void upsertAdmin(@Param("username") String username,
                     @Param("passwordHash") String passwordHash,
                     @Param("email") String email,
                     @Param("role") String role);

    User findById(@Param("id") Long id);

    long countByRole(@Param("role") String role);

    void updateRole(@Param("id") Long id, @Param("role") String role);

    void updateManagerId(@Param("id") Long id, @Param("managerId") Long managerId);

    User findFirstByRole(@Param("role") String role);

    List<User> findByManagerId(@Param("managerId") Long managerId);

    long countByManagerId(@Param("managerId") Long managerId);

    List<User> findSecondaryAdminsWithSpace(@Param("maxGroupSize") int maxGroupSize);
}
