package iot.controller;

import iot.domain.User;
import iot.domain.UserDeletionRequest;
import iot.dto.DeletionRequestDTO;
import iot.dto.DeletionRequestResponse;
import iot.service.UserDeletionRequestService;
import iot.service.UserService;
import jakarta.validation.Valid;
import java.util.List;
import java.util.stream.Collectors;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

/**
 * REST controller for managing user deletion requests
 */
@RestController
@RequestMapping("/api/deletion-requests")
public class UserDeletionRequestController {

    private final UserDeletionRequestService deletionRequestService;
    private final UserService userService;

    public UserDeletionRequestController(
            UserDeletionRequestService deletionRequestService,
            UserService userService) {
        this.deletionRequestService = deletionRequestService;
        this.userService = userService;
    }

    /**
     * Create a new user deletion request (Secondary admin only)
     *
     * @param request Deletion request containing target user ID and reason
     * @return Created deletion request response
     */
    @PostMapping
    @PreAuthorize("hasAuthority('ROLE_SECONDARY_ADMIN')")
    public ResponseEntity<DeletionRequestResponse> createRequest(
            @Valid @RequestBody DeletionRequestDTO request) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();
        User currentUser = userService.findByUsername(currentUsername).orElse(null);

        if (currentUser == null) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        UserDeletionRequest created = deletionRequestService.createRequest(
                request.getTargetUserId(),
                currentUser.getId(),
                request.getReason()
        );

        return ResponseEntity.status(HttpStatus.CREATED).body(toResponse(created));
    }

    /**
     * Get all pending deletion requests (Primary admin only)
     *
     * @return List of pending deletion requests
     */
    @GetMapping("/pending")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<List<DeletionRequestResponse>> getPendingRequests() {
        List<DeletionRequestResponse> requests = deletionRequestService
                .getPendingRequests(1, 100)
                .stream()
                .map(this::toResponse)
                .collect(Collectors.toList());
        return ResponseEntity.ok(requests);
    }

    /**
     * Get deletion requests created by the current user (Secondary admin only)
     *
     * @return List of own deletion requests
     */
    @GetMapping("/my-requests")
    @PreAuthorize("hasAuthority('ROLE_SECONDARY_ADMIN')")
    public ResponseEntity<List<DeletionRequestResponse>> getMyRequests() {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();
        User currentUser = userService.findByUsername(currentUsername).orElse(null);

        if (currentUser == null) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        List<DeletionRequestResponse> requests = deletionRequestService
                .getRequestsByRequester(currentUser.getId(), 1, 100)
                .stream()
                .map(this::toResponse)
                .collect(Collectors.toList());
        return ResponseEntity.ok(requests);
    }

    /**
     * Approve a deletion request (Primary admin only)
     *
     * @param id Deletion request ID
     * @return No content on success
     */
    @PostMapping("/{id}/approve")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<Void> approveRequest(@PathVariable Long id) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();
        User currentUser = userService.findByUsername(currentUsername).orElse(null);

        if (currentUser == null) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        deletionRequestService.approveRequest(id, currentUser.getId());
        return ResponseEntity.noContent().build();
    }

    /**
     * Reject a deletion request (Primary admin only)
     *
     * @param id Deletion request ID
     * @return No content on success
     */
    @PostMapping("/{id}/reject")
    @PreAuthorize("hasAuthority('ROLE_PRIMARY_ADMIN')")
    public ResponseEntity<Void> rejectRequest(@PathVariable Long id) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        String currentUsername = authentication.getName();
        User currentUser = userService.findByUsername(currentUsername).orElse(null);

        if (currentUser == null) {
            return ResponseEntity.status(HttpStatus.FORBIDDEN).build();
        }

        deletionRequestService.rejectRequest(id, currentUser.getId());
        return ResponseEntity.noContent().build();
    }

    /**
     * Convert UserDeletionRequest domain object to response DTO
     *
     * @param request Domain object
     * @return Response DTO with user details
     */
    private DeletionRequestResponse toResponse(UserDeletionRequest request) {
        DeletionRequestResponse response = new DeletionRequestResponse();
        response.setId(request.getId());
        response.setTargetUserId(request.getTargetUserId());
        response.setRequesterId(request.getRequesterId());
        response.setStatus(request.getStatus());
        response.setReason(request.getReason());
        response.setReviewedBy(request.getReviewedBy());
        response.setReviewedAt(request.getReviewedAt());
        response.setCreatedAt(request.getCreatedAt());

        // Populate user details
        userService.findById(request.getTargetUserId()).ifPresent(targetUser -> {
            response.setTargetUsername(targetUser.getUsername());
            response.setTargetEmail(targetUser.getEmail());
        });

        userService.findById(request.getRequesterId()).ifPresent(requester -> {
            response.setRequesterUsername(requester.getUsername());
            response.setRequesterEmail(requester.getEmail());
        });

        if (request.getReviewedBy() != null) {
            userService.findById(request.getReviewedBy()).ifPresent(reviewer -> {
                response.setReviewerUsername(reviewer.getUsername());
            });
        }

        return response;
    }
}
