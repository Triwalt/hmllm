# FeiQ / IPMSG Protocol Reference

## Core Concepts

- Transport: UDP over IPv4, default port 2425; broadcasts use subnet broadcast addresses.
- Datagram format: `VERSION:PACKET_NO:SENDER_NAME:SENDER_HOST:COMMAND:ADDITIONAL` encoded as UTF-8.
- Packet number: 32-bit unsigned integer; unique per sender and incrementing.
- Command field: lower 8 bits indicate base command, higher bits encode options.

## Base Commands

| Command Name | Code (hex) | Direction | Purpose |
| ------------ | ---------- | --------- | ------- |
| BR_ENTRY | 0x00000001 | Broadcast | Announce presence when coming online. |
| ANS_ENTRY | 0x00000003 | Unicast | Reply with presence details to a BR_ENTRY. |
| BR_EXIT | 0x00000002 | Broadcast | Notify peers that the user is going offline. |
| BR_ABSENCE | 0x00000004 | Broadcast | Announce absence status (e.g., away message). |
| SENDMSG | 0x00000020 | Unicast | Deliver a chat message, optional features via options. |
| RECVMSG | 0x00000021 | Unicast | Acknowledge receipt of a SENDMSG when SENDCHECK option is set. |
| READMSG | 0x00000030 | Unicast | Indicate message was read; FeiQ uses as delivery/read receipt. |
| DELMSG | 0x00000031 | Unicast | Request deletion of queued messages (rarely used). |
| GETINFO | 0x00000040 | Unicast | Request detailed user information (status, signature). |
| SENDINFO | 0x00000041 | Unicast | Response to GETINFO containing requested data. |
| GET_ABSENCE_INFO | 0x00000050 | Unicast | Request predefined absence messages. |
| SEND_ABSENCE_INFO | 0x00000051 | Unicast | Provide absence message content. |

## Option Flags

| Option Name | Code (hex) | Meaning |
| ----------- | ---------- | ------- |
| SENDCHECK | 0x00000100 | Sender expects RECVMSG acknowledgment. |
| SECRETOPT | 0x00000200 | Message should not be displayed automatically. |
| BROADCASTOPT | 0x00000400 | Message is broadcast to all peers. |
| ABSENCEOPT | 0x00000800 | Sender is in absence mode (away). |
| SERVEROPT | 0x00001000 | Indicates a host acting as message proxy. |
| DIALUPOPT | 0x00002000 | Sender is on dial-up; avoid large transfers. |
| FILEATTACHOPT | 0x00200000 | Additional section contains file descriptors (IPMSG_FILEATTACH). |
| ENCRYPTOPT | 0x00400000 | Payload is encrypted (FeiQ rarely uses). |

## Additional Field Encoding

The `ADDITIONAL` segment uses `\a` (bell) as a delimiter inside the string.

- Presence (BR_ENTRY/ANS_ENTRY): `username\ahost\aversion\atimestamp[\aext_fields]` in FeiQ.
- Chat messages (SENDMSG): `message_text[\aext_data]`. FeiQ puts plain text first, then optional metadata.
- File attachments: appended after message, encoded per `IPMSG_FILELIST`. Each entry: `file_id:file_name:file_size:mtime:file_attrs` separated by `\a`.

FeiQ-specific extensions include:

- UTF-8 content by default; older IPMSG used Shift-JIS/GBK.
- Extended user profile: `nickname`, `signature`, `mac_address`, `avatar_hash` embedded in ext_fields.
- Group chat: FeiQ encodes group identifier in ext_fields with key-value pairs like `groupid=...`.

## Behaviour Expectations

1. **Presence Handling**
   - On startup: send `BR_ENTRY` to all broadcast addresses.
   - On receipt of `BR_ENTRY`: respond with `ANS_ENTRY` unicast; update or insert user record.
   - Periodic keepalive: resend `BR_ENTRY` every ~60 seconds (FeiQ default). Send `BR_EXIT` before shutdown.

2. **Messaging**
   - `SENDMSG` includes options for acknowledgments (`SENDCHECK`) and attachments (`FILEATTACHOPT`).
   - Receivers must send `RECVMSG` when `SENDCHECK` set, echoing original packet number in `ADDITIONAL`.
   - `READMSG` optionally sent after displaying to user; FeiQ uses it for delivery receipts.

3. **Absence / Status**
   - Absence mode advertised via `ABSENCEOPT` flag and optional absence message appended.
   - Detailed status queries use `GETINFO`/`SENDINFO` round trip.

4. **Files**
   - File transfer is negotiated via `SENDMSG` + `FILEATTACHOPT`; FeiQ then opens a separate TCP port (default 2425 or negotiated) for the actual transfer protocol.
   - Attachment descriptors inform peer about name, size, modify time, permissions.

## Mapping Targets for Kylin Messenger

| Feature | Current Implementation | Target IPMSG Approach |
| ------- | ---------------------- | --------------------- |
| Presence broadcast | Custom `NetworkPacket::createPresencePacket` over UDP | Replace with `BR_ENTRY`/`ANS_ENTRY`/`BR_EXIT` handling, remove custom header. |
| Reliable messaging | TCP sockets with custom framing | Use UDP `SENDMSG` with `SENDCHECK` + optional delivery tracking; rely on IPMSG semantics. |
| Typing indicators | Custom `TypingIndicator` message type | Not part of IPMSG; needs optional FeiQ extension or remove until mapped to supported feature. |
| File share offer | Custom `FileOffer` packet | Encode using `FILEATTACHOPT` descriptors and follow FeiQ transfer handshake. |
| ProtoBuf support | Optional | Remove; IPMSG uses plain-text payloads. |

## Next Steps

1. Replace UDP/TCP handling code with IPMSG-specific packet flow.
2. Map `UserInfo` serialization/deserialization to FeiQ fields.
3. Align message sending/receiving with `SENDMSG`/`RECVMSG` handshake.
4. Introduce file descriptor parsing for attachments in later stage.
