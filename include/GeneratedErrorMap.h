// Generated automatically. DO NOT EDIT!

#pragma once
#include <array>
#include <cstdint>

enum class ErrorStrategy : uint8_t {
    Ignore = 0,
    Retry = 1,
    Abort = 2,
    Custom = 3,
};

enum class ErrorName : uint8_t {
    InvalidIP = 0,
};

 alignas(64) inline constexpr std::array<ErrorStrategy, 128> errno_strategies = {
    ErrorStrategy::Ignore, // 0: Success
    ErrorStrategy::Abort, // 1: Operation not permitted 
    ErrorStrategy::Ignore, // 2: No such file or directory
    ErrorStrategy::Abort, // 3: No such process 
    ErrorStrategy::Retry, // 4: Interrupted system call 
    ErrorStrategy::Abort, // 5: Input/output error 
    ErrorStrategy::Ignore, // 6: No such device or address
    ErrorStrategy::Ignore, // 7: Argument list too long
    ErrorStrategy::Ignore, // 8: Exec format error
    ErrorStrategy::Ignore, // 9: Bad file descriptor
    ErrorStrategy::Ignore, // 10: No child processes
    ErrorStrategy::Retry, // 11: Resource temporarily unavailable 
    ErrorStrategy::Abort, // 12: Cannot allocate memory 
    ErrorStrategy::Abort, // 13: Permission denied 
    ErrorStrategy::Ignore, // 14: Bad address
    ErrorStrategy::Ignore, // 15: Block device required
    ErrorStrategy::Ignore, // 16: Device or resource busy
    ErrorStrategy::Ignore, // 17: File exists
    ErrorStrategy::Ignore, // 18: Invalid cross-device link
    ErrorStrategy::Ignore, // 19: No such device
    ErrorStrategy::Ignore, // 20: Not a directory
    ErrorStrategy::Ignore, // 21: Is a directory
    ErrorStrategy::Abort, // 22: Invalid argument 
    ErrorStrategy::Ignore, // 23: Too many open files in system
    ErrorStrategy::Ignore, // 24: Too many open files
    ErrorStrategy::Ignore, // 25: Inappropriate ioctl for device
    ErrorStrategy::Ignore, // 26: Text file busy
    ErrorStrategy::Ignore, // 27: File too large
    ErrorStrategy::Abort, // 28: No space left on device 
    ErrorStrategy::Ignore, // 29: Illegal seek
    ErrorStrategy::Ignore, // 30: Read-only file system
    ErrorStrategy::Ignore, // 31: Too many links
    ErrorStrategy::Abort, // 32: Broken pipe 
    ErrorStrategy::Ignore, // 33: Numerical argument out of domain
    ErrorStrategy::Ignore, // 34: Numerical result out of range
    ErrorStrategy::Ignore, // 35: Resource deadlock avoided
    ErrorStrategy::Ignore, // 36: File name too long
    ErrorStrategy::Ignore, // 37: No locks available
    ErrorStrategy::Ignore, // 38: Function not implemented
    ErrorStrategy::Ignore, // 39: Directory not empty
    ErrorStrategy::Ignore, // 40: Too many levels of symbolic links
    ErrorStrategy::Ignore, // 41: Unknown error 41
    ErrorStrategy::Ignore, // 42: No message of desired type
    ErrorStrategy::Ignore, // 43: Identifier removed
    ErrorStrategy::Ignore, // 44: Channel number out of range
    ErrorStrategy::Ignore, // 45: Level 2 not synchronized
    ErrorStrategy::Ignore, // 46: Level 3 halted
    ErrorStrategy::Ignore, // 47: Level 3 reset
    ErrorStrategy::Ignore, // 48: Link number out of range
    ErrorStrategy::Ignore, // 49: Protocol driver not attached
    ErrorStrategy::Ignore, // 50: No CSI structure available
    ErrorStrategy::Ignore, // 51: Level 2 halted
    ErrorStrategy::Ignore, // 52: Invalid exchange
    ErrorStrategy::Ignore, // 53: Invalid request descriptor
    ErrorStrategy::Ignore, // 54: Exchange full
    ErrorStrategy::Ignore, // 55: No anode
    ErrorStrategy::Ignore, // 56: Invalid request code
    ErrorStrategy::Ignore, // 57: Invalid slot
    ErrorStrategy::Ignore, // 58: Unknown error 58
    ErrorStrategy::Ignore, // 59: Bad font file format
    ErrorStrategy::Ignore, // 60: Device not a stream
    ErrorStrategy::Ignore, // 61: No data available
    ErrorStrategy::Ignore, // 62: Timer expired
    ErrorStrategy::Ignore, // 63: Out of streams resources
    ErrorStrategy::Ignore, // 64: Machine is not on the network
    ErrorStrategy::Ignore, // 65: Package not installed
    ErrorStrategy::Ignore, // 66: Object is remote
    ErrorStrategy::Ignore, // 67: Link has been severed
    ErrorStrategy::Ignore, // 68: Advertise error
    ErrorStrategy::Ignore, // 69: Srmount error
    ErrorStrategy::Ignore, // 70: Communication error on send
    ErrorStrategy::Ignore, // 71: Protocol error
    ErrorStrategy::Ignore, // 72: Multihop attempted
    ErrorStrategy::Ignore, // 73: RFS specific error
    ErrorStrategy::Ignore, // 74: Bad message
    ErrorStrategy::Ignore, // 75: Value too large for defined data type
    ErrorStrategy::Ignore, // 76: Name not unique on network
    ErrorStrategy::Ignore, // 77: File descriptor in bad state
    ErrorStrategy::Ignore, // 78: Remote address changed
    ErrorStrategy::Ignore, // 79: Can not access a needed shared library
    ErrorStrategy::Ignore, // 80: Accessing a corrupted shared library
    ErrorStrategy::Ignore, // 81: .lib section in a.out corrupted
    ErrorStrategy::Ignore, // 82: Attempting to link in too many shared libraries
    ErrorStrategy::Ignore, // 83: Cannot exec a shared library directly
    ErrorStrategy::Ignore, // 84: Invalid or incomplete multibyte or wide character
    ErrorStrategy::Ignore, // 85: Interrupted system call should be restarted
    ErrorStrategy::Ignore, // 86: Streams pipe error
    ErrorStrategy::Ignore, // 87: Too many users
    ErrorStrategy::Ignore, // 88: Socket operation on non-socket
    ErrorStrategy::Ignore, // 89: Destination address required
    ErrorStrategy::Ignore, // 90: Message too long
    ErrorStrategy::Ignore, // 91: Protocol wrong type for socket
    ErrorStrategy::Ignore, // 92: Protocol not available
    ErrorStrategy::Ignore, // 93: Protocol not supported
    ErrorStrategy::Ignore, // 94: Socket type not supported
    ErrorStrategy::Ignore, // 95: Operation not supported
    ErrorStrategy::Ignore, // 96: Protocol family not supported
    ErrorStrategy::Ignore, // 97: Address family not supported by protocol
    ErrorStrategy::Ignore, // 98: Address already in use
    ErrorStrategy::Ignore, // 99: Cannot assign requested address
    ErrorStrategy::Ignore, // 100: Network is down
    ErrorStrategy::Ignore, // 101: Network is unreachable
    ErrorStrategy::Ignore, // 102: Network dropped connection on reset
    ErrorStrategy::Ignore, // 103: Software caused connection abort
    ErrorStrategy::Retry, // 104: Connection reset by peer 
    ErrorStrategy::Ignore, // 105: No buffer space available
    ErrorStrategy::Ignore, // 106: Transport endpoint is already connected
    ErrorStrategy::Ignore, // 107: Transport endpoint is not connected
    ErrorStrategy::Ignore, // 108: Cannot send after transport endpoint shutdown
    ErrorStrategy::Ignore, // 109: Too many references: cannot splice
    ErrorStrategy::Ignore, // 110: Connection timed out
    ErrorStrategy::Ignore, // 111: Connection refused
    ErrorStrategy::Ignore, // 112: Host is down
    ErrorStrategy::Ignore, // 113: No route to host
    ErrorStrategy::Ignore, // 114: Operation already in progress
    ErrorStrategy::Ignore, // 115: Operation now in progress
    ErrorStrategy::Ignore, // 116: Stale file handle
    ErrorStrategy::Ignore, // 117: Structure needs cleaning
    ErrorStrategy::Ignore, // 118: Not a XENIX named type file
    ErrorStrategy::Ignore, // 119: No XENIX semaphores available
    ErrorStrategy::Ignore, // 120: Is a named type file
    ErrorStrategy::Ignore, // 121: Remote I/O error
    ErrorStrategy::Ignore, // 122: Disk quota exceeded
    ErrorStrategy::Ignore, // 123: No medium found
    ErrorStrategy::Ignore, // 124: Wrong medium type
    ErrorStrategy::Ignore, // 125: Operation canceled
    ErrorStrategy::Ignore, // 126: Required key not available
    ErrorStrategy::Ignore, // 127: Key has expired
};
 alignas(64) inline constexpr std::array<ErrorStrategy, 1> error_strategies = {
    ErrorStrategy::Abort};
