package iot.service.impl;

import iot.domain.User;
import iot.domain.UserDeletionRequest;
import iot.mapper.UserDeletionRequestMapper;
import iot.mapper.UserMapper;
import iot.service.UserDeletionRequestService;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import java.time.LocalDateTime;
import java.util.List;

@Service
@Transactional(readOnly = true)
public class UserDeletionRequestServiceImpl implements UserDeletionRequestService {

    private final UserDeletionRequestMapper deletionRequestMapper;
    private final UserMapper userMapper;

    public UserDeletionRequestServiceImpl(UserDeletionRequestMapper deletionRequestMapper,
                                         UserMapper userMapper) {
        this.deletionRequestMapper = deletionRequestMapper;
        this.userMapper = userMapper;
    }

    @Override
    @Transactional
    public UserDeletionRequest createRequest(Long targetUserId, Long requesterId, String reason) {
        // Validate target user exists
        User targetUser = userMapper.findById(targetUserId);
        if (targetUser == null) {
            throw new RuntimeException("Target user not found with id: " + targetUserId);
        }

        // Validate requester exists and is a secondary admin
        User requester = userMapper.findById(requesterId);
        if (requester == null) {
            throw new RuntimeException("Requester not found with id: " + requesterId);
        }
        if (!"ROLE_SECONDARY_ADMIN".equals(requester.getRole())) {
            throw new RuntimeException("Only secondary admins can request user deletions");
        }

        // Validate that the target user is managed by this secondary admin
        if (!requesterId.equals(targetUser.getManagerId())) {
            throw new RuntimeException("Secondary admin can only request deletion of users they manage");
        }

        // Prevent deletion of protected users
        if (Boolean.TRUE.equals(targetUser.getIsProtected())) {
            throw new RuntimeException("Cannot request deletion of protected user account");
        }

        // Prevent deletion of admin accounts
        if ("ROLE_PRIMARY_ADMIN".equals(targetUser.getRole()) ||
            "ROLE_SECONDARY_ADMIN".equals(targetUser.getRole())) {
            throw new RuntimeException("Cannot request deletion of admin accounts");
        }

        // Create the deletion request
        UserDeletionRequest request = new UserDeletionRequest();
        request.setTargetUserId(targetUserId);
        request.setRequesterId(requesterId);
        request.setStatus("pending");
        request.setReason(reason);
        request.setCreatedAt(LocalDateTime.now());

        deletionRequestMapper.insert(request);
        return request;
    }

    @Override
    @Transactional
    public void approveRequest(Long requestId, Long reviewerId) {
        // Validate request exists
        UserDeletionRequest request = deletionRequestMapper.findById(requestId);
        if (request == null) {
            throw new RuntimeException("Deletion request not found with id: " + requestId);
        }

        // Validate request is still pending
        if (!"pending".equals(request.getStatus())) {
            throw new RuntimeException("Request has already been processed with status: " + request.getStatus());
        }

        // Validate reviewer exists and is a primary admin
        User reviewer = userMapper.findById(reviewerId);
        if (reviewer == null) {
            throw new RuntimeException("Reviewer not found with id: " + reviewerId);
        }
        if (!"ROLE_PRIMARY_ADMIN".equals(reviewer.getRole())) {
            throw new RuntimeException("Only primary admins can approve deletion requests");
        }

        // Validate target user still exists and is not protected
        User targetUser = userMapper.findById(request.getTargetUserId());
        if (targetUser == null) {
            throw new RuntimeException("Target user no longer exists");
        }
        if (Boolean.TRUE.equals(targetUser.getIsProtected())) {
            throw new RuntimeException("Cannot delete protected user account");
        }

        // Update request status
        deletionRequestMapper.updateStatus(requestId, "approved", reviewerId, LocalDateTime.now());

        // Delete the user
        userMapper.deleteById(request.getTargetUserId());
    }

    @Override
    @Transactional
    public void rejectRequest(Long requestId, Long reviewerId) {
        // Validate request exists
        UserDeletionRequest request = deletionRequestMapper.findById(requestId);
        if (request == null) {
            throw new RuntimeException("Deletion request not found with id: " + requestId);
        }

        // Validate request is still pending
        if (!"pending".equals(request.getStatus())) {
            throw new RuntimeException("Request has already been processed with status: " + request.getStatus());
        }

        // Validate reviewer exists and is a primary admin
        User reviewer = userMapper.findById(reviewerId);
        if (reviewer == null) {
            throw new RuntimeException("Reviewer not found with id: " + reviewerId);
        }
        if (!"ROLE_PRIMARY_ADMIN".equals(reviewer.getRole())) {
            throw new RuntimeException("Only primary admins can reject deletion requests");
        }

        // Update request status
        deletionRequestMapper.updateStatus(requestId, "rejected", reviewerId, LocalDateTime.now());
    }

    @Override
    public List<UserDeletionRequest> getPendingRequests(int page, int pageSize) {
        int limit = Math.max(pageSize, 1);
        int offset = Math.max(page - 1, 0) * limit;
        return deletionRequestMapper.findByStatus("pending", offset, limit);
    }

    @Override
    public List<UserDeletionRequest> getRequestsByRequester(Long requesterId, int page, int pageSize) {
        int limit = Math.max(pageSize, 1);
        int offset = Math.max(page - 1, 0) * limit;
        return deletionRequestMapper.findByRequester(requesterId, offset, limit);
    }

    @Override
    public long countPendingRequests() {
        return deletionRequestMapper.countByStatus("pending");
    }
}
