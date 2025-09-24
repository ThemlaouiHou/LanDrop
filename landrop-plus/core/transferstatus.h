#ifndef TRANSFERSTATUS_H
#define TRANSFERSTATUS_H

/**
 * @enum TransferStatus
 * @brief Describes the current state of a file transfer.
 */
enum class TransferStatus {
    WAITING,      ///< Transfer is waiting for confirmation or not started
    IN_PROGRESS,  ///< Transfer is actively sending/receiving
    FINISHED,     ///< Transfer finished successfully
    CANCELLED,    ///< Transfer was cancelled by user
    ERROR         ///< Transfer failed due to error or interruption
};

#endif // TRANSFERSTATUS_H
