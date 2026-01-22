package iot.service;

import iot.domain.UserDeletionRequest;
import java.util.List;

public interface UserDeletionRequestService {

    /**
     * Create a new user deletion request
     * @param targetUserId User to be deleted
     * @param requesterId Secondary admin requesting deletion
     * @param reason Reason for deletion
     * @return Created request
     */
    UserDeletionRequest createRequest(Long targetUserId, Long requesterId, String reason);

    /**
     * Approve a deletion request and delete the user
     * @param requestId Request ID
     * @param reviewerId Primary admin who approves
     */
    void approveRequest(Long requestId, Long reviewerId);

    /**
     * Reject a deletion request
     * @param requestId Request ID
     * @param reviewerId Primary admin who rejects
     */
    void rejectRequest(Long requestId, Long reviewerId);

    /**
     * Get all pending deletion requests
     * @param page Page number (1-based)
     * @param pageSize Items per page
     * @return List of pending requests
     */
    List<UserDeletionRequest> getPendingRequests(int page, int pageSize);

    /**
     * Get deletion requests by requester
     * @param requesterId Requester user ID
     * @param page Page number (1-based)
     * @param pageSize Items per page
     * @return List of requests
     */
    List<UserDeletionRequest> getRequestsByRequester(Long requesterId, int page, int pageSize);

    /**
     * Count pending requests
     * @return Number of pending requests
     */
    long countPendingRequests();
}
